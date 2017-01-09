/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.crop;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.util.FloatMath;
import android.view.MotionEvent;
import android.widget.ImageView;

public class OperateCropImageView extends ImageView{
    private Matrix mMatrix = new Matrix();
    private Matrix mTempMatrix = new Matrix();
    PointF mStartPointF = new PointF();
    private HighlightView mHighlightView;
    private enum MODE{NONE,DRAG,SCALE};
    private MODE mMode = MODE.NONE;
    private float mOldDist;
    private float mNewDist;
    private PointF mMidPointF = new PointF();
    private float mTranX = 0;
    private float mTranY = 0;
    private boolean isTouching = false;

    public OperateCropImageView(Context context) {
        super(context);
    }
    public OperateCropImageView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }
    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        mHighlightView.draw(canvas);
    }
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        isTouching = true;
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
        case MotionEvent.ACTION_DOWN :
            mMatrix.set(getImageMatrix());
            mTempMatrix.set(mMatrix);
            mStartPointF.set(event.getX(), event.getY());
            mMode = MODE.DRAG;
            break;
        case MotionEvent.ACTION_POINTER_DOWN :
            if(mMode == MODE.SCALE) {
                break;
            }
            if(mTranX != 0 || mTranY != 0) {
                break;
            }
            mOldDist = calculateDistance(event);
            if (mOldDist > 20f) {
                mTempMatrix.set(mMatrix);
                midPoint(mMidPointF, event);
                mMode = MODE.SCALE;
            }
            break;
        case MotionEvent.ACTION_UP :
        case MotionEvent.ACTION_POINTER_UP :
            if(mMode == MODE.SCALE) {
                if(ensureScale(mMatrix)) {
                    mMatrix.set(mTempMatrix);
                }
            } else if(mMode == MODE.DRAG) {
                if(mTranX != 0 || mTranY != 0) {
                    float[] xy = ensureTranslate();
                    mMatrix.set(mTempMatrix);
                    mMatrix.postTranslate(xy[0], xy[1]);
                }
            }
            mTranX = 0;
            mTranY = 0;
            mMode = MODE.NONE;
            isTouching = false;
            break;
        case MotionEvent.ACTION_MOVE:
            if (mMode == MODE.DRAG) {
                mMatrix.set(mTempMatrix);
                mTranX = event.getX() - mStartPointF.x;
                mTranY = event.getY() - mStartPointF.y;
                mMatrix.postTranslate(event.getX() - mStartPointF.x, event.getY() - mStartPointF.y);
            } else if(mMode == MODE.SCALE) {
                mNewDist = calculateDistance(event);
                mMatrix.set(mTempMatrix);
                if(mNewDist > 20f) {
                    float scale = mNewDist / mOldDist;
                    mMatrix.postScale(scale, scale, mMidPointF.x, mMidPointF.y);
//                    setImageMatrix(mMatrix);
                }
            }
            break;
        }
        setImageMatrix(mMatrix);
        return true;
    }
    private float calculateDistance(MotionEvent event) {
        float x = event.getX(0) - event.getX(1);
        float y = event.getY(0) - event.getY(1);
        return FloatMath.sqrt(x * x + y * y);
    }
    private void midPoint(PointF point, MotionEvent event) {
        float x = event.getX(0) + event.getX(1);
        float y = event.getY(0) + event.getY(1);
        point.set(x / 2, y / 2);
    }
    private boolean ensureScale(Matrix matrix) {
        Rect crop = mHighlightView.getCropRect();
        final float[] offset = {0, 0, getDrawable().getIntrinsicWidth(), getDrawable().getIntrinsicHeight()};
        getImageMatrix().mapPoints(offset);
        if(offset[0] > crop.left || offset[1] > crop.top || offset[2] < crop.right || offset[3] < crop.bottom) {
            return true;
        }
        return false;
    }
    private float[] ensureTranslate() {
        Rect crop = mHighlightView.getCropRect();
        final float[] offset = {0, 0, getDrawable().getIntrinsicWidth(), getDrawable().getIntrinsicHeight()};
        getImageMatrix().mapPoints(offset);
        float[] valuesOld = new float[9];
        mTempMatrix.getValues(valuesOld);
        float[] valuesNew = new float[9];
        mTempMatrix.getValues(valuesNew);
        float transX = 0f;
        float transY = 0f;
        if(offset[0] > crop.left) {
            transX = offset[0] - crop.left;
        }
        if(offset[1] > crop.top) {
            transY = offset[1] - crop.top;
        }
        if(offset[2] < crop.right) {
            transX = offset[2] - crop.right;
        }
        if(offset[3] < crop.bottom) {
            transY = offset[3] - crop.bottom;
        }
        return new float[]{mTranX - transX, mTranY - transY};
    }
    public Bitmap save() {
//        Rect rect = mHighlightView.getCropRect();
//        RectF rect1 = new RectF();
//        mTempMatrix.mapRect(rect1, new RectF(rect));
//        Bitmap m = Bitmap.createBitmap(getDrawable().getIntrinsicWidth(), getDrawable().getIntrinsicHeight(), Config.ARGB_8888);
//        Canvas s = new Canvas(m);
//        s.drawBitmap(((BitmapDrawable)getDrawable()).getBitmap(), getImageMatrix(), new Paint());
//        Bitmap bitmap = Bitmap.createBitmap(rect.width(), rect.height(), Config.ARGB_8888);
//        Canvas canvas = new Canvas(bitmap);
//        canvas.drawBitmap(m, rect, new Rect(0, 0, rect.width(), rect.height()), new Paint());
//        setImageBitmap(bitmap);
//        return bitmap;
        Rect rect = mHighlightView.getCropRect();
        Bitmap m = Bitmap.createBitmap(getWidth(), getHeight(), Config.ARGB_8888);
        Canvas s = new Canvas(m);
        s.drawBitmap(((BitmapDrawable)getDrawable()).getBitmap(), getImageMatrix(), new Paint());
        Bitmap bitmap = Bitmap.createBitmap(rect.width(), rect.height(), Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        canvas.drawBitmap(m, rect, new Rect(0, 0, rect.width(), rect.height()), new Paint());
//        setImageBitmap(bitmap);
        return bitmap;
    }
    public void addHighLightView(HighlightView view) {
        mHighlightView = view;
    }
    public boolean isTouch() {
        return isTouching;
    }
}
