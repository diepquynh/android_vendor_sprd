package test.sprd.helloworld.plugin;

import android.app.Activity;
import test.sprd.helloworld.R;
import android.app.AddonManager;
import android.content.Context;

public class LeavingUtils {

    static LeavingUtils sInstance;

    public static LeavingUtils getInstance(Context context) {
        if (sInstance != null) return sInstance;
        sInstance = (LeavingUtils) AddonManager.getDefault().getAddon(R.string.feature_leaving, LeavingUtils.class);
        return sInstance;
    }

    public LeavingUtils() {
    }

    public boolean canIGo(final Activity activity) {
        return true;
    }
}
