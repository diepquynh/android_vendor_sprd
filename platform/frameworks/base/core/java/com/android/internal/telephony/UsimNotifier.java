package com.android.internal.telephony;

import android.app.Activity;
import android.app.AddonManager;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.res.Resources;

import com.android.internal.R;

public class UsimNotifier {
    private static boolean sLoaded = false;
    private static boolean sEnabled = false;

    private static UsimNotifier sInstance;

    public static boolean isNotifyEnabled(Context context) {
        // To avoid duplicate query because of resume
        if (sLoaded) return sEnabled;

        if (context != null) {
            sLoaded = true;
            if (context.getResources() == null) return sEnabled;
            String[] usimWorkaroundPackages = context.getResources()
                    .getStringArray(R.array.usim_notify_packages);
            if (usimWorkaroundPackages != null && usimWorkaroundPackages.length > 0) {
                ApplicationInfo appInfo = context.getApplicationInfo();
                String packageName = null;
                if (appInfo != null) {
                    packageName = appInfo.packageName;
                }
                for (String workaroundPackage : usimWorkaroundPackages) {
                    if (null != workaroundPackage) {
                        if (workaroundPackage.equals(packageName)) {
                            sEnabled = true;
                            break;
                        }
                    }
                }
            }
        }
        return sEnabled;
    }

    public static UsimNotifier getInstance() {
        if (sInstance == null) {
            sInstance = (UsimNotifier) AddonManager.getDefault().getAddon(
                    R.string.addon_usim_notify, UsimNotifier.class);
            // Should not happen
            if (sInstance == null) {
                sInstance = new UsimNotifier();
            }
        }
        return sInstance;
    }

    public void startNotify(Activity activity) {
        // Do nothing
    }
}
