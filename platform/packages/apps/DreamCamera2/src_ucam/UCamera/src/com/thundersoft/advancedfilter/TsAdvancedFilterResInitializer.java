/*
 * Copyright (C) 2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.thundersoft.advancedfilter;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.content.Context;
import android.os.Environment;
import android.util.Log;

import com.sprd.camera.storagepath.StorageUtilProxy;
import com.ucamera.ucam.modules.utils.LogUtils;

public class TsAdvancedFilterResInitializer {

    private static final String DEFAULT_DIR = "/DCIM/Camera";
    public final static String RESOURCE_DIRECTORY = getFileDir() + "/.UCam";
    private static final String TAG = "MagicResInitializer";

    /*
     * copy magiclens resources
     */
    private static Context mContext = null;
    public static void initializeRes(Context context) {
        if (mContext != null) {
            return;
        }

        if (context != null) {
            mContext = context.getApplicationContext();
        }

        File picFile=new File(RESOURCE_DIRECTORY);
        if(!picFile.exists()){
            picFile.mkdirs();
        }
        Log.e("nanxn", "initializeRes");
        new Thread(new Runnable() {
            @Override
            public void run() {
                String[] gpuTextureRes={"effect_rainbrown_1.png", "res_instagram_lomocolor.png",
                        "effect_autumn_1.png"};
                for(int i=0;i<gpuTextureRes.length;i++){
                    createFileToSdcard(gpuTextureRes[i], RESOURCE_DIRECTORY);
                }
            }
        }).start();
    }

    public static void createFileToSdcard(String strFileName, String strDirectory) {
        String strOnlyFileName = strFileName.substring(strFileName.lastIndexOf("/") + 1);
        String pathAndName = strDirectory + "/" + strOnlyFileName;
        File file2copy = new File(pathAndName);
        LogUtils.debug(TAG + "OwenX", "createFileToSdcard(): strFileName is " + strFileName + ", strDirectory is " + strDirectory
                + ", strOnlyFileName is " + strOnlyFileName + ", pathAndName is " + pathAndName);

       long filesize = 0;
       try {
            filesize = mContext.getAssets().openFd(strFileName).getLength();
        } catch (IOException e) {
            Log.e(TAG, "failed to get size info from assets :"+strFileName);
            return;
        }

        if (file2copy != null && file2copy.isFile() && file2copy.exists() &&
                file2copy.length() == filesize) {
            return ;
        }

//        // create directory
//        File directory = new File(strDirectory);
//        if (!directory.isDirectory()) {
//            if (!directory.mkdirs()) {
//                return ;
//            }
//        }

        File file = new File(strDirectory, strOnlyFileName);
        FileOutputStream outputStream = null ;
        try {
            outputStream = new FileOutputStream(file);
        }
        catch (FileNotFoundException e) {
            return ;
        }
        catch (SecurityException e) {
             return ;
        }

        InputStream in = null;
        try {
            in  = mContext.getAssets().open(strFileName);
        }
        catch (IOException e) {
            return ;
        }

       BufferedOutputStream bout = new BufferedOutputStream(outputStream);
        byte[] b = new byte[8096];
        try {
            int len = in.read(b);
            while (len != -1){
                bout.write(b, 0, len);
                len = in.read(b);
            }
        }
        catch (IOException e) {
            return ;
        }

        try {
            in.close();
            bout.close();
            outputStream.close();
        }
        catch (IOException e) {
            return ;
        }
    }

    public static String getFileDir() {
        Log.i(TAG, "getFileDir");
        if (getStoragePathState("Internal")) {
            return StorageUtilProxy.getInternalStoragePath().toString() + DEFAULT_DIR;
        } else if (getStoragePathState("External")) {
            return StorageUtilProxy.getExternalStoragePath().toString() + DEFAULT_DIR;
        } else {
            return null;
        }
    }

    public static boolean getStoragePathState(String storage) {
        Log.i(TAG, "getStoragePathState");
        return "External".equals(storage) ?
                Environment.MEDIA_MOUNTED.equals(StorageUtilProxy.getExternalStoragePathState()) :
                    Environment.MEDIA_MOUNTED.equals(StorageUtilProxy.getInternalStoragePathState());
    }

}
