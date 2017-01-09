/**
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ugallery.gif;

import java.io.FileNotFoundException;
import java.io.InputStream;

import android.net.Uri;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.content.ContentResolver;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Bitmap;
import android.graphics.Paint;
import android.graphics.RectF;

import com.ucamera.ugallery.GifPlayerActivity;

public class GifView extends View implements Runnable {
    private Bitmap mBitmap;

    private Bitmap mBitmapReserved;

    private GifDecoder mDecoder;

    private int mGifCount;

    private Handler mHandler;

    private static final String TAG = "GifView";

    private int mPlaySpeed = 300;
    private boolean mIsDestroyed;
    /**
     *
     * @param context context
     * @param gifUri gifUri
     * @param handler handler
     */
    public GifView(Context context, Uri gifUri, Handler handler) {
        super(context);

        mHandler = handler;
        mDecoder = new GifDecoder();
        ContentResolver cr = context.getContentResolver();
        try {
            InputStream inputStream = cr.openInputStream(gifUri);
            mDecoder.readQPhoneContent(inputStream);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }

        mGifCount = mDecoder.getQPhoneFrameCount();
        Log.d(TAG, mGifCount + " frames in gif file");
        mBitmap = mDecoder.getQPhoneFrame(0);
        // mBitmapReserved = mBitmap;
        /* FIX BUG : 4529
         * BUG COMMENT : get gif speed value by mDecoder object
         * DATE : 2013-07-16
         */
        mPlaySpeed = mDecoder.getQPhoneDelay(1);

        Thread t = new Thread(this);
        t.start();
    }

    /**
     *
     * @param context context
     * @param aSet aSet
     */
    public GifView(Context context, AttributeSet aSet) {
        super(context, aSet);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        try {
            resizeBitmap(canvas);
            mBitmapReserved = mBitmap;
            mBitmap = mDecoder.getQPhoneNext();
            if (mBitmap == null) {
                mBitmap = mBitmapReserved;
            }
            // Log.d(TAG, "mBitmap2: " + mBitmap.toString());
            // Log.d(TAG, "mBitmapReserved2: " + mBitmapReserved.toString());

            // if(mBitmapReserved.equals(mBitmap) == false && mBitmapReserved !=
            // null){
            // Log.d(TAG, "mBitmapReserved.recycle();");
            // mBitmapReserved.recycle();
            // mBitmapReserved = null;
            // }

        } catch (NullPointerException e) {
            // e.printStackTrace();
            mHandler.obtainMessage();
            mHandler.sendEmptyMessage(GifPlayerActivity.MSG_DECODE_GIF_ERROR);

        } catch (Exception e) {
            // e.printStackTrace();
            mHandler.obtainMessage();
            mHandler.sendEmptyMessage(GifPlayerActivity.MSG_DECODE_GIF_ERROR);
        }
    }

    protected void resizeBitmap(Canvas canvas) {
        float gifWidth = mBitmap.getWidth();
        float gifHeight = mBitmap.getHeight();
        float windowWidth = getWidth();
        float windowHeight = getHeight();
        float scaleWidth;
        float scaleHight;
        float left;
        float top;
        double scale;

        if(gifWidth > windowWidth || gifHeight > windowHeight){
            if((gifWidth/windowWidth) <= (gifHeight/windowHeight)){
                scale = gifHeight/windowHeight;
                scaleWidth = (float) (gifWidth / scale);
                left = (windowWidth-scaleWidth)/2;
                scaleWidth = left + scaleWidth;
                canvas.drawBitmap(mBitmap, null, new RectF(left, 0, scaleWidth, windowHeight), new Paint());
            }else{
                scale = gifWidth / windowWidth;
                scaleHight = (float) (gifHeight/scale);
                top = (windowHeight-scaleHight)/2;
                scaleHight = top + scaleHight;
                canvas.drawBitmap(mBitmap, null, new RectF(0, top, windowWidth, scaleHight), new Paint());
            }
        }else{
            scale = Math.min(windowWidth / gifWidth, windowHeight / gifHeight);
            scale = scale > 4 ? 2: 1;
            top = (windowHeight-(float)(gifHeight*scale)) / 2;
            left = (windowWidth-(float)(gifWidth*scale)) / 2;
            canvas.drawBitmap(mBitmap,null, new RectF(left,top,windowWidth-left,windowHeight-top),new Paint());
        }
    }

    public void setDestroyed(boolean isDestroyed) {
        this.mIsDestroyed = isDestroyed;
    }
    /**
     * run
     */
    public void run() {
        while (true) {
            if(mIsDestroyed) {
                return;
            }
            try {
                this.postInvalidate();
                Thread.sleep(mPlaySpeed);

            } catch (InterruptedException e) {
                e.printStackTrace();

            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * recycleGifFrame
     */
    public void recycleGifFrame() {
        Log.d(TAG, "recycleGifFrame");
        for (int i = 0; i < mGifCount; i++) {
            Bitmap bitmap = mDecoder.getQPhoneFrame(i);
            if (bitmap != null) {
                bitmap.recycle();
                bitmap = null;
            }
        }
    }
}