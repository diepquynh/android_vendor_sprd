package com.sprd.contacts.plugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import com.android.contacts.R;
import android.view.Menu;
import com.android.contacts.common.model.Contact;

public class DefaultContactUtils {

    private static DefaultContactUtils sInstance;

    public static DefaultContactUtils getInstance(){
        if (sInstance != null) {
            return sInstance;
        }
        sInstance = (DefaultContactUtils)AddonManager.getDefault().getAddon(R.string.feature_default_contact,
                DefaultContactUtils.class);
        Log.d("DefaultContactUtils", "sInstance: " + sInstance.hashCode());
        return sInstance;
    }


}
