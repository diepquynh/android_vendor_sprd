package android.os;

import android.content.Context;
import android.util.Log;

import java.lang.reflect.Constructor;

public class PMFactory {

    private static final String TAG = "PMFactory";
    private static PMFactory sInstance;

    public PMFactory(){

    }

    public synchronized static PMFactory getInstance(){
        if (sInstance != null) {
            return sInstance;
        }

        Class clazz = null;
        // Load vendor specific factory
        try {
            clazz = Class.forName("android.os.PMFactoryEx");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific PMFactoryEx");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getDeclaredConstructor();
                if (ctor != null) {
                    sInstance = (PMFactory) ctor.newInstance();
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory");
            }
        }

        if (sInstance == null) {
            // Fallback to default factory
            sInstance = new PMFactory();
        }
        return sInstance;
    }

    public AbsPowerManager createExtraPowerManager(Context context){
        return new AbsPowerManager(){};
    }

}
