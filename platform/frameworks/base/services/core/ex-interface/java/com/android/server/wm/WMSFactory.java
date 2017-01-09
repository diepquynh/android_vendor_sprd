package com.android.server.wm;

import android.content.Context;
import android.util.Log;
import java.lang.reflect.Constructor;

public class WMSFactory {
    private static final String TAG = "WMSFactory";
    private static WMSLog sLogInstance;
    private static WMSHelper sHelperInstance;

    public synchronized static WMSLog getLogInstance() {
        if (sLogInstance != null) {
            return sLogInstance;
        }
        Class clazz = null;
        try {
            clazz = Class.forName("com.android.server.wm.WMSLogEx");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific WMSLogEx");
        }
        if (clazz != null) {
            try {
                Constructor ctor = clazz.getConstructor();
                if (ctor != null) {
                    sLogInstance = (WMSLog) ctor.newInstance();
                    Log.d(TAG, "Create WMSLogEx");
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory");
            }
        }
        if (sLogInstance == null) {
            sLogInstance = new WMSLog();
        }
        return sLogInstance;
    }

    /*
    * SPRD: Add for bug 605052 avoid pressing the home key or the menu key to exit camera,
    * the screen will light when enter the camera again.{@
    */
    public synchronized static WMSHelper getHelperInstance() {
      if (sHelperInstance != null) {
          return sHelperInstance;
      }
      Class clazz = null;
      try {
        clazz = Class.forName("com.android.server.wm.WMSHelperEx");
      } catch (Throwable t) {
        Log.d(TAG,"Can't find specific WMSHelper");
      }
      if (clazz != null) {
         try {
            Constructor ctor = clazz.getConstructor();
            if (ctor != null) {
               sHelperInstance = (WMSHelper)ctor.newInstance();
               Log.d(TAG,"Create WMSHelperEx");
            }
         } catch (Throwable t) {
            Log.e(TAG,"Cant't create specific ObjectFactory");
         }
      }
      if (sHelperInstance == null) {
          sHelperInstance = new WMSHelper();
      }
      return sHelperInstance;
    }
    /* @{ */
}
