package plugin.sprd.teleservicesupportoperator;

import android.app.AddonManager;
import android.content.Context;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.telephony.OperatorInfo;
import com.android.internal.telephony.TeleUtils;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccRecords;
import com.sprd.phone.TeleServicePluginsHelper;

/**
 * Plugin implementation for telephony service related modification.
 * @Author SPRD
 */
public class TeleServiceSupportOperator extends TeleServicePluginsHelper implements
        AddonManager.InitialCallback {

    private Context mAddonContext;

    // Issuer identification number for CUCC SIMs
    private static final String CUCC_SIM = "898601";
    // Issuer identification number for HK CUCC SIMs
    private static final String HK_CUCC_SIM = "8985207";
    // CMCC PLMN List
    private static final String CMCC_PLMN = " 46000 46002 46004 46007 46008 46020 45412 ";

    public TeleServiceSupportOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    /**
     * Bug#476003 CUCC case 5.1.6.7.14.2
     * If it is a CUCC sim and in CMCC PLMN return false
     * Or FPLMN return false.
     * @param ni OperatorInfo
     * @param phoneId
     * @return false if it is a CUCC sim and in CMCC PLMN
     */
    public boolean getDisplayNetworkList(OperatorInfo ni, int phoneId) {
        //operatorNumeric is reported as PLMN + ACT
        if ((ni.getOperatorNumeric().length() >= 5 &&
                CMCC_PLMN.contains(" " + ni.getOperatorNumeric().substring(0,5) + " ")
                && isCuccSimCard(phoneId)) || ni.getState() == OperatorInfo.State.FORBIDDEN) {
            return false;
        }
        return true;
    }

    /**
     * Bug#476003
     * Check is CUCC sim including HK CUCC
     * Firstly, try to match by IccId IIN.
     * If IccId not available, then try to match by PLMN.
     * @param phoneId
     * @return true if is CUCC sim
     */
    private boolean isCuccSimCard(int phoneId) {
        UiccController uc = UiccController.getInstance();

        if (uc != null) {
            IccRecords iccRecords = uc.getIccRecords(phoneId, UiccController.APP_FAM_3GPP);
            if (iccRecords != null) {
                String iccId = iccRecords.getIccId();
                // Firstly, try to check by IccId Issuer identification number (IIN)
                if (!TextUtils.isEmpty(iccId) && iccId.length() >= HK_CUCC_SIM.length()
                        && (iccId.substring(0, CUCC_SIM.length()).equals(CUCC_SIM) ||
                            iccId.substring(0, HK_CUCC_SIM.length()).equals(HK_CUCC_SIM))) {
                    return true;
                }

                //Sometimes IccId cannot be read, use MCCMNC to match.
                String mccMnc = iccRecords.getOperatorNumeric();
                if (mccMnc != null && mccMnc.length() > 3) {
                    String mcc = mccMnc.substring(0, 3);
                    String mnc = mccMnc.substring(3);
                    try {
                        String tmpMccMnc = mcc + Integer.parseInt(mnc);
                        String operatorName = TeleUtils.updateOperator(tmpMccMnc, "numeric_to_operator");
                        if ("China Unicom".equals(operatorName) || "Unicom".equals(operatorName)) {
                            return true;
                        }
                    } catch (NumberFormatException e) {
                        Log.e("TeleServiceSupportOperator","Invalid MNC code: " + mnc);
                        return false;
                    }
                }
            }
        }
        return false;
    }
}
