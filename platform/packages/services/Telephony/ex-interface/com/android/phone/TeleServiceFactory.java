
package com.android.phone;

import java.lang.reflect.Constructor;

import android.content.Context;
import android.preference.PreferenceScreen;
import android.util.Log;

public class TeleServiceFactory {
    private final static String TAG = "TeleServiceFactory";
    private static TeleServiceFactory sInstance;

    public synchronized static TeleServiceFactory getInstance() {
        if (sInstance != null) {
            return sInstance;
        }
        Class clazz = null;
        try {
            clazz = Class.forName("com.android.phone.TeleServiceFactoryEx");
        } catch (Exception e) {
            Log.d(TAG, "can't find specific TeleServiceFactoryEx");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getDeclaredConstructor();
                if (ctor != null) {
                    sInstance = (TeleServiceFactory) ctor.newInstance();
                }
            } catch (Exception e) {
                Log.e(TAG, "can't create specfic ObjectFactory");
            }

        }
        if (sInstance == null) {
            sInstance = new TeleServiceFactory();
        }
        return sInstance;
    }

    public void updateNetworkTypeOptions(Context context, PreferenceScreen prefSet,
            int phoneSubId) {
        // do nothing
    }

    public void disposeNetworkTypeOptions() {
        // do nothing
    }

    public void UpdateEnabledNetworksValueAndSummary(int NetworkMode) {
        // do nothing
    }
}
