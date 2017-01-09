package com.sprd.phone;

import android.app.AddonManager;
import android.content.Context;
import com.android.internal.telephony.OperatorInfo;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;

import com.android.phone.R;

/**
 * Helper class for Telephony Service related plugins
 * @Author: SPRD
 */
public class TeleServicePluginsHelper {

    static TeleServicePluginsHelper mInstance;

    public TeleServicePluginsHelper() {
    }

    public static TeleServicePluginsHelper getInstance(Context context) {
        if (mInstance != null) {
            return mInstance;
        }
        mInstance = (TeleServicePluginsHelper) new AddonManager(context)
                .getAddon(R.string.feature_addon_for_teleService, TeleServicePluginsHelper.class);
        return mInstance;
    }

    private static TeleServicePluginsHelper mInstanceToShowCallingNumber;

    public static TeleServicePluginsHelper getInstanceToShowCallingNumber(Context context) {
        if (mInstanceToShowCallingNumber == null) {
            mInstanceToShowCallingNumber = (TeleServicePluginsHelper) new AddonManager(context).getAddon(
                    R.string.feature_show_calling_number,
                    TeleServicePluginsHelper.class);
        }
        return mInstanceToShowCallingNumber;
    }

    public void setCallingNumberShownEnabled() {
        // do nothing
    }

    public boolean getDisplayNetworkList(OperatorInfo ni,int phoneId) {
        return true;
    }

    public PreferenceScreen needShwoDataSwitch(PreferenceScreen prefSet,SwitchPreference dataSwitch,int subId,Context context) {
        return prefSet;
    }

    public SwitchPreference updateDataSwitch(SwitchPreference dataSwitch,int subId) {
        return dataSwitch;
    }

    public void needRegisterMobileData(Context context) {
    }

    public void needUnregisterMobieData(Context context) {
    }

    public SwitchPreference needSetDataEnable(SwitchPreference beforePref,int subId) {
        return beforePref;
    }

    public boolean needShowNetworkType(Context context,int subId) {
        return false;
    }

    public boolean removeNetworkType (Context context,int subId) {
        return false;
    }
}
