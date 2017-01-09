package com.android.phone;

import java.util.Collections;
import java.util.Iterator;
import java.util.List;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.UserInfo;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.UserManager;
import android.telephony.PhoneStateListener;
import android.telephony.ServiceState;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager.OnSubscriptionsChangedListener;
import android.util.ArrayMap;
import android.util.Log;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.IccCard;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.phone.SubscriptionInfoHelper;
import com.android.phone.R;

public class CallForwardHelper {
    private static final String TAG = CallForwardHelper.class.getSimpleName();
    private static CallForwardHelper mInstance;
    private static final String KEY_ICC_ID = "cfu_icc_id";
    private static final String KEY_CFU_VOICE_VALUE = "cfu_voice_value";

    // Extra on intent containing the id of a subscription.
    private static final String SUB_ID_EXTRA =
            "com.android.phone.settings.SubscriptionInfoHelper.SubscriptionId";
    // Extra on intent containing the label of a subscription.
    private static final String SUB_LABEL_EXTRA =
            "com.android.phone.settings.SubscriptionInfoHelper.SubscriptionLabel";

    private PhoneGlobalsEx mApplication;
    private SharedPreferences mPreference;
    private Editor mEditor;
    private int[] mServiceState;
    TelephonyManager mTelephonyManager;
    SubscriptionManager mSubscriptionManager;
    private UserManager mUserManager;
    private NotificationManager mNotificationManager;
    /* SPRD: modify for bug526598 @{ */
    //final Map<Integer, Integer> mSubInfoMap =
    //        new LinkedHashMap<Integer, Integer>(TelephonyManager.getDefault().getPhoneCount());
    private ArrayMap<Integer, PhoneStateListener> mPhoneStateListeners =
            new ArrayMap<Integer, PhoneStateListener>();
    private boolean mFirstQuery = true;
    /* @} */
    private boolean mFirstCallBack = true;

    static final int CALL_FORWARD_NOTIFICATION = 4;
    private static final int[] sCallfwdIcon = new int[] {
        R.drawable.stat_sys_phone_call_forward_sub1_ex,
        R.drawable.stat_sys_phone_call_forward_sub2_ex,
    };

    public static CallForwardHelper getInstance() {
        if (mInstance == null) {
            mInstance = new CallForwardHelper();
        }
        return mInstance;
    }

    private CallForwardHelper() {
        mApplication = PhoneGlobalsEx.getInstance();
        int phoneCount = TelephonyManager.from(mApplication).getPhoneCount();
        mServiceState = new int[phoneCount];
        for (int i = 0; i < phoneCount; i++) {
            mServiceState[i] = ServiceState.STATE_POWER_OFF;
        }
        mSubscriptionManager = SubscriptionManager.from(mApplication);
        mTelephonyManager = (TelephonyManager) mApplication.getSystemService(
                Context.TELEPHONY_SERVICE);
        mUserManager = (UserManager) mApplication.getSystemService(Context.USER_SERVICE);
        mNotificationManager =
                (NotificationManager) mApplication.getSystemService(Context.NOTIFICATION_SERVICE);
        // SPRD: update file mode to default PRIVATE. See bug #462167.
        mPreference = mApplication.getApplicationContext().getSharedPreferences(TAG,
                mApplication.getApplicationContext().MODE_PRIVATE);
        mEditor = mPreference.edit();

        SubscriptionManager.from(mApplication).addOnSubscriptionsChangedListener(
                new OnSubscriptionsChangedListener() {
                    @Override
                    public void onSubscriptionsChanged() {
                        updatePhoneStateListeners();
                    }
                });
    }

    private boolean containsSubId(List<SubscriptionInfo> subInfos, int subId) {
        if (subInfos == null) {
            return false;
        }

        for (int i = 0; i < subInfos.size(); i++) {
            if (subInfos.get(i).getSubscriptionId() == subId) {
                return true;
            }
        }
        return false;
    }

    /* SPRD: modify for bug526598 @{
    protected boolean hasCorrectSubinfo(List<SubscriptionInfo> allSubscriptions) {
        for (SubscriptionInfo info : allSubscriptions) {
            log("hasCorrectSubinfo()        info.subId = " + info.getSubscriptionId());
            if (!mSubInfoMap.containsKey(info.getSubscriptionId())) {
                mSubInfoMap.put(info.getSubscriptionId(), 1);
                return true;
            }
        }
        return false;
    }*/

    protected void updatePhoneStateListeners() {
        List<SubscriptionInfo> subscriptions = SubscriptionManager.from(
                mApplication).getActiveSubscriptionInfoList();

        // Unregister phone listeners for inactive subscriptions.
        /* SPRD: modify for bug526598 @{ */
        // Iterator<Integer> itr = mSubInfoMap.keySet().iterator();
        Iterator<Integer> itr = mPhoneStateListeners.keySet().iterator();
        /* @} */
        while (itr.hasNext()) {
            int subId = itr.next();
            if (subscriptions == null || !containsSubId(subscriptions, subId)) {
                /* SPRD: modify for bug526598 @{ */
                log("set Listening to LISTEN_NONE and removes the listener.");
                // Hide the outstanding notifications.

                updateCfi(subId, false);

                // Listening to LISTEN_NONE removes the listener.
                mTelephonyManager.listen(
                        mPhoneStateListeners.get(subId), PhoneStateListener.LISTEN_NONE);
                /* @} */
                itr.remove();
            }
        }

        if (subscriptions == null) {
            subscriptions = Collections.emptyList();
        }

        // Register new phone listeners for active subscriptions.
        /* SPRD: modify for bug526598 @{ */
        for (int i = 0; i < subscriptions.size(); i++) {
            int subId = subscriptions.get(i).getSubscriptionId();
            if (!mPhoneStateListeners.containsKey(subId)) {
                log("register listener for sub[" + subId + "]");
                PhoneStateListener listener = getPhoneStateListener(subId);
                mTelephonyManager.listen(listener,
                        PhoneStateListener.LISTEN_SERVICE_STATE
                                | PhoneStateListener.LISTEN_CALL_FORWARDING_INDICATOR);
                mPhoneStateListeners.put(subId, listener);
            }
        }
        /* @} */
    }

    private PhoneStateListener getPhoneStateListener(int subId) {
        final int phoneId = SubscriptionManager.getPhoneId(subId);
        log("getPhoneStateListener for phone[" + phoneId + "]" + ", [subId: " + subId + "]");

        PhoneStateListener phoneStateListener = new PhoneStateListener(subId) {
            @Override
            public void onCallForwardingIndicatorChanged(boolean cfi) {
                log("onCallForwardingIndicatorChanged for phone:" + phoneId
                        + " cfi :" + cfi + " mFirstCallBack: " + mFirstCallBack);
                if (mServiceState[phoneId] == ServiceState.STATE_IN_SERVICE && (!mFirstCallBack || cfi)) {
                    log("onCallForwardingIndicatorChangedByServiceClass->(phoneId:" + phoneId
                            + ") mServiceState[phoneId]:" + mServiceState[phoneId]);
                    onCfiChanged(cfi, phoneId);
                }
                mFirstCallBack = false;
            }

            @Override
            public void onServiceStateChanged(ServiceState serviceState) {
                if (mTelephonyManager != null && !mTelephonyManager.hasIccCard(phoneId)) {
                    log("(phoneId" + phoneId + ") card doesn't exist");
                    return;
                }
                mServiceState[phoneId] = serviceState.getState();
                log("(phoneId" + phoneId + ") onServiceStateChanged(), state: "
                        + serviceState.getState());
                switch (serviceState.getState()) {
                    case ServiceState.STATE_OUT_OF_SERVICE:
                    case ServiceState.STATE_POWER_OFF:
                        updateCfiVisibility(false, phoneId);
                        break;
                    case ServiceState.STATE_IN_SERVICE:
                        // SPRD: modify for bug526598
                        if (SystemProperties.getInt("persist.sys.callforwarding", 0) == 1
                                || mFirstQuery) {
                            queryAllCfu(phoneId);
                        }
                        break;
                    default:
                        break;
                }
            }
        };
        return phoneStateListener;
    }

    private void onCfiChanged(boolean visible, int phoneId) {
        log("onCfiChanged(): " + visible + ", phoneId = " + phoneId);
        if (phoneId < 0 || phoneId > TelephonyManager.getDefault().getPhoneCount()) {
            return;
        }
        checkIccId(phoneId);
        saveCfiToPreference(visible, phoneId);
        updateCfiVisibility(visible, phoneId);
        /* SPRD: modify for bug526598 @{ */
        String savedIccId = mPreference.getString(KEY_ICC_ID + phoneId, null);
        String currentIccId = getCurrentIccId(phoneId);
        if (visible && savedIccId != null && savedIccId.equalsIgnoreCase(currentIccId)) {
            mFirstQuery = false;
        }
        /* @} */
    }

    private void saveCfiToPreference(boolean visible, int phoneId) {
        log("saveCfiToPreference->visible = " + visible + ",phoneId = " + phoneId);
        mEditor.putBoolean(KEY_CFU_VOICE_VALUE + phoneId, visible);
        mEditor.apply();
    }

    private void checkIccId(int phoneId) {
        String savedIccId = mPreference.getString(KEY_ICC_ID + phoneId, null);
        String currentIccId = getCurrentIccId(phoneId);
        if (currentIccId == null) {
            log("checkIccId->currentIccId is null!");
            return;
        }
        if (savedIccId == null || !savedIccId.equalsIgnoreCase(currentIccId)) {
            log("checkIccId->current id is not saved id->savedIccId:" + savedIccId + " currentIccId:"
                    + currentIccId);
            mEditor.putString(KEY_ICC_ID + phoneId, currentIccId);
            mEditor.putBoolean(KEY_CFU_VOICE_VALUE + phoneId, false);
            mEditor.apply();
            if (mServiceState[phoneId] == ServiceState.STATE_IN_SERVICE &&
                    (SystemProperties.getInt("persist.sys.callforwarding", 0) == 1
                    || SystemProperties.getInt("persist.sys.callforwarding", 0) == 0)) {
                PhoneFactory.getPhone(phoneId).getCallForwardingOption(
                        CommandsInterface.CF_REASON_UNCONDITIONAL, null);
            }
        }
    }

    private String getCurrentIccId(int phoneId) {
        String iccId = null;
        Phone phone = PhoneFactory.getPhone(phoneId);
        /* SPRD: add for bug 529444 to avoid NullPoiontException @{ */
        if (phone != null) {
            IccCard iccCard = phone.getIccCard();
            if (iccCard != null) {
                IccRecords iccRecords = iccCard.getIccRecords();
                if (iccRecords != null) {
                    iccId = iccRecords.getIccId();
                }
            }
        }
        /* @} */
        return iccId;
    }

    private void queryAllCfu(int phoneId) {
        log("queryAllCfu(),phoneId = " + phoneId);
        checkIccId(phoneId);
        boolean showVoiceCfi = mPreference.getBoolean(KEY_CFU_VOICE_VALUE + phoneId, false);
        updateCfiVisibility(showVoiceCfi, phoneId);
    }

    /**
     * Updates the message call forwarding indicator notification.
     *
     * @param visible true if there are messages waiting
     */
    /* package */ void updateCfi(int subId, boolean visible) {
        log("updateCfi(): " + visible);
        if (visible) {
            // If Unconditional Call Forwarding (forward all calls) for VOICE
            // is enabled, just show a notification.  We'll default to expanded
            // view for now, so the there is less confusion about the icon.  If
            // it is deemed too weird to have CF indications as expanded views,
            // then we'll flip the flag back.

            // TODO: We may want to take a look to see if the notification can
            // display the target to forward calls to.  This will require some
            // effort though, since there are multiple layers of messages that
            // will need to propagate that information.

            SubscriptionInfo subInfo = mSubscriptionManager.getActiveSubscriptionInfo(subId);
            if (subInfo == null) {
                log("Found null subscription info for: " + subId);
                return;
            }
            /* SPRD: add for bug510093 @{ */
            boolean isMultiSimEnabled = mTelephonyManager.isMultiSimEnabled();
            int iconId;
            if (isMultiSimEnabled) {
                int phoneId = SubscriptionManager.getPhoneId(subId);
                iconId = sCallfwdIcon[phoneId];
            } else {
                iconId = R.drawable.stat_sys_phone_call_forward;
            }
            /* @} */
            String notificationTitle;
            if (mTelephonyManager.getPhoneCount() > 1) {
                notificationTitle = subInfo.getDisplayName().toString();
            } else {
                notificationTitle = mApplication.getString(R.string.labelCF);
            }

            Notification.Builder builder = new Notification.Builder(mApplication)
                    .setSmallIcon(iconId)
                    .setColor(subInfo.getIconTint())
                    .setContentTitle(notificationTitle)
                    .setContentText(mApplication.getString(R.string.sum_cfu_enabled_indicator))
                    .setShowWhen(false)
                    .setOngoing(true);

            Intent intent = new Intent(Intent.ACTION_MAIN);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            intent.setClassName("com.android.phone", "com.android.phone.CallFeaturesSetting");
            addExtrasToIntent(
                    intent, mSubscriptionManager.getActiveSubscriptionInfo(subId));
            PendingIntent contentIntent =
                    PendingIntent.getActivity(mApplication, subId /* requestCode */, intent, 0);

            List<UserInfo> users = mUserManager.getUsers(true);
            for (int i = 0; i < users.size(); i++) {
                final UserInfo user = users.get(i);
                if (user.isManagedProfile()) {
                    continue;
                }
                UserHandle userHandle = user.getUserHandle();
                builder.setContentIntent(userHandle.isOwner() ? contentIntent : null);
                mNotificationManager.notifyAsUser(
                        Integer.toString(subId) /* tag */,
                        CALL_FORWARD_NOTIFICATION,
                        builder.build(),
                        userHandle);
            }
        } else {
            mNotificationManager.cancelAsUser(
                    Integer.toString(subId) /* tag */,
                    CALL_FORWARD_NOTIFICATION,
                    UserHandle.ALL);
        }
    }
    private void updateCfiVisibility(boolean visible, int phoneId) {
        log("updateCfiVisibility->visible = " + visible + ",phoneId = " + phoneId);
        /* SPRD: add for bug 529444 to avoid NullPoiontException @{ */
        Phone phone = PhoneFactory.getPhone(phoneId);
        if (phone != null) {
            int subId = phone.getSubId();
            updateCfi(subId, visible);
        }
        /* @} */
    }

    private void addExtrasToIntent(Intent intent, SubscriptionInfo subscription) {
        if (subscription == null) {
            return;
        }

        intent.putExtra(SUB_ID_EXTRA, subscription.getSubscriptionId());
        intent.putExtra(SUB_LABEL_EXTRA, subscription.getDisplayName().toString());
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }
}
