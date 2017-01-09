/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.camera;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.widget.ImageView;
import android.graphics.Xfermode;
import android.graphics.AvoidXfermode;
import android.graphics.Color;
import android.view.View;
import android.graphics.Bitmap;

import com.android.camera2.R;

import android.graphics.drawable.Drawable;
import android.graphics.drawable.BitmapDrawable;

import com.android.camera.debug.Log;

/**
 *
class PanoProgressBar extends ImageView {
 */
public class PanoProgressBar extends ImageView {
    @SuppressWarnings("unused")
    private static final Log.Tag TAG = new Log.Tag("PanoProgressBar");
    public static final int DIRECTION_NONE = 0;
    public static final int DIRECTION_LEFT = 1;
    public static final int DIRECTION_RIGHT = 2;
    private float mProgress = 0;
    private float mMaxProgress = 0;
    private float mLeftMostProgress = 0;
    private float mRightMostProgress = 0;
    private float mProgressOffset = 0;
    private float mIndicatorWidth = 0;
    private int mDirection = 0;
    private final Paint mBackgroundPaint = new Paint();
    private final Paint mDoneAreaPaint = new Paint();
    private final Paint mIndicatorPaint = new Paint();
    private float mWidth;
    private float mHeight;
    private RectF mDrawBounds;
    private OnDirectionChangeListener mListener = null;
    private Context mContext;

    public interface OnDirectionChangeListener {
        public void onDirectionChange(int direction);
    }

    public PanoProgressBar(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        mDoneAreaPaint.setStyle(Paint.Style.FILL);
        mDoneAreaPaint.setAlpha(0xff);

        mBackgroundPaint.setStyle(Paint.Style.FILL);
        mBackgroundPaint.setAlpha(0xff);

        mIndicatorPaint.setStyle(Paint.Style.FILL);
        mIndicatorPaint.setAlpha(0xff);

        mDrawBounds = new RectF();

        topx = -dip2px(mContext, 7);
        topy = -dip2px(mContext, 30);
        bottomx = -dip2px(mContext, 7);
        bottomy =  dip2px(mContext, 18);
        rectWidth = dip2px(mContext, 110);
        rectHeight = dip2px(mContext, 40);
    }

    public void setOnDirectionChangeListener(OnDirectionChangeListener l) {
        mListener = l;
    }

    private void setDirection(int direction) {
        if (mDirection != direction) {
            mDirection = direction;
            if (mListener != null) {
                mListener.onDirectionChange(mDirection);
            }
            invalidate();
        }
    }

    public int getDirection() {
        return mDirection;
    }

    @Override
    public void setBackgroundColor(int color) {
        mBackgroundPaint.setColor(color);
        invalidate();
    }

    public void setDoneColor(int color) {
        mDoneAreaPaint.setColor(color);
        invalidate();
    }

    public void setIndicatorColor(int color) {
        mIndicatorPaint.setColor(color);
        invalidate();
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        mWidth = w;
        mHeight = h;
        mDrawBounds.set(0, 0, mWidth, mHeight);
    }

    public void setMaxProgress(int progress) {
        mMaxProgress = progress;
    }

    public void setIndicatorWidth(float w) {
        mIndicatorWidth = w;
        invalidate();
    }

    public void setRightIncreasing(boolean rightIncreasing) {
        if (rightIncreasing) {
            mLeftMostProgress = 0;
            mRightMostProgress = 0;
            mProgressOffset = 0;
            setDirection(DIRECTION_RIGHT);
        } else {
            mLeftMostProgress = mWidth;
            mRightMostProgress = mWidth;
            mProgressOffset = mWidth;
            setDirection(DIRECTION_LEFT);
        }
        invalidate();
    }

    public void setProgress(int progress) {
        // The panning direction will be decided after user pan more than 10 degrees in one
        // direction.
        if (mDirection == DIRECTION_NONE) {
            if (progress > 10) {
                setRightIncreasing(true);
            } else if (progress < -10) {
                setRightIncreasing(false);
            }
        }
        // mDirection might be modified by setRightIncreasing() above. Need to check again.
        if (mDirection != DIRECTION_NONE) {
            mProgress = progress * mWidth / mMaxProgress + mProgressOffset;
            // value bounds.
            mProgress = Math.min(mWidth, Math.max(0, mProgress));
            if (mDirection == DIRECTION_RIGHT) {
                // The right most progress is adjusted.
                mRightMostProgress = Math.max(mRightMostProgress, mProgress);
            }
            if (mDirection == DIRECTION_LEFT) {
                // The left most progress is adjusted.
                mLeftMostProgress = Math.min(mLeftMostProgress, mProgress);
            }
            invalidate();
        }
    }

    public void reset() {
        mProgress = 0;
        mProgressOffset = 0;
        setDirection(DIRECTION_NONE);
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if(canvas.isHardwareAccelerated()){
            this.setLayerType(View.LAYER_TYPE_SOFTWARE, null);
        }
        // the background
        canvas.drawRect(mDrawBounds, mBackgroundPaint);
        if (mDirection != DIRECTION_NONE) {
            // the progress area
            canvas.drawRect(mLeftMostProgress, mDrawBounds.top, mRightMostProgress,
                    mDrawBounds.bottom, mDoneAreaPaint);
            // the indication bar
            float l;
            float r;
            if (mDirection == DIRECTION_RIGHT) {
                l = Math.max(mProgress - mIndicatorWidth, 0f);
                r = mProgress;
            } else {
                l = mProgress;
                r = Math.min(mProgress + mIndicatorWidth, mWidth);
            }
            canvas.drawRect(l, mDrawBounds.top, r, mDrawBounds.bottom, mIndicatorPaint);
        }


        RectF topRectF = new RectF();
        topRectF.set(0, 0, rectWidth, rectHeight);

        // paint for oval
        Paint avoidXfermodePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        PorterDuffXfermode porterDuffXfermode = new PorterDuffXfermode(PorterDuff.Mode.CLEAR);
        avoidXfermodePaint.setARGB(0,0,0,0);
        avoidXfermodePaint.setXfermode(porterDuffXfermode);

        // top
        canvas.save();
        canvas.translate(topx, topy);
        canvas.drawOval(topRectF, avoidXfermodePaint);
        canvas.restore();

        // bottom
        canvas.save();
        canvas.translate(bottomx, bottomy);
        canvas.drawOval(topRectF, avoidXfermodePaint);
        canvas.restore();

//      super.onDraw(canvas);
//        Paint avoidXfermodePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
//        AvoidXfermode avoidXfermode = new AvoidXfermode(Color.BLACK, 10, AvoidXfermode.Mode.TARGET);

//        mPaint.setAlpha(0x00);
//        avoidXfermodePaint.setARGB(0,0,0,0);
//        avoidXfermodePaint.setXfermode(avoidXfermode);
//        canvas.drawRect(mDrawBounds, avoidXfermodePaint);

        // ui check 109
        // because there is always a sawtooth, add a line bitmap to perfect it.
//        Paint linePaint = new Paint();
//        Drawable lineDrawable = mContext.getResources().getDrawable(
//                R.drawable.ic_pan_progression_line);
//        BitmapDrawable bd = (BitmapDrawable) lineDrawable;
//        Bitmap bitmap = bd.getBitmap();
//        canvas.drawBitmap(bitmap, mDrawBounds.left, mDrawBounds.top, linePaint);

    }

    float topx = 0;
    float topy = 0;
    float bottomx = 0;
    float bottomy =  0;
    float rectWidth = 0;
    float rectHeight = 0;

    public static int dip2px(Context context, float dpValue) {

        final float scale = context.getResources().getDisplayMetrics().density;

        return (int) (dpValue * scale + 0.5f);

    }

}
