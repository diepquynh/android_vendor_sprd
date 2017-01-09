package com.sprd.stk;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.stk.R;

public class StkCuccOperatorPluginsHelper {

    public StkCuccOperatorPluginsHelper() {
    }

    static StkCuccOperatorPluginsHelper mInstance;

    public static StkCuccOperatorPluginsHelper getInstance(Context context) {
        if (mInstance != null){
            return mInstance;
        }
        mInstance = (StkCuccOperatorPluginsHelper) new AddonManager(context).
                getAddon(R.string.feature_support_sekcucc,StkCuccOperatorPluginsHelper.class);
        return mInstance;
    }

    public boolean isCUCCOperator()  {
        Log.d("StkCuccOperatorPluginsHelper" , "isCUCCOperator()");
        return false;
    }
}
