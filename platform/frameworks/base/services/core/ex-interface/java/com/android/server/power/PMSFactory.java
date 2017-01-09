package com.android.server.power;

import android.content.Context;
import android.hardware.display.DisplayManagerInternelEx;
import android.util.Log;

import com.android.server.display.AbsDisplayPowerController;
import com.android.server.display.AbsDisplayPowerState;

import java.lang.reflect.Constructor;

public class PMSFactory {

    private static PMSFactory sInstance;

    private static final String TAG = "PMSFactory";

    public synchronized static PMSFactory getInstance(){
        if (sInstance != null) {
            return sInstance;
        }

        Class clazz = null;
        // Load vendor specific factory
        try {
            clazz = Class.forName("com.android.server.power.PMSFactoryEx");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific PMSFactoryEx");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getDeclaredConstructor();
                if (ctor != null) {
                    sInstance = (PMSFactory) ctor.newInstance();
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory");
            }
        }

        if (sInstance == null) {
            // Fallback to default factory
            sInstance = new PMSFactory();
        }
        return sInstance;
    }

    public void initExtraPowerManagerService(Context context){

    }

    public AbsPowerManagerServiceUtils createPowerManagerServiceUtils(Context context){
        return new AbsPowerManagerServiceUtils(){};
    }

    public AbsDisplayPowerController createExtraDisplayPowerController(Context context){
        return new AbsDisplayPowerController(){};
    }

    public AbsDisplayPowerState createExtraDisplayPowerState(Context context){
        return new AbsDisplayPowerState(){};
    }

    public DisplayManagerInternelEx createExtraDisplayManagerService(Context context){
        return new DisplayManagerInternelEx(){};
    }
}
