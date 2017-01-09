/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import java.io.IOException;
import java.io.InputStream;

import com.ucamera.uphoto.mosaic.PaintMosaic;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.Bitmap.Config;
import android.util.AttributeSet;
import android.util.FloatMath;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.GridView;
import android.widget.HorizontalScrollView;
import android.widget.SeekBar;;

public class ImageEditViewMosaic extends ImageEditViewPreview implements SeekBar.OnSeekBarChangeListener{
    private static final String TAG = "ImageEditViewMosaic";
    protected float mX = 0.f;
    protected float mY = 0.f;
    protected final static float MOVE_TOLERANCE = 4.0f;
    private PaintMosaic mMosaicPaint;
    private Bitmap mBitmap;
    private float[] values = new float[9];
    private static final int SCALE = 1;
    private static final int DRAW = 0;
    private int mode;
    private float lastStartX;
    private float lastStartY;
    private float lastEndX;
    private float lastEndY;
    private float oldDist;
    private PointF mid = new PointF();
    private ImageEditViewPreview mPreview;
    private boolean mIsDrawMosaic = false;
    private ImageEditDesc mImageEditDesc;
    private SeekBar mSeekBar;
    private static final int BRUSH_SIZE = 20;
    private GridView mGridView;
    private HorizontalScrollView mScrollView;
    private Bitmap mMosaicBlockBitmap;
    private Bitmap mMosaicBitmap;
    public ImageEditViewMosaic(Context context, AttributeSet attrs) {
        super(context, attrs);
    }
    public ImageEditViewMosaic(Context context, Bitmap bitmap, SeekBar bar, GridView gridView, HorizontalScrollView scrollView) {
        super(context);
        mSeekBar = bar;
        mGridView = gridView;
        mScrollView = scrollView;
        mSeekBar.setOnSeekBarChangeListener(this);
        mImageEditDesc = ImageEditDesc.getInstance();
        init(bitmap);
    }
    public PaintMosaic getMosaicPait() {
        return mMosaicPaint;
    }
    private void init(Bitmap bitmap) {
        /*
         * FIX BUG: 6023
         * BUG CAUSE: nullpointer
         * DATE: 2014-03-07
         */
        if(bitmap == null) {
            return;
        }
        setCertainTouchable(false);
        Bitmap tempBitmap = null;
        do {
            try {
                tempBitmap = Bitmap.createBitmap(bitmap.getWidth(), bitmap.getHeight(), Bitmap.Config.ARGB_8888);
            } catch(OutOfMemoryError oom) {
            }
            if(tempBitmap == null) {
                //index is one, means that any operation can not be Completed
                if(mImageEditDesc.reorganizeQueue() < 2) {
                    return;
                }
            }
        } while (tempBitmap == null);
        Bitmap result = tempBitmap;
        if (!result.isMutable()) {
            result = tempBitmap.copy(Config.ARGB_8888, true);
        }
        /* SPRD: CID 110908 : Resource leak (RESOURCE_LEAK) @{ */
        InputStream stream = getResources().openRawResource(R.raw.mosaic_block);
        mMosaicBlockBitmap = BitmapFactory.decodeStream(stream);
        if (stream != null) {
            try {
                stream.close();
            } catch (IOException e) {
            }
        }
        // mMosaicBlockBitmap = BitmapFactory.decodeStream(getResources().openRawResource(R.raw.mosaic_block));
        /* @} */
        mBitmap = result;
        setImageBitmap(mBitmap);
        mMosaicPaint = new PaintMosaic();
        mMosaicPaint.createCanvas(mBitmap);
    }
    public void setMosaicType(int type) {
        mMosaicPaint.setBrushType(type);
        mMosaicPaint.setBrushSize(mSeekBar.getProgress() + BRUSH_SIZE);
    }
    public Bitmap getMosaicBitmap() {
        mMosaicBitmap = Utils.makeMosaicBitmap(mPreview.getBitmap(), mMosaicBlockBitmap);
        return mMosaicBitmap;
    }
    public void setBottomLayer(ImageEditViewPreview preview) {
        mPreview = preview;
    }
    public void updateMosaicShader(Bitmap bitmap) {
        mMosaicPaint.updateMosaicDrawShader(bitmap);
        invalidate();
    }
    public boolean isDrawMosaic() {
        return mIsDrawMosaic;
    }
    public Bitmap composeGraffitiBitmap(){
        Bitmap bitmap = null;
        Bitmap bottomLayer = mPreview.getBitmap();
        do {
            try {
                Config config = bottomLayer.getConfig();
                bitmap = Bitmap.createBitmap(bottomLayer.getWidth(), bottomLayer.getHeight(), (config != null ? config: Config.ARGB_8888));
            } catch(OutOfMemoryError oom) {
                Log.w(TAG, "composeMosaicBitmap(): code has a memory leak is detected...");
            }
            if(bitmap == null) {
                //index is one, means that any operation can not be Completed
                if(mImageEditDesc.reorganizeQueue() < 2) {
                    return null;
                }
            }
        } while(bitmap == null);

        Matrix matrix = new Matrix();
        Paint p = new Paint();
        Canvas canvas = new Canvas(bitmap);
        canvas.drawBitmap(bottomLayer, matrix, p);
//        RectF rectF = ImageEditOperationUtil.resizeRectF(bottomLayer, getWidth(), getHeight(), false);
//        float width =  rectF.width();
//        float height =  rectF.height();
//        float sx = bottomLayer.getWidth() / width ;
//        float sy = bottomLayer.getHeight() / height;
//        matrix.postScale(sx, sy);
        canvas.drawBitmap(mBitmap, matrix, p);
        mIsDrawMosaic = false;
        return bitmap;
    }
    public void clear() {
        /*
         * SPRD Bug:527652 Recycled a null bitmap var. @{
         * Original ucam code:

        if(mMosaicBlockBitmap != null) {

         */
        if (mMosaicBlockBitmap != null && mMosaicBitmap != null) {
            mMosaicBitmap.recycle();
            mMosaicBitmap = null;
        }
        if(mBitmap != null) {
            mBitmap.recycle();
            mBitmap = null;
        }
        mMosaicPaint.clear();
    }
    private void touchDown(float x, float y) {
        mX = x;
        mY = y;
        mMosaicPaint.paintBegin(x, y);
    }
    private void touchMove(float x, float y) {
        float dx = Math.abs(x - mX);
        float dy = Math.abs(y - mY);
        if (dx >= MOVE_TOLERANCE || dy >= MOVE_TOLERANCE) {
            mMosaicPaint.paintMove(mX, mY, (x + mX) / 2, (y + mY) / 2);
            mX = x;
            mY = y;
        }
    }
    private void touchUp(float x, float y) {
        /* SPRD: CID 109150 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE) @{
        float dx = Math.abs(x - mX);
        float dy = Math.abs(y - mY);
        @} */
        mIsDrawMosaic = true;
        mMosaicPaint.paintEnd(mX, mY);
    }
    private PointF firstPointF = new PointF();
    private PointF secondPointF = new PointF();
    private void hideGridView() {
        if(mGridView.getVisibility() == View.VISIBLE) {
            mGridView.setVisibility(View.GONE);
            mScrollView.setVisibility(View.GONE);
        }
    }
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        hideGridView();
        float x = event.getX();
        float y = event.getY();

        Matrix inverse = new Matrix();
        getImageMatrix().invert(inverse);
        inverse.getValues(values);

        x = x * values[0] + values[2];
        y = y * values[4] + values[5];

        float startX = firstPointF.x;
        float startY = firstPointF.y;
        float endX = event.getX();
        float endY = event.getY();

        switch (event.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_DOWN:
                touchDown(x, y);
                invalidate();
                break;
             case MotionEvent.ACTION_POINTER_DOWN:
               oldDist = spacing(event);
               if (oldDist > 10f) {
                   midPoint(mid, event);
                   mode = SCALE;
               }
               mode = SCALE;
               firstPointF.set(event.getX(0), event.getY(0));
               secondPointF.set(event.getX(1), event.getY(1));
                break;
            case MotionEvent.ACTION_MOVE:
                if(startX == lastStartX && startY == lastStartY
                        && endX == lastEndX && endY == lastEndY){
                    break;
                }
                lastStartX = startX;
                lastStartY = startY;
                lastEndX = endX;
                lastEndY = endY;
                if (mode == SCALE) {
                   if(event.getPointerCount() < 2) {
                       break;
                   }
                   if(getScale() > 1f /*&& isCocurrent(event)*/){
                       firstPointF.set(event.getX(0), event.getY(0));
                       secondPointF.set(event.getX(1), event.getY(1));
                   }
                   float newDist = spacing(event);
                   if (newDist > 10f) {
                       float distance = FloatMath.sqrt(getWidth() * getWidth()
                               + getHeight() * getHeight());
                       float scale = 1 + (newDist - oldDist) / (distance*2/3);
                       if((getScale() < 0.25f && scale <= 1) || getScale() >= 4f && scale >=1) {
                           break;
                       }
                       mPreview.getSuppMatrix().postScale(scale, scale, mid.x, mid.y);
                       mPreview.setImageMatrix(mPreview.getImageViewMatrix());
                       mPreview.center(true, true);

                       getSuppMatrix().postScale(scale, scale, mid.x, mid.y);
                       setImageMatrix(getImageViewMatrix());
                       center(true, true);
                   }
                   oldDist = spacing(event);
               }else{
                   touchMove(x, y);
                   invalidate();
                   break;
               }
                break;
            case MotionEvent.ACTION_UP:
                if(mode == DRAW) {
                    touchUp(x, y);
                    invalidate();
                }
                mode = DRAW;
                break;
            case MotionEvent.ACTION_POINTER_UP:
//                mode = DRAW;
                break;
        }
        return true;
    }
    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        return super.dispatchTouchEvent(event);
    }
    private float spacing(MotionEvent event) {
        if(event.getPointerCount() < 2) {
            return 0;
        }
        float x = event.getX(0) - event.getX(1);
        float y = event.getY(0) - event.getY(1);
        return FloatMath.sqrt(x * x + y * y);
    }

    private void midPoint(PointF point, MotionEvent event) {
        float x = event.getX(0) + event.getX(1);
        float y = event.getY(0) + event.getY(1);
        point.set(x / 2, y / 2);
    }
    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        mMosaicPaint.setBrushSize(progress + BRUSH_SIZE);
    }
    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
    }
    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
    }
}
