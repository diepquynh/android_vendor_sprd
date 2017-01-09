
package plugin.sprd.keyguardoperator;

import com.sprd.keyguard.KeyguardPluginsHelper;
import android.app.AddonManager;
import android.content.Context;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.util.Log;

public class KeyguardFeaturesForOperator extends KeyguardPluginsHelper implements
        AddonManager.InitialCallback {

    public static final String LOG_TAG = "KeyguardOperatorForCUCC";
    private Context mAddonContext;

    public KeyguardFeaturesForOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    /**
     * CUCC case:
     * Can not show spn when plmn should be shown.
     */
    public String updateNetworkName(Context context, boolean showSpn, String spn, boolean showPlmn,
            String plmn) {
        Log.d(LOG_TAG, "updateNetworkName showSpn=" + showSpn + " spn=" + spn
                + " showPlmn=" + showPlmn + " plmn=" + plmn);
        StringBuilder str = new StringBuilder();
        if (showPlmn && plmn != null) {
            if (context.getString(com.android.internal.R.string.emergency_calls_only)
                    .equals(plmn)) {
                plmn = context.getResources()
                        .getString(com.android.internal.R.string.lockscreen_carrier_default);
            }
            str.append(plmn);
        } else if (showSpn && spn != null) {
            str.append(spn);
        }
        return str.toString();
    }

    /**
    * @return false don't show eccButton, true show eccButton
    */
    public boolean makeEmergencyVisible() {
        boolean hasIccCard = false;
        TelephonyManager telePhonyManager = TelephonyManager.from(mAddonContext);
        int numPhones = telePhonyManager.getPhoneCount();
        for (int i = 0; i < numPhones; i++) {
            hasIccCard |= telePhonyManager.hasIccCard(i);
        }
        boolean isAirPlaneMode = (Settings.Global.getInt(mAddonContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) == 1);
        if (isAirPlaneMode || !hasIccCard) {
            Log.d(LOG_TAG, "ShowEccButtonInPlugin : false" );
            return false;
        }
        Log.d(LOG_TAG, "ShowEccButtonInPlugin : true" );
        return true;
    }

    /* SPRD: modify by BUG 540847 @{ */
    public boolean getBoolShowRatFor2G() {
        return true;
    }

    protected boolean showLteFor4G() {
        return true;
    }
    /* @} */
}
