package com.sprd.dialer.plugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.widget.TextView;

import com.sprd.dialer.DialerPlugin;

/**
 * Various utilities for dealing with phone number strings.
 */
public class AddonDialerPlugin extends DialerPlugin implements AddonManager.InitialCallback
{
    private Context mAddonContext;
    private static final String TAG = "[CMCC::AddonDialerPlugin]";

    public AddonDialerPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean callLogDetailShowDuration() {
        log("callLogDetail_showDuration");
        return false;
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }
}
