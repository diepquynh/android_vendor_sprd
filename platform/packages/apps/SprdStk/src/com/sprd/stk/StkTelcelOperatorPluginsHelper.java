package com.sprd.stk;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.stk.R;

public class StkTelcelOperatorPluginsHelper {

    public StkTelcelOperatorPluginsHelper() {
    }

    static StkTelcelOperatorPluginsHelper mInstance;

    public static StkTelcelOperatorPluginsHelper getInstance(Context context) {
        if (mInstance != null){
            return mInstance;
        }
        mInstance = (StkTelcelOperatorPluginsHelper) new AddonManager(context).
                getAddon(R.string.feature_support_stktelcel,StkTelcelOperatorPluginsHelper.class);
        return mInstance;
    }

    public boolean isTelcelOperator()  {
        Log.d("StkTelcelOperatorPluginsHelper" , "isTelcelOperator()");
        return false;
    }
}
