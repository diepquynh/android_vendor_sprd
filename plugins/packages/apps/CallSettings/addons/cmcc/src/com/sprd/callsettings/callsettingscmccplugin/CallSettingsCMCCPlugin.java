package com.sprd.callsettings.callsettingscmccplugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.callsettings.plugins.CallSettingsCMCCHelper;


/**
 * This class is used to manager Dialer CMCC Plugin
 */
public class CallSettingsCMCCPlugin extends CallSettingsCMCCHelper
        implements AddonManager.InitialCallback {
    private Context mContext;
    private static final String TAG = "CallSettingsCMCCPlugin";

    public CallSettingsCMCCPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        Log.i(TAG,"clazz: " + clazz);
        mContext = context;
        return clazz;
    }

    @Override 
    public boolean isCallTimeForwardSupportIn3g2g() {
        Log.d(TAG, "isCallTimeForwardSupport");
        return true;
    }

    /* SPRD: fix bug590100 @{ */
    @Override
    public boolean isCallOutOptionEnableInVolte() {
        Log.d(TAG, "isCallOutOptionEnableInVolte false");
        return false;
    }
    /* @} */

    /**
     * CMCC new case : remove videoCallForward;
     * see bug 611966
     */
    public boolean isVideoCallForwardSupport() {
        Log.d(TAG, "isVideoCallForwardSupport");
        return false;
    }

}
