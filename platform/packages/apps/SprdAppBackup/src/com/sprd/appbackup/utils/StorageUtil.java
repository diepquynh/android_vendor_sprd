package com.sprd.appbackup.utils;

import java.io.File;
import android.os.Environment;
import android.os.StatFs;

/**
 * This is a interface, you can use this to set <b>
 * {@link StorageChangedListener}</b> to watch storage states, if you want to
 * get ExternalStorage or InternalStorage, using <b>
 * {@link StorageUtil#getExternalStorage()}</b> and <b>
 * {@link StorageUtil#getInternalStorage()}</b>, before you use them, using
 * {@link StorageUtil#getExternalStorageState()} and
 * {@link StorageUtil#getInternalStorageState()} to
 *
 */
public class StorageUtil {
    public static final String EXTERNAL_STORAGE_STATE_KEY = "external_storage_state";
    public static final String INTERNAL_STORAGE_STATE_KEY = "internal_storage_state";
    /**
     * You can use this interface to watch the storage states, if the storage
     * changed, I'll tell you <b>which storage path</b> has changed and <b>if
     * available</b> currently using the call back named <b>
     * {@link StorageChangedListener#onStorageChanged(File, boolean)}</b>
     */
    public interface StorageChangedListener {
        public void onStorageChanged(File path, boolean available);
    }

    protected static final String TAG = "StorageUitl";

    public static void setStorageChangeListener(StorageChangedListener scl) {
        StorageUtilImpl.setStorageChangeListener(scl);
    }

    public static void removeListener(StorageChangedListener scl) {
        StorageUtilImpl.removeListener(scl);
    }

    public static File getExternalStorage() {
        return StorageUtilImpl.getExternalStorage();
    }

    public static File getInternalStorage() {
        return StorageUtilImpl.getInternalStorage();
    }

    public static boolean getExternalStorageState() {
        return StorageUtilImpl.getExternalStorageState();
    }

    public static boolean getInternalStorageState() {
        return StorageUtilImpl.getInternalStorageState();
    }

    /**
    * get internal availale space
    * @return
    */
    static public long getAvailableInternalMemorySize() {
        StatFs stat = new StatFs(getInternalStorage().getPath());
        long blockSize = stat.getBlockSize();
        long availableBlocks = stat.getAvailableBlocks();
        return availableBlocks * blockSize;
    }

    /**
    * get external availale space
    * @return
    */
    static public long getAvailableExternalMemorySize() {
        StatFs stat = new StatFs(getExternalStorage().getPath());
        long blockSize = stat.getBlockSize();
        long availableBlocks = stat.getAvailableBlocks();
        return availableBlocks * blockSize;
    }
}
