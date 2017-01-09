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

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Bitmap;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.hardware.Camera.Face;
import android.os.AsyncTask;
import android.os.Handler;
import android.view.Display;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import com.android.camera.app.AppController;
import com.android.camera.app.OrientationManager;
import com.android.camera.captureintent.PictureDecoder;
import com.android.camera.debug.Log;
import com.android.camera.hardware.HardwareSpec;
import com.android.camera.module.ModuleController;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.ui.CountDownView;
import com.android.camera.ui.FaceView;
import com.android.camera.ui.PhotoVoiceRecordProgress;
import com.android.camera.ui.PreviewOverlay;
import com.android.camera.ui.PreviewStatusListener;
import com.android.camera.ui.RotateImageView;
import com.android.camera.ui.focus.FocusRing;
import com.android.camera.util.CameraUtil;
import com.android.camera.widget.ModeOptionsOverlay;
import com.android.camera2.R;
import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.ex.camera2.portability.CameraSettings;
import com.sprd.camera.aidetection.AIDetectionController;
import com.sprd.camera.freeze.FaceDetectionController;
import com.sprd.hz.selfportrait.detect.DetectEngine;
import com.sprd.hz.selfportrait.util.ContextUtil;
import com.sprd.hz.selfportrait.util.SoundUtil;
import com.sprd.ucam.vgesutre.DetectView;
import com.sprd.ucam.vgesutre.TimerCountView;
import com.ucamera.ucam.modules.ui.UcamFilterPhotoUI;
import com.ucamera.ucam.modules.utils.UCamUtill;


public class PhotoUI implements PreviewStatusListener,
    CameraAgent.CameraFaceDetectionCallback, PreviewStatusListener.PreviewAreaChangedListener, FaceDetectionController,
    OrientationManager.OnOrientationChangeListener{

    private static final Log.Tag TAG = new Log.Tag("PhotoUI");
    private static final int DOWN_SAMPLE_FACTOR = 4;
    private static final float UNSET = 0f;

    private final PreviewOverlay mPreviewOverlay;
    private final FocusRing mFocusRing;
    private final CameraActivity mActivity;
    private final PhotoController mController;

    private final View mRootView;
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
    // SPRD: Fix 474843 Add for Filter Feature
    private ImageView mFilterButton;

    /*SPRD:fix bug 474672 add for ucam beauty @{ */
    private ImageView mMakeupButton;
    private LinearLayout  mBeautyControllerView;
    private SeekBar mBeautySeekBar;
    private long mLastBeautyTime = 0;
    public static final int UCAM_BEAUTY_MODULE_INDEX = 7;
    /* @} */

    /* SPRD: New feature vgesture detect @{ */
    private DetectView mDetectView;
    private ImageView mViewMode = null;
    private ImageView mGestureButton = null;
    private RotateImageView mGestureGuideView = null; // SPRD: Fix bug 581382
    private TextView mGestureHelpView = null;
    protected TimerCountView mTimerView = null;
    protected DetectEngine mDetectEngine = null;
    private boolean mIsHideGuide = false;
    private boolean mIsHideHelp = true;
    private boolean mVGestureStarted = false;
    private RelativeLayout mVgesture = null;
    private SoundUtil mSoundUtil = null;
    private AppController mAppController = null;
    private CameraAgent.CameraProxy mCameraDevice = null;
    private int mVgesturePreviewWidth = 0;
    private int mVgesturePreviewHeight = 0;
    /* @} */

    // SPRD: Fix bug 535110, Photo voice record.
    private PhotoVoiceRecordProgress mPhotoVoiceRecordProgress;

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
        mCountdownView.cancelCountDown();
    }

    @Override
    public void onPreviewAreaChanged(RectF previewArea) {
        if (mFaceView != null) {
            mFaceView.onPreviewAreaChanged(previewArea);
        }
        mCountdownView.onPreviewAreaChanged(previewArea);
        /* SPRD: New feature vgesture detect @{ */
        if (mDetectEngine != null) {
            mDetectEngine.onPreviewAreaChanged(previewArea);
        }
        /* @} */
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
        mAppController = mActivity;

        ViewGroup moduleRoot = (ViewGroup) mRootView.findViewById(R.id.module_layout);
        mActivity.getLayoutInflater().inflate(R.layout.photo_module,
                 moduleRoot, true);
        /* SPRD: fix bug539498 shows gap between Video icon area and option button area @{ */
        ModeOptionsOverlay mo = (ModeOptionsOverlay) mRootView.findViewById(R.id.mode_options_overlay);
        mo.setPadding(0,0,0,0);
        /* @} */
        initIndicators();

        Log.i(TAG, "PhotoUI  isSupportFilterFeature=" + UCamUtill.isUcamBeautyEnable()+",isImageCaptureIntent="+mController.isImageCaptureIntent());

        initFilterMakeupButton();// SPRD: fix bug 474672

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

            /* SPRD: Fix 474843 Add for Filter Feature @{ */
            if (mFilterButton != null) {
                mFilterButton.setVisibility(View.GONE);
            }
            /* @} */

            /* SPRD: Add for bug 563611 @{ */
            if (mMakeupButton != null) {
                mMakeupButton.setVisibility(View.GONE);
            }
            /* @} */
        }

        mShutterButton = (ShutterButton) mRootView.findViewById(R.id.shutter_button);//SPRD:fix bug 473462

        // SPRD: Fix bug 535110, Photo voice record.
        mPhotoVoiceRecordProgress = (PhotoVoiceRecordProgress) mRootView.findViewById(R.id.photo_voice_record_progress);
    }

    public boolean isUcamBeautyCanBeUsed() {
        Log.i(TAG, "isUcamBeautyCanBeUsed  isUcamBeautyEnable=" + UCamUtill.isUcamBeautyEnable()+",isImageCaptureIntent="+mController.isImageCaptureIntent());
        return UCamUtill.isUcamBeautyEnable() && !mController.isImageCaptureIntent();
    }

    /* SPRD: Fix 474843 Add for Filter Feature @{ */
    public boolean isUcamFilterCanBeUsed() {
        Log.i(TAG, "isUcamFilterCanBeUsed  isUcamFilterCanBeUsed=" + UCamUtill.isUcamFilterEnable()+",isImageCaptureIntent="+mController.isImageCaptureIntent());
        return UCamUtill.isUcamFilterEnable() && !mController.isImageCaptureIntent();
    }
    /* @} */

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
        /* SPRD: fix bug 474672 add for beauty @{ */
        if (isUcamBeautyCanBeUsed()) {
            initMakeupButton();
            if (mActivity.getSettingsManager().getBoolean(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_BEAUTY_ENTERED)) {
                resumeMakeupControllerView();
            } else {
                pauseMakeupControllerView();
            }
        }
        /* @} */
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

    @Override
    public void onOrientationChanged(OrientationManager orientationManager,
            OrientationManager.DeviceOrientation deviceOrientation) {
        rotateUI();
        if (mDetectEngine!=null) {
            updateDetectEngineDisplayOritation();
        }
    }

    public void setDisplayOrientation(int orientation) {
        if (mFaceView != null) {
            mFaceView.setDisplayOrientation(orientation);
        }
        /* SPRD: New feature vgesture detect @{ */
        if (mDetectEngine!=null) {
            updateDetectEngineDisplayOritation();
        }
        /* @} */
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

    public void setSwipingEnabled(boolean enable) {
        mActivity.setSwipingEnabled(enable);
    }

    public void onPause() {
        if (mFaceView != null) {
            mFaceView.clear();
        }
        if (mDialog != null) {
            mDialog.dismiss();
        }
        /* SPRD: New feature vgesture detect @{ */
        if (mVGestureStarted && mDetectEngine != null) {
            mDetectEngine.destroy();
        }
        if (mVgesture!=null) {
            mVgesture.setVisibility(View.INVISIBLE);
        }
        if (mTimerView!=null) {
            mTimerView.stopTimer();
        }
        /* @} */
        /* SPRD: Add for bug 569343 (487754 in 5.1) @{ */
        if (mDetectView != null) {
            mDetectView.clear();
        }
        /* @} */
        // recalculate aspect ratio when restarting.
        mAspectRatio = 0.0f;

        dismissMakeupFilter();
        // SPRD: Fix bug 535110, Photo voice record.
        hideAudioNoteTip();
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
        if ((!mVGestureStarted && (mAIController == null || mAIController.isChooseOff()))
               /* || Keys.isHdrOn(mActivity.getSettingsManager())*/) {//now face is not mutex with ai in UE's doc.
            if (mFaceView != null) {
                mFaceView.clear();
            }
            return;
        }
        // SPRD: Add for new feature VGesture
        if (mVGestureStarted) {
            if (mDetectEngine!=null&&faces != null) {
                mDetectView.onDetectFace(mFaceView.getFaceRect(faces));
                mDetectEngine.setFaces(mFaceView.getFaceRect(faces),mFaceView.getRealArea());
                }
        } else if (mAIController.isChooseFace() && mFaceView != null && faces != null) {
            mFaceView.setFaces(faces);
        } else if (mAIController.isChooseSmile()) {
            if (faces != null && mFaceView != null) {
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
                        mController.onShutterButtonClick();
                        /* SPRD:fix bug 530633 setCaptureCount after capture @*/
                        mController.setCaptureCount(0);
                    }
                }
            }
        }
    }

    public void intializeAIDetection(SettingsManager sm) {
        mAIController = new AIDetectionController(sm);
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
                    /* SPRD: Fix bug 563064 @{ */
                    boolean isHasEnteredFilter = settingsManager.getBoolean(
                            SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_FILTER_ENTERED);
                    if (isHasEnteredFilter) return;
                    /* @} */
                    boolean isHasEnteredBeauty = settingsManager.getBoolean(
                            SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_BEAUTY_ENTERED);
                    Log.i(TAG, "onClick isHasEnteredBeauty = " + isHasEnteredBeauty);
                    if (!isHasEnteredBeauty) {
                        resumeMakeupControllerView();
                    } else {
                        pauseMakeupControllerView();
                    }
                    /* SPRD: Fix bug 569018 @{ */
                    ModuleController moduleController = mActivity.getCurrentModuleController();
                    mActivity.getCameraAppUI().applyModuleSpecs(moduleController.getHardwareSpec(),
                            moduleController.getBottomBarSpec());
                    /* @} */
                }
            });

            mBeautyControllerView = (LinearLayout) mRootView.findViewById(R.id.ucam_makeup_controls);
            mBeautySeekBar = (SeekBar)mRootView.findViewById(R.id.makeup_seekbar);
            mBeautySeekBar.setPadding(8,0,8,0);
            mBeautySeekBar.setOnSeekBarChangeListener(new MakeUpSeekBarChangedListener());
        }
        /* SPRD: Fix 474843 Add for Filter Feature @{ */
        if (UCamUtill.isUcamFilterEnable()) {
            initFilterButton();
        }
        /* @} */
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

        // SPRD: Add for Filter Feature
        FrameLayout.LayoutParams layoutFilter = new FrameLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,Gravity.CENTER);

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

        // SPRD: Fix 474843 Add for Filter Feature
        if (mFilterButton != null) {
            layoutFilter.setMargins(0, 0, (int) mPort, 0);
            mFilterButton.setLayoutParams(layoutFilter);
        }
        /* @} */
    }

    public void setFilterMakeupButtonEnabled(boolean enable) {
        if (mMakeupButton != null) {
            mMakeupButton.setEnabled(enable);
        }
        if (mFilterButton != null) {
            mFilterButton.setEnabled(enable);
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

    private void initMakeupButton() {
        mMakeupButton = (ImageView) mRootView.findViewById(R.id.btn_beauty_button);
        if (mMakeupButton != null) {
            mMakeupButton.setVisibility(View.VISIBLE);
            setFilterMakeupButtonEnabled(true);
        }
    }

    /* SPRD: Fix 474843 Add for Filter Feature @{ */
    private void initFilterButton() {
        mFilterButton = (ImageView) mRootView.findViewById(R.id.btn_filter_button);
        if (mFilterButton != null) {
            mFilterButton.setVisibility(View.VISIBLE);
            mFilterButton.setEnabled(true);
        }
    }
    /* @} */

    private void resumeMakeupControllerView() {
        Log.i(TAG, "resumeMakeupControllerView");
        SettingsManager settingsManager = mActivity.getSettingsManager();
        settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_BEAUTY_ENTERED, true);

        doMakeupLevel();//SPRD: Fix bug 513768
        // SPRD Bug: 512594
        /*
         * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
         * Original Android code:

        mActivity.getOrientationManager().setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

         */

        mActivity.getCameraAppUI().setBottomBarShutterIcon(UCAM_BEAUTY_MODULE_INDEX);

        int bottom = mActivity.getResources().getDimensionPixelSize(R.dimen.mode_option_overlay_makeup_height);
        ModeOptionsOverlay mo = (ModeOptionsOverlay) mRootView.findViewById(R.id.mode_options_overlay);
        mo.setPadding(0,0,0,bottom);
        mo.requestLayout();

        mBeautyControllerView.setVisibility(View.VISIBLE);

        /* SPRD add for bug 533661 @{ */
        String burstNumber = settingsManager.getString(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_CONTINUE_CAPTURE);
        if (!("one".equals(burstNumber))) {
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_CONTINUE_CAPTURE);
        }

        /* SPRD: Fix bug 545097, Add V gestures and beauty mutex  @{ */
        if (Keys.isVGestureOn(settingsManager)) {
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_VGESTURE);
            ((PhotoModule) mController).updateVGesture();
        }

        if (UCamUtill.isVgestureEnnable()) {
            ButtonManager buttonManager = mActivity.getButtonManager();
            buttonManager.disableButton(ButtonManager.BUTTON_VGESTURE);
        }

        /* @} */

        /* SPRD: Fix 474843 Add for Filter Feature @{ */
        if (mFilterButton != null) {
            mFilterButton.setVisibility(View.GONE);
        }
        /* @} */
    }

    private void pauseMakeupControllerView() {
        Log.i(TAG, "pasueMakeupControllerView");
        SettingsManager settingsManager = mActivity.getSettingsManager();
        settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_BEAUTY_ENTERED, false);

        doMakeupLevel();//SPRD: Fix bug 513768
        // SPRD Bug: 512594
        /*
         * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
         * Original Android code:

        mActivity.getOrientationManager().resetOrientaion();

         */

        mActivity.getCameraAppUI().setBottomBarShutterIcon(0);

        ModeOptionsOverlay mo = (ModeOptionsOverlay) mRootView.findViewById(R.id.mode_options_overlay);
        mo.setPadding(0,0,0,0);
        mo.requestLayout();

        mBeautyControllerView.setVisibility(View.GONE);

        /* SPRD: Fix 474843 Add for Filter Feature @{ */
        if (mFilterButton != null) {
            mFilterButton.setVisibility(View.VISIBLE);
        }
        /* @} */
    }

    /*
     * Add mBeautyControllerView reason: PhotoUI in resume makeup module, change to videoUI, makeup
     * progress bar shows. Expected: videoUI doesn't show any makeup ui.
     * @{
     */
    private void dismissMakeupFilter() {
        if (mMakeupButton != null) {
            mMakeupButton.setVisibility(View.GONE);
        }

        /* SPRD: Fix 474843 Add for Filter Feature @{ */
        if (mFilterButton != null) {
            mFilterButton.setVisibility(View.GONE);
        }
        /* @} */
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
        if (isUcamBeautyCanBeUsed()) {
            if (mActivity.getSettingsManager().getBoolean(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_BEAUTY_ENTERED) && mBeautyControllerView != null) {
                mBeautyControllerView.setVisibility(View.VISIBLE);
            }
        }
    }

    public void hidePanels() {
        if (isUcamBeautyCanBeUsed()) {
            if (mActivity.getSettingsManager().getBoolean(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_BEAUTY_ENTERED) && mBeautyControllerView != null) {
                mBeautyControllerView.setVisibility(View.INVISIBLE);
            }
        }
    }
    /* @} */

    /* SPRD: New Feature for VGesture @{ */
    public void onStartVGestureDetection(int orientation, boolean mirror,CameraAgent.CameraProxy cameraDevice,Handler h,int PreviewWidth,int PreviewHeight){
        mCameraDevice = cameraDevice;
        mVgesturePreviewWidth = PreviewWidth;
        mVgesturePreviewHeight = PreviewHeight;
        initControls(h,PreviewWidth,PreviewWidth);
        mDetectEngine.startDetect(cameraDevice,PreviewWidth,PreviewHeight);
        rotateUI();
        updateDetectEngineDisplayOritation();
        mDetectEngine.setMirror(mirror);
    }

    public void onStopVGestureDetection(){
        if (mDetectEngine != null) {
            mDetectEngine.destroy();
        }
        if (mVgesture != null) {
            mVgesture.setVisibility(View.INVISIBLE);
        }
        /* SPRD: Add for bug 569343 (487754 in 5.1) @{ */
        if (mDetectView != null) {
            mDetectView.clear();
        }
        /* @} */
    }

    public void showGuide(boolean bShow) {
        if (bShow) {
            mGestureGuideView.setVisibility(View.VISIBLE);
            mGestureButton.setImageResource(R.drawable.guideline_on);
            mIsHideGuide = false;
        } else {
            mGestureGuideView.setVisibility(View.INVISIBLE);
            mGestureButton.setImageResource(R.drawable.guideline_off);
            mIsHideGuide = true;
        }
    }

    public void showHelp(boolean bShow) {
        if (bShow) {
            mGestureHelpView.setVisibility(View.VISIBLE);
            mViewMode.setImageResource(R.drawable.information_on);
            mIsHideHelp = false;
        } else {
            mGestureHelpView.setVisibility(View.INVISIBLE);
            mViewMode.setImageResource(R.drawable.information_off);
            mIsHideHelp = true;
        }
    }

    public void initControls(Handler h, int PreviewWidth, int PreviewHeight) {
        mVgesture = (RelativeLayout) mRootView
                .findViewById(R.id.vgesture_root_layout);
        // SPRD: fix the bug 567527 the V-gesture layout will display before PhotoModule preview
        View filterGLView = mRootView.findViewById(R.id.advanced_filter_glview);
        if (filterGLView != null && filterGLView.getVisibility() == View.VISIBLE) {
            new Handler().postDelayed(new Runnable() {
                public void run() {
                    mVgesture.setVisibility(View.VISIBLE);
                }
            }, UcamFilterPhotoUI.FILTER_FREEZE_SCREEN_DELAY_TIME);
        } else {
            mVgesture.setVisibility(View.VISIBLE);
        }

        ContextUtil.getInstance().appContext = mAppController
                .getAndroidContext();
        RelativeLayout.LayoutParams param = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.MATCH_PARENT,
                RelativeLayout.LayoutParams.MATCH_PARENT);
        RelativeLayout.LayoutParams timeparam = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT);
        timeparam.addRule(RelativeLayout.CENTER_IN_PARENT, RelativeLayout.TRUE);
        mTimerView = new TimerCountView(mActivity);
        mVgesture.addView(mTimerView, timeparam);
        mTimerView.setHandler(h);
//        mDetectView = new DetectView(mActivity);
        mDetectView = (DetectView)mRootView.findViewById(R.id.camera_view_detect);
//        mVgesture.addView(mDetectView, param);
        mViewMode = (ImageView) mVgesture.findViewById(R.id.camera_mode_view);
        mGestureButton = (ImageView) mVgesture
                .findViewById(R.id.camera_gesture_button);
        mGestureGuideView = (RotateImageView) mVgesture
                .findViewById(R.id.camera_gesture_image);
        mGestureGuideView.enableScaleup(); // SPRD: Fix bug 581382
        mActivity.getOrientationManager().addOnOrientationChangeListener(this);
        mGestureHelpView = (TextView) mVgesture
                .findViewById(R.id.camera_help_text);
        mDetectView.setHandler(h);
        mDetectEngine = new DetectEngine();
        mDetectEngine.setHandler(h);
        mDetectEngine.setDisplaySize(PreviewWidth, PreviewHeight);
        mDetectEngine.setDetectType(DetectEngine.DETECT_TYPE_GESTURE);
        mDetectView.setEngine(mDetectEngine);
        mViewMode.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                showHelp(mIsHideHelp);
            }
        });
        mGestureButton.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                showGuide(mIsHideGuide);
            }
        });
    }

    /*
    public void setOrientationChanged(int orientation, int orientation_unknow) {
        if (mVGestureStarted) {
            int mOrientationCompensation = 0;
            int mOrientation = orientation_unknow;
            mOrientation = CameraUtil.roundOrientation(orientation,
                    mOrientation);
            // When the screen is unlocked, display rotation may change. Always
            // calculate the up-to-date orientationCompensation.
            int orientationCompensation = mOrientation
                    + CameraUtil.getDisplayRotation();
            if (mOrientationCompensation != orientationCompensation) {
                mOrientationCompensation = orientationCompensation;
                // setOrientationIndicator(mOrientationCompensation);
                if (mDetectEngine != null)
                    mDetectEngine.setDeviceRotation(mOrientation);
                rotateUI(mOrientation);
            }
        }
    }
     */

    public void rotateUI() {
        int orientation = mAppController.getOrientationManager().getDeviceOrientation().getDegrees();
        if (orientation == 90) {
            orientation = 270;
        } else if (orientation == 270) {
            orientation = 90;
        }
        if (mTimerView != null) {
            mTimerView.setRotation(orientation);
        }
        // SPRD: remove for 467556, GestureGuideView has been rotated in layout
        // mGestureGuideView.setRotation(degrees);
    }

    public void setVGestureStart(boolean isStart) {
        if (isStart) {
            mVGestureStarted = true;
        } else {
            mVGestureStarted = false;
        }
    }

    public void startTimer(int detectMode) {
        mDetectEngine.stopDetect();
        // If the timer is already counting, start capture directly.
        if (!mTimerView.isTimerCounting()) {
            mTimerView.startTimer();
        } else {
            mTimerView.startTimerFromDetector(detectMode);
        }
    }

    public void startDetect(){
        if (mDetectEngine!=null) {
            mDetectEngine.startDetect(mCameraDevice, mVgesturePreviewWidth, mVgesturePreviewHeight);
        }
    }

    public void stopDetect(){
        if (mDetectEngine!=null) {
            mDetectEngine.stopDetect();
        }
    }

    public void updateDetectEngineDisplayOritation(){
        int orientation = mAppController.getOrientationManager().getDeviceOrientation().getDegrees();
        orientation += 90;
        mDetectEngine.setDisplayOritation(orientation%360);
    }
    /* @} */

    /* SPRD: Fix bug 535110, Photo voice record. @{ */
    public void showAutioNoteTip() {
        mPhotoVoiceRecordProgress.showTip();
    }

    public void hideAudioNoteTip() {
        mPhotoVoiceRecordProgress.hideTip();
    }

    public void showAudioNoteProgress() {
        mPhotoVoiceRecordProgress.startVoiceRecord();
        mActivity.getCameraAppUI().showStopRecordVoiceButton();
        mActivity.getCameraAppUI().hideCaptureIndicator();
        if (mMakeupButton != null) {
            mMakeupButton.setVisibility(View.GONE);
        }

        /* SPRD: Fix 474843 Add for Filter Feature @{ */
        if (mFilterButton != null) {
            mFilterButton.setVisibility(View.GONE);
        }
        /* @} */
        if (mBeautyControllerView != null) {
            mBeautyControllerView.setVisibility(View.GONE);
        }
        ModeOptionsOverlay mo = (ModeOptionsOverlay) mRootView
                .findViewById(R.id.mode_options_overlay);
        mo.setVisibility(View.GONE);
    }

    public void hideAudioNoteProgress() {
        mPhotoVoiceRecordProgress.stopVoiceRecord();
        mActivity.getCameraAppUI().hideStopRecordVoiceButton();
        if (mMakeupButton != null) {
            mMakeupButton.setVisibility(View.VISIBLE);
        }

        /* SPRD: Fix 474843 Add for Filter Feature @{ */
        if (mFilterButton != null
                && !mActivity.getSettingsManager().getBoolean(SettingsManager.SCOPE_GLOBAL,
                     Keys.KEY_CAMERA_BEAUTY_ENTERED)) {
            mFilterButton.setVisibility(View.VISIBLE);
        }
        /* @} */
        if (mActivity.getSettingsManager().getBoolean(SettingsManager.SCOPE_GLOBAL,
                Keys.KEY_CAMERA_BEAUTY_ENTERED) && mBeautyControllerView != null) {
            mBeautyControllerView.setVisibility(View.VISIBLE);
        }
        ModeOptionsOverlay mo = (ModeOptionsOverlay) mRootView
                .findViewById(R.id.mode_options_overlay);
        mo.setVisibility(View.VISIBLE);
    }
    /* @} */
}
