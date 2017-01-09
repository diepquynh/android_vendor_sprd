package plugin.sprd.policysupportoperator;

import android.content.Context;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppState;
import com.android.internal.telephony.uicc.IccCardStatus.CardState;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.TeleUtils;
import com.android.internal.telephony.policy.IccPolicy;
import com.android.internal.telephony.policy.IccPolicy.SimPriority;


public class ReliancePolicy extends IccPolicy{

    private static final String TAG = "ReliancePolicy";
    private static final String OPERATOR_NAME_JIO = "Jio";

    public SimPriority getSimPriority(int phoneId) {
        boolean isJioSimCard = false;

        SimPriority priority = SimPriority.NONE;
        UiccController uc = UiccController.getInstance();

        if (uc != null) {
            IccRecords iccRecords = uc.getIccRecords(phoneId, UiccController.APP_FAM_3GPP);

            if (iccRecords != null) {
                String mccMnc = iccRecords.getOperatorNumeric();
                if (!TextUtils.isEmpty(mccMnc)) {
                    String mcc = mccMnc.substring(0, 3);
                    String mnc = mccMnc.substring(3);
                    try {
                         String tmpMccMnc = mcc + Integer.parseInt(mnc);

                         String operatorName = TeleUtils
                                .updateOperator(tmpMccMnc, "numeric_to_operator");
                         if (OPERATOR_NAME_JIO.equalsIgnoreCase(operatorName)) {
                             isJioSimCard = true;
                          }
                    } catch (NumberFormatException ex){
                        Log.d(TAG, "mcc mnc is invalid");
                    }
                }
            }

            if (uc != null) {
                UiccCardApplication currentApp = uc.getUiccCardApplication(phoneId,
                        UiccController.APP_FAM_3GPP);
                if (currentApp != null) {

                    if (currentApp.getState() == AppState.APPSTATE_READY) {
                        boolean isUsimCard = currentApp.getType() == AppType.APPTYPE_USIM;
                        Log.d(TAG, "phone " + phoneId + " : isJioSimCard = "
                                + isJioSimCard + " isUsimCard = " + isUsimCard);
                        if (isJioSimCard) {
                            priority = isUsimCard ? SimPriority.HIGH_USIM : SimPriority.LOW_SIM;
                        } else {
                            priority = isUsimCard ? SimPriority.LOW_USIM : SimPriority.LOWER_USIM_SIM;
                        }
                    } else {
                        priority = SimPriority.LOCKED;
                    }

                }
            }
        }

        Log.d(TAG, "get sim " + phoneId + " priority:" + priority.toString());
        return priority;
    }

    public boolean isPrimaryCardNeedManualSet() {
        return mMaxPriorityCount >= 2 &&
                (mSimPriorities[mMaxPriorityPhoneId] == SimPriority.HIGH_USIM
                || mSimPriorities[mMaxPriorityPhoneId] == SimPriority.LOW_SIM
                || mSimPriorities[mMaxPriorityPhoneId] == SimPriority.LOW_USIM
                || mSimPriorities[mMaxPriorityPhoneId] == SimPriority.LOWER_USIM_SIM);
    }
}
