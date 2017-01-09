package com.android.callsettings.plugins;

import android.app.AddonManager;
import android.util.Log;
import android.content.Context;
import com.android.callsettings.R;

public class CallSettingsCMCCHelper {

    private static final String TAG = "CallSettingsCMCCHelper";
    static CallSettingsCMCCHelper sInstance;

    public static CallSettingsCMCCHelper getInstance(Context context) {
        Log.d(TAG, "enter CallSettingsCMCCHelper");
        if (sInstance != null) return sInstance;
        AddonManager addonManager = new AddonManager(context);
        sInstance = (CallSettingsCMCCHelper) addonManager.getAddon(
                R.string.callsettings_cmcc_plugin, CallSettingsCMCCHelper.class);
        return sInstance;
    }

    public CallSettingsCMCCHelper() {
    }

    /**
     * CMCC new case : allow user to set call time forward in 3g2g ; see
     * bug552776
     */
    public boolean isCallTimeForwardSupportIn3g2g() {
        Log.d(TAG, "isCallTimeForwardSupport");
        return false;
    }

    /**
     * CMCC new case : disable the call out barring option in volte ; see
     * bug590100
     */
    public boolean isCallOutOptionEnableInVolte() {
        Log.d(TAG, "isCallOutOptionEnableInVolte true");
        return true;
    }

    /**
     * CMCC new case : remove videoCallForward;
     * see bug 611966
     */
    public boolean isVideoCallForwardSupport() {
        Log.d(TAG, "isVideoCallForwardSupport");
        return true;
    }
}
