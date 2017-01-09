package com.sprd.telecom.logEmergencyNumberPlugin;

import android.app.AddonManager;
import android.content.Context;

import com.sprd.server.telecom.LogEmergencyNumbersHelper;

/**
 * Allow emergency number in call log feature.
 */
public class LogEmergencyNumbersPlugin extends LogEmergencyNumbersHelper
        implements AddonManager.InitialCallback {

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public LogEmergencyNumbersPlugin() {
    }

    public boolean okToLogEmergencyNumber(Context context) {
        return true;
    }
}
