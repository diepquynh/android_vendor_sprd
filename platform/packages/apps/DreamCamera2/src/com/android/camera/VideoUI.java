/*
 * Copyright (C) 2012 The Android Open Source Project
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

import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.SurfaceTexture;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.FrameLayout;
import android.widget.TextView;

import com.android.camera.app.OrientationManager;
import com.android.camera.debug.Log;
import com.android.camera.ui.PreviewOverlay;
import com.android.camera.ui.PreviewStatusListener;
import com.android.camera.ui.RotateLayout;
import com.android.camera.ui.focus.FocusRing;
import com.android.camera.widget.ModeOptionsOverlay;
import com.android.camera2.R;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.ex.camera2.portability.CameraSettings;

public class VideoUI implements PreviewStatusListener {
    private static final Log.Tag TAG = new Log.Tag("VideoUI");

    private final static float UNSET = 0f;
    private final PreviewOverlay mPreviewOverlay;
    // module fields
    protected final CameraActivity mActivity;
    protected final View mRootView;
    private final FocusRing mFocusRing;
    // An review image having same size as preview. It is displayed when
    // recording is stopped in capture intent.
    private ImageView mReviewImage;
    protected TextView mRecordingTimeView;
    private LinearLayout mLabelsLinearLayout;
    private RotateLayout mRecordingTimeRect;
    protected boolean mRecordingStarted = false;
    protected final VideoController mController;
    private float mZoomMax;

    private float mAspectRatio = UNSET;
    private final AnimationManager mAnimationManager;

    @Override
    public void onPreviewLayoutChanged(View v, int left, int top, int right,
            int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom) {
    }

    @Override
    public boolean shouldAutoAdjustTransformMatrixOnLayout() {
        return true;
    }

    @Override
    public void onPreviewFlipped() {
        mController.updateCameraOrientation();
    }

    private final GestureDetector.OnGestureListener mPreviewGestureListener
            = new GestureDetector.SimpleOnGestureListener() {
        @Override
        public boolean onSingleTapUp(MotionEvent ev) {
            mController.onSingleTapUp(null, (int) ev.getX(), (int) ev.getY());
            return true;
        }
    };

    public VideoUI(CameraActivity activity, VideoController controller, View parent) {
        mActivity = activity;
        mController = controller;
        mRootView = parent;
        ViewGroup moduleRoot = (ViewGroup) mRootView.findViewById(R.id.module_layout);
        mActivity.getLayoutInflater().inflate(R.layout.video_module,
                moduleRoot, true);

        mPreviewOverlay = (PreviewOverlay) mRootView.findViewById(R.id.preview_overlay);

        /* SPRD: fix bug539498 shows gap between Video icon area and option button area @{ */
        ModeOptionsOverlay mo = (ModeOptionsOverlay) mRootView.findViewById(R.id.mode_options_overlay);
        mo.setPadding(0,0,0,0);
        /* @} */
        initializeMiscControls();
        mAnimationManager = new AnimationManager();
        mFocusRing = (FocusRing) mRootView.findViewById(R.id.focus_ring);

        initUI();
    }

    public void setPreviewSize(int width, int height) {
        if (width == 0 || height == 0) {
            Log.w(TAG, "Preview size should not be 0.");
            return;
        }
        float aspectRatio;
        if (width > height) {
            aspectRatio = (float) width / height;
        } else {
            aspectRatio = (float) height / width;
        }
        setAspectRatio(aspectRatio);
    }

    public FocusRing getFocusRing() {
        return mFocusRing;
    }

    /**
     * Cancels on-going animations
     */
    public void cancelAnimations() {
        mAnimationManager.cancelAnimations();
    }

    public void setOrientationIndicator(int orientation, boolean animation) {
        // We change the orientation of the linearlayout only for phone UI
        // because when in portrait the width is not enough.
        if (mLabelsLinearLayout != null) {
            if (((orientation / 90) & 1) == 0) {
                mLabelsLinearLayout.setOrientation(LinearLayout.VERTICAL);
            } else {
                mLabelsLinearLayout.setOrientation(LinearLayout.HORIZONTAL);
            }
        }

        /*
         * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
         * Original Android code:

        mRecordingTimeRect.setOrientation(0, animation);

         */
    }

    private void initializeMiscControls() {
        mReviewImage = (ImageView) mRootView.findViewById(R.id.review_image);
        mRecordingTimeView = (TextView) mRootView.findViewById(R.id.recording_time);
        mRecordingTimeRect = (RotateLayout) mRootView.findViewById(R.id.recording_time_rect);
        // The R.id.labels can only be found in phone layout.
        // That is, mLabelsLinearLayout should be null in tablet layout.
        mLabelsLinearLayout = (LinearLayout) mRootView.findViewById(R.id.labels);

        // SPRD Bug:474704 Feature:Video Recording Pause.
        mPauseButton = (ImageView) mRootView.findViewById(R.id.btn_video_pause);
    }

    public void updateOnScreenIndicators(CameraSettings settings) {
    }

    public void setAspectRatio(float ratio) {
        if (ratio <= 0) {
            return;
        }
        float aspectRatio = ratio > 1 ? ratio : 1 / ratio;
        if (aspectRatio != mAspectRatio) {
            mAspectRatio = aspectRatio;
            mController.updatePreviewAspectRatio(mAspectRatio);
        }
    }

    public void setSwipingEnabled(boolean enable) {
        mActivity.setSwipingEnabled(enable);
    }

    public void showPreviewBorder(boolean enable) {
       // TODO: mPreviewFrameLayout.showBorder(enable);
    }

    /*
     * SPRD Bug:474704 Feature:Video Recording Pause. @{
     * Original Android code:

    public void showRecordingUI(boolean recording) {

     */
    public void showRecordingUI(boolean recording, int orientation) {
    /* @} */

        // SPRD Bug:474704 Feature:Video Recording Pause.
        if (recording == false)
            mPauseRecording = false;
        if (mPauseButton != null) {
            mPauseButton.setVisibility(recording ? View.VISIBLE : View.GONE);
            mPauseButton.setImageResource(
                    mPauseRecording ? R.drawable.icn_play_uui : R.drawable.btn_new_video_pause);
            mPauseButton.setAlpha(0.6f);
        }

        mRecordingStarted = recording;
        if (recording) {
            mRecordingTimeView.setText("");
            mRecordingTimeView.setVisibility(View.VISIBLE);
            mRecordingTimeView.announceForAccessibility(
                    mActivity.getResources().getString(R.string.video_recording_started));

            // SPRD Bug:474704 Feature:Video Recording Pause.
            mPauseButton.setVisibility(View.VISIBLE);
            setPauseButtonLayout(orientation);
        } else {
            mRecordingTimeView.announceForAccessibility(
                    mActivity.getResources().getString(R.string.video_recording_stopped));
            mRecordingTimeView.setVisibility(View.GONE);

            // SPRD Bug:474704 Feature:Video Recording Pause.
            mPauseButton.setVisibility(View.GONE);
        }
    }

    public void showReviewImage(Bitmap bitmap) {
        mReviewImage.setImageBitmap(bitmap);
        mReviewImage.setVisibility(View.VISIBLE);
    }

    public void showReviewControls() {
        mActivity.getCameraAppUI().transitionToIntentReviewLayout();
        mReviewImage.setVisibility(View.VISIBLE);
    }

    public void initializeZoom(CameraSettings settings, CameraCapabilities capabilities) {
        mZoomMax = capabilities.getMaxZoomRatio();
        // Currently we use immediate zoom for fast zooming to get better UX and
        // there is no plan to take advantage of the smooth zoom.
        // TODO: setup zoom through App UI.
        mPreviewOverlay.setupZoom(mZoomMax, settings.getCurrentZoomRatio(),
                new ZoomChangeListener());
    }

    /* SPRD: fix bug 553567 slow motion does not support takeSnapShot and zoom @{ */
    public void hideZoomProcessorIfNeeded() {
        mPreviewOverlay.hideZoomProcessorIfNeeded();
    }

    public void setRecordingTime(String text) {
        mRecordingTimeView.setText(text);
    }

    public void setRecordingTimeTextColor(int color) {
        mRecordingTimeView.setTextColor(color);
    }

    public boolean isVisible() {
        return false;
    }

    @Override
    public GestureDetector.OnGestureListener getGestureListener() {
        return mPreviewGestureListener;
    }

    @Override
    public View.OnTouchListener getTouchListener() {
        return null;
    }

    /**
     * Hide the focus indicator.
     */
    public void hidePassiveFocusIndicator() {
        if (mFocusRing != null) {
            Log.v(TAG, "mFocusRing.stopFocusAnimations()");
            mFocusRing.stopFocusAnimations();
        }
    }

    /**
     * Show the passive focus indicator.
     */
    public void showPassiveFocusIndicator() {
        if (mFocusRing != null) {
            mFocusRing.startPassiveFocus();
        }
    }


    /**
     * @return The size of the available preview area.
     */
    public Point getPreviewScreenSize() {
        return new Point(mRootView.getMeasuredWidth(), mRootView.getMeasuredHeight());
    }

    /**
     * Adjust UI to an orientation change if necessary.
     */
    public void onOrientationChanged(OrientationManager orientationManager,
                                     OrientationManager.DeviceOrientation deviceOrientation) {
        // do nothing.
    }

    private class ZoomChangeListener implements PreviewOverlay.OnZoomChangedListener {
        @Override
        public void onZoomValueChanged(float ratio) {
            mController.onZoomChanged(ratio);
        }

        @Override
        public void onZoomStart() {
        }

        @Override
        public void onZoomEnd() {
        }
    }

    /* SPRD: Fix bug 568154 @{ */
    public void setPreviewOverlayZoom(float zoom) {
        mPreviewOverlay.setZoom(zoom);
    }
    /* @} */

    public void hideZoomUI() {
        mPreviewOverlay.hideZoomUI();
    }

    // SurfaceTexture callbacks
    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        mController.onPreviewUIReady();
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        mController.onPreviewUIDestroyed();
        Log.d(TAG, "surfaceTexture is destroyed");
        return true;
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {
    }

    public void onPause() {
        // recalculate aspect ratio when restarting.
        mAspectRatio = 0.0f;
    }

    /*
     * SPRD Bug:474704 Feature:Video Recording Pause. @{
     */
    private ImageView mPauseButton;
    protected boolean mPauseRecording = false;

    private void setPauseButtonLayout(int orientation) {
        Log.i(TAG, "setPauseButtonLayout   orientation = " + orientation);

        FrameLayout.LayoutParams layout = new FrameLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.CENTER);
        float m = mActivity.getResources().getDimension(R.dimen.video_play_button_magin);
        if (orientation == 0) {
            layout.setMargins((int) m, 0, 0, 0);
        } else if (orientation == 90) {
            layout.setMargins(0, (int) m, 0, 0);
        } else if (orientation == 180) {
            layout.setMargins((int) m, 0, 0, 0);
        } else if (orientation == 270) {
            layout.setMargins(0, 0, 0, (int) m);
        }
        mPauseButton.setLayoutParams(layout);
    }

    public void onPauseClicked(boolean mPauseRecorderRecording) {
        // reset pause button icon
        mPauseRecording = mPauseRecorderRecording;
        mPauseButton.setImageResource(mPauseRecorderRecording ? R.drawable.icn_play_uui
                : R.drawable.btn_new_video_pause);
        mPauseButton.setAlpha(0.6f);
    }
    /* @} */

    /* SPRD: Add for bug 559531 @{ */
    public void enablePauseButton(boolean enable) {
        if (mPauseButton!= null) {
            mPauseButton.setEnabled(enable);
        }
    }
    /* @} */

    public void initUI() {

    }

}
