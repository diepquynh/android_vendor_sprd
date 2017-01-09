
package plugin.sprd.keyguardoperator;

import com.sprd.keyguard.KeyguardPluginsHelper;
import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

public class KeyguardFeaturesForOperator extends KeyguardPluginsHelper implements
        AddonManager.InitialCallback {

    public static final String LOG_TAG = "KeyguardFeaturesForTelcel";
    private Context mAddonContext;

    public KeyguardFeaturesForOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    /* SPRD: Add for BUG 562188,don't append ACT after PLMN. @{ */
    public boolean getBoolAppendRAT() {
        return false;
    }
    /* @} */
}
