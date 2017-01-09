package com.android.server.power;

import android.util.Log;
import java.lang.reflect.Constructor;

public class ShutdownAnimationFactory {
    private static final String TAG = "ShutDownAnimationFactory";
    private static ShutdownAnimation sInstance;
    public synchronized static ShutdownAnimation getInstance() {

        Class clazz = null;
        // Load vendor specific factory
        try {
            clazz = Class.forName("com.android.server.power.ShutdownAnimationEx");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific ShutDownAnimationEx");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getDeclaredConstructor();
                if (ctor != null) {
                    sInstance = (ShutdownAnimation) ctor.newInstance();
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ShutDownAnimationEx");
            }
        }

        if (sInstance == null) {
            // Fallback to default factory
            sInstance = new ShutdownAnimation();
        }
        return sInstance;
    }
}
