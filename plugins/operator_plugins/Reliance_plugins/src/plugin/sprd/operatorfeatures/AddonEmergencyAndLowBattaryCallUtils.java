package plugin.sprd.CallUtilsPlugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.os.SystemProperties;
import com.android.telephony.EmergencyAndLowBattaryCallUtils;
import android.os.BatteryManager;
import android.content.Intent;
import android.content.IntentFilter;


public class AddonEmergencyAndLowBattaryCallUtils extends EmergencyAndLowBattaryCallUtils implements AddonManager.InitialCallback {

    public static final String TAG = "AddonEmergencyAndLowBattaryCallUtils";
    private Context mAddonContext;

    public AddonEmergencyAndLowBattaryCallUtils() {
    }


    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean isBatteryLow() {

        Log.d(TAG, "in isBatteryLow ");

        Intent batteryInfoIntent = mAddonContext.registerReceiver(null, new IntentFilter(
                Intent.ACTION_BATTERY_CHANGED));

        int current = batteryInfoIntent.getIntExtra("level", 0);
        int total = batteryInfoIntent.getIntExtra("scale", 0);

        if (current * 1.0 / total < 0.15) {
            Log.d(TAG, "in isBattery Low 15%");
            return true;
        } else {
            Log.d(TAG, "in isBatteryLow high 15%");
            return false;
        }
    }

    @Override
    public String AddEmergencyNO(int key, String fastcall) {

        Log.d(TAG, "AddonCallUtilPlugins AddEmergencyNO");
        String fastCallNo = fastcall;
        if (key == 9) {

            fastCallNo = "112";

        }
        return fastCallNo;
    }
}
