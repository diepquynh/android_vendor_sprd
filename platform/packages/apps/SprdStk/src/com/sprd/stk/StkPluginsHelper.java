package com.sprd.stk;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.stk.R;

public class StkPluginsHelper {

    public StkPluginsHelper() {
    }

    static StkPluginsHelper mInstance;

    public static StkPluginsHelper getInstance(Context context) {
        if (mInstance != null){
            return mInstance;
        }
        mInstance = (StkPluginsHelper) new AddonManager(context).getAddon(R.string.feature_support_sessionend, StkPluginsHelper.class);
        return mInstance;
    }

    public boolean needCloseMenu() {
        Log.d("StkAppPlugin" , "needCloseMenu in StkPluginsHelper");
        return false;
    }
}
