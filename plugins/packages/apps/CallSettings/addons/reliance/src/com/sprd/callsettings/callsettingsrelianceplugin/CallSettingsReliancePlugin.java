package com.sprd.callsettings.callsettingsrelianceplugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.callsettings.fastdial.FastDialManager;
import com.android.callsettings.plugins.CallSettingsRelianceHelper;


/**
 * This class is used to manager Dialer CMCC Plugin
 */
public class CallSettingsReliancePlugin extends CallSettingsRelianceHelper
        implements AddonManager.InitialCallback {
    private Context mContext;
    private static final String TAG = "CallSettingsReliancelPlugin";

    public CallSettingsReliancePlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        Log.i(TAG,"clazz: " + clazz);
        mContext = context;
        return clazz; 
    }

    @Override
    public void initEmergencyNumber(FastDialManager manager) {
        Log.d(TAG, "initEmergencyNumber");
        manager.saveFastDialNumber(9, "112", null, null, null, null, null);
    }

    @Override
    public boolean canEditFastDialNumber(int fastDialIndex) {
        Log.d(TAG, "canEditFastDialNumber");
        if (fastDialIndex == 9) {
            return false;
        }
        return true;
    }
}
