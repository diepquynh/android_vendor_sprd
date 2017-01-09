package com.android.phone;

import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.PersistableBundle;
import android.preference.ListPreference;
import android.preference.PreferenceScreen;
import android.provider.Settings.Global;
import android.provider.Settings.System;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.ProxyController;
import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.IccCardConstants;

import android.telephony.RadioAccessFamily;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.sprd.phone.TeleServicePluginsHelper;
import android.util.Log;

public class NetworkTypeOptions {
    private Context mContext;
    private PreferenceScreen mPrefSet;
    private ListPreference mButtonEnabledNetworks;
    private boolean isNotAllow3g2gOnly = false;
    private final static String TAG = "NetworkTypeOptions";
    private final StateChangeReciever mStateChangeReciever = new StateChangeReciever();
    private int mSubId = 0;

    public NetworkTypeOptions(Context context) {
        mContext = context;
        CarrierConfigManagerEx ccme = (CarrierConfigManagerEx) mContext
                .getSystemService("carrier_config_ex");
        PersistableBundle carrierConfig = ccme
                .getConfigForSubId(SubscriptionManager.getDefaultDataSubscriptionId());
        if (carrierConfig != null) {
            isNotAllow3g2gOnly = carrierConfig.getBoolean(
                    CarrierConfigManagerEx.KEY_NETWORK_NOT_ALLOW_3G_AND_2G_BOOL,
                    false);
        }
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
        intentFilter.addAction(TelephonyManager.ACTION_PHONE_STATE_CHANGED);
        // SPRD: modify for bug508651
        intentFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        mContext.registerReceiver(mStateChangeReciever, intentFilter);
    }

    //SPRD: modify for 610272
    public void updateNetworkOptions(int phoneSubId,PreferenceScreen prefSet) {
        Log.d(TAG, "updateNetworkOptions");
        mSubId = phoneSubId;
        mPrefSet = prefSet;
        mButtonEnabledNetworks = (ListPreference) mPrefSet.findPreference("enabled_networks_key");
        if (isDeviceSupportLte(phoneSubId)) {
            mButtonEnabledNetworks.setEntries(R.array.lte_preferred_networks_choices_for_common);
            mButtonEnabledNetworks
                    .setEntryValues(R.array.lte_network_mode_choices_values_for_common);
        } else if (isDeviceSupportWCDMA(phoneSubId) ){
            mButtonEnabledNetworks.setEntries(R.array.enabled_networks_except_lte_choices);
            mButtonEnabledNetworks
                    .setEntryValues(R.array.enabled_networks_except_lte_values);
        }

        if (isNotAllow3g2gOnly
                && !isUsimCard(SubscriptionManager
                        .getPhoneId(SubscriptionManager.getDefaultDataSubscriptionId()))) {
            mPrefSet.removePreference(mButtonEnabledNetworks);
        }
        // SPRD: [Bug588574] Enable/disable switching network type button when in/out of a call.
        if ((phoneSubId != SubscriptionManager.getDefaultDataSubscriptionId() && !isDeviceSupportWCDMA(phoneSubId))
                || isPhoneInCall()) {
            mPrefSet.removePreference(mButtonEnabledNetworks);
        }
        if (TeleServicePluginsHelper.getInstance(mContext).removeNetworkType(mContext,mSubId)) {
            mPrefSet.removePreference(mButtonEnabledNetworks);
        }

    }

    private boolean isPhoneInCall() {
        // Call SubscriptionController directly To avoid redundant RPC because
        // switching network
        // type works in phone process.
        int[] activeSubIdList = SubscriptionController.getInstance().getActiveSubIdList();

        for (int subId : activeSubIdList) {
            if (TelephonyManager.from(mContext).getCallState(subId) != TelephonyManager.CALL_STATE_IDLE) {
                return true;
            }
        }
        return false;
    }

    private class StateChangeReciever extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(action)) {
                Log.d(TAG, "handler ACTION_SIM_STATE_CHANGED");
                String stateExtra = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                Log.d(TAG, "SIM state: " + stateExtra);
                if (stateExtra != null
                        && IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(stateExtra)) {
                    dismissDialog();
                }
            } else {
                Log.d(TAG, "Airplane changed recieve,updateBody");
                updateBody();
            }
        }
    }

    private void updateBody() {
        if (mPrefSet != null) {
            mPrefSet.setEnabled(
                    System.getInt(mContext.getContentResolver(), Global.AIRPLANE_MODE_ON, 0) == 0);
         // SPRD: [Bug588574] Enable/disable switching network type button when in/out of a call.
            if (isPhoneInCall() && mButtonEnabledNetworks != null) {
                // SPRD: MODIFY FOR BUG 601149
                Dialog dialog = mButtonEnabledNetworks.getDialog();
                if(dialog != null){
                    dialog.dismiss();
                }
                mPrefSet.removePreference(mButtonEnabledNetworks);
            } else if (!isPhoneInCall() && mButtonEnabledNetworks != null
                    && isShowNetworkOption(mSubId)) {
                mPrefSet.addPreference(mButtonEnabledNetworks);
            }
        }
    }

    private boolean isShowNetworkOption(int subId) {
        if (TeleServicePluginsHelper.getInstance(mContext).needShowNetworkType(mContext,subId)) {
            return true;
        } else {
            return  mSubId == SubscriptionManager.getDefaultDataSubscriptionId();
        }
    }

    private boolean isDeviceSupportLte(int subId) {
        int phoneId = SubscriptionManager.getPhoneId(subId);
        if (SubscriptionManager.isValidPhoneId(phoneId)) {
            int rafMax = PhoneFactory.getPhone(phoneId).getRadioAccessFamily();
            return (rafMax & RadioAccessFamily.RAF_LTE) == RadioAccessFamily.RAF_LTE;
        }
        return false;
    }

    private boolean isDeviceSupportWCDMA(int subId) {
        int WCDMA = RadioAccessFamily.RAF_HSPAP | RadioAccessFamily.RAF_HSDPA | RadioAccessFamily.RAF_HSPA
                | RadioAccessFamily.RAF_HSUPA | RadioAccessFamily.RAF_UMTS;
            int phoneId = SubscriptionManager.getPhoneId(subId);
            if (SubscriptionManager.isValidPhoneId(phoneId)) {
            int accessFamily = PhoneFactory.getPhone(phoneId).getRadioAccessFamily();
            return (WCDMA & accessFamily) == WCDMA;
        }
        return false;
    }

    private boolean isUsimCard(int primaryCard) {
        UiccController uc = UiccController.getInstance();
        if (uc != null) {
            UiccCardApplication currentApp = uc.getUiccCardApplication(primaryCard,
                    UiccController.APP_FAM_3GPP);
            if (currentApp != null) {
                Log.d(TAG, "APPTYPE=" + currentApp.getType());
                return currentApp.getType() == AppType.APPTYPE_USIM;
            }
        }
        return true;
    }

    public void UpdateEnabledNetworksValueAndSummary(int NetworkMode) {
        switch (NetworkMode) {
            case Phone.NT_MODE_WCDMA_ONLY:
                mButtonEnabledNetworks.setValue(
                        Integer.toString(Phone.NT_MODE_WCDMA_ONLY));
                mButtonEnabledNetworks.setSummary(R.string.network_wcdma_only);
                break;
            case Phone.NT_MODE_WCDMA_PREF:
                mButtonEnabledNetworks.setValue(
                        Integer.toString(Phone.NT_MODE_WCDMA_PREF));
                if (!(mContext.getResources().getBoolean(R.bool.config_enabled_lte))) {
                    mButtonEnabledNetworks.setSummary(R.string.network_wcdma_pref);
                } else {
                    mButtonEnabledNetworks.setSummary(R.string.network_3g_2g);
                }
                break;
            case Phone.NT_MODE_GSM_ONLY:
                mButtonEnabledNetworks.setValue(
                        Integer.toString(Phone.NT_MODE_GSM_ONLY));
                if (!(mContext.getResources().getBoolean(R.bool.config_enabled_lte))) {
                    mButtonEnabledNetworks.setSummary(R.string.network_gsm_only);
                } else {
                    mButtonEnabledNetworks.setSummary(R.string.network_2g_only);
                }
                break;
            case Phone.NT_MODE_LTE_GSM_WCDMA:
                if (isDeviceSupportLte(mSubId)) {
                    mButtonEnabledNetworks.setValue(
                            Integer.toString(Phone.NT_MODE_LTE_GSM_WCDMA));
                    mButtonEnabledNetworks.setSummary(R.string.network_lte_pref);
                } else {
                    String errMsg = "Invalid Network Mode (" + NetworkMode + "). Ignore.";
                    mButtonEnabledNetworks.setSummary(errMsg);
                }
                break;
            case Phone.NT_MODE_LTE_ONLY:
                mButtonEnabledNetworks.setValue(
                        Integer.toString(Phone.NT_MODE_LTE_ONLY));
                mButtonEnabledNetworks.setSummary(R.string.network_lte_only);
                break;
            default:
                return;
        }
    }

    public void dispose() {
        Log.d(TAG, "dispose");
        mContext.unregisterReceiver(mStateChangeReciever);
        mContext = null;
        mButtonEnabledNetworks = null;
        mPrefSet = null;
    }

    private void dismissDialog(){
        if(mButtonEnabledNetworks.getDialog() != null && mButtonEnabledNetworks.getDialog().isShowing()){
            Log.d(TAG, "NetworksNetworks dismiss");
            mButtonEnabledNetworks.getDialog().dismiss();
        }
    }
}
