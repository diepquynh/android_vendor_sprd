package com.sprd.appbackup.utils;

import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;

import android.content.SharedPreferences;
import android.util.Log;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.SystemProperties;

/**
 * @hide
 */
class StorageUtilImpl extends StorageUtil {
    private static final Object sLock = new Object();
    private static final int SDK_VERSION_KITKAT = 19;
    /* SPRD: the method of getStoragePath changed. @{ */
    static boolean MMC_SUPPORT;
    protected static boolean mIsNAND = false;
    private static final File EXTERNAL_STORAGE_DIRECTORY;

    private static final File SECONDRARY_STORAGE_DIRECTORY;
    protected static ArrayList<StorageChangedListener> sListeners = new ArrayList<StorageUtil.StorageChangedListener>();

    static {
        MMC_SUPPORT = SystemProperties.getBoolean("ro.device.support.mmc", false);
        String path = System.getenv(getMainStoragePathKey());
        EXTERNAL_STORAGE_DIRECTORY = path == null ? Environment.getExternalStorageDirectory() : new File(path);
        File internalFile = null;
        try {
            Method method = Environment.class.getMethod("getInternalStoragePath");
            Object receiveObject = method.invoke(null);
            if (receiveObject != null && receiveObject instanceof File) {
                internalFile = (File) receiveObject;
            }
        } catch (Exception e) {
            Log.d(TAG, "getMethod failed call getInternalStoragePath method");
        }
        if(internalFile == null) {
            path = System.getenv(getInternalStoragePathKey());
            path = path == null ? "/mnt/internal/" : path;
            internalFile = new File(path);
        }
        SECONDRARY_STORAGE_DIRECTORY = internalFile;
    }
    /* @} */
    public static void setStorageChangeListener(StorageChangedListener scl) {
        synchronized (sLock) {
            sListeners.add(scl);
        }

    }

    public static void removeListener(StorageChangedListener scl) {
        synchronized (sLock) {
            sListeners.remove(scl);
        }

    }

    public static void notifyStorageChanged(File path, boolean available, SharedPreferences settings) {
        synchronized (sLock) {

            if (settings != null) {
                settings.edit().putBoolean(path.getAbsolutePath(),available).commit();
            }
            Log.d("AppBackupActivity", "notifyStorageChanged path = "+path+", available = "+available);
            if (sListeners != null && !sListeners.isEmpty()) {
                Log.d("AppBackupActivity", "sListeners != null && !sListeners.isEmpty()");
                for (StorageChangedListener l : sListeners) {
                    Log.d("AppBackupActivity", "l.onStorageChanged()");
                    l.onStorageChanged(path, available);
                }
            }
        }

    }

    private static String getMainStoragePathKey() {
        // FIXME: Continue highlight at this one on 12b_pxx branch, there is
        // no SECONDARY_STORAGE_TYPE

//       if (Build.VERSION.SDK_INT >= SDK_VERSION_KITKAT) {
//            Log.d("AppBackupActivity", "version_code = " + Build.VERSION.SDK_INT);
//            return "SECONDARY_STORAGE";
//        }
        try {
            // add a protection to fix if no SECONDARY_STORAGE_TYPE
            if ((null == System.getenv("SECOND_STORAGE_TYPE") || ""
                    .equals(System.getenv("SECOND_STORAGE_TYPE").trim()))
                    && MMC_SUPPORT) {
                // TODO ADD YOUR OWN TAG OR REMOVE THIS ONE.
                Log.d(TAG, "No SECOND_STORAGE_TYPE and support emmc");
                return "SECONDARY_STORAGE";
            }
            switch (Integer.parseInt(System.getenv("SECOND_STORAGE_TYPE"))) {
                case 0:
                    mIsNAND = true;
                    return "EXTERNAL_STORAGE";
                case 1:
                    return "EXTERNAL_STORAGE";
                case 2:
                    return "SECONDARY_STORAGE";
                default:
                    // TODO ADD YOUR OWN TAG OR REMOVE THIS ONE.
                    // USEFUL LOG
                    Log.e(TAG, "Please check \"SECOND_STORAGE_TYPE\" "
                               + "\'S value after parse to int in System.getenv for framework");
                    if (MMC_SUPPORT) {
                        return "SECONDARY_SOTRAGE";
                    }
                    return "EXTERNAL_STORAGE";
            }
        } catch (Exception parseError) {
            Log.e(TAG, "Parsing SECOND_STORAGE_TYPE crashed.\n" + parseError);
            switch (SystemProperties.getInt("persist.storage.type", -1)) {
                case 0:
                    mIsNAND = true;
                    return "EXTERNAL_STORAGE";
                case 1:
                    return "EXTERNAL_STORAGE";
                case 2:
                    return "SECONDARY_STORAGE";
                default:
                    if (MMC_SUPPORT) {
                        return "SECONDARY_SOTRAGE";
                    }
            }

            return "EXTERNAL_STORAGE";
        }

    }

    private static String getInternalStoragePathKey() {
//        if (Build.VERSION.SDK_INT >= SDK_VERSION_KITKAT) {
//            return "EXTERNAL_STORAGE";
//        }
        String keyPath = getMainStoragePathKey();
        if (keyPath != null) {
            return keyPath.equals("EXTERNAL_STORAGE") ? "SECONDARY_STORAGE"
                    : "EXTERNAL_STORAGE";
        }
        return "SECONDARY_STORAGE";
    }

    public static boolean getExternalStorageState() {
        try {
            /* SPRD: 488328 Modify for showing the Internal Storage and External Storage. @{ */
            //android.os.storage.IMountService ims = android.os.storage.IMountService.Stub.asInterface(android.os.ServiceManager.getService("mount"));
            //return "mounted".equals(ims.getVolumeState(EXTERNAL_STORAGE_DIRECTORY.toString()));
            return isStorageMounted(getExternalStorage());
            /* @} */
        } catch (Exception rex) {
            return false;
        }

    }

    public static boolean getInternalStorageState() {
        try {
            /* SPRD: 488328 Modify for showing the Internal Storage and External Storage. @{ */
            //android.os.storage.IMountService ims = android.os.storage.IMountService.Stub.asInterface(android.os.ServiceManager.getService("mount"));
            //return "mounted".equals(ims.getVolumeState(SECONDRARY_STORAGE_DIRECTORY.toString()));
            return isStorageMounted(getInternalStorage());
            /* @} */
        } catch (Exception rex) {
            return false;
        }
    }

    private static File getDirectory(String variableName, String defaultPath) {
        String path = System.getenv(variableName);
        return path == null ? new File(defaultPath) : new File(path);
    }

    public static File getExternalStorage() {
        /* SPRD: 476980 modify external storage path @{ */
        //return EXTERNAL_STORAGE_DIRECTORY;
        return EnvironmentEx.getExternalStoragePath();
        /* @} */
    }

    public static File getInternalStorage() {
        /* SPRD: 476980 modify internal storage path @{ */
        //return SECONDRARY_STORAGE_DIRECTORY;
        return EnvironmentEx.getInternalStoragePath();
        /* @} */
    }

    public static boolean isNAND() {
        return mIsNAND;
    }

    /* SPRD: 488328 Add for showing the Internal and External Storage @{ */
    public static boolean isStorageMounted(File path) {
        return Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(path));
    }
    /* @} */
}
