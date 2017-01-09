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

import java.util.ArrayList;

import android.app.Dialog;
import android.content.DialogInterface;
import android.content.res.Configuration;
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
import android.net.Uri;

import com.android.camera.MultiToggleImageButton;
import com.android.camera.captureintent.PictureDecoder;
import com.android.camera.debug.DebugPropertyHelper;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.ui.CountDownView;
import com.android.camera.ui.FaceView;
import com.android.camera.ui.PreviewOverlay;
import com.android.camera.ui.PreviewStatusListener;
import com.android.camera.ui.Rotatable;
import com.android.camera.ui.focus.FocusRing;
import com.android.camera2.R;
import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.ex.camera2.portability.CameraSettings;
import com.sprd.camera.freeze.FaceDetectionController;
import com.sprd.camera.panora.WideAnglePanoramaUI;
import com.sprd.camera.aidetection.AIDetectionController;
import com.android.camera.app.AppController;
import com.android.camera.app.OrientationManager;

import android.view.View.OnClickListener;
import android.widget.RelativeLayout;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.view.WindowManager;
import android.view.Display;
import android.widget.Toast;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

import com.android.camera.widget.ModeOptionsOverlay;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.android.camera.util.CameraUtil;
import com.android.camera.ui.RotateImageView;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.DreamOrientation;
import com.dream.camera.DreamUI;
import com.dream.camera.MakeupController;
import com.dream.camera.ButtonManagerDream;

public class PhotoUI extends DreamUI implements PreviewStatusListener,
    CameraAgent.CameraFaceDetectionCallback, PreviewStatusListener.PreviewAreaChangedListener, FaceDetectionController{

    private static final Log.Tag TAG = new Log.Tag("PhotoUI");
    private static final int DOWN_SAMPLE_FACTOR = 4;
    private static final float UNSET = 0f;

    private final PreviewOverlay mPreviewOverlay;
    private final FocusRing mFocusRing;
    protected final CameraActivity mActivity;
    protected final PhotoController mController;
    protected MakeupController mMakeupController;

    protected final View mRootView;
    private Dialog mDialog = null;

    // TODO: Remove face view logic if UX does not bring it back within a month.
    private final FaceView mFaceView;
    private DecodeImageForReview mDecodeTaskForReview = null;

    private float mZoomMax;

    private int mPreviewWidth = 0;
    private int mPreviewHeight = 0;
    private float mAspectRatio = UNSET;

    private ImageView mIntentReviewImageView;
    private AIDetectionController mAIController;//SPRD:Add for ai detect

    private ShutterButton mShutterButton;//SPRD:fix bug 473462

    /*SPRD:fix bug 474672 add for ucam beauty @{ */
    private ImageView mMakeupButton;
    private LinearLayout  mBeautyControllerView;
    private SeekBar mBeautySeekBar;
    private long mLastBeautyTime = 0;
    public static final int UCAM_BEAUTY_MODULE_INDEX = 7;
    /* @} */


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
        int width = right - left;
        int height = bottom - top;
        if (mPreviewWidth != width || mPreviewHeight != height) {
            mPreviewWidth = width;
            mPreviewHeight = height;
        }
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
            /* SPRD: Fix bug 538131 scale the bitmap to fill in the screen @{ */
            if (mIntentReviewImageView instanceof RotateImageView) {
                ((RotateImageView) mIntentReviewImageView).enableScaleup();
            }
            /* @} */
            mIntentReviewImageView.setImageBitmap(bitmap);
            showIntentReviewImageView();

            mDecodeTaskForReview = null;
        }
    }

    public PhotoUI(CameraActivity activity, PhotoController controller, View parent) {
        mActivity = activity;
        mController = controller;
        mRootView = parent;

        ViewGroup moduleRoot = (ViewGroup) mRootView.findViewById(R.id.module_layout);
        mActivity.getLayoutInflater().inflate(R.layout.photo_module,
                 moduleRoot, true);
        initIndicators();

        Log.i(TAG, "PhotoUI  isSupportFilterFeature=" + UCamUtill.isUcamBeautyEnable()+",isImageCaptureIntent="+mController.isImageCaptureIntent());


        mFocusRing = (FocusRing) mRootView.findViewById(R.id.focus_ring);
        mPreviewOverlay = (PreviewOverlay) mRootView.findViewById(R.id.preview_overlay);
        mCountdownView = (CountDownView) mRootView.findViewById(R.id.count_down_view);
        // Show faces if we are in debug mode.
        /*SPRD:Modify for ai detect @{
        if (DebugPropertyHelper.showCaptureDebugUI()) {
        */
            mFaceView = (FaceView) mRootView.findViewById(R.id.face_view);
        /*
        } else {
            mFaceView = null;
        }
        @} */

        if (mController.isImageCaptureIntent()) {
            initIntentReviewImageView();
        }

        mShutterButton = (ShutterButton) mRootView.findViewById(R.id.shutter_button);//SPRD:fix bug 473462

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

        if (mAspectRatio != aspectRatio) {
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
        initializeExposureCompensation(capabilities);
    }

    /*SPRD:fix bug 622818 add for exposure to adapter auto @{*/
    protected float mExposureCompensationStep = 0.0f;
    private void initializeExposureCompensation(CameraCapabilities capabilities) {
        if (!DataModuleManager.getInstance(mActivity).getCurrentDataModule().isEnableSettingConfig(Keys.KEY_EXPOSURE) || capabilities == null) {
            return;
        }

        mExposureCompensationStep = capabilities.getExposureCompensationStep();
        updateExposureUI();
    }

    protected void updateExposureUI() {};
    /* @} */

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
        if(DataModuleManager.getInstance(mActivity).getCurrentDataModule().isEnableSettingConfig(Keys.KEY_DREAM_ZOOM_ENABLE_PHOTO_MODULE) &&
                !DataModuleManager.getInstance(mActivity).getCurrentDataModule().getBoolean(Keys.KEY_DREAM_ZOOM_ENABLE_PHOTO_MODULE)){
            return;
        }
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
        // In image capture mode, back button should:
        // 1) if there is any popup, dismiss them, 2) otherwise, get out of
        // image capture
        if (mController.isImageCaptureIntent()) {
            mController.onCaptureCancelled();
            return true;
        } else if (!mController.isCameraIdle()) {
            // ignore backs while we're taking a picture
            return true;
        } else {
            return false;
        }
    }

    protected void showCapturedImageForReview(byte[] jpegData, int orientation, boolean mirror) {
        mDecodeTaskForReview = new DecodeImageForReview(jpegData, orientation, mirror);
        mDecodeTaskForReview.execute();

        mActivity.getCameraAppUI().transitionToIntentReviewLayout();
        pauseFaceDetection();
    }

    protected void hidePostCaptureAlert() {
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

    public void setSwipingEnabled(boolean enable) {
        mActivity.setSwipingEnabled(enable);
    }

    public void onResume() {

    }

    public void onPause() {
        if (mFaceView != null) {
            mFaceView.clear();
        }
        if (mDialog != null) {
            mDialog.dismiss();
        }
        // recalculate aspect ratio when restarting.
        mAspectRatio = 0.0f;
        dismissMakeup();
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

    /* SPRD:Add for ai detect @{ */
    private int mFaceSimleCount = 0;

    /* SPRD:Modify for add ai detect bug 474723 @{
    @Override
    public void onFaceDetection(Face[] faces, CameraAgent.CameraProxy camera) {
        if (mFaceView != null) {
            mFaceView.setFaces(faces);
        }
    }
    */
    @Override
    public void onFaceDetection(Face[] faces, CameraAgent.CameraProxy camera) {
        /*SPRD:modify for Coverity 109107
         * Orginal android code
        int countDownDuration = mActivity.getSettingsManager().getInteger(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_COUNTDOWN_DURATION);
         */
        /*
         * SPRD Bug:492347 Show FaceView in CountDown Mode. @{
         * Original Android code:

        boolean isCountDown = mActivity.getSettingsManager().getBoolean(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_COUNTDOWN_DURATION);
        if (((mAIController == null || mAIController.isChooseOff()))
                || Keys.isHdrOn(mActivity.getSettingsManager()) || isCountDown) {

         */
        if (isNeedClearFaceView()
                || ((mController instanceof PhotoModule) && !((PhotoModule)mController).mFaceDetectionStarted)
               /* || Keys.isHdrOn(mActivity.getSettingsManager())*/) {//now face is not mutex with ai in UE's doc.
            if (mFaceView != null) {
                mFaceView.clear();
            }
            return;
        }

        // SPRD: Add for new feature VGesture but just empty interface
        if (isDetectView()) {
            doDectView(faces,mFaceView);
        } else if (mAIController.isChooseFace() && mFaceView != null && faces != null) {
            mFaceView.setFaces(faces);
        } else if (mAIController.isChooseSmile()) {
            if (faces != null && mFaceView != null) {
                if (isCountingDown()) {
                    return;
                }
                mFaceView.clear();
                int length = faces.length;
                int[] smileCount = new int[length];
                for (int i = 0, len = faces.length; i < len; i++) {
                    Log.i(TAG, " len=" + len + " faces[i].score=" + faces[i].score);
                    // mAIController.resetSmileScoreCount(faces[i].score >= 90);
                    // SPRD: Fix bug 536674 The smile face score is low.
                    mAIController.resetSmileScoreCount(faces[i].score > 40 && faces[i].score < 100);
                    if (faces[i].score > 40 && faces[i].score < 100) {
                        smileCount[i] = faces[i].score;
                    }
                }
                SurfaceTexture st = mActivity.getCameraAppUI().getSurfaceTexture();
                // SPRD: Fix bug 536674 The smile face score is low.
                if (smileCount.length > 0 && smileCount[0] > 40 && smileCount[0] < 100
                        && !mActivity.isPaused() && st != null && mController.isShutterEnabled()
                        && !mFaceView.isPause()) {
                    mFaceSimleCount++;
                    if (mFaceSimleCount > 5) {
                        mFaceView.setFaces(faces);
                        mFaceView.clearFacesDelayed();
                        mFaceSimleCount = 0;
                        Log.i(TAG, "smileCount=" + smileCount[0] + "Do Capture ... ...");
                        //SPRD: fix bug 613650 close smile capture when open camera settings
                        if(mActivity.getCameraAppUI().isSettingLayoutOpen()){
                            return;
                        }
                        mController.onShutterButtonClick();
                        /* SPRD:fix bug 530633 setCaptureCount after capture @*/
                        mController.setCaptureCount(0);
                    }
                }
            }
        }
    }

    public void intializeAIDetection(DataModuleBasic dataModuleCurrent) {

        mAIController = new AIDetectionController();
        mAIController.getChooseValue(dataModuleCurrent.getString(Keys.KEY_CAMERA_AI_DATECT));
    }
    /* @} */

    /* SPRD:fix bug 473462 add burst capture @*/
    private OnScreenHint mBurstHint;

    public void showBurstScreenHint(int count) {
        String message = String.format(mActivity.getString(R.string.burst_mode_saving), count);
        if (mBurstHint == null) {
            mBurstHint = OnScreenHint.makeText(mActivity, message);
        } else {
            mBurstHint.setText(message);
        }
        mBurstHint.show();
    }

    public void dismissBurstScreenHit() {
        if (mBurstHint != null) {
            mBurstHint.cancel();
            mBurstHint = null;
            enablePreviewOverlayHint(true);
        }
    }

    public boolean isReviewShow() {// SPRD BUG:402084
        if (mIntentReviewImageView == null) {
            return false;
        }
        return mIntentReviewImageView.getVisibility() == View.VISIBLE;
    }

    public boolean isOnTouchInside(MotionEvent ev) {
        if (mShutterButton != null) {
            return mShutterButton.isOnTouchInside(ev);
        }
        return false;
    }
    /* @}*/

    /* SPRD:Fix bug 411062 @{ */
    public void enablePreviewOverlayHint(boolean enable) {
        mPreviewOverlay.setDetector(enable);
    }
    /* @} */


    /*SPRD:Fix bug 502060 hide the ZoomProcessor if it shown after the button clicked @{*/
    public void hideZoomProcessorIfNeeded() {
        mPreviewOverlay.hideZoomProcessorIfNeeded();
    }
    /* SPRD: fix bug 474672 add for beauty @{ */
    private void initFilterMakeupButton() {
        Log.i(TAG, "initFilterMakeupButton  isSupportFilterFeature=" + UCamUtill.isUcamBeautyEnable());

        if(UCamUtill.isUcamBeautyEnable()) {
            mMakeupButton = (ImageView) mRootView.findViewById(R.id.btn_beauty_button);
            if (mMakeupButton == null) {
                return;
            }
            initMakeupButton();

            /*
             * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
             * Original Android code:

            // SPRD Bug:514488 Click Button when rotation.
            mMakeupButton.setOnClickListener(new View.OnClickListener() {

                boolean clickable = true;

                @Override
                public void onClick(View view) {
                    if (!clickable)
                        return;
                    SettingsManager settingsManager = mActivity.getSettingsManager();
                    boolean isHasEnteredBeauty = settingsManager.getBoolean(
                            SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_BEAUTY_ENTERED);
                    Log.i(TAG, "onClick   isHasEnteredBeauty = " + isHasEnteredBeauty);
                    if (!isHasEnteredBeauty) {
                        clickable = false;
                        mActivity.getOrientationManager().setRequestedOrientation(
                                ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
                        ((PhotoModule) mController).postDelayed(new Runnable() {

                            int tryTime = 0;

                            @Override
                            public void run() {
                                if (mActivity.getRequestedOrientation() == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
                                        && mActivity.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {
                                    resumeMakeupControllerView();
                                    clickable = true;
                                } else {
                                    if (++tryTime <= 50) {
                                        ((PhotoModule) mController).postDelayed(this, 20);
                                    } else {
                                        Log.e(TAG,
                                                "onClick setRequestedOrientation PORTRAIT timeout.");
                                        return;
                                    }
                                }
                            }
                        }, 20);
                    } else {
                        clickable = false;
                        pauseMakeupControllerView();
                        mActivity.getOrientationManager().resetOrientaion();
                        ((PhotoModule) mController).postDelayed(new Runnable() {

                            int tryTime = 0;

                            @Override
                            public void run() {
                                if (mActivity.getRequestedOrientation() != ActivityInfo.SCREEN_ORIENTATION_PORTRAIT) {
                                    clickable = true;
                                } else {
                                    if (++tryTime <= 50) {
                                        ((PhotoModule) mController).postDelayed(this, 20);
                                    } else {
                                        Log.e(TAG,
                                                "onClick resetOrientaion timeout.");
                                        return;
                                    }
                                }
                            }
                        }, 20);

                    }
                }
            });

             */

            mMakeupButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    SettingsManager settingsManager = mActivity.getSettingsManager();
                    boolean isHasEnteredBeauty = settingsManager.getBoolean(
                            SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_BEAUTY_ENTERED);
                    Log.i(TAG, "onClick isHasEnteredBeauty = " + isHasEnteredBeauty);
                    if (!isHasEnteredBeauty) {
                        mMakeupController.resumeMakeupControllerView();
                    } else {
                        mMakeupController.pauseMakeupControllerView();
                    }
                }
            });

            mBeautyControllerView = (LinearLayout) mRootView.findViewById(R.id.ucam_makeup_controls);
            mBeautySeekBar = (SeekBar)mRootView.findViewById(R.id.makeup_seekbar);
            mBeautySeekBar.setPadding(8,0,8,0);
            mBeautySeekBar.setOnSeekBarChangeListener(new MakeUpSeekBarChangedListener());
        }
        setButtonOrientation(CameraUtil.getDisplayRotation());
    }

    /* SPRD: fix bug 487525 save makeup level for makeup module
     * Note mBeautySeekBar.getProgress must be int*11 @{
     */
    public void initMakeupLevel() {
        int makeupLevel = mActivity.getSettingsManager().getInteger(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_MAKEUP_MODE_LEVEL,
                mBeautySeekBar.getProgress());
        mBeautySeekBar.setProgress(makeupLevel);
        mController.onBeautyValueChanged(makeupLevel);
    }
    /* @} */
    public void setButtonOrientation(int orientation) {
        Log.i(TAG, "setButtonOrientation   orientation = "+orientation);

        FrameLayout.LayoutParams layoutBeautiy = new FrameLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,Gravity.CENTER);

        float margin = mActivity.getResources().getDimension(R.dimen.filter_make_up_button_magin);
        float buttomsize = mActivity.getResources().getDimension(R.dimen.filter_make_up_button_half_size);

        WindowManager wm = (WindowManager) mActivity.getSystemService(Context.WINDOW_SERVICE);
        Display display = wm.getDefaultDisplay();
        int width = display.getWidth();
        // CID 123679 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // int height = display.getHeight();
        float mPort = width/2 - margin - buttomsize;
        // CID 123679 : DLS: Dead local store (FB.DLS_DEAD_LOCAL_STORE)
        // float mLand = height/2 - margin - buttomsize;

        /*
         * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
         * Original Android code:

        if (orientation == 0){
            layoutBeautiy.setMargins((int)mPort,0,0,0);
        } else if(orientation == 90){
            layoutBeautiy.setMargins(0,0,0,(int)mLand);
        } else if(orientation == 180){
            layoutBeautiy.setMargins(0,0,(int)mPort,0);
        } else if(orientation == 270){
            layoutBeautiy.setMargins(0,(int)mLand,0,0);
        }

         */
        if (mMakeupButton != null) {
            layoutBeautiy.setMargins((int) mPort, 0, 0, 0);
            mMakeupButton.setLayoutParams(layoutBeautiy);
        }


    }

    public void setFilterMakeupButtonEnabled(boolean enable) {
        if (mMakeupButton != null) {
            mMakeupButton.setEnabled(enable);
        }
    }

    /**
     * SPRD: Fix bug 513768 makeup module switch to normal photo module, makeup effect still works. @{
     */
    private void doMakeupLevel() {
        boolean isMakeupEntry = mActivity.getSettingsManager().getBoolean(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_BEAUTY_ENTERED, false);

        if (isMakeupEntry) {
            initMakeupLevel();
        } else {
            mController.onBeautyValueReset();
        }
    }
    /* @} */

    private boolean initMakeupButton() {
        mMakeupButton = (ImageView) mRootView.findViewById(R.id.btn_beauty_button);
        if (mMakeupButton != null) {
            mMakeupButton.setVisibility(View.VISIBLE);
            setFilterMakeupButtonEnabled(true);
            return true;
        }
        return false;
    }

    /*
     * Add mBeautyControllerView reason: PhotoUI in resume makeup module, change to videoUI, makeup
     * progress bar shows. Expected: videoUI doesn't show any makeup ui.
     * @{
     */
    private void dismissMakeup() {
        if (mMakeupButton != null) {
            mMakeupButton.setVisibility(View.GONE);
        }

        if (mBeautyControllerView != null) {
            mBeautyControllerView.setVisibility(View.GONE);
        }
        // SPRD Bug: 512594
        /*
         * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
         * Original Android code:

        mActivity.getOrientationManager().resetOrientaion();

         */

        ModeOptionsOverlay mo = (ModeOptionsOverlay) mRootView
                .findViewById(R.id.mode_options_overlay);
        mo.setPadding(0, 0, 0, 0);
        mo.requestLayout();
    }

    /* @} */

     class MakeUpSeekBarChangedListener implements OnSeekBarChangeListener {
        @Override
        public void onProgressChanged(SeekBar SeekBarId, int progress,
                boolean fromUser) {
            /* SPRD: too many beauty set actions will result "Camera master
             * thread job queue full" error thrown @{ */
            long currentTime = System.currentTimeMillis();
            long delta = currentTime - mLastBeautyTime;
            /*if (delta > 100) {
                mLastBeautyTime = currentTime;
            } else {
                return;
            }*/
            /* @} */
            int offset = progress % 11;
            int set = progress / 11;
            int parameterLevel = 0;
            if (offset == 0) {
                parameterLevel = progress;
            } else if (offset <= 5) {
                mBeautySeekBar.setProgress(set * 11);
                parameterLevel = set * 11;
            } else {
                mBeautySeekBar.setProgress(set * 11 + 11);
                parameterLevel = set * 11 + 11;
            }
            if (delta > 100) {
                mLastBeautyTime = currentTime;
                mController.onBeautyValueChanged(parameterLevel);
            }
            // TODO Auto-generated method stub
        }

        @Override
        public void onStartTrackingTouch(SeekBar SeekBarId) {
            // TODO Auto-generated method stub
        }

        @Override
        public void onStopTrackingTouch(SeekBar SeekBarId) {
         // SPRD: fix bug 487525 save makeup level for makeup module
            mController.onBeautyValueChanged(mBeautySeekBar.getProgress());
        }
    }
    /* @} */

    /*
     * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
     */
    public void showPanels() {

    }

    public void hidePanels() {

    }
    /* @} */

    public void initUI() {

    }

    public ViewGroup getModuleRoot(){
        return (ViewGroup) mRootView.findViewById(R.id.dream_module_layout);
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

    public boolean isDetectView(){
        return false;
    }

    public void doDectView(Face[] faces, FaceView faceView) {

    }

    public View getRootView() {
        return mRootView;
    }
    /**
     * @}
     */

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

    public void onSettingReset() {}
    public boolean getBurstHintVisibility() {
        return mBurstHint != null;
    }

    public void setButtonVisibility(int buttonId, int visibility) {
        ((ButtonManagerDream)mActivity.getButtonManager()).setButtonVisibility(buttonId,visibility);
    }

    protected void setPictureInfo(int width, int height, int orientation){}//SPRD:fix bug 625571
    //Sprd Fix Bug: 665197
    public boolean isZooming(){
        if (mPreviewOverlay == null) {
            return false;
        }
        return mPreviewOverlay.isZooming();
    }
}
