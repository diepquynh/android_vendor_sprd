/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.free;

import android.graphics.Matrix;
import android.graphics.PointF;
import android.util.FloatMath;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

public class MultiTouchListener implements OnTouchListener {
    private static final String TAG = "MultiTouchListener";
    Matrix mMatrix = new Matrix();
    Matrix mSavedMatrix = new Matrix();

    // We can be in one of these 3 states
    static final int NONE = 0;
    static final int DRAG = 1;
    static final int ZOOM = 2;
    int mMode = NONE;

    // Remember some things for zooming
    PointF mStartPointF = new PointF();
    PointF mStartRotatePointF = new PointF();
    PointF mMidPointF = new PointF();
    float mOldDist = 1f;
    PointF mRotatePointF = new PointF();
    private float mRoateDegree = 0f;
    MultiTouchListener() {
        super();
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        FreePuzzlePiece view = (FreePuzzlePiece) v;
        // Dump touch event to log
//        dumpEvent(event);

        switch (event.getAction() & MotionEvent.ACTION_MASK) {
            case MotionEvent.ACTION_DOWN :
                mMatrix.set(view.getImageMatrix());
                mSavedMatrix.set(mMatrix);
                mStartPointF.set(event.getX(), event.getY());
                mMode = DRAG;
                break;
            case MotionEvent.ACTION_POINTER_DOWN :
                mOldDist = calculateDistance(event);
                if (mOldDist > 20f) {
                    mStartRotatePointF.set(event.getX(1), event.getY(1));
                    mSavedMatrix.set(mMatrix);
                    midPoint(mMidPointF, event);
                    mMode = ZOOM;
                    Log.d(TAG, "mode=ZOOM");
                }
                break;
            case MotionEvent.ACTION_UP :
            case MotionEvent.ACTION_POINTER_UP :
                mMode = NONE;
                mRoateDegree = 0f;
                break;
            case MotionEvent.ACTION_MOVE :
                if (mMode == DRAG) {
                    if (view.isPointInView((int)event.getX(), (int)event.getY())) {
                        mMatrix.set(mSavedMatrix);
                        mMatrix.postTranslate(event.getX() - mStartPointF.x, event.getY() - mStartPointF.y);
                    }
                }
                else if (mMode == ZOOM) {
                    float newDist = calculateDistance(event);
                    mMatrix.set(mSavedMatrix);
                    if (newDist > 20f) {
                        float scale = newDist / mOldDist;
                        mMatrix.postScale(scale, scale, mMidPointF.x, mMidPointF.y);
                        view.setImageMatrix(mMatrix);
                        mRoateDegree += getRotateDegree(event);
                        mMatrix.postRotate(mRoateDegree, view.getBitmapCenter().x, view.getBitmapCenter().y);
                    }
                }
                break;
        }
        view.setImageMatrix(mMatrix);
        return true;
    }

    private float getRotateDegree(MotionEvent event) {
        float degree = 0f;
        float distance = calculateTwoPointDistance(new PointF(event.getX(1), event.getY(1)), mStartRotatePointF);
        midPoint(mRotatePointF, event);
        if (!judgeRotateClockWise(event, mRotatePointF)) {
            distance = -distance;
        }
        mStartRotatePointF.set(event.getX(1), event.getY(1));
        float radius = calculateTwoPointDistance(mRotatePointF, new PointF(event.getX(1), event.getY(1)));
        if (radius == 0) {
            return 0f;
        }
        degree = (float) Math.asin(distance / (2 * radius));
        if (Float.isNaN(degree)) {
            degree = 0f;
        }
        else {
            degree =  ((float) (360 * degree / Math.PI));
        }
        return degree;
    }

    private float calculateTwoPointDistance(PointF p1, PointF p2) {
        float x = p1.x - p2.x;
        float y = p1.y - p2.y;
        return FloatMath.sqrt(x * x + y * y);
    }

    private boolean judgeRotateClockWise(MotionEvent event, PointF p1) {
        if ((p1.x - mStartRotatePointF.x) * (event.getY(1) - p1.y) - (p1.y - mStartRotatePointF.y) * (event.getX(1) - p1.x) > 0) {
            // counter clockwise
            return false;
        }
        // clockwise
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
}
