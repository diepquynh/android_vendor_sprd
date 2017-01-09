
package com.android.server.power;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TelephonyIntents;

public class SprdNatTimeUtils {
    private TelephonyManager mTelephonyManager;
    private CarrierConfigManagerEx mConfigManagerEx;
    private String mccMnc;
    private int mNetworkClass;
    private int mNatOvertime;
    private Context mContext;
    private final String TAG = "SprdNatTimeUtils";
    private int mDefaultNatOvertime = 5;

    SprdNatTimeUtils(Context context) {
        mContext = context;
        mTelephonyManager = (TelephonyManager) mContext
                .getSystemService(mContext.TELEPHONY_SERVICE);
        int dataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
        mccMnc = mTelephonyManager.getNetworkOperator(dataSubId);
        mNetworkClass = TelephonyManager.getNetworkClass(mTelephonyManager
                .getNetworkType(dataSubId));
        mConfigManagerEx = (CarrierConfigManagerEx) context.getSystemService("carrier_config_ex");
        mNatOvertime = getNatOvertimeByNetClass(mNetworkClass, dataSubId);

        IntentFilter filter = new IntentFilter();
        filter.addAction(TelephonyIntents.ACTION_SERVICE_STATE_CHANGED);
        filter.addAction(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);
        context.registerReceiver(mTelephonyReceiver, filter);

    }

    /**
     * Add for Heartbeat NAT time operator network adaptation.get NAT overtime
     * setting from CarrierConfig by current MccMnc and networkClass.
     *
     * @param networkClass 2G/3G/4G
     * @param subId the data SIM ID
     * @return new NAT OT setting time
     */
    public int getNatOvertimeByNetClass(int networkClass, int subId) {
        PersistableBundle persistableBundle = mConfigManagerEx.getConfigForSubId(subId);
        switch (networkClass) {
            case TelephonyManager.NETWORK_CLASS_2_G:
                return persistableBundle.getInt(mConfigManagerEx.KEY_NETWORK_NAT_OVERTIME_2G);
            case TelephonyManager.NETWORK_CLASS_3_G:
                return persistableBundle.getInt(mConfigManagerEx.KEY_NETWORK_NAT_OVERTIME_3G);
            case TelephonyManager.NETWORK_CLASS_4_G:
                return persistableBundle.getInt(mConfigManagerEx.KEY_NETWORK_NAT_OVERTIME_4G);
            default:
                return mDefaultNatOvertime;
        }
    }

    BroadcastReceiver mTelephonyReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            int newNatOvertime = mNatOvertime;
            String newMccMnc = mccMnc;
            int newNetworkClass = mNetworkClass;
            int subId = intent.getIntExtra(PhoneConstants.SUBSCRIPTION_KEY, 0);
            // Only handle data connection SIM
            if (subId == SubscriptionManager.getDefaultDataSubscriptionId()) {
                if (TelephonyIntents.ACTION_SERVICE_STATE_CHANGED.equals(action)) {
                    ServiceState serviceState = ServiceState.newFromBundle(intent.getExtras());
                    newMccMnc = serviceState.getDataOperatorNumeric();
                    newNetworkClass = TelephonyManager.getNetworkClass(serviceState
                            .getDataNetworkType());
                    // Only deal with mccMnc did not change but network class changed.
                    if (newMccMnc != null && mccMnc != null && newMccMnc .equals(mccMnc) && newNetworkClass != mNetworkClass) {
                        newNatOvertime = getNatOvertimeByNetClass(newNetworkClass, subId);
                    }
                } else if (CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED.equals(action)) {
                    newMccMnc = mTelephonyManager.getNetworkOperator(subId);
                    // Only deal with mccMnc changed.
                    if (newMccMnc != null && mccMnc != null && !newMccMnc.equals(mccMnc)) {
                        newNetworkClass = TelephonyManager.getNetworkClass(mTelephonyManager
                                .getDataNetworkType(subId));
                        newNatOvertime = getNatOvertimeByNetClass(newNetworkClass, subId);
                    }
                }
            }
            if (newNatOvertime != mNatOvertime) {
                Log.d(TAG, "setAlarmAlignTime newNatOvertime : "+ newNatOvertime);
                Intent mIntent = new Intent("android.intent.action.setAlarmAligntTime");
                mIntent.putExtra("aligntime", newNatOvertime);
                context.sendBroadcast(mIntent);
            }

            mNetworkClass = newNetworkClass;
            mccMnc = newMccMnc;
            mNatOvertime = newNatOvertime;
        }
    };

}
