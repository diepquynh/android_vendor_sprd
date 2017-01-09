
package plugin.sprd.dynanavigationbar;

import android.app.AddonManager;
import android.content.Context;
import com.sprd.settings.navigation.DynaNavigationBarPluginsUtils;


public class SettingsForDynaNavigationBar extends DynaNavigationBarPluginsUtils implements
        AddonManager.InitialCallback {

    private static final String TAG = "SettingsForDynaNavigationBar";
    private Context mAddonContext;

    public SettingsForDynaNavigationBar() {
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
