/*
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ucam.modules.ugif;

import com.ucamera.ucam.modules.compatible.Models;
import com.ucamera.ucam.modules.utils.UiUtils;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PaintFlagsDrawFilter;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.RelativeLayout.LayoutParams;

public class GifView extends View implements Runnable {
    private static final String TAG = "GifView";
    private int mCount = 0;
    private int mPlayIndex = 0;
    private Bitmap[] mPlayBmps;
    private boolean mIsPlaying = false;
    private int mPlaySpeed = 300;
    private RectF mRc;
    private int[] mValidIndex;
    private boolean mIsLocalGif;
    private boolean mDrawing = true;
    private Thread playThread = null;
    private int mViewWidth;
    private int mViewHeight;
    private int mViewPaddingTop;

    public GifView(Context context, AttributeSet a) {
        super(context, a);
    }

    // gif preview or play local gif
    public void init(Bitmap[] bmps, boolean isLocalgif, RectF rc) {
        mIsLocalGif = isLocalgif;
        mPlayIndex = 0;
        mPlayBmps = bmps;
        mRc = rc;
        /*
         * FIX BUG: 6160
         * BUG COMMENT: avoid null pointer exception
         * DATE: 2014-03-26
         */
        if(mPlayBmps != null) {
            mCount = mPlayBmps.length;
        }
        mDrawing = true;
    }

    /*
     * FIX BUG: 5475 5476
     * BUG CAUSE: don't set correct size for GifView
     * FIX COMMETN: calculate GifView size by screen size
     * DATE: 2013-11-29
     */
    public void init(Bitmap[] bmps, boolean isLocalgif) {
        if(UiUtils.screenDensity() == 1) {
            mViewWidth = UiUtils.screenWidth() * 214 / 320;
            mViewHeight = UiUtils.screenHeight() * 214 / 480;
            mViewPaddingTop = UiUtils.screenHeight() * 145 / 480;
        }else {
            mViewWidth = UiUtils.screenWidth() * 358 / 480;
            mViewHeight = UiUtils.screenHeight() * 345 / 800;
            mViewPaddingTop = UiUtils.screenHeight() * 200 / 800;
        }
        RectF rc = new RectF(0, 0, mViewWidth, mViewHeight);
        init(bmps, isLocalgif, rc);
        setViewSize();
    }

    private void setViewSize() {
        LayoutParams params = (LayoutParams) getLayoutParams();
        params.width = mViewWidth;
        params.height = mViewHeight;
        params.setMargins(0, mViewPaddingTop, 0, 0);
        setLayoutParams(params);
    }

    public void setSpeed(int speed) {
        mPlaySpeed = speed;
    }

    public void setValidIndex(int[] index) {
        mValidIndex = index;
        mCount = mValidIndex.length;
        mPlayIndex = 0;
        mDrawing = true;
    }

    public void stopDrawing() {
        mDrawing = false;
    }

    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        // first show pic one
        canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG
                | Paint.FILTER_BITMAP_FLAG)); // remove sawtooth
        Bitmap bmp = null;
        if (mDrawing && mCount > 0) {
            /*
             * FIX BUG: 3012
             * FIX COMMENT: After drawn each frame completion, the index value add 1;
             * DATE: 2012-03-06
             */
            if (mPlayIndex >= mCount)
                mPlayIndex = 0;
            if (!mIsPlaying && !mIsLocalGif) {
                bmp = mPlayBmps[mValidIndex[0]];
                // play gif
                /**
                 * FIX BUG: 126 BUG CAUSE: the count is beyond the bmps's size.
                 * FIX COMMENT: limit the count in the valid range Date:
                 * 2011-12-09
                 */
            } else if (mPlayIndex >= 0 && mPlayIndex <= mCount - 1 && mIsPlaying) {
                if (mIsLocalGif) {
                    bmp = mPlayBmps[mPlayIndex];
                } else {
                    bmp = mPlayBmps[mValidIndex[mPlayIndex]];
                }
            } else {
                Log.e(TAG, "The index is out of range, the index is " + mPlayIndex
                        + " and the count is " + mCount);
            }
            if (bmp != null && !bmp.isRecycled()) {
                try {
                    canvas.drawBitmap(bmp, null, mRc, null);
                } catch (Throwable e) {
                    Log.e(TAG, "Error occured while GifView is playing!");
                }
            }
            mPlayIndex++;
        }
    }
    public void stop() {
        if (mIsPlaying) {
            Thread tempThread = playThread;
            playThread = null; // destroy the thread
            tempThread.interrupt();
            mPlayIndex = 0;
            try {
                postInvalidate();
            } catch (NullPointerException e) {

            }
            mIsPlaying = false;
        }

    }

    public void start() {
        if (!mIsPlaying) {
            mIsPlaying = true;
            mPlayIndex = 0;
            playThread = new Thread(this);
            playThread.start();
        }
    }

    public boolean isPlaying() {
        return mIsPlaying;
    }

    public void run() {
        while (!Thread.currentThread().isInterrupted() && playThread == Thread.currentThread()) {
            /*
             * FIX BUG: 1443
             * FIX COMMENT: unknown reason this method raise NullpointException just catch that exception and break Date:
             * DATE: 2012-08-10
             */
            if(!mIsPlaying) return;
            try {
                postInvalidate();
            } catch (NullPointerException e) {
                break;
            }

            try {
                Thread.sleep(mPlaySpeed);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
    }
}
