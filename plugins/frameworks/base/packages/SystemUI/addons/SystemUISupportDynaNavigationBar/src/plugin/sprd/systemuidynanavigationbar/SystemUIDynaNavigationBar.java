
package plugin.sprd.systemuidynanavigationbar;

import android.app.AddonManager;
import android.content.Context;
import com.sprd.systemui.SystemUIDynaNavigationBarUtils;

public class SystemUIDynaNavigationBar extends SystemUIDynaNavigationBarUtils implements
        AddonManager.InitialCallback {

    private static final String TAG = "SystemUIDynaNavigationBar";
    private Context mAddonContext;

    public SystemUIDynaNavigationBar() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean isSupportDynaNaviBar() {
        return true;
    }
}
