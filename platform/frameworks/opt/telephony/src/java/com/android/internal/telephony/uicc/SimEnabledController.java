
package com.android.internal.telephony.uicc;

import java.util.HashMap;
import java.util.Map;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.ProxyController;
import com.android.internal.telephony.RadioInteraction;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.telephony.Rlog;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;

public class SimEnabledController {

    private static String TAG = "SimEnabledController";

    private static final int EVENT_SET_SIM_ENABLED = 0;

    private static final int EVENT_POWER_RADIO_OFF_DONE = 1;
    private static final int EVENT_POWER_RADIO_ON_DONE = 2;
    private static final int EVENT_POWER_ALL_RADIOS_OFF_DONE = 3;

    private static SimEnabledController sInstance;

    private static Map<Integer, SetSimEnabled> mSetSimEnabledRequestList = new HashMap<Integer, SetSimEnabled>();

    private int mPhoneCount = TelephonyManager.getDefault().getPhoneCount();
    private Context mContext;
    private RadioController mRC;

    private SimEnabledController(Context context) {
        mContext = context;
        mRC = RadioController.getInstance();
    }

    public static SimEnabledController getInstance() {
        return sInstance;
    }

    public static void init(Context context) {
        if (sInstance == null) {
            sInstance = new SimEnabledController(context);
        }
    }

    public void setSimEnabled(int phoneId, final boolean turnOn) {
        Rlog.d(TAG, "set sim enabled[" + phoneId + "] " + turnOn);
        mHandler.obtainMessage(EVENT_SET_SIM_ENABLED, phoneId, turnOn ? 1 : 0).sendToTarget();
    }

    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (msg.what == EVENT_SET_SIM_ENABLED) {

                mRC.setRadioBusy(mContext, true);

                Phone phone = PhoneFactory.getPhone(msg.arg1);
                if (phone != null) {
                    new SetSimEnabled(phone, msg.arg2 == 1).setSimEnabled();
                }
            }
        };
    };

    private class SetSimEnabled extends Handler {
        private final Phone mPhone;
        private final boolean mEnabled;
        private RadioInteraction mRI;

        public SetSimEnabled(Phone phone, boolean turnOn) {
            mPhone = phone;
            mEnabled = turnOn;
        }

        private void setSimEnabled() {
            synchronized (SimEnabledController.this) {
                if (mSetSimEnabledRequestList.get(mPhone.getPhoneId()) == null) {
                    mSetSimEnabledRequestList.put(mPhone.getPhoneId(), this);
                } else {
                    mSetSimEnabledRequestList.put(mPhone.getPhoneId(), this);
                    return;
                }
            }

            if (mEnabled) {
                mRI = new RadioInteraction(mPhone);
                mRI.setCallBack(new Runnable() {
                    @Override
                    public void run() {
                        // Power off all radios first before power on the only exist SIM which
                        // is not the primary card.
                        if (isNeedPowerOffAllRadios()) {
                            Rlog.d(TAG, "Power off all radios first.");
                            mRC.setRadioPower(mPhoneCount, false,
                                    obtainMessage(EVENT_POWER_ALL_RADIOS_OFF_DONE));
                        } else {
                            mRC.setRadioPower(mPhone.getPhoneId(), true,
                                    obtainMessage(EVENT_POWER_RADIO_ON_DONE));
                        }
                    }
                });
                mRI.powerOnIccCard(RadioInteraction.ICC_POWER_TIMEOUT);

            } else {
                mRC.setRadioPower(mPhone.getPhoneId(), false,
                        obtainMessage(EVENT_POWER_RADIO_OFF_DONE));
            }
        }

        public void handleMessage(Message msg) {
            switch (msg.what) {
                case EVENT_POWER_RADIO_OFF_DONE:
                    mRI = new RadioInteraction(mPhone);
                    mRI.setCallBack(new Runnable() {
                        @Override
                        public void run() {
                            if (getActiveSimCount() == 0) {
                                mRC.setRadioPower(mPhone.getPhoneId(), true,
                                        obtainMessage(EVENT_POWER_RADIO_ON_DONE));
                            } else {
                                finish();
                            }
                        }
                    });
                    mRI.powerOffIccCard(RadioInteraction.ICC_POWER_TIMEOUT);
                    break;

                case EVENT_POWER_RADIO_ON_DONE:
                    finish();
                    break;
                case EVENT_POWER_ALL_RADIOS_OFF_DONE:
                    mRC.setRadioPower(mPhone.getPhoneId(), true,
                            obtainMessage(EVENT_POWER_RADIO_ON_DONE));
                    break;
                default:
                    break;
            }
        }

        private void finish() {
            if (mRI != null) {
                mRI.destoroy();
                mRI = null;
            }

            SetSimEnabled sse = null;
            synchronized (SimEnabledController.this) {
                sse = mSetSimEnabledRequestList.remove(mPhone.getPhoneId());
            }

            if (sse != null && sse != this && sse.mEnabled != mEnabled) {
                sse.setSimEnabled();
            } else {
                mRC.setRadioBusy(mContext, false);
            }
        }

        private boolean isNeedPowerOffAllRadios() {
            if (getActiveSimCount() == 1 && mPhone.getIccCard().hasIccCard()) {
                if (mPhone.getRadioAccessFamily() != ProxyController.getInstance()
                        .getMaxRafSupported()) {
                    return true;
                } else {
                    for (int i = 0; i < mPhoneCount; i++) {
                        Phone phone = PhoneFactory.getPhone(i);
                        if (phone != null && i != mPhone.getPhoneId() && phone.getServiceState()
                                .getState() != ServiceState.STATE_POWER_OFF) {
                            return true;
                        }
                    }
                }
            }
            return false;
        }
    }


    private int getActiveSimCount() {
        int activeSimCount = 0;
        for (int i = 0; i < mPhoneCount; i++) {
            Phone phone = PhoneFactory.getPhone(i);
            /* SPRD Modified for Bug 627613 @{ */
            if(phone != null) {
                if (phone.getIccCard().hasIccCard()) {
                    Rlog.d(TAG, "hasIccCard[" + i + "]= " + phone.getIccCard().hasIccCard());
                    activeSimCount++;
                }
            }
            /* @} */
        }
        return activeSimCount;
    }
}
