package com.android.internal.telephony;

import android.app.ActivityThread;
import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

import com.android.internal.R;

public class EccPluginHelper {

    private static EccPluginHelper mInstance;

    public EccPluginHelper() {
    }

    public static EccPluginHelper getInstance() {
        if (mInstance != null)
            return mInstance;
        mInstance = (EccPluginHelper) AddonManager.getDefault().getAddon(R.string.feature_ecc_list_customized_by_operator,
                EccPluginHelper.class);
        return mInstance;
    }

    public void customizedEccList(String[] simEccs,String[] networkEccs) {
        // Do nothing.
    }

}
