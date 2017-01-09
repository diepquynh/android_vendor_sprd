package com.sprd.contacts.plugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.contacts.R;
import android.view.Menu;
import com.android.contacts.common.model.Contact;

public class BlacklistUtils {
    static BlacklistUtils sInstance;
    public static BlacklistUtils getInstance(){
        if (sInstance != null) return sInstance;
        sInstance = (BlacklistUtils)AddonManager.getDefault().getAddon(R.string.feature_blacklist,BlacklistUtils.class);
        Log.d("blacklist","sInstance: " + sInstance.hashCode());
        return sInstance;
    }
    public BlacklistUtils() {
    }

    public void addBlackItem(Context context, Menu menu, Contact mContactData) {
    }

    public void blackOrNot(Context context, Menu menu, Contact mContactData) {
    }
}
