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

package com.ucamera.ucam.modules.ui;

import android.app.Dialog;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.Camera.Face;
import android.os.AsyncTask;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.PhotoModule;
import com.android.camera.OnScreenHint;
import com.android.camera.app.OrientationManager;
import com.android.camera.captureintent.PictureDecoder;
import com.android.camera.debug.DebugPropertyHelper;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.ui.CountDownView;
import com.android.camera.ui.FaceView;
import com.android.camera.ui.PreviewOverlay;
import com.android.camera.ui.PreviewStatusListener;
import com.android.camera.ui.focus.FocusRing;
import com.android.camera2.R;
import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.ex.camera2.portability.CameraSettings;
import com.sprd.camera.aidetection.AIDetectionController;
import com.sprd.camera.freeze.FaceDetectionController;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.DreamOrientation;
import com.dream.camera.DreamUI;

public class BasicUI extends DreamUI implements PreviewStatusListener,
    CameraAgent.CameraFaceDetectionCallback, PreviewStatusListener.PreviewAreaChangedListener,
    /* SPRD: Add for bug 461734 */FaceDetectionController{

    private static final Log.Tag TAG = new Log.Tag("BasicUI");
    private static final int DOWN_SAMPLE_FACTOR = 4;
    private static final float UNSET = 0f;

    private final PreviewOverlay mPreviewOverlay;
    private final FocusRing mFocusRing;
    public final CameraActivity mActivity;
    public final PhotoController mController;

    protected final View mRootView;
    protected Dialog mDialog = null;

    // TODO: Remove face view logic if UX does not bring it back within a month.
    protected final FaceView mFaceView;
    protected DecodeImageForReview mDecodeTaskForReview = null;

    protected float mZoomMax;

    protected int mPreviewWidth = 0;
    protected int mPreviewHeight = 0;
    protected float mAspectRatio = UNSET;
    public int mWidth;
    public int mHeight;

    private ImageView mIntentReviewImageView;
    private AIDetectionController mAIController;

    private final GestureDetector.OnGestureListener mPreviewGestureListener
            = new GestureDetector.SimpleOnGestureListener() {
        @Override
        public boolean onSingleTapUp(MotionEvent ev) {
            mController.onSingleTapUp(null, (int) ev.getX(), (int) ev.getY());
            return true;
        }
    };
    private final DialogInterface.OnDismissListener mOnDismissListener
            = new DialogInterface.OnDismissListener() {
        @Override
        public void onDismiss(DialogInterface dialog) {
            mDialog = null;
        }
    };
    private final CountDownView mCountdownView;

    @Override
    public GestureDetector.OnGestureListener getGestureListener() {
        return mPreviewGestureListener;
    }

    @Override
    public View.OnTouchListener getTouchListener() {
        return null;
    }

    @Override
    public void onPreviewLayoutChanged(View v, int left, int top, int right,
            int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom) {
        mWidth = right - left;
        mHeight = bottom - top;
        if (mPreviewWidth != mWidth || mPreviewHeight != mHeight) {
            mPreviewWidth = mWidth;
            mPreviewHeight = mHeight;
        }
        setTransformMatrix();
    }

    @Override
    public boolean shouldAutoAdjustTransformMatrixOnLayout() {
        return true;
    }

    @Override
    public void onPreviewFlipped() {
        mController.updateCameraOrientation();
    }

    /**
     * Starts the countdown timer.
     *
     * @param sec seconds to countdown
     */
    public void startCountdown(int sec) {
        updateStartCountDownUI();
        mCountdownView.startCountDown(sec);
    }

    /**
     * Sets a listener that gets notified when the countdown is finished.
     */
    public void setCountdownFinishedListener(CountDownView.OnCountDownStatusListener listener) {
        mCountdownView.setCountDownStatusListener(listener);
    }

    /**
     * Returns whether the countdown is on-going.
     */
    public boolean isCountingDown() {
        return mCountdownView.isCountingDown();
    }

    /**
     * Cancels the on-going countdown, if any.
     */
    public void cancelCountDown() {
        mActivity.getCameraAppUI().updatePreviewUI(View.VISIBLE);
        mCountdownView.cancelCountDown();
    }

    @Override
    public void onPreviewAreaChanged(RectF previewArea) {
        if (mFaceView != null) {
            mFaceView.onPreviewAreaChanged(previewArea);
        }
        mCountdownView.onPreviewAreaChanged(previewArea);
    }

    private class DecodeTask extends AsyncTask<Void, Void, Bitmap> {
        private final byte [] mData;
        private final int mOrientation;
        private final boolean mMirror;

        public DecodeTask(byte[] data, int orientation, boolean mirror) {
            mData = data;
            mOrientation = orientation;
            mMirror = mirror;
        }

        @Override
        protected Bitmap doInBackground(Void... params) {
            // Decode image in background.
            return PictureDecoder.decode(mData, DOWN_SAMPLE_FACTOR, mOrientation, mMirror);
        }
    }

    private class DecodeImageForReview extends DecodeTask {
        public DecodeImageForReview(byte[] data, int orientation, boolean mirror) {
            super(data, orientation, mirror);
        }

        @Override
        protected void onPostExecute(Bitmap bitmap) {
            if (isCancelled()) {
                return;
            }

            mIntentReviewImageView.setImageBitmap(bitmap);
            showIntentReviewImageView();

            mDecodeTaskForReview = null;
        }
    }

    public BasicUI(int cameraId, CameraActivity activity, PhotoController controller, View parent) {
        mActivity = activity;
        mController = controller;
        mRootView = parent;

        ViewGroup moduleRoot = (ViewGroup) mRootView.findViewById(R.id.module_layout);
        mActivity.getLayoutInflater().inflate(R.layout.photo_module,
                 moduleRoot, true);
        initIndicators();
        mFocusRing = (FocusRing) mRootView.findViewById(R.id.focus_ring);
        mPreviewOverlay = (PreviewOverlay) mRootView.findViewById(R.id.preview_overlay);
        mCountdownView = (CountDownView) mRootView.findViewById(R.id.count_down_view);
        // Show faces if we are in debug mode.
//        if (DebugPropertyHelper.showCaptureDebugUI()) {
            mFaceView = (FaceView) mRootView.findViewById(R.id.face_view);
//        } else {
//            mFaceView = null;
//        }

        if (mController.isImageCaptureIntent()) {
            initIntentReviewImageView();
        }
        initUI();
    }

    private void initIntentReviewImageView() {
        mIntentReviewImageView = (ImageView) mRootView.findViewById(R.id.intent_review_imageview);
        mActivity.getCameraAppUI().addPreviewAreaChangedListener(
                new PreviewStatusListener.PreviewAreaChangedListener() {
                    @Override
                    public void onPreviewAreaChanged(RectF previewArea) {
                        FrameLayout.LayoutParams params =
                            (FrameLayout.LayoutParams) mIntentReviewImageView.getLayoutParams();
                        params.width = (int) previewArea.width();
                        params.height = (int) previewArea.height();
                        params.setMargins((int) previewArea.left, (int) previewArea.top, 0, 0);
                        mIntentReviewImageView.setLayoutParams(params);
                    }
                });
    }

    /**
     * Show the image review over the live preview for intent captures.
     */
    public void showIntentReviewImageView() {
        if (mIntentReviewImageView != null) {
            mIntentReviewImageView.setVisibility(View.VISIBLE);
        }
    }

    /**
     * Hide the image review over the live preview for intent captures.
     */
    public void hideIntentReviewImageView() {
        if (mIntentReviewImageView != null) {
            mIntentReviewImageView.setVisibility(View.INVISIBLE);
        }
    }


    public FocusRing getFocusRing() {
        return mFocusRing;
    }

    public void updatePreviewAspectRatio(float aspectRatio) {
        if (aspectRatio <= 0) {
            Log.e(TAG, "Invalid aspect ratio: " + aspectRatio);
            return;
        }
        if (aspectRatio < 1f) {
            aspectRatio = 1f / aspectRatio;
        }

        updateUI(aspectRatio);
        // CID 109127 : FE: Test for floating point equality (FB.FE_FLOATING_POINT_EQUALITY)
        if(java.lang.Math.abs(mAspectRatio - aspectRatio) > 0){
        // if (mAspectRatio != aspectRatio) {
            mAspectRatio = aspectRatio;
            // Update transform matrix with the new aspect ratio.
            mController.updatePreviewAspectRatio(mAspectRatio);
        }
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
        mController.onPreviewUIReady();
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
        // Ignored, Camera does all the work for us
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
        mController.onPreviewUIDestroyed();
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surface) {
    }

    private void initIndicators() {
        // TODO init toggle buttons on bottom bar here
    }

    public void onCameraOpened(CameraCapabilities capabilities, CameraSettings settings) {
        initializeZoom(capabilities, settings);
    }

    public void animateCapture(final byte[] jpegData, int orientation, boolean mirror) {
        // Decode jpeg byte array and then animate the jpeg
        DecodeTask task = new DecodeTask(jpegData, orientation, mirror);
        task.execute();
    }

    // called from onResume but only the first time
    public void initializeFirstTime() {

    }

    // called from onResume every other time
    public void initializeSecondTime(CameraCapabilities capabilities, CameraSettings settings) {
        initializeZoom(capabilities, settings);
        if (mController.isImageCaptureIntent()) {
            hidePostCaptureAlert();
        }
    }

    public void initializeZoom(CameraCapabilities capabilities, CameraSettings settings) {
        if ((capabilities == null) || settings == null ||
                !capabilities.supports(CameraCapabilities.Feature.ZOOM)) {
            return;
        }
        mZoomMax = capabilities.getMaxZoomRatio();
        // Currently we use immediate zoom for fast zooming to get better UX and
        // there is no plan to take advantage of the smooth zoom.
        // TODO: Need to setup a path to AppUI to do this
        mPreviewOverlay.setupZoom(mZoomMax, settings.getCurrentZoomRatio(),
                new ZoomChangeListener());
    }

    public void animateFlash() {
        mController.startPreCaptureAnimation();
    }

    public boolean onBackPressed() {
        Log.i(TAG, "onBackPressed");
        // In image capture mode, back button should:
        // 1) if there is any popup, dismiss them, 2) otherwise, get out of
        // image capture
        if (mController.isImageCaptureIntent()) {
            mController.onCaptureCancelled();
            return true;
        } else if (!mController.isCameraIdle()) {
            Log.i(TAG, "onBackPressed ignore backs while we're taking a picture");
            // ignore backs while we're taking a picture
            return true;
        } else {
            return false;
        }
    }

    public void showCapturedImageForReview(byte[] jpegData, int orientation, boolean mirror) {
        mDecodeTaskForReview = new DecodeImageForReview(jpegData, orientation, mirror);
        mDecodeTaskForReview.execute();

        mActivity.getCameraAppUI().transitionToIntentReviewLayout();
        pauseFaceDetection();
    }

    public void hidePostCaptureAlert() {
        if (mDecodeTaskForReview != null) {
            mDecodeTaskForReview.cancel(true);
        }
        resumeFaceDetection();
    }

    public void setDisplayOrientation(int orientation) {
        if (mFaceView != null) {
            mFaceView.setDisplayOrientation(orientation);
        }
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

    public void hideZoomProcessorIfNeeded() {
        mPreviewOverlay.hideZoomProcessorIfNeeded();
    }

    public void setSwipingEnabled(boolean enable) {
        mActivity.setSwipingEnabled(enable);
    }

    /* SPRD: Fix 474843 Add for Filter Feature @{ */
    public void onResume() {

    }
    /* @} */

    public void onPause() {
        if (mFaceView != null) {
            mFaceView.clear();
        }
        if (mDialog != null) {
            mDialog.dismiss();
        }
        // recalculate aspect ratio when restarting.
        mAspectRatio = 0.0f;
        resetUI();
    }

    public void clearFaces() {
        if (mFaceView != null) {
            mFaceView.clear();
        }
    }

    @Override
    public void pauseFaceDetection() {
        if (mFaceView != null) {
            mFaceView.pause();
        }
    }

    @Override
    public void resumeFaceDetection() {
        if (mFaceView != null) {
            mFaceView.resume();
        }
    }

    public void onStartFaceDetection(int orientation, boolean mirror) {
        if (mFaceView != null) {
            mFaceView.clear();
            mFaceView.setVisibility(View.VISIBLE);
            mFaceView.setDisplayOrientation(orientation);
            mFaceView.setMirror(mirror);
            mFaceView.resume();
        }
    }
    private int mFaceSimleCount = 0;
    @Override
    public void onFaceDetection(Face[] faces, CameraAgent.CameraProxy camera) {
        /**
         * CID 109135 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        int countDownDuration = mActivity.getSettingsManager().getInteger(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_COUNTDOWN_DURATION);
        */
        /* SPRD Bug:492347 Show FaceView in CountDown Mode.@{
        boolean isCountDown = mActivity.getSettingsManager().getBoolean(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_COUNTDOWN_DURATION);
        @}*/
        if (isNeedClearFaceView()/*((mAIController == null || mAIController.isChooseOff()))
                || Keys.isHdrOn(mActivity.getSettingsManager()) || isCountDown*/) {
            if (mFaceView != null) {
                mFaceView.clear();
            }
            return;
        }
        if (mAIController.isChooseFace() && mFaceView != null && faces != null) {
            mFaceView.setFaces(faces);
        } else if (mAIController.isChooseSmile()) {
            if (faces != null && mFaceView != null) {
                int length = faces.length;
                int[] smileCount = new int[length];
                for (int i = 0, len = faces.length; i < len; i++) {
                    Log.i(TAG, " len=" + len + " faces[i].score=" + faces[i].score);
                    // mAIController.resetSmileScoreCount(faces[i].score >= 90);
                    mAIController.resetSmileScoreCount(faces[i].score > 30 && faces[i].score < 100);
                    if (faces[i].score > 30 && faces[i].score < 100) {
                        smileCount[i] = faces[i].score;
                    }
                }
                SurfaceTexture st = mActivity.getCameraAppUI().getSurfaceTexture();
                if (smileCount.length > 0 && smileCount[0] > 30 && smileCount[0] < 100 && !mActivity.isPaused() && st != null && mController.isShutterEnabled() && !mFaceView.isPause()) {
                    mFaceSimleCount ++;
                    if (mFaceSimleCount > 3) {
                        mFaceView.setFaces(faces);
                        mFaceView.clearFacesDelayed();
                        mFaceSimleCount = 0;
                        Log.i(TAG, "smileCount=" + smileCount[0] + "Do Capture ... ...");
                        mController.onShutterButtonClick();
                    }
                }
            }
        }
    }

    public void intializeAIDetection(SettingsManager sm) {
        mAIController = new AIDetectionController(sm);
    }

    public void intializeAIDetection(DataModuleBasic dataModuleCurrent) {

        mAIController = new AIDetectionController();
        mAIController.getChooseValue(dataModuleCurrent.getString(Keys.KEY_CAMERA_AI_DATECT));
    }

    // CID 123779 : UuF: Unused field (FB.UUF_UNUSED_FIELD)
    // private OnScreenHint mBurstHint;
    public void enableBurstScreenHint(boolean enable) {
//        if (enable) {
//            String message =
//                mActivity.getString(R.string.notice_progress_text_burst_mode);
//            if (mBurstHint == null) {
//                mBurstHint = OnScreenHint.makeText(mActivity, message);
//            } else {
//                mBurstHint.setText(message);
//            }
//            mBurstHint.show();
//        } else if (mBurstHint != null) {
//            mBurstHint.cancel();
//            mBurstHint = null;
//        }
    }

    public boolean isReviewShow() {// SPRD BUG:402084
        if (mIntentReviewImageView == null) {
            return false;
        }
        return mIntentReviewImageView.getVisibility() == View.VISIBLE;
    }

    /* SPRD: add for filter function @{ */
    public void setButtonOrientation(OrientationManager.DeviceOrientation DeviceOrientation) { }

    public void setTransformMatrix() {}
    public void updateUI(float aspectRatio) {}
    public void resetUI() {}
    /*  @} */

    /**
     * add For dream camera
     */
    public void initUI() {

    }

    // UI CHECK 71
    public void updateStartCountDownUI() {
        mActivity.getCameraAppUI().updatePreviewUI(View.GONE);
    }

    /**
     * Extract code into function
     *
     * @{
     */
    public boolean isNeedClearFaceView() {
        return mAIController == null || mAIController.isChooseOff();
    }

    /*
     * Add for ui check 122 @{
     */
    public void onOrientationChanged(OrientationManager orientationManager,
            OrientationManager.DeviceOrientation deviceOrientation) {
        int orientation = deviceOrientation.getDegrees();
        DreamOrientation.setOrientation(mRootView, orientation, true);
    }
    /*
     * @}
     */

    /* SPRD: Fix bug 535110, Photo voice record. @{ */
    public void showAutioNoteTip() {}

    public void hideAudioNoteTip() {}

    public void showAudioNoteProgress() {}

    public void hideAudioNoteProgress() {}
    /* @} */
    public void enablePreviewOverlayHint(boolean enable) {
        mPreviewOverlay.setDetector(enable);
    }
    //Sprd Fix Bug: 665197
    public boolean isZooming(){
        if (mPreviewOverlay == null) {
            return false;
        }
        return mPreviewOverlay.isZooming();
    }
}
