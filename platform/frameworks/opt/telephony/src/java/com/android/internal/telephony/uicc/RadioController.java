
package com.android.internal.telephony.uicc;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.RadioInteraction;

import android.content.Context;
import android.os.Message;
import android.provider.Settings;
import android.provider.SettingsEx;
import android.telephony.Rlog;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

public class RadioController {

    private static final String TAG = "RadioController";
    public static final int NONE = 0;
    public static final int POWER_ON = 1;
    public static final int POWER_OFF = 2;

    private final int mPhoneCount = TelephonyManager.getDefault().getPhoneCount();
    private static RadioController sInstance = new RadioController();

    private RadioController() {
    }

    public static RadioController getInstance() {
        return sInstance;
    }

    private static List<RadioTask> mRadioTaskList = new ArrayList<RadioTask>();

    public void setRadioPower(int phoneId, boolean onOff, Message msg) {
        int[] ops = new int[mPhoneCount];
        if (SubscriptionManager.isValidPhoneId(phoneId)) {
            Arrays.fill(ops, NONE);
            ops[phoneId] = onOff ? POWER_ON : POWER_OFF;
        } else if (phoneId == mPhoneCount) {
            Arrays.fill(ops, onOff ? POWER_ON : POWER_OFF);
        }

        setRadioPower(ops, msg);
    }

    public void setRadioPower(int[] ops, Message response) {
        RadioTask newTask = new RadioTask(ops, response);
        boolean handleNow = true;
        synchronized (mRadioTaskList) {
            if (!mRadioTaskList.isEmpty()) {
                handleNow = false;
            }
            mRadioTaskList.add(newTask);
        }

        if (handleNow) {
            handleRadioTask(newTask);
        }
    }

    private int mRadioOperationCount = 0;

    private void handleRadioTask(RadioTask task) {
        Rlog.d(TAG, "handleRadioTask task = " + task.toString());
        if (task.mOps != null) {
            for (int phoneId = 0; phoneId < task.mOps.length; phoneId++) {
                if (task.mOps[phoneId] != NONE) {
                    boolean onOff = task.mOps[phoneId] == POWER_ON;
                    Phone phone = PhoneFactory.getPhone(phoneId);
                    //Modify for bug 625595
                    if (phone != null) {
                        if ((phone.getServiceState()
                                .getState() != ServiceState.STATE_POWER_OFF) != onOff) {
                            synchronized (RadioController.class) {
                                mRadioOperationCount++;
                            }

                            final RadioInteraction ri = new RadioInteraction(phone);
                            ri.setCallBack(new Runnable() {
                                @Override
                                public void run() {
                                    ri.destoroy();

                                    synchronized (RadioController.class) {
                                        if (--mRadioOperationCount == 0) {
                                            task.finish();
                                        }
                                    }
                                }
                            });

                            if (onOff) {
                                ri.powerOnRadio(RadioInteraction.RADIO_POWER_ON_TIMEOUT);
                            } else {
                                ri.powerOffRadio(RadioInteraction.RADIO_POWER_OFF_TIMEOUT);
                            }
                        }
                    }
                }

            }
        }

        synchronized (RadioController.class) {
            if (mRadioOperationCount == 0) {
                task.finish();
            }
        }
    }

    private class RadioTask {
        private List<Message> mResponseMsgList = new ArrayList<Message>();
        private int[] mOps = null;

        RadioTask(int[] ops, Message response) {
            mOps = ops;
            mResponseMsgList.add(response);
        }

        void finish() {
            if (mResponseMsgList != null) {
                for (Message msg : mResponseMsgList) {
                    msg.sendToTarget();
                }
                mResponseMsgList = null;
            }

            RadioTask task = null;
            synchronized (mRadioTaskList) {
                mRadioTaskList.remove(this);

                if (!mRadioTaskList.isEmpty()) {
                    task = mRadioTaskList.get(0);
                }
            }

            if (task != null) {
                handleRadioTask(task);
            }
        }

        @Override
        public String toString() {
            String info = mResponseMsgList.toString() + " : ";
            for (int i = 0; i < mOps.length; i++) {
                info += mOps[i];
            }
            return info;
        }
    }


    public void setRadioBusy(Context context, boolean busy) {
        Settings.Global.putInt(context.getContentResolver(), SettingsEx.GlobalEx.RADIO_BUSY,
                busy ? 1 : 0);
    }

    public boolean isRadioBusy(Context context) {
        return Settings.Global.getInt(context.getContentResolver(),SettingsEx.GlobalEx.RADIO_BUSY,
                0) == 1;
    }
}
