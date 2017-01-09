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

/**
 * Created by spreadst
 */

public class TeleServiceSupportOperator extends TeleServicePluginsHelper implements
        AddonManager.InitialCallback {

    private Context mAddonContext;

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
}
