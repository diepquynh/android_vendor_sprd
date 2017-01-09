/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.text.TextUtils;
import android.util.Log;
import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

import com.sprd.camera.storagepath.StorageUtil;

import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import com.ucamera.uphoto.Const;
import com.sprd.camera.storagepath.StorageUtilProxy;

import android.content.Context;

public class PuzzleStorage {

    private Context mContext;
    public static final String DCIM = Environment
            .getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)
            .toString();
    private static final String DEFAULT_DIR = "/DCIM/Camera";
    public static final String DIRECTORY = DCIM + "/Camera";
    public static final String INTERNALDIR = StorageUtilProxy
            .getInternalStoragePath().toString() + DEFAULT_DIR;
    public static final String EXTERNALDIR = StorageUtilProxy
            .getExternalStoragePath().toString() + DEFAULT_DIR;
    public static final String KEY_DEFAULT_INTERNAL = "Internal";
    public static final String KEY_DEFAULT_EXTERNAL = "External";

    private PuzzleStorage(Context context){
        mContext = context;
    }

    public static PuzzleStorage getStorage(Context context){
        return new PuzzleStorage(context);
    }

    public Uri saveBitmap(Context context, Bitmap bitmap, String filepath, String filename){
        if(TextUtils.isEmpty(filepath)) {
            filepath = getStoragePath();
        }
        if(TextUtils.isEmpty(filename)) {
            filename = makeBaseFilename(System.currentTimeMillis());
        }
        String title = null;
        if(filename.lastIndexOf(".")>0) {
            title = filename.substring(0 ,filename.lastIndexOf("."));
        }
        /*
         * FIX BUG: 6100
         * BUG COMMENT: remove last thumbnail of file dir after save image in uphoto
         * DATE: 2014-03-13
         */
        File file = new File(context.getFilesDir(),"last_image_thumb");
        if(file.exists() && file.isFile()) {
            /* SPRD:  CID 109187 : RV: Bad use of return value (FB.RV_RETURN_VALUE_IGNORED_BAD_PRACTICE) @{ */
            if(!file.delete()){
                return null;
            }
            // file.delete();
            /* @} */
        }
        return com.ucamera.uphoto.ImageManager.addImage(
                mContext.getContentResolver(),
                title,
                System.currentTimeMillis(),
                null,
                filepath,
                filename,
                bitmap,
                null,
                new int[]{0},
                false,
                // SPRD add : in Puzzle editer we don't need save voice of picture, so originalUri is null
                null);
    }

    public String makeBaseFilename(long dateTaken) {
        Date date = new Date(dateTaken);
        SimpleDateFormat dateFormat =
            new SimpleDateFormat(mContext.getString(R.string.puzzle_file_name_format), Locale.US);
        return dateFormat.format(date) + ".jpg";
    }
    /*
    public String getStoragePath() {
        // SharedPreferences prefs =
        // PreferenceManager.getDefaultSharedPreferences(mContext);
        // return prefs.getString(com.ucamera.uphoto.Const.KEY_UCAM_SELECT_PATH,
        // com.ucamera.uphoto.ImageManager.getCameraUCamPath());

//        Context content = null;
//        try {
//            content = mContext.createPackageContext("com.android.camera2", Context.CONTEXT_IGNORE_SECURITY);
//        } catch (NameNotFoundException ex) {
//            Log.e("PuzzleStorage", "Find package error!");
//        }
//        if (content == null) {
//            return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)
//                    .toString() + "/Camera";
//        }
//        SharedPreferences prefs = content.getSharedPreferences(
//                "com.android.camera2_preferences",
//                Context.MODE_WORLD_READABLE | Context.MODE_MULTI_PROCESS);
//        if (prefs == null) {
//            return Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)
//                    .toString() + "/Camera";
//        }
//        String path = prefs.getString("pref_camera_storage_path",
//                com.ucamera.uphoto.ImageManager.getCameraUCamPath());
//        Log.d("PuzzleStorage", "getStoragePath path=" + path);
//        //return path+"/DCIM/Camera";
//        return"/storage/emulated/0/DCIM/Camera";
         SharedPreferences prefs =
         PreferenceManager.getDefaultSharedPreferences(mContext);
         String path = prefs.getString("pref_camera_storage_path", com.ucamera.uphoto.ImageManager.getCameraUCamPath());
         if (KEY_DEFAULT_INTERNAL.equals(path)) {
            path = INTERNALDIR;
         } else if (KEY_DEFAULT_EXTERNAL.equals(path)) {
             path = EXTERNALDIR;
         } else {
             path = INTERNALDIR;
         }
         Log.i("PuzzleActivity", "path="+path);
         return path;
    }
    */
    /**
     * SPRD Bug:608528 storage path wrong when save puzzle pic
     * @return
     */
    public String getStoragePath() {
        StorageUtil storageUtil = StorageUtil.getInstance();
        return storageUtil.getFileDir();
    }
}

