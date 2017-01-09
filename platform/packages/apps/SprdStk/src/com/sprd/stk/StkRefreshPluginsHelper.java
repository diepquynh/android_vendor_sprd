package com.sprd.stk;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.stk.R;

public class StkRefreshPluginsHelper {

    public StkRefreshPluginsHelper() {
    }

    static StkRefreshPluginsHelper mInstance;

    public static StkRefreshPluginsHelper getInstance(Context context) {
        if (mInstance != null){
            return mInstance;
        }
        mInstance = (StkRefreshPluginsHelper) new AddonManager(context).getAddon(R.string.feature_support_stknotoast, StkRefreshPluginsHelper.class);
        return mInstance;
    }

    public boolean refreshNoToast() {
        return false;
    }
}
