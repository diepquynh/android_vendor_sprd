/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.util.FloatMath;
import android.view.GestureDetector;
import android.view.MotionEvent;

public class ImageEditViewInputProcessor {
    private ImageEditViewPreview mEditView;
    private GestureDetector mGestureDetector;

    public ImageEditViewInputProcessor(ImageEditViewPreview editView, Context context) {
        mEditView = editView;
        mGestureDetector = new GestureDetector(context, new ImageEditViewGestureListener());
    }

    private float oldDist;
    private PointF start = new PointF();
    private PointF start2 = new PointF();
    private PointF mid = new PointF();
    public float mLastXposition = 0;
    private Matrix matrix = new Matrix();
    private Matrix savedMatrix = new Matrix();
    private float lastStartX;
    private float lastStartY;
    private float lastEndX;
    private float lastEndY;
    private float scale = 0;
    private boolean mSetZoomMin = false;
    private float mMinZoom = 0.25f;
    // We can be in one of these 3 states
    static final int NONE = 0; //normal status
    static final int DRAG = 1; //move status
    static final int ZOOM = 2; //zoom in or zoom out
    int mode = NONE;

    public boolean onTouchEvent(MotionEvent event) {
        int deltaX = 0;
        final int action = event.getAction() & MotionEvent.ACTION_MASK;

        float startX = start.x;
        float startY = start.y;
        float endX = event.getX();
        float endY = event.getY();

        float distanceX = endX - startX;
        float distanceY = endY - startY;
        switch(action){
            case MotionEvent.ACTION_DOWN:
                mLastXposition = event.getRawX();
                savedMatrix.set(matrix);
                start.set(event.getX(), event.getY());
                start2.set(event.getX(), event.getY());
                mode = DRAG;
                break;

            case MotionEvent.ACTION_MOVE:
                if (mode == DRAG) {
                    deltaX = (int)(mLastXposition - event.getRawX());
                    if(mEditView.getScale() > 1f){
                        if(startX == lastStartX && startY == lastStartY &&
                             endX == lastEndX && endY == lastEndY){
                            break;
                        }
                        lastStartX = startX;
                        lastStartY = startY;
                        lastEndX = endX;
                        lastEndY = endY;

                        //If we're zoomed in, moving the image, it will follow the move with your fingers
                        mEditView.postTranslateCenter(distanceX, distanceY);
                        start.set(event.getX(), event.getY());
                    }else{
//                        populateMoveEvent(deltaX);
                    }

                }else if (mode == ZOOM) {
                    float newDist = spacing(event);
                    if (newDist > 10f) {
                        float distance = FloatMath.sqrt(mEditView.getWidth() * mEditView.getWidth()
                                + mEditView.getHeight() * mEditView.getHeight());
                        scale = 1 + (newDist - oldDist) / (distance*2/3);
                        //when scale is smaller or larger, stoping the zoom in or zoom out action.
                        if((mEditView.getScale() < mMinZoom && scale <= 1) || mEditView.getScale() >= 4f && scale >=1) {
                            break;
                        }
                        mEditView.getSuppMatrix().postScale(scale, scale, mid.x, mid.y);
                        mEditView.setImageMatrix(mEditView.getImageViewMatrix());
                        mEditView.center(true, true);
                    }
                    oldDist = spacing(event);
                    if(mSetZoomMin && mEditView.getScale() < mMinZoom) {
                        mEditView.zoomTo(mMinZoom);
                    }
                }
                break;

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                if(mode == DRAG){
                    deltaX = (int)(mLastXposition - event.getRawX());
                    stopMoveEvent(deltaX);
                }
                mode = NONE;
                break;

            case MotionEvent.ACTION_POINTER_UP:
                mode = NONE;
                break;

            case MotionEvent.ACTION_POINTER_DOWN:
                oldDist = spacing(event);
                if (oldDist > 10f) {
                    savedMatrix.set(matrix);
                    midPoint(mid, event);
                    mode = ZOOM;
                }
                deltaX = (int)(mLastXposition - event.getRawX());
                stopMoveEvent(deltaX);
                break;

            default:

        }

        return false;
    }

    public void setMinZoom(boolean bSetMinZoom, float fMinZoom) {
        mSetZoomMin = bSetMinZoom;
        mMinZoom = fMinZoom;
    }
    /**
     * @param deltaX deltaX
     */
    public void populateMoveEvent(int deltaX) {
        mEditView.populateMoveEvent(deltaX);
    }

    private float spacing(MotionEvent event) {
        float x = event.getX(0) - event.getX(1);
        float y = event.getY(0) - event.getY(1);
        return FloatMath.sqrt(x * x + y * y);
    }

    private void midPoint(PointF point, MotionEvent event) {
        float x = event.getX(0) + event.getX(1);
        float y = event.getY(0) + event.getY(1);
        point.set(x / 2, y / 2);
    }

    /**
     * @param deltaX deltaX
     */
    public void stopMoveEvent(int deltaX) {
        if (deltaX == 0) {
            return;
        }
    }

    public void dispatchTouchEvent(MotionEvent event) {
        mGestureDetector.onTouchEvent(event);
    }

    class ImageEditViewGestureListener extends GestureDetector.SimpleOnGestureListener{
        public boolean onDoubleTap(MotionEvent e) {
            mEditView.pupulateScale(e.getX(), e.getY(), 500);
            return true;
        }
    }
}

