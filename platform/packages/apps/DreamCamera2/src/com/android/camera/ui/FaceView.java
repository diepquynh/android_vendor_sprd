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

package com.android.camera.ui;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.RectF;
import android.hardware.Camera.Face;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.view.View;

import com.android.camera.debug.Log;
import com.android.camera.util.CameraUtil;
import com.android.camera2.R;
import com.sprd.camera.aidetection.AIDetectionController;
import android.graphics.Rect;

public class FaceView extends View
    implements Rotatable, PreviewStatusListener.PreviewAreaChangedListener {
    private static final Log.Tag TAG = new Log.Tag("FaceView");
    private final boolean LOGV = false;
    // The value for android.hardware.Camera.setDisplayOrientation.
    private int mDisplayOrientation;
    // The orientation compensation for the face indicator to make it look
    // correctly in all device orientations. Ex: if the value is 90, the
    // indicator should be rotated 90 degrees counter-clockwise.
    private int mOrientation;
    private boolean mMirror;
    private boolean mPause;
    private Matrix mMatrix = new Matrix();
    private RectF mRect = new RectF();
    // As face detection can be flaky, we add a layer of filtering on top of it
    // to avoid rapid changes in state (eg, flickering between has faces and
    // not having faces)
    private Face[] mFaces;
    private Face[] mPendingFaces;
    private int mColor;
    private Paint mPaint;
    private volatile boolean mBlocked;

    private static final int MSG_SWITCH_FACES = 1;
    private static final int SWITCH_DELAY = 70;
    private static final int MSG_CLEAR_FACES = 2;//SPRD:Add for ai detect
    private static final int CLEAR_DELAY = 300;//SPRD:Add for ai detect
    private boolean mStateSwitchPending = false;
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MSG_SWITCH_FACES:
                mStateSwitchPending = false;
                // SPRD:add for bug 615103 when close the face detection , the face box will not close
                if (mFaces != null) {
                    mFaces = mPendingFaces;
                }
                invalidate();
                break;
            /* SPRD:Add for ai detect @{ */
            case MSG_CLEAR_FACES:
                clear();
                break;
            /* @} */
            }
        }
    };
    private final RectF mPreviewArea = new RectF();

    public FaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        Resources res = getResources();
        mColor = res.getColor(R.color.face_detect_start);
        mPaint = new Paint();
        mPaint.setAntiAlias(true);
        mPaint.setStyle(Style.STROKE);
        mPaint.setStrokeWidth(res.getDimension(R.dimen.face_circle_stroke));
    }

    public void setFaces(Face[] faces) {
        /* SPRD:Modify for add ai detect bug 474723 @{
        if (LOGV) {
            Log.v(TAG, "Num of faces=" + faces.length);
        }
        */
        // Change the Strategy of log print, I think it should not always print a same log
        if (mFaces == null || faces.length != mFaces.length) {
            Log.v(TAG, "Num of faces=" + faces.length);
        }
        /* @} */
        if (mPause) return;
        if (mFaces != null) {
            if ((faces.length > 0 && mFaces.length == 0)
                    || (faces.length == 0 && mFaces.length > 0)) {
                mPendingFaces = faces;
                if (!mStateSwitchPending) {
                    mStateSwitchPending = true;
                    mHandler.sendEmptyMessageDelayed(MSG_SWITCH_FACES, SWITCH_DELAY);
                }
                return;
            }
        }
        if (mStateSwitchPending) {
            mStateSwitchPending = false;
            mHandler.removeMessages(MSG_SWITCH_FACES);
        }
        mFaces = faces;
        invalidate();
    }

    public void setDisplayOrientation(int orientation) {
        mDisplayOrientation = orientation;
        if (LOGV) {
            Log.v(TAG, "mDisplayOrientation=" + orientation);
        }
    }

    @Override
    public void setOrientation(int orientation, boolean animation) {
        /*
         * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
         * Original Android code:

        mOrientation = orientation;
        invalidate();

         */
    }

    public void setMirror(boolean mirror) {
        mMirror = mirror;
        if (LOGV) {
            Log.v(TAG, "mMirror=" + mirror);
        }
    }

    public boolean faceExists() {
        return (mFaces != null && mFaces.length > 0);
    }

    public void clear() {
        // Face indicator is displayed during preview. Do not clear the
        // drawable.
        mFaces = null;
        invalidate();
    }

    public void pause() {
        mPause = true;
    }

    public void resume() {
        mPause = false;
    }

    public void setBlockDraw(boolean block) {
        mBlocked = block;
    }

    /* SPRD:Add for ai detect @{ */
    public void clearFacesDelayed() {
        if (!mHandler.hasMessages(MSG_CLEAR_FACES)) {
            mHandler.sendEmptyMessageDelayed(MSG_CLEAR_FACES, CLEAR_DELAY);
        }
    }
    /* @} */

    @Override
    protected void onDraw(Canvas canvas) {
        if (!mBlocked && (mFaces != null) && (mFaces.length > 0)) {
            int rw, rh;
            rw = (int) mPreviewArea.width();
            rh = (int) mPreviewArea.height();
            // Prepare the matrix.
            if (((rh > rw) && ((mDisplayOrientation == 0) || (mDisplayOrientation == 180)))
                    || ((rw > rh) && ((mDisplayOrientation == 90) || (mDisplayOrientation == 270)))) {
                int temp = rw;
                rw = rh;
                rh = temp;
            }
            CameraUtil.prepareMatrix(mMatrix, mMirror, mDisplayOrientation, rw, rh);
            // Focus indicator is directional. Rotate the matrix and the canvas
            // so it looks correctly in all orientations.
            canvas.save();
            mMatrix.postRotate(mOrientation); // postRotate is clockwise
            canvas.rotate(-mOrientation); // rotate is counter-clockwise (for canvas)
            AIDetectionController mAIController = new AIDetectionController();//SPRD:Add for ai detect
            for (int i = 0; i < mFaces.length; i++) {
                // Filter out false positives.
                /*SPRD:Modify for ai detect @{
                if (mFaces[i].score < 50) continue;
                */
                if (mAIController.isChooseSmile() && mFaces[i].score < 30) {
                    continue;
                }
                /* @} */

                // Transform the coordinates.
                mRect.set(mFaces[i].rect);
                if (LOGV) {
                    CameraUtil.dumpRect(mRect, "Original rect");
                }
                mMatrix.mapRect(mRect);
                if (LOGV) {
                    CameraUtil.dumpRect(mRect, "Transformed rect");
                }
                mPaint.setColor(mColor);
                mRect.offset(mPreviewArea.left, mPreviewArea.top);
                canvas.drawRect(mRect, mPaint);
            }
            canvas.restore();
        }
        super.onDraw(canvas);
    }

    /* SPRD:Add for ai detect @{ */
    public boolean isPause() {
        return mPause;
    }
    /* @} */

    @Override
    public void onPreviewAreaChanged(RectF previewArea) {
        mPreviewArea.set(previewArea);
    }

    /* SPRD: New Feature for VGesture @{ */
    public Rect[] getFaceRect(Face[] faces){
        if (mPause)
            return null;// SPRD BUG : 397097
        RectF vrect = new RectF();
        Rect[] rect = new Rect[faces.length];
         if ((faces != null) && (faces.length > 0)) {
             int rw, rh;
             rw = (int) mPreviewArea.width();
             rh = (int) mPreviewArea.height();
             // Prepare the matrix.
             if (((rh > rw) && ((mDisplayOrientation == 0) || (mDisplayOrientation == 180)))
                     || ((rw > rh) && ((mDisplayOrientation == 90) || (mDisplayOrientation == 270)))) {
                 int temp = rw;
                 rw = rh;
                 rh = temp;
             }
             CameraUtil.prepareMatrix(mMatrix, mMirror, mDisplayOrientation, rw, rh);
             // Focus indicator is directional. Rotate the matrix and the canvas
             // so it looks correctly in all orientations.
             mMatrix.postRotate(mOrientation); // postRotate is clockwise
//             canvas.rotate(-mOrientation); // rotate is counter-clockwise (for canvas)
             for (int i = 0; i < faces.length; i++) {

                 // Transform the coordinates.
                 rect[i] = new Rect();
                 vrect.set(faces[i].rect);
                 mMatrix.mapRect(vrect);
                 vrect.offset(mPreviewArea.left, mPreviewArea.top);
                 vrect.roundOut(rect[i]);
             }
         }
         return rect;
    }

    public Rect getRealArea(){
        Rect rect = new Rect();
       mPreviewArea.roundOut(rect);
       return rect;
    }
    /* @} */
}
