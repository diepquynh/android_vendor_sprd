package com.android.server.am;

import android.app.ActivityManagerNative;
import android.content.Context;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.text.TextUtils;
import android.util.Slog;
import android.view.inputmethod.InputMethodInfo;

import com.android.internal.view.IInputMethodManager;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

public class LmKillerTracker extends Thread {
    private final static String TAG = "LmKillerTracker";
    private static final String LMK_TRACKER_SOCKET = "lmfs";
    private ActivityManagerService mService;
    private boolean mConnected;
    private final List<String> mLmKillerBypassPackages = new ArrayList<String>();

    public LmKillerTracker() {
        mService = (ActivityManagerService) ActivityManagerNative.getDefault();
        mLmKillerBypassPackages.add("com.android");
        mLmKillerBypassPackages.add("com.google");
        mLmKillerBypassPackages.add("com.quicinc.vellamo");
    }

    @Override
    public void run() {
        int retryCount = 0;
        LocalSocket lmkillerSocket = null;
        mConnected = false;
        try {
            while(true) {
                LocalSocket s = null;
                LocalSocketAddress lSAddr;
                try {
                    s = new LocalSocket();
                    lSAddr = new LocalSocketAddress(LMK_TRACKER_SOCKET,
                            LocalSocketAddress.Namespace.RESERVED);
                    s.connect(lSAddr);
                } catch (IOException e) {
                    mConnected = false;
                    try {
                        if (s != null) {
                            s.close();
                        }
                    } catch (Exception e2) {
                    }

                    if (retryCount == 8) {
                        Slog.e(TAG, "can't find lmkiller socket after "
                                + retryCount + " retry, abort LmkTracker");
                        return;
                    } else if ( retryCount >= 0 && retryCount < 8) {
                        Slog.d(TAG, "retrying " + retryCount);
                    }

                    try {
                        Thread.sleep(500);
                    } catch (InterruptedException er) {
                    }
                    retryCount++;
                    continue;
                }
                retryCount = 0;
                lmkillerSocket = s;
                Slog.i(TAG, "connected to lmkiller");
                mConnected = true;
                try {
                    InputStream is = lmkillerSocket.getInputStream();
                    BufferedReader reader = new BufferedReader(new InputStreamReader(is));
                    while (true) {
                        String line = reader.readLine();
                        try {
                            int pid = Integer.parseInt(line);
                            doLmkForceStop(pid);
                        } catch (Exception e) {
                            Slog.e(TAG, "doLmkForceStop encounter exception, read line "
                                    + line, e);
                        }
                    }
                } catch (Exception e) {
                    Slog.e(TAG, "caugth exception, closing lmk tracker", e);
                }

                try{
                    if (lmkillerSocket != null) lmkillerSocket.close();
                } catch (IOException e) {
                }
            }
        } catch (Exception e) {
        }
    }

    public void doLmkForceStop(int pid) {
        Slog.d(TAG, "doLmkForceStop pid=" + pid);
        if (pid <= 0) {
            return;
        }
        String pkgName = null;
        ProcessRecord pr = null;
        ProcessRecord parent = null;
        try {
            synchronized (mService.mPidsSelfLocked) {
                pr = mService.mPidsSelfLocked.get(pid);
            }

            if (pr == null) return;
            if (pr.info != null) {
                if (TextUtils.isEmpty(pr.processName)) return;
                if (pr.processName.indexOf(":") > 0) {
                    synchronized (mService) {
                        parent = mService.getProcessRecordLocked(
                                pr.info.packageName, pr.uid, true);
                    }
                }
                pkgName = pr.info.packageName;
            }
        } catch (Exception e) {
            Slog.w(TAG, "get process record from ams failed", e);
        }

        if (pkgName == null) return;

        // Found nothing, no processes found, abort directly.
        if (pr == null && parent == null) return;

        // Current process record can not be stopped.
        if (pr != null && !shouldForceStop(pr)) return;

        // package process record can not be stopped.
        if (parent != null && !shouldForceStop(parent)) return;

        Slog.i(TAG, "force stop pkg:" + pkgName + ", pid:" + pid
                + " (adj " + pr.setAdj + ")");
        mService.forceStopPackage(pkgName, UserHandle.USER_CURRENT);
    }

    public boolean isConnected() {
        return mConnected;
    }

    private boolean hasActivityInTopTask(ProcessRecord pr) {
        ActivityStack mainStack = mService.mStackSupervisor.getFocusedStack();
        TaskRecord topTask = (mainStack != null)?mainStack.topTask():null;
        if (topTask == null) return false;

        List<ActivityRecord> activities = pr.activities;

        for(ActivityRecord ar:activities) {
            if (ar.task == topTask) return true;
        }

        return false;
    }

    private boolean hasRelativeToptaskPackageInProcess(ProcessRecord pr) {
        ActivityStack mainStack = mService.mStackSupervisor.getFocusedStack();
        TaskRecord topTask = (mainStack != null)?mainStack.topTask():null;
        if (topTask == null) return false;

        // TODO, that may not safe.
        List<ActivityRecord> topActivities = topTask.mActivities;
        Set<String> pkgRunningInProcess = pr.pkgList.keySet();

        for(String pkg: pkgRunningInProcess) {
            for(ActivityRecord ar: topActivities) {
                String arPkg = ar.packageName;
                if(arPkg.equals(pkg)) return true;
            }
        }
        return false;
    }

    private boolean isSystemApp(ProcessRecord pr) {
        if (pr.info.isPrivilegedApp() || pr.info.isSystemApp()
                || pr.info.isUpdatedSystemApp()) {
            return true;
        }
        return false;
    }

    private boolean isEnabledInputMethod(String pkgName){
        if (pkgName == null) return false;
        IBinder b = ServiceManager.getService(Context.INPUT_METHOD_SERVICE);
        IInputMethodManager service = IInputMethodManager.Stub.asInterface(b);
        List<InputMethodInfo> inputMethods;
        try {
            inputMethods = service.getEnabledInputMethodList();
        } catch (RemoteException e) {
            return false;
        }
        if (inputMethods == null || inputMethods.size() == 0) return false;
        for (InputMethodInfo info : inputMethods){
            if (info == null || info.getPackageName() == null) continue;
            if (info.getPackageName().equals(pkgName)) return true;
        }
        return false;
    }

    private boolean isInstrumentedApp(ProcessRecord pr) {
        String pkgName = pr.info.packageName;

        if (pr.instrumentationInfo != null) return true;

        for (String token : mLmKillerBypassPackages) {
            if (pkgName.startsWith(token)) return true;
        }
        return false;
    }

    private boolean shouldForceStop(ProcessRecord pr) {
        String pkgName = pr.info.packageName;
        int pid = pr.pid;
        if (hasActivityInTopTask(pr)) {
            Slog.i(TAG, "Skipped process pkgName:" + pkgName + ", pid:" + pid +
                    ", has activity in top task !");
            return false;
        } else if (hasRelativeToptaskPackageInProcess(pr)) {
            Slog.i(TAG, "Skipped process pkgName:" + pkgName + ", pid:" + pid +
                    ", has relative top task package !");
            return false;
        } else if (isSystemApp(pr)) {
            Slog.i(TAG, "Skipped process pkgName:" + pkgName + ", pid:" + pid +
                    ", is system app !");
            return false;
        } else if (isInstrumentedApp(pr)) {
            Slog.i(TAG, "Skipped process pkgName:" + pkgName + ", pid:" + pid +
                    ", the application being instrumented !");
            return false;
        } else if (isEnabledInputMethod(pkgName)) {
            Slog.i(TAG, "Skipped process pkgName:" + pkgName + ", pid:" + pid +
                    ", the application is EnabledInputMethod !");
            return false;
        }
        return true;
    }

}
