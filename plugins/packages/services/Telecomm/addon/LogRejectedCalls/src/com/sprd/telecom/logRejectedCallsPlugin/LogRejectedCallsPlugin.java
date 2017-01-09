package com.sprd.telecom.logRejectedCallsPlugin;

import android.app.AddonManager;
import android.content.Context;
import com.android.server.telecom.Call;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
import android.text.TextUtils;
import android.util.Log;
import com.sprd.server.telecom.LogRejectedCallsHelper;
/**
 * Mark reject call as missed type feature for cmcc&cucc case
 */
public class LogRejectedCallsPlugin extends LogRejectedCallsHelper
        implements AddonManager.InitialCallback {

    private static final String TAG = "LogRejectedCallsPlugin";
    private Context mContext;

    public LogRejectedCallsPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        mContext = context;
        return clazz;
    }

    @Override
    public boolean isSupportMarkRejectCallAsMissedType() {
        log("isSupportMarkRejectCallAsMissedType ---> true");
        return true;
    }

    public String getSimCardInfo(Call call) {
        final String contentInfo = getSlotInfo(call) + getSimcard_Info(call);
        Log.d(TAG, "contentInfo------------=" + contentInfo);

        return contentInfo;
    }

    private String getSlotInfo(Call call) {
        int subId = -1;
        TelephonyManager telephonyManager = (TelephonyManager) mContext
                .getSystemService(Context.TELEPHONY_SERVICE);
        TelecomManager mgr = (TelecomManager) mContext
                .getSystemService(Context.TELECOM_SERVICE);
        try {
            PhoneAccount account = mgr.getPhoneAccount(call
                    .getTargetPhoneAccount());
            subId = telephonyManager.getSubIdForPhoneAccount(account);
        } catch (NullPointerException e) {
            Log.d(TAG, "Exception raised during parsing int.");
        }
        int defaultDataSubId = SubscriptionManager.from(mContext).getDefaultDataSubscriptionId();
        String card_slot = "";
        if (subId == defaultDataSubId) {
            card_slot = mContext.getString(R.string.main_card_slot);
        } else {
            card_slot = mContext.getString(R.string.vice_card_slot);
        }
        return card_slot;
    }

    private String getSimcard_Info(Call call) {
        String card_Info = "";
        String info = ":";
        TelecomManager mgr = (TelecomManager) mContext
                .getSystemService(Context.TELECOM_SERVICE);
        PhoneAccountHandle accountHandle = call.getTargetPhoneAccount();
        PhoneAccount account = mgr.getPhoneAccount(accountHandle);
        if (account != null && !TextUtils.isEmpty(account.getLabel())) {
            card_Info = account.getLabel().toString();
        }
        Log.d(TAG, "card_Info------------=" + card_Info);
        if (card_Info == null || card_Info == "") {
            return card_Info;
        } else {
            return info + card_Info;
        }
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }

}
