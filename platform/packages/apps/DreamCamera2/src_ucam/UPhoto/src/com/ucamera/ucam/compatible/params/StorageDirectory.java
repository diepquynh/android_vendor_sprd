/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.compatible.params;

import android.os.Environment;

import com.sprd.camera.storagepath.StorageUtilProxy;
import com.ucamera.ucam.compatible.Models;
import com.ucamera.ucam.utils.LogUtils;
import com.ucamera.ucam.variant.Build;

import java.io.File;
import java.util.Arrays;

public class StorageDirectory {

    private static final String TAG = "StorageDirectory";
    public static final String DCIM = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM).toString();

    public static final StorageDirectory instance() {
        return new StorageDirectory();
    }

    public String getSystemExternalStorageDir() {
        String external = Environment.getExternalStorageDirectory().toString();

//        if(Models.Samsung_GT_I9508.equals(Models.getModel()) || Models.Samsung_GT_I9500.equals(Models.getModel())) {
//            external = "/storage/extSdCard";
//        } else if(Models.HUAWEI_P6.equals(Models.getModel()) || Models.LAJIAO_LA3_W.equals(Models.getModel())) {
//            external = "/storage/sdcard1";
//        } else if(Models.GOOGLE_Nexus_5.equals(Models.getModel())) {
//            external = "/storage/sdcard0/";
//        }
        return external;
    }

    public String getExternalStorageDirectory() {
//        String defaultDir = Environment.getExternalStorageDirectory().toString();
        String defaultDir = getSystemExternalStorageDir();

        if (Models.Oppo_X907.equalsIgnoreCase(Models.getModel())){
            defaultDir = defaultDir +"/我的照片/UCam";
        } else if (Models.isMeizu()) {
            defaultDir = defaultDir + "/Camera/UCam";
        } else if (Build.isKingSun()) {
            defaultDir = createPath("storage", "sdcard1", "UCam");
        } else if(Build.isTelecomCloud()) {
            defaultDir = DCIM + "/Camera";
        } else {
            defaultDir = DCIM + "/UCam";
        }

//        if(defaultDir.equals("/storage/emulated/0/DCIM/UCam")) {
//            defaultDir = createPath("storage", "sdcard0", "DCIM", "UCam");
//        }
        LogUtils.debug(TAG, "DEFAULT_DIRECTORY = "+ defaultDir);
        return defaultDir;
    }

//    public String getInternalStorageDirectory () {
//        String defaultInternalDir = null;
//
//        if(!Build.isTelecomCloud() && Models.msm8x25Q.equals(Models.getModel())){
//            defaultInternalDir = createPath("storage", "sdcard1", "DCIM", "UCam");
//        }
//
//        return defaultInternalDir;
//    }

    public static String getInternalStorageDirectory(){
        String internalRootDir = null;
        if(!Build.isTelecomCloud() && Models.msm8x25Q.equals(Models.getModel())){
            internalRootDir = createPath("storage", "sdcard1", "DCIM", "UCam");
        }else if(Models.Sony_IS12S.equals(Models.getModel())) {
            internalRootDir = "ext_card";
        }else if(Models.Samsung_GT_I9300.equals(Models.getModel()) ||
                Models.Samsung_GT_N7100.equals(Models.getModel()) ||
                Models.Samsung_GT_N7102.equals(Models.getModel()) ||
                Models.Samsung_GT_N719.equals(Models.getModel()) ||
                Models.Samsung_GT_I9502.equals(Models.getModel())) {
            internalRootDir = createPath("storage","extSdCard");
        } else if(Models.Samsung_GT_I9500.equals(Models.getModel()) || Models.Samsung_GT_I9508.equals(Models.getModel()) ||
                Models.HUAWEI_P6.equals(Models.getModel()) || Models.isLaJiaoPepper()) {
            internalRootDir = createPath("storage","sdcard0");
        } else if(Environment.getExternalStorageDirectory().toString().startsWith(createPath("storage", "emulated"))) {
            internalRootDir = createPath("storage","sdcard0");
        }

        if(internalRootDir != null) {
            File dir = new File(internalRootDir);
            if (dir.exists() && dir.canWrite()) {
                return internalRootDir;
            }
        }

        if(isOtherHasInternalStorageDevice()) {
            return createPath("mnt","sdcard1");
        }
        return null;
    }

    private static boolean isOtherHasInternalStorageDevice() {
        return Arrays.asList(new String[]{
                Models.HTC_X920e,
                Models.HTC_BUFFERFLY,
                Models.Samsung_SCH_I959
        }).contains(Models.getModel());
    }
    public String getExternalStoragePublicDirectory(String type) {
        return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM).toString();
    }

    public String getRootStorageDir() {
        /*
         * FIX BUG: 479
         * BUG CAUSE: Some phones has a internal sdcard, so Environment.getExternalStorageDirectory()
         * returns the internal sdcard's path
         * FIX COMMENT: Relocate the root directory to sdcard'parent
         * DATE: 2012-02-08
         */
        String rootDir = getSystemExternalStorageDir();
        rootDir = rootDir.substring(0, rootDir.lastIndexOf("/"));
        if (rootDir == null || rootDir.length() == 0) {
            rootDir = "/";
        }

        /*
         * FIX BUG: 4844
         * BUG CAUSE: can't get correct root directory by system interface
         * BUG COMMENT: set correct root directory
         * DATE: 2013-09-10
         */
        if(rootDir.startsWith(createPath("storage", "emulated"))) {
            rootDir = createPath("storage");
        }

        return rootDir;
    }

    public String getResourceStorageDir() {
        return StorageUtilProxy.getInternalStoragePath().toString() + "/DCIM/" + ".UCam";
    }

    public String getDownloadStorageDir() {
        return Environment.getExternalStorageDirectory() + "/UCam/download/";
    }

    public String getLogsStorageDir() {
        return Environment.getExternalStorageDirectory().toString() + "/UCam/logs/";
    }

    public String getDebugStorageDir() {
        return Environment.getExternalStorageDirectory().toString() + "/UCam/debug/";
    }

    public static final String createPath(String ... parts) {
        StringBuilder sb = new StringBuilder();
        for (String s: parts) {
            sb.append(File.separator).append(s);
        }
        return sb.toString();
    }

}
