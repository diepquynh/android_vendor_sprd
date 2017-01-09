package android.view;

import android.content.Context;
import android.util.Log;


import java.lang.reflect.Constructor;

public class VRIFactory {
    private static final String TAG = "VRIFactory";
    public static ViewRootImpl getNewObject(Context context, Display display) {
        ViewRootImpl sInstance = null;

        Class clazz = null;

        try {
            clazz = Class.forName("android.view.SprdViewRootImpl");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific SprdViewRootImpl");
        }

        if (clazz != null) {
            try {
                Class[] cArg = new Class[2];
                cArg[0] = Context.class;
                cArg[1] = Display.class;
                Constructor ctor = clazz.getDeclaredConstructor(cArg);
                if (ctor != null) {
                    sInstance = (ViewRootImpl) ctor.newInstance((Object)context, (Object)display);
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory" + t);
            }
        }

        if (sInstance == null) {
            sInstance = new ViewRootImpl(context, display);
        }

        return sInstance;
    }
}

