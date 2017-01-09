package com.sprd.settings;

import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.res.Resources;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.telephony.TelephonyManagerEx;
import android.telephony.SubscriptionInfo;
import java.util.List;

import com.android.settings.sim.SimDialogActivity;
import com.sprd.settings.SettingsRelianceHelper;

public class SettingsOperatorPlugin extends SettingsOperatorHelper implements
        AddonManager.InitialCallback {

    public static Context mContext;
    private static final String TAG = "SettingsCMCCPlugin";
    private static final String[] ICCID_PREFIX_CODES_CMCC = {
            "898600", "898602"
    };

    public SettingsOperatorPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public boolean isNeedSetDataEnable(Context context) {
        SubscriptionManager subManager = SubscriptionManager.from(context);
        List<SubscriptionInfo> activeSubinfo = subManager.getActiveSubscriptionInfoList();
        if (activeSubinfo == null || activeSubinfo.size() < 2) {
            return true;
        }
        return false;
    }

    public boolean isSimSlotForPrimaryCard(SubscriptionInfo subInfoRecord){
        return TelephonyManagerEx.from(mContext).isSimSlotSupportLte(subInfoRecord.getSimSlotIndex());
    }

    public boolean isNeedSetSimEnable(SubscriptionInfo subscriptionInfo) {
        return isCMCCCard(subscriptionInfo);
    }

    private boolean isCMCCCard(SubscriptionInfo subInfo) {
        if (subInfo == null) return false;
        String simIccId = subInfo.getIccId();
        if (!TextUtils.isEmpty(simIccId)) {
            for (String iccIdPrefix : ICCID_PREFIX_CODES_CMCC) {
                if (simIccId.startsWith(iccIdPrefix)) {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean removeDataSwitch() {return true;}
}
