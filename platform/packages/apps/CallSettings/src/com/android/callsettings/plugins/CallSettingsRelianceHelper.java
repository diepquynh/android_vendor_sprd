package com.android.callsettings.plugins;

import android.app.AddonManager;
import android.util.Log;
import android.content.Context;
import com.android.callsettings.fastdial.FastDialManager;
import com.android.callsettings.R;

public class CallSettingsRelianceHelper {

    private static final String TAG = "CallSettingsRelianceHelper";
    static CallSettingsRelianceHelper sInstance;

    public static CallSettingsRelianceHelper getInstance() {
        Log.d(TAG, "enter CallSettingsRelianceHelper");
        if (sInstance != null) return sInstance;
        sInstance = (CallSettingsRelianceHelper) AddonManager.getDefault().getAddon(
                R.string.callsettings_reliance_plugin, CallSettingsRelianceHelper.class);
        return sInstance;
    }

    public CallSettingsRelianceHelper() {
    }

    public void initEmergencyNumber(FastDialManager manager) {
        Log.d(TAG, "initEmergencyNumber");
    }

    public boolean canEditFastDialNumber(int fastDialIndex) {
        Log.d(TAG, "canEditFastDialNumber");
        return true;
    }
}
