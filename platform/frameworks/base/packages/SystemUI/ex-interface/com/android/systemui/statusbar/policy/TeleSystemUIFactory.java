package com.android.systemui.statusbar.policy;

import android.content.Context;
import android.util.Log;
import java.lang.reflect.Constructor;

public class TeleSystemUIFactory {
    private static final String TAG = "TeleSystemUIFactory";
    private static TeleSystemUIFactory sInstance;

    public synchronized static TeleSystemUIFactory getInstance() {
        if (sInstance != null) {
            return sInstance;
        }

        Class clazz = null;
        // Load vendor specific factory
        try {
            clazz = Class.forName("com.android.systemui.statusbar.policy.TeleSystemUIFactoryEx");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific TeleSystemUIFactoryEx");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getDeclaredConstructor();
                if (ctor != null) {
                    sInstance = (TeleSystemUIFactory) ctor.newInstance();
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory");
            }
        }

        if (sInstance == null) {
            // Fallback to default factory
            sInstance = new TeleSystemUIFactory();
        }
        return sInstance;
    }

    public CallbackHandler createCallbackHandler () {
        return new CallbackHandler();
    }

}
