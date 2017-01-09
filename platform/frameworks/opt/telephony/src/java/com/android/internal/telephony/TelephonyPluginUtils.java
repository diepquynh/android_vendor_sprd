package com.android.internal.telephony;

import android.app.AddonManager;
import android.content.Context;
import com.android.internal.R;

/**
 * Created by SPRD for CMCC new case:Mobile card priority strategy
 */

public class TelephonyPluginUtils {
    private static TelephonyPluginUtils mInstance;
    public static TelephonyPluginUtils getInstance() {
        if (mInstance == null) {
            mInstance = (TelephonyPluginUtils)AddonManager.getDefault().getAddon(R.string.feature_uicc_telephonly_plugin,TelephonyPluginUtils.class);
        }
        return  mInstance;
    }

    public void setDefaultDataSubId(Context context, int subId) {
        SubscriptionController subscriptionController = SubscriptionController.getInstance();
        subscriptionController.setDefaultDataSubId(subId);
    }

    public boolean needSkipSetRadioCapability(Context context,int setSubId) {
        return false;
    }

}
