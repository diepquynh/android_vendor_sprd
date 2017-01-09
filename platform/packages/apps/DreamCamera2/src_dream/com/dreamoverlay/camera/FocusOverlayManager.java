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

import android.graphics.Rect;
import android.graphics.RectF;
import android.hardware.Camera.Area;
import android.media.MediaActionSound;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;

import com.android.camera.app.AppController;
import com.android.camera.app.MotionManager;
import com.android.camera.debug.Log;
import com.android.camera.one.Settings3A;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.ui.PreviewStatusListener;
import com.android.camera.ui.TouchCoordinate;
import com.android.camera.util.ApiHelper;
import com.android.camera.ui.focus.CameraCoordinateTransformer;
import com.android.camera.ui.focus.FocusRing;
import com.android.camera.util.CameraUtil;
import com.android.camera.stats.UsageStatistics;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.dream.camera.settings.DataModuleManager;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;

/* A class that handles everything about focus in still picture mode.
 * This also handles the metering area because it is the same as focus area.
 *
 * The test cases:
 * (1) The camera has continuous autofocus. Move the camera. Take a picture when
 *     CAF is not in progress.
 * (2) The camera has continuous autofocus. Move the camera. Take a picture when
 *     CAF is in progress.
 * (3) The camera has face detection. Point the camera at some faces. Hold the
 *     shutter. Release to take a picture.
 * (4) The camera has face detection. Point the camera at some faces. Single tap
 *     the shutter to take a picture.
 * (5) The camera has autofocus. Single tap the shutter to take a picture.
 * (6) The camera has autofocus. Hold the shutter. Release to take a picture.
 * (7) The camera has no autofocus. Single tap the shutter and take a picture.
 * (8) The camera has autofocus and supports focus area. Touch the screen to
 *     trigger autofocus. Take a picture.
 * (9) The camera has autofocus and supports focus area. Touch the screen to
 *     trigger autofocus. Wait until it times out.
 * (10) The camera has no autofocus and supports metering area. Touch the screen
 *     to change metering area.
 */
public class FocusOverlayManager implements PreviewStatusListener.PreviewAreaChangedListener,
        MotionManager.MotionListener {
    private static final Log.Tag TAG = new Log.Tag("FocusOverlayMgr");

    private static final int RESET_TOUCH_FOCUS = 0;

    private static final int RESET_TOUCH_FOCUS_DELAY_MILLIS = Settings3A.getFocusHoldMillis();

    public static final float AF_REGION_BOX = Settings3A.getAutoFocusRegionWidth();
    public static final float AE_REGION_BOX = Settings3A.getMeteringRegionWidth();

    private int mState = STATE_IDLE;
    private static final int STATE_IDLE = 0; // Focus is not active.
    private static final int STATE_FOCUSING = 1; // Focus is in progress.
    // Focus is in progress and the camera should take a picture after focus finishes.
    private static final int STATE_FOCUSING_SNAP_ON_FINISH = 2;
    private static final int STATE_SUCCESS = 3; // Focus finishes and succeeds.
    private static final int STATE_FAIL = 4; // Focus finishes and fails.

    private boolean mInitialized;
    private boolean mFocusAreaSupported;
    private boolean mMeteringAreaSupported;
    private boolean mLockAeAwbNeeded;
    private boolean mAeAwbLock;
    private CameraCoordinateTransformer mCoordinateTransformer;

    private boolean mMirror; // true if the camera is front-facing.
    private int mDisplayOrientation;
    private List<Area> mFocusArea; // focus area in driver format
    private List<Area> mMeteringArea; // metering area in driver format
    private CameraCapabilities.FocusMode mFocusMode;
    private final List<CameraCapabilities.FocusMode> mDefaultFocusModes;
    private CameraCapabilities.FocusMode mOverrideFocusMode;
    private CameraCapabilities mCapabilities;
    private final AppController mAppController;
    private final SettingsManager mSettingsManager;
    private final Handler mHandler;
    Listener mListener;
    TouchListener mTouchListener;
    private boolean mPreviousMoving;
    private final FocusRing mFocusRing;
    private final Rect mPreviewRect = new Rect(0, 0, 0, 0);
    private boolean mFocusLocked;

    private MediaActionSound mCameraSound;

    /** Manual tap to focus parameters */
    private TouchCoordinate mTouchCoordinate;
    private long mTouchTime;

    public interface Listener {
        public void autoFocus();
        public void cancelAutoFocus();
        public boolean capture();
        public void startFaceDetection();
        public void stopFaceDetection();
        public void setFocusParameters();
    }
    /* Dream Camera test ui check 20 @{ */
    public interface TouchListener {
        public void touchCapture();
    }
    /* @} */

    /**
     * TODO: Refactor this so that we either don't need a handler or make
     * mListener not be the activity.
     */
    private static class MainHandler extends Handler {
        /**
         * The outer mListener at the moment is actually the CameraActivity,
         * which we would leak if we didn't break the GC path here using a
         * WeakReference.
         */
        final WeakReference<FocusOverlayManager> mManager;
        public MainHandler(FocusOverlayManager manager, Looper looper) {
            super(looper);
            mManager = new WeakReference<FocusOverlayManager>(manager);
        }

        @Override
        public void handleMessage(Message msg) {
            FocusOverlayManager manager = mManager.get();
            if (manager == null) {
                return;
            }

            switch (msg.what) {
                case RESET_TOUCH_FOCUS: {
                    manager.cancelAutoFocus();
                    //manager.mListener.startFaceDetection();//SPRD:Modify for ai detect
                    break;
                }
            }
        }
    }

    public FocusOverlayManager(AppController appController,
            List<CameraCapabilities.FocusMode> defaultFocusModes, CameraCapabilities capabilities,
            Listener listener, boolean mirror, Looper looper, FocusRing focusRing) {
        mAppController = appController;
        mSettingsManager = appController.getSettingsManager();
        mHandler = new MainHandler(this, looper);
        mDefaultFocusModes = new ArrayList<CameraCapabilities.FocusMode>(defaultFocusModes);
        updateCapabilities(capabilities);
        mListener = listener;
        setMirror(mirror);
        mFocusRing = focusRing;
        mFocusLocked = false;
    }
    /* Dream Camera test ui check 20 @{ */
    public FocusOverlayManager(AppController appController,
            List<CameraCapabilities.FocusMode> defaultFocusModes, CameraCapabilities capabilities,
            Listener listener, boolean mirror, Looper looper, FocusRing focusRing, TouchListener touchListener) {
        mAppController = appController;
        mSettingsManager = appController.getSettingsManager();
        mHandler = new MainHandler(this, looper);
        mDefaultFocusModes = new ArrayList<CameraCapabilities.FocusMode>(defaultFocusModes);
        updateCapabilities(capabilities);
        mListener = listener;
        setMirror(mirror);
        mFocusRing = focusRing;
        mFocusLocked = false;
        mTouchListener = touchListener;
    }
    /* @} */
    public void updateCapabilities(CameraCapabilities capabilities) {
        // capabilities can only be null when onConfigurationChanged is called
        // before camera is open. We will just return in this case, because
        // capabilities will be set again later with the right capabilities after
        // camera is open.
        if (capabilities == null) {
            return;
        }
        mCapabilities = capabilities;
        mFocusAreaSupported = mCapabilities.supports(CameraCapabilities.Feature.FOCUS_AREA);
        mMeteringAreaSupported = mCapabilities.supports(CameraCapabilities.Feature.METERING_AREA);
        mLockAeAwbNeeded = (mCapabilities.supports(CameraCapabilities.Feature.AUTO_EXPOSURE_LOCK)
                || mCapabilities.supports(CameraCapabilities.Feature.AUTO_WHITE_BALANCE_LOCK));
    }

    /** This setter should be the only way to mutate mPreviewRect. */
    public void setPreviewRect(Rect previewRect) {
        if (!mPreviewRect.equals(previewRect)) {
            mPreviewRect.set(previewRect);
            mFocusRing.configurePreviewDimensions(CameraUtil.rectToRectF(mPreviewRect));
            resetCoordinateTransformer();
            mInitialized = true;
        }
    }

    @Override
    public void onPreviewAreaChanged(RectF previewArea) {
        setPreviewRect(CameraUtil.rectFToRect(previewArea));
    }

    public void setMirror(boolean mirror) {
        mMirror = mirror;
        resetCoordinateTransformer();
    }

    public void setDisplayOrientation(int displayOrientation) {
        mDisplayOrientation = displayOrientation;
        resetCoordinateTransformer();
    }

    private void resetCoordinateTransformer() {
        if (mPreviewRect.width() > 0 && mPreviewRect.height() > 0) {
            mCoordinateTransformer = new CameraCoordinateTransformer(mMirror, mDisplayOrientation,
                  CameraUtil.rectToRectF(mPreviewRect));
        } else {
            Log.w(TAG, "The coordinate transformer could not be built because the preview rect"
                  + "did not have a width and height");
        }
    }


    private void lockAeAwbIfNeeded() {
        if (mLockAeAwbNeeded && !mAeAwbLock) {
            mAeAwbLock = true;
            mListener.setFocusParameters();
        }
    }

    private void unlockAeAwbIfNeeded() {
        if (mLockAeAwbNeeded && mAeAwbLock && (mState != STATE_FOCUSING_SNAP_ON_FINISH)) {
            mAeAwbLock = false;
            mListener.setFocusParameters();
        }
    }

    /* SPRD: fix bug 473602 add for half-press @{*/
    public void onShutterDown(CameraCapabilities.FocusMode currentFocusMode) {
        if (!mInitialized) return;

        boolean autoFocusCalled = false;
        if (needAutoFocusCall(currentFocusMode) && mState == STATE_IDLE) {
            // Do not focus if touch focus has been triggered.
            if (mPreviewRect.width() == 0 || mPreviewRect.height() == 0) {
                return;
            }

            autoFocus();
            autoFocusCalled = true;
        }

        if (!autoFocusCalled) lockAeAwbIfNeeded();
    }

    public void updateFocusUI() {
        Log.i(TAG, "updateFocusUI mInitialized="+mInitialized+" mState="+mState);
        if (!mInitialized) {
            // Show only focus indicator or face indicator.
            return;
        }
        if (mState == STATE_IDLE) {
            if (mFocusArea != null) {
                // Users touch on the preview and the indicator represents the
                // metering area. Either focus area is not supported or
                // autoFocus call is not required.
                mFocusRing.startActiveFocus();
            }
        } else if (mState == STATE_FOCUSING) {
            if (mFocusArea == null) {
                mFocusRing.centerFocusLocation();
            }
            mFocusRing.startActiveFocus();
        } else {
            if (mState == STATE_SUCCESS) {
                /* Dream Camera test ui check 20 @{ */
                if (mTouchListener != null) {
                    mTouchListener.touchCapture();
                }
                /* @} */
                mFocusRing.startActiveFocusedFocus();
                if (mFocusArea != null) {
                    if(mAppController.isPlaySoundEnable()){
                        mCameraSound.play(MediaActionSound.FOCUS_COMPLETE);
                    }
                } else {
                    mFocusRing.centerFocusLocation();
                }
            } else if (mState == STATE_FAIL) {
                /*SPRD:fix bug596278 touch capture can not work when af fail @{*/
                if (mFocusRing.isActiveFocusRunning()) {
                    mFocusRing.stopFocusAnimations();
                }
                if (mTouchListener != null) {
                    mTouchListener.touchCapture();
                }
                /* @}*/
            }
        }
    }

    public void onShutterUp(CameraCapabilities.FocusMode currentFocusMode) {
        if (!mInitialized) {
            return;
        }

        if (needAutoFocusCall(currentFocusMode)) {
            // User releases half-pressed focus key.
            if (mState == STATE_FOCUSING || mState == STATE_SUCCESS
                    || mState == STATE_FAIL) {
                cancelAutoFocus();
            }
        }

        // Unlock AE and AWB after cancelAutoFocus. Camera API does not
        // guarantee setParameters can be called during autofocus.
        unlockAeAwbIfNeeded();
    }

    public void focusAndCapture(CameraCapabilities.FocusMode currentFocusMode) {
        if (!mInitialized) {
            return;
        }

        /**
         * SPRD:fix bug 473602 add for half-press @{
        if (!needAutoFocusCall(currentFocusMode)) {
         */
        if (!needAutoFocusCall(currentFocusMode) && mState != STATE_FOCUSING) {
        /**
         * @}
         */
            // Focus is not needed.
            Log.i(TAG, "Focus is not needed.");
            capture();
        } else if (mState == STATE_SUCCESS || mState == STATE_FAIL) {
            // Focus is done already.
            Log.i(TAG, "Focus is done already.");
            capture();
        } else if (mState == STATE_FOCUSING) {
            // Still focusing and will not trigger snap upon finish.
            Log.i(TAG, "till focusing and will not trigger snap upon finish.");
            mState = STATE_FOCUSING_SNAP_ON_FINISH;
        } else if (mState == STATE_IDLE) {
            autoFocusAndCapture();
        }
    }

    public void onAutoFocus(boolean focused, boolean shutterButtonPressed) {
        Log.i(TAG, "onAutoFocus focused:" + focused + ",mState:" + mState);
        if (mState == STATE_FOCUSING_SNAP_ON_FINISH) {
            // Take the picture no matter focus succeeds or fails. No need
            // to play the AF sound if we're about to play the shutter
            // sound.
            if (focused) {
                mState = STATE_SUCCESS;
            } else {
                mState = STATE_FAIL;
            }
            updateFocusUI();
            capture();
        } else if (mState == STATE_FOCUSING) {
            // This happens when (1) user is half-pressing the focus key or
            // (2) touch focus is triggered. Play the focus tone. Do not
            // take the picture now.
            if (focused) {
                mState = STATE_SUCCESS;
            } else {
                mState = STATE_FAIL;
            }
            updateFocusUI();//SPRD:fix bug 473602 add for half-press
            // If this is triggered by touch focus, cancel focus after a
            // while.
            if (mFocusArea != null) {
                mFocusLocked = true;
                mHandler.sendEmptyMessageDelayed(RESET_TOUCH_FOCUS, RESET_TOUCH_FOCUS_DELAY_MILLIS);
            }
            if (shutterButtonPressed) {
                // Lock AE & AWB so users can half-press shutter and recompose.
                lockAeAwbIfNeeded();
            }
        } else if (mState == STATE_IDLE) {
            // User has released the focus key before focus completes.
            // Do nothing.
        }
    }

    public void onAutoFocusMoving(boolean moving) {
        //moving == true, begin, moving == false, success/fail.
        Log.i(TAG, "onAutoFocusMoving moving = " + moving + ",mState =" + mState);
        if (!mInitialized) {
            return;
        }

        // Ignore if we have requested autofocus. This method only handles
        // continuous autofocus.
        if (mState != STATE_IDLE) {
            return;
        }

        // animate on false->true trasition only b/8219520
        if (moving && !mPreviousMoving) {
            // Auto focus at the center of the preview.
            mFocusRing.centerFocusLocation();
            mFocusRing.startPassiveFocus();
        /**
         * SPRD:fix bug594887
         * original code
        } else if (!moving && mFocusRing.isPassiveFocusRunning()) {
            mFocusRing.stopFocusAnimations();
         */
        } else if (!moving) {
            if (mFocusRing.isPassiveFocusRunning())
                mFocusRing.stopFocusAnimations();
            mFocusRing.centerFocusLocation();
            mFocusRing.startPassiveFocusedFocus();
        }
        mPreviousMoving = moving;
    }

    /** Returns width of auto focus region in pixels. */
    private int getAFRegionSizePx() {
        return (int) (Math.min(mPreviewRect.width(), mPreviewRect.height()) * AF_REGION_BOX);
    }

    /** Returns width of metering region in pixels. */
    private int getAERegionSizePx() {
        return (int) (Math.min(mPreviewRect.width(), mPreviewRect.height()) * AE_REGION_BOX);
    }

    private void initializeFocusAreas(int x, int y) {
        if (mFocusArea == null) {
            mFocusArea = new ArrayList<Area>();
            mFocusArea.add(new Area(new Rect(), 1));
        }

        // Convert the coordinates to driver format.
        mFocusArea.get(0).rect = computeCameraRectFromPreviewCoordinates(x, y, getAFRegionSizePx());
    }

    private void initializeMeteringAreas(int x, int y) {
        if (mMeteringArea == null) {
            mMeteringArea = new ArrayList<Area>();
            mMeteringArea.add(new Area(new Rect(), 1));
        }

        // Convert the coordinates to driver format.
        mMeteringArea.get(0).rect = computeCameraRectFromPreviewCoordinates(x, y, getAERegionSizePx());
    }

    public void onSingleTapUp(int x, int y) {
        /**
         * SPRD: fix bug473602 add for hal-press @{
        if (!mInitialized || mState == STATE_FOCUSING_SNAP_ON_FINISH) {
            return;
        }

        // Let users be able to cancel previous touch focus.
        if ((mFocusArea != null) && (mState == STATE_FOCUSING ||
                    mState == STATE_SUCCESS || mState == STATE_FAIL)) {
            cancelAutoFocus();
        }
         */
        if (!mInitialized || mState == STATE_FOCUSING_SNAP_ON_FINISH || mState == STATE_FOCUSING ) {
            return;
        }

        // Let users be able to cancel previous touch focus.
        if ((mFocusArea != null) && (mState == STATE_SUCCESS || mState == STATE_FAIL)) {
            cancelAutoFocus();
        }
        if (mPreviewRect.width() == 0 || mPreviewRect.height() == 0) {
            return;
        }
        /**
         * @}
         */
        // Initialize variables.
        // Initialize mFocusArea.
        if (mFocusAreaSupported) {
            initializeFocusAreas(x, y);
            //SPRD:fix bug533976 add touch AE for FF
            //mFocusRing.startActiveFocus();
            mFocusRing.setFocusLocation(x, y);
        }
        // Initialize mMeteringArea.
        if (mMeteringAreaSupported) {
            initializeMeteringAreas(x, y);
        }

        /*SPRD:fix bug533976 add touch AE for FF
         * android original code
        mFocusRing.startActiveFocus();
        mFocusRing.setFocusLocation(x, y);
        */

        // Log manual tap to focus.
        mTouchCoordinate = new TouchCoordinate(x, y, mPreviewRect.width(), mPreviewRect.height());
        mTouchTime = System.currentTimeMillis();

        // Stop face detection because we want to specify focus and metering area.
        mListener.stopFaceDetection();

        // Set the focus area and metering area.
        mListener.setFocusParameters();
        if (mFocusAreaSupported) {
            autoFocus();
        } else {  // Just show the indicator in all other cases.
            // Reset the metering area in 4 seconds.
            mHandler.removeMessages(RESET_TOUCH_FOCUS);
            mHandler.sendEmptyMessageDelayed(RESET_TOUCH_FOCUS, RESET_TOUCH_FOCUS_DELAY_MILLIS);
        }
    }

    public void onPreviewStarted() {
        mState = STATE_IDLE;
        // Avoid resetting touch focus if N4, b/18681082.
        if (!ApiHelper.IS_NEXUS_4) {
            resetTouchFocus();
        }
        if(mCameraSound == null){
            mCameraSound = new MediaActionSound();
            mCameraSound.load(MediaActionSound.FOCUS_COMPLETE);
        }
    }

    public void onPreviewStopped() {
        // If auto focus was in progress, it would have been stopped.
        mState = STATE_IDLE;
    }

    public void onCameraReleased() {
        onPreviewStopped();
        if (mCameraSound != null) {
            mCameraSound.release();
            mCameraSound = null;
        }
    }

    @Override
    public void onMoving() {
        if (mFocusLocked) {
            Log.d(TAG, "onMoving: Early focus unlock.");
            cancelAutoFocus();
        }
    }

    /**
     * Triggers the autofocus and sets the specified state.
     *
     * @param focusingState The state to use when focus is in progress.
     */
    private void autoFocus(int focusingState) {
        Log.i(TAG, "autoFocus focusingState:" + focusingState);
        mListener.autoFocus();
        mState = focusingState;
        updateFocusUI();//SPRD:fix bug 594887
        mHandler.removeMessages(RESET_TOUCH_FOCUS);
    }

    /**
     * Triggers the autofocus and set the state to indicate the focus is in
     * progress.
     */
    private void autoFocus() {
        autoFocus(STATE_FOCUSING);
    }

    /**
     * Triggers the autofocus and set the state to which a capture will happen
     * in the following autofocus callback.
     */
    private void autoFocusAndCapture() {
        Log.i(TAG, "autoFocusAndCapture.");
        autoFocus(STATE_FOCUSING_SNAP_ON_FINISH);
    }

    /* SPRD: fix bug 498954 If the function of flash is on and the camera is counting
     * down, the flash should not run here but before the capture.@{*/
    public void focusAfterCountDownFinishWhileFlashOn() {
        Log.i(TAG, "focusAfterCountDownFinishWhileFlashOn.");
        autoFocusAndCapture();
    }
    /* @} */

    private void cancelAutoFocus() {
        Log.v(TAG, "Cancel autofocus.");
        // Reset the tap area before calling mListener.cancelAutofocus.
        // Otherwise, focus mode stays at auto and the tap area passed to the
        // driver is not reset.
        resetTouchFocus();
        mListener.cancelAutoFocus();
        mState = STATE_IDLE;
        mFocusLocked = false;
        mHandler.removeMessages(RESET_TOUCH_FOCUS);
    }

    private void capture() {
        if (mListener.capture()) {
            mState = STATE_IDLE;
            mHandler.removeMessages(RESET_TOUCH_FOCUS);
        }
    }

    public CameraCapabilities.FocusMode getFocusMode(
            final CameraCapabilities.FocusMode currentFocusMode) {
        if (mOverrideFocusMode != null) {
            Log.i(TAG, "returning override focus: " + mOverrideFocusMode);
            return mOverrideFocusMode;
        }
        if (mCapabilities == null) {
            Log.i(TAG, "no capabilities, returning default AUTO focus mode");
            return CameraCapabilities.FocusMode.AUTO;
        }

        if (mFocusAreaSupported && mFocusArea != null) {
            Log.i(TAG, "in tap to focus, returning AUTO focus mode");
            // Always use autofocus in tap-to-focus.
            mFocusMode = CameraCapabilities.FocusMode.AUTO;
        } else {
//            String focusSetting = mSettingsManager.getString(mAppController.getCameraScope(),
//                    Keys.KEY_FOCUS_MODE);

            String focusSetting = DataModuleManager
                    .getInstance(mAppController.getAndroidContext())
                    .getDataModuleCamera().getString(Keys.KEY_FOCUS_MODE);

            Log.i(TAG, "stored focus setting for camera: " + focusSetting);
            // The default is continuous autofocus.
            mFocusMode = mCapabilities.getStringifier().focusModeFromString(focusSetting);
            Log.i(TAG, "focus mode resolved from setting: " + mFocusMode);
            // Try to find a supported focus mode from the default list.
            if (mFocusMode == null) {
                for (CameraCapabilities.FocusMode mode : mDefaultFocusModes) {
                    if (mCapabilities.supports(mode)) {
                        mFocusMode = mode;
                        Log.v(TAG, "selected supported focus mode from default list" + mode);
                        break;
                    }
                }
            }
        }
        if (!mCapabilities.supports(mFocusMode)) {
            // For some reasons, the driver does not support the current
            // focus mode. Fall back to auto.
            if (mCapabilities.supports(CameraCapabilities.FocusMode.AUTO)) {
                Log.v(TAG, "no supported focus mode, falling back to AUTO");
                mFocusMode = CameraCapabilities.FocusMode.AUTO;
            } else {
                Log.v(TAG, "no supported focus mode, falling back to current: " + currentFocusMode);
                mFocusMode = currentFocusMode;
            }
        }
        return mFocusMode;
    }

    public List<Area> getFocusAreas() {
        return mFocusArea;
    }

    public List<Area> getMeteringAreas() {
        return mMeteringArea;
    }

    public void resetTouchFocus() {
        Log.i(TAG, "resetTouchFocus mInitialized:" + mInitialized);
        if (!mInitialized) {
            return;
        }

        mFocusArea = null;
        mMeteringArea = null;
        // This will cause current module to call getFocusAreas() and
        // getMeteringAreas() and send updated regions to camera.
        mListener.setFocusParameters();

        if (mTouchCoordinate != null) {
            UsageStatistics.instance().tapToFocus(mTouchCoordinate,
                    0.001f * (System.currentTimeMillis() - mTouchTime));
            mTouchCoordinate = null;
        }
    }

    private Rect computeCameraRectFromPreviewCoordinates(int x, int y, int size) {
        int left = CameraUtil.clamp(x - size / 2, mPreviewRect.left,
                mPreviewRect.right - size);
        int top = CameraUtil.clamp(y - size / 2, mPreviewRect.top,
                mPreviewRect.bottom - size);

        RectF rectF = new RectF(left, top, left + size, top + size);
        return CameraUtil.rectFToRect(mCoordinateTransformer.toCameraSpace(rectF));
    }

    /* package */ int getFocusState() {
        return mState;
    }

    /* SPRD:Add for bug 443439 @{ */
    public boolean isInStateFocusing() {
        return mState == STATE_FOCUSING;
    }
    /* @} */

    public boolean isFocusCompleted() {
        return mState == STATE_SUCCESS || mState == STATE_FAIL;
    }

    public boolean isFocusingSnapOnFinish() {
        return mState == STATE_FOCUSING_SNAP_ON_FINISH;
    }

    public void removeMessages() {
        mHandler.removeMessages(RESET_TOUCH_FOCUS);
    }

    public void overrideFocusMode(CameraCapabilities.FocusMode focusMode) {
        mOverrideFocusMode = focusMode;
    }

    public void setAeAwbLock(boolean lock) {
        mAeAwbLock = lock;
    }

    public boolean getAeAwbLock() {
        return mAeAwbLock;
    }

    /**
     * SPRD: fix bug 473602 CAF do not need AF @{
    private boolean needAutoFocusCall(CameraCapabilities.FocusMode focusMode) {
        return !(focusMode == CameraCapabilities.FocusMode.INFINITY
                || focusMode == CameraCapabilities.FocusMode.FIXED
                || focusMode == CameraCapabilities.FocusMode.EXTENDED_DOF);
    }
    */
    private boolean needAutoFocusCall(CameraCapabilities.FocusMode focusMode) {
        return !(focusMode == CameraCapabilities.FocusMode.INFINITY
                || focusMode == CameraCapabilities.FocusMode.FIXED
                || focusMode == CameraCapabilities.FocusMode.EXTENDED_DOF
                || focusMode == CameraCapabilities.FocusMode.CONTINUOUS_PICTURE
                || focusMode == CameraCapabilities.FocusMode.CONTINUOUS_VIDEO);
    }
    /**
     * @}
     */
    /**
     * Add For Dream Camera, new Interface for Settings.
     * @param currentFocusMode: original param,mCameraSettings.focusMode
     * @param settingsFocusMode: String focusSetting = mSettingsManager.getString(mAppController.getCameraScope(),
                    Keys.KEY_FOCUS_MODE);
     * @return
     */
    public CameraCapabilities.FocusMode getFocusMode(
            final CameraCapabilities.FocusMode currentFocusMode, final String settingsFocusMode) {
        if (mOverrideFocusMode != null) {
            Log.v(TAG, "returning override focus: " + mOverrideFocusMode);
            return mOverrideFocusMode;
        }
        if (mCapabilities == null) {
            Log.v(TAG, "no capabilities, returning default AUTO focus mode");
            return CameraCapabilities.FocusMode.AUTO;
        }

        Log.d(TAG,"AA getFocusMode "+mFocusAreaSupported+","+mFocusArea);
        if (mFocusAreaSupported && mFocusArea != null) {
            Log.v(TAG, "in tap to focus, returning AUTO focus mode");
            // Always use autofocus in tap-to-focus.
            mFocusMode = CameraCapabilities.FocusMode.AUTO;
        } else {
            Log.i(TAG, "AA getFocusMode stored focus setting for camera: " + settingsFocusMode);
            // The default is continuous autofocus.
            mFocusMode = mCapabilities.getStringifier().focusModeFromString(settingsFocusMode);
            Log.v(TAG, "focus mode resolved from setting: " + mFocusMode);
            // Try to find a supported focus mode from the default list.
            if (mFocusMode == null) {
                for (CameraCapabilities.FocusMode mode : mDefaultFocusModes) {
                    if (mCapabilities.supports(mode)) {
                        mFocusMode = mode;
                        Log.v(TAG, "selected supported focus mode from default list" + mode);
                        break;
                    }
                }
            }
        }
        if (!mCapabilities.supports(mFocusMode)) {
            // For some reasons, the driver does not support the current
            // focus mode. Fall back to auto.
            if (mCapabilities.supports(CameraCapabilities.FocusMode.AUTO)) {
                Log.v(TAG, "no supported focus mode, falling back to AUTO");
                mFocusMode = CameraCapabilities.FocusMode.AUTO;
            } else {
                Log.v(TAG, "no supported focus mode, falling back to current: " + currentFocusMode);
                mFocusMode = currentFocusMode;
            }
        }
        return mFocusMode;
    }
}
