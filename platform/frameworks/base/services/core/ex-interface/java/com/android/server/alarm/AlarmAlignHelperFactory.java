package com.android.server;

import android.content.Context;
import android.util.Log;

import java.lang.reflect.Constructor;

public class AlarmAlignHelperFactory {

    private static AlarmAlignHelper sInstance;

    private static final String TAG = "AlarmAlignHelperFactory";

    public synchronized static AlarmAlignHelper getInstance(Context context){
        if (sInstance != null) {
            return sInstance;
        }

        Class clazz = null;
        // Load vendor specific factory
        try {
            clazz = Class.forName("com.android.server.AlarmAlignHelperEx");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific AlarmAlignHelperEx");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getConstructor(Context.class);
                if (ctor != null) {
                    sInstance = (AlarmAlignHelper) ctor.newInstance(context);
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory");
            }
        }

        if (sInstance == null) {
            // Fallback to default factory
            sInstance = new AlarmAlignHelper(context);
        }
        return sInstance;
    }

}
