package com.sprd;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.sprd.incallui.ExplicitCallTransferPluginHelper;

/**
 * Various utilities for dealing with phone number strings.
 */
public class ExplicitCallTransferPlugin extends ExplicitCallTransferPluginHelper implements AddonManager.InitialCallback {
    private Context mAddonContext;
    private static final String TAG = "[ExplicitCallTransferPlugin]";

    public ExplicitCallTransferPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        mAddonContext = context;
        return clazz;
    }

    public boolean isExplicitCallTransferPlugin() {
        return true;
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }
}
