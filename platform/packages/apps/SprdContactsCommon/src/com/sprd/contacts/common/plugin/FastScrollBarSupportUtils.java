package com.sprd.contacts.common.plugin;

import android.app.AddonManager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import com.android.contacts.common.R;

public class FastScrollBarSupportUtils {
    private static FastScrollBarSupportUtils sInstance;

    public static FastScrollBarSupportUtils getInstance() {
        if (sInstance != null) {
            return sInstance;
        }
        sInstance = (FastScrollBarSupportUtils) AddonManager.getDefault()
                .getAddon(R.string.feature_fast_scrollbar_support,
                        FastScrollBarSupportUtils.class);
        Log.d("FastScrollBarSupportUtils", "sInstance: " + sInstance.hashCode());
        return sInstance;
    }

    public FastScrollBarSupportUtils() {
    }

    public boolean hasSupportFastScrollBar() {
        return false;
    }
}
