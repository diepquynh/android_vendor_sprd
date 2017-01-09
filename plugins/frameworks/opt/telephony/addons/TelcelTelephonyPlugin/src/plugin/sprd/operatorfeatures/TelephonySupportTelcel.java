package plugin.sprd.operatorfeatures;

import android.app.AddonManager;
import android.content.Context;
import com.android.internal.telephony.plugin.TelephonyForTelcelPluginsUtils;
import android.util.Log;
import android.os.PowerManager;

public class TelephonySupportTelcel extends TelephonyForTelcelPluginsUtils implements
        AddonManager.InitialCallback {
    private static String LOG_TAG = "TelephonySupportTelcel";

    private Context mAddonContext;
    public TelephonySupportTelcel(){
    }
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }
    public void needRebootPhone() {
         Log.d(LOG_TAG, "needRebootPhone method");
         PowerManager pm = (PowerManager) mAddonContext.getSystemService(Context.POWER_SERVICE);
         pm.reboot(null);
    }
}
