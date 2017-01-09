/** Create by Spreadst */

package com.spreadst.lockscreen;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.util.Log;
import android.widget.AbsLockScreen;
import android.widget.AbsLockScreenProxy;
import android.widget.ILockScreenListener;
import dalvik.system.PathClassLoader;

public class LockscreenPoxy extends AbsLockScreenProxy {

    private static final String TAG = "LockscreenPoxy";

    private static LockClassLoader mLockClassLoader = null;

    private static ArrayList<LockClassLoader> mLockClassLoaderList = new ArrayList<LockClassLoader>();

    private Context mContext;

    private ILockScreenListener mLockScreenListener;

    private ExpandLockscreenInfo mElsInfo;

    public LockscreenPoxy(Context context, ILockScreenListener listener)
            throws NameNotFoundException {
        super(context, listener);
        Log.d(TAG, " LockscreenPoxy()... ");
        mContext = context;
        mLockScreenListener = listener;
        Resources res = null;
        res = context.getPackageManager().getResourcesForApplication(
                Constants.CURRENT_PACKAGE_NAME);
        int elsId = Tools.getLockViewID(context);
        if (elsId > 0) {
            mElsInfo = ElsModel.getInstance().getCurrentElsInfoById(elsId, res,
                    true);
            if (mElsInfo != null) {
                Log.d(TAG, "mElsInfo.getApk_name()" + mElsInfo.getApk_name());
                mLockClassLoader = getLockClassLoader(mElsInfo.getApk_name());
            } else {
                Log.d(TAG, "mElsInfo is NULL!");
            }
        }
    }

    @Override
    public AbsLockScreen getLockViewOfCustom() {
        Log.d(TAG, " getLockViewOfCustom... ");
        AbsLockScreen ls = null;
        int elsId = Tools.getLockViewID(mContext);
        Log.d(TAG, " lockId=" + elsId);

        switch (elsId) {
            case -1:
                ls = null;
                break;
            case 0:
                ls = null;
                break;
            default:

                Log.d(TAG, "mElsInfo.getApk_name()" + mElsInfo.getApk_name());
                Log.d(TAG, "mElsInfo.getPackage_name()" + mElsInfo.getPackage_name());
                ls = loadDynamicScreen(mContext, mLockScreenListener,
                        mElsInfo.getPackage_name() + "." + mElsInfo.getClass_name());

                break;
        }

        return ls;
    }

    private LockClassLoader getLockClassLoader(String apk_name) {
        LockClassLoader lockClassLoader = null;
        for (LockClassLoader tmpLoader : mLockClassLoaderList) {
            Log.d(TAG, " mLockClassLoaderList: " + tmpLoader.toString());
            Log.d(TAG, " mLockClassLoaderList: " + apk_name);
            if (tmpLoader.toString().contains(
                    Constants.LOCKSCREEN_APK_PATH + apk_name)) {
                lockClassLoader = tmpLoader;
                break;
            }
        }
        if (lockClassLoader == null) {
            Log.d(TAG, " create new lockClassLoader.. ");
            lockClassLoader = new LockClassLoader(apk_name);
            mLockClassLoaderList.add(lockClassLoader);
        }
        return lockClassLoader;
    }

    private static class LockClassLoader extends PathClassLoader {

        public LockClassLoader(String apk_name) {
            super(Constants.LOCKSCREEN_APK_PATH + apk_name, ClassLoader
                    .getSystemClassLoader());
            Log.d(TAG, "path =" + Constants.LOCKSCREEN_APK_PATH + apk_name);
        }
    }

    private AbsLockScreen loadDynamicScreen(Context context,
            ILockScreenListener listener, String className) {
        try {
            Class c = null;
            c = Class.forName(className, true, mLockClassLoader);
            Constructor<AbsLockScreen> constructor = c.getConstructor(
                    Context.class, ILockScreenListener.class);
            return (AbsLockScreen) constructor.newInstance(context, listener);
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
            Log.d(TAG, e.toString());
        } catch (NoSuchMethodException e) {
            e.printStackTrace();
            Log.d(TAG, e.toString());
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
            Log.d(TAG, e.toString());
        } catch (InstantiationException e) {
            e.printStackTrace();
            Log.d(TAG, e.toString());
        } catch (IllegalAccessException e) {
            e.printStackTrace();
            Log.d(TAG, e.toString());
        } catch (InvocationTargetException e) {
            e.printStackTrace();
            Log.d(TAG, e.toString());
        }
        return null;
    }

}
