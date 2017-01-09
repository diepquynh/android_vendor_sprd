package com.sprd.dialer;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

import com.sprd.dialer.plugins.DialerRelianceHelper;

public class DialerReliancePlugin extends DialerRelianceHelper
        implements AddonManager.InitialCallback {
    private static final String TAG = "DialerReliancePlugin";
    private static final boolean DBG = true;

    public DialerReliancePlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        Log.i(TAG , "clazz: " + clazz);
        return clazz;
    }

    public boolean isMultiCallDisabledByOperator() {
        Log.i(TAG , "isMultiCallDisableByOperator = true");
        return true;
    }
}
