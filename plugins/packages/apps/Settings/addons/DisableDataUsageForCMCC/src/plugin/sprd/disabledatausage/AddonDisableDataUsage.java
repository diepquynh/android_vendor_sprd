package plugin.sprd.disabledatausage;

import android.app.AddonManager;
import android.content.Context;
import com.android.settings.DisableDataUsage;

public class AddonDisableDataUsage extends DisableDataUsage implements AddonManager.InitialCallback {

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    public boolean isCmcc(){
        return true;
    }
}
