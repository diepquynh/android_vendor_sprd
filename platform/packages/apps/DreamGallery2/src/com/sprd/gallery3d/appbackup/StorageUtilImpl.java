package com.sprd.gallery3d.appbackup;

import java.io.File;
import java.util.ArrayList;
import android.content.SharedPreferences;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.SystemProperties;
import android.util.Log;

/**
 * @hide
 */
class StorageUtilImpl extends StorageUtil {
    private static final Object sLock = new Object();
    private static final int SDK_VERSION_KITKAT = 19;
    static boolean MMC_SUPPORT;
    protected static boolean mIsNAND = false;
    private static final File EXTERNAL_STORAGE_DIRECTORY;
    private static final File INTERNAL_STORAGE_DIRECTORY;
    protected static ArrayList<StorageChangedListener> sListeners = new ArrayList<StorageUtil.StorageChangedListener>();

    static {
        MMC_SUPPORT = SystemProperties.getBoolean("ro.device.support.mmc", false);
        // SPRD: EXTERNAL_STORAGE_DIRECTORY is not return right, so restore failed. @{
        String path = null;
        File externalFile = null;
        externalFile =  EnvironmentEx.getExternalStoragePath();
        if (externalFile == null) {
            Log.d(TAG, "result of getExternalStoragePath() is null, try another ways to get external storage path");
            path = System.getenv(getMainStoragePathKey());
            externalFile = path == null ? Environment.getExternalStorageDirectory() : new File(path);
        }
        EXTERNAL_STORAGE_DIRECTORY = externalFile;
        File internalFile = null;
        internalFile = EnvironmentEx.getInternalStoragePath();
        if (internalFile == null) {
            Log.d(TAG, "result of getInternalStoragePath() is null, try another ways to get internal storage path");
            path = System.getenv(getInternalStoragePathKey());
            path = path == null ? "/mnt/internal/" : path;
            internalFile = new File(path);
        }
        INTERNAL_STORAGE_DIRECTORY = internalFile;
        // @}
    }
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

        //if (Build.VERSION.SDK_INT >= SDK_VERSION_KITKAT) {
        //     Log.d("AppBackupActivity", "version_code = " + Build.VERSION.SDK_INT);
        //     return "SECONDARY_STORAGE";
        // }
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
        //if (Build.VERSION.SDK_INT >= SDK_VERSION_KITKAT) {
        //    return "EXTERNAL_STORAGE";
        //}
        String keyPath = getMainStoragePathKey();
        if (keyPath != null) {
            return keyPath.equals("EXTERNAL_STORAGE") ? "SECONDARY_STORAGE"
                    : "EXTERNAL_STORAGE";
        }
        return "SECONDARY_STORAGE";
    }

    public static boolean getExternalStorageState() {
        try {
            if (EXTERNAL_STORAGE_DIRECTORY.canRead()) {
                // File doubleCheck = new
                // File(EXTERNAL_STORAGE_DIRECTORY.getPath());
                Log.d(TAG,
                        "backup Double Check storage is canread ="
                                + String.valueOf(android.os.EnvironmentEx
                                        .getExternalStoragePath()
                                        .canRead()));
                return true;
            }
        } catch (Exception rex) {
            return false;
        }
        return false;
    }

    public static boolean getInternalStorageState() {
        try {
            if (INTERNAL_STORAGE_DIRECTORY.canRead()) {
                return true;
            }
        } catch (Exception rex) {
            return false;
        }
        return false;
    }

    public static File getExternalStorage() {
        return EXTERNAL_STORAGE_DIRECTORY;
    }

    public static File getInternalStorage() {
        return INTERNAL_STORAGE_DIRECTORY;
    }

    public static boolean isNAND() {
        return mIsNAND;
    }
}

