
package plugin.sprd.mmidialogdismiss;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;

import com.android.services.telephony.plugin.MMICompleteUtils;

public class AddonMMICompletePlugin extends MMICompleteUtils implements
        AddonManager.InitialCallback {
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public AddonMMICompletePlugin() {
    }

    public boolean isDismissMMIDialog() {
        Log.i("AddonMMICompletePlugin", "isDismissMMIDialog=true");
        return true;
    }
}
