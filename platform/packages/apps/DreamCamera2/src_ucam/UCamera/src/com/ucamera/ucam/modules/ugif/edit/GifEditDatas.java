/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.ArrayList;

import com.ucamera.ucam.modules.utils.UiUtils;
import com.ucamera.ucam.modules.utils.Utils;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.Bitmap.Config;
import android.util.Log;
import com.ucamera.uphoto.ImageEditOperationUtil;

/**
 * this class saved all the gifedit datas, include origin bitmaps and edited
 * bitmaps and the byte arrays of bitmaps. The operation will take effect on the
 * datas
 */
public class GifEditDatas {
    private final static String TAG = "GifEditDatas";
    private static Bitmap mOriginBitmaps[] = null;
    private static Bitmap mEditedBitmaps[] = null;
    private static Bitmap mResultBitmaps[] = null;
    private static ArrayList<byte[]> mOriginJpegDataList = null;
    //SPRD:fix bug530324,532799 java.lang.IndexOutOfBoundsException
    public static boolean isDataReady = true;
    public static Object mLock = new Object();

    public static int initBitmaps(ArrayList<String> uriList, int fitGifSize) {
        Bitmap bitmap;
        int bitmapNums = uriList.size();
        mOriginJpegDataList = new ArrayList<byte[]>(bitmapNums);
        mOriginBitmaps = new Bitmap[bitmapNums];
        mResultBitmaps = new Bitmap[bitmapNums];
        for (int i = 0, j = 0; i < uriList.size(); i++) {
            bitmap = getScaleBitmap(uriList.get(i), fitGifSize);
            if (bitmap != null) {
                if(UiUtils.highMemo()) {
                    mOriginBitmaps[j] = bitmap;
                } else {
                    mResultBitmaps[j] = bitmap;
                }
                j++;
                mOriginJpegDataList.add(ImageEditOperationUtil.transformBitmapToBuffer(bitmap));
            } else {
                bitmapNums--;
            }
        }
        return bitmapNums;
    }

    public static void initEditBitmaps() {
        if (UiUtils.highMemo() && mOriginBitmaps == null) {
            return;
        }
        recycleEditBitmaps(true);
        if (mEditedBitmaps == null) {
            mEditedBitmaps = new Bitmap[UiUtils.highMemo() ? mOriginBitmaps.length: mResultBitmaps.length];
        }
        if( UiUtils.highMemo() ) {
            recycleResultBitmaps();
            if(mResultBitmaps == null) {
                mResultBitmaps = new Bitmap[mEditedBitmaps.length];
            }
            for (int i = 0; i < mOriginBitmaps.length; i++) {
                if (mResultBitmaps[i] != mOriginBitmaps[i]) {
                    Bitmap tmp = mResultBitmaps[i];
                    mResultBitmaps[i] = mOriginBitmaps[i].copy(Config.ARGB_8888, true);
                    Utils.recycleBitmap(tmp);
                }
            }
        }
    }

    public static void pushBitmaps() {
        /* SPRD: Fix bug 536577 java.lang.IndexOutOfBoundsException @{ */
        synchronized (mLock) {
            if(mOriginJpegDataList == null) {
                mOriginJpegDataList = new ArrayList<byte[]>(mEditedBitmaps.length);
            }else {
                mOriginJpegDataList.clear();
            }
            for(int i = 0; i < mEditedBitmaps.length; i++) {
                if(mResultBitmaps[i] != mEditedBitmaps[i] && !mEditedBitmaps[i].isRecycled()) {
                    Bitmap bitmap = mResultBitmaps[i];
                    mResultBitmaps[i] = mEditedBitmaps[i].copy(Config.ARGB_8888, true);
                    bitmap.recycle();
                }
                mOriginJpegDataList.add(ImageEditOperationUtil.transformBitmapToBuffer(mEditedBitmaps[i]));
            }
        }
        /* @} */
    }

    public static void initPreEditBitmap() {
        /*
         * FIX BUG : 4740
         * BUG COMMENT : avoid null point exception
         * DATE : 2013-08-23
         */
        if(mResultBitmaps == null) return;
        for(int i = 0; i < mResultBitmaps.length; i++) {
            if(mEditedBitmaps[i] != mResultBitmaps[i] && mResultBitmaps[i] != null) {
                Bitmap bitmap = mEditedBitmaps[i];
                mEditedBitmaps[i] = mResultBitmaps[i].copy(Config.ARGB_8888, true);
                Utils.recycleBitmap(bitmap);
            }
        }
    }

    public static void updateEditBitmaps(Bitmap[] bitmaps) {
        mEditedBitmaps = bitmaps;
    }

    public static void initBitmaps(Bitmap[] bitmaps, int size) {
        if(UiUtils.highMemo()) {
            mOriginBitmaps = bitmaps;
        } else {
            mOriginBitmaps = new Bitmap[size];
            mResultBitmaps = bitmaps;
        }
        mOriginJpegDataList = new ArrayList<byte[]>(size);
        for(int i=0;i<size;i++) {
            mOriginJpegDataList.add(ImageEditOperationUtil.transformBitmapToBuffer(bitmaps[i]));
        }
    }

    public static Bitmap[] getOriBitmaps() {
        return mOriginBitmaps;
    }

    public static Bitmap[] getEditBitmaps() {
        return mEditedBitmaps;
    }

    public static Bitmap[] getResultBitmaps() {
        return mResultBitmaps;
    }

    public static ArrayList<byte[]> getOriginJpegDataList() {
        return mOriginJpegDataList;
    }

    public static Bitmap[] getOriginBitmapByJpeg() {
        if(mOriginJpegDataList == null){
            return null;
        }
        int length = mOriginJpegDataList.size();
        Bitmap[] bitmaps = new Bitmap[length];
        for(int i=0;i < length ;i++) {
            bitmaps[i] = Utils.transformBufferToBitmap(mOriginJpegDataList.get(i));
        }
        return bitmaps;
    }

    public static void addOriginJpegData(byte[] data) {
        if (mOriginJpegDataList != null) {
            mOriginJpegDataList.add(data);
        }
    }

    public static void recycleEditBitmaps(boolean needReset) {
        if (mEditedBitmaps == null) {
            return ;
        }
        for (int i = 0; i < mEditedBitmaps.length; i++) {
            if (mOriginBitmaps == null || // SPRD:fix bug536950 Attempt to get length of null array
                    i >= mOriginBitmaps.length || mEditedBitmaps[i] != mOriginBitmaps[i]) {
                Utils.recycleBitmap(mEditedBitmaps[i]);
            }
        }

        if(needReset) {
            mEditedBitmaps = null;
        }
        System.gc();
    }

    public static void recycleResultBitmaps() {
        if(mResultBitmaps == null) {
            return;
        }
        for (int i = 0; i < mResultBitmaps.length; i++) {
            Utils.recycleBitmap(mResultBitmaps[i]);
        }
        mResultBitmaps = null;
        System.gc();
    }

    public static void recyleAll() {
        recycleEditBitmaps(true);
        recycleResultBitmaps();
        Utils.recycleBitmaps(mOriginBitmaps);
        mOriginBitmaps = null;
        /* SPRD: Fix bug 536577 java.lang.IndexOutOfBoundsException @{ */
        synchronized (mLock) {
            if (mOriginJpegDataList != null) {
                mOriginJpegDataList.clear();
            }
            mOriginJpegDataList = null;
        }
        /* @} */
    }

    /**
     * get the fit bitmap.first compress the bitmap to save memory then cut the
     * compressed bitmap to square.
     *
     * @param path
     *            the bitmap's path .
     * @param fitSize
     *            the size to scale.
     * @return
     */
    private static Bitmap getScaleBitmap(String path, int fitSize) {
        fitSize = Math.min(fitSize, 260);

        BitmapFactory.Options options = Utils.getNativeAllocOptions();
        options.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(path, options);

        options.inJustDecodeBounds = false;
        int be;
        if (options.outHeight <= options.outWidth) {
            be = (int) (options.outHeight / (float) fitSize);
        } else {
            be = (int) (options.outWidth / (float) fitSize);
        }
        if (be <= 0)
            be = 1;
        options.inSampleSize = be;
        // Bitmap bmp = BitmapFactory.decodeFile(path, options);
        Bitmap bmp = null;
        /* SPRD:  CID 110969 : Resource leak (RESOURCE_LEAK) @{ */
        FileInputStream in = null;
        try {
            in = new FileInputStream(path);
            bmp = BitmapFactory.decodeStream(in, null, options);
        } catch (FileNotFoundException e) {
            Log.w(TAG, "The path " + path + " is not found.", e);
        } catch (OutOfMemoryError oom) {
            Log.w(TAG, "OOM occured when decode the file.", oom);
        }

        try {
            if (in != null) {
                in.close();
              }
        } catch ( java.io.IOException e) {
            Log.w(TAG, "FileInputStream close error.", e);
        }

        /**
        try {
            in = new FileInputStream(path);
            bmp = BitmapFactory.decodeStream(in, null, options);
        } catch (FileNotFoundException e) {
            Log.w(TAG, "The path " + path + " is not found.", e);
        } catch (OutOfMemoryError oom) {
            Log.w(TAG, "OOM occured when decode the file.", oom);
        }
         */
        /* @} */
        if (bmp != null) {
            int widthCut = Math.min(bmp.getWidth(), bmp.getHeight());
            int x = widthCut < bmp.getWidth() ? (bmp.getWidth() - widthCut) / 2 : 0;
            int y = widthCut < bmp.getHeight() ? (bmp.getHeight() - widthCut) / 2 : 0;
            float scale = (float) fitSize / widthCut;
            Matrix matrix = new Matrix();
            matrix.setScale(scale, scale);

            Bitmap tmp;
            tmp = Bitmap.createBitmap(bmp, x, y, widthCut, widthCut, matrix, true);
            /*
             * FIX BUG: 1671 BUG CAUSE: Recycle needs to be used in Bitmap. FIX
             * COMMENT: If the new Bitmap is same as the original bitmap, will
             * not recycle the original bitmap. DATE: 2012-10-11
             */
            if (tmp != null && !tmp.sameAs(bmp)) {
                Utils.recycleBitmap(bmp);
            }
            bmp = tmp;

            /*
             * FIX BUG: 6683 BUG CAUSE: the degree is wrong FIX COMMENT: get the
             * degree from exif and rotate it. DATE: 2011-12-09
             */
            int degree = Utils.getExifOrientation(path);
            if (degree != 0) {
                bmp = Utils.rotate(bmp, degree);
            }
        }
        return bmp;
    }
}
