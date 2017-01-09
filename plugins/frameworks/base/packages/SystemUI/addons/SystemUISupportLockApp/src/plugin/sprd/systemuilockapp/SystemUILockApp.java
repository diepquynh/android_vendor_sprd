
package plugin.sprd.systemuilockapp;

import android.app.AddonManager;
import android.content.Context;
import src.com.sprd.systemui.SystemUILockAppUtils;

public class SystemUILockApp extends SystemUILockAppUtils implements
        AddonManager.InitialCallback {

    private static final String TAG = "SystemUILockApp";
    private Context mAddonContext;

    public SystemUILockApp() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean isSupportLockApp() {
        return true;
    }
}
