package plugin.sprd.universaldualcardswitchutilsforsettings;


import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.settings.sim.UniversalDualCardSwitchPluginHelper;

public class UniversalDualCardSwitchUtilsForSettings extends UniversalDualCardSwitchPluginHelper implements AddonManager.InitialCallback {

    public static final String LOG_TAG = "UniversalDualCardSwitchUtils";
    //private Context mAddonContext;

    public UniversalDualCardSwitchUtilsForSettings() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        //mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean isUniversalDualCardSwitchOn() {
        Log.d(LOG_TAG, "isUniversalDualCardSwitchOn = true");
        return true;
    }
}
