package android.telephony;

import android.app.AddonManager;
import android.util.Log;
import android.content.Context;

import android.content.Intent;
import android.telecom.VideoProfile;
import com.android.internal.R;

public class LowBatteryCallUtils {
    private static final String TAG = "LowBatteryCallUtils";
    static LowBatteryCallUtils sInstance;

    public static LowBatteryCallUtils getInstance() {
       Log.d(TAG, "enter LowBatteryCallUtils");
        if (sInstance != null)
            return sInstance;
        sInstance = (LowBatteryCallUtils) AddonManager.getDefault().getAddon(R.string.feature_AddonLowBatteryCallUtils,LowBatteryCallUtils.class);
        return sInstance;
    }

    public LowBatteryCallUtils() {
    }

    public boolean isBatteryLow() {
        Log.d(TAG, "not reliance project, isBatteryLow = false");
        return false;
    }

    public void showLowBatteryDialDialog(Context context,Intent intent,boolean isDialingByDialer) {
        Log.d(TAG, "showLowBatteryDialDialog");
    }

    public void showLowBatteryInCallDialog(Context context,android.telecom.Call telecomCall) {
        Log.d(TAG, "showLowBatteryInCallDialog");
    }

    public void showLowBatteryChangeToVideoDialog(android.telecom.Call telecomCall,VideoProfile videoProfile) {
        Log.d(TAG, "showLowBatteryChangeToVideoDialog");
    }

}
