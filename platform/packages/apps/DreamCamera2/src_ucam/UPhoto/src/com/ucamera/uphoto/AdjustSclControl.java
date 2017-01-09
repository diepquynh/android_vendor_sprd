/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.animation.Animation;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

public class AdjustSclControl extends ImageViewTouchBase
implements SeekBar.OnSeekBarChangeListener{
    private String TAG = "AdjustSclControl";
    private Bitmap mBitmap;

    private SeekBar mContrastSeekBar;
    private SeekBar mSaturationSeekBar;
    private SeekBar mLightSeekBar;
    /* SPRD: CID 109195 : UrF: Unread field (FB.URF_UNREAD_FIELD) @{
    private TextView mContrastPercentTextView;
    private TextView mSaturationPercentTextView;
    private TextView mLightPercentTextView;
    @} */
    private RelativeLayout mAdjustSclLayout;
    private int mAdjustType = ImageEditConstants.NOTHING_ADJUST;
    private Handler mHandler = null;
    private Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private int mContrastProgress = 50;
    private int mSaturationProgress = 50;
    private int mLightProgress = 50;
    private ImageEditDesc mImageEditDesc;
    private int mCenterWidth = 0;
    private int mCenterHeight = 0;
    private boolean mStartTracking = false;
    private ColorMatrix mColorMatrix = new ColorMatrix();

    public AdjustSclControl(Context context) {
        super(context);
    }

    public void initControl(Bitmap bitmap, Activity activity, Handler handler, ImageEditDesc imageEditDesc) {
        mBitmap = bitmap;
        mHandler = handler;
        mImageEditDesc = imageEditDesc;
        mAdjustSclLayout = (RelativeLayout)activity.findViewById(R.id.adjust_scl);

        mContrastSeekBar = (SeekBar)activity.findViewById(R.id.contrast_seek_bar);
        mSaturationSeekBar = (SeekBar)activity.findViewById(R.id.saturation_seek_bar);
        mLightSeekBar = (SeekBar)activity.findViewById(R.id.light_seek_bar);
        mContrastSeekBar.setOnSeekBarChangeListener(this);
        mSaturationSeekBar.setOnSeekBarChangeListener(this);
        mLightSeekBar.setOnSeekBarChangeListener(this);

        /* SPRD: CID 109195 : UrF: Unread field (FB.URF_UNREAD_FIELD) @{
        mContrastPercentTextView = (TextView) activity.findViewById(R.id.contrast_percent);
        mSaturationPercentTextView = (TextView) activity.findViewById(R.id.saturation_percent);
        mLightPercentTextView = (TextView) activity.findViewById(R.id.light_percent);
        @} */
    }

    @Override
    protected boolean setFrame(int l, int t, int r, int b) {
        mCenterWidth = r;
        mCenterHeight = b;
        return super.setFrame(l, t, r, b);
    }

    public void invisibleAdjustAndSaveBitmap() {
        if (mStartTracking) {// fix bug 29096
//            saveBitmap();
        }
        adjustControlVisibility(false);
    }
    public void saveBitmap(boolean isUpdate) {
        Bitmap bitmap = null;
        //fixed the bug31682
        if (mBitmap != null && mStartTracking) {
            do {
                try {
                    bitmap = ImageEditOperationUtil.getDstBitmap(mBitmap, null, mPaint, null);
                } catch(OutOfMemoryError oom) {
                    Log.w(TAG, "saveBitmap(): code has a memory leak is detected.");
                }
                if(bitmap == null) {
                    if(mImageEditDesc.reorganizeQueue() < 2) {
                        return;
                    }
                }
            } while(bitmap == null);
            if(isUpdate) {
                mImageEditDesc.setUpdateBitmap(bitmap);
            } else {
                mImageEditDesc.updateBitmap(getContext(), bitmap);
            }
            /**
             * FIX BUG: 6862
             * BUG CAUSE: when the operation is complete, the value of mStartTracking is not recovered
             * FIX COMMENT: the current operation is complete, restore the value of the mStartTracking
             * DATE: 2012-01-19
             */
            mStartTracking = false;
        }
    }

    public void addBitmap() {
        try{
            Bitmap bitmap = ImageEditOperationUtil.getDstBitmap(mBitmap, null, mPaint, null);
            mImageEditDesc.updateBitmap(getContext(), bitmap);
        }catch(OutOfMemoryError oom){
            Log.w(TAG, "saveBitmap(): code has a memory leak is detected.");
        }
    }

    public void adjustControlVisibility(boolean bVisible) {
        int nVisible = bVisible ? View.VISIBLE : View.GONE;
        mAdjustSclLayout.setVisibility(nVisible);
        if(bVisible && mAnimation != null){
            mAdjustSclLayout.startAnimation(mAnimation);
        }
        if (!bVisible) {
            mAdjustType = ImageEditConstants.NOTHING_ADJUST;
        }
    }

    Animation mAnimation = null;
    public void setToneAnimation(Animation animation) {
        mAnimation = animation;
    }

    public void setProcessBitmap(Bitmap bitmap) {
        mBitmap = bitmap;
    }

    public void setAdjustType(int nAdjustType) {
        if (nAdjustType != mAdjustType) {
            //restoreSeekBar();
        }
        mAdjustType = nAdjustType;
        mStartTracking = false;
    }

    public void restoreSeekBar(){
        mContrastSeekBar.setProgress(50);
        mSaturationSeekBar.setProgress(50);
        mLightSeekBar.setProgress(50);
        //mContrastPercentTextView.setText(transformToText(50));
        //mSaturationPercentTextView.setText(transformToText(50));
        //mLightPercentTextView.setText(transformToText(50));
        mContrastProgress = 50;
        mSaturationProgress = 50;
        mLightProgress = 50;
    }

    private void setColorParam(ColorMatrix cm, float contrast, float light, float saturation) {
        float scale = contrast + 1.f;

        final float invSat = 1 - saturation;
        final float R = 0.213f * invSat;
        final float G = 0.715f * invSat;
        final float B = 0.072f * invSat;

        cm.set(new float[] {
            (R + saturation) * scale,  G,                       B,                       0, light,
             R,                       (G + saturation) * scale, B,                       0, light,
             R,                        G,                      (B + saturation) * scale, 0, light,
             0,                        0,                       0,                       1,  0 });
    }

    private void setLight(ColorMatrix cm, float light) {
        int scale = 1;
        cm.set(new float[] {
               scale, 0, 0, 0, light,
               0, scale, 0, 0, light,
               0, 0, scale, 0, light,
               0, 0, 0, scale, 0 });
    }

    private float getContrast() {
        int nProgress = mContrastProgress;//fix bug 28783
        if (nProgress > 60) {
            return (nProgress - 60)/60.f; // 0~0.67
        }
        else {
            return (nProgress - 60)/90.f; // -0.67 ~ 0
        }

    }

    private float getLight() {
        return (mLightProgress - 50); // -50~50
    }

    private float getSaturation() {
        return (mSaturationProgress)/50.f; // 0~2
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress,boolean fromUser) {
        int selectedId = seekBar.getId();
        switch(selectedId){
        case R.id.contrast_seek_bar:
            mContrastProgress = progress;
            //mContrastPercentTextView.setText(transformToText(progress));
            setAdjustType(ImageEditConstants.CONTRAST_ADJUST);
            break;
        case R.id.light_seek_bar:
            mLightProgress = progress;
            //mLightPercentTextView.setText(transformToText(progress));
            setAdjustType(ImageEditConstants.LIGHT_ADJUST);
            break;
        case R.id.saturation_seek_bar:
            mSaturationProgress = progress;
            //mSaturationPercentTextView.setText(transformToText(progress));
            setAdjustType(ImageEditConstants.SATURATION_ADJUST);
            break;
        }
        if (mHandler != null) {
            mHandler.sendEmptyMessage(ImageEditConstants.ADJUST_BITMAP);
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        mStartTracking = true;
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        /**
         * FIX BUG: 323
         * BUG CAUSE: the result of the current process, will not be saved into the stack
         * FIX COMMENT: save the current bitmap into stack, and then to continue operating to update the top of the stack
         * Date: 2012-01-05
         */
        Bitmap topBitmap = mImageEditDesc.getBitmap();
        if(mBitmap != null && !mBitmap.isRecycled() && topBitmap != null && !topBitmap.isRecycled()) {
            boolean isUpdate = false;
            if(!mBitmap.equals(topBitmap)) {
                isUpdate = true;
            }
            saveBitmap(isUpdate);
        }
    }

    public void setImageBitmap(Bitmap bitmap) {
        if (bitmap != null) {
            mBitmap = bitmap;
        }
        /**
         * FIX BUG: 6119
         * FIX COMMENT: the bitmap has been rotateed
         * Date: 2014-03-28
         */
        super.setImageBitmap(mBitmap/*, mImageEditDesc.getRotation()*/);
    }

    public void onDraw(Canvas canvas) {
        Paint paint = mPaint;
        mColorMatrix.reset();

        setColorParam(mColorMatrix, getContrast(), getLight(), getSaturation());
        paint.setColorFilter(new ColorMatrixColorFilter(mColorMatrix));

        /**
         * FIX BUG: 815
         * BUG CAUSE: When the calculation bitmap display area, use of a scaling algorithm
         * FIX COMMENT: Calculate to display area by the size of the bitmap
         * Date: 2012-04-11
         */
        //RectF rc = ImageEditOperationUtil.resizeRectF(mBitmap, mCenterWidth, mCenterHeight, false);
        /**
         * FIX BUG: 1079 1122 1449 4708
         * BUG CAUSE: mBitmap may be NULL/ mImageEditDesc.getbitmap() my be recycled.
         *            This is Google's issue on the 4.0.2 version, the problem is in the downstream
         *            GLES20Canvas.drawBitmap() function.
         * FIX COMMENT: avoid NullPointerException / avoid to use recycled bitmap / Use Rect object instead of null;avoid picture size change when click tone
         * Date: 2012-04-11 / 2012-08-03 / 2012-08-13 / 2013-09-10
         */
        if(mBitmap != null && mBitmap.isRecycled()) {
            if(mImageEditDesc.getBitmap() != null && !mImageEditDesc.getBitmap().isRecycled()) {
                RectF rc = ImageEditOperationUtil.resizeRectF(mBitmap, mCenterWidth, mCenterHeight, true);
                canvas.drawBitmap(mImageEditDesc.getBitmap(), srcRect(mImageEditDesc.getBitmap()), rc, paint);
            }
        } else if(mBitmap != null && !mBitmap.isRecycled()) {
            /**
             * FIX BUG: 5795
             * FIX COMMENT: set the correct matrix position of mBitmap
             * Date: 2014-01-15
             */
//            RectF rc = ImageEditOperationUtil.resizeRectF(mBitmap, mCenterWidth, mCenterHeight, true);
//            canvas.drawBitmap(mBitmap, srcRect(mBitmap), rc, paint);
            canvas.drawBitmap(mBitmap, getImageMatrix(), mPaint);
        }
    }

    private Rect srcRect(Bitmap bitmap) {
        return new Rect(0, 0, bitmap.getWidth(), bitmap.getHeight());
    }

    /**
     * adjust whether the seekbar value has been changed
     *
     * @return true if seekbar value is changed, else false
     */
    public boolean getIsChangeSeekbarValue() {
        return mStartTracking;
    }

    private String transformToText(int percent) {
        try {
            return Integer.toString(percent);
        } catch(NumberFormatException e) {
            return "0";
        }
    }
}
