/*
 * Copyright (C) 2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucam;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import com.ucamera.ucam.compatible.params.StorageDirectory;
import com.ucamera.ucam.jni.ImageProcessJni;
import com.ucamera.ucam.utils.LogUtils;
import com.ucamera.ucam.variant.Build;

import android.content.Context;

public class ResourceInitializer {
    public final static String mGhostFile = "ghost.mp3";
    public final static String mOldmovieFile = "res-oldmovie.jpg";
    public final static String mRainFile = "res-rain.jpg";
    public final static String mBubbleFile = "res-bubble.jpg";
    public final static String mBurnFile = "res-burn.jpg";
    public final static String mMetalFile = "res-metal.jpg";
    public final static String mStarFile = "res-star.png";
    public final static String mPhotoFrameDirectory = "photoframe/photoframe";
    public final static String mTextureDirectory = "texture/texture";
    public final static String mPuzzleDirectory = "puzzle";
    public final static String RESOURCE_DIRECTORY = StorageDirectory.instance().getResourceStorageDir();
    public final static String mTimeStampResPath = RESOURCE_DIRECTORY + "/imagedigit";
    public final static String PHOTO_FRAME_DIRECTORY = RESOURCE_DIRECTORY + "/" + mPhotoFrameDirectory;
    public final static String TEXTURE_DIRECTORY = RESOURCE_DIRECTORY + "/" + mTextureDirectory;
    public final static String PUZZLE_DIRECTORY = RESOURCE_DIRECTORY + "/" + mPuzzleDirectory;
    private static final String TAG = "ResourceInitializer";

    private static Context mContext = null;
    public static void initializeResource(Context context) {
        if (mContext != null) {
            return;
        }
        mContext = context;

        ImageProcessJni.SetResourcePath(RESOURCE_DIRECTORY);
        String sopathString = "/data/data/com.ucamera.ucam/lib/";
        if (com.ucamera.ucam.variant.Build.isQrd() || com.ucamera.ucam.variant.Build.isTelecomCloud() || Build.isPepper()
                || com.ucamera.ucam.variant.Build.isVestel()) {
            sopathString = "/system/lib/";
        }
        LogUtils.debug(TAG, "SetJNISOPath:%s", sopathString);
        ImageProcessJni.SetJNISOPath(sopathString);

        new Thread(new Runnable() {
            @Override
            public void run() {
                createFileToSdcard(mGhostFile, RESOURCE_DIRECTORY);
                createFileToSdcard(mOldmovieFile, RESOURCE_DIRECTORY);
                createFileToSdcard(mRainFile, RESOURCE_DIRECTORY);
                createFileToSdcard(mBubbleFile, RESOURCE_DIRECTORY);
                createFileToSdcard(mBurnFile, RESOURCE_DIRECTORY);
                createFileToSdcard(mMetalFile, RESOURCE_DIRECTORY);
                createFileToSdcard(mStarFile, RESOURCE_DIRECTORY);

                copyDirectoryToSDCard();

                for (int i = 0; i < 12; ++i) {
                    String strFileName = i + ".png";
                    createFileToSdcard(strFileName, mTimeStampResPath);
                }
            }
        }).start();
    }

    public static void createFileToSdcard(String strFileName, String strDirectory) {
        String strOnlyFileName = strFileName.substring(strFileName.lastIndexOf("/") + 1);
        String pathAndName = strDirectory + "/" + strOnlyFileName;
        File file2copy = new File(pathAndName);

        /*
         * BUG FIX: 4318
         * BUG CAUSE: did not check asset validity effectively
         * BUG COMMENT: add check file size
         * DATE: 2011-9-16
         */
       long filesize = 0;
       try {
            /* NOTE: Because bmp format is taken as compressed file format, so all extension
             * of bitmap files are modified to mp3.Or the function below will fail.
             */
            filesize = mContext.getAssets().openFd(strFileName).getLength();
        } catch (IOException e) {
            LogUtils.error(TAG, "failed to get size info from assets :"+strFileName);
            return;
        }

        if (file2copy != null && file2copy.isFile() && file2copy.exists() &&
                file2copy.length() == filesize) {
            return ;
        }

        // create directory
        File directory = new File(strDirectory);
        if (!directory.isDirectory()) {
            if (!directory.mkdirs()) {
                return ;
            }
        }

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
            /* SPRD: CID 109000 : Resource leak on an exceptional path (RESOURCE_LEAK)
             * CID 109001 : Resource leak on an exceptional path (RESOURCE_LEAK)
             * @{ */
            try {
                bout.close();
                in.close();
            } catch (Exception ex) {
            }
            return ;
            // return ;
            /* @} */
        }

        try {
            in.close();
            bout.close();
            outputStream.close();
        }
        catch (IOException e) {
            /*sprd:bug515203*/
            try {
                outputStream.close();
            } catch (IOException ex) {
                return;
            }
            return ;
        }
    }

    private static void copyDirectoryToSDCard() {
//        new Thread(new Runnable() {
//            @Override
//            public void run() {
                createDirectoryToSdcard(mPhotoFrameDirectory, PHOTO_FRAME_DIRECTORY);
                createDirectoryToSdcard(mTextureDirectory, TEXTURE_DIRECTORY);
                createDirectoryToSdcard(mPuzzleDirectory, PUZZLE_DIRECTORY);
//            }
//        }).start();
    }

    private static void createDirectoryToSdcard(String strDirectory, String strDest) {
        String[] arrFilesNames = null;
        try {
            arrFilesNames = mContext.getAssets().list(strDirectory);
        } catch (IOException e) {
            e.printStackTrace();
            return ;
        }

        for (String strFileName : arrFilesNames) {
            strFileName = strDirectory + "/" + strFileName;
            createFileToSdcard(strFileName, strDest);
        }
        return ;
    }
}
