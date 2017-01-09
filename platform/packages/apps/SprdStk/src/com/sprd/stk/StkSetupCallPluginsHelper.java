package com.sprd.stk;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.stk.R;

public class StkSetupCallPluginsHelper {

    public StkSetupCallPluginsHelper() {
    }

    static StkSetupCallPluginsHelper mInstance;

    public static StkSetupCallPluginsHelper getInstance(Context context) {
        if (mInstance != null){
            return mInstance;
        }
        mInstance = (StkSetupCallPluginsHelper) new AddonManager(context).getAddon(R.string.feature_support_setupcall_noconfirm, StkSetupCallPluginsHelper.class);
        return mInstance;
    }

    public boolean needConfirm() {
        return false;
    }
}
