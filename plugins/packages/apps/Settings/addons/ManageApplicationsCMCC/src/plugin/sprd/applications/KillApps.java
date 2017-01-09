/** Create by Spreadst */
package plugin.sprd.applications;

import android.app.ActivityManager;
import android.util.Log;
import android.widget.Toast;
import android.content.res.Resources;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ServiceInfo;
import android.os.Debug;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.android.settings.R;

public class KillApps {
    private static final String TAG = "KillApps";

    static final int MAX_SERVICES = 100;
    static final int CHECK_APP = 0;
    static final int CHECK_SERVICE = 1;

    private String[] mKeepApp;
    private String[] mKeepService;
    private ActivityManager am;
    private PackageManager mpm;
    private Context mContext;

    public KillApps(Context context, ActivityManager am, PackageManager pm) {
        mContext = context;
        final Resources resources = mContext.getResources();
        mKeepApp = resources.getStringArray(R.array.recent_keep_app);
        mKeepService = resources.getStringArray(R.array.recent_keep_app);
        this.am = am;
        this.mpm = pm;
    }

    /**
     * keep the app that we want to keep
     *
     */
    private boolean checkKeepAppOrService(String Pkg, int flag) {
        boolean rth = false;
        switch (flag) {
        case CHECK_APP:
            if (mKeepApp != null) {
                for (String pkg : mKeepApp) {
                    if (pkg != null && pkg.equals(Pkg)) {
                        rth = true;
                    }
                }
            }
            break;
        case CHECK_SERVICE:
            if (mKeepService != null) {
                for (String pkg : mKeepService) {
                    if (pkg != null && pkg.equals(Pkg)) {
                        rth = true;
                    }
                }
            }
            break;
        }
        return rth;
    }

    /**
     * get running app
     *
     */
    private List<ApplicationInfo> getRunningApp() {
        List<ApplicationInfo> appList = new ArrayList<ApplicationInfo>();
        List<ActivityManager.RunningAppProcessInfo> procList = getRunningAppProcessesList();
        if ((procList == null) || (procList.size() == 0)) {
            return appList;
        }
        // Retrieve running processes from ActivityManager
        for (ActivityManager.RunningAppProcessInfo appProcInfo : procList) {
            if ((appProcInfo != null) && (procList != null)) {
                int pkgsize = appProcInfo.pkgList.length;
                for (int j = 0; j < pkgsize; j++) {
                    ApplicationInfo appInfo = null;
                    try {
                        appInfo = mpm.getApplicationInfo(appProcInfo.pkgList[j],
                                PackageManager.GET_UNINSTALLED_PACKAGES);
                    } catch (NameNotFoundException e) {
                        Log.w(TAG, "Error retrieving ApplicationInfo for pkg:" + appProcInfo.pkgList[j]);
                        continue;
                    }
                    if (appInfo != null
                            && !checkKeepAppOrService(appInfo.packageName,
                                    CHECK_APP)) {
                        appList.add(appInfo);
                    }
                }
            }
        }
        return appList;
    }

    private List<ActivityManager.RunningAppProcessInfo> getRunningAppProcessesList() {
        return am.getRunningAppProcesses();
    }

    /**
     * kill app
     *
     */
    public void killApp() {
        List<ApplicationInfo> allRunningAppList = getRunningApp();
        for (ApplicationInfo appInfo : allRunningAppList) {
            Log.d(TAG, "KillApps & the names of be killed apps:" + "****** " + appInfo.packageName + " ******");
            am.forceStopPackage(appInfo.packageName);
        }
    }

    /**
     * get service who will be killed
     *
     */
    public List<ActivityManager.RunningServiceInfo> getService() {
        List<ActivityManager.RunningServiceInfo> services = am
                .getRunningServices(MAX_SERVICES);
        List<ActivityManager.RunningServiceInfo> mServices = new ArrayList<ActivityManager.RunningServiceInfo>();
        final int NS = services != null ? services.size() : 0;
        for (int i = 0; i < NS; i++) {
            // We likewise don't care about services running in a
            // persistent process like the system or phone.
            if ((services.get(i).flags & ActivityManager.RunningServiceInfo.FLAG_PERSISTENT_PROCESS) != 0
                    || (services.get(i).flags & ActivityManager.RunningServiceInfo.FLAG_SYSTEM_PROCESS) != 0
                    || checkKeepAppOrService(services.get(i).service.getPackageName(), CHECK_SERVICE)) {
                continue;
            }
            mServices.add(services.get(i));
        }
        return mServices;
    }

    /**
     * kill service
     *
     */
    public void killService() {
        List<ActivityManager.RunningServiceInfo> allRunningServiceList = getService();
        int size = allRunningServiceList.size();
        for (int i = 0; i < size; i++) {
            Log.d(TAG, "KillApps & the names of be killed services:"+ "****** "
                    + allRunningServiceList.get(i).service.getPackageName()
                    + " ******");
            try {
                mContext.stopService(new Intent().setComponent(allRunningServiceList
                        .get(i).service));
            } catch (SecurityException exception) {
                Log.w(TAG, "can't stopService : " + allRunningServiceList.get(i).service.getClassName()
                        + "for current user : " + android.os.UserHandle.myUserId()
                        + ", for reason of " + exception);
            }
        }
    }

}
