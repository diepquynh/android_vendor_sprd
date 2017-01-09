
package com.android.internal.telephony.uicc.policy;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import com.android.internal.telephony.DataApnHelper;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.ProxyController;
import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.SubscriptionInfoUpdater;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.TelephonyPluginUtils;
import com.android.internal.telephony.uicc.RadioController;
import com.android.internal.telephony.uicc.SimEnabledController;
import com.dmyk.android.telephony.DmykAbsTelephonyManager;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.database.ContentObserver;
import android.os.Handler;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.provider.Settings;
import android.provider.SettingsEx;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.RadioAccessFamily;
import android.telephony.Rlog;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import android.text.TextUtils;
import android.util.Log;

public class PrimaryCardController extends Handler {

    private static final boolean DBG = true;
    private static final String TAG = "PrimaryCardController";

    private static final int PHONE_COUNT = TelephonyManager.getDefault().getPhoneCount();

    private static final int EVENT_SIM_STATE_CHANGED = 0;
    private static final int EVENT_RESET_RADIO_STATE_COMPLETE = 1;
    private static final int EVENT_SET_PRIMARY_CARD_TIMEOUT = 2;

    private static final int NEW_REQUEST_NONE = 0;
    private static final int NEW_REQUEST = 1;
    private static final int NEW_REQUEST_HOT_SWAP = 2;

    public static final String ICCIDS_PREFS_NAME = "msms.info.iccid";
    public static final String SIM_ICC_ID = "icc_id";
    // SPRD: Bug 590814 phone is the encryption process,don't display primary card dialog
    private static final String DECRYPT_STATE = "trigger_restart_framework";//the state indicate that decrypt the encrypted phone and trigger restart framework
    private TelephonyManager mTelephonyManager;

    private static PrimaryCardController mInstance;
    private static int mSimStateChangedFlag = 0;
    private static String[] mSimStates = new String[PHONE_COUNT];
    private static IccPolicy mIccPolicy;
    private int[] mNeedSetSimPhoneId = new int[PHONE_COUNT];

    private SubscriptionController mSubController;
    private Context mContext;
    private int mNewRequest = NEW_REQUEST_NONE;
    private boolean isFirstRun=true;

    // record set primary card status
    private int mSetPrimaryCardStatus;
    private RadioController mRC;

    private static final int SET_PC_STATUS_IDLE = 0;
    private static final int SET_PC_STATUS_PREPARING = 1;
    private static final int SET_PC_STATUS_PREPARED = 2;
    private static final int SET_PC_STATUS_APPLYING = 3;

    public static PrimaryCardController init(Context context) {
        Log.d(TAG, "--- init ---");
        synchronized (PrimaryCardController.class) {
            if (mInstance == null) {
                mInstance = new PrimaryCardController(context);
            } else {
                Log.wtf(TAG, "init() called multiple times!  mInstance = " + mInstance);
            }
        }

        SimEnabledController.init(context);

        return mInstance;
    }

    private PrimaryCardController(Context context) {
        mContext = context;

        // OperatorPolicy instance will be created instead of DefaultPolicy through plugins if
        // current version is operator version. e.g. CMCCPolicy will be created on CMCC version.
        mIccPolicy = IccPolicyFactory.getInstance(mContext).createIccPolicy();

        mRC = RadioController.getInstance();
        mRC.setRadioBusy(mContext, false);

        mSubController = SubscriptionController.getInstance();
        final IntentFilter filter = new IntentFilter(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        filter.addAction(TelephonyIntents.ACTION_SET_RADIO_CAPABILITY_DONE);
        filter.addAction(TelephonyIntents.ACTION_SET_RADIO_CAPABILITY_FAILED);
        mContext.registerReceiver(mReceiver, filter);
        /* SPRD: MODIFY FOR BUG 588569:SMS sending set value exception @{ */
        mTelephonyManager = (TelephonyManager) TelephonyManager.from(mContext);
        mContext.getContentResolver().registerContentObserver(
                Settings.Global.getUriFor(Settings.Global.DEVICE_PROVISIONED), false,
                mSetupWizardCompleteObserver);

        mContext.getContentResolver().registerContentObserver(
                Settings.Global.getUriFor(SettingsEx.GlobalEx.RADIO_BUSY), false,
                mRadioBusyObserver);

        for (int i = 0; i < PHONE_COUNT; i++) {
            mNeedSetSimPhoneId[i] = -1;
        }

        mSetPrimaryCardStatus = SET_PC_STATUS_IDLE;
    }

    private ContentObserver mSetupWizardCompleteObserver = new ContentObserver(new Handler()) {
        public void onChange(boolean selfChange) {
            super.onChange(selfChange);
            boolean isDeviceProvisioned = Settings.Global.getInt(mContext.getContentResolver(),
                    Settings.Global.DEVICE_PROVISIONED, 0) != 0;
            Log.d(TAG, "mSetupWizardCompleteObserver onChange : isDeviceProvisioned = "
                    + isDeviceProvisioned);
            if (isDeviceProvisioned) {
                autoSetDefaultPhones();
            }
        };
    };

    private ContentObserver mRadioBusyObserver = new ContentObserver(new Handler()) {
        public void onChange(boolean selfChange) {
            super.onChange(selfChange);
            if (DBG)
                Rlog.d(TAG, "Radio busy changed: " + mRC.isRadioBusy(mContext) + " new request: "
                        + mNewRequest);
            if (!mRC.isRadioBusy(mContext)) {
                if (mNewRequest != NEW_REQUEST_NONE) {
                    autoSetPrimaryCard(mNewRequest == NEW_REQUEST_HOT_SWAP);
                }
            }
        };
    };

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (DBG)
                Rlog.d(TAG, "receive broadcast : " + action);

            if (action.equals(TelephonyIntents.ACTION_SIM_STATE_CHANGED)) {
                int phoneId = intent.getIntExtra(PhoneConstants.PHONE_KEY,
                        SubscriptionManager.INVALID_PHONE_INDEX);
                String simState = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                if (DBG)
                    Rlog.d(TAG, "SIM_STATE_CHANGED: simState[" + phoneId + "] = " + simState);

                if (!SubscriptionManager.isValidPhoneId(phoneId)) {
                    return;
                }

                /*
                 * SPRD: Bug 537083 phone is the encryption process,don't display primary card
                 * dialog @{
                 */
                String decryptState = SystemProperties.get("vold.decrypt");
                Log.d(TAG, "decrypt state --> " + decryptState);
                if (!TextUtils.isEmpty(decryptState) && !DECRYPT_STATE.equals(decryptState)) {
                    return;
                }
                /* @} */
                if (IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(simState)
                        || IccCardConstants.INTENT_VALUE_ICC_LOADED.equals(simState)
                        || IccCardConstants.INTENT_VALUE_ICC_LOCKED.equals(simState)
                        || IccCardConstants.INTENT_VALUE_ICC_UNKNOWN.equals(simState)) {

                    // Hot-swap or Disable/Enable SIM
                    boolean isHotSwap = false;
                    if ((!TextUtils.isEmpty(mSimStates[phoneId])
                            && !mSimStates[phoneId].equals(simState))
                            && (IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(mSimStates[phoneId])
                                    || IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(simState))) {
                        isHotSwap = true;
                        if (mTelephonyManager.getPhoneCount() == 2) {
                            int currentSimState = mTelephonyManager.getSimState(phoneId);
                            Intent Dmyintent = new Intent(
                                    DmykAbsTelephonyManager.ACTION_SIM_STATE_CHANGED);
                            Dmyintent.putExtra(DmykAbsTelephonyManager.EXTRA_SIM_PHONEID, phoneId);
                            if (currentSimState > 5) {
                                Dmyintent.putExtra(DmykAbsTelephonyManager.EXTRA_SIM_STATE,
                                        DmykAbsTelephonyManager.SIM_STATE_UNKNOWN);
                            } else {
                                Dmyintent.putExtra(DmykAbsTelephonyManager.EXTRA_SIM_STATE,
                                        currentSimState);
                            }
                            context.sendBroadcast(Dmyintent);
                        }
                    }

                    if (DBG)
                        Log.d(TAG, "onReceive: isHotSwap = " + isHotSwap);

                    // Update SIM priorities when SIM state changed.
                    mIccPolicy.updateSimPriorities();
                    mSimStates[phoneId] = simState;

                    for (int i = 0; i < PHONE_COUNT; i++) {
                        if (mNeedSetSimPhoneId[i] == -1
                                && simState.equals(IccCardConstants.INTENT_VALUE_ICC_LOADED)) {
                            mNeedSetSimPhoneId[i] = phoneId;
                            break;
                        }
                    }

                    mSimStateChangedFlag |= (1 << phoneId);

                    sendMessage(obtainMessage(EVENT_SIM_STATE_CHANGED, mSimStateChangedFlag,
                            isHotSwap ? 1 : 0));
                }

            } else if (action.equals(TelephonyIntents.ACTION_SET_RADIO_CAPABILITY_DONE)
                    || action.equals(TelephonyIntents.ACTION_SET_RADIO_CAPABILITY_FAILED)) {
                onSetPrimaryCardFinished();
            }
        }
    };

    @Override
    public void handleMessage(Message msg) {
        if (DBG)
            Rlog.d(TAG, "[handleMessage]: " + msg.what);
        switch (msg.what) {
            case EVENT_SIM_STATE_CHANGED:
                if (DBG)
                    Rlog.d(TAG, "[handleMessage] EVENT_SIM_STATE_CHANGED: mSimStateChangedFlag = "
                            + msg.arg1);
                if (msg.arg1 == ((1 << PHONE_COUNT) - 1)) {
                    handleBothSimStateChanged(msg.arg2 == 1);
                }
                break;
            case EVENT_RESET_RADIO_STATE_COMPLETE:
                onSetPrimaryCardPrepared(msg.arg1 == 1);
                break;
            case EVENT_SET_PRIMARY_CARD_TIMEOUT:
                onSetPrimaryCardFinished();
                break;
            default:
                if (DBG)
                    Rlog.d(TAG, "Unknown msg:" + msg.what);
        }
    }

    private void handleBothSimStateChanged(boolean isHotSwap) {
        boolean isDualSimChanged = false;

        // No need to save ICCID if any card is unknown because both SIMs may haven't
        // been changed.
        if (!isAnySimUnknown()) {
            SharedPreferences preferences = mContext.getSharedPreferences(ICCIDS_PREFS_NAME, 0);
            for (int i = 0; i < PHONE_COUNT; i++) {
                String lastIccId = preferences.getString(SIM_ICC_ID + i, null);
                String newIccId = getIccId(i);
                if (DBG)
                    Rlog.d(TAG, "[handleBothSimStateChanged] lastIccId = " + lastIccId
                            + " newIccId = " + newIccId);

                if (!isIccIdEquals(lastIccId, newIccId)) {
                    isDualSimChanged = true;

                    if (!isAnySimLocked()) {
                        SharedPreferences.Editor editor = preferences.edit();
                        editor.putString(SIM_ICC_ID + i, newIccId);
                        editor.commit();
                        if (DBG)
                            Rlog.d(TAG, "[handleBothSimStateChanged] SIM " + i
                                    + " changed, save new iccid: " + newIccId);
                    }
                }
            }
        }

        Rlog.d(TAG, "[handleBothSimStateChanged] isDualSimChanged = " + isDualSimChanged);

        if (isDualSimChanged) {
            // SPRD: MODIFY FOR BUG 588569:SMS sending set value exception
            autoSetDefaultPhones();
            autoSetPrimaryCard(isHotSwap);
        }
    }

    private void autoSetPrimaryCard(boolean isHotSwap) {
        // If already in process, just hold the new request and handle it after the previous one
        // complete.
        if (mSetPrimaryCardStatus != SET_PC_STATUS_IDLE || mRC.isRadioBusy(mContext)) {
            mNewRequest = isHotSwap ? NEW_REQUEST_HOT_SWAP : NEW_REQUEST;
        } else {
            mSetPrimaryCardStatus = SET_PC_STATUS_PREPARING;
            mNewRequest = NEW_REQUEST_NONE;
            mRC.setRadioBusy(mContext, true);
            resetRadioStateAccordingToSimState(isHotSwap);
        }
    }

    /**
     * Start to set primary card According to Icc Policy.
     */
    private void onSetPrimaryCardPrepared(boolean isHotSwap) {
        mSetPrimaryCardStatus = SET_PC_STATUS_PREPARED;

        boolean isCurrentPrimaryCardActive = mSubController
                .isActiveSubId(mSubController.getDefaultDataSubId());
        int primaryPhoneId = mIccPolicy.getPrimaryCardAccordingToPolicy();

        if (DBG)
            Log.d(TAG, "onSetPrimaryCardPrepared: isHotSwap = " + isHotSwap
                    + " isPrimaryCardActive = " + isCurrentPrimaryCardActive);

        // By default, do not automatically set primary card if current one is active after hot
        // swap.
        if (needSetPrimaryCardByOrder(isCurrentPrimaryCardActive) || !isHotSwap
                || !isCurrentPrimaryCardActive
                || getCarrierConfig().getBoolean(
                        CarrierConfigManagerEx.KEY_FORCE_AUTO_SET_PRIMARY_CARD_AFTER_HOT_SWAP)) {
            if (needSetPrimaryCardByOrder(isCurrentPrimaryCardActive)) {
                primaryPhoneId = mNeedSetSimPhoneId[0];
            }
            int primarySubId = mSubController.getSubIdUsingPhoneId(primaryPhoneId);

            if (DBG)
                Rlog.d(TAG, "[onSetPrimaryCardPrepared] setPrimaryCard: phoneId = "
                        + primaryPhoneId + " subId = " + primarySubId);

            if (SubscriptionManager.isValidSubscriptionId(primarySubId)) {
                setDefaultDataSubId(primarySubId);

                // Start the service to show APN config popup if SIMs changed.
                DataApnHelper.getInstance(mContext).showDataOrApnPopUpIfNeed();

                // Pop up SIM settings screen to prompt users it's available to set primary card
                // manually.
                if (mIccPolicy.isNeedPromptUserSetPrimaryCard() && isAllSimLoaded()) {
                    if (DBG)
                        Rlog.d(TAG,
                                "[onSetPrimaryCardPrepared] startActivity : SimSettingsActivity");
                    Intent intent = new Intent("com.android.settings.sim.SIM_SUB_INFO_SETTINGS");
                    intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
                    try {
                        // Avoid to prompt a new one if already exist an Activity which has been
                        // started by different contexts.
                        Context context = mContext.createPackageContext("com.android.settings",
                                Context.CONTEXT_IGNORE_SECURITY);
                        context.startActivity(intent);
                    } catch (NameNotFoundException e) {
                        Log.d(TAG, "Start SimSettingsActivity failed.");
                    }
                }

                sendEmptyMessageDelayed(EVENT_SET_PRIMARY_CARD_TIMEOUT, 125000);
            } else if (mSubController.getActiveSubInfoCount(mContext.getPackageName()) > 0) {
                setDefaultDataSubId(mSubController.getActiveSubIdList()[0]);
            }
        }

        if (mSetPrimaryCardStatus != SET_PC_STATUS_APPLYING) {
            // No thing to do indeed, just finish.
            onSetPrimaryCardFinished();
        }
    }

    private boolean needSetPrimaryCardByOrder(boolean isCurrentPrimaryCardActive) {
        if (PHONE_COUNT == 1 || isFirstRun) {
            return false;
        }
        /** SPRD: Bug 646102 auto set primary card don't need remember slot card */
        if (getCarrierConfig().getBoolean(
                CarrierConfigManagerEx.KEY_FORCE_AUTO_SET_PRIMARY_CARD_AFTER_HOT_SWAP)) {
            return false;
        }
        if (mNeedSetSimPhoneId[0] != mNeedSetSimPhoneId[1] && mNeedSetSimPhoneId[0] != -1
                && mNeedSetSimPhoneId[1] != -1) {
            return true;
        }
        if (isCurrentPrimaryCardActive && mNeedSetSimPhoneId[0] != -1
                && mNeedSetSimPhoneId[1] == -1) {
            mNeedSetSimPhoneId[0] = -1;
        }
        return false;
    }

    private void onSetPrimaryCardFinished() {
        if (DBG)
            Rlog.d(TAG, "[onSetPrimaryCardFinished]: mHasNewRequest = " + mNewRequest);
        isFirstRun=false;
        mSetPrimaryCardStatus = SET_PC_STATUS_IDLE;
        removeMessages(EVENT_SET_PRIMARY_CARD_TIMEOUT);
        for (int i = 0; i < PHONE_COUNT; i++) {
            mNeedSetSimPhoneId[i] = -1;
        }

        mRC.setRadioBusy(mContext, false);
    }

    private PersistableBundle getCarrierConfig() {
        CarrierConfigManagerEx configManagerEx = CarrierConfigManagerEx.from(mContext);
        if (configManagerEx != null) {
            return configManagerEx.getConfigForDefaultPhone();
        }
        return new PersistableBundle();
    }

    /**
     * Check whether radio state can match SIM status. If not, reset radios and don't set primary
     * card until radio operations are entirely complete.
     */
    private void resetRadioStateAccordingToSimState(final boolean isHotSwap) {
        boolean isAllSimAbsent = true;
        for (int phoneId = 0; phoneId < PHONE_COUNT; phoneId++) {
            if (hasIccCard(phoneId)) {
                isAllSimAbsent = false;
                break;
            }
        }

        if (DBG)
            Rlog.d(TAG, "[resetRadioState]: isAllSimAbsent = " + isAllSimAbsent);

        int[] ops = new int[PHONE_COUNT];
        for (int phoneId = 0; phoneId < PHONE_COUNT; phoneId++) {
            boolean desiredRadioState = false;
            if (isAllSimAbsent) {
                ProxyController proxyController = ProxyController.getInstance();
                desiredRadioState = proxyController.getRadioAccessFamily(phoneId) == proxyController
                        .getMaxRafSupported();
            } else {
                desiredRadioState = hasIccCard(phoneId);
            }

            desiredRadioState = desiredRadioState && !isAirplaneModeOn();

            ops[phoneId] = desiredRadioState ? RadioController.POWER_ON : RadioController.POWER_OFF;
        }

        mRC.setRadioPower(ops, obtainMessage(EVENT_RESET_RADIO_STATE_COMPLETE, isHotSwap ? 1 : 0, -1));
    }

    public void setDefaultDataSubId(int subId) {
        if (subId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID) {
            throw new RuntimeException("setDefaultDataSubId called with DEFAULT_SUB_ID");
        }

        if (!TelephonyPluginUtils.getInstance().needSkipSetRadioCapability(mContext,subId)) {
            Rlog.d(TAG,"set radioCapability");
            ProxyController proxyController = ProxyController.getInstance();

            boolean result = true;
            RadioAccessFamily[] rafs = new RadioAccessFamily[PHONE_COUNT];
            if (SubscriptionManager.isValidSubscriptionId(subId)) {
                // Only re-map modems if the new default data sub is valid
                boolean atLeastOneMatch = false;
                for (int phoneId = 0; phoneId < PHONE_COUNT; phoneId++) {
                    Phone phone = PhoneFactory.getPhone(phoneId);
                    int raf;
                    int id = phone.getSubId();
                    if (id == subId) {
                        // TODO Handle the general case of N modems and M subscriptions.
                        raf = proxyController.getMaxRafSupported();
                        atLeastOneMatch = true;
                    } else {
                        // TODO Handle the general case of N modems and M subscriptions.
                        raf = proxyController.getMinRafSupported();
                    }

                    if (DBG)
                        Rlog.d(TAG, "[setDefaultDataSubId] phoneId=" + phoneId + " subId=" + id
                                + " RAF=" + raf);
                    rafs[phoneId] = new RadioAccessFamily(phoneId, raf);
                }

                if (atLeastOneMatch) {
                    result = proxyController.setRadioCapability(rafs);
                } else {
                    if (DBG)
                        Rlog.d(TAG, "[setDefaultDataSubId] no valid subId's found - not updating.");
                }
            }

            if (DBG)
                Rlog.d(TAG, "[setDefaultDataSubId]: result=" + result);

            // Do not set default data subId if set radio capability failed.
            if (result) {
                boolean same = true;
                // Check we actually need to do anything
                for (int i = 0; i < PHONE_COUNT; i++) {
                    if (PhoneFactory.getPhone(i).getRadioAccessFamily() != rafs[i]
                            .getRadioAccessFamily()) {
                        same = false;
                        break;
                    }
                }

                if (!same) {
                    mSetPrimaryCardStatus = SET_PC_STATUS_APPLYING;
                }
            }
        }


        // FIXME is this still needed?
        for (int phoneId = 0; phoneId < PHONE_COUNT; phoneId++) {
            if (DBG)
                Rlog.d(TAG, "[updateAllDataConnectionTrackers] phoneId=" + phoneId);
            PhoneFactory.getPhone(phoneId).updateDataConnectionTracker();
        }

        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.MULTI_SIM_DATA_CALL_SUBSCRIPTION, subId);

        // Broadcast an Intent for default data sub change
        if (DBG)
            Rlog.d(TAG, "[broadcastDefaultDataSubIdChanged] subId=" + subId);
        Intent intent = new Intent(TelephonyIntents.ACTION_DEFAULT_DATA_SUBSCRIPTION_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING);
        intent.putExtra(PhoneConstants.SUBSCRIPTION_KEY, subId);
        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);

    }

    private boolean hasIccCard(int phoneId) {
        Phone phone = PhoneFactory.getPhone(phoneId);
        if (phone != null) {
            return phone.getIccCard().hasIccCard();
        }
        return false;
    }

    /* SPRD: auto set default sms/voice sub id @{ */
    /*
     * Set the default voice/sms sub to reasonable values if the user hasn't selected a sub or the
     * user selected sub is not present. This method won't change user preference.
     */
    private void autoSetDefaultPhones() {
        SubscriptionManager subManager = SubscriptionManager.from(mContext);
        List<SubscriptionInfo> activeSubInfoList = getActiveSubInfoList();
        List<SubscriptionInfo> subInfoList = subManager.getActiveSubscriptionInfoList();
        int defaultVoiceSubId = SubscriptionManager.getDefaultVoiceSubscriptionId();
        Log.d(TAG, "autoSetDefaultPhones: defaultVoiceSubId = " + defaultVoiceSubId);
        /* SPRD: modify for Bug608056 @{ */
        if (activeSubInfoList.size() < 1
                && subInfoList != null && subInfoList.size() > 0) {
            subManager.setDefaultVoiceSubId(subInfoList.get(0).getSubscriptionId());
        }
        /* @} */
        if (activeSubInfoList.size() > 0 && !isSubIdActive(defaultVoiceSubId)) {
            int subId = activeSubInfoList.get(0).getSubscriptionId();
            subManager.setDefaultVoiceSubId(subId);
            setDefaultVoiceSubId(subId);
        }

        int defaultSmsSubId = SubscriptionManager.getDefaultSmsSubscriptionId();
        Log.d(TAG, "autoSetDefaultPhones: defaultSmsSubId = " + defaultSmsSubId);
        if (!isSubIdActive(defaultSmsSubId) && activeSubInfoList.size() > 0) {
            subManager.setDefaultSmsSubId(activeSubInfoList.get(0).getSubscriptionId());
        }
    }

    private List<SubscriptionInfo> getActiveSubInfoList() {
        SubscriptionManager subManager = SubscriptionManager.from(mContext);
        List<SubscriptionInfo> availableSubInfoList = subManager.getActiveSubscriptionInfoList();
        if (availableSubInfoList == null) {
            return new ArrayList<SubscriptionInfo>();
        }
        Iterator<SubscriptionInfo> iterator = availableSubInfoList.iterator();
        while (iterator.hasNext()) {
            SubscriptionInfo subInfo = iterator.next();
            int phoneId = subInfo.getSimSlotIndex();
            boolean isSimReady = mTelephonyManager
                    .getSimState(phoneId) == TelephonyManager.SIM_STATE_READY;
            if (!isSimReady) {
                iterator.remove();
            }
        }
        return availableSubInfoList;
    }

    private boolean isSubIdActive(long subId) {
        List<SubscriptionInfo> activeSubInfoList = getActiveSubInfoList();

        for (SubscriptionInfo subInfo : activeSubInfoList) {
            if (subInfo.getSubscriptionId() == subId) {
                return true;
            }
        }
        return false;
    }

    private void setDefaultVoiceSubId(int subId) {
        Log.d(TAG, "setDefaultVoiceSubId = " + subId);
        TelecomManager telecomManager = TelecomManager.from(mContext);
        PhoneAccountHandle phoneAccountHandle = subscriptionIdToPhoneAccountHandle(subId);
        telecomManager.setUserSelectedOutgoingPhoneAccount(phoneAccountHandle);
    }

    private PhoneAccountHandle subscriptionIdToPhoneAccountHandle(final int subId) {
        final TelecomManager telecomManager = TelecomManager.from(mContext);
        final TelephonyManager telephonyManager = TelephonyManager.from(mContext);
        final Iterator<PhoneAccountHandle> phoneAccounts = telecomManager
                .getCallCapablePhoneAccounts().listIterator();

        while (phoneAccounts.hasNext()) {
            final PhoneAccountHandle phoneAccountHandle = phoneAccounts.next();
            final PhoneAccount phoneAccount = telecomManager.getPhoneAccount(phoneAccountHandle);

            if (telephonyManager.getSubIdForPhoneAccount(phoneAccount) == subId) {
                return phoneAccountHandle;
            }
        }
        return null;
    }
    /* @} */

    private boolean isAirplaneModeOn() {
        return Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
    }

    private boolean isIccIdEquals(String iccId1, String iccId2) {
        if (iccId1 != null) {
            return iccId1.equals(iccId2);
        } else {
            return iccId2 == null;
        }
    }

    /**
     * Do not use {@code PhoneFactory.getPhone(phoneId).getIccSerialNumber()} because it will return
     * null when SIM is locked. {@link SubscriptionInfoUpdater} will load ICCID additionally and
     * update database when all ICCIDs are loaded. So use {@link SubscriptionInfo} to get ICCID
     * instead.
     * 
     * @param phoneId
     * @return
     */
    private String getIccId(int phoneId) {
        // Do not use SubscriptionManager in phone process because it will cause unwanted RPC.
        SubscriptionController controller = SubscriptionController.getInstance();
        SubscriptionInfo subInfo = controller.getActiveSubscriptionInfoForSimSlotIndex(phoneId,
                mContext.getOpPackageName());
        if (subInfo != null) {
            return subInfo.getIccId();
        }
        return null;
    }

    private boolean isAllSimLoaded() {
        for (int i = 0; i < PHONE_COUNT; i++) {
            if (!IccCardConstants.INTENT_VALUE_ICC_LOADED.equals(mSimStates[i])) {
                return false;
            }
        }
        return true;
    }

    private boolean isAnySimUnknown() {
        for (int i = 0; i < PHONE_COUNT; i++) {
            if (IccCardConstants.INTENT_VALUE_ICC_UNKNOWN.equals(mSimStates[i])) {
                return true;
            }
        }
        return false;
    }

    private boolean isAnySimLocked() {
        for (int i = 0; i < PHONE_COUNT; i++) {
            if (IccCardConstants.INTENT_VALUE_ICC_LOCKED.equals(mSimStates[i])) {
                return true;
            }
        }
        return false;
    }
}
