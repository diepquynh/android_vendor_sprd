package com.sprd.stk;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.stk.R;

public class StkSetupMenuPluginsHelper {

    public StkSetupMenuPluginsHelper() {
    }

    static StkSetupMenuPluginsHelper mInstance;

    public static StkSetupMenuPluginsHelper getInstance(Context context) {
        if (mInstance != null){
            return mInstance;
        }
        mInstance = (StkSetupMenuPluginsHelper) new AddonManager(context).getAddon(R.string.feature_support_setupmenu, StkSetupMenuPluginsHelper.class);
        return mInstance;
    }

    public boolean isShowSetupMenuTitle() {
        return false;
    }
}
