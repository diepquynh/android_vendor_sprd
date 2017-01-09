package com.sprd.dialer.dialerCmccPlugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.sprd.dialer.DialerCmccHelper;


/**
 * This class is used to manager Dialer CMCC Plugin
 */
public class DialerCmccPlugin extends DialerCmccHelper implements AddonManager.InitialCallback {
    private Context mContext;
    private static final String TAG = "DialerCmccPlugin";

    public DialerCmccPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        Log.i(TAG,"clazz: " + clazz);
        mContext = context;
        return clazz;
    }

    @Override
    public boolean showCallLogDetailDuration() {
        Log.i(TAG, "showCallLogDetailDuration");
        return false;
    }
}
