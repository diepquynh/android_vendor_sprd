package plugin.sprd.teleservicesupportoperator;

import android.app.AddonManager;
import android.content.Context;
import android.preference.ListPreference;
import android.util.Log;

import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppType;
import com.android.phone.R;
import com.sprd.phone.TeleServicePluginsHelper;


public class TeleServiceSupportOperator extends TeleServicePluginsHelper implements
        AddonManager.InitialCallback {

    private Context mAddonContext;
    private final String TAG = "TeleServiceSupportOperator";

    public TeleServiceSupportOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean needSetByPrimaryCardUsim(int primaryCard) {
        UiccController uc = UiccController.getInstance();
        if (uc != null) {
            UiccCardApplication currentApp = uc.getUiccCardApplication(primaryCard,
                    UiccController.APP_FAM_3GPP);
            if (currentApp != null) {
                return currentApp.getType() == AppType.APPTYPE_USIM;
            }
        }
        return false;
    }

    public ListPreference setLtePreferenceValues(ListPreference buttonLtePreferredNetworkMode) {
        Log.d(TAG, "setLtePreferenceValues");
        buttonLtePreferredNetworkMode.setEntries(R.array.lte_preferred_networks_choices);
        buttonLtePreferredNetworkMode.setEntryValues(R.array.network_mode_choices_values);
        mButtonLtePreferredNetworkMode = buttonLtePreferredNetworkMode;
        return buttonLtePreferredNetworkMode;
    }

    public ListPreference updateLtePreferenceSummary(String preferredNetworkMode) {
        if (mButtonLtePreferredNetworkMode != null) {
            if (String.valueOf(TeleServicePluginsHelper.PREFERRED_NETWORK_MODE_4G_3G_2G).equals(preferredNetworkMode)) {
                mButtonLtePreferredNetworkMode.setValueIndex(0);
                mButtonLtePreferredNetworkMode.setSummary(mButtonLtePreferredNetworkMode.getEntry());
            } else {
                String errMsg = "Invalid Network Mode (" + preferredNetworkMode + "). Ignore.";
                Log.d(TAG, errMsg);
                mButtonLtePreferredNetworkMode.setSummary(errMsg);
            }
        }
        return mButtonLtePreferredNetworkMode;
    }

    public boolean isSupportPlmn(){
        return false;
    }

    /**
     * CMCC new case : Not allow user to set network type to 3g2g ;
     * When insert SIM card,remove MobileNetworkSetting preference
     * see bug522182
     */
    public boolean isMainSlotInsertSIMCard(int phoneId) {
        return !needSetByPrimaryCardUsim(phoneId);
    }

    /**
     * CMCC new case : allow user to set call time forward in 3g2g ;
     * see bug552776
     */
    public boolean isCallTimeForwardSupport() {
        Log.d(TAG, "isCallTimeForwardSupport plugin");
        return true;
    }

    public boolean callOutOptionEnable() {
        Log.d(TAG, "callOutOptionEnable false.");
        return false;
    }
}
