
package plugin.sprd.operatorpolicy;

import android.content.Context;
import android.telephony.Rlog;
import android.text.TextUtils;

import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppState;
import com.android.internal.telephony.uicc.IccCardStatus.CardState;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.TeleUtils;
import com.android.internal.telephony.uicc.policy.IccPolicy;
import com.android.internal.telephony.uicc.policy.IccPolicy.SimPriority;

public class CUCCPolicy extends IccPolicy {

    private static final String TAG = "CUCCPolicy";
    private static final String[] ICCID_PREFIX_CODES_CUCC = {
            "898601", "898609"
    };

    public SimPriority getSimPriority(int phoneId) {
        boolean isCUCCSimCard = false;

        SimPriority priority = SimPriority.NONE;
        UiccController uc = UiccController.getInstance();

        if (uc != null) {
            IccRecords iccRecords = uc.getIccRecords(phoneId, UiccController.APP_FAM_3GPP);

            // To determine whether SIM card is CUCC card according to its ICCID.
            if (iccRecords != null) {
                String iccId = iccRecords.getIccId();
                if (!TextUtils.isEmpty(iccId)) {
                    for (String iccIdPrefix : ICCID_PREFIX_CODES_CUCC) {
                        if (iccId.startsWith(iccIdPrefix)) {
                            isCUCCSimCard = true;
                            break;
                        }
                    }
                }
            }

            if (uc != null) {
                UiccCardApplication currentApp = uc.getUiccCardApplication(phoneId,
                        UiccController.APP_FAM_3GPP);

                if (currentApp != null) {
                    if (currentApp.getState() == AppState.APPSTATE_READY) {
                        boolean isUsimCard = currentApp.getType() == AppType.APPTYPE_USIM;
                        Rlog.d(TAG, "phone " + phoneId + " : isCUCCSimCard = " + isCUCCSimCard
                                + " isUsimCard = " + isUsimCard);
                        if (isCUCCSimCard) {
                            priority = isUsimCard ? SimPriority.HIGH_USIM : SimPriority.LOW_SIM;
                        } else {
                            priority = isUsimCard ? SimPriority.LOW_USIM : SimPriority.LOWEST;
                        }
                    } else {
                        priority = SimPriority.LOCKED;
                    }

                }
            }
        }

        Rlog.d(TAG, "getSimPriority[" + phoneId + "]: " + priority.toString());
        return priority;
    }

    public boolean isNeedPromptUserSetPrimaryCard() {
        return mMaxPriorityCount >= 2 &&
                (mSimPriorities[mMaxPriorityPhoneId] == SimPriority.HIGH_USIM
                        || mSimPriorities[mMaxPriorityPhoneId] == SimPriority.LOW_USIM
                        || mSimPriorities[mMaxPriorityPhoneId] == SimPriority.LOW_SIM);
    }
}
