package com.sprd.contacts.common.plugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.contacts.common.R;

public class EFDisplaySupportUtils {

    private static EFDisplaySupportUtils sInstance;

    public static EFDisplaySupportUtils getInstance() {
        if (sInstance != null) {
            return sInstance;
        }
        sInstance = (EFDisplaySupportUtils) AddonManager.getDefault().getAddon(
                R.string.feature_ef_display_support, EFDisplaySupportUtils.class);
        Log.d("EFDisplaySupportUtils", "sInstance: " + sInstance.hashCode());
        return sInstance;
    }

    public EFDisplaySupportUtils() {
    }

    public boolean isEFDisplaySupport() {
        return false;
    }
}