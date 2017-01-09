package com.sprd.jiemail;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import android.content.Context;
import android.app.Activity;
import com.android.mail.R;

public class SprdJiEmailAddonStub {

    private static final String TAG = "SprdJiEmailAddonStub";

    static SprdJiEmailAddonStub sInstance;

    public SprdJiEmailAddonStub(){
    }

    public static SprdJiEmailAddonStub getInstance(){
        if (sInstance == null) {
            sInstance = (SprdJiEmailAddonStub) instance(R.string.feature_jiemail_cmcc, SprdJiEmailAddonStub.class);
            if (sInstance == null ){
                sInstance = new SprdJiEmailAddonStub();
            }
        }
        android.util.Log.d(TAG, "getInstance. sInstance = "+sInstance);
        return sInstance;
    }

    public void startJiEmailActivity(Activity mActivity) {
        return;
    }

    private static Object instance(int addOnNameResId, Class stubClass) {
        Object obj = null;
        try {
            Class clazz = Class.forName("android.app.AddonManager");

            Method defMethod = clazz.getMethod("getDefault");
            Object addonManager = defMethod.invoke(null);

            Method getMethod = clazz.getMethod("getAddon", new Class[] {int.class, Class.class});
            obj = getMethod.invoke(addonManager, addOnNameResId, stubClass);
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        }
        return obj;
    }
}
