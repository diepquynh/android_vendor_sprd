package com.android.server.power;

import android.content.Context;
import android.util.Log;

import java.lang.reflect.Constructor;

public class PowerGuruServiceFactory {

    private static PowerGuruServiceFactory sInstance;

    private static final String TAG = "PowerGuruServiceFactory";

    public synchronized static PowerGuruServiceFactory getInstance(){
        if (sInstance != null) {
            return sInstance;
        }

        Class clazz = null;
        // Load vendor specific factory
        try {
            clazz = Class.forName("com.android.server.power.PowerGuruServiceFactoryEx");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific PowerGuruServiceFactoryEx");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getDeclaredConstructor();
                if (ctor != null) {
                    sInstance = (PowerGuruServiceFactory) ctor.newInstance();
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory");
            }
        }

        if (sInstance == null) {
            // Fallback to default factory
            sInstance = new PowerGuruServiceFactory();
        }
        return sInstance;
    }

    public AbsPowerGuruService createPowerGuruService(Context context){
        return new AbsPowerGuruService(context){};
    }
}
