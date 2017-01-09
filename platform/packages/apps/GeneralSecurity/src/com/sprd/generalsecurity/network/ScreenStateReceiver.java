package com.sprd.generalsecurity.network;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.AppGlobals;
import android.app.KeyguardManager;
import android.app.LoaderManager.LoaderCallbacks;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.Loader;
import android.content.pm.ApplicationInfo;
import android.content.pm.IPackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;

import android.net.TrafficStats;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.os.UserManager;
import android.preference.PreferenceManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;
import com.sprd.generalsecurity.R;
import com.sprd.generalsecurity.utils.DateCycleUtils;
import com.sprd.generalsecurity.utils.TeleUtils;
import com.sprd.generalsecurity.utils.Formatter;
import com.sprd.generalsecurity.utils.TeleUtils.NetworkType;
import java.lang.System;
import java.text.Collator;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;

import java.io.File;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

import android.app.KeyguardManager;
import com.sprd.generalsecurity.utils.Contract;

public class ScreenStateReceiver extends BroadcastReceiver {
    private static String TAG = "ScreenStateReceiver";
    private Context mContext;

    private static SCREEN_STATE lastMode = SCREEN_STATE.NONE;

    private enum SCREEN_STATE {
        NONE, SCREEN_OFF, SCREEN_ON, USER_PRESENT
    }

    private SharedPreferences mSharedPref;

    private long mBytesUsedInKeyguard;

    private static final String NETWORK_STAT_PATH = "/proc/uid_stat/";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();

        if (mSharedPref == null) {
            mSharedPref = PreferenceManager.getDefaultSharedPreferences(context);
        }
        KeyguardManager keyManager = (KeyguardManager) mContext.getSystemService(Context.KEYGUARD_SERVICE);

        if (Intent.ACTION_SCREEN_ON.equals(action)) {
            KeyguardManager keyguardManager =
                (KeyguardManager) context.getSystemService(Context.KEYGUARD_SERVICE);
            if (keyguardManager.isKeyguardLocked()) {
                FloatKeyView.getInstance(context).hide();
            } else {
                //keyguard on, should show speed if user set
                if (getSpeedSetting(context)) {
                    FloatKeyView.getInstance(context).startRealSpeed();
                    FloatKeyView.getInstance(context).show();
                }
            }
            lastMode = SCREEN_STATE.SCREEN_ON;
        } else if (Intent.ACTION_SCREEN_OFF.equals(action)) {
            if (lastMode == SCREEN_STATE.USER_PRESENT || lastMode == SCREEN_STATE.NONE) {
                lastMode = SCREEN_STATE.SCREEN_OFF;

                Thread t = new Thread(new Runnable() {
                    @Override
                    public void run() {
                        getLockScreenDataUsage(mUsageList);
                    }
                });

                t.start();
            }

            //stop real speed view when screen off.
            FloatKeyView.getInstance(context).hide();
            FloatKeyView.getInstance(context).stopRealSpeed();
        } else if (Intent.ACTION_USER_PRESENT.equals(action) && !keyManager.isKeyguardLocked()) {
            lastMode = SCREEN_STATE.USER_PRESENT;
            if (getSpeedSetting(context)) {
                FloatKeyView.getInstance(context).startRealSpeed();
                FloatKeyView.getInstance(context).show();
            }

            if (mSharedPref.getBoolean(Contract.KEY_LOCK_DATA_SWITCH, false)) {
                Thread t = new Thread(new Runnable() {
                    @Override
                    public void run() {
                        getLockScreenDataUsage(mLatestUsageList);
                        mBytesUsedInKeyguard = generateUsageReport();
                        notifyLockScreenDataUsed();
                    }
                });

                t.start();
            }
        }
    }

    boolean getSpeedSetting(Context context) {
        if (mSharedPref == null) {
            mSharedPref = PreferenceManager.getDefaultSharedPreferences(context);
        }
        if (mSharedPref.getBoolean("networkspeed_switch", false)) {
            return true;
        } else {
            return false;
        }
    }

    public void registerScreenReceiver(Context context) {
        mContext = context;
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(Intent.ACTION_USER_PRESENT);
        context.registerReceiver(this, filter);
    }

    public void unRegisterScreenReceiver(Context context) {
        context.unregisterReceiver(this);
    }

    void notifyLockScreenDataUsed() {
        NetworkType type = TeleUtils.getCurrentNetworkType(mContext);
        //intent to start DataFlowMainEntry when notification clicked.
        Intent resultIntent = new Intent(mContext, LockPeriodFlowActivity.class);
        resultIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        resultIntent.putExtra(LockPeriodFlowActivity.LOCKSCREEN_NETWORK_TYPE, type.ordinal());
        PendingIntent resultPendingIntent = PendingIntent.getActivity(mContext, 0, resultIntent, PendingIntent.FLAG_CANCEL_CURRENT);

        String msg = String.format(mContext.getResources().getString(R.string.data_used_lockscreen_message),
                Formatter.formatFileSize(mContext, mBytesUsedInKeyguard, false));
        Notification notification = new Notification.Builder(mContext)
                .setContentTitle(mContext.getResources().getString(R.string.data_used_lockscreen_title))
                .setAutoCancel(true)
                .setContentText(msg)
                .setContentIntent(resultPendingIntent)
                .setSmallIcon(R.drawable.lock_flow)
                .setStyle(new Notification.BigTextStyle().bigText(msg))
                .build();

        NotificationManager nm = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        nm.notify(1, notification);
    }

    public void getLockScreenDataUsage(ArrayList<AppUsageInfo> list) {
        File dir = new File(NETWORK_STAT_PATH);
        String[] children = dir.list();
        list.clear();

        try {
             for (String path: children) {
                long bytes = getTotalBytesManual(path);
                AppUsageInfo info = new AppUsageInfo();
                info.uid = Integer.valueOf(path);
                info.bytesUsed = bytes;
                list.add(info);
            }
        } catch(IOException e) {
            Log.e(TAG, "exception happened:" + e);
        }
    }

    private static ArrayList<AppUsageInfo> mUsageList = new ArrayList<AppUsageInfo>();
    private static ArrayList<AppUsageInfo> mLatestUsageList = new ArrayList<AppUsageInfo>();

    public class AppUsageInfo {
        int uid;
        String appName;
        long bytesUsed;
    }

    private Long getTotalBytesManual(String localUid) throws IOException{
        File uidFileDir = new File("/proc/uid_stat/"+ localUid);
        File uidActualFileReceived = new File(uidFileDir,"tcp_rcv");
        File uidActualFileSent = new File(uidFileDir,"tcp_snd");

        String textReceived = "0";
        String textSent = "0";
        BufferedReader brReceived = null, brSent = null;

        try {
                brReceived = new BufferedReader(new FileReader(uidActualFileReceived));
                brSent = new BufferedReader(new FileReader(uidActualFileSent));
                String receivedLine;
                String sentLine;

                if ((receivedLine = brReceived.readLine()) != null) {
                    textReceived = receivedLine;
                }
                if ((sentLine = brSent.readLine()) != null) {
                    textSent = sentLine;
                }
        } finally {
            brReceived.close();
            brSent.close();
        }

        return Long.valueOf(textReceived).longValue() + Long.valueOf(textSent).longValue();
    }

    public static ArrayList<AppUsageInfo> mReportList = new ArrayList<AppUsageInfo>();

    private long generateUsageReport() {
        long dataUsed = 0;
        mReportList.clear();
        for (AppUsageInfo info : mLatestUsageList) {
            for (AppUsageInfo infoPrevious: mUsageList) {
                if (infoPrevious.uid == info.uid) {
                    //uid matched, compute the usage used in lock period
                    long used = info.bytesUsed - infoPrevious.bytesUsed;
                    if (used > 0) {
                        AppUsageInfo reportInfoItem = new AppUsageInfo();
                        reportInfoItem.uid = info.uid;
                        reportInfoItem.bytesUsed = used;
                        mReportList.add(reportInfoItem);
                        dataUsed += used;
                        Log.d(TAG, "report added:" + reportInfoItem.uid + ":" + reportInfoItem.bytesUsed);
                    }
                    break;
                }
            }
        }

        Log.d(TAG, "final report used:" + dataUsed);

        return dataUsed;
    }

    public static void clearDataLists() {
        mReportList.clear();
        mUsageList.clear();
        mLatestUsageList.clear();
    }
}
