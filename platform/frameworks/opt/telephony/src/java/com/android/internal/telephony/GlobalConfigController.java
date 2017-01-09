
package com.android.internal.telephony;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.AsyncResult;
import android.os.BaseBundle;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.SystemProperties;
import android.telephony.CarrierConfigManager;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.PhoneNumberUtils;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.uicc.ExtraIccRecords;
import com.android.internal.telephony.uicc.ExtraIccRecordsController;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.telephony.RadioInteractorCallbackListener;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.UiccCard;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.UsimFileHandler;

import java.util.ArrayList;

/**
 * This class manages cached copies of all the telephony configuration for each phone ID. A phone ID
 * loosely corresponds to a particular SIM or network.
 */
public class GlobalConfigController {
    private static final String TAG = "GlobalConfigController";

    public static final String GLO_CONF_ECC_LIST_NO_CARD = CarrierConfigManagerEx.KEY_GLO_CONF_ECC_LIST_NO_CARD;
    public static final String GLO_CONF_ECC_LIST_WITH_CARD = CarrierConfigManagerEx.KEY_GLO_CONF_ECC_LIST_WITH_CARD;
    public static final String GLO_CONF_FAKE_ECC_LIST_WITH_CARD = CarrierConfigManagerEx.KEY_GLO_CONF_FAKE_ECC_LIST_WITH_CARD;
    public static final String Network_RAT_Prefer = CarrierConfigManagerEx.KEY_Network_RAT_Prefer_INT;

    private static volatile GlobalConfigController sInstance = new GlobalConfigController();

    public static GlobalConfigController getInstance() {
        return sInstance;
    }

    private static final int EVENT_GET_CATEGORY_ECC_DONE =100;

    private String[] mSimEccLists;
    private String[] mNetworkEccList;
    private Bundle[] mGlobalConfig;
    private Context mContext;
    private boolean eccListready;
    private int mPhoneCount;
    private SubscriptionController mSubscriptionController;
    private TelephonyManager mTelephonyManager;
    private ExtraIccRecordsController mExtralIccRecordsController;
    private ExtraIccRecords[] mExtraIccRecords;
    private RadioInteractor mRadioInteractor;
    private RadioInteractorCallbackListener[] mRadioInteractorCallbackListener;

    /**
     * This receiver listens for changes made to carrier config and for a broadcast telling us the
     * CarrierConfigLoader has loaded or updated the carrier config information when sim loaded or
     * network registered. When either of these broadcasts are received, we rebuild the TeleConfig
     * table.
     */
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG, "Receiver action: " + action);
            if (action.equals(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED)) {
                loadInBackground();
            } else if (Intent.ACTION_LOCALE_CHANGED.equals(action)) {
                updateDiaplayCarrierName();
            }
        }
    };

    public void init(final Context context) {
        mContext = context;
        mSubscriptionController = SubscriptionController.getInstance();
        mTelephonyManager = TelephonyManager.from(context);
        mPhoneCount = mTelephonyManager.getPhoneCount();
        mGlobalConfig = new Bundle[mPhoneCount];
        mSimEccLists = new String[mPhoneCount];
        mNetworkEccList = new String[mPhoneCount];
        mExtralIccRecordsController = ExtraIccRecordsController.getInstance();
        mExtraIccRecords = mExtralIccRecordsController.getExtraIccRecords();
        mRadioInteractor = new RadioInteractor(context);
        mRadioInteractorCallbackListener = new RadioInteractorCallbackListener[mPhoneCount];
        setNetworkChangedListener();
        for (int i = 0; i < mPhoneCount; i++) {
            mExtraIccRecords[i].registerForEccRecordsChanged(this.mHandler, EVENT_GET_CATEGORY_ECC_DONE, null);
        }

        final IntentFilter intentFilterLoaded =
                new IntentFilter(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);
        intentFilterLoaded.addAction(Intent.ACTION_LOCALE_CHANGED);
        context.registerReceiver(mReceiver, intentFilterLoaded);
    }

    private void loadInBackground() {
        new Thread() {
            @Override
            public void run() {
                Log.d(TAG, "Load global config in background...");
                load();
                updateEccProperties();
                setPreferredNetworkRAT();
            }
        }.start();
    }

    public Bundle getGlobalConfigBySubId(int subId) {
        Log.d(TAG, "get tele config by sub ID : " + subId);
        int phoneId = mSubscriptionController.getPhoneId(subId);
        return getGlobalConfigByPhoneId(phoneId);
    }

    /**
     * Find and return the global config for a particular phone id.
     *
     * @param phoneId Phone id of the desired global config bundle
     * @return Global config bundle for the particular phone id.
     */
    public Bundle getGlobalConfigByPhoneId(int phoneId) {
        Bundle teleConfig = null;
        if (SubscriptionManager.isValidPhoneId(phoneId) && phoneId < mGlobalConfig.length) {
            synchronized (mGlobalConfig) {
                teleConfig = mGlobalConfig[phoneId];
            }
        }
        Log.d(TAG, "tele config for phone " + phoneId + ": " + teleConfig);
        // Return a copy so that callers can mutate it.
        if (teleConfig != null) {
            return new Bundle(teleConfig);
        }
        return null;
    }

    /**
     * This loads the global config for each phone. Global config is fetched from
     * GlobalConfigController and filtered to only include global config variables. The resulting
     * bundles are stored in mGlobalConfig.
     */
    private void load() {
        // Load all the config bundles into a new array and then swap it with the real array to avoid
        // blocking.
        final int phoneCount = mTelephonyManager.getPhoneCount();
        final Bundle[] GlobalConfigController = new Bundle[phoneCount];
        final CarrierConfigManagerEx configManager = CarrierConfigManagerEx.from(mContext);
        for (int i = 0; i < phoneCount; i++) {
            PersistableBundle config = configManager.getConfigForPhoneId(i);
            GlobalConfigController[i] = getGlobalConfig(config);
        }

        synchronized (mGlobalConfig) {
            mGlobalConfig = GlobalConfigController;
        }
    }

    private RadioInteractorCallbackListener getRadioInteractorCallbackListener(final int phoneId) {
        return new RadioInteractorCallbackListener(phoneId){
            @Override
            public void onEccNetChangedEvent(Object object){
                AsyncResult ar =(AsyncResult)object;
                mNetworkEccList[phoneId] = (String)ar.result;
                updateEccProperties();
            }
        };
    }

    private void setPreferredNetworkRAT() {
        int phoneCount = mTelephonyManager.getPhoneCount();
        if (phoneCount != mGlobalConfig.length) {
            return;
        }
        for (int i = 0; i < phoneCount; i++) {
            int networkRATPrefer = mGlobalConfig[i].getInt(Network_RAT_Prefer);
            Log.d(TAG, "networkRATPrefer:" + networkRATPrefer);
            mRadioInteractor.setNetworkSpecialRATCap(networkRATPrefer, i);
        }
    }

    public void setNetworkChangedListener() {
        mContext.bindService(
                new Intent("com.android.sprd.telephony.server.RADIOINTERACTOR_SERVICE")
                        .setPackage("com.android.sprd.telephony.server"),
                new ServiceConnection() {
                    @Override
                    public void onServiceConnected(ComponentName name, IBinder service) {
                        Log.d(TAG, "on radioInteractor service connected");
                        for (int i = 0; i < mPhoneCount; i++) {
                            mRadioInteractorCallbackListener[i] = getRadioInteractorCallbackListener(
                                    i);
                            mRadioInteractor.listen(mRadioInteractorCallbackListener[i],
                                    RadioInteractorCallbackListener.LISTEN_ECC_NETWORK_CHANGED_EVENT,
                                    false);
                        }
                    }

                    @Override
                    public void onServiceDisconnected(ComponentName name) {
                        for (int i = 0; i < mPhoneCount; i++) {
                            mRadioInteractor.listen(mRadioInteractorCallbackListener[i],
                                    RadioInteractorCallbackListener.LISTEN_NONE);
                        }
                    }
                }, Context.BIND_AUTO_CREATE);
    }

    /*
     * Write system property for ecc after get ecclist config from carrier config
     * service and network. @{
     */
    private void updateEccProperties() {

        int phoneCount = mTelephonyManager.getPhoneCount();
        if (mSimEccLists.length >= phoneCount) {
            for (int i = 0; i < phoneCount; i++) {
                String eccList = "";
                String eccListFromCarrierConfig = getEccListFromCarrierConfig(i);
                Log.d(TAG, "updateEccProperties ECC[" + i + "]:" + " eccListFromSim = "
                        + mSimEccLists[i]
                        + " mNetworkEccList = " + mNetworkEccList[i]
                        + " eccListFromCarrierConfig = " + eccListFromCarrierConfig);
                //According spec 3GPP TS22.101,add ECC Number
                eccList = mTelephonyManager.hasIccCard(i)
                        ? "112,911"
                        : "112,911,000,08,110,118,119,999";
                //Add ECC Number from CarrierConfig
                eccList = TeleUtils.concatenateEccList(
                        TeleUtils.removeDupNumber(eccList, eccListFromCarrierConfig), eccListFromCarrierConfig);
                //Add ECC Number from SIM
                eccList = TeleUtils.concatenateEccList(
                        TeleUtils.removeDupNumber(eccList, mSimEccLists[i]), mSimEccLists[i]);
                //Add ECC Number from Network
                eccList = TeleUtils.concatenateEccList(
                        TeleUtils.removeDupNumber(eccList, mNetworkEccList[i]), mNetworkEccList[i]);
                //Add ECC Number from Fake
                eccList = getFakeEccListByPhoneId(eccList, i);
                SystemProperties.set(i == 0 ? "ril.ecclist" : ("ril.ecclist" + i),
                        TeleUtils.removeCategory(eccList));
            }

        }

        // ECC plugin
        EccPluginHelper.getInstance().customizedEccList(mSimEccLists,mNetworkEccList);
    }
    /* @} */

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            AsyncResult ar;
            int phoneId;
            switch (msg.what) {
                case EVENT_GET_CATEGORY_ECC_DONE:
                    ar = (AsyncResult) msg.obj;
                    if (ar.exception != null) {
                        return;
                    }
                    phoneId = (int)ar.result;
                    mSimEccLists[phoneId] = mExtraIccRecords[phoneId].getSimEccList();
                    updateEccProperties();
                    break;
            }
        };
    };

    private String getEccListFromCarrierConfig(int phoneId) {
        String eccList = "";
        Bundle config = getGlobalConfigByPhoneId(phoneId);
        if (config != null) {
            if (TelephonyManager.from(mContext).getSimState(phoneId) ==
                    TelephonyManager.SIM_STATE_READY) {
                eccList = config.getString(GLO_CONF_ECC_LIST_WITH_CARD);
            } else {
                eccList = config.getString(GLO_CONF_ECC_LIST_NO_CARD);
            }
        }
        return eccList;
    }

    /**
     * get fake Ecc List when the network changed.
     */
    private String getFakeEccListByPhoneId(String realEccList, int phoneId) {
        Bundle config = getGlobalConfigByPhoneId(phoneId);
        String eccList = realEccList;
        String fakeEccList = "";
        if (config != null) {
            fakeEccList = config.getString(GLO_CONF_FAKE_ECC_LIST_WITH_CARD);
            eccList = TeleUtils.concatenateEccList(
                    TeleUtils.removeDupNumber(fakeEccList, realEccList), realEccList);
            realEccList = TeleUtils.removeDupNumber(realEccList, fakeEccList);
        }

        /* SPRD: modify for Bug597769 @{*/
        if (TelephonyManager.from(mContext).getSimState(phoneId) !=
                TelephonyManager.SIM_STATE_READY) {
            realEccList = eccList;
        }
        /* @} */
        mRadioInteractor.updateRealEccList(realEccList, phoneId);

        Log.d(TAG, "realEccList[" + phoneId + "]:" + realEccList);
        Log.d(TAG, "fakeEccList[" + phoneId + "]:" + fakeEccList);
        return eccList;
    }

    private Bundle getGlobalConfig(BaseBundle config) {
        Bundle filtered = new Bundle();
        filtered.putString(GLO_CONF_ECC_LIST_NO_CARD, config.getString(GLO_CONF_ECC_LIST_NO_CARD));
        filtered.putString(GLO_CONF_ECC_LIST_WITH_CARD, config.getString(GLO_CONF_ECC_LIST_WITH_CARD));
        filtered.putString(GLO_CONF_FAKE_ECC_LIST_WITH_CARD, config.getString(GLO_CONF_FAKE_ECC_LIST_WITH_CARD));
        filtered.putInt(Network_RAT_Prefer, config.getInt(Network_RAT_Prefer));
        return filtered;
    }

    /**
     * local language has changed,the spn need to update
     */
    private void updateDiaplayCarrierName() {
        for (int i =0;i<mPhoneCount;i++) {
            String carrierName = mTelephonyManager.getSimOperatorNameForPhone(i);
            OperatorNameHandler.getInstance().updateSpnFromCarrierConfig(i, carrierName);
        }
    }
}
