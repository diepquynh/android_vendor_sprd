
package com.android.internal.telephony.uicc;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TelephonyIntents;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.Message;
import android.telephony.Rlog;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;

public class ExtraIccRecordsController extends Handler {

    private static final String LOG_TAG = "ExtendedIccRecordsLoader";

    private static final int EVENT_SIM_READY = 0;
    private static final int EVENT_SIM_UNAVAILABLE = 1;

    /** The singleton instance. */
    private static ExtraIccRecordsController mInstance;
    private CommandsInterface[] mCi;
    private static int mPhoneCount = TelephonyManager.getDefault().getPhoneCount();
    private ExtraIccRecords[] mExIccRecords = new ExtraIccRecords[mPhoneCount];

    public static ExtraIccRecordsController init(Context context, CommandsInterface[] ci) {
        synchronized (ExtraIccRecordsController.class) {
            if (mInstance == null) {
                mInstance = new ExtraIccRecordsController(context, ci);
            } else {
                Log.wtf(LOG_TAG, "init() called multiple times!  mInstance = " + mInstance);
            }
            return mInstance;
        }
    }

    private ExtraIccRecordsController(Context context, CommandsInterface[] ci) {
        mCi = ci;

        for (int i = 0; i < mPhoneCount; i++) {
            mExIccRecords[i] = new ExtraIccRecords(context, i, mCi[i]);
        }

        IntentFilter filter = new IntentFilter(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        context.registerReceiver(mReceiver, filter);
    }

    public static ExtraIccRecordsController getInstance() {
        if (mInstance == null) {
            Log.wtf(LOG_TAG, "getInstance null");
        }
        return mInstance;
    }

    @Override
    public void handleMessage(Message msg) {

        switch (msg.what) {
            case EVENT_SIM_READY:
                mExIccRecords[msg.arg1].fetchExtendedIccRecords();
                break;

            case EVENT_SIM_UNAVAILABLE:
                mExIccRecords[msg.arg1].resetRecords();
                break;
            default:
                break;
        }

    }

    public ExtraIccRecords[] getExtraIccRecords() {
        return mExIccRecords;
    }

    public ExtraIccRecords getExtraIccRecords(int phoneId) {
        return mExIccRecords[phoneId];
    }

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(action)) {
                int phoneId = intent.getIntExtra(PhoneConstants.PHONE_KEY,
                        SubscriptionManager.DEFAULT_PHONE_INDEX);

                Rlog.d(LOG_TAG, "receive broadcast ACTION_SIM_STATE_CHANGED " + phoneId);

                if (!SubscriptionManager.isValidPhoneId(phoneId)) {
                    return;
                }

                String simState = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                if (IccCardConstants.INTENT_VALUE_ICC_READY.equals(simState)) {
                    sendMessage(obtainMessage(EVENT_SIM_READY, phoneId, -1));
                } else if (IccCardConstants.INTENT_VALUE_ICC_NOT_READY.equals(simState)
                        || IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(simState)
                        || IccCardConstants.INTENT_VALUE_ICC_CARD_IO_ERROR.equals(simState)
                        || IccCardConstants.INTENT_VALUE_ICC_UNKNOWN.equals(simState)) {
                    sendMessage(obtainMessage(EVENT_SIM_UNAVAILABLE, phoneId, -1));
                }
            }
        }
    };

}
