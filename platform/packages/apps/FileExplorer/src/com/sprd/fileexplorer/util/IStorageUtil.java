package com.sprd.fileexplorer.util;

import java.io.File;
import android.content.Intent;

public interface IStorageUtil {
    
    /**
     * You can use this interface to watch the storage states, if the storage
     * changed, I'll tell you <b>which storage path</b> has changed and <b>if
     * available</b> currently using the call back named <b>
     * {@link StorageChangedListener#onStorageChanged(File, boolean)}</b>
     */
    public interface StorageChangedListener {
        // SPRD: Modify for bug509242.
        public void onStorageChanged(String path, boolean available, boolean sdcard);
    }
    
    public void addStorageChangeListener(StorageChangedListener scl);
    public void removeStorageChangeListener(StorageChangedListener scl);
    // SPRD: Modify for bug509242.
    public void notifyStorageChanged(String path, boolean available, boolean sdcard);
    public boolean isNand();
    public File getExternalStorage();
    public File getInternalStorage();
    public File getUSBStorage();
    public File[] getUSBVolume();
    public File getUSBStorage(int num);
    public int getUSBCount();
    public int inWhichUSBStorage(String path);
    public boolean getExternalStorageState();
    public boolean getInternalStorageState();
    public boolean getUSBStorageState();
    public boolean isInExternalStorage(String path);
    public boolean isInInternalStorage(String path);
    public boolean isInUSBStorage(String path);
    public boolean isInUSBVolume(String path);
    public boolean isStorageMounted(File path);
}
