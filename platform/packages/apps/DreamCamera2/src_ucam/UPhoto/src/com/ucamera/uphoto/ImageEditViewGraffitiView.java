/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import com.ucamera.uphoto.brush.BrushPainting;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.Bitmap.Config;
import android.util.FloatMath;
import android.util.Log;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.widget.Toast;

public class ImageEditViewGraffitiView extends ImageEditViewPreview{
    private static final String TAG = "ImageEditViewGraffitiView";

    private Canvas mCanvas;
    private Bitmap mBitmap;
    private Context mContext;
    private GestureDetector mGestureDetector;
    private ImageEditViewPreview preview;
    private boolean mIsSaved;
    private ImageEditDesc mImageEditDesc;
    private float mX, mY;
    private float oldDist;
    private PointF mid = new PointF();
    private static final int DRAW = 0; //normal status
    private static final int MOVE = 1; //zoom in or zoom out
    int mode = DRAW;  //drawing
    private float scale = 0;
    private float lastStartX;
    private float lastStartY;
    private float lastEndX;
    private float lastEndY;

    private PointF firstPointF = new PointF();
    private PointF secondPointF = new PointF();
    boolean mFlag = false;

    private int mWidth;
    private int mHeight;
    private BrushPainting mBrushPainting;
    private float[] values = new float[9];
    private static final float TOUCH_TOLERANCE = 4;
    private int mCanvasColor;

    public ImageEditViewGraffitiView(Context context) {
        super(context);
        mContext = context;
        mImageEditDesc = ImageEditDesc.getInstance();
        mGestureDetector = new GestureDetector(context, new ImageEditGraffitiViewGestureListener());
    }

    public void setGraffitiDimens(final int width, final int height, boolean isAdjust, BrushPainting brushPainting){
        setGraffitiDimens(width, height, brushPainting);
        if(preview != null && isAdjust){
            getSuppMatrix().postScale(preview.getScale(),preview.getScale(), width / 2, height / 2);
            setImageMatrix(getImageViewMatrix());
        }
    }

    public void setGraffitiDimens(final int width, final int height, BrushPainting brushPainting){
        int w;
        int h;
        if(preview != null && preview.getBitmap() != null){
//            RectF rectF = ImageEditOperationUtil.resizeRectF(preview.getBitmap(), width, height, true);
            RectF rectF = ImageEditOperationUtil.resizeRectF(preview.getBitmap(), width, height, false);
            w = (int)rectF.width();
            h = (int)rectF.height();
        }else{
            w = width;
            h = height;
        }
        mWidth = w;
        mHeight = h;
        Bitmap tempBitmap = null;
        do {
            try {
                tempBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
            } catch(OutOfMemoryError oom) {
                Log.w(TAG, "setGraffitiDimens(): code has a memory leak is detected...");
            }
            if(tempBitmap == null) {
                //index is one, means that any operation can not be Completed
                if(mImageEditDesc.reorganizeQueue() < 2) {
                    failoverSetCanvas(brushPainting);
                    return;
                }
            }
        } while (tempBitmap == null);

        /**
         * FIX BUG: 1246
         * BUG CAUSE: tempBitmap maybe immutable, and create a Canvas with immutable
         *            bitmap is not allowed.
         * FIX COMMENT: if immutable is immutable, we will create a mutable one instead
         * DATE: 2012-07-16
         */
        Bitmap result = tempBitmap;
        if (!result.isMutable()) {
            result = tempBitmap.copy(Config.ARGB_8888, true);
        }
        mBitmap = result;
        mCanvas = null;
        mCanvas =  new Canvas(mBitmap);
        setImageBitmap(mBitmap);
        setPainting(brushPainting);
    }

    private void failoverSetCanvas(BrushPainting brushPainting) {
        /**
         * FIX BUG: 982 1436
         * BUG CAUSE: To appeared OutOfMemoryError and release half the number of Bitmap, the stack has only one Bitmap
         *          before initialize the variable mBrushPainting, so that the variable mBrushPainting is null.
         *            The bitmap which get from ImageEditDesc.getBitmap() maybe immutable, and create a Canvas with immutable
         *          bitmap is not allowed.
         * FIX COMMENT: If appeared OutOfMemoryError, and the stack has only oneBitmap, assign value to variable mBrushPainting.
         *              if immutable is immutable, we will create a mutable one instead.
         * DATE: 2012-05-18 2012-08-10
         */

        if(mBitmap !=null) {
            mBitmap.recycle();
            mBitmap = null;
        }
        Bitmap result = mImageEditDesc.getBitmap();
        /*
         * FIX BUG: 6086
         * BUG CAUSE: nullpointer
         * DATE: 2014-03-11
         */
        if(result == null) {
            return;
        }
        if (!result.isMutable()) {
            result = mImageEditDesc.getBitmap().copy(Config.ARGB_8888, true);
        }
        mBitmap = result;
        mCanvas = null;
        mCanvas =  new Canvas(mBitmap);
        setPainting(brushPainting);
    }

    public boolean isSaved() {
        return mIsSaved;
    }

    public ImageEditViewPreview BottomLayer() {
        return preview;
    }

    public void setBottomLayer(ImageEditViewPreview preview2) {
        preview = preview2;
    }

    public Bitmap composeGraffitiBitmap(){
        Bitmap bitmap = null;
        /*
         * FIX BUG: 6046
         * BUG CAUSE: nullpointer
         * DATE: 2014-03-07
         */
        Bitmap bottomLayer = preview == null ? null : preview.getBitmap();
        if(bottomLayer == null) {
            return null;
        }
        do {
            try {
                Config config = bottomLayer.getConfig();
                bitmap = Bitmap.createBitmap(bottomLayer.getWidth(), bottomLayer.getHeight(), (config != null ? config: Config.ARGB_8888));
            } catch(OutOfMemoryError oom) {
                Log.w(TAG, "composeGraffitiBitmap(): code has a memory leak is detected...");
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
        RectF rectF = ImageEditOperationUtil.resizeRectF(bottomLayer, getWidth(), getHeight(), false);
        float width =  rectF.width();
        float height =  rectF.height();
        float sx = bottomLayer.getWidth() / width ;
        float sy = bottomLayer.getHeight() / height;
        matrix.postScale(sx, sy);
        canvas.drawBitmap(mBitmap, matrix, p);
        mIsSaved = false;
        return bitmap;
    }

    public void chooseGraffitiPattern(int index, int canvasColor, BrushPainting brushPainting) {
        switch (index) {
            case ImageEditConstants.BRUSH_ITEM_CLEAR:
                if(mIsSaved){
                    Intent intent  = new Intent();
                    intent.setAction(ImageEditConstants.ACTION_GRAFFITI_CAN_BE_SAVE_CANCEL_RECEIVERD);
                    mContext.sendBroadcast(intent);
                }
                mIsSaved = false;
                Bitmap tempBitmap = mBitmap;
                do {
                    try {
                        Config config = mBitmap.getConfig();
                        tempBitmap = Bitmap.createBitmap(mBitmap.getWidth(), mBitmap.getHeight(), (config != null ? config : Config.ARGB_8888));
                    } catch(OutOfMemoryError oom) {
                        Log.w(TAG, "chooseGraffitiPattern(): code has a memory leak is detected...");
                    }
                    if(tempBitmap == null) {
                        //index is one, means that any operation can not be Completed
                        if(mImageEditDesc.reorganizeQueue() < 2) {
                            failoverSetCanvas(brushPainting);
                            return;
                        }
                    }
                } while(tempBitmap == null);

                /*
                 * FIX BUG: 1077
                 * BUG CAUSE: Recycled bitmap can't be erased.
                 * DATE: 2012-08-17
                 */
                if(tempBitmap == null || tempBitmap.isRecycled()) {
                    break;
                }

                mBitmap = tempBitmap;
                if(mCanvasColor != 0) {
                    mBitmap.eraseColor(mCanvasColor);
                } else {
                    mBitmap.eraseColor(Color.TRANSPARENT);
                }
                setImageBitmap(mBitmap);
                mCanvas = null;
                mCanvas =  new Canvas(mBitmap);
                setPainting(mBrushPainting);
                invalidate();
                break;
        }
    }

    private void touchBegin(float x, float y) {
        mBrushPainting.strokeFrom(x, y);

        mX = x;
        mY = y;
    }

     private void drawMoved(float x, float y) {
         if(mFlag) {
             return;
         }
         float dx = Math.abs(x - mX);
         float dy = Math.abs(y - mY);
         if(Math.hypot(dx, dy) < TOUCH_TOLERANCE){
             mX = x;
             mY = y;
             return;
         }

         mBrushPainting.strokeTo(x, y);

         mX = x;
         mY = y;
     }

     private void touchEnded() {
         if(mFlag) {
             mFlag = false;
             return;
         }
         mBrushPainting.strokeEnd(mX, mY);

         mIsSaved = true;
         Intent intent = new Intent();
         intent.setAction(ImageEditConstants.ACTION_GRAFFITI_CAN_BE_SAVE_RECEIVERD);
         mContext.sendBroadcast(intent);

         /*if((mBrushPainting.getBrushStyle() == BrushConstant.EraserBrush && mBrushPainting.getStrokeCount() != 0) ||
                 mBrushPainting.getBrushStyle() != BrushConstant.EraserBrush ) {
             mIsSaved = true;
             Intent intent = new Intent();
             intent.setAction(ImageEditConstants.ACTION_GRAFFITI_CAN_BE_SAVE_RECEIVERD);
             mContext.sendBroadcast(intent);
         }*/
     }

     @Override
     public boolean onTouchEvent(MotionEvent event) {
         if(mGestureDetector.onTouchEvent(event)){
             mFlag = true;
             return true;
         }
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

         float distanceX = endX - startX;
         float distanceY = endY - startY;
         switch (event.getAction() & MotionEvent.ACTION_MASK) {
             case MotionEvent.ACTION_DOWN:
                 touchBegin(x, y);
                 invalidate();
                 break;
              case MotionEvent.ACTION_POINTER_DOWN:
                oldDist = spacing(event);
                if (oldDist > 10f) {
                    midPoint(mid, event);
                    mode = MOVE;
                }
                mode = MOVE;
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
                 if (mode == MOVE) {
                    if(getScale() > 1f && isCocurrent(event)){
                        firstPointF.set(event.getX(0), event.getY(0));
                        secondPointF.set(event.getX(1), event.getY(1));
                        postTranslateCenter(distanceX, distanceY);
                        if(preview != null){
                            preview.postTranslateCenter(distanceX, distanceY);
                        }
                        break;
                    }
                    float newDist = spacing(event);
                    if (newDist > 10f) {
                        float distance = FloatMath.sqrt(getWidth() * getWidth()
                                + getHeight() * getHeight());
                        scale = 1 + (newDist - oldDist) / (distance*2/3);
                        if((getScale() < 0.25f && scale <= 1) || getScale() >= 4f && scale >=1) {
                            break;
                        }
                        preview.getSuppMatrix().postScale(scale, scale, mid.x, mid.y);
                        preview.setImageMatrix( preview.getImageViewMatrix());
                        preview.center(true, true);

                        getSuppMatrix().postScale(scale, scale, mid.x, mid.y);
                        setImageMatrix(getImageViewMatrix());
                        center(true, true);
                    }
                    oldDist = spacing(event);
                }else{
                    drawMoved(x, y);
                    invalidate();
                    break;
                }
                 break;
             case MotionEvent.ACTION_UP:
                 touchEnded();
                 invalidate();
                 break;
             case MotionEvent.ACTION_POINTER_UP:
                 mode = DRAW;
                 mFlag = true;
                 break;
         }
         return true;
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

     private boolean isCocurrent(MotionEvent event) {
         boolean flag = false;
         /*
          * FIX BUG:5200
          * BUG COMMENT: Fix IllegalArgumentException, this bug is cause by android system
          * DATE: 2013-11-11
          */
         try{
             flag = (event.getX(0) - firstPointF.x) * (event.getX(1) - secondPointF.x) > 0
                     && (event.getY(0) - firstPointF.y) * (event.getY(1) - secondPointF.y) > 0;
                     firstPointF.set(event.getX(0), event.getY(0));
                     secondPointF.set(event.getX(1), event.getY(1));
         }catch(IllegalArgumentException e) {
             LogUtils.error(TAG, "IllegalArgumentException : "+e);
         }
         return flag;
     }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        return super.dispatchTouchEvent(event);
    }
    private float getBaseScale() {
        float viewWidth = getWidth();
        float viewHeight = getHeight();

        float w = bm.getWidth();
        float h = bm.getHeight();
        float widthScale = viewWidth / w;
        float heightScale = viewHeight / h;
        return Math.min(Math.min(widthScale, heightScale), 3);
    }
    public void pupulateScale(float centerX, float centerY, float durationMs){
        Log.d(TAG, "pupulateScale(): getScale() = " + getScale());
        if(getScale() > 1F){
            zoomTo(1f, centerX, centerY, durationMs);
        }else{
            if(bm != null) {
                zoomTo(getBaseScale(), centerX, centerY, durationMs);
            }
        }
    }

    class ImageEditGraffitiViewGestureListener extends GestureDetector.SimpleOnGestureListener{
        @Override
        public boolean onDoubleTap(MotionEvent e) {
            Log.d(TAG, "onDoubleTap(): getScale() = " + getScale());
            mFlag = true;
            if(getScale() < 3F){
                ImageEditOperationUtil.showToast(mContext, R.string.text_edit_double_tap_tip, Toast.LENGTH_SHORT);
            }
            pupulateScale(e.getX(), e.getY(), 500);
            if(preview != null){
                preview.pupulateScale(e.getX(), e.getY(), 500);
            }
            return true;
        }
    }

    private void setPainting(BrushPainting brushPainting) {
        mBrushPainting = brushPainting;
        mBrushPainting.createCanvas(mWidth, mHeight, mCanvas, mBitmap, this, mCanvasColor);
    }
}
