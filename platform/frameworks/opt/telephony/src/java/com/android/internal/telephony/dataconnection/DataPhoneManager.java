/* Created by Spreadst */

package com.android.internal.telephony.dataconnection;

import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants.State;

import java.util.ArrayList;
import java.util.HashMap;

class DataPhoneManager extends Handler {
    private static final String TAG = "DataPhoneManager";

    private static final int MAX_ACTIVE_PHONES = 1;

    private static final int EVENT_PRECISE_CALL_STATE_CHANGED = 0;

    private static DataPhoneManager sInstance;

    private int mMaxActivePhones = 1; // Default: DSDS phone has only one active phone
    private ArrayList<Phone>[] mPhones;
    private HashMap<Phone, State> mPhoneStates = new HashMap<Phone, State>();

    public static DataPhoneManager getInstance() {
        if (sInstance == null) {
            sInstance = new DataPhoneManager(MAX_ACTIVE_PHONES);
        }

        return sInstance;
    }

    private DataPhoneManager(int maxActivePhones) {
        mMaxActivePhones = maxActivePhones;
        mPhones = new ArrayList[maxActivePhones];
        for (int i = 0; i < maxActivePhones; i++) {
            mPhones[i] = new ArrayList<Phone>();
        }
    }

    public void registerPhone(Phone p) {
        int pool = getPoolId(p);
        if (pool >= 0 && pool < mMaxActivePhones) {
            ArrayList<Phone> phones = mPhones[pool];
            if (!phones.contains(p)) {
                phones.add(p);
                mPhoneStates.put(p, p.getState());
                if (p.getState() != State.IDLE) {
                    onVoiceCallStarted(p);
                }
                p.registerForPreciseCallStateChanged(this, EVENT_PRECISE_CALL_STATE_CHANGED, null);
            }
        }
    }

    public void unregisterPhone(Phone p) {
        for (int i = 0; i < mMaxActivePhones; i++) {
            ArrayList<Phone> phones = mPhones[i];
            if (phones.contains(p)) {
                phones.remove(p);
                p.unregisterForPreciseCallStateChanged(this);
            }
        }
    }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
            case EVENT_PRECISE_CALL_STATE_CHANGED:
                AsyncResult ar = (AsyncResult) msg.obj;
                Phone p = (Phone) ar.result;
                State state = p.getState();
                State oldState = mPhoneStates.get(p);

                if (oldState != state) {
                    mPhoneStates.put(p, state);
                    if (oldState == State.IDLE) {
                        onVoiceCallStarted(p);
                    } else if (state == State.IDLE) {
                        onVoiceCallEnded(p);
                    }
                }
                break;

            default:
                break;
        }
    }

    private void onVoiceCallStarted(Phone phone) {
        for (int i = 0; i < mMaxActivePhones; i++) {
            ArrayList<Phone> phones = mPhones[i];
            if (phones.contains(phone)) {
                for (Phone p : phones) {
                    if (p != phone) {
                        DcTrackerEx dct = (DcTrackerEx) p.mDcTracker;
                        dct.suspend(Phone.REASON_VOICE_CALL_STARTED);
                    }
                }
                break;
            }
        }
    }

    private void onVoiceCallEnded(Phone phone) {
        for (int i = 0; i < mMaxActivePhones; i++) {
            ArrayList<Phone> phones = mPhones[i];
            if (phones.contains(phone)) {
                for (Phone p : phones) {
                    if (p != phone) {
                        DcTrackerEx dct = (DcTrackerEx) p.mDcTracker;
                        dct.resume(Phone.REASON_VOICE_CALL_ENDED);
                    }
                }
                break;
            }
        }
    }

    private int getPoolId(Phone p) {
        TelephonyManager.MultiSimVariants msimVariant = TelephonyManager.getDefault()
                .getMultiSimConfiguration();
        switch (msimVariant) {
            case DSDS:
                return 0;
            case TSTS:
                return 0;
            case DSDA:
                return p.getPhoneId();
        }
        return 0;
    }
}
