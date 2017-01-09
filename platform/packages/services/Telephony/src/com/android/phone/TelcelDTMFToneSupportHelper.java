package com.android.phone;

import android.app.AddonManager;
import android.content.Context;

import com.android.phone.R;

/**
 * Created by sprd on 1/17/17.
 */
public class TelcelDTMFToneSupportHelper {

    private static TelcelDTMFToneSupportHelper sInstance;

    public void TelcelDTMFToneSupportHelper () {
    }

    public static TelcelDTMFToneSupportHelper getInstance (Context context) {
        if (sInstance == null) {
            synchronized (TelcelDTMFToneSupportHelper.class) {
                AddonManager addonManager = new AddonManager(context);
                sInstance = (TelcelDTMFToneSupportHelper) addonManager.getAddon(R.string.feature_telcel_dtmftone_support_plugin,
                        TelcelDTMFToneSupportHelper.class);
            }
        }

        return sInstance;
    }

    public boolean isSupportDTMFTone () {
        return true;
    }
}
