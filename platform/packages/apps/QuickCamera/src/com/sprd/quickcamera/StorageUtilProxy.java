
package com.sprd.quickcamera;

import java.io.File;
import java.lang.reflect.Method;
import java.util.List;

import android.os.Build;
import android.os.storage.VolumeInfo;
import android.util.Log;

public class StorageUtilProxy {

    public static final String TAG = "StorageUtilProxy";
    private static Class<?> environmentExClazz = null;
    private static Class<?> environmentClazz = null;

    private static boolean OLD_SDK_VERSION = Build.VERSION.SDK_INT <= Build.VERSION_CODES.M ? true : false;
    static {
        try {
            environmentClazz = Class.forName("android.os.Environment");
            environmentExClazz = Class.forName("android.os.EnvironmentEx");
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

    }

    public static File getInternalStoragePath() {
        Method method;
        File file = null;
        try {
            if (OLD_SDK_VERSION) {
                method = environmentClazz.getMethod("getInternalStoragePath");
                file = (File) method.invoke(environmentClazz);
            } else {
                method = environmentExClazz.getMethod("getInternalStoragePath");
                file = (File) method.invoke(environmentExClazz);
            }
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

        return file;

    }

    public static File getExternalStoragePath() {
        Method method;
        File file = null;
        try {
            if (OLD_SDK_VERSION) {
                method = environmentClazz.getMethod("getExternalStoragePath");
                file = (File) method.invoke(environmentClazz);
            } else {
                method = environmentExClazz.getMethod("getExternalStoragePath");
                file = (File) method.invoke(environmentExClazz);
            }
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

        return file;
    }

    public static String getInternalStoragePathState() {
        Method method;
        String state = "";
        try {
            if (OLD_SDK_VERSION) {
                Log.e(TAG, "use environmentClazz");
                method = environmentClazz.getMethod("getInternalStoragePathState");
                state = (String) method.invoke(environmentClazz);
            } else {
                Log.e(TAG, "use environmentExClazz");
                method = environmentExClazz.getMethod("getInternalStoragePathState");
                state = (String) method.invoke(environmentExClazz);
            }
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

        return state;
    }

    public static String getExternalStoragePathState() {
        Method method;
        String state = "";
        try {
            if (OLD_SDK_VERSION) {
                method = environmentClazz.getMethod("getExternalStoragePathState");
                state = (String) method.invoke(environmentClazz);
            } else {
                method = environmentExClazz.getMethod("getExternalStoragePathState");
                state = (String) method.invoke(environmentExClazz);
            }
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

        return state;
    }
    
    public static String getUsbdiskVolumeState(File path) {
        Method method;
        String state = "";
        try {
            if (OLD_SDK_VERSION) {
                method = environmentClazz.getMethod("getUsbdiskVolumeState", File.class);
                state = (String) method.invoke(environmentClazz, path);
            } else {
                method = environmentExClazz.getMethod("getUsbdiskVolumeState", File.class);
                state = (String) method.invoke(environmentExClazz, path);
            }
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

        return state;
    }
    
    public static List<VolumeInfo> getUsbdiskVolumes() {
        Method method;
        List<VolumeInfo> info = null;
        try {
            if (OLD_SDK_VERSION) {
                method = environmentClazz.getMethod("getUsbdiskVolumes");
                info = (List<VolumeInfo>) method.invoke(environmentClazz);
            } else {
                method = environmentExClazz.getMethod("getUsbdiskVolumes");
                info = (List<VolumeInfo>) method.invoke(environmentExClazz);
            }
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

        return info;
    }
}
