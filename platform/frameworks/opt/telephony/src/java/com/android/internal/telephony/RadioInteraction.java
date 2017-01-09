package com.android.internal.telephony;

import android.provider.Settings;
import com.android.sprd.telephony.RadioInteractor;

import android.content.Context;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.telephony.Rlog;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.util.Log;

public class RadioInteraction {
    private final static String TAG = "RadioInteraction";
    private static final int MSG_POWER_OFF_RADIO = 1;
    private static final int MSG_POWER_ON_RADIO = 2;
    private static final int MSG_POWER_OFF_ICC = 3;
    private static final int MSG_POWER_ON_ICC = 4;

    public static final int RADIO_POWER_ON_TIMEOUT = 65000;
    public static final int RADIO_POWER_OFF_TIMEOUT = 95000;
    public static final int ICC_POWER_TIMEOUT = 5000;

    private TelephonyManager mTelephonyManager;
    private int mPhoneId;

    private volatile Looper mMsgLooper;
    private volatile MessageHandler mMsgHandler;
    private Context mContext;
    private Phone mPhone;

    private Runnable mRunnable;
    private RadioInteractor mRi;

    public RadioInteraction(Phone phone) {
        mPhone = phone;
        mPhoneId = phone.getPhoneId();
        mContext = phone.getContext();
        mTelephonyManager = TelephonyManager.getDefault();
        mRi = new RadioInteractor(mContext);

        HandlerThread thread = new HandlerThread("RadioInteraction[" + mPhoneId + "]");
        thread.start();

        mMsgLooper = thread.getLooper();
        mMsgHandler = new MessageHandler(mMsgLooper);
    }

    public void setCallBack(Runnable callback) {
        mRunnable = callback;
    }

    private final class MessageHandler extends Handler {
        public MessageHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            Log.i(TAG, "MessageHandler handleMessage " + msg);
            int timeout = Integer.parseInt(String.valueOf(msg.obj));
            switch (msg.what) {
                case MSG_POWER_OFF_RADIO:
                    powerOffRadioInner(timeout);
                    break;
                case MSG_POWER_ON_RADIO:
                    powerOnRadioInner(timeout);
                    break;
                case MSG_POWER_OFF_ICC:
                    powerOffIccCardInner(timeout);
                    break;
                case MSG_POWER_ON_ICC:
                    powerOnIccCardInner(timeout);
                    break;
                default:
                    break;
            }

        }
    }

    public void destoroy() {
        mMsgLooper.quit();
    }

    /*
     * The interface of ITelephony.setRadioPower is a-synchronized handler. But some case should
     * be synchronized handler. A method to power off the radio.
     */
    public void powerOffRadio(int timeout) {
        Rlog.i(TAG, "powerOffRadio for Phone" + mPhoneId);
        mMsgHandler.sendMessage(mMsgHandler.obtainMessage(MSG_POWER_OFF_RADIO, timeout));
    }

    private void powerOffRadioInner(int timeout) {
        Rlog.i(TAG, "powerOffRadioInner for Phone" + mPhoneId);
        final long endTime = SystemClock.elapsedRealtime() + timeout;
        boolean radioOff = false;

        radioOff = mPhone.getServiceState().getState() == ServiceState.STATE_POWER_OFF;
        if (!radioOff) {
            mPhone.setRadioPower(false);
            Rlog.i(TAG, "Powering off radio...");
        }

        Rlog.i(TAG, "Waiting for radio poweroff...");

        while (SystemClock.elapsedRealtime() < endTime) {
            // To give a chance for CPU scheduler
            try {
                Thread.sleep(200);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            if (radioOff) {
                Rlog.i(TAG, "Radio turned off.");
                break;
            } else {
                radioOff = mPhone.getServiceState().getState() == ServiceState.STATE_POWER_OFF;
            }
        }

        if (mRunnable != null) {
            Rlog.i(TAG, "Run the callback.");
            mRunnable.run();
        }
    }

    public void powerOnRadio(int timeout) {
        Rlog.i(TAG, "powerOnIRadio for Phone" + mPhoneId);
        Message msg = mMsgHandler.obtainMessage(MSG_POWER_ON_RADIO, timeout);
        mMsgHandler.sendMessage(msg);
    }

    public void powerOnRadioInner(int timeout) {
        Rlog.i(TAG, "powerOnRadioInner for Phone" + mPhoneId);
        final long endTime = SystemClock.elapsedRealtime() + timeout;

        boolean radioOn = false;
        boolean isAirplaneModeOn = Settings.Global.getInt(mPhone.getContext().getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
        radioOn = mPhone.getServiceState().getState() != ServiceState.STATE_POWER_OFF;
        if (!isAirplaneModeOn && !radioOn) {
            mPhone.setRadioPower(true);
            Rlog.i(TAG, "Powering on radio...");
        }

        Rlog.i(TAG, "Waiting for radio power on...");

        while (SystemClock.elapsedRealtime() < endTime) {
            // To give a chance for CPU scheduler
            try {
                Thread.sleep(200);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            if (isAirplaneModeOn || radioOn) {
                Log.i(TAG, "Radio turned on.");
                break;
            } else {
                radioOn = mPhone.getServiceState().getState() != ServiceState.STATE_POWER_OFF;
            }
        }

        if (mRunnable != null) {
            Rlog.i(TAG, "Run the callback.");
            mRunnable.run();
        }
    }

    public void powerOffIccCard(int timeout) {
        Rlog.i(TAG, "powerOffIccCard for Phone" + mPhoneId);
        mMsgHandler.sendMessage(mMsgHandler.obtainMessage(MSG_POWER_OFF_ICC, timeout));
    }

    private void powerOffIccCardInner(int timeout) {
        Rlog.i(TAG, "powerOffIccCardInner for Phone" + mPhoneId);
        final long endTime = SystemClock.elapsedRealtime() + timeout;
        boolean iccOff = false;

        iccOff = !mPhone.getIccCard().hasIccCard();
        if (!iccOff) {
            mRi.setSimPower(mPhoneId, false);
            Rlog.i(TAG, "Powering off IccCard...");
        }

        Rlog.i(TAG, "Waiting for icc off...");

        while (SystemClock.elapsedRealtime() < endTime) {
            // To give a chance for CPU scheduler
            try {
                Thread.sleep(200);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            if (iccOff) {
                Rlog.i(TAG, "IccCard turned off.");
                break;
            } else {
                iccOff = !mPhone.getIccCard().hasIccCard();
            }
        }

        if (mRunnable != null) {
            Rlog.i(TAG, "Run the callback.");
            mRunnable.run();
        }
    }

    public void powerOnIccCard(int timeout) {
        Rlog.i(TAG, "powerOnIccCard for Phone" + mPhoneId);
        mMsgHandler.sendMessage(mMsgHandler.obtainMessage(MSG_POWER_ON_ICC, timeout));
    }

    private void powerOnIccCardInner(int timeout) {
        Log.i(TAG, "powerOnIccCardInner for Phone" + mPhoneId);
        final long endTime = SystemClock.elapsedRealtime() + timeout;
        boolean iccOn = false;

        iccOn = mPhone.getIccCard().hasIccCard();
        if (!iccOn) {
            mRi.setSimPower(mPhoneId, true);
            Rlog.i(TAG, "Powering on IccCard...");
        }

        Log.i(TAG, "Waiting for icc on...");

        while (SystemClock.elapsedRealtime() < endTime) {
            // To give a chance for CPU scheduler
            try {
                Thread.sleep(200);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            if (iccOn) {
                Rlog.i(TAG, "IccCard turned on.");
                break;
            } else {
                iccOn = mPhone.getIccCard().hasIccCard();
            }
        }

        if (mRunnable != null) {
            Rlog.i(TAG, "Run the callback.");
            mRunnable.run();
        }
    }
}
