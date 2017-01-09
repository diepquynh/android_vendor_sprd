package android.os;

import android.content.Context;
import android.util.Log;

import java.lang.reflect.Constructor;

public class HprofFactory {
    private static final String TAG = "HprofFactory";
    private static HprofDebug sInstance;
    public synchronized static HprofDebug getInstance() {
        if (sInstance != null) {
            return sInstance;
        }
        Class clazz = null;

        try {
            clazz = Class.forName("android.os.HprofDebugEx");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific HprofDebugEx");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getConstructor();
                if (ctor != null) {
                    sInstance = (HprofDebug) ctor.newInstance();
                    Log.d(TAG, "Create HprofDebugEx");
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory");
            }
        }

        if (sInstance == null) {
            sInstance = new HprofDebug();
        }

        return sInstance;
    }
}