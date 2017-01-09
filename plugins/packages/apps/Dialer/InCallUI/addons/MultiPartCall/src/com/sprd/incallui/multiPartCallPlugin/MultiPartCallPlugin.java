package com.sprd.incallui.multiPartCallPlugin;

import android.app.AddonManager;
import android.content.Context;
import com.android.sprd.incallui.MultiPartCallHelper;

public class MultiPartCallPlugin extends MultiPartCallHelper implements
        AddonManager.InitialCallback {

    private static final String TAG = MultiPartCallPlugin.class.getSimpleName();
    private Context mContext;

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mContext = context;
        return clazz;
    }

    public void MultiPartCallPlugin() {
    }

    @Override
    public boolean isSupportMultiPartCall() {
        return true;
    }
}
