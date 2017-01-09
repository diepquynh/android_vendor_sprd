/* Created by Spreadst */
package com.android.internal.telephony;

import android.content.Context;
import android.content.SharedPreferences;

import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.SystemProperties;

import android.preference.PreferenceManager;
import android.telecom.VideoProfile;
import android.telephony.CarrierConfigManager;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.PhoneNumberUtils;
import android.telephony.Rlog;
import android.telephony.TelephonyManagerEx;
import android.telephony.ServiceState;
import android.text.TextUtils;
import android.util.Log;

import com.android.ims.ImsManager;
import com.android.internal.telephony.MmiCode;
import com.android.internal.telephony.gsm.GsmMmiCode;
import com.android.internal.telephony.gsm.GsmMmiCodeEx;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.sprd.telephony.uicc.IccCardStatusEx;

import java.util.ArrayList;
import java.util.List;

public class GsmCdmaPhoneEx extends GsmCdmaPhone {
    private static final String TAG = "GsmCdmaPhoneEx";
    private static final boolean LOCAL_DEBUG = true;
    private static final boolean DBG = true;
    private static final int GSM_TYPE = 15;
    private static final int UCS2_TYPE = 72;
    protected static final int EVENT_SET_CALL_BARRING_DONE = 40;
    protected static final int EVENT_GET_CALL_BARRING_DONE = 41;
    protected static final int EVENT_CHANGE_CALL_BARRING_PASSWORD_DONE = 44;
    private static final int EVENT_SUPPLY_DEPERSONALISATION_DONE = 45;
    public static final int CB_REASON_AO = 0;
    public static final int CB_REASON_OI = 1;
    public static final int CB_REASON_OX = 2;
    public static final int CB_REASON_AI = 3;
    public static final int CB_REASON_IR = 4;
    public static final int CB_REASON_AB = 5;

    public static final int CB_ACTION_DISABLE = 0;
    public static final int CB_ACTION_ENABLE = 1;

    public static final String VIDEO_STATE = "video_state";

    //add for Bug 634508
    private boolean mIsSimLoaded;

    public GsmCdmaPhoneEx(Context context, CommandsInterface ci, PhoneNotifier notifier,
            int phoneId,
            int precisePhoneType, TelephonyComponentFactory telephonyComponentFactory) {
        this(context, ci, notifier, false, phoneId, precisePhoneType, telephonyComponentFactory);
    }

    public GsmCdmaPhoneEx(Context context, CommandsInterface ci, PhoneNotifier notifier,
            boolean unitTestMode, int phoneId, int precisePhoneType,
            TelephonyComponentFactory telephonyComponentFactory) {
        super(context, ci, notifier, unitTestMode, phoneId, precisePhoneType,
                telephonyComponentFactory);
    }

    /* SPRD: added for sim lock @{ */
    public void supplyDepersonalisation(boolean isLock, String password, Message onComplete) {
        int serviceClassX = 0;
        int type = (int)onComplete.obj;
        String facility = getSimlockTypes(type);
        Rlog.d(LOG_TAG, "supplyDepersonalisation facility : " + facility + " type :" + type);
        mCi.setFacilityLock(facility, isLock, password, serviceClassX,
                obtainMessage(EVENT_SUPPLY_DEPERSONALISATION_DONE, 0, type, onComplete));
    }
    /* @} */

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;
        Message onComplete;
        switch (msg.what) {
            case EVENT_USSD:
                ar = (AsyncResult) msg.obj;
                Rlog.w(LOG_TAG, "handle EVENT_USSD message");
                String[] ussdResult = (String[]) ar.result;
                if (ussdResult.length > 1) {
                    try {
                        onIncomingUSSD(Integer.parseInt(ussdResult[0]), ussdResult[1]);
                    } catch (NumberFormatException e) {
                        Rlog.w(LOG_TAG, "error parsing USSD");
                    }
                }
                break;

            case EVENT_SET_CALL_BARRING_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    // nothing to do so far
                }
                onComplete = (Message) ar.userObj;
                if (onComplete != null) {
                    AsyncResult.forMessage(onComplete, ar.result, ar.exception);
                    onComplete.sendToTarget();
                }
                break;

            case EVENT_GET_CALL_BARRING_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    // nothing to do so far
                }
                onComplete = (Message) ar.userObj;
                if (onComplete != null) {
                    AsyncResult.forMessage(onComplete, ar.result, ar.exception);
                    onComplete.sendToTarget();
                }
                break;

            case EVENT_CHANGE_CALL_BARRING_PASSWORD_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    // nothing to do so far
                }
                onComplete = (Message) ar.userObj;
                if (onComplete != null) {
                    AsyncResult.forMessage(onComplete, ar.result, ar.exception);
                    onComplete.sendToTarget();
                }
                break;

            /* SPRD: added for sim lock @{ */
            case EVENT_SUPPLY_DEPERSONALISATION_DONE:
                ar = (AsyncResult) msg.obj;
                int type = msg.arg2;
                onChangeFacilitySimLock(ar,type);
                break;
            /* @} */
                /*SPRD: add for Bug 634508 @{ */
            case EVENT_CARRIER_CONFIG_CHANGED:
                if(mIsSimLoaded){
                    logd("EVENT_CARRIER_CONFIG_CHANGED: mIsSimLoaded = " + mIsSimLoaded);
                    mIsSimLoaded = false;
                    ImsManager.updateImsServiceConfig(mContext, mPhoneId, true);
                }
                super.handleMessage(msg);
                break;
            case EVENT_SIM_RECORDS_LOADED:
                logd("EVENT_SIM_RECORDS_LOADED: mPhoneId = " + mPhoneId);
                mIsSimLoaded = true;
                super.handleMessage(msg);
                break;
                /*@}*/
            default:
                super.handleMessage(msg);
        }
    }

    private void onIncomingUSSD(int ussdMode, String ussdMessage) {
        boolean isUssdError;
        boolean isUssdRequest;
        boolean isUssdRelease;

        isUssdRequest = (ussdMode == CommandsInterface.USSD_MODE_REQUEST);

        isUssdError = (ussdMode != CommandsInterface.USSD_MODE_NOTIFY
                && ussdMode != CommandsInterface.USSD_MODE_REQUEST
                // SPRD: add for bug494828
                && ussdMode != CommandsInterface.USSD_MODE_NW_RELEASE);

        isUssdRelease = (ussdMode == CommandsInterface.USSD_MODE_NW_RELEASE);
        // See comments in GsmMmiCode.java
        // USSD requests aren't finished until one
        // of these two events happen
        GsmMmiCode found = null;
        List <MmiCode> pendingMMIList = (List <MmiCode>)getPendingMmiCodes();
        for (int i = 0, s = pendingMMIList.size(); i < s; i++) {
            if (((GsmMmiCode)pendingMMIList.get(i)).isPendingUSSD()) {
                found = (GsmMmiCode)pendingMMIList.get(i);
                break;
            }
        }
        if (LOCAL_DEBUG) {
            Rlog.d(TAG, "ussdMode = " + ussdMode + ", found = " + found + ", ussdMessage = "
                    + ussdMessage);
        }

        if (found != null) {
            // Complete pending USSD
            if (LOCAL_DEBUG) {
                Rlog.d(TAG, "USSD state = " + found.getState());
            }

            if (isUssdRelease) {
                found.onUssdRelease();
            } else if (isUssdError) {
                found.onUssdFinishedError();
            } else {
                found.onUssdFinished(ussdMessage, isUssdRequest);
            }
        } else { // pending USSD not found
            // The network may initiate its own USSD request

            // ignore everything that isnt a Notify or a Request
            // also, discard if there is no message to present
            if (!isUssdError && ussdMessage != null) {
                GsmMmiCode mmi;
                mmi = GsmMmiCode.newNetworkInitiatedUssd(ussdMessage,
                        isUssdRequest,
                        this,
                        mUiccApplication.get());
                onNetworkInitiatedUssd(mmi);
            } else if (isUssdError) {
                GsmMmiCode mmi;
                mmi = GsmMmiCodeEx.newNetworkInitiatedUssdError(ussdMessage,
                        isUssdRequest,
                        this,
                        mUiccApplication.get());
                onNetworkInitiatedUssd(mmi);
            }
        }
    }

    private void
            onNetworkInitiatedUssd(GsmMmiCode mmi) {
        mMmiCompleteRegistrants.notifyRegistrants(
                new AsyncResult(null, mmi, null));
    }

    private String getSimlockTypes(int type){
         String facility = "";
         switch(type) {
         case IccCardStatusEx.UNLOCK_SIM:
             facility = IccCardStatusEx.CB_FACILITY_BA_PS;
             break;
         case IccCardStatusEx.UNLOCK_NETWORK:
             facility = IccCardStatusEx.CB_FACILITY_BA_PN;
             break;
         case IccCardStatusEx.UNLOCK_NETWORK_SUBSET:
             facility = IccCardStatusEx.CB_FACILITY_BA_PU;
             break;
         case IccCardStatusEx.UNLOCK_SERVICE_PORIVDER:
             facility = IccCardStatusEx.CB_FACILITY_BA_PP;
             break;
         case IccCardStatusEx.UNLOCK_CORPORATE:
             facility = IccCardStatusEx.CB_FACILITY_BA_PC;
             break;
         case IccCardStatusEx.UNLOCK_NETWORK_PUK:
             facility = IccCardStatusEx.CB_FACILITY_BA_PN_PUK;
             break;
         case IccCardStatusEx.UNLOCK_NETWORK_SUBSET_PUK:
             facility = IccCardStatusEx.CB_FACILITY_BA_PU_PUK;
             break;
         case IccCardStatusEx.UNLOCK_SERVICE_PORIVDER_PUK:
             facility = IccCardStatusEx.CB_FACILITY_BA_PP_PUK;
             break;
         case IccCardStatusEx.UNLOCK_CORPORATE_PUK:
             facility = IccCardStatusEx.CB_FACILITY_BA_PC_PUK;
             break;
         case IccCardStatusEx.UNLOCK_SIM_PUK:
             facility = IccCardStatusEx.CB_FACILITY_BA_PS_PUK;
             break;
          default :
             facility = IccCardStatusEx.CB_FACILITY_BA_PN;
             break;
         }
         return facility;
    }

    private void onChangeFacilitySimLock(AsyncResult ar, int type) {
        int attemptsRemaining = -1;

        if (ar.exception != null) {
            attemptsRemaining = parseFacilityErrorResultEx(ar);
            Rlog.e(LOG_TAG, "Error change facility lock with exception " + ar.exception);
        }
        Message response = (Message)ar.userObj;
        AsyncResult.forMessage(response).exception = ar.exception;
        response.arg1 = attemptsRemaining;
        response.arg2 = type ;
        response.sendToTarget();
    }

    private int parseFacilityErrorResultEx(AsyncResult ar) {
        int[] result = (int[]) ar.result;
        if (result == null) {
            return -1;
        } else {
            int length = result.length;
            int attemptsRemaining = -1;
            if (length > 0) {
                attemptsRemaining = result[0];
            }
            Rlog.d(LOG_TAG, "parsePinPukErrorResult: attemptsRemaining=" + attemptsRemaining);
            return attemptsRemaining;
        }
    }


    public void changeBarringPassword(String facility, String oldPwd, String newPwd,
            Message onComplete) {
        Rlog.d(LOG_TAG, "changeBarringPassword: " + facility);
        mCi.changeBarringPassword(facility, oldPwd, newPwd,
                obtainMessage(EVENT_CHANGE_CALL_BARRING_PASSWORD_DONE, onComplete));
    }

    public void setFacilityLock(String facility, boolean lockState, String password,
            int serviceClass, Message onComplete) {
        Rlog.d(LOG_TAG, "setFacilityLock: " + facility);
        mCi.setFacilityLock(facility, lockState, password,
                serviceClass,
                obtainMessage(EVENT_SET_CALL_BARRING_DONE, onComplete));
    }

    public void queryFacilityLock(String facility, String password, int serviceClass,
            Message onComplete) {
        Rlog.d(LOG_TAG, "queryFacilityLock: " + facility);
        mCi.queryFacilityLock(facility, password, serviceClass,
                obtainMessage(EVENT_GET_CALL_BARRING_DONE, onComplete));
    }

    @Override
    public String getVoiceMailNumber() {
        String number = null;
        if (isPhoneTypeGsm()) {
            // Read from the SIM. If its null, try reading from the shared preference area.
            IccRecords r = mIccRecords.get();
            number = (r != null) ? r.getVoiceMailNumber() : "";
            if (TextUtils.isEmpty(number)) {
                SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());
                number = sp.getString("vm_number_key" + getPhoneId(), null);
            }
        } else {
            SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(getContext());
            number = sp.getString( "vm_number_key_cdma" + getPhoneId(), null);
        }

        /* SPRD: [bug476017] Load voicemail number from carrier config. @{ */
        if (TextUtils.isEmpty(number)) {
            CarrierConfigManagerEx carrierConfigManager = CarrierConfigManagerEx.from(mContext);
            if (carrierConfigManager != null) {
                PersistableBundle globalConfig = carrierConfigManager.getConfigForPhoneId(getPhoneId());
                if (globalConfig != null) {
                    if (getServiceState().getRoaming()) {
                        number = globalConfig.getString(CarrierConfigManagerEx.KEY_GLO_CONF_ROAMING_VOICEMAIL_NUMBER);
                    } else {
                        number = globalConfig.getString(CarrierConfigManagerEx.KEY_GLO_CONF_VOICEMAIL_NUMBER);
                    }
                }
                Log.d(LOG_TAG, "Get global config vm number[" + getPhoneId() + "]: " + number);
            }
        }
        /* @} */

        if (TextUtils.isEmpty(number)) {
            String[] listArray = getContext().getResources()
                .getStringArray(com.android.internal.R.array.config_default_vm_number);
            if (listArray != null && listArray.length > 0) {
                for (int i=0; i<listArray.length; i++) {
                    if (!TextUtils.isEmpty(listArray[i])) {
                        String[] defaultVMNumberArray = listArray[i].split(";");
                        if (defaultVMNumberArray != null && defaultVMNumberArray.length > 0) {
                            if (defaultVMNumberArray.length == 1) {
                                number = defaultVMNumberArray[0];
                            } else if (defaultVMNumberArray.length == 2 &&
                                    !TextUtils.isEmpty(defaultVMNumberArray[1]) &&
                                    isMatchGid(defaultVMNumberArray[1])) {
                                number = defaultVMNumberArray[0];
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (!isPhoneTypeGsm() && TextUtils.isEmpty(number)) {
            // Read platform settings for dynamic voicemail number
            if (getContext().getResources().getBoolean(com.android.internal
                    .R.bool.config_telephony_use_own_number_for_voicemail)) {
                number = getLine1Number();
            } else {
                number = "*86";
            }
        }

        return number;
    }

    public Connection dial(String dialString, UUSInfo uusInfo, int videoState, Bundle intentExtras)
            throws CallStateException {
        if (!isPhoneTypeGsm() && uusInfo != null) {
            throw new CallStateException("Sending UUS information NOT supported in CDMA!");
        }

        boolean isEmergency = PhoneNumberUtils.isEmergencyNumber(dialString);
        Phone imsPhone = mImsPhone;

        CarrierConfigManager configManager =
                (CarrierConfigManager) mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        boolean alwaysTryImsForEmergencyCarrierConfig = configManager.getConfigForSubId(getSubId())
                .getBoolean(CarrierConfigManager.KEY_CARRIER_USE_IMS_FIRST_FOR_EMERGENCY_BOOL);

        boolean imsUseEnabled = isImsUseEnabled()
                 && imsPhone != null
                 && (imsPhone.isVolteEnabled() || imsPhone.isWifiCallingEnabled() ||
                 (imsPhone.isVideoEnabled() && VideoProfile.isVideo(videoState)))
                 && (imsPhone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE);

        boolean useImsForEmergency = imsPhone != null
                && isEmergency
                && alwaysTryImsForEmergencyCarrierConfig
                && ImsManager.isNonTtyOrTtyOnVolteEnabled(mContext)
                && (imsPhone.getServiceState().getState() != ServiceState.STATE_POWER_OFF)
                // SPRD: add for bug 594286, Trying (non-IMS) CS Ecall when (non-IMS) in service
                && !(getServiceState().getState() == ServiceState.STATE_IN_SERVICE
                && imsPhone.getServiceState().getState() != ServiceState.STATE_IN_SERVICE);

        String dialPart = PhoneNumberUtils.extractNetworkPortionAlt(PhoneNumberUtils.
                stripSeparators(dialString));
        boolean isUt = (dialPart.startsWith("*") || dialPart.startsWith("#"))
                && dialPart.endsWith("#");

        boolean useImsForUt = imsPhone != null && imsPhone.isUtEnabled();

        if (DBG) {
            logd("imsUseEnabled=" + imsUseEnabled
                    + ", useImsForEmergency=" + useImsForEmergency
                    + ", useImsForUt=" + useImsForUt
                    + ", isUt=" + isUt
                    + ", imsPhone=" + imsPhone
                    + ", imsPhone.isVolteEnabled()="
                    + ((imsPhone != null) ? imsPhone.isVolteEnabled() : "N/A")
                    + ", imsPhone.isVowifiEnabled()="
                    + ((imsPhone != null) ? imsPhone.isWifiCallingEnabled() : "N/A")
                    + ", imsPhone.isVideoEnabled()="
                    + ((imsPhone != null) ? imsPhone.isVideoEnabled() : "N/A")
                    + ", imsPhone.getServiceState().getState()="
                    + ((imsPhone != null) ? imsPhone.getServiceState().getState() : "N/A"));
        }

        Phone.checkWfcWifiOnlyModeBeforeDial(mImsPhone, mContext);

        if ((imsUseEnabled && (!isUt || useImsForUt)) || useImsForEmergency) {
            try {
                if (DBG) logd("Trying IMS PS call");
                return imsPhone.dial(dialString, uusInfo, videoState, intentExtras);
            } catch (CallStateException e) {
                if (DBG) logd("IMS PS call exception " + e +
                        "imsUseEnabled =" + imsUseEnabled + ", imsPhone =" + imsPhone);
                if (!Phone.CS_FALLBACK.equals(e.getMessage())) {
                    CallStateException ce = new CallStateException(e.getMessage());
                    ce.setStackTrace(e.getStackTrace());
                    throw ce;
                }
            }
        }

        if (mSST != null && mSST.mSS.getState() == ServiceState.STATE_OUT_OF_SERVICE
                && mSST.mSS.getDataRegState() != ServiceState.STATE_IN_SERVICE && !isEmergency) {
            throw new CallStateException("cannot dial in current state");
        }
        if (DBG) logd("Trying (non-IMS) CS call");

        return dialInternal(dialString, null, videoState, intentExtras);
    }

    @Override
    protected Connection dialInternal(String dialString, UUSInfo uusInfo, int videoState,
                                      Bundle intentExtras)
            throws CallStateException {

        // Need to make sure dialString gets parsed properly
        String newDialString = PhoneNumberUtils.stripSeparators(dialString);

        if (isPhoneTypeGsm()) {
            // handle in-call MMI first if applicable
            if (handleInCallMmiCommands(newDialString)) {
                return null;
            }

            intentExtras.putShort(VIDEO_STATE, (short) videoState);
            // Only look at the Network portion for mmi
            String networkPortion = PhoneNumberUtils.extractNetworkPortionAlt(newDialString);
            GsmMmiCode mmi =
                    GsmMmiCode.newFromDialString(networkPortion, this, mUiccApplication.get());
            if (DBG) logd("dialing w/ mmi '" + mmi + "'...");

            if (mmi == null) {
                return mCT.dial(newDialString, uusInfo, intentExtras);
            } else if (mmi.isTemporaryModeCLIR()) {
                return mCT.dial(mmi.mDialingNumber, mmi.getCLIRMode(), uusInfo, intentExtras);
            } else {
                ArrayList <MmiCode> mmis = (ArrayList <MmiCode>)getPendingMmiCodes();
                mmis.add(mmi);
                mMmiRegistrants.notifyRegistrants(new AsyncResult(null, mmi, null));
                try {
                    mmi.processCode();
                } catch (CallStateException e) {
                    //do nothing
                }

                // FIXME should this return null or something else?
                return null;
            }
        } else {
            return mCT.dial(newDialString);
        }
    }

    @Override
    public void acceptCall(int videoState) throws CallStateException {
        Phone imsPhone = mImsPhone;
        if ( imsPhone != null && imsPhone.getRingingCall().isRinging() ) {
            imsPhone.acceptCall(videoState);
        } else {
            for (Connection c : getRingingCall().getConnections()) {
                if (videoState == 0 && c != null
                        && c.getVideoState() == VideoProfile.STATE_BIDIRECTIONAL) {
                    ((GsmCdmaCallTrackerEx)mCT).fallBack();
                    return;
                }
            }
            mCT.acceptCall();
        }
    }
    /* SPRD: fix bug 513309 @{ */
    @Override
    public void notifyNewRingingConnection(Connection c) {
         boolean supportVT = SystemProperties.getBoolean("persist.sys.support.vt", false);
         boolean supportCSVT = SystemProperties.getBoolean("persist.sys.csvt", false);
         if (c.getVideoState() == VideoProfile.STATE_BIDIRECTIONAL && !supportVT && supportCSVT) {
             try {
                 ((GsmCdmaCallTrackerEx)mCT).fallBack();
             } catch (CallStateException ex) {
                  logd("WARN: unexpected error on fallback   "+ ex);
             }
         }else{
             super.notifyNewRingingConnectionP(c);
         }
    }
    /* @} */
    private void logd(String s) {
        Rlog.d(TAG, "[GsmCdmaPhoneEx] " + s);
    }

    @Override
    public void onMMIDone(MmiCode mmi) {
        if(mmi != null && mmi instanceof GsmMmiCodeEx) {
            logd("onMMIDone " + mmi);
            ((GsmMmiCodeEx)mmi).releaseNetWorkRequest(false);
        }
        super.onMMIDone(mmi);
    }
}
