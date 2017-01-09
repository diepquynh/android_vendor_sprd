
package com.android.phone;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.CallManager;
import com.android.internal.telephony.MmiCode;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.TelephonyIntentsEx;
import com.android.internal.telephony.TelephonyIntents;
import com.android.phone.CallForwardHelper;
import com.android.phone.CarrierConfigLoaderEx;
import com.android.phone.SuppServiceConsumer;
import com.android.phone.TelephonyOrangeHelper;

import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.Message;
import android.os.SystemProperties;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import android.util.Log;
import com.sprd.phone.TeleServicePluginsHelper;

import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.telephony.RadioInteractorFactory;
import com.android.sprd.telephony.uicc.IccCardStatusEx;
import com.android.internal.telephony.PhoneConstants;

/**
 * It is a convenient class for other feature support. We had better put this class in the same
 * package with {@link PhoneGlobals}, so we can touch all the package permission variables and
 * methods in it.
 */
public class PhoneGlobalsEx extends ContextWrapper {
    private static final String TAG = "PhoneGlobalsEx";

    /* SPRD: add for bug490253 @{ */
    protected static final int EVENT_SEND_DISMISS_DIALOG_COMPLETE = 19;
    /* SPRD: add for bug620380 @{ */
    public static final int PHONE_STATE_CHANGED = 20;
    private CallManager mCM;
    /* @} */
    private static PhoneGlobalsEx mInstance;
    private Context mContext;
    private SimLockManager mSimLockManager;
    public PhoneInterfaceManagerEx phoneMgrEx;
    private AlertDialog mMmiPreviousAlertDialog;
    //SPRD:Add for Reliance simlock
    private static byte mSimAbsentFlag = 0;
    RadioInteractor mRadioInteractor;

    private final BroadcastReceiver mReceiver = new PhoneAppBroadcastReceiver();

    public static PhoneGlobalsEx getInstance() {
        return mInstance;
    }

    public PhoneGlobalsEx(Context context) {
        super(context);
        mInstance = this;
        mContext = context;
    }

    public void onCreate() {
        Log.d(TAG, "onCreate");
        RadioInteractorFactory.init(mContext);
        CallForwardHelper.getInstance();
        /* SPRD: add for bug620380 @{ */
        boolean isIncomingCallDialogHide = TelephonyOrangeHelper.getInstance(
                mContext).isIncomingCallDialogHide();
        if (isIncomingCallDialogHide && mCM == null) {
            mCM = CallManager.getInstance();
            mCM.registerForPreciseCallStateChanged(mHandler,
                    PHONE_STATE_CHANGED, null);
        }
        /* @} */
        TeleServicePluginsHelper.getInstanceToShowCallingNumber(mContext)
                .setCallingNumberShownEnabled();

        //SPRD:Add for Carrier Config Loader
        CarrierConfigLoaderEx.init(mInstance);
        phoneMgrEx = PhoneInterfaceManagerEx.init(this, PhoneFactory.getDefaultPhone());
        for (Phone phone : PhoneFactory.getPhones()) {
            // SPRD: Porting SS. Register Consumer Supplementary Service.
            SuppServiceConsumer.getInstance(mInstance, phone);
        }
        /* SPRD: Add for Shutdown bell feature @{ */
        IntentFilter intentFilter =
                new IntentFilter(Intent.ACTION_SHUTDOWN);
        registerReceiver(mReceiver, intentFilter);
        /* @} */
        /* SPRD: added for sim lock @{ */
        IntentFilter simlockIntentFilter = new IntentFilter();
        simlockIntentFilter.addAction(TelephonyIntentsEx.SHOW_SIMLOCK_UNLOCK_SCREEN_ACTION);
        simlockIntentFilter.addAction(TelephonyIntentsEx.SHOW_SIMLOCK_UNLOCK_SCREEN_BYNV_ACTION);
        //SPRD: Add for Reliance simlock
        simlockIntentFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        registerReceiver(mUnlockScreenReceiver, simlockIntentFilter);

        mSimLockManager = SimLockManager.getInstance(mContext, R.string.feature_support_simlock);
        mSimLockManager.registerForSimLocked(mContext);
        /* @} */
        // SPRD: Add for fast shutdown
        FastShutdownHelper.init(this);
    }

    /**
     * Receiver for misc intent broadcasts the Phone app cares about.
     */
    private class PhoneAppBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_SHUTDOWN)) {
                Log.d(TAG,"ACTION_SHUTDOWN phone state : "
                                + CallManager.getInstance().getState());
                PhoneUtils.hangup(CallManager.getInstance());
                return;
            }
        }
    }

    /** SPRD: added for sim lock @{ */
    private BroadcastReceiver mUnlockScreenReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(TelephonyIntentsEx.SHOW_SIMLOCK_UNLOCK_SCREEN_ACTION)) {
                int simlockSlotFlag = intent.getIntExtra(TelephonyIntentsEx.EXTRA_SIMLOCK_UNLOCK, 0);
                if (simlockSlotFlag == 0) return;
                TelephonyManager tm = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
                int phoneCount = tm.getPhoneCount();
                int allFlag = (1<<phoneCount)-1;
                for (int i=0; i<phoneCount; i++ ) {
                    if (((1 << i) & simlockSlotFlag) != 0) {
                        int simState = TelephonyManagerEx.getSimStateEx(i);
                        Log.d(TAG, "simState[" + i + "] = " + simState);
                        Message msg = mSimLockManager.decodeMessage(simState, i);
                        if (msg != null && msg.what != 0) {
                            mHandler.sendMessage(msg);
                        }
                    }
                }
            } else if (action.equals(TelephonyIntentsEx.SHOW_SIMLOCK_UNLOCK_SCREEN_BYNV_ACTION)) {
                mSimLockManager.showPanelForUnlockByNv(mContext);
            }
            /* SPRD: Add for Reliance simlock @{ */
            else if (action.equals(TelephonyIntents.ACTION_SIM_STATE_CHANGED)){
                if (TelephonyManagerEx.isRelianceSimlock()) {
                    String state = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                    int phoneId = intent.getIntExtra(PhoneConstants.PHONE_KEY, SubscriptionManager.INVALID_PHONE_INDEX);
                    Log.d(TAG, "action = " + action + ", state = " + state + ", phoneId = " + phoneId);

                    if(IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(state)){
                        int phoneCount = ((TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE)).getPhoneCount();
                        if (mRadioInteractor == null) {
                            mRadioInteractor = new RadioInteractor(context);
                        }
                        boolean isNetworkLock = mRadioInteractor.getSimLockStatus(IccCardStatusEx.UNLOCK_NETWORK, 0) == 1 ? true : false;
                        Log.d(TAG, "isNetworkLock = " + isNetworkLock);

                        mSimAbsentFlag |= (1 << phoneId);
                        if((mSimAbsentFlag == ((1<<phoneCount)-1)) && isNetworkLock){
                            Message message = mSimLockManager.decodeMessage(TelephonyManager.SIM_STATE_NETWORK_LOCKED, 0);
                            if (message != null && message.what != 0) {
                                mHandler.sendMessage(message);
                            }
                        }
                    }
                }
            }
            /* @} */
        }
    };

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case EVENT_SEND_DISMISS_DIALOG_COMPLETE:
                    if (mMmiPreviousAlertDialog != null) {
                        mMmiPreviousAlertDialog.dismiss();
                        mMmiPreviousAlertDialog = null;
                    }
                break;
                /* SPRD: add for bug620380 @{ */
                case PHONE_STATE_CHANGED:
                    onPhoneStateChanged();
                    break;
                /* @} */
                default:
                    mSimLockManager.showPanel(mContext, msg);
                    break;
            }
        }
    };
    /** @} */

    /**
     * SPRD:add for bug490253
     */
    public void handleMMIDialogDismiss(final Phone phone, Context context, final MmiCode mmiCode,
            Message dismissCallbackMessage,
            AlertDialog previousAlert) {
        mMmiPreviousAlertDialog = PhoneUtils.displayMMIComplete(mmiCode.getPhone(), context,
                mmiCode, null, null);
        mHandler.removeMessages(EVENT_SEND_DISMISS_DIALOG_COMPLETE);
        mHandler.sendMessageDelayed(mHandler.obtainMessage(EVENT_SEND_DISMISS_DIALOG_COMPLETE),
                180000);
    }

    /* SPRD: add for bug620380 @{ */
    private void onPhoneStateChanged() {
        PhoneConstants.State state = mCM.getState();
        if (state == PhoneConstants.State.RINGING) {
            hideUssdDialog();
        }
        if (state == PhoneConstants.State.IDLE) {
            showUssdDialog();
        }
    }

    public void hideUssdDialog(){
        if (mMmiPreviousAlertDialog != null && mMmiPreviousAlertDialog.isShowing()) {
            mMmiPreviousAlertDialog.hide();
        }
    }

    public void showUssdDialog(){
        if (mMmiPreviousAlertDialog != null && mMmiPreviousAlertDialog.isShowing()) {
            mMmiPreviousAlertDialog.show();
        }
    }
    /* @} */
}
