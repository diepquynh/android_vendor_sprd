package com.android.internal.telephony.plugin;

import com.android.internal.R;
import android.app.AddonManager;
import android.util.Log;

public class TelephonyForTelcelPluginsUtils {
    private static final String LOGTAG = "TelephonyForTelcelPluginsUtils";
    static TelephonyForTelcelPluginsUtils sInstance;

    public static TelephonyForTelcelPluginsUtils getInstance() {
        if (sInstance != null) return sInstance;
        Log.d(LOGTAG, "TelephonyForTelcelPluginsUtils getInstance");
        sInstance = (TelephonyForTelcelPluginsUtils) AddonManager.getDefault().getAddon(R.string.feature_for_Telcel, TelephonyForTelcelPluginsUtils.class);
        return sInstance;
    }

    public TelephonyForTelcelPluginsUtils() {
    }

    public void needRebootPhone(){
        Log.d(LOGTAG, "needRebootPhone empty method");
   }
}
