
package plugin.sprd.supportoperator;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.systemui.statusbar.policy.SystemUIPluginsHelper;

public class SystemUIFeaturesForOperator extends SystemUIPluginsHelper implements AddonManager.InitialCallback {

    private static final String TAG = "SystemUIPlugin";

    public SystemUIFeaturesForOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public boolean needShowDataTypeIcon() {
        Log.d(TAG, "[needShowDataTypeIcon] : false");
        return false;
    }

    /* SPRD: Add for BUG 562188,don't append ACT after PLMN. @{ */
    public boolean getBoolAppendRAT() {
        return false;
    }
    /* @} */
}
