/** Create by Spreadst */
package plugin.sprd.applications;

import android.app.ActivityManager;
import android.app.AddonManager;
import android.content.Context;
import android.content.pm.PackageManager;

import com.android.settings.applications.KillAppsUtils;

public class KillAppsUtilsCMCC extends KillAppsUtils implements
        AddonManager.InitialCallback {

    private Context mAddonContext;

    public KillAppsUtilsCMCC() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean isSupportKillAllApps() {
        return true;
    }

    public void startKillAllApps(Context context, ActivityManager am, PackageManager pm) {
        KillApps rpu = new KillApps(context, am, pm);
        rpu.killApp();
        rpu.killService();
    }
}