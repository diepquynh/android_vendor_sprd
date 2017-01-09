/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
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
import java.util.ArrayList;

import android.content.SharedPreferences;
import android.os.RemoteException;
import android.os.StatFs;
import android.os.SystemProperties;
import android.util.Log;
import java.util.Collections;
import android.os.Environment;
import android.text.TextUtils;
import com.android.internal.app.IMediaContainerService;
import com.sprd.engineermode.utils.EngineerModeNative;

/**
 * @hide
 */
class StorageUtilImpl extends StorageUtil {
    private static final Object sLock = new Object();
    static boolean MMC_SUPPORT = "1".equals(android.os.SystemProperties
            .get("ro.device.support.mmc"));
    private static final File EXTERNAL_STORAGE_DIRECTORY = getDirectory(
            getMainStoragePathKey(), android.os.Environment
                    .getExternalStorageDirectory().getPath());
    private static final String ENV_PHYSICAL_STORAGE = "PHYSICAL_STORAGE";
    private static final String ENV_PHYSICAL1_STORAGE = "PHYSICAL1_STORAGE";
    private static final String physicalPathString = getPathString(ENV_PHYSICAL_STORAGE, "/storage/sdcard0");
    private static final String physical1PathString = getPathString(ENV_PHYSICAL1_STORAGE, "/storage/sdcard1");
    protected static final boolean SPECIAL_EMMC_EXTERNAL = SystemProperties.getInt("persist.storage.type" ,-1) == 1;

    private static final File SECONDRARY_STORAGE_DIRECTORY = getDirectory(
            getInternalStoragePathKey(), "/mnt/internal/");
    protected static ArrayList<StorageChangedListener> sListeners = new ArrayList<StorageUtil.StorageChangedListener>();
    private static ArrayList<StorageChangedListener> reverseListeners;

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

    public static void notifyStorageChanged(String path, boolean available) {
        Log.d(TAG,"Sdcard state has change,SlogUI will listen this change");
        synchronized (sLock) {
            reverseListeners = (ArrayList<StorageChangedListener>)sListeners.clone();
            if (reverseListeners != null || !reverseListeners.isEmpty()) {
                Collections.reverse(reverseListeners);
                for (StorageChangedListener l : reverseListeners) {
                    l.onStorageChanged(path, available);
                }
            }
        }

    }

    private static String getMainStoragePathKey() {
        // FIXME: Continue highlight at this one on 12b_pxx branch, there is
        // no SECONDARY_STORAGE_TYPE
        // Finally, Double T-flash solution has been changed, remove this one.
        //if (android.os.Build.VERSION.SDK_INT >= 19) {
        //    return "SECONDARY_STORAGE";
        //}
        try {
            // add a protection to fix if no SECONDARY_STORAGE_TYPE
            if ((null == System.getenv("SECOND_STORAGE_TYPE") || ""
                    .equals(System.getenv("SECOND_STORAGE_TYPE").trim()))
                    && MMC_SUPPORT) {
                // TODO ADD YOUR OWN TAG OR REMOVE THIS ONE.
                switch (SystemProperties.getInt("persist.storage.type", -1)) {
                    case 0:
                        return "EXTERNAL_STORAGE";
                    case 1:
                        return "EXTERNAL_STORAGE";
                    case 2:
                        return "SECONDARY_STORAGE";
                    default:
                        if (MMC_SUPPORT) {
                            return "SECONDARY_STORAGE";
                        }
                }

                return "EXTERNAL_STORAGE";
            }
            switch (Integer.parseInt(System.getenv("SECOND_STORAGE_TYPE"))) {
                case 0:
                    return "EXTERNAL_STORAGE";
                case 1:
                    return "EXTERNAL_STORAGE";
                case 2:
                    return "SECONDARY_STORAGE";
                default:
                    // TODO ADD YOUR OWN TAG OR REMOVE THIS ONE.
                    // USEFUL LOG
                    Log.e("",
                            "Please check \"SECOND_STORAGE_TYPE\" "
                                    + "\'S value after parse to int in System.getenv for framework");
                    if (MMC_SUPPORT) {
                        return "SECONDARY_SOTRAGE";
                    }
                    return "EXTERNAL_STORAGE";
            }
        } catch (Exception parseError) {
            Log.e("", "Parsing SECOND_STORAGE_TYPE crashed.\n" + parseError);
            // The new optimize of dual TF Card
            switch (SystemProperties.getInt("persist.storage.type", -1)) {
                case 0:
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
        // if (android.os.Build.VERSION.SDK_INT >= 19) {
        //     return "EXTERNAL_STORAGE";
        // }
        String keyPath = getMainStoragePathKey();
        if (keyPath != null) {
            return keyPath.equals("EXTERNAL_STORAGE") ? "SECONDARY_STORAGE"
                    : "EXTERNAL_STORAGE";
        }
        return "SECONDARY_STORAGE";
    }

    public static boolean isStorageMounted(File path) {
        String state = Environment.getExternalStorageState(path);
        return "mounted".equals(state);
    }

    public static File getExternalStorage() {
        File externalPath = null;
        if (internalIsEmulated()) {
            externalPath = getStoragePathFormKey("vold.sdcard0.path");
        } else {
            externalPath = getStoragePathFormKey("vold.sdcard1.path");
        }
        if (externalPath == null) {
            externalPath = new File(EngineerModeNative.native_get_sd_file_path());
        }
        return externalPath;
    }

    static String getPathString(String variableName, String defaultPath) {
                String path = System.getenv(variableName);
                return path == null ? defaultPath : path;
            }

    public static File getExternalStorageLinkPath() {
        if (internalIsEmulated()) {
            return new File(physicalPathString);
        } else {
            return new File(physical1PathString);
        }
    }

    private static File getStoragePathFormKey(String key) {
        String path = SystemProperties.get(key);
        if (TextUtils.isEmpty(path)) {
            return null;
        } else {
            return new File(path);
        }
    }

    public static boolean internalIsEmulated() {
        int internalType = SystemProperties.getInt("sys.internal.emulated", 1);
        Log.d(TAG, "internalIsEmulated : " + (internalType == 1));
        return internalType == 1;
    }

    public static boolean getExternalStorageState() {
        boolean isTrue = isStorageMounted(getExternalStorage());
        if (isTrue)
            return true;
        return (EngineerModeNative.native_check_sdcard_mounted_default() == 1);
    }

    public static boolean getInternalStorageState() {
        try {
            android.os.storage.IMountService ims =
                    android.os.storage.IMountService.Stub.asInterface(
                            android.os.ServiceManager
                                    .getService("mount"));
            return "mounted".equals(ims.getVolumeState(SECONDRARY_STORAGE_DIRECTORY
                    .toString()));
        } catch (Exception rex) {
            return false;
        }
    }

    private static File getDirectory(String variableName, String defaultPath) {
        String path = System.getenv(variableName);
        return path == null ? new File(defaultPath) : new File(path);
    }

    public static File getInternalStorage() {
        return SECONDRARY_STORAGE_DIRECTORY;
    }

    public static boolean isInExternalStorage(String path) {
        if(path == null)
            return false;
        return path.startsWith(EXTERNAL_STORAGE_DIRECTORY.getAbsolutePath());
    }

    public static boolean isInInternalStorage(String path) {
        if(path == null)
            return false;
        return path.startsWith(SECONDRARY_STORAGE_DIRECTORY.getAbsolutePath());
    }

    public static long getFreeSpaceIceCreamSandwich(File location) {
        if (location == null) {
            return 0;
        }
        final StatFs stat = new StatFs(location.getAbsolutePath());
        final long blockSize = stat.getBlockSize();
        final long availableBlocks = stat.getAvailableBlocks();

        return availableBlocks * blockSize;
    }

    public static long getTotalSpaceIceCreamSandwich(File location) {
        if (location == null) {
            Log.e(TAG, "The location is null, return 0");
            return 0;
        }
        final StatFs stat = new StatFs(location.getAbsolutePath());
        final long blockSize = stat.getBlockSize();
        final long totalBlocks = stat.getBlockCount();

        return totalBlocks * blockSize;
    }

    public static long getFreeSpaceJellyBeans(IMediaContainerService imcs, File location) {
        if (imcs == null || location == null) {
            Log.e(TAG, "IMediaContainerService or Location is null, return 0");
            return 0;
        }
        try {
            final long[] stats = imcs.getFileSystemStats(location.getAbsolutePath());
            return stats[1];
        } catch (RemoteException e) {
            Log.w(TAG, "RemoteException, Container service may die. " + e.getMessage(), e);
            return 0;
        } catch (IllegalStateException e) {
            if (SPECIAL_EMMC_EXTERNAL) {
                if (location.equals(android.os.Environment.getDataDirectory())) {
                    return 0;
                }
                // All right, there's a problem yet, workaround , we're tired.
                return SlogAction.getFreeSpace(imcs, false);
            } else {
                Log.d(TAG,"sd is illegal" + e.toString());
                return 0;
            }
        }
        /*
          * XXX
          * Won't try to catch IllegalStateException here, it caused by putting
          * wrong path into MediaContainerService, we should find the reason
          * about this fatal mistake. If catched, hidding "FATAL" issue it
          * would be.
          * catch (IllegalStateException e) {
          * Log.w(TAG, "Problem in container service", e);
          * return 0;
        } */

    }

    public static long getTotalSpaceJellyBeans(IMediaContainerService imcs, File location) {
        if (imcs == null || location == null) {
            Log.e(TAG, "IMediaContainerService or Location is null, return 0");
            return 0;
        }
        try {
            final long[] stats = imcs.getFileSystemStats(location.getAbsolutePath());
            return stats[0];
        } catch (RemoteException e) {
            Log.w(TAG, "Problem in container service", e);
            return 0;
            /* bug288331 start*/
        } catch (IllegalStateException e) {
            if (SPECIAL_EMMC_EXTERNAL) {
                if (location.equals(android.os.Environment.getDataDirectory())) {
                    return 0;
                }
                // All right, there's a problem yet, workaround , we're tired.
                return SlogAction.getTotalSpace(imcs, false);
            } else {
                Log.d(TAG,"sd is illegal" + e.toString());
                return 0;
            }
            /* bug288331 end*/
        }

    }
}
