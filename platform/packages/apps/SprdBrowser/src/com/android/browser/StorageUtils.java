/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.browser;

import android.content.Context;
import android.util.Log;
import android.os.Environment;
import java.io.File;
import com.sprd.common.Common;

public class StorageUtils {

    private static final String USB_DISK_PATH = "/storage/usbdisk";
    public static final int STORAGE_PRIMARY_EXTERNAL = 1;
    public static final int STORAGE_PRIMARY_INTERNAL = 2;

    //android original API
    public static File getExternalStorageDirectory(){
        return Environment.getExternalStorageDirectory();
    }

    public static String getExternalStorageState() {
        return Environment.getExternalStorageState();
    }
    //SPRD add API
    public static File getInternalStoragePath(){
        return Common.getStoragePlugIn().getInternalStoragePath();
    }

    public static String getInternalStoragePathState(){
        return Common.getStoragePlugIn().getInternalStoragePathState();
    }

    public static File getExternalStoragePath(){
        return Common.getStoragePlugIn().getExternalStoragePath();
    }

    public static String getExternalStoragePathState(){
        return Common.getStoragePlugIn().getExternalStoragePathState();
    }

    public static File getUsbdiskStoragePath(){
        File[] files = Common.getStoragePlugIn().getUsbdiskVolumePaths();
        if (files != null && files.length > 0) {
            if (files[0] != null)
                return files[0];
        }
        return new File(USB_DISK_PATH);
    }

    public static String getUsbdiskStoragePathState(File path){
        return Common.getStoragePlugIn().getUsbdiskVolumeState(path);
    }

    public static String getDefaultStoragePath(){
        return getInternalStoragePath() + "/Download";
    }

    public static boolean isExternalStorageMounted() {
        return Environment.MEDIA_MOUNTED.equals(getExternalStoragePathState());
    }

    public static boolean isInternalStorageMounted() {
        return Environment.MEDIA_MOUNTED.equals(getInternalStoragePathState());
    }

    public static boolean isUsbdiskStorageMounted() {
        return Environment.MEDIA_MOUNTED.equals(getUsbdiskStoragePathState(getUsbdiskStoragePath()));
    }

    public static boolean checkStoragePathAvailable(String path){
        if (path != null && getExternalStoragePath() != null && path.startsWith(getExternalStoragePath().getPath())){
            if(Environment.MEDIA_MOUNTED.equals(getExternalStoragePathState())){
                Log.i("StorageUtils", "checkStoragePathAvailable, is ExternalStorage path");
                return true;
            }
        } else if (path != null && getInternalStoragePath() != null && path.startsWith(getInternalStoragePath().getPath())){
            if(Environment.MEDIA_MOUNTED.equals(getInternalStoragePathState())){
                Log.i("StorageUtils", "checkStoragePathAvailable, is InternalStorage path");
                return true;
            }
        } else if (path != null && getUsbdiskStoragePath() != null && path.startsWith(getUsbdiskStoragePath().getPath())){
            if(isUsbdiskStorageMounted()){
                Log.i("StorageUtils", "checkStoragePathAvailable, is Usbdisk path");
                return true;
            }
        }
        Log.i("StorageUtils", "checkStoragePathAvailable, not valid path");
        return false;
    }
}

