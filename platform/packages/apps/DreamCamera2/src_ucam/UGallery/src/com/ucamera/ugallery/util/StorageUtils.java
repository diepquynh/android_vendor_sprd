/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 *
 * Copyright (C) 2010 The Android Open Source Project
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

package com.ucamera.ugallery.util;

import static com.ucamera.ugallery.util.Models.*;

import java.io.File;
import java.util.Arrays;

import com.ucamera.ugallery.gallery.privateimage.util.Constants;

import android.os.Environment;
public class StorageUtils {
    private static final String TAG = "StorageUtils";
    // Match the code in MediaProvider.computeBucketValues().
    public static final String DCIM = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM).toString();
    public static final String EXTERNAL_STORAGE_DIRECTORY = Environment.getExternalStorageDirectory().toString();

    /**
     * Matches code in MediaProvider.computeBucketValues.
     */
    /*
     * FIX BUG: 3539
     * BUG CAUSE: we should modify the path if it has contain "DCIM"
     * FIX COMMENT: modify the content behind "DCIM" to "Photography"
     * DATE: 2013-04-16
     */
    public static String generateBucketId(String path) {
        String filePath = path;
        if(path.startsWith(EXTERNAL_STORAGE_DIRECTORY + "/DCIM")
            && (SN_N06C.equals(getModel())
                || SN_IS11N.equals(getModel())
                || SN_IS11CA.equals(getModel())
                || SN_N_06C.equals(getModel())
                || SN_N_03E.equals(getModel())
                || SN_N_04C.equals(getModel())
                || SN_N_04D.equals(getModel())
                || SN_N_07D.equals(getModel()))) {
            filePath = EXTERNAL_STORAGE_DIRECTORY + "/DCIM/Photography";
        }else if(path.startsWith(createPath("mnt","sdcard","external_sd","DCIM"))
                && SN_N_04D.equals(getModel())){
            filePath = createPath("mnt","sdcard","external_sd","DCIM","Photography");
        }
        return String.valueOf(filePath.toLowerCase().hashCode());
    }
    public static final String createPath(String ... parts) {
        StringBuilder sb = new StringBuilder();
        for (String s: parts) {
            sb.append(File.separator).append(s);
        }
        return sb.toString();
    }
    public static boolean isStartWith(String path) {
        String filePath = path;
        if(path.startsWith(EXTERNAL_STORAGE_DIRECTORY + "/DCIM")
            && (SN_N06C.equals(getModel())
                || SN_IS11N.equals(getModel())
                || SN_IS11CA.equals(getModel())
                || SN_N_06C.equals(getModel())
                || SN_N_03E.equals(getModel())
                || SN_N_04C.equals(getModel())
                || SN_N_04D.equals(getModel())
                || SN_N_07D.equals(getModel()))) {
            filePath = EXTERNAL_STORAGE_DIRECTORY + "/DCIM/Photography";
        }else if(path.startsWith(createPath("mnt","sdcard","external_sd","DCIM"))
                && SN_N_04D.equals(getModel())){
            filePath = createPath("mnt","sdcard","external_sd","DCIM","Photography");
        }
        if(filePath.startsWith(Constants.STORE_DIR_LOCKED.concat(EXTERNAL_STORAGE_DIRECTORY + "/DCIM"))) {
            return true;
        }
        return false;
    }
    public static boolean isContains() {
        return Arrays.asList(
                new String[] { SN_N06C, SN_IS11N, SN_IS11CA, SN_N_06C,
                        SN_N_03E, SN_N_04C, SN_N_04D, SN_N_07D }).contains(
                getModel());
    }
}
