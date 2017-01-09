/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 *
 * Copyright (C) 2009 The Android Open Source Project
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

package com.ucamera.ucam.modules.ui;


import com.android.camera2.R;

import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.util.Log;
import android.graphics.RectF;
/**
 * A layout which handles the preview aspect ratio and the position of
 * the gripper.
 */
public class PreviewFrameLayout extends ViewGroup {

    /** A callback to be invoked when the preview frame's size changes. */
    public interface OnSizeChangedListener {
        public void onSizeChanged();
    }
    private static final String TAG = "PreviewFrameLayout";
    private static final double AspectRatio43 = 4.0 / 3.0;
    private double mAspectRatio = AspectRatio43;
    private static final int mHdpiBottomHeight = 85;
    private static final int mMdpiBottomHeight = 53;
    int mPreviewMinWidth = 0;
    int mPreviewHMarging = 0;
    // CID 109236 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // boolean mPreviewVisible = true;
    private int mPreviewVMarging = 0;
    /**
     *  CID 109236 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    private int mHandleIconW = 0;
    private int mHandleIconInW = 0;
    */
    private FrameLayout mFrame;
    // CID 28560 : UuF: Unused field (FB.UUF_UNUSED_FIELD)
    // private ImageView mHandleIcon;
    //private FocusRectangle mFocus;
    private OnSizeChangedListener mSizeListener;
    public static final DisplayMetrics mMetrics = new DisplayMetrics();
    private static double ScreenAspectRatio = 0;
    private boolean mGifModeFrontCamera = false;
    private boolean mIsGifMode = false;
    private boolean mIsRatioChanged = false;
    private Activity mActivity = null;
    private int[] mFrameLayoutParas;

    private RectF mPreviewArea = new RectF();

    public PreviewFrameLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        ((Activity) context).getWindowManager()
                .getDefaultDisplay().getMetrics(mMetrics);
        ScreenAspectRatio = (double)mMetrics.heightPixels/(double)mMetrics.widthPixels;
        mPreviewMinWidth = mMetrics.widthPixels * 3 / 8;
        /*if (Compatible.instance().mIsHTCDesireHD){
            mPreviewMinWidth = 120;
        }*/

        mActivity = (Activity) context;
        /*if (context.getClass().getName().contains("GIFMode")) {
            mIsGifMode = true;
        }*/
        /*
         * FIX BUG: 1724
         * BUG CAUSE: The init value of mApspectRatio in gifmode is 4/3, but
         *            the ScreenAspectRatio may not equal 4/3, this will cause
         *            interruptStartpreview in gifmode
         * FIX COMMIT: set the init value of mApspectRatio in gifmode to 4/3
         * FIX DATE:   2012-10-15
         */
        //if (!mIsGifMode) {
        mAspectRatio = ScreenAspectRatio;
        //}
        Log.d(TAG, "window size is " + mMetrics.widthPixels + "X" + mMetrics.heightPixels +
                " density:" + mMetrics.densityDpi);
    }

    public void setGifMode(boolean value){
        mIsGifMode = value;
        mAspectRatio = AspectRatio43;
    }

    public void setOnSizeChangedListener(OnSizeChangedListener listener) {
        mSizeListener = listener;
    }

    @Override
    protected void onFinishInflate() {
        mFrame = (FrameLayout)findViewById(R.id.frame_layout);
        if (mFrame == null) {
            throw new IllegalStateException(
                    "must provide child with id as \"frame\"");
        }
        SurfaceView cameraPreview = (SurfaceView)findViewById(R.id.preview_surface_view);
        if (cameraPreview != null){
            ViewGroup.LayoutParams layoutParams = cameraPreview.getLayoutParams();
            // initial a MarginLayoutParams
            if (layoutParams != null && layoutParams instanceof ViewGroup.MarginLayoutParams){
                ViewGroup.MarginLayoutParams marginLayoutParams = (ViewGroup.MarginLayoutParams)layoutParams;
                mPreviewHMarging = marginLayoutParams.leftMargin + marginLayoutParams.rightMargin;
                mPreviewVMarging = marginLayoutParams.topMargin + marginLayoutParams.bottomMargin;
            }
        }

//        if (ApiHelper.HAS_FACE_DETECTION) {
            //ViewStub faceViewStub = (ViewStub) findViewById(R.id.face_view_stub);
            /* preview_frame_video.xml does not have face view stub, so we need to
             * check that.
             */
            //if (faceViewStub != null) {
              //  faceViewStub.inflate();
            //}
//        }

        /*mFocus = (FocusRectangle) findViewById(R.id.focus_rectangle);
        if (mFocus == null) {
            Log.v(TAG,"Cannot find resource");
        }*/
    }

    // return true for the ratio changed and to requestLayout
    // return false when the ratio is not change
    public boolean setAspectRatio(double ratio) {
        Log.d(TAG, "setAspectRatio(): ratio = " + ratio + ", mAspectRatio = " + mAspectRatio);
        if (ratio <= 0.0) throw new IllegalArgumentException();
        mIsGifMode = false;
        /*
         * FIX BUG: 1818
         * FIX COMMENT: do not reset layout if Math.abs(mAspectRatio - ratio) less than 0.01
         * DATE: 2012-07-16
         */
        if (Math.abs(mAspectRatio - ratio) > 0.01) {
            mAspectRatio = ratio;
            mIsRatioChanged = true;
            requestLayout();
            return true;
        }
        else {
            return false;
        }
    }

    // return true for the ratio changed and to requestLayout
    // return false when the ratio is not change
    public boolean setGifAspectRatio(double ratio,boolean isFrontCamera) {
        if (ratio <= 0.0) throw new IllegalArgumentException();
        Log.d(TAG, "setAspectRatio = " + ratio);
        mIsGifMode = true;
        mGifModeFrontCamera = isFrontCamera;
        mActivity.getWindowManager().getDefaultDisplay().getMetrics(mMetrics);
        if (Math.abs(mAspectRatio - ratio) > 0.02) {
            mAspectRatio = ratio;
            mIsRatioChanged = true;
            requestLayout();
            return true;
        }
        else {
            /*
             * FIX BUG: 1223
             * BUG CAUSE: Honer-ICS need refresh after set aspect ratio
             * FIX COMMENT: still refresh when ratio is not change
             * DATE: 2012-07-16
             */
            requestLayout();
            invalidate();
            return false;
        }
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        // Try to layout the "frame" in the center of the area, and put
        // "gripper" just to the left of it. If there is no enough space for
        // the gripper, the "frame" will be moved a little right so that
        // they won't overlap with each other.

        int frameWidth = getWidth();
        int frameHeight = getHeight();
        int horizontalPadding = mPreviewHMarging;
        int verticalPadding = mPreviewVMarging;
//        FrameLayout f = mFrame;

        // Ignore the vertical paddings, so that we won't draw the frame on the
        // top and bottom sides
        int previewHeight = frameHeight - verticalPadding;
        int previewWidth = frameWidth - horizontalPadding;

        // resize frame and preview for aspect ratio
        if (previewHeight > previewWidth * mAspectRatio) {
            previewHeight = (int) (previewWidth * mAspectRatio + .5);
        } else {
            previewWidth = (int) (previewHeight / mAspectRatio + .5);
        }
        // limit the previewWidth large than minimalWidth
        if (previewWidth < mPreviewMinWidth){
            previewWidth = mPreviewMinWidth;
            previewHeight = (int)(previewWidth * mAspectRatio + .5);
        }

        frameWidth = previewWidth + mPreviewHMarging;
        frameHeight = previewHeight + mPreviewVMarging;

        int hSpace = ((r - l) - frameWidth) / 2;
        int vSpace = ((b - t) - frameHeight);

        // calc the tPads
        int tPads = 0;

        if (vSpace > 0) {
            // calc the bottom control bar width for special density
            int bottomHeight = 0;
            switch (mMetrics.densityDpi){
            case DisplayMetrics.DENSITY_HIGH:
                bottomHeight = mHdpiBottomHeight;
                break;
            case DisplayMetrics.DENSITY_MEDIUM:
                bottomHeight = mMdpiBottomHeight;
                break;
            case DisplayMetrics.DENSITY_LOW:
                bottomHeight = mMdpiBottomHeight * DisplayMetrics.DENSITY_LOW / DisplayMetrics.DENSITY_MEDIUM;
                break;
            case DisplayMetrics.DENSITY_XHIGH:
                bottomHeight = mHdpiBottomHeight * DisplayMetrics.DENSITY_XHIGH / DisplayMetrics.DENSITY_HIGH;
                break;
            default:
                bottomHeight = mHdpiBottomHeight * mMetrics.densityDpi / DisplayMetrics.DENSITY_HIGH;
                break;
            }

            int tmp = vSpace - bottomHeight;
            if (tmp > 0) {
                /*int cameraId = ((CameraActivity)mActivity).mCameraId;
                if (Compatible.instance().mIsIriverMX100 && cameraId == Const.CAMERA_FRONT){
                    tPads = vSpace/2;
                }
                else if (!RotationUtil.calcNeedRevert(cameraId)) {
                    //large-screen preview align with the up edge of control bar
                    tPads = tmp;
                }
                else {*///small-screen preview vertical center the screen
                    tPads = vSpace/2;
                //}
            }else {
                tPads = 0;
            }
        }
        if (mIsGifMode) {       // GIF mode preview size is specified
            int width = (int) (mMetrics.widthPixels * 0.9);//(int)getResources().getDimension(R.dimen.gif_mode_preview_width);
            /*
             * FIX BUG: 5484
             * FIX COMMENT: some devices the width is larger than height,so these devices's ratio value is width divided height
             * DATE: 2013-12-05
             */
            double ratio  = mAspectRatio;
            if(ratio < 1) {
                ratio = 1 / ratio;
            }
            int height = (int) (width * ratio);//(int)getResources().getDimension(R.dimen.gif_mode_preview_height);
            int frontPaddingTop = (int)getResources().getDimension(R.dimen.gif_mode_preview_padding_top);
            int backPaddingLeft = (mMetrics.widthPixels - width)/2;
            int backPaddingTop = frontPaddingTop - (height - width)/2;
            mFrame.measure(
                    MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(height, MeasureSpec.EXACTLY));
            if (!mGifModeFrontCamera) {
                 mFrame.layout(backPaddingLeft, backPaddingTop, width+backPaddingLeft, height+backPaddingTop);
                 mPreviewArea.set(backPaddingLeft, backPaddingTop, width+backPaddingLeft, height+backPaddingTop);
            }else {
                mFrame.layout(0, frontPaddingTop, height, frontPaddingTop+width);
                mPreviewArea.set(0, frontPaddingTop, height, frontPaddingTop+width);
            }
        } else {
            mFrame.measure(
                    MeasureSpec.makeMeasureSpec(frameWidth, MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(frameHeight, MeasureSpec.EXACTLY));
            mFrame.layout(l + hSpace, t + tPads, r - hSpace, b - vSpace + tPads);
            int[] layoutPars = new int[4];
            layoutPars[0] = l + hSpace;
            layoutPars[1] = t + tPads;
            layoutPars[2] = r - hSpace;
            layoutPars[3] = b - vSpace + tPads;
            setmFrameLayoutParas(layoutPars);
        }


        /*if (mFocus != null) {
            int size = mFocus.getWidth()/2;
            int x = mFocus.getTouchIndexX();
            int y = mFocus.getTouchIndexY();
            if ((x >= 0)&&( y >= 0)){
                // touchafaec mode
                mFocus.layout(x - size, y - size,x + size,y + size);
            }else{
                mFocus.setPosition(frameWidth/2, frameHeight/2);
            }
        }*/

        if (mSizeListener != null) {
            mSizeListener.onSizeChanged();
        }

        if (mIsRatioChanged) {/*
            mIsRatioChanged = false;
            if (mActivity instanceof CameraActivity){
                Handler handler = ((CameraActivity)mActivity).getHandler();
//                handler.sendEmptyMessage(Camera.MULTI_PIC_INITIALIZE);
                handler.sendEmptyMessage(Camera.UPDATE_SPLITE_LINE);
                if (Camera.mCurrentCameraMode == Camera.CAMERA_MODE_MAGIC_LENS
                        || Camera.mCurrentCameraMode == Camera.CAMERA_MODE_PIP) {
                    handler.sendEmptyMessage(Camera.RESTART_PREVIEW);
                }
                *//**
                 * FIX BUG: 1348
                 * FIX COMMENT: let layout adaptive.
                 * DATE: 2012-07-30
                 *//*
                Message message = new Message();
                message.what = Camera.UPDATE_REVIEW_EDIT_BAR_SIZE;
                message.arg1 = tPads;
                handler.handleMessage(message);
            }
        */}
        /*
        View view = mActivity.findViewById(R.id.top_menu_root);
        int topviewHeight = (view != null)?view.getHeight():0;
        // initialize the original top bar height
        if ((mInitTopBarHeight == 0) && (topviewHeight > 0)){
            mInitTopBarHeight = topviewHeight;
        }

        if (mInitTopBarHeight != 0) {
            // reset the height of top bar view

            int nHeight = 0;
            if (tPads > mInitTopBarHeight){
                nHeight = tPads;
            }
            else{
                nHeight = mInitTopBarHeight;
            }

            if (mActivity instanceof CameraActivity){
                Handler handler = ((CameraActivity)mActivity).getHandler();
                Message m = handler.obtainMessage(VideoCamera.RELAYOUT_TOP_BAR, nHeight, 0, null);
                handler.sendMessage(m);
            }else{
                ViewGroup.LayoutParams layoutParams = view.getLayoutParams();
                layoutParams.width = LayoutParams.FILL_PARENT;
                layoutParams.height = nHeight;
                view.setLayoutParams(layoutParams);
            }
        }
        */
    }

    public int[] getmFrameLayoutParas() {
        return mFrameLayoutParas;
    }

    public void setmFrameLayoutParas(int[] frameLayoutParas) {
        this.mFrameLayoutParas = frameLayoutParas.clone();
    }

    /**
     * SPRD:Fix bug 595400 the freeze screen for gif 
     * Returns a new copy of the preview area, to avoid internal data being
     * modified from outside of the class.
     */
    public RectF getPreviewArea() {
        return new RectF(mPreviewArea);
    }

}

