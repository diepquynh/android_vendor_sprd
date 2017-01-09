package com.android.phone;

import android.app.ActivityManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.preference.PreferenceManager;
import com.android.internal.telephony.ProxyController;
import android.telephony.RadioAccessFamily;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.text.TextUtils;
import android.util.Log;
import android.os.AsyncResult;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Process;

import com.android.ims.ImsManager;
import com.android.internal.telephony.ITelephonyEx;
import com.android.internal.telephony.OperatorNameHandler;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TeleFrameworkFactory;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.internal.telephony.uicc.SimEnabledController;
import com.android.internal.telephony.uicc.ExtraIccRecords;
import com.android.internal.telephony.uicc.ExtraIccRecordsController;
import com.android.internal.telephony.uicc.UiccCard;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.DefaultPhoneNotifier;

import java.io.IOException;
import android.media.MediaPlayer;
import android.net.Uri;

/**
 * Implementation of the ITelephonyEx interface.
 */
public class PhoneInterfaceManagerEx extends ITelephonyEx.Stub {
    private static final String LOG_TAG = "PhoneInterfaceManagerEx";
    private static final boolean DBG = (PhoneGlobals.DBG_LEVEL >= 2);


    /** The singleton instance. */
    private static PhoneInterfaceManagerEx sInstance;

    private PhoneGlobalsEx mApp;
    private Phone mPhone;
    private SubscriptionController mSubscriptionController;
    private SharedPreferences mTelephonySharedPreferences;
    private static final String PREF_CARRIERS_ALPHATAG_PREFIX = "carrier_alphtag_";
    private static final String PREF_CARRIERS_NUMBER_PREFIX = "carrier_number_";
    private static final String PREF_CARRIERS_SUBSCRIBER_PREFIX = "carrier_subscriber_";
    private MediaPlayer mLocalPlayer;

    /**
     * Initialize the singleton PhoneInterfaceManagerEx instance.
     * This is only done once, at startup, from PhoneApp.onCreate().
     */
    /* package */ static PhoneInterfaceManagerEx init(PhoneGlobalsEx app, Phone phone) {
        synchronized (PhoneInterfaceManager.class) {
            if (sInstance == null) {
                sInstance = new PhoneInterfaceManagerEx(app, phone);
            } else {
                Log.wtf(LOG_TAG, "init() called multiple times!  sInstance = " + sInstance);
            }
            return sInstance;
        }
    }

    /** Private constructor; @see init() */
    private PhoneInterfaceManagerEx(PhoneGlobalsEx app, Phone phone) {
        mApp = app;
        mPhone = phone;
        mSubscriptionController = SubscriptionController.getInstance();
        mTelephonySharedPreferences =
                PreferenceManager.getDefaultSharedPreferences(mPhone.getContext());

        publish();
    }

    private void publish() {
        if (DBG) log("publish: " + this);

        ServiceManager.addService("phone_ex", this);
    }

    /**
     * @return true if a IccFdn enabled
    */

    public boolean getIccFdnEnabledForSubscriber(int subId) {
        return getPhone(subId).getIccCard().getIccFdnEnabled();
    }

    public String getHighPriorityPlmn(int phoneId, String mccmnc) {
        OperatorNameHandler handler = OperatorNameHandler.getInstance();
        return handler == null ? mccmnc : handler.getHighPriorityPlmn(phoneId, mccmnc);
    }

    public String updateNetworkList(int phoneId, String[] operatorInfo) {
        OperatorNameHandler handler = OperatorNameHandler.getInstance();
        return handler == null ? null : handler.updateNetworkList(phoneId, operatorInfo);
    }

    // returns phone associated with the subId.
    private Phone getPhone(int subId) {
        return PhoneFactory.getPhone(mSubscriptionController.getPhoneId(subId));
    }

    /**
     * {@hide}
     * Returns Default subId, 0 in the case of single standby.
     */
    private int getDefaultSubscription() {
        return mSubscriptionController.getDefaultSubId();
    }

    /* SPRD: 474686 Feature for Uplmn @{ */
    public boolean isUsimCard(int phoneId) {
        int appFam = UiccController.APP_FAM_3GPP;
        if (PhoneFactory.getPhone(phoneId).getPhoneType() == PhoneConstants.PHONE_TYPE_CDMA) {
            appFam = UiccController.APP_FAM_3GPP2;
        }
        UiccCardApplication application =
                UiccController.getInstance().getUiccCardApplication(phoneId, appFam);
        if (application != null) {
            return  application.getType() == AppType.APPTYPE_USIM;
        }
        return false;
    }
    /* @} */

    /**
    * SPRD: add option edit sim card's number
    */
    public boolean setLine1NumberForDisplayForSubscriberEx(int subId, String alphaTag,
            String number) {

        int phoneId = SubscriptionManager.getPhoneId(subId);
        if (!SubscriptionManager.isValidPhoneId(phoneId)) {
            return false;
        }

        final String iccId = getIccId(subId);
        final String subscriberId = getPhone(subId).getSubscriberId();

        log("Setting line number for ICC=" + iccId + ", subscriberId="
                + subscriberId + " to " + number + " phoneId = " + phoneId
                + " subId = " + subId);

        if (TextUtils.isEmpty(iccId)) {
            return false;
        }

        final SharedPreferences.Editor editor = mTelephonySharedPreferences.edit();

        final String alphaTagPrefKey = PREF_CARRIERS_ALPHATAG_PREFIX + iccId;
        if (alphaTag == null) {
            editor.remove(alphaTagPrefKey);
        } else {
            editor.putString(alphaTagPrefKey, alphaTag);
        }

        // Record both the line number and IMSI for this ICCID, since we need to
        // track all merged IMSIs based on line number
        final String numberPrefKey = PREF_CARRIERS_NUMBER_PREFIX + iccId;
        final String subscriberPrefKey = PREF_CARRIERS_SUBSCRIBER_PREFIX + iccId;
        if (number == null) {
            editor.remove(numberPrefKey);
            editor.remove(subscriberPrefKey);
        } else {
            editor.putString(numberPrefKey, number);
            editor.putString(subscriberPrefKey, subscriberId);
        }

        editor.commit();
        return true;
    }

    private String getIccId(int subId) {
        final Phone phone = getPhone(subId);
        UiccCard card = phone == null ? null : phone.getUiccCard();
        if (card == null) {
            Log.e(LOG_TAG, "getIccId: No UICC");
            return null;
        }
        String iccId = card.getIccId();
        if (TextUtils.isEmpty(iccId)) {
            Log.e(LOG_TAG, "getIccId: ICC ID is null or empty.");
            return null;
        }
        return iccId;
    }

    public void setSimEnabled(int phoneId, final boolean turnOn) {
        enforceModifyPermission();
        log("setSimEnabled["+phoneId+"]= " + turnOn);
        Phone phone = PhoneFactory.getPhone(phoneId);
        SimEnabledController.getInstance().setSimEnabled(phoneId, turnOn);
    }

    /**
     * Make sure the caller has the MODIFY_PHONE_STATE permission.
     *
     * @throws SecurityException if the caller does not have the required permission
     */
    private void enforceModifyPermission() {
        mApp.enforceCallingOrSelfPermission(android.Manifest.permission.MODIFY_PHONE_STATE, null);
    }

    private static void log(String msg) {
        Log.d(LOG_TAG, "[PhoneIntfMgrEx] " + msg);
    }

    /* SPRD for smsc @{ */
    private int getPreferredSmsSubscription() {
        return SubscriptionManager.getDefaultSmsSubscriptionId();
    }

    public String getSmsc() {
        return getSmscForSubscriber(getPreferredSmsSubscription());
    }

    public String getSmscForSubscriber(int subId) {
        enforceModifyPermission();
        Phone phone = getPhone(subId);
        final GetSetSMSC getSMSC = new GetSetSMSC(phone, null);
        getSMSC.start();
        return getSMSC.getSmsc();
    }

    public boolean setSmsc(String smscAddr) {
        return setSmscForSubscriber(smscAddr, getPreferredSmsSubscription());
    }

    public boolean setSmscForSubscriber(String smscAddr, int subId) {
        enforceModifyPermission();
        Phone phone = getPhone(subId);
        final GetSetSMSC getSMSC = new GetSetSMSC(phone, smscAddr);
        getSMSC.start();
        return getSMSC.setSmsc();
    }

    private static class GetSetSMSC extends Thread {

        private final Phone mPhone;
        private final String mSmscStr;
        private boolean mDone = false;
        private String mResult;
        private boolean bResult = false;

        private Handler mHandler;

        // For async handler to identify request type
        private static final int QUERY_SMSC_DONE = 100;
        private static final int UPDATE_SMSC_DONE = 101;

        public GetSetSMSC(Phone phone, String SmscStr) {
            mPhone = phone;
            mSmscStr = SmscStr;
        }

        @Override
        public void run() {
            Looper.prepare();
            synchronized (GetSetSMSC.this) {
                mHandler = new Handler() {
                    @Override
                    public void handleMessage(Message msg) {
                        AsyncResult ar = (AsyncResult) msg.obj;
                        switch (msg.what) {
                        case QUERY_SMSC_DONE:
                            Log.d(LOG_TAG, "[smsc]QUERY_SMSC_DONE");
                            synchronized (GetSetSMSC.this) {
                                if (ar.exception == null) {
                                    mResult = (String) ar.result;
                                } else {
                                    mResult = "refresh error";
                                }
                                Log.d(LOG_TAG, "=====smsc=========mResult: "
                                        + mResult);
                                mDone = true;
                                GetSetSMSC.this.notifyAll();
                            }
                            getLooper().quit();
                            break;
                        case UPDATE_SMSC_DONE:
                            Log.d(LOG_TAG, "[smsc]UPDATE_SMSC_DONE");
                            synchronized (GetSetSMSC.this) {
                                bResult = (ar.exception == null);
                                Log.d(LOG_TAG, "=====smsc=========bResult: "
                                        + bResult);
                                mDone = true;
                                GetSetSMSC.this.notifyAll();
                            }
                            getLooper().quit();
                            break;
                        }
                    }
                };
                GetSetSMSC.this.notifyAll();
            }
            Looper.loop();
        }

        synchronized String getSmsc() {
            while (mHandler == null) {
                try {
                    wait();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }

            mPhone.getSmscAddress(mHandler.obtainMessage(QUERY_SMSC_DONE));

            while (!mDone) {
                try {
                    Log.d(LOG_TAG, "[smsc]wait get for done");
                    wait();
                } catch (InterruptedException e) {
                    // Restore the interrupted status
                    Thread.currentThread().interrupt();
                }
            }
            Log.d(LOG_TAG, "=====smsc=====getSmsc()====mResult: " + mResult);
            return mResult;
        }

        synchronized boolean setSmsc() {

            while (mHandler == null) {
                try {
                    wait();
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }

            mPhone.setSmscAddress(mSmscStr,
                    mHandler.obtainMessage(UPDATE_SMSC_DONE));

            while (!mDone) {
                try {
                    Log.d(LOG_TAG, "[smsc]wait set for done");
                    wait();
                } catch (InterruptedException e) {
                    // Restore the interrupted status
                    Thread.currentThread().interrupt();
                }
            }
            Log.d(LOG_TAG,
                    "=====smsc====setSmsc()====[smsc]set done. result = "
                            + bResult);
            return bResult;
        }
    }

    /* SPRD: Add for Bug588409 @{ */
    public Bundle getCellLocationForPhone(int phoneId) {
        try {
            mApp.enforceCallingOrSelfPermission(
                    android.Manifest.permission.ACCESS_FINE_LOCATION, null);
        } catch (SecurityException e) {
            mApp.enforceCallingOrSelfPermission(
                    android.Manifest.permission.ACCESS_COARSE_LOCATION, null);
        }
        if (checkIfCallerIsSelfOrForegroundUser()) {
            if (DBG)
                log("getCellLocation: is active user");
            Bundle data = new Bundle();
            Phone phone = PhoneFactory.getPhone(phoneId);
            phone.getCellLocation().fillInNotifierBundle(data);
            return data;
        } else {
            if (DBG)
                log("getCellLocation: suppress non-active user");
            return null;
        }
    }

    /**
     * SPRD: [Bug602746]
     * Returns the PNN home name.
     */
    public String getPnnHomeName(int subId) {
        ExtraIccRecords extraRecords = ExtraIccRecordsController.getInstance()
                .getExtraIccRecords(getPhone(subId).getPhoneId());
        if (extraRecords != null) {
            return extraRecords.getPnnHomeName();
        }
        return null;
    }

    private static boolean checkIfCallerIsSelfOrForegroundUser() {
        boolean ok;

        boolean self = Binder.getCallingUid() == Process.myUid();
        if (!self) {
            // Get the caller's user id then clear the calling identity
            // which will be restored in the finally clause.
            int callingUser = UserHandle.getCallingUserId();
            long ident = Binder.clearCallingIdentity();

            try {
                // With calling identity cleared the current user is the foreground user.
                int foregroundUser = ActivityManager.getCurrentUser();
                ok = (foregroundUser == callingUser);
                if (DBG) {
                    log("checkIfCallerIsSelfOrForegoundUser: foregroundUser=" + foregroundUser
                            + " callingUser=" + callingUser + " ok=" + ok);
                }
            } catch (Exception ex) {
                if (DBG) loge("checkIfCallerIsSelfOrForegoundUser: Exception ex=" + ex);
                ok = false;
            } finally {
                Binder.restoreCallingIdentity(ident);
            }
        } else {
            if (DBG) log("checkIfCallerIsSelfOrForegoundUser: is self");
            ok = true;
        }
        if (DBG) log("checkIfCallerIsSelfOrForegoundUser: ret=" + ok);
        return ok;
    }

    /* SPRD: add for bug589362 @{ */
    public boolean isRingtongUriAvailable(Uri uri) {
        boolean isRingtongUriAvailable = true;
        if (uri != null) {
            destroyLocalPlayer();
            mLocalPlayer = new MediaPlayer();
            try {
                mLocalPlayer.setDataSource(mPhone.getContext(), uri);
            } catch (SecurityException | IOException | NullPointerException e) {
                isRingtongUriAvailable = false;
                destroyLocalPlayer();
            }
        }
        log("uri = " + uri + " isRingtongUriAvailable : " + isRingtongUriAvailable);
        return isRingtongUriAvailable;
    }
    /* @} */

    /* SPRD: add for bug589362 @{ */
    private void destroyLocalPlayer() {
        if (mLocalPlayer != null) {
            mLocalPlayer.reset();
            mLocalPlayer.release();
            mLocalPlayer = null;
        }
    }
    /* @} */

    /* SPRD: Add interface to obtain device capability @{*/
    public boolean isDeviceSupportLte() {
        ProxyController pc = ProxyController.getInstance();
        int rafMax = 0;
        if (pc != null) {
            rafMax = pc.getMaxRafSupported();
        }
        rafMax = rafMax & (RadioAccessFamily.RAF_LTE | RadioAccessFamily.RAF_LTE_CA);
        return (rafMax == (RadioAccessFamily.RAF_LTE | RadioAccessFamily.RAF_LTE_CA))
                | (rafMax == RadioAccessFamily.RAF_LTE) | (rafMax == RadioAccessFamily.RAF_LTE_CA);
    }
    /* @} */

    private static void loge(String msg) {
        Log.e(LOG_TAG, "[PhoneIntfMgrEx] " + msg);
    }
    /* @} */

    /* SPRD: Add interface to obtain sim slot capability @{*/
    public boolean isSimSlotSupportLte(int phoneId) {
        if (SubscriptionManager.isValidPhoneId(phoneId)) {
            Phone phone = PhoneFactory.getPhone(phoneId);
            if (phone != null) {
                int rafMax = phone.getRadioAccessFamily();
                return (rafMax & RadioAccessFamily.RAF_LTE) == RadioAccessFamily.RAF_LTE;
            }
        }
        return false;
    }
    /* @} */
    /* SPRD: Add interface set default data subId for CMCC case SK_0013 @{*/
    public void setDefaultDataSubId(int subId) {
        TeleFrameworkFactory teleFrameworkFactory = TeleFrameworkFactory.getInstance();
        if (teleFrameworkFactory != null) {
            teleFrameworkFactory.setDefaultDataSubId(mPhone.getContext(),subId);
        }
    }
    /* @} */

    public boolean isVolteEnabledByPlatform() {
        return ImsManager.isVolteEnabledByPlatform(mApp);
    }

    public boolean isEnhanced4gLteModeSettingEnabledByUser() {
        return ImsManager.isEnhanced4gLteModeSettingEnabledByUser(mApp);
    }

    public void setEnhanced4gLteModeSetting(boolean enabled) {
        ImsManager.setEnhanced4gLteModeSetting(mApp, enabled);
    }

    public boolean isInternationalNetworkRoaming(int subId){
        Phone phone = getPhone(subId);
        if(phone != null && phone.getServiceState().getVoiceRoamingType() == ServiceState.ROAMING_TYPE_INTERNATIONAL){
            return true;
        }
        return false;
    }

    public int getDataState(int subId) {
        Phone phone = getPhone(subId);
        if (phone != null) {
            return DefaultPhoneNotifier.convertDataState(phone.getDataConnectionState());
        } else {
            return DefaultPhoneNotifier.convertDataState(PhoneConstants.DataState.DISCONNECTED);
        }
    }
}
