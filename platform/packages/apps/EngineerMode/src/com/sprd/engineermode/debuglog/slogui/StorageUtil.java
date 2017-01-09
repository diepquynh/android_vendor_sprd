/*
- * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sprd.engineermode.debuglog.slogui;

import java.io.File;

import android.content.SharedPreferences;
import android.util.Log;

import com.android.internal.app.IMediaContainerService;

/**
 * This is a interface, you can use this to set <b>
 * {@link StorageChangedListener}</b> to watch storage states, if you want to
 * get ExternalStorage or InternalStorage, using <b>
 * {@link StorageUtil#getExternalStorage()}</b> and <b>
 * {@link StorageUtil#getInternalStorage()}</b>, before you use them, using
 * {@link StorageUtil#getExternalStorageState()} and
 * {@link StorageUtil#getInternalStorageState()} to
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
    interface StorageChangedListener {
        public void onStorageChanged(String path, boolean available);
    }

    protected static final String TAG = "StorageUitl";

    public static void setStorageChangeListener(StorageChangedListener scl) {
        StorageUtilImpl.setStorageChangeListener(scl);
    }

    public static void removeListener(StorageChangedListener scl) {
        StorageUtilImpl.removeListener(scl);
    }
    
    public static void notifyStorageChanged(String path, boolean available) {
        Log.d(TAG,"Sdcard state has change");
        StorageUtilImpl.notifyStorageChanged(path, available);
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
    
    public static boolean isInExternalStorage(String path) {
        return StorageUtilImpl.isInExternalStorage(path);
    }
    
    public static boolean isInInternalStorage(String path) {
        return StorageUtilImpl.isInInternalStorage(path);
    }

    public static long getFreespace(IMediaContainerService imcs,
            File storageLocation) {
        if (storageLocation == null) {
            Log.e(TAG, "storageLocation is null, return 0");
            return 0;
        }
        return StorageUtilImpl.getFreeSpaceJellyBeans(imcs, storageLocation);
    }

    /**
     * Get the total space of storageLocation
     * 
     * @param imcs In IceCreamSandwich or lower, the parameter could be null,
     *            but in JellyBeans it would not be null.
     * @param storageLocation Should be {@code StorageUtil#getExternalStorage()}
     *            or {@code Environment#getDataDirectory()}
     * @return the size(long)
     */
    public static long getTotalSpace(IMediaContainerService imcs,
            File storageLocation) {
        if (storageLocation == null) {
            Log.e(TAG, "storageLocation is null, return 0");
            return 0;
        }

        return StorageUtilImpl.getTotalSpaceJellyBeans(imcs, storageLocation);

    }
}
