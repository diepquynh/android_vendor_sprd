package com.android.internal.telephony;

import java.lang.reflect.Constructor;

import android.content.Context;
import android.os.Parcel;
import android.service.carrier.CarrierIdentifier;
import android.telephony.AbsCarrierConfigManager;
import android.telephony.AbsTelephonyManager;
import android.util.Log;

public class FrameworkFactory {

    private static final String TAG = "FrameworkFactory";
    private static FrameworkFactory sInstance;

    public synchronized static FrameworkFactory getInstance() {
        if (sInstance != null) {
            return sInstance;
        }

        Class clazz = null;
        // Load vendor specific factory
        try {
            clazz = Class.forName("com.android.internal.telephony.FrameworkFactoryEx");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific FrameworkFactoryEx");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getDeclaredConstructor();
                if (ctor != null) {
                    sInstance = (FrameworkFactory) ctor.newInstance();
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific FrameworkFactoryEx");
            }
        }

        if (sInstance == null) {
            // Fallback to default factory
            sInstance = new FrameworkFactory();
        }
        return sInstance;
    }

    public AbsCarrierConfigManager createExtraCarrierConfigManager() {
        return new AbsCarrierConfigManager();
    }

    public AbsTelephonyManager createExtraTelephonyManager(Context context) {
        return new AbsTelephonyManager();
    }
}


