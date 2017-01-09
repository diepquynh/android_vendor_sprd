package plugin.sprd.universaldualcardswitchutilforstelephony;


import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.sprd.phone.UniversalDualCardSwitchPluginHelper;

public class UniversalDualCardSwitchUtilsForTelephony extends UniversalDualCardSwitchPluginHelper implements AddonManager.InitialCallback {

    public static final String LOG_TAG = "UniversalDualCardSwitchUtils";
    //private Context mAddonContext;

    public UniversalDualCardSwitchUtilsForTelephony() {
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
