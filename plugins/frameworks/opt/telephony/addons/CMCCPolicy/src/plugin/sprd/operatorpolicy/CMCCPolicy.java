
package plugin.sprd.operatorpolicy;

import android.content.Context;
import android.os.SystemProperties;
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

public class CMCCPolicy extends IccPolicy {

    private static final String TAG = "CMCCPolicy";
    private static final String ICCID_PREFIX_CODE_CHINA = "8986";
    private static final String[] ICCID_PREFIX_CODES_CMCC = {
            "898600", "898602"
    };

    public SimPriority getSimPriority(int phoneId) {
        boolean isCMCCSimCard = false;
        boolean isChineseOperator = false;

        SimPriority priority = SimPriority.NONE;
        UiccController uc = UiccController.getInstance();

        if (uc != null) {
            IccRecords iccRecords = uc.getIccRecords(phoneId, UiccController.APP_FAM_3GPP);

            // To determine whether SIM card is CMCC card according to its ICCID.
            if (iccRecords != null) {
                String iccId = iccRecords.getIccId();
                if (!TextUtils.isEmpty(iccId)) {
                    if (iccId.startsWith(ICCID_PREFIX_CODE_CHINA)) {
                        isChineseOperator = true;

                        for (String iccIdPrefix : ICCID_PREFIX_CODES_CMCC) {
                            if (iccId.startsWith(iccIdPrefix)) {
                                isCMCCSimCard = true;
                                break;
                            }
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
                        Rlog.d(TAG,
                                "phone " + phoneId + " : isChineseOperator = " + isChineseOperator
                                        + " isCMCCSimCard = " + isCMCCSimCard + " isUsimCard = "
                                        + isUsimCard);
                        if (isChineseOperator) {
                            if (isCMCCSimCard) {
                                priority = isUsimCard ? SimPriority.HIGH_USIM : SimPriority.HIGH_SIM;
                            } else {
                                priority = SimPriority.LOWEST;
                            }
                        } else {
                            priority = isUsimCard ? SimPriority.LOW_USIM : SimPriority.LOW_SIM;
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
                        || mSimPriorities[mMaxPriorityPhoneId] == SimPriority.HIGH_SIM
                        || mSimPriorities[mMaxPriorityPhoneId] == SimPriority.LOW_USIM
                        || mSimPriorities[mMaxPriorityPhoneId] == SimPriority.LOW_SIM);
    }

    /**
     * SPRD modified for CMCC new case,see bug632507
     */
    public void updateSimPriorities() {
        super.updateSimPriorities();
        SimPriority lowSimPriority = null;
        for (int i = 0;i<PHONE_COUNT;i++) {
            if( i!= mMaxPriorityPhoneId) {
                lowSimPriority = mSimPriorities[i];
            }
        }

        if (mSimPriorities[mMaxPriorityPhoneId].compareTo(SimPriority.HIGH_SIM) >= 0
                && lowSimPriority.compareTo(SimPriority.NONE) > 0 && lowSimPriority.compareTo(SimPriority.HIGH_SIM) < 0) {
            SystemProperties.set("persist.radio.network.unable","true");
        } else {
            SystemProperties.set("persist.radio.network.unable","false");
        }
    }
}
