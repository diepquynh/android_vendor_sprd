/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.crop;

import android.graphics.Bitmap;

public class CropImageBitmapConstant {
    private static Bitmap mCropBitmap;
    public static void setCropBitmap(Bitmap bitmap) {
        if(mCropBitmap != null && !mCropBitmap.isRecycled()) {
            mCropBitmap.recycle();
        }
        mCropBitmap = bitmap;
    }
    public static Bitmap getCropBitmap() {
        return mCropBitmap;
    }
    public static void clear() {
        if(mCropBitmap != null && !mCropBitmap.isRecycled()) {
            mCropBitmap.recycle();
        }
        mCropBitmap = null;
    }
}
