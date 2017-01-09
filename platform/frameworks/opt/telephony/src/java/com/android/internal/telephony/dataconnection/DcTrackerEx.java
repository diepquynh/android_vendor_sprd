/* Created by Spreadst */

package com.android.internal.telephony.dataconnection;

import android.app.AlarmManager;
import android.app.AlertDialog;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.os.AsyncResult;
import android.os.BatteryManager;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.Rlog;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionManager.OnSubscriptionsChangedListener;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Pair;

import com.android.internal.telephony.DctConstants;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.dataconnection.DcTracker;
import com.android.internal.telephony.plugin.DataConnectionUtils;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.sprd.telephony.RadioInteractor;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;

public final class DcTrackerEx extends DcTracker
        implements DataConnectionUtils.RadioInteractorCallback {
    protected final String LOG_TAG = "DctEx";
    private static final boolean DBG = true;

    protected static final String INTENT_CLEANUP_DATA_ALARM =
            "com.sprd.telephony.data-cleanup";
    protected static final int DATA_KEEP_ALIVE_DURATION = 30 * 60 * 1000; // 30 min
    private static final String DATA_ALWAYS_ONLINE = "data_always_online";

    private static final int DATA_ROAM_DISABLE = 0;
    private static final int DATA_ROAM_NATIONAL = 1;
    private static final int DATA_ROAM_ALL = 2;

    private static final String INTENT_RETRY_CLEAR_CODE =
            "com.sprd.internal.telephony.data-retry-clear-code";
    private static final String INTENT_RETRY_CLEAR_CODE_EXTRA_TYPE = "retry_clear_code_extra_type";
    private static final String INTENT_RETRY_CLEAR_CODE_EXTRA_REASON =
            "retry_clear_code_extra_reason";
    private static final String INTENT_RETRY_FROM_FAILURE =
            "com.sprd.internal.telephony.data-retry-from-failure";
    private static final String INTENT_RETRY_FROM_FAILURE_ALARM_EXTRA_TYPE =
            "retry_from_faliure_alarm_extra_type";

    /* Copy from DcTracker */
    private Phone mPhone;
    private final AlarmManager mAlarmManager;
    private boolean mIsScreenOn = true;
    private ContentResolver mResolver;

    /* Whether data is suspended */
    protected boolean mSuspended = false;
    /* Whether network is shared */
    private boolean mNetworkShared = false;
    /* Whether the phone is charging */
    private boolean mCharging = false;
    /* Whether always online */
    private boolean mAlwaysOnline = true;
    /* Whether in Deep Sleep Mode */
    private boolean mDeepSleep = false;
    /* Handles AOL settings change */
    private AolObserver mAolObserver;
    /* Alarm before going to deep sleep */
    private PendingIntent mCleaupAlarmIntent = null;
    /* Various configs */
    private DctConfig mConfig;
    private SharedPreferences mSharedPrefs;

    // Plugin interface
    private DataConnectionUtils mUtils;

    private boolean mSupportTrafficClass;

    protected boolean mSupportSpecialClearCode = false;
    private DcFailCause mPreFailcause = null;
    private AtomicInteger mClearCodeLatch = new AtomicInteger(0);
    private PendingIntent mRetryIntent = null;
    private AlertDialog mErrorDialog = null;
    private ClearCodeRetryController mRetryController;
    private int mFailCount = 0;
    private int MAX_RETRY = 3;
    //SPRD: bug618350 add single pdp allowed by plmns feature
    private boolean isOnlySingleDcAllowed = false;

    // Interface to radio interactor
    private RadioInteractor mRi;

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            log("onReceive() action=" + action);
            if (action.equals(Intent.ACTION_SCREEN_ON)) {
                onScreenOn();
            } else if (action.equals(Intent.ACTION_SCREEN_OFF)) {
                onScreenOff();
            } else if (action.equals(ConnectivityManager.ACTION_TETHER_STATE_CHANGED)) {
                ArrayList<String> tetheredIf = intent.getStringArrayListExtra(
                        ConnectivityManager.EXTRA_ACTIVE_TETHER);
                onNetworkShared(tetheredIf != null && !tetheredIf.isEmpty());
            } else if (action.equals(Intent.ACTION_BATTERY_CHANGED)) {
                boolean charging = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, 0) != 0;
                onBatteryChanged(charging);
            } else if (action.equals(INTENT_CLEANUP_DATA_ALARM)) {
                onActionIntentCleanupData();
            } else if (action.startsWith(INTENT_RETRY_CLEAR_CODE)) {
                if (DBG) log("Retry for clear code");
                onActionIntentRetryClearCode(intent);
            } else if (action.startsWith(INTENT_RETRY_FROM_FAILURE)) {
                if (DBG) log("Retry from previous failure");
                onActionIntentRetryFromFailure(intent);
            }
        }
    };

    private SubscriptionManager mSubscriptionManager;
    private final OnSubscriptionsChangedListener mOnSubscriptionsChangedListener =
            new OnSubscriptionsChangedListener() {
        @Override
        public void onSubscriptionsChanged() {
            int subId = mPhone.getSubId();
            log("SubscriptionListener.onSubscriptionInfoChanged, subId=" + subId);
            if (SubscriptionManager.isValidSubscriptionId(subId)) {
                mAlwaysOnline = mSharedPrefs.getBoolean(DATA_ALWAYS_ONLINE + subId, true);
                if (mAolObserver != null) {
                    mAolObserver.unregister();
                }
                mAolObserver = new AolObserver(subId);
                mAolObserver.register();
            }
        }
    };

    private class AolObserver implements SharedPreferences.OnSharedPreferenceChangeListener {
        private String mKey = DATA_ALWAYS_ONLINE;

        public AolObserver(int subId) {
            mKey = DATA_ALWAYS_ONLINE + mPhone.getSubId();
        }

        public void register() {
            mSharedPrefs.registerOnSharedPreferenceChangeListener(this);
        }

        public void unregister() {
            mSharedPrefs.unregisterOnSharedPreferenceChangeListener(this);
        }

        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            if (mKey.equals(key)) {
                boolean aol = mSharedPrefs.getBoolean(mKey, true);
                if (mAlwaysOnline != aol) {
                    log("Always online -> " + aol);
                    if (aol) {
                        cancelAlarmForDeepSleep();
                        if (mDeepSleep) {
                            onTrySetupData(Phone.REASON_DATA_ENABLED);
                            mDeepSleep = false;
                        }
                    } else if (!mIsScreenOn && !mCharging) {
                        startAlarmForDeepSleep();
                    }
                    mAlwaysOnline = aol;
                }
            }
        }
    }

    public DcTrackerEx(Phone p) {
        super(p);
        mPhone = p;
        if (DBG) log("DctEx.constructor");
        mResolver = p.getContext().getContentResolver();

        DataPhoneManager.getInstance().registerPhone(p);

        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(ConnectivityManager.ACTION_TETHER_STATE_CHANGED);
        filter.addAction(Intent.ACTION_BATTERY_CHANGED);
        filter.addAction(INTENT_CLEANUP_DATA_ALARM);
        mPhone.getContext().registerReceiver(mReceiver, filter, null, mPhone);

        mAlarmManager =
                (AlarmManager) p.getContext().getSystemService(Context.ALARM_SERVICE);

        mSharedPrefs = PreferenceManager.getDefaultSharedPreferences(mPhone.getContext());
        mSubscriptionManager = SubscriptionManager.from(mPhone.getContext());
        mSubscriptionManager
                .addOnSubscriptionsChangedListener(mOnSubscriptionsChangedListener);

        mConfig = DctConfig.getInstance(p.getContext());

        mUtils = DataConnectionUtils.getInstance(mPhone.getContext());
        mSupportTrafficClass = mUtils.supportTrafficClass();
        if (mSupportTrafficClass) {
            mUtils.addRadioInteractorCallback(this);
        }

        mSupportSpecialClearCode = mUtils.supportSpecialClearCode();
        if (mSupportSpecialClearCode) {
            for (ApnContext apnContext : mApnContexts.values()) {
                IntentFilter f = new IntentFilter();
                f.addAction(INTENT_RETRY_CLEAR_CODE + '.' + apnContext.getApnType());
                f.addAction(INTENT_RETRY_FROM_FAILURE + '.' + apnContext.getApnType());
                p.getContext().registerReceiver(mReceiver, f, null, p);
            }

            mRetryController = new ClearCodeRetryController(p, this);
        }
    }

    @Override
    public void dispose() {
        if (DBG) log("DctEx.dispose");
        super.dispose();
        DataPhoneManager.getInstance().unregisterPhone(mPhone);
        if (mAolObserver != null) {
            mAolObserver.unregister();
        }
        mSubscriptionManager
                .removeOnSubscriptionsChangedListener(mOnSubscriptionsChangedListener);
        if (mSupportTrafficClass) {
            mUtils.removeRadioInteractorCallback(this);
        }
        mConfig.dispose();
        if (mRetryController != null) {
            mRetryController.dispose();
            mRetryController = null;
        }
    }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
            case DctConstants.EVENT_DATA_SETUP_COMPLETE:
                onDataSetupComplete((AsyncResult) msg.obj);
                break;

            case DctConstants.EVENT_RADIO_OFF_OR_NOT_AVAILABLE:
                onRadioOffOrNotAvailable();
                break;

            case DctConstants.EVENT_DATA_RAT_CHANGED:
                if (mRetryController != null) {
                    AsyncResult ar = (AsyncResult) msg.obj;
                    Pair<Integer, Integer> drsRatPair = (Pair<Integer, Integer>) ar.result;
                    mRetryController.handleDataServiceChange(drsRatPair.second,
                            drsRatPair.first);
                }
                break;
        }
        super.handleMessage(msg);
    }

    @Override
    public boolean isSuspended() {
        return mSuspended;
    }

    @Override
    public boolean isDeepSleep() {
        return mDeepSleep;
    }

    @Override
    protected boolean checkVendorDataAllowed() {
        // Phone is in DSM, don't allow data
        if (mDeepSleep) {
            log("not allowed - deep sleep");
            return false;
        }

        // If support traffic class, we must wait until radio interactor service is started
        if (mSupportTrafficClass && mRi == null) {
            log("not allowed - RI not connected");
            return false;
        }

        // Clear code 33/29
        if (mUtils.isSpecialCode(mPreFailcause)) {
            if (mFailCount >= MAX_RETRY || mClearCodeLatch.get() > 0) {
                // not allow data if the retrying timer doesn't expire or exceed max retry count
                log("not allowed - clear code " + mPreFailcause);
                return false;
            }
        }

        return true;
    }
    /*SPRD: bug608747 add esm flag feature @{*/
    @Override
    public int getEsmFlag(String operator) {
        int esmFlag = 0;
        String plmnsConfig = "";
        CarrierConfigManagerEx carrierConfig = CarrierConfigManagerEx.from(mPhone.getContext());
        if (carrierConfig != null) {
            PersistableBundle config = carrierConfig.getConfigForPhoneId(mPhone.getPhoneId());
            if (config != null){
                plmnsConfig = config.getString(
                        CarrierConfigManagerEx.KEY_FEATURE_PLMNS_ESM_FLAG_STRING, "");
            }
        }
        if (DBG) {
            log("plmnsConfig = " + plmnsConfig);
        }
        if (!TextUtils.isEmpty(plmnsConfig)) {
            String[] plmns = plmnsConfig.split(",");
            for (int i = 0; i < plmns.length; i++) {
                if (!TextUtils.isEmpty(plmns[i]) && plmns[i].equals(operator)) {
                    esmFlag = 1;
                    break;
                }
            }
        }
        return esmFlag;
    }

    @Override
    public boolean getAttachApnEnable() {
        boolean attachApnEnable = true;
        CarrierConfigManagerEx carrierConfig = CarrierConfigManagerEx.from(mPhone.getContext());
        if (carrierConfig != null) {
            PersistableBundle config = carrierConfig.getConfigForPhoneId(mPhone.getPhoneId());
            if (config != null){
                attachApnEnable = config.getBoolean(
                        CarrierConfigManagerEx.KEY_FEATURE_ATTACH_APN_ENABLE_BOOL, true);
            }
        }
        return attachApnEnable;
    }
    /* @} */
    /*SPRD: bug618350 add single pdp allowed by plmns feature@{*/
    public void setSinglePDNAllowedByNetwork(String plmn) {
        String plmnsConfig = "";
        CarrierConfigManagerEx carrierConfig = CarrierConfigManagerEx.from(mPhone.getContext());
        if (carrierConfig != null) {
            PersistableBundle config = carrierConfig.getConfigForPhoneId(mPhone.getPhoneId());
            if (config != null){
                plmnsConfig = config.getString(CarrierConfigManagerEx.KEY_PLMNS_SINGLE_PDN, "");
            }
        }
        if (DBG) {
            log("isOnlySingleDcAllowed = " + plmnsConfig);
        }
        if (!TextUtils.isEmpty(plmnsConfig)) {
            String[] plmns = plmnsConfig.split(",");
            for (int i = 0; i < plmns.length; i++) {
                if (!TextUtils.isEmpty(plmns[i]) && plmns[i].equals(plmn)) {
                    isOnlySingleDcAllowed = true;
                    break;
                }
            }
        }
        RadioInteractor ri = new RadioInteractor(mPhone.getContext());
        if (ri != null) {
            ri.requestSetSinglePDNByNetwork(isOnlySingleDcAllowed, mPhone.getPhoneId());
        }else {
            log("ri is null, do nothings!");
        }
    }
    public boolean isSinglePDNAllowedByNetwork(){
        return isOnlySingleDcAllowed;
    }
    /* @} */
    /*
     * Do some preparation before setting up data call, for exmaple set traffic class
     */
    @Override
    public void prepareDataCall(ApnSetting apnSetting) {
        if (mSupportTrafficClass) {
            if (mRi != null) {
                try {
                    String trafficClass = apnSetting.getTrafficClass();
                    if (trafficClass != null) {
                        int tc = Integer.parseInt(trafficClass);
                        mRi.requestDCTrafficClass(tc, mPhone.getPhoneId());
                    }
                } catch (NumberFormatException e) {
                    log("Error: illegal traffic class value");
                }
            } else {
                log("Error: radio interactor service not connected");
            }
        }
    }

    @Override
    protected void onCreateApnSetting(Cursor cursor, ApnSetting apn) {
        if (mSupportTrafficClass) {
            if (apn != null && cursor != null) {
                 int trafficClassIndex = cursor.getColumnIndex("traffic_class");
                 if (trafficClassIndex >= 0) {
                     apn.setTrafficClass(cursor.getString(trafficClassIndex));
                 }
            }
        }
    }

    @Override
    public void onRiConnected(RadioInteractor ri) {
        mRi = ri;
        log("Radio interactor connected, proceed to setup data call");
        onTrySetupData(Phone.REASON_DATA_ENABLED);
    }

    /**
     * Return current {@link android.provider.Settings.Global#DATA_ROAMING} value.
     */
    @Override
    public boolean getDataOnRoamingEnabled() {
        boolean isDataRoamingEnabled = "true".equalsIgnoreCase(SystemProperties.get(
                "ro.com.android.dataroaming", "false"));
        final int phoneSubId = mPhone.getSubId();
        int dataRoamingSetting = DATA_ROAM_DISABLE;

        try {
            // For single SIM phones, this is a per phone property.
            if (TelephonyManager.getDefault().getSimCount() == 1) {
                dataRoamingSetting = Settings.Global.getInt(mResolver,
                        Settings.Global.DATA_ROAMING, isDataRoamingEnabled ? 1 : 0);
            } else {
                dataRoamingSetting = TelephonyManager.getIntWithSubId(mResolver,
                        Settings.Global.DATA_ROAMING, phoneSubId);
            }
        } catch (SettingNotFoundException snfe) {
            if (DBG) log("getDataOnRoamingEnabled: SettingNofFoundException snfe=" + snfe);
        }

        if (DBG) log("getDataOnRoamingEnabled, settingValue: " + dataRoamingSetting);
        switch (dataRoamingSetting) {
            case DATA_ROAM_ALL:
                isDataRoamingEnabled = true;
                break;

            case DATA_ROAM_NATIONAL:
                if (mConfig != null && mConfig.isNationalDataRoamingEnabled()) {
                    if (mPhone.getServiceState().getDataRoamingType()
                            != ServiceState.ROAMING_TYPE_DOMESTIC) {
                        isDataRoamingEnabled = false;
                    }
                }
                break;

            default:
                isDataRoamingEnabled = false;
                break;
        }

        if (DBG) {
            log("getDataOnRoamingEnabled: phoneSubId=" + phoneSubId +
                    " isDataRoamingEnabled=" + isDataRoamingEnabled);
        }
        return isDataRoamingEnabled;
    }

    @Override
    protected boolean handleSpecialClearCode(DcFailCause cause, ApnContext apnContext) {
        log("handleSpecialClearCode(" + cause + ")");
        mPreFailcause = cause;
        if (mUtils.isSpecialCode(cause)) {
            // Handle special clear code
            mFailCount++;
            ApnSetting apn = apnContext.getApnSetting();
            log("[ClearCode] mFailCount=" + mFailCount + ", apn=" + apn);
            if (mFailCount < MAX_RETRY) {
                log("[ClearCode] next retry");
                apnContext.setState(DctConstants.State.SCANNING);
                if (apn != null) {
                    apn.permanentFailed = false;
                }
                // If the fail count < 3, retry it after some time
                int delay = DataConnectionUtils.RETRY_DELAY_LONG;
                if (is4G()) {
                    delay = DataConnectionUtils.RETRY_DELAY_SHORT;
                }
                log("[ClearCode] retry connect APN delay=" + delay);
                startAlarmForRetryClearCode(delay, apnContext);
            } else {
                log("[ClearCode] process fail");
                if (is4G()) {
                    log("[ClearCode] In 4G, switch to 3G");
                    if (mRetryController != null) {
                        mRetryController.switchTo3G();
                    }
                } else {
                    log("[ClearCode] Max retry reached, remove all waiting apns");
                    apnContext.setState(DctConstants.State.FAILED);
                    apnContext.setWaitingApns(new ArrayList<ApnSetting>());
                    mPhone.notifyDataConnection(Phone.REASON_APN_FAILED, apnContext.getApnType());
                    apnContext.setDataConnectionAc(null);

                    log("[ClearCode] All retry attempts failed, show user notification");
                    userNotification(cause);
                    startAlarmForRetryFromFailure(DataConnectionUtils.RETRY_FROM_FAILURE_DELAY,
                            apnContext);
                }
            }
            // We will do special retry, so stop other retry timer
            cancelAllReconnectAlarms();
            // return true and don't proceed to onDataSetupCompleteError
            return true;
        } else if (mSupportTrafficClass) {
            if (cause == DcFailCause.MISSING_UNKNOWN_APN) {
                ApnSetting apn = apnContext.getApnSetting();
                if (apn != null) {
                    apn.permanentFailed = false;
                }
            }
            return false;
        }

        return false;
    }

    void suspend(String reason) {
        log("suspend(" + reason + ")");
        mSuspended = true;
        suspendData(reason);
    }

    void resume(String reason) {
        log("resume(" + reason + ")");
        mSuspended = false;
        resumeData(reason);
    }

    private void onScreenOn() {
        mIsScreenOn = true;
        cancelAlarmForDeepSleep();
        if (mDeepSleep) {
            mDeepSleep = false;
            onTrySetupData(Phone.REASON_DATA_ENABLED);
        }
    }

    private void onScreenOff() {
        mIsScreenOn = false;
        if (!mAlwaysOnline && !mCharging) {
            startAlarmForDeepSleep();
        }
    }

    private void onBatteryChanged(boolean charging) {
        if (charging != mCharging) {
            mCharging = charging;
            if (charging) {
                cancelAlarmForDeepSleep();
                // Do we need to re-connecte data when charger is plugged?
                // Currently the screen is turned on when charger is plugged,
                // data will be re-connected in onScreenOn()
            } else if (!mIsScreenOn && !mAlwaysOnline) {
                startAlarmForDeepSleep();
            }
        }
    }

    private void onNetworkShared(boolean shared) {
        mNetworkShared = shared;
    }

    private void onActionIntentCleanupData() {
        cleanUpDataForDeepSleep();
    }

    /* If mobile data always online setting is off and
     *   1) screen is off,
     *   2) battery is not charging,
     * the data will be disconnected after 30min
     */
    private void startAlarmForDeepSleep() {
        Intent intent = new Intent(INTENT_CLEANUP_DATA_ALARM);
        mCleaupAlarmIntent = PendingIntent.getBroadcast(mPhone.getContext(), 0,
                                        intent, PendingIntent.FLAG_UPDATE_CURRENT);
        int delay = mConfig.getDataKeepAliveDuration(DATA_KEEP_ALIVE_DURATION);

        if (DBG) {
            log("startAlarmForDeepSleep delay=" + delay);
        }

        mAlarmManager.setExact(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                SystemClock.elapsedRealtime() + delay, mCleaupAlarmIntent);
    }

    private void cancelAlarmForDeepSleep() {
        if (mCleaupAlarmIntent != null) {
            if (DBG) {
                log("cancelAlarmForDeepSleep");
            }
            mAlarmManager.cancel(mCleaupAlarmIntent);
            mCleaupAlarmIntent = null;
        }
    }

    private void cleanUpDataForDeepSleep() {
        if (!mNetworkShared && !mCharging) {
            cleanUpAllConnections(Phone.REASON_DATA_DISABLED);
            mDeepSleep = true;
        }
    }

    private void onTrySetupData(String reason) {
        sendMessage(obtainMessage(DctConstants.EVENT_TRY_SETUP_DATA, reason));
    }

    private void onDataSetupComplete(AsyncResult ar) {
        if (ar.exception == null) {
            mPreFailcause = null;
            mFailCount = 0;
        }
    }

    private void onRadioOffOrNotAvailable() {
        if (mUtils.isSpecialCode(mPreFailcause)) {
            // shutdown radio, so reset state
            if (mRetryController != null) {
                mRetryController.notifyRadioOffOrNotAvailable();
            }
            stopFailRetryAlarm();
        }
    }

    private void startAlarmForRetryClearCode(int delay, ApnContext apnContext) {
        String apnType = apnContext.getApnType();

        Intent intent = new Intent(INTENT_RETRY_CLEAR_CODE + "." + apnType);
        intent.putExtra(INTENT_RETRY_CLEAR_CODE_EXTRA_REASON, apnContext.getReason());
        intent.putExtra(INTENT_RETRY_CLEAR_CODE_EXTRA_TYPE, apnType);
        // SPRD: Bug 606723 Send broadcast with foregroud priority
        intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);

        // Get current sub id.
        int subId = SubscriptionManager.getDefaultDataSubscriptionId();
        intent.putExtra(PhoneConstants.SUBSCRIPTION_KEY, subId);

        if (DBG) {
            log("startAlarmForRetryClearCode: delay=" + delay + " action=" + intent.getAction()
                    + " apn=" + apnContext);
        }
        // When PDP activation failed with special clear codes, the UE should not issue another PDP
        // request during 45s(or 10s in 4G). We use this flag to lock out the data
        mClearCodeLatch.set(1);

        PendingIntent alarmIntent = PendingIntent.getBroadcast (mPhone.getContext(), 0,
                                        intent, PendingIntent.FLAG_UPDATE_CURRENT);
        mRetryIntent = alarmIntent;
        mAlarmManager.setExact(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                SystemClock.elapsedRealtime() + delay, alarmIntent);
    }

    private void startAlarmForRetryFromFailure(int delay, ApnContext apnContext) {
        String apnType = apnContext.getApnType();
        Intent intent = new Intent(INTENT_RETRY_FROM_FAILURE + "." + apnType);
        intent.putExtra(INTENT_RETRY_FROM_FAILURE_ALARM_EXTRA_TYPE, apnType);
        // SPRD: Bug 606723 Send broadcast with foregroud priority
        intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);

        if (DBG) {
            log("startAlarmForRetryFromFailure: delay=" + delay + " action="
                    + intent.getAction() + " apn=" + apnContext);
        }
        PendingIntent alarmIntent = PendingIntent.getBroadcast(
                mPhone.getContext(), 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT);
        mRetryIntent = alarmIntent;
        // Sometimes the timer not exact.
        mAlarmManager.setExact(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                SystemClock.elapsedRealtime() + delay, alarmIntent);
    }

    void cancelAllReconnectAlarms() {
        for (ApnContext apnContext : mApnContexts.values()) {
            PendingIntent intent = apnContext.getReconnectIntent();
            if (intent != null) {
                AlarmManager am =
                    (AlarmManager) mPhone.getContext().getSystemService(Context.ALARM_SERVICE);
                am.cancel(intent);
                apnContext.setReconnectIntent(null);
            }
        }
    }

    void stopFailRetryAlarm() {
        if (mRetryIntent != null) {
            mAlarmManager.cancel(mRetryIntent);
            mRetryIntent = null;
        }
    }

    void clearPreFailCause() {
        mPreFailcause = null;
        mFailCount = 0;
        mClearCodeLatch.set(0);
    }

    private void onActionIntentRetryClearCode(Intent intent) {
        // unlock data
        mClearCodeLatch.set(0);

        String reason = intent.getStringExtra(INTENT_RETRY_CLEAR_CODE_EXTRA_REASON);
        String apnType = intent.getStringExtra(INTENT_RETRY_CLEAR_CODE_EXTRA_TYPE);
        int phoneSubId = mPhone.getSubId();
        int currSubId = intent.getIntExtra(PhoneConstants.SUBSCRIPTION_KEY,
                SubscriptionManager.INVALID_SUBSCRIPTION_ID);
        log("onActionIntentRetryClearCode: currSubId = " + currSubId + " phoneSubId=" + phoneSubId);

        // Stop reconnect if not current subId is not correct.
        if (!SubscriptionManager.isValidSubscriptionId(currSubId) || (currSubId != phoneSubId)) {
            log("receive retry alarm but subId incorrect, ignore");
            return;
        }

        ApnContext apnContext = mApnContexts.get(apnType);

        if (DBG) {
            log("onActionIntentRetryClearCode: reason=" + reason +
                    " apnType=" + apnType + " apnContext=" + apnContext);
        }

        if ((apnContext != null) && (apnContext.isEnabled())) {
            apnContext.setReason(reason);
            sendMessage(obtainMessage(DctConstants.EVENT_TRY_SETUP_DATA, apnContext));
        }
    }

    private void onActionIntentRetryFromFailure(Intent intent) {
        mRetryIntent = null;
        String apnType = intent
                .getStringExtra(INTENT_RETRY_FROM_FAILURE_ALARM_EXTRA_TYPE);
        ApnContext apnContext = mApnContexts.get(apnType);
        if (DBG) {
            log("onActionIntentRetryFromFailure: apnType=" + apnType + " apnContext=" + apnContext);
        }
        // restart from begining
        if (mRetryController != null) {
            mRetryController.restartCycle(apnContext);
        }
    }

    private boolean is4G() {
        return mPhone.getServiceState().getRilVoiceRadioTechnology() ==
                 ServiceState.RIL_RADIO_TECHNOLOGY_LTE
                 || mPhone.getServiceState().getRilVoiceRadioTechnology() ==
                      ServiceState.RIL_RADIO_TECHNOLOGY_LTE_CA;
    }

    private void userNotification(DcFailCause cause) {
        if (mErrorDialog != null && mErrorDialog.isShowing()) {
            mErrorDialog.dismiss();
        }

        mErrorDialog = mUtils.getErrorDialog(cause);
        if (mErrorDialog != null) {
            mErrorDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
                @Override
                public void onDismiss(DialogInterface dialog) {
                    mErrorDialog = null;
                }
            });
            mErrorDialog.show();
        }
    }

    private void log(String s) {
        Rlog.d(LOG_TAG, "[" + mPhone.getPhoneId() + "]" + s);
    }
}
