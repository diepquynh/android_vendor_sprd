package plugin.sprd.keyguardTelephonyAddon;

import com.sprd.keyguard.KeyguardPluginsHelper;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.telephony.ServiceState;

public class KeyguardFeaturesForOperator extends KeyguardPluginsHelper implements AddonManager.InitialCallback {

    public static final String LOG_TAG = "KeyguardSupportOrange";
    private Context mAddonContext;

    public KeyguardFeaturesForOperator() {
    }
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    protected boolean show3G(ServiceState state) {
        String mccmnc = state.getOperatorNumeric();
        if (mccmnc != null && mccmnc.startsWith("460")) {
            return true;
        }
        return false;
    }
}
