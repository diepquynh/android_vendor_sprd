package plugin.sprd.keyguardTelephonyAddon;

import com.sprd.keyguard.KeyguardPluginsHelper;

import android.app.AddonManager;
import android.content.Context;
import android.provider.Settings;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

public class KeyguardFeaturesForOperator extends KeyguardPluginsHelper implements AddonManager.InitialCallback {

    public static final String LOG_TAG = "KeyguardPluginForCUCC";
    private Context mAddonContext;

    public KeyguardFeaturesForOperator() {
    }
    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean makeEmergencyInvisible() {
        boolean hasIccCard = false;
        TelephonyManager telePhonyManager = TelephonyManager.from(mAddonContext);
        int numPhones = telePhonyManager.getPhoneCount();
        for (int i = 0; i < numPhones; i++) {
            hasIccCard |= telePhonyManager.hasIccCard(i);
        }
        boolean isAirPlaneMode = (Settings.Global.getInt(mAddonContext.getContentResolver(),
                Settings.Global.AIRPLANE_MODE_ON, 0) == 1);
        if (isAirPlaneMode || !hasIccCard) {
            return true;
        }
        return false;
    }
}
