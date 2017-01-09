/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.text.TextUtils;
import android.util.Log;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.lang.Object;

/**
 * The class is mainly used to keep the edit bitmap and its informaton as an entity.
 */
public class ImageEditDesc {
    private final static String TAG = "ImageEditDesc";
    private Bitmap mBitmap;
    private ArrayList<Bitmap> memoQueue = new ArrayList<Bitmap>();
    private int memoIndex = 0;   // index of bitmap queue
    private int mRotation = 0;  // rotation of mBitmap
    private Uri originalUri;
    private Uri editImageUri;
    private String originalName;
    private String imageEditPath;
    private Handler handler;
    private static ImageEditDesc editDesc = new ImageEditDesc();
    private static Object loc = new Object();
    // SPRD:Fix bug 520951
    private static int instanceCount = 0;

    private ImageEditDesc() {
     // SPRD:Fix bug 520951
        instanceCount = 0;
    }

    public static ImageEditDesc getInstance() {
        return editDesc;
    }

    /* SPRD:fix bug 520951 @{ */
    public static ImageEditDesc getCountInstance() {
        synchronized(loc){
            instanceCount++;
        }
        return editDesc;
       }

    public void uninitilizeCount() {
        synchronized (loc) {
            instanceCount--;

            if (instanceCount > 0) {
                return;
            }

            uninitilize();
        }
    }
    /* }@ */

    public void uninitilize() {
        setEditImageUri(null);
        clearAllQueue();
        releaseTempBitmap();
        setOperationHandler(null);
    }

    public void setImageEditedPath(String path) {
        imageEditPath = path;
    }

    public String getImageEditPath() {
        /*
         * BUG FIX: 1980 1981
         * BUG CAUSE: re-edit the same picture
         *              will save the result as the same filename
         * FIX COMMENT: just name it different filename in different session
         * Date: 2012-11-22
         */
        /*
         * BUG FIX: 3367
         * BUG CAUSE: if every time add _00000 suffix, will cause filename too long
         * FIX COMMENT:
         * The new rule is
         *   ABC.jpg => ABC_U000000.jpg
         *   ABC_U000000.jpg => ABC_U111111.jpg
         *
         * DATE: 2013-03-27
         */
        if (!TextUtils.isEmpty(imageEditPath)) {
            String now = getTime();
            String name = imageEditPath;
            String ext  = "";
            int index = name.lastIndexOf(".");
            if (index != -1) {
                ext  = name.substring(index);
            }
            int index2 = name.lastIndexOf("/");
            if(index2 != -1) {
                name = name.substring(0, index2);
            }
            imageEditPath = name + "/UPHOTO_"+ now + ext;
        }
        return imageEditPath;
    }

    public String getRootEditPath() {
        String rootPath = Environment.getExternalStorageDirectory().toString();
        String now = getTime();
        imageEditPath = rootPath + "/UPHOTO_"+ now + ".jpg";
        return imageEditPath;
    }

    private static String getTime (){
        SimpleDateFormat format = new SimpleDateFormat("yyyyMMdd_HHmmss");
        return  format.format(new Date(System.currentTimeMillis()));

    }
    public String getOriginalName() {
        return originalName;
    }

    public void setOriginalName(String originalName) {
        this.originalName = originalName;
    }

    public int getRotation() {
        return mRotation;
    }

    public void setRotation(int mRotation) {
        this.mRotation = mRotation;
    }


    public Uri getOriginalUri() {
        return originalUri;
    }

    public void setOriginalUri(Uri oldUri) {
        this.originalUri = oldUri;
    }

    public Bitmap getBitmap() {
        return mBitmap;
    }

    public Uri getEditImageUri() {
        return editImageUri;
    }

    public void setEditImageUri(Uri editImageUri) {
        this.editImageUri = editImageUri;
    }

    public void setBitmap(Bitmap bitmap) {
        mBitmap = bitmap;
        if(memoQueue.size() == 0){
            memoQueue.add(bitmap);
        }
        memoIndex = memoQueue.indexOf(mBitmap);
        Log.d(TAG, "setBitmap(): memoIndex = " + memoIndex + ", mBitmap = " + mBitmap);
    }

    /**
     * update the bitmap of top stack
     * @param bitmap current bitmap
     */
    public void setUpdateBitmap(Bitmap bitmap) {
        mBitmap = bitmap;
        if(memoQueue != null && !memoQueue.isEmpty()) {
            int size = getqueueSize();
            Bitmap topBitmap = memoQueue.get(size - 1);
            if(topBitmap != null) {
                topBitmap.recycle();
                topBitmap = null;
            }
            memoQueue.set(size - 1, bitmap);
            memoIndex = memoQueue.indexOf(mBitmap);
        }
    }

    public Bitmap updateBitmap(Context context, Bitmap bitmap){
        if(bitmap == null){
            return null;
        }
        // if in low memory, release some previous bitmap
        if (Utils.checkDangousMemory(context)){
            reorganizeQueue();
        }
        clearReDoQueue();
        if(bitmap!= null && !bitmap.isRecycled() &&  !memoQueue.contains(bitmap)){
            memoQueue.add(bitmap);
        }
        Log.d(TAG, "updateBitmap(): bitmap = " + bitmap + ", mBitmap = " + mBitmap + ", memoIndex = " + memoIndex);
        mBitmap = bitmap;
        memoIndex = memoQueue.indexOf(mBitmap);
//        mRotation = 0;  // fix bug 28599
        onUndoIconChanged(true);
        return mBitmap;
    }

    public void clearReDoQueue(){
        onRedoIconChanged(false);
        /* SPRD: CID 108939 : Dereference after null check (FORWARD_NULL) @{ */
        if((memoQueue == null) || (memoQueue != null && memoQueue.size() <= 1) || (memoQueue.indexOf(mBitmap) >= memoQueue.size() - 1)){
            return;
        }
        /**
        if(memoQueue != null && memoQueue.size() <= 1 || memoQueue.indexOf(mBitmap) >= memoQueue.size() - 1){
            return;
        }
        */
        /* @} */

        Log.d(TAG, "clearReDoQueue(): memoIndex = " + memoIndex + ", (memoQueue.size() - 1 ) = "  + (memoQueue.size() - 1 ));
        do {
            Bitmap bmp = memoQueue.get(memoQueue.indexOf(mBitmap) + 1);
            memoQueue.remove(memoQueue.indexOf(mBitmap) + 1);
            bmp.recycle();
            bmp = null;
        } while (memoQueue.size() - 1 > memoQueue.indexOf(mBitmap));
        memoIndex = memoQueue.indexOf(mBitmap);
        Log.d(TAG, "clearReDoQueue(): mBitmap" + mBitmap + ", mBitmap's index = "
                + memoQueue.indexOf(mBitmap) + ", memoIndex = " + memoIndex);
    }

    public void clearAllQueue(){
        int size = memoQueue.size()-1;
        for(int i = 0; i < size; i++){
            Bitmap bmp = memoQueue.get(i);
            bmp.recycle();
            bmp = null;
        }
        memoIndex = 0;
        memoQueue.clear();
        onRedoIconChanged(false);
        onUndoIconChanged(false);
    }

    public void releaseTempBitmap() {
        Utils.recyleBitmap(mBitmap);
        mBitmap = null;
    }

    public boolean executeUndo() {
        Log.d(TAG, "executeUndo(): memoIndex: " + memoIndex);
        if (memoQueue.size() <= 1 || memoIndex == 0) {
            return false;
        }
        if (memoIndex > 0 && memoIndex <= memoQueue.size()) {
            memoIndex--;
        } else if (memoIndex <= 0) {
            Log.d(TAG, "executeUndo(): has exception: memoIndex < 0");
            return false;
        } else if (memoIndex > memoQueue.size()) {
            Log.d(TAG, "executeUndo(): has exception: memoIndex > memoQueue.size() - 1");
            return false;
        }
        mBitmap = memoQueue.get(memoIndex);
        Log.d(TAG, "executeUndo(): memoIndex = " + memoIndex + ", mBitmap: " + mBitmap);

        onRedoIconChanged(true);
        if (memoIndex == 0) {
            onUndoIconChanged(false);
        }
        return true;
    }

    public boolean executeRedo() {
        if(memoIndex >= memoQueue.size() - 1){
            return false;
        }
        if (memoIndex >= 0 && memoIndex < memoQueue.size() - 1) {
            memoIndex++;
            Log.d(TAG, "executeRedo(): memoIndex " + memoIndex);
        } else if (memoIndex < 0) {
            Log.d(TAG, "executeRedo(): has exception: memoIndex <= 0");
            return false;
        } else if (memoIndex >= memoQueue.size() - 1) {
            Log.d(TAG, "executeRedo(): has exception: memoIndex >= memoQueue.size()-1");
            return false;
        }
        mBitmap = memoQueue.get(memoIndex);
        Log.d(TAG, "executeRedo(): memoIndex = " + memoIndex + ", queueSize = " + memoQueue.size() + ", mBitmap = " + mBitmap);
        if (memoIndex == memoQueue.size() - 1) {
            onRedoIconChanged(false);
        }
        onUndoIconChanged(true);
        return true;
    }

    public int getqueueSize(){
        return memoQueue.size();
    }

    public int getCurrentBitmapIndex(){
        return memoIndex;
    }

    private void onRedoIconChanged(boolean isFocus){
        if (handler == null) {
            Log.d(TAG,"ImageEditDesc.handler is not set.");
            return;
        }
        if(isFocus){
            handler.sendEmptyMessage(ImageEditConstants.ACTION_REDO_ICON_FOCUSED);
        }else{
            handler.sendEmptyMessage(ImageEditConstants.ACTION_REDO_ICON_NOT_FOCUSED);
        }
    }

    private void onUndoIconChanged(boolean isFocus) {
        if (handler == null) {
            Log.d(TAG,"ImageEditDesc.handler is not set.");
            return;
        }
        if(isFocus){
            handler.sendEmptyMessage(ImageEditConstants.ACTION_UNDO_ICON_FOCUSED);
        }else{
            handler.sendEmptyMessage(ImageEditConstants.ACTION_UNDO_ICON_NOT_FOCUSED);
        }
    }

    public void setOperationHandler(Handler handler) {
        this.handler = handler;
    }

    /**
     *  quit the previous half step to recycle bitmap for releasing memory.
     */
    public int reorganizeQueue() {
        int index = 0;
        int count = memoQueue.size();
        int size = (count - 1) / 2 ;
        Log.d(TAG, "reorganizeQueue(): memoIndex = " + memoIndex + ", count = " + count
                + ", size = " + size + ", mBitmap = " + mBitmap);
        for(int i = size; i > 0; i--){
            Bitmap bmp = memoQueue.get(i);
            if(mBitmap.equals(bmp) && mBitmap.hashCode() == bmp.hashCode()){
                continue;
            }
            bmp.recycle();
            bmp = null;
            memoQueue.remove(i);
        }
        Log.d(TAG, "reorganizeQueue(): mBitmap = " + mBitmap + ", memoQueue = " + memoQueue);
        memoIndex = memoQueue.indexOf(mBitmap);
        index = memoIndex;
        Log.d(TAG, "reorganizeQueue(): memoIndex = " + memoIndex + ", mBitmap: " + mBitmap);
        return index;
    }

    public void clearQueueWithoutFisrtAndCurrent(){
        clearReDoQueue();
        while(memoQueue.indexOf(mBitmap) > 0){
            int index = 0;
            Bitmap bmp = memoQueue.get(index);
            bmp.recycle();
            bmp = null;
            memoQueue.remove(index);
        }
        memoIndex = memoQueue.indexOf(mBitmap);
        Log.d(TAG, "clearQueueWithoutFisrtAndCurrent(): memoIndex = " + memoIndex + ", mBitmap = " + mBitmap);
    }

    public static int getScreenWidth() {
        return UiUtils.screenWidth();
    }

    public static int getScreenHeight() {
        return UiUtils.screenHeight();
    }

    /**
     * calculate the angle by orientation
     * @param orientation ExifInterface attribute
     * @return current angle
     */
    public static int getDegree(int orientation) {
        int angle = 0;
        if(orientation == 1){
            angle = 0;
        }else if(orientation == 8){
            angle = 90;
        }else if(orientation == 3){
            angle = 180;
        }else if(orientation == 6){
            angle = 270;
        }
        return angle;
    }

    public static float getLabelScaleRatio() {
        float density = 1.0f;
        boolean isHoneyCombo = Build.VERSION.SDK_INT > Build.VERSION_CODES.GINGERBREAD_MR1
        && Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH;

        if(isHoneyCombo) {
            density = 2.0f;
        } else if(Math.abs(UiUtils.screenDensity() - 1.0) < 0.1) {
            density = 0.75f;
        }

        return density;
    }
}
