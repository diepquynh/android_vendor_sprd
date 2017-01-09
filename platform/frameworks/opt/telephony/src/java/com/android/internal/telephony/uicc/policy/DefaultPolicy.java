
package com.android.internal.telephony.uicc.policy;

import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppState;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;

import android.telephony.Rlog;
import android.telephony.SubscriptionManager;

public class DefaultPolicy extends IccPolicy {

    private static final String TAG = "DefaultPolicy";

    public SimPriority getSimPriority(int phoneId) {

        SimPriority priority = SimPriority.NONE;
        UiccController uc = UiccController.getInstance();

        if (uc != null) {
            UiccCardApplication currentApp = uc.getUiccCardApplication(phoneId,
                    UiccController.APP_FAM_3GPP);
            if (currentApp != null) {
                if (currentApp.getState() == AppState.APPSTATE_READY) {
                    priority = currentApp.getType() == AppType.APPTYPE_USIM ? SimPriority.HIGH_USIM
                            : SimPriority.HIGH_SIM;
                } else {
                    priority = SimPriority.LOCKED;
                }
            }
        }
        Rlog.d(TAG, "getSimPriority[" + phoneId + "]: " + priority.toString());
        return priority;
    }

    @Override
    public boolean isNeedPromptUserSetPrimaryCard() {
        return mMaxPriorityCount >= 2
                && (mSimPriorities[mMaxPriorityPhoneId] == SimPriority.HIGH_USIM);
    }
}
