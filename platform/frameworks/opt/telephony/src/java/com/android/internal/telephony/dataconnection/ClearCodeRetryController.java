package com.android.internal.telephony.dataconnection;

import android.content.Intent;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.telephony.PhoneStateListener;
import android.telephony.Rlog;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.internal.telephony.DctConstants;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.dataconnection.DataConnection;
import com.android.internal.telephony.plugin.DataConnectionUtils;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.telephony.RadioInteractorCallbackListener;

public class ClearCodeRetryController extends Handler
        implements DataConnectionUtils.RadioInteractorCallback {
    private static final String TAG = "ClearCode";

    private static final int EVENT_RADIO_ON = 0;
    private static final int EVENT_RAU_SUCCESS = 1;
    private static final int EVENT_CLEAR_CODE_FALLBACK = 2;
    private static final int EVENT_TRY_SETUP_DATA = 3;
    private static final int EVENT_RADIO_OFF_OR_NOT_AVAILABLE = 4;

    // when the data can't register to LTE in 40s, we will retry again.
    private static final int SWITCH_TIMEOUT = 40 * 1000;

    private static final int IDLE = 0;
    private static final int SWITCHING_TO_3G = 1;
    private static final int SWITCHING_TO_4G = 2;
    private static final int MAX_PDN_REJ_TIMES = 3;

    private Phone mPhone;
    private DcTrackerEx mDct;
    private Handler mH;
    private HandlerThread mHt;
    private boolean mLteDisabled = false;
    // PDN reject count reported by Modem
    private int mPDNRejTimes = 0;
    private int mState = IDLE;
    private MobilePhoneStateListener mPhoneStateListener;
    private RadioInteractorListener mRadioInteractorListener;
    private RadioInteractor mRi;

    public ClearCodeRetryController(Phone phone, DcTrackerEx dct) {
        mPhone = phone;
        mDct =  dct;
        DataConnectionUtils.getInstance(phone.getContext()).addRadioInteractorCallback(this);
        mPhone.mCi.registerForOn(this, EVENT_RADIO_ON, null);
        mHt = new HandlerThread(TAG);
        mHt.start();
        mH = new OemRequestHandler(mHt.getLooper(), phone);
    }

    public void dispose() {
        log("ClearCodeRetryController.dispose()");
        mPhone.mCi.unregisterForOn(this);
        if (mRi != null && mRadioInteractorListener != null) {
            mRi.listen(mRadioInteractorListener, RadioInteractorCallbackListener.LISTEN_NONE);
            mRadioInteractorListener = null;
        }
        DataConnectionUtils.getInstance(mPhone.getContext()).removeRadioInteractorCallback(this);
        mHt.quit();
        mH = null;
    }

    @Override
    public void onRiConnected(RadioInteractor ri) {
        mRi = ri;
        mRadioInteractorListener = new RadioInteractorListener(mPhone.getPhoneId());
        ri.listen(mRadioInteractorListener,
                RadioInteractorCallbackListener.LISTEN_RAU_SUCCESS_EVENT |
                RadioInteractorCallbackListener.LISTEN_CLEAR_CODE_FALLBACK_EVENT, true);
    }

    private class MobilePhoneStateListener extends PhoneStateListener {
        public MobilePhoneStateListener(int subId) {
            super(subId);
        }

        @Override
        public void onServiceStateChanged(ServiceState state) {
            int voiceNetType = state.getVoiceNetworkType();
            int voiceRegState = state.getVoiceRegState();
            log("onServiceStateChanged voiceNetType=" + voiceNetType
                    + " voiceRegState=" + voiceRegState);
            handleVoiceServiceChange(voiceNetType, voiceRegState);
        }
    }

    private class RadioInteractorListener extends RadioInteractorCallbackListener {
        public RadioInteractorListener(int slotId) {
            super(slotId);
        }

        @Override
        public void onRauSuccessEvent() {
            ClearCodeRetryController.this.sendEmptyMessage(EVENT_RAU_SUCCESS);
        }

        @Override
        public void onClearCodeFallbackEvent() {
            ClearCodeRetryController.this.sendEmptyMessage(EVENT_CLEAR_CODE_FALLBACK);
        }
    }

    public void registerVoiceStateListener() {
        if (mPhoneStateListener == null) {
            mPhoneStateListener = new MobilePhoneStateListener(mPhone.getSubId());
        }
        log("registerVoiceStateListener subId = " + mPhone.getSubId());
        TelephonyManager.from(mPhone.getContext()).listen(mPhoneStateListener,
                PhoneStateListener.LISTEN_SERVICE_STATE);
    }

    public void unregisterVoiceStateListener() {
        log("unregisterVocieStateListener");
        TelephonyManager.from(mPhone.getContext()).listen(mPhoneStateListener,
                PhoneStateListener.LISTEN_NONE);
        mPhoneStateListener = null;
    }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
            case EVENT_TRY_SETUP_DATA:
                log("EVENT_TRY_SETUP_DATA");
                mDct.clearPreFailCause();
                mState = IDLE;
                mDct.sendMessage(mDct.obtainMessage(DctConstants.EVENT_TRY_SETUP_DATA, msg.obj));
                break;

            case EVENT_RAU_SUCCESS:
                log("EVENT_RAU_SUCCESS");
                onDataConnectionRau();
                break;

            case EVENT_RADIO_ON:
                log("EVENT_RADIO_ON");
                mDct.clearPreFailCause();
                mState = IDLE;
                // in case the lte is disabled before, enable it now
                if (mLteDisabled || mPDNRejTimes >= MAX_PDN_REJ_TIMES) {
                    enableLte(true);
                    mLteDisabled = false;
                }
                mPDNRejTimes = 0;
                break;

            case EVENT_CLEAR_CODE_FALLBACK:
                log("EVENT_CLEAR_CODE_FALLBACK");
                mPDNRejTimes++;
                break;

            case EVENT_RADIO_OFF_OR_NOT_AVAILABLE:
                log("EVENT_RADIO_OFF_OR_NOT_AVAILABLE");
                // clear flags when radio is off
                mDct.clearPreFailCause();
                mState = IDLE;
                break;
        }
    }

    public void notifyRadioOffOrNotAvailable() {
        sendMessage(obtainMessage(EVENT_RADIO_OFF_OR_NOT_AVAILABLE));
    }

    public void switchTo3G() {
        log("Start switching to 3G...");
        dataConnectionAttach(false);
        enableLte(false);
        mLteDisabled = true;
        mState = SWITCHING_TO_3G;
        registerVoiceStateListener();
    }

    // Wait for the RAT we want to register to
    public void handleDataServiceChange(int rat, int regState) {
        log("Data RAT=" + rat + ", regState=" + regState + ", mState = " + mState);
        if (mState == SWITCHING_TO_3G) {
            if (rat != ServiceState.RIL_RADIO_TECHNOLOGY_LTE &&
                    rat != ServiceState.RIL_RADIO_TECHNOLOGY_LTE_CA &&
                    rat != ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN) {
                // retry in 3G, so clear mPreFailcause
                mDct.clearPreFailCause();
                mState = IDLE;
            }
        } else if (mState == SWITCHING_TO_4G) {
            if (rat == ServiceState.RIL_RADIO_TECHNOLOGY_LTE ||
                    rat == ServiceState.RIL_RADIO_TECHNOLOGY_LTE_CA) {
                // restart from beginning, so clear mPreFailcause
                mDct.clearPreFailCause();
                mState = IDLE;
                this.removeMessages(EVENT_TRY_SETUP_DATA);
            }
        }
    }

    // Finish the "detach -> disable lte -> attach" sequence
    public void handleVoiceServiceChange(int rat, int regState) {
        if (rat != TelephonyManager.NETWORK_TYPE_LTE
                && rat != TelephonyManager.NETWORK_TYPE_LTE_CA
                && regState == ServiceState.STATE_IN_SERVICE) {
            log("Voice registerd on " + rat + ", switching to 3G completed");
            dataConnectionAttach(true);
            unregisterVoiceStateListener();
        }
    }

    private void onDataConnectionRau() {
        restartCycle("RoutingAreaUpdate");
        // We will restart from begin, so stop all retry alarms
        mDct.stopFailRetryAlarm();
        mDct.cancelAllReconnectAlarms();
    }

    // After 2 hours or RAU, restart from beginning
    public void restartCycle(Object obj) {
        log("restartCycle: " + obj + ", mLteDisabled = " + mLteDisabled
                + ", mPDNRejTimes = " + mPDNRejTimes);
        mState = IDLE;
        if (mLteDisabled || mPDNRejTimes >= MAX_PDN_REJ_TIMES) {
            enableLte(true);
            mState = SWITCHING_TO_4G;
            mLteDisabled = false;
            // wait for RAT change to 4G or timeout
            sendMessageDelayed(obtainMessage(EVENT_TRY_SETUP_DATA, obj),
                    SWITCH_TIMEOUT);
        } else {
            sendMessage(obtainMessage(EVENT_TRY_SETUP_DATA, obj));
        }
        mPDNRejTimes = 0;
    }

    private void enableLte(boolean enable) {
        if (mH != null) {
            mH.sendMessage(mH.obtainMessage(OemRequestHandler.MSG_ENABLE_LTE, enable ? 1 : 0, 0));
        }
    }

    private void dataConnectionAttach(boolean attach) {
        if (mH != null) {
            mH.sendMessage(mH.obtainMessage(OemRequestHandler.MSG_ATTACH_DATA, attach ? 1 : 0, 0));
        }
    }

    private void log(String s) {
        Rlog.d(TAG, "[" + mPhone.getPhoneId() + "]" + s);
    }

    /**
     * Helper class to send AT commands in dedicate thread
     */
    class OemRequestHandler extends Handler {
        public static final int MSG_ATTACH_DATA = 0;
        public static final int MSG_ENABLE_LTE = 1;

        private Phone mPhone;
        public OemRequestHandler(Looper looper, Phone phone) {
            super(looper);
            mPhone = phone;
        }

        public void handleMessage(Message msg) {
            String[] atCommand = null;
            switch (msg.what) {
                case MSG_ATTACH_DATA:
                    boolean attach = msg.arg1 == 1;
                    if (mRi != null) {
                        mRi.attachDataConn(attach, mPhone.getPhoneId());
                    }
                    break;
                case MSG_ENABLE_LTE:
                    boolean enable = msg.arg1 == 1;
                    if (mRi != null) {
                        mRi.setLteEnabled(enable, mPhone.getPhoneId());
                    }
                    break;
            }
        }
    }
}
