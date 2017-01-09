package plugin.sprd.logEmergencyCalls;

import android.app.AddonManager;
import android.content.Context;

import com.sprd.server.telecom.CallLogUtils;

// SPRD: modify for bug532286.
public class AddonTelecomPlugin extends CallLogUtils implements AddonManager.InitialCallback {

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    // default constructor
    public AddonTelecomPlugin() {
    }

    // SPRD: modify for bug532286.
    public boolean okToLogEmergencyNumber() {
        return true;
    }
}
