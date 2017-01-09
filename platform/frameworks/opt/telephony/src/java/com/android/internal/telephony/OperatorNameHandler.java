
package com.android.internal.telephony;

import com.android.internal.telephony.uicc.ExtraIccRecords;
import com.android.internal.telephony.uicc.ExtraIccRecordsController;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.os.Handler;
import android.os.PersistableBundle;
import android.os.SystemProperties;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.CellLocation;
import android.telephony.Rlog;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.gsm.GsmCellLocation;
import android.text.TextUtils;
import android.util.Log;

public class OperatorNameHandler extends Handler {

    private static final boolean DBG = true;
    private static final String LOG_TAG = "OperatorNameHandler";

    private static Context mContext;
    private static OperatorNameHandler mInstance;

    public static OperatorNameHandler init(Context context) {
        synchronized (OperatorNameHandler.class) {
            if (mInstance == null) {
                mInstance = new OperatorNameHandler(context);
            } else {
                Log.wtf(LOG_TAG, "init() called multiple times!  mInstance = " + mInstance);
            }
            return mInstance;
        }
    }

    private OperatorNameHandler(Context context) {
        mContext = context;
    }

    public static OperatorNameHandler getInstance() {
        if (mInstance == null) {
            Log.wtf(LOG_TAG, "getInstance null");
        }
        return mInstance;
    }

    /**
     * Update PLMN network name,try to get operator name from SIM if ONS exists or regplmn matched
     * OPL/PNN PLMN showed priorities: OPL/PNN > ONS(CPHS) > NITZ > numeric_operator.xml > mcc+mnc
     * See 3GPP TS 22.101 for details.
     */
    public String getHighPriorityPlmn(int phoneId, String mccmnc) {
        if (!SubscriptionManager.isValidPhoneId(phoneId) || TextUtils.isEmpty(mccmnc)
                || mccmnc.length() <= 3) {
            return mccmnc;
        }

        String highPriorityPlmn = null;
        ExtraIccRecords exIccRecords = ExtraIccRecordsController.getInstance()
                .getExtraIccRecords(phoneId);
        if (exIccRecords.isSimOplPnnSupport()) {
            // Try to get OPL/PNN
            int lac = -1;
            Phone phone = PhoneFactory.getPhone(phoneId);
            CellLocation cellLoc = phone == null ? null : phone.getCellLocation();
            if (cellLoc != null && cellLoc instanceof GsmCellLocation) {
                lac = ((GsmCellLocation) cellLoc).getLac();
            }
            highPriorityPlmn = exIccRecords.getPnn(mccmnc, lac);
        }

        if (TextUtils.isEmpty(highPriorityPlmn)) {
            // Try to get ONS
            highPriorityPlmn = exIccRecords.getSimOns(mccmnc);
            if (DBG) {
                Rlog.d(LOG_TAG,
                        "Didn't get pnn from sim, try ons next. ONS = " + highPriorityPlmn);
            }
        }

        if (TextUtils.isEmpty(highPriorityPlmn)) {
            // Try to get NITZ operator name
            String propName = phoneId == 0 ? "persist.radio.nitz.operator"
                    : "persist.radio.nitz.operator" + phoneId;
            String nitzOperatorInfo = SystemProperties.get(propName);
            if (DBG)
                Rlog.d(LOG_TAG, "nitzOperatorInfo = " + nitzOperatorInfo);
            if (!TextUtils.isEmpty(nitzOperatorInfo)) {
                String[] nitzSubs = nitzOperatorInfo.split(",");
                if (nitzSubs.length == 3 && nitzSubs[2].equals(mccmnc)) {
                    highPriorityPlmn = nitzSubs[0];
                }
            }
        }

        if (TextUtils.isEmpty(highPriorityPlmn)) {
            // Try to get PLMN from numeric_operator.xml
            String tmpMccmnc = mccmnc.substring(0, 3) + Integer.parseInt(mccmnc.substring(3));
            String plmnFromXml = TeleUtils.updateOperator(tmpMccmnc, "numeric_to_operator");
            if (!tmpMccmnc.equals(plmnFromXml)) {
                highPriorityPlmn = plmnFromXml;
                if (DBG)
                    Rlog.d(LOG_TAG, "get higher priority plmn from xml : " + highPriorityPlmn);
            }
        }
        return TextUtils.isEmpty(highPriorityPlmn) ? mccmnc : highPriorityPlmn;
    }

    public String updateNetworkList(int phoneId, String[] operatorInfo) {
        Rlog.d(LOG_TAG, "updateNetworkList");
        if (!SubscriptionManager.isValidPhoneId(phoneId) || operatorInfo == null
                || operatorInfo.length < 4) {
            return null;
        }

        String operatorName = operatorInfo[0];
        String mccmncAct = operatorInfo[2];
        String stateString = operatorInfo[3];
        Rlog.d(LOG_TAG, "updateNetworkList: " + mccmncAct + " " + stateString);

        // OperatorNumeric is reported in format "mccmnc act",
        if (mccmncAct != null && mccmncAct.length() > 5) {
            String[] mccmncInfos = mccmncAct.split(" ");
            String mccmnc = mccmncInfos[0];

            if (!TextUtils.isEmpty(mccmnc)) {
                String highPriorityPlmn = getHighPriorityPlmn(phoneId, mccmnc);
                if (!mccmnc.equals(highPriorityPlmn)) {
                    operatorName = highPriorityPlmn;
                }
            }

            if (!TextUtils.isEmpty(operatorName)) {
                operatorName = TeleUtils.updateOperator(operatorName, "operator");
                //SPRD: modify for Bug 629352,Circumventing the operator name as "xx xG", if the operator in this case,
                //remove "*G" from operator name and add ACT type again.
                if (operatorName.length() > 2 && operatorName.substring(operatorName.length()-2).matches("\\dG")) {
                    operatorName = operatorName.substring(0,operatorName.length()-2);
                }
                if (getCarrierConfig().getBoolean(
                        CarrierConfigManagerEx.KEY_FEATURE_QUERY_NETWORK_RESULT_SHOW_TYPE_SUFFIX)) {
                    // Display Act as 2G/3G/4G
                    // Act code: 0-GSM(2G)/1-GSMCompact(2G)/2-UTRAN(3G)/7-E-UTRAN(4G)
                    String act = mccmncInfos.length > 1 ? mccmncInfos[1] : "";
                    switch (act) {
                        case "0":
                        case "1":
                            operatorName += " 2G";
                            break;
                        case "2":
                            operatorName += " 3G";
                            break;
                        case "7":
                            operatorName += " 4G";
                            break;

                        default:
                            Log.e(LOG_TAG, "ACT was not reported by RIL with PLMN " + mccmnc);
                            break;
                    }
                }

                if (getCarrierConfig().getBoolean(
                        CarrierConfigManagerEx.KEY_FEATURE_QUERY_NETWORK_RESULT_SHOW_STATE_SUFFIX)) {
                    // Add display state string UNKNOWN/FORBIDDEN
                    Resources res = Resources.getSystem();
                    if (OperatorInfo.State.FORBIDDEN.toString().equalsIgnoreCase(stateString)) {
                        operatorName += res
                                .getString(com.android.internal.R.string.network_inhibit);
                    } else if (OperatorInfo.State.UNKNOWN.toString().equalsIgnoreCase(stateString)) {
                        operatorName += res
                                .getString(com.android.internal.R.string.network_unknown);
                    }
                }
            }
        }

        Rlog.d(LOG_TAG, "updateNetworkList operatorName : " + operatorName);
        return operatorName;
    }

    public void updateSpnFromCarrierConfig(int phoneId, String carrierName) {
        // Update SPN specially for MVNO
        SubscriptionController controller = SubscriptionController.getInstance();
        int subId = controller.getSubIdUsingPhoneId(phoneId);
        String carrier = TeleUtils.updateOperator(carrierName,"spn");
        Rlog.d(LOG_TAG, "[carrier]= " + carrier);
        if (SubscriptionManager.isValidSubscriptionId(subId)) {
            SubscriptionInfo subInfo = controller.getActiveSubscriptionInfoForSimSlotIndex(phoneId, mContext.getOpPackageName());
            if (subInfo != null && subInfo.getNameSource() != SubscriptionManager.NAME_SOURCE_USER_INPUT && carrier!= null
                    && !carrier.equals(subInfo.getDisplayName())) {
                Rlog.d(LOG_TAG, "updateSpnFromCarrierConfig SPN[" + phoneId + "]: " + carrier);
                controller.setDisplayName(carrier, subId);
            }
        }
    }

    private PersistableBundle getCarrierConfig() {
        CarrierConfigManagerEx configManagerEx = CarrierConfigManagerEx.from(mContext);
        if (configManagerEx != null) {
            return configManagerEx.getConfigForDefaultPhone();
        }
        return new PersistableBundle();
    }
}
