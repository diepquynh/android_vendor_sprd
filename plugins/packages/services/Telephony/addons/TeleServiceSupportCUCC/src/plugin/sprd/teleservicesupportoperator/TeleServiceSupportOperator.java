package plugin.sprd.teleservicesupportoperator;

import android.app.AddonManager;
import android.content.Context;
import android.content.res.Resources;
import android.preference.ListPreference;
import android.text.TextUtils;
import android.util.Log;
import com.android.phone.R;

import com.android.internal.telephony.OperatorInfo;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccRecords;
import com.sprd.phone.TeleServicePluginsHelper;


public class TeleServiceSupportOperator extends TeleServicePluginsHelper implements
        AddonManager.InitialCallback {

    private Context mAddonContext;
    /** Bug#476003 @{ **/
    private static final String CUCC_SIM = "898601";
    private static final String CMCC_PLMN = " 46000 46002 46007 46008 46020 ";
    /** @} **/

    public TeleServiceSupportOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public ListPreference setLtePreferenceValues(ListPreference buttonLtePreferredNetworkMode) {
        buttonLtePreferredNetworkMode.setEntries(R.array.lte_network_mode_choices);
        buttonLtePreferredNetworkMode.setEntryValues(R.array.lte_network_mode_choices_values);
        mButtonLtePreferredNetworkMode = buttonLtePreferredNetworkMode;
        return buttonLtePreferredNetworkMode;
    }

    public ListPreference updateLtePreferenceSummary(String preferredNetworkMode) {
        if (mButtonLtePreferredNetworkMode != null) {
            if (String.valueOf(PREFERRED_NETWORK_MODE_4G_3G_2G).equals(preferredNetworkMode)) {
                mButtonLtePreferredNetworkMode.setValueIndex(3);
                mButtonLtePreferredNetworkMode.setSummary(mButtonLtePreferredNetworkMode.getEntry());
            } else if (String.valueOf(PREFERRED_NETWORK_MODE_3G_2G).equals(preferredNetworkMode)) {
                mButtonLtePreferredNetworkMode.setValueIndex(2);
                mButtonLtePreferredNetworkMode.setSummary(mButtonLtePreferredNetworkMode.getEntry());
            } else if (String.valueOf(PREFERRED_NETWORK_MODE_3G_ONLY).equals(preferredNetworkMode)) {
                mButtonLtePreferredNetworkMode.setValueIndex(1);
                mButtonLtePreferredNetworkMode.setSummary(mButtonLtePreferredNetworkMode.getEntry());
            } else if (String.valueOf(PREFERRED_NETWORK_MODE_2G_ONLY).equals(preferredNetworkMode)) {
                mButtonLtePreferredNetworkMode.setValueIndex(0);
                mButtonLtePreferredNetworkMode.setSummary(mButtonLtePreferredNetworkMode.getEntry());
            } else {
                String errMsg = "Invalid Network Mode (" + preferredNetworkMode + "). Ignore.";
                Log.d("TeleServiceSupportOperator",errMsg);
                mButtonLtePreferredNetworkMode.setSummary(errMsg);
            }
        }
        return mButtonLtePreferredNetworkMode;
    }

    /**
     * Bug#476003
     * If it is a CUCC sim and in CMCC PLMN return false
     * @param ni OperatorInfo
     * @param phoneId
     * @return false if it is a CUCC sim and in CMCC PLMN
     */
    public boolean getDisplayNetworkList(OperatorInfo ni, int phoneId) {
        //operatorNumeric is reported as PLMN + ACT
        if (ni.getOperatorNumeric().length() >= 5 &&
                CMCC_PLMN.contains(" " + ni.getOperatorNumeric().substring(0,5) + " ")
                && isCuccSimCard(phoneId)) {
            return false;
        }
        return true;
    }

    /**
     * Bug#476003
     * Check is CUCC sim
     * @param phoneId
     * @return true if is CUCC sim
     */
    private boolean isCuccSimCard(int phoneId) {
        UiccController uc = UiccController.getInstance();

        if (uc != null) {
            IccRecords iccRecords = uc.getIccRecords(phoneId, UiccController.APP_FAM_3GPP);
            if (iccRecords != null) {
                String iccId = iccRecords.getIccId();
                if (!TextUtils.isEmpty(iccId) && iccId.length() >= CUCC_SIM.length()
                        && iccId.substring(0, CUCC_SIM.length()).equals(CUCC_SIM)) {
                    return true;
                }
            }
        }
        return false;
    }
}
