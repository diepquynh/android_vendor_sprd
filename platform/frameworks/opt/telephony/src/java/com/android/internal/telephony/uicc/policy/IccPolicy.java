
package com.android.internal.telephony.uicc.policy;

import android.telephony.Rlog;
import android.telephony.TelephonyManager;

public abstract class IccPolicy {

    public enum SimPriority {
        NONE,
        LOWEST,
        LOW_SIM,
        LOW_USIM,
        HIGH_SIM,
        LOCKED,
        HIGH_USIM;
    }

    private static final String TAG = "IccPolicy";

    protected static final int PHONE_COUNT = TelephonyManager.getDefault().getPhoneCount();
    protected int mMaxPriorityCount;
    protected int mMaxPriorityPhoneId;
    protected SimPriority[] mSimPriorities = new SimPriority[PHONE_COUNT];

    public void updateSimPriorities() {
        initMaxPriorityValues();

        for (int i = 0; i < PHONE_COUNT; i++) {
            mSimPriorities[i] = getSimPriority(i);
            Rlog.d(TAG, "[updateSimPriorities] mSimPriorities[" + i + "]= " + mSimPriorities[i]);
        }

        updateMaxPriorityValues();
    }

    public abstract SimPriority getSimPriority(int phoneId);

    public abstract boolean isNeedPromptUserSetPrimaryCard();

    private void initMaxPriorityValues() {
        mMaxPriorityPhoneId = 0;
        mMaxPriorityCount = 1;
    }

    private void updateMaxPriorityValues() {
        for (int i = 1; i < PHONE_COUNT; i++) {
            if (mSimPriorities[mMaxPriorityPhoneId].compareTo(mSimPriorities[i]) < 0) {
                mMaxPriorityPhoneId = i;
                mMaxPriorityCount = 1;
            } else if (mSimPriorities[mMaxPriorityPhoneId] == mSimPriorities[i]) {
                mMaxPriorityCount++;
            }
        }

        Rlog.d(TAG, "mMaxPriorityPhoneId = " + mMaxPriorityPhoneId + " mMaxPriorityCount = "
                + mMaxPriorityCount);
    }

    protected int getPrimaryCardAccordingToPolicy() {
        return mMaxPriorityPhoneId;
    }
}
