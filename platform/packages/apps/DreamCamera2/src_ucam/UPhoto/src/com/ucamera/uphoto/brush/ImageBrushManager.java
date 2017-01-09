/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.util.Log;

import com.ucamera.uphoto.R;

import java.util.ArrayList;

public class ImageBrushManager {
    private static final String TAG = "ImageBrushManager";
    private Context mContext;
    private ImageBrushInfo[] mImageBrushInfoList;
    private int[] mPixels;
    public int mCurrentStyle;
    public int mCurrentSize;
    public int mCurrentColor;
    public Bitmap mCurrentBtimap;
    public int mWidth;
    public int mHeight;
    private ArrayList<ImageBrushItem> mRawBtimapList = null;

    public ImageBrushManager(Context context) {
        this.mContext = context;
        this.mPixels = new int[10000];

        mImageBrushInfoList = new ImageBrushInfo[12];

        ImageBrushInfo brushInfo00 = new ImageBrushInfo(BrushConstant.RectThinBrush, R.drawable.brush_rect_thin, 50, 10, 5, 1.5F);
        mImageBrushInfoList[0] = brushInfo00;

        ImageBrushInfo brushInfo01 = new ImageBrushInfo(BrushConstant.RectDenseBrush, R.drawable.brush_rect_dense, 50, 10, 5, 1.8F);
        mImageBrushInfoList[1] = brushInfo01;

        ImageBrushInfo brushInfo02 = new ImageBrushInfo(BrushConstant.RectFilamentousBrush, R.drawable.brush_rect_filamentous, 50, 10, 5, 2.5F);
        mImageBrushInfoList[2] = brushInfo02;

        ImageBrushInfo brushInfo03 = new ImageBrushInfo(BrushConstant.RectBlurBrush, R.drawable.brush_rect_blur, 50, 10, 5, 2.5F);
        mImageBrushInfoList[3] = brushInfo03;

        ImageBrushInfo brushInfo04 = new ImageBrushInfo(BrushConstant.RectThornsBrush, R.drawable.brush_rect_thorns, 50, 10, 5, 1.3F);
        mImageBrushInfoList[4] = brushInfo04;

        ImageBrushInfo brushInfo05 = new ImageBrushInfo(BrushConstant.RectOilyBrush, R.drawable.brush_rect_oily, 50, 10, 5, 2.5F);
        mImageBrushInfoList[5] = brushInfo05;

        ImageBrushInfo brushInfo06 = new ImageBrushInfo(BrushConstant.RoundMistBrush, R.drawable.brush_round_mist, 50, 10, 5, 1.2F);
        mImageBrushInfoList[6] = brushInfo06;

        ImageBrushInfo brushInfo07 = new ImageBrushInfo(BrushConstant.RoundThinBrush, R.drawable.brush_thin_brush, 50, 10, 5, 1.2F);
        mImageBrushInfoList[7] = brushInfo07;

        ImageBrushInfo brushInfo08 = new ImageBrushInfo(BrushConstant.RoundDenseBrush, R.drawable.brush_round_dense, 50, 10, 5, 2F);
        mImageBrushInfoList[8] = brushInfo08;

        ImageBrushInfo brushInfo09 = new ImageBrushInfo(BrushConstant.OvalBrush, R.drawable.brush_oval, 50, 10, 5, 1.2F);
        mImageBrushInfoList[9] = brushInfo09;

        ImageBrushInfo brushInfo10 = new ImageBrushInfo(BrushConstant.RectMeshBrush, R.drawable.brush_rect_mesh, 50, 10, 5, 1.4F);
        mImageBrushInfoList[10] = brushInfo10;

        ImageBrushInfo brushInfo11 = new ImageBrushInfo(BrushConstant.RectMistBrush, R.drawable.brush_rect_mist, 50, 10, 5, 1.2F);
        mImageBrushInfoList[11] = brushInfo11;

        mRawBtimapList = new ArrayList<ImageBrushItem>();
    }

    private void changeToColor(int brushColor, int width, int height) {
        int red = Color.red(brushColor);
        int green = Color.green(brushColor);
        int blue = Color.blue(brushColor);
        int index = 0;
        for(int i = 0; i < width; i++) {
            for(int j = 0; j < height; j++) {
                int alpha = Color.alpha(mPixels[index]);
                int argb = Color.argb(alpha, red, green, blue);
                mPixels[index] = argb;
                index++;
            }
        }
    }

    private ImageBrushInfo getImageBrushInfo(int brushStyle) {
        int index = 0;
        int count = 0;
        ImageBrushInfo ImageBrushInfo = null;
        if(mImageBrushInfoList != null && mImageBrushInfoList.length > 0) {
            count = mImageBrushInfoList.length;
            do {
                ImageBrushInfo = mImageBrushInfoList[index];
                if(ImageBrushInfo.style == brushStyle) {
                    return ImageBrushInfo;
                }
                index++;
            } while (index < count);

            return ImageBrushInfo;
        }

        return ImageBrushInfo;
    }

    public Bitmap getImageBrushBtimap(int brushStyle, int brushSize, int brushColor) {
        if(brushStyle != mCurrentStyle || brushSize != mCurrentSize || brushColor != mCurrentColor) {
            if(brushStyle == mCurrentStyle) {
                if(brushSize == mCurrentSize) {
                    changeToColor(brushColor, mWidth, mHeight);
                    try {
                        mCurrentBtimap = Bitmap.createBitmap(mPixels, mWidth, mHeight, Bitmap.Config.ARGB_8888);
                    } catch(OutOfMemoryError oom) {
                        Log.w(TAG, "OOM occurred of getImageBrushBtimap to create mCurrentBitmap at first!");
                    }
                    mCurrentColor = brushColor;
                    return mCurrentBtimap;
                }
            }
            /*
             * FIX BUG: 1054
             * BUG CAUSE: May be NullPointerException or OOM;
             * FIX COMMENT:Catch OOM/Prevent NullPointerException;
             * DATE: 2012-08-07
             */
            Bitmap rawBitmap = getRawBtimap(brushStyle);
            if(rawBitmap != null) {
                int rawHeight = rawBitmap.getHeight() * brushSize;
                int rawWidth = rawBitmap.getWidth();
                mWidth = brushSize;
                mHeight = rawHeight / rawWidth;
                /*
                 * FIX BUG: 6030
                 * BUG CAUSE: IllegalArgumentException: width must be > 0
                 * DATE: 2014-03-07
                 */
                if(mWidth <= 0 || mHeight <= 0) {
                    return null;
                }
                Bitmap previewBitmap = null;
                try {
                    previewBitmap = Bitmap.createScaledBitmap(rawBitmap, mWidth, mHeight, true);
                    if(previewBitmap != null) {
                        previewBitmap.getPixels(mPixels, 0, mWidth, 0, 0, mWidth, mHeight);
                        previewBitmap.recycle();
                        previewBitmap = null;
                    }
                } catch(OutOfMemoryError oom) {
                    Log.w(TAG, "OOM occurred of getImageBrushBtimap to create previewBitmap!");
                }
                changeToColor(brushColor, mWidth, mHeight);
            }

            try {
                mCurrentBtimap = Bitmap.createBitmap(mPixels, mWidth, mHeight, Bitmap.Config.ARGB_8888);
            } catch(OutOfMemoryError oom) {
                Log.w(TAG, "OOM occurred of getImageBrushBtimap to create mCurrentBitmap");
                return null;
            }
            mCurrentColor = brushColor;
            mCurrentStyle = brushStyle;
            mCurrentSize = brushSize;
        }
        return mCurrentBtimap;
    }

    private Bitmap getRawBtimap(int brushStyle) {
        int count = mRawBtimapList.size();
        ImageBrushItem ImageBrushItem = null;
        Bitmap bitmap = null;
        boolean existsBitmap = false;
        int index = 0;
        /** FIX BUG: 844
         * BUG CAUSE:  It only compares the first element in the list every time
         * FIX COMMENT: Compares all elements
         * Date: 2012-04-12
         */
        while (index < count) {
            ImageBrushItem = (ImageBrushItem) mRawBtimapList.get(index);
            if(ImageBrushItem.brushStyle == brushStyle) {
                existsBitmap = true;
                break;
            }
            index++;
        }

        if(existsBitmap) {
            bitmap = ImageBrushItem.brushBitmap;
        } else {
            try {
                bitmap = BitmapFactory.decodeResource(mContext.getResources(), getImageBrushInfo(brushStyle).resID);
                ImageBrushItem = new ImageBrushItem(brushStyle, bitmap);
                mRawBtimapList.add(ImageBrushItem);
            } catch (OutOfMemoryError oom) {
                Log.w(TAG, "OOM occurred of getRawBtimap!");
            }
        }

        return bitmap;
    }

    public float getAlphaScale(int brushStyle) {
        return getImageBrushInfo(brushStyle).alphaScale;
    }

    public int getMaxSize(int brushStyle) {
        return getImageBrushInfo(brushStyle).maxSize;
    }

    public int getMinSize(int brushStyle) {
        return getImageBrushInfo(brushStyle).minSize;
    }

    public int getSpacing(int brushStyle) {
        return getImageBrushInfo(brushStyle).spacing;
    }

    class ImageBrushInfo {
        public int style;
        public int resID;
        public int maxSize;
        public int minSize;
        public int spacing;
        public float alphaScale;

        public ImageBrushInfo(int style, int resID, int maxSize, int minSize, int spacing, float alphaScale) {
            this.style = style;
            this.resID = resID;
            this.maxSize = maxSize;
            this.minSize = minSize;
            this.spacing = spacing;
            this.alphaScale = alphaScale;
        }
    }

    class ImageBrushItem {
        public int brushStyle;
        public Bitmap brushBitmap;

        public ImageBrushItem(int brushStyle, Bitmap brushBitmap) {
            this.brushStyle = brushStyle;
            this.brushBitmap = brushBitmap;
        }
    }
}
