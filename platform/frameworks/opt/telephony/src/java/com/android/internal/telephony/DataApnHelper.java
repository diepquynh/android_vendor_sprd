
package com.android.internal.telephony;

import java.util.List;

import android.content.Context;
import android.content.Intent;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.Rlog;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

public class DataApnHelper {

    private static final String TAG = "DataApnUtils";
    private static final boolean DBG = true;

    private static DataApnHelper mInstance;

    /* SPRD: FEATURE_OF_APN_AND_DATA_POP_UP @{ */
    private SubscriptionManager mSubscriptionManager;
    private TelephonyManager mTelephonyManager;
    /* @} */
    private Context mContext;

    private DataApnHelper(Context context) {
        mContext = context;
        mSubscriptionManager = SubscriptionManager.from(mContext);
        mTelephonyManager = (TelephonyManager)
                mContext.getSystemService(Context.TELEPHONY_SERVICE);
    }

    public static DataApnHelper getInstance(Context context) {
        if (mInstance == null) {
            mInstance = new DataApnHelper(context);
        }
        return mInstance;
    }

    public void showDataOrApnPopUpIfNeed() {
        /* SPRD: FEATURE_OF_APN_AND_DATA_POP_UP @{ */
        // start the service to show apn config popup if sims changed
        if (mContext != null && needShowDataOrApnPopUp()) {
            Intent apnIntent = new Intent("com.sprd.settings.ApnConfigService");
            apnIntent.setPackage("com.android.settings");
            if (DBG) Rlog.d(TAG, "startService");
            mContext.startService(apnIntent);
        }
        /* @} */
    }

    /* SPRD: FEATURE_OF_APN_AND_DATA_POP_UP @{ */
    private boolean needShowDataOrApnPopUp() {
        CarrierConfigManagerEx carrierConfigManager = (CarrierConfigManagerEx) mContext
                .getApplicationContext().getSystemService("carrier_config_ex");
        if (carrierConfigManager != null
                && carrierConfigManager.getConfig() != null) {
            // get the switch of apn config popup
            boolean needShowApnAndData = carrierConfigManager
                    .getConfigForDefaultPhone()
                    .getBoolean(CarrierConfigManagerEx.KEY_FEATURE_DATA_AND_APN_POP_BOOL);
            if (DBG) Rlog.d(TAG, "needShowApnAndData = " + needShowApnAndData);
            if (needShowApnAndData) {
                List<SubscriptionInfo> subscriptionInfoList = mSubscriptionManager
                        .getActiveSubscriptionInfoList();
                // some oprator would not show apn config popup we can set in carrier config
                // for example:23430,23433
                String operator = carrierConfigManager
                        .getConfig()
                        .getString(
                                CarrierConfigManagerEx.KEY_FEATURE_DATA_AND_APN_POP_OPERATOR_STRING);
                if (DBG) Rlog.d(TAG, "operator = " + operator);
                String[] operators = operator.split(",");
                int activeSubCount = mSubscriptionManager.getActiveSubscriptionInfoCount();
                if (activeSubCount > 0) {
                    for (SubscriptionInfo info : subscriptionInfoList) {
                        int subId = info.getSubscriptionId();
                        int phoneId = info.getSimSlotIndex();
                        String mccmnc = mTelephonyManager.getSimOperator(subId);
                        if (DBG) Rlog.d(TAG, "Check the sim, mccmnc = " + mccmnc);
                        for (String str : operators) {
                            // if there are two sim card and one of them is PIN locked,return false
                            if (isSimLocked(phoneId)
                                    || str.equals(mccmnc)) {
                                return false;
                            }
                        }
                    }
                } else {
                    return false;
                }
            }
            return needShowApnAndData;
        }
        return false;
    }

    private boolean isSimLocked(int phoneId) {
        int simState = mTelephonyManager.getSimState(phoneId);
        if (simState == TelephonyManager.SIM_STATE_PIN_REQUIRED
                || simState == TelephonyManager.SIM_STATE_PUK_REQUIRED
                || simState == TelephonyManager.SIM_STATE_NETWORK_LOCKED) {
            return true;
        }
        return false;
    }
    /* @} */
}
