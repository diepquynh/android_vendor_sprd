
package com.android.server.policy;

import android.content.Context;
import android.util.Log;
import android.view.WindowManagerPolicy;


import java.lang.reflect.Constructor;

public class PWMFactory {
    private static final String TAG = "PwmFactory";
    private static WindowManagerPolicy sInstance;
    public synchronized static WindowManagerPolicy getInstance() {
        if (sInstance != null) {
            return sInstance;
        }
        Class clazz = null;

        try {
            clazz = Class.forName("com.android.server.policy.SprdPhoneWindowManager");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific SprdPhoneWindowManager");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getConstructor();
                if (ctor != null) {
                    sInstance = (WindowManagerPolicy) ctor.newInstance();
                    Log.d(TAG, "Create SprdPhoneWindowManager");
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory");
            }
        }

        if (sInstance == null) {
            sInstance = new PhoneWindowManager();
        }

        return sInstance;
    }
}
