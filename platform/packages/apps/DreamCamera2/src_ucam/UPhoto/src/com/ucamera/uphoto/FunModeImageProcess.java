/**
 * Copyright (C) 2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;
import com.ucamera.ucam.jni.ImageProcessJni;

import android.os.Handler;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Bitmap.Config;
import android.util.Log;

import android.net.Uri;
import android.content.ContentResolver;
import android.location.Location;

public class FunModeImageProcess {
    private static String TAG = "FunMode";
    private Handler mHandler = null;
    private byte[] mJpegData = null;
    private int mCurrentFunMode = 0;
    private Location mLocation = null;
    private long mDateTaken = 0;
    private int mDegree = 0;
    private Bitmap mBitmap = null;
    private Uri mLastContentUri = null;
    private ContentResolver mContentResolver = null;
    private String mOriginFileName = null;
    private Thread mProcessThread = null;
    private boolean mAddTimeStamp = false;
//    private int mImageWidth = 0;
//    private int mImageHeight = 0;
//    private int mOrientation = 0;
    private boolean mIsThreadRunning = false;
    private int mCurrentCategoryEffectCount;
    Bitmap[] mAllEffectThumbnails = null;
    boolean mToResetThumbnails = true;
    private int mEffectThumbnailsWidth;

    public boolean isThreadRunning() {
        return mIsThreadRunning;
    }
    public void setThreadState(boolean mThreadState) {
        this.mIsThreadRunning = mThreadState;
    }

    public FunModeImageProcess(int effectThumbnailsWidth) {
        mEffectThumbnailsWidth = effectThumbnailsWidth;
    }

    public FunModeImageProcess(ContentResolver contentResolver, Handler handler,
        Location loc, int effectThumbnailsWidth) {
        mContentResolver = contentResolver;
        mHandler = handler;
        mLocation = loc;
        mEffectThumbnailsWidth = effectThumbnailsWidth;
    }


    public void setDateTaken(long dateTaken) {
        mDateTaken = dateTaken;
    }

    public void setOriginFileName(String strOriginFileName) {
        mOriginFileName = strOriginFileName;
    }

    public Uri getLastContentUri() {
        return mLastContentUri;
    }

    public byte[] getLastJpegData() {
        return mJpegData;
    }

//    public void doImageProcess(byte[] data, int nCurrentFunMode, boolean nTimeStamp, int nImageWidth, int nImageHeight){
//        mJpegData = data;
//        mCurrentFunMode = nCurrentFunMode;
//        mAddTimeStamp = nTimeStamp;
//
//        mProcessThread = new Thread(new Runnable() {
//            public void run() {
//                generateAndGetAllEffectThumbnails();
//                setFunEffectResult(mCurrentFunMode);
//                //getAllEffectThumbnails(); // this has to be called after  setFunEffectResult
//                if (mHandler != null ) {
//                    Message m = mHandler.obtainMessage(Camera.FUN_IMAGE_PROCESS, mDegree, 0, mBitmap);
//                    mHandler.sendMessage(m);
//                }
//            }
//        });
//        setThreadState(true);
//        mProcessThread.start();
//    }

//    public boolean setFunEffectResult(int currentFunMode) {
//        String strFilePath = ImageManager.getCameraImageBucketName();
////        String strFileName =  mOriginFileName + ".jpg";
//        String strResultFileName = mOriginFileName + "_1" + ".jpg";
//
//        //ImageProcessJni.ImageColorProcessFile(strOrginFile, currentFunMode, strTemp);
//        String strDateTime = null;
//        if (mAddTimeStamp) {
//            strDateTime = ((Camera)(Camera.gCameraActivity)).getCurrentTime(mDateTaken);
//        }
//
//        //mJpegData = ImageProcessJni.ImageColorProcessBuffer(mJpegData, mJpegData.length, currentFunMode, strDateTime);
//        ImageProcessJni.SetEffectSrcBuffer(mJpegData, mJpegData.length, currentFunMode, strDateTime);
//        mJpegData = ImageProcessJni.ExecuteEffect();
//
//        int [] degree = new int[1];
//        degree[0] = 0;
//        // if the image with this filename exists, delete the record first
//        uniqueFilename(strFilePath,strResultFileName);
//
//        mLastContentUri = ImageManager.addImage(mContentResolver,
//            strResultFileName,
//            mDateTaken,
//            mLocation,
//            strFilePath,
//            strResultFileName,
//            null, mJpegData, degree, false);
//        // register new file
//        /*mLastContentUri = ImageManager.registerResultFile(mContentResolver,
//                strResultFileName,
//                mDateTaken,
//                mLocation,
//                strFilePath,
//                strResultFileName,
//                degree);*/
//        mDegree = degree[0];
//        getResultFileJpegData();
//        return true;
//    }

//    private void uniqueFilename(String strFilePath, String strResultFileName) {
//        String filePath = strFilePath + "/" + strResultFileName;
//        String[] IMAGE_PROJECTION = new String[] { Media._ID };
//        Cursor rc = Media.query(mContentResolver,
//                Images.Media.EXTERNAL_CONTENT_URI, IMAGE_PROJECTION, Images.Media.DATA
//                        + "= '" + filePath+"'", null, null);
//        if (rc != null) {
//            rc.moveToFirst();
//            if(rc.getCount() != 0){
//
//                Uri delUri = Uri.parse(Images.Media.EXTERNAL_CONTENT_URI.toString()
//                        + "/" + String.valueOf(rc.getLong(0)));
//                ImageManager.deleteUri(mContentResolver, delUri, null, null);
//            }
//        }
//    }


    private void getResultFileJpegData() {
//        try {
//            if (Camera.mFunModeHardwareEffect == mCurrentFunMode) {
                mBitmap = BitmapFactory.decodeByteArray(mJpegData, 0, mJpegData.length,Utils.getNativeAllocOptions());
//            }else {
//                InputStream is = mContentResolver.openInputStream(mLastContentUri);
//                mBitmap = BitmapFactory.decodeStream(is,null,Util.getNativeAllocOptions());
//            }
//        } catch (FileNotFoundException e) {
//            e.printStackTrace();
//        }

        if (mBitmap == null) {
            Log.e(TAG, "Decode from last uri is null!");
            return ;
        }

        /*ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        mBitmap.compress(Bitmap.CompressFormat.JPEG, 100,  buffer);
        mJpegData = buffer.toByteArray();
        if (buffer != null) {
            try {
                buffer.close();
            } catch (Throwable t) {
                // do nothing
            }
        }*/

        return ;
    }

    public Thread getProcessThread() {
        return mProcessThread;
    }
    public void emptyProcessThread() {
        mProcessThread = null;
    }

    public boolean interruptFunProcessThread() {
        boolean bReturen = false;
        if (mProcessThread != null) {
            mProcessThread.interrupt();
//            try {
//                Log.d(TAG, "wait for thread exit");
//                mProcessThread.join();
//            } catch (InterruptedException ex) {
//                // ignore
//            }
            bReturen = true;
        }
        mProcessThread = null;
        return bReturen;
    }

    public  Bitmap[] getEffectThumbnails(){
        return mAllEffectThumbnails;
    }

    public void setJpegData(byte[] date){
        mJpegData = date;
    }
    public  void generateAndGetAllEffectThumbnails(){
        if(mToResetThumbnails){
            if (mJpegData == null){
                return;
            }

            mToResetThumbnails = false;
            ImageProcessJni.GenerateThumbnails(mJpegData, mJpegData.length);

            // get thumbnails
            mAllEffectThumbnails = new Bitmap[mCurrentCategoryEffectCount];
            for (int i = 0; i < mCurrentCategoryEffectCount; i++) {
                int[] data= ImageProcessJni.GetEffectBmp(i);
                if (data == null || data.length == 0) {
                    return;
                }
                mAllEffectThumbnails[i] = Bitmap.createBitmap(data, 0, mEffectThumbnailsWidth,
                        mEffectThumbnailsWidth, mEffectThumbnailsWidth, Config.ARGB_8888);
            }
        }
    }

    public void setCurrentCategoryEffectCount(int currentCategoryEffectCount) {
        mCurrentCategoryEffectCount = currentCategoryEffectCount;
    }

    public void resetEffectThumbnails(){
        mToResetThumbnails = true;
//        for (int i = 0; i < Camera.FUN_COUNT; i++) {
//            if (mAllEffectThumbnails[i]!=null && !mAllEffectThumbnails[i].isRecycled()) {
//                mAllEffectThumbnails[i].recycle();
//            }
//        }
    }

    public boolean needToResetEffectThumbnails() {
        return mToResetThumbnails;
    }

 }
