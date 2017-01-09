package android.app;

import android.content.Context;
import android.util.Log;

import java.lang.reflect.Constructor;

public class PowerGuruFactory {

    private static PowerGuruFactory sInstance;

    private static final String TAG = "PowerGuruFactory";

    public synchronized static PowerGuruFactory getInstance(){
        if (sInstance != null) {
            return sInstance;
        }

        Class clazz = null;
        // Load vendor specific factory
        try {
            clazz = Class.forName("android.app.PowerGuruFactoryEx");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific PowerGuruFactoryEx");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getDeclaredConstructor();
                if (ctor != null) {
                    sInstance = (PowerGuruFactory) ctor.newInstance();
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory");
            }
        }

        if (sInstance == null) {
            // Fallback to default factory
            sInstance = new PowerGuruFactory();
        }
        return sInstance;
    }

    public AbsPowerGuru createExtraPowerGuru(IPowerGuru service, Context context){
        return new AbsPowerGuru(service, context){};
    }

}

