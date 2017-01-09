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

import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import com.android.camera.data.FilmstripItemUtils;
import com.android.camera2.R;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.location.Location;
import android.media.AudioManager;
import android.media.CamcorderProfile;
import android.media.CameraProfile;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.provider.MediaStore;
import android.provider.MediaStore.MediaColumns;
import android.provider.MediaStore.Video;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Toast;

import com.android.camera.app.AppController;
import com.android.camera.app.CameraAppUI;
import com.android.camera.app.LocationManager;
import com.android.camera.app.MediaSaver;
import com.android.camera.app.MemoryManager;
import com.android.camera.app.MemoryManager.MemoryListener;
import com.android.camera.app.OrientationManager;
import com.android.camera.debug.Log;
import com.android.camera.exif.ExifInterface;
import com.android.camera.hardware.HardwareSpec;
import com.android.camera.hardware.HardwareSpecImpl;
import com.android.camera.module.ModuleController;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.settings.SettingsUtil;
import com.android.camera.stats.UsageStatistics;
import com.android.camera.stats.profiler.Profile;
import com.android.camera.stats.profiler.Profilers;
import com.android.camera.ui.TouchCoordinate;
import com.android.camera.util.AndroidServices;
import com.android.camera.util.ApiHelper;
import com.android.camera.util.CameraUtil;
import com.android.camera.util.Size;
import com.android.camera.util.ToastUtil;
import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraAgent.CameraPictureCallback;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.ex.camera2.portability.CameraDeviceInfo.Characteristics;
import com.android.ex.camera2.portability.CameraSettings;
import com.dream.camera.ButtonManagerDream;
import com.dream.camera.MakeupController;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleBasic.DreamSettingChangeListener;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataModuleManager.ResetListener;
import com.dream.camera.settings.DataStructSetting;
import com.dream.camera.util.DreamUtil;
import com.dream.camera.settings.DataConfig;
import com.google.common.logging.eventprotos;
import com.sprd.camera.storagepath.MultiStorage;
import com.sprd.camera.storagepath.StorageUtil;
import com.sprd.camera.storagepath.StorageUtilProxy;
import com.dream.camera.util.DreamProxy;

import android.os.storage.StorageManager;
import android.os.storage.VolumeInfo;

public class VideoModule extends CameraModule implements
        FocusOverlayManager.Listener, MediaRecorder.OnErrorListener,
        MediaRecorder.OnInfoListener, MemoryListener,
        OrientationManager.OnOrientationChangeListener, VideoController,
        DreamSettingChangeListener {

    private static final Log.Tag TAG = new Log.Tag("VideoModule");
    // Messages defined for the UI thread handler.
    private static final int MSG_CHECK_DISPLAY_ROTATION = 4;
    private static final int MSG_UPDATE_RECORD_TIME = 5;
    private static final int MSG_ENABLE_SHUTTER_BUTTON = 6;
    private static final int MSG_SWITCH_CAMERA = 8;
    private static final int MSG_SWITCH_CAMERA_START_ANIMATION = 9;
    private static final int MSG_ENABLE_SHUTTER_PAUSE_BUTTON = 10;
    private static final long SHUTTER_BUTTON_TIMEOUT = 500L; // 500ms
    private static final long ENABLE_SHUTTER_PAUSE_BUTTON_TIMEOUT = 3000L;
    /**
     * An unpublished intent flag requesting to start recording straight away
     * and return as soon as recording is stopped. TODO: consider publishing by
     * moving into MediaStore.
     */
    private static final String EXTRA_QUICK_CAPTURE = "android.intent.extra.quickCapture";
    /*
     * SPRD Bug:474704 Feature:Video Recording Pause. @{
     */
    private static final int RECORD_LIMIT_TIME = 3000; // 3s
    // SPRD: Fix bug 540246 recording and music work together after we end call
    private final String PAUSE_ACTION = "com.android.music.musicservicecommand.pause";
    private final Handler mHandler = new MainHandler();
    // module fields
    protected CameraActivity mActivity;
    private final MediaSaver.OnMediaSavedListener mOnPhotoSavedListener = new MediaSaver.OnMediaSavedListener() {
        @Override
        public void onMediaSaved(Uri uri) {
            if (uri != null) {
                mActivity.notifyNewMedia(uri);
            }
        }
    };
    protected AppController mAppController;
    boolean mPreviewing = false; // True if preview is started.
    private boolean mPaused;
    // if, during and intent capture, the activity is paused (e.g. when app
    // switching or reviewing a
    // shot video), we don't want the bottom bar intent ui to reset to the
    // capture button
    private boolean mDontResetIntentUiOnResume;
    private int mCameraId;
    private CameraSettings mCameraSettings;
    private CameraCapabilities mCameraCapabilities;
    private HardwareSpec mHardwareSpec;
    private boolean mIsInReviewMode;
    private boolean mSnapshotInProgress = false;
    // Preference must be read before starting preview. We check this before
    // starting
    // preview.
    private boolean mPreferenceRead;
    private boolean mIsVideoCaptureIntent;
    private boolean mQuickCapture;
    private MediaRecorder mMediaRecorder;
    /** Manager used to mute sounds and vibrations during video recording. */
    private AudioManager mAudioManager;
    /*
     * The ringer mode that was set when video recording started. We use this to
     * reset the mode once video recording has stopped.
     */
    private int mOriginalRingerMode;
    private boolean mSwitchingCamera;
    private boolean mMediaRecorderRecording = false;
    private long mRecordingStartTime;
    private boolean mRecordingTimeCountsDown = false;
    private long mOnResumeTime;
    // The video file that the hardware camera is about to record into
    // (or is recording into.
    private String mVideoFilename;
    private ParcelFileDescriptor mVideoFileDescriptor;

    // The video file that has already been recorded, and that is being
    // examined by the user.
    private String mCurrentVideoFilename;
    private Uri mCurrentVideoUri;
    private final View.OnClickListener mDoneCallback = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            onReviewDoneClicked(v);
        }
    };
    private boolean mCurrentVideoUriFromMediaSaved;
    private ContentValues mCurrentVideoValues;
    private CamcorderProfile mProfile;
    private final View.OnClickListener mReviewCallback = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            onReviewPlayClicked(v);
        }
    };
    // The video duration limit. 0 means no limit.
    private int mMaxVideoDurationInMs;
    // The display rotation in degrees. This is only valid when mPreviewing is
    // true.
    private int mDisplayRotation;
    private int mCameraDisplayOrientation;
    private int mDesiredPreviewWidth;
    private int mDesiredPreviewHeight;
    private ContentResolver mContentResolver;
    private final View.OnClickListener mCancelCallback = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            onReviewCancelClicked(v);
        }
    };
    private LocationManager mLocationManager;
    private int mPendingSwitchCameraId;
    private VideoUI mUI;

    private CameraProxy mCameraDevice;

    private float mZoomValue; // The current zoom ratio.
    private Toast mToast;

    private int mTimeBetweenTimeLapseFrameCaptureMs;// SPRD: for bug 509708 add
                                                    // time lapse
    private boolean mCaptureTimeLapse;// SPRD: for bug 509708 add time lapse

    // Add for dream settings.
    protected DataModuleBasic mDataModule;
    private DataModuleBasic mDataModuleCurrent;
    private DataModuleBasic mTempModule;

    private boolean played = false;

    // SPRD: Fix bug 650297 that video thumbnail is showed slowly
    private int mSnapshotCount = 0;

    // SPRD: add for mutext between float window and camera
    public static final String KEY_DISABLE_FLOAT_WINDOW = "disable-float-window";

    /* SPRD: fix bug Bug 577352 Slow recording process, repeatedly tap the screen,the
     * phone interface will now show time "slow record does not support the camera @{ */
    private void showToast(Context context, int msg, int duration) {
        if (mToast == null) {
            mToast = Toast.makeText(context, msg, duration);
        } else {
            mToast.setText(msg);
        }
        mToast.show();
    }
    /* @} */

    private final MediaSaver.OnMediaSavedListener mOnVideoSavedListener = new MediaSaver.OnMediaSavedListener() {
        @Override
        public void onMediaSaved(Uri uri) {
            /*
             * SPRD:fix bug523146 Airtel, Shark L shows Camera force close@{ if
             * (uri != null) {
             */
            if (uri != null && mAppController.getCameraProvider() != null) {
            /* @} */
                mCurrentVideoUri = uri;
                mCurrentVideoUriFromMediaSaved = true;
                onVideoSaved();
                mActivity.notifyNewMedia(uri);
            }
        }
    };

    public final ButtonManager.ButtonCallback mFlashCallback = new ButtonManager.ButtonCallback() {
        @Override
        public void onStateChanged(int state) {
            if (mPaused) {
                return;
            }
            // Update flash parameters.
            enableTorchMode(true);
        }
    };

    public final ButtonManager.ButtonCallback mMakeupCallback = new ButtonManager.ButtonCallback() {
        @Override
        public void onStateChanged(int state) {
            if (mPaused
                    || mAppController.getCameraProvider().waitingForCamera()) {
                return;
            }
            updateMakeLevel();
        }
    };

    private String mFlashModeBeforeSceneMode;
    private FocusOverlayManager mFocusManager;
    private final CameraAgent.CameraAFCallback mAutoFocusCallback = new CameraAgent.CameraAFCallback() {
        @Override
        public void onAutoFocus(boolean focused, CameraProxy camera) {
            if (mPaused) {
                return;
            }
            mFocusManager.onAutoFocus(focused, false);
        }
    };
    private final Object mAutoFocusMoveCallback = ApiHelper.HAS_AUTO_FOCUS_MOVE_CALLBACK ? new CameraAgent.CameraAFMoveCallback() {
        @Override
        public void onAutoFocusMoving(boolean moving, CameraProxy camera) {
            mFocusManager.onAutoFocusMoving(moving);
        }
    }
            : null;
    private boolean mMirror;
    public final ButtonManager.ButtonCallback mCameraCallback = new ButtonManager.ButtonCallback() {
        @Override
        public void onStateChanged(int state) {
            if (mPaused
                    || mAppController.getCameraProvider().waitingForCamera()) {
                return;
            }
            ButtonManager buttonManager = mActivity.getButtonManager();
            buttonManager.disableCameraButtonAndBlock();
            mPendingSwitchCameraId = state;
            Log.d(TAG, "Start to copy texture.");

            // Disable all camera controls.
            mSwitchingCamera = true;
            switchCamera();
        }
    };
    private boolean mFocusAreaSupported;
    private boolean mMeteringAreaSupported;
    private BroadcastReceiver mReceiver = null;
    private int mShutterIconId;
    //SPRD:fix bug545455 video may not switch camera
    private boolean isShutterButtonClicked = false;
    private boolean mPauseRecorderRecording = false;
    private long mPauseTime = 0;
    /* @} */
    private long mResumeTime = 0;
    private long mResultTime = 0;
    private long mAllPauseTime = 0;

    private boolean mIsBatteryLow = false;
    /**
     * Construct a new video module.
     */
    public VideoModule(AppController app) {
        super(app);
    }

    /**
     * Calculates the preview size and stores it in mDesiredPreviewWidth and
     * mDesiredPreviewHeight.
     * <p>
     * This function checks
     * {@link com.android.camera.cameradevice.CameraCapabilities#getPreferredPreviewSizeForVideo()}
     * but also considers the current preview area size on screen and make sure
     * the final preview size will not be smaller than 1/2 of the current on
     * screen preview area in terms of their short sides. This function has
     * highest priority of WYSIWYG, 1:1 matching as its best match, even if
     * there's a larger preview that meets the condition above.
     * </p>
     *
     * @return The preferred preview size or {@code null} if the camera is not
     * opened yet.
     */
    private static Point getDesiredPreviewSize(CameraCapabilities capabilities,
                                               CamcorderProfile profile, Point previewScreenSize) {
        if (capabilities.getSupportedVideoSizes() == null) {
            // Driver doesn't support separate outputs for preview and video.
            return new Point(profile.videoFrameWidth, profile.videoFrameHeight);
        }

        final int previewScreenShortSide = (previewScreenSize.x < previewScreenSize.y ? previewScreenSize.x
                : previewScreenSize.y);
        List<Size> sizes = Size
                .convert(capabilities.getSupportedPreviewSizes());
        Size preferred = new Size(
                capabilities.getPreferredPreviewSizeForVideo());
        final int preferredPreviewSizeShortSide = (preferred.width() < preferred
                .height() ? preferred.width() : preferred.height());
        if (preferredPreviewSizeShortSide * 2 < previewScreenShortSide) {
            preferred = new Size(profile.videoFrameWidth,
                    profile.videoFrameHeight);
        }
        int product = preferred.width() * preferred.height();
        Iterator<Size> it = sizes.iterator();
        // Remove the preview sizes that are not preferred.
        while (it.hasNext()) {
            Size size = it.next();
            if (size.width() * size.height() > product) {
                it.remove();
            }
        }

        // Take highest priority for WYSIWYG when the preview exactly matches
        // video frame size. The variable sizes is assumed to be filtered
        // for sizes beyond the UI size.
        for (Size size : sizes) {
            if (size.width() == profile.videoFrameWidth
                    && size.height() == profile.videoFrameHeight) {
                Log.v(TAG, "Selected =" + size.width() + "x" + size.height()
                        + " on WYSIWYG Priority");
                return new Point(profile.videoFrameWidth,
                        profile.videoFrameHeight);
            }
        }

        Size optimalSize = CameraUtil.getOptimalPreviewSize(sizes,
                (double) profile.videoFrameWidth / profile.videoFrameHeight);
        return new Point(optimalSize.width(), optimalSize.height());
    }

    private static void setCaptureRate(MediaRecorder recorder, double fps) {
        recorder.setCaptureRate(fps);
    }

    private static void setSlowMotionRate(MediaRecorder recorder, double fps) {
        recorder.setSlowMotionRate(fps);
    }

    private static String millisecondToTimeString(long milliSeconds,
                                                  boolean displayCentiSeconds) {
        long seconds = milliSeconds / 1000; // round down to compute seconds
        long minutes = seconds / 60;
        long hours = minutes / 60;
        long remainderMinutes = minutes - (hours * 60);
        long remainderSeconds = seconds - (minutes * 60);

        StringBuilder timeStringBuilder = new StringBuilder();

        // Hours
        if (hours > 0) {
            if (hours < 10) {
                timeStringBuilder.append('0');
            }
            timeStringBuilder.append(hours);

            timeStringBuilder.append(':');
        }

        // Minutes
        if (remainderMinutes < 10) {
            timeStringBuilder.append('0');
        }
        timeStringBuilder.append(remainderMinutes);
        timeStringBuilder.append(':');
        if (remainderSeconds < 10) {
            timeStringBuilder.append('0');
        }
        timeStringBuilder.append(remainderSeconds);

        // Centi seconds
        if (displayCentiSeconds) {
            timeStringBuilder.append('.');
            long remainderCentiSeconds = (milliSeconds - seconds * 1000) / 10;
            if (remainderCentiSeconds < 10) {
                timeStringBuilder.append('0');
            }
            timeStringBuilder.append(remainderCentiSeconds);
        }

        return timeStringBuilder.toString();
    }

    private static boolean isSupported(String value, List<String> supported) {
        return supported == null ? false : supported.indexOf(value) >= 0;
    }

    @Override
    public String getPeekAccessibilityString() {
        return mAppController.getAndroidContext().getResources()
                .getString(R.string.video_accessibility_peek);
    }

    private String createName(long dateTaken) {
        Date date = new Date(dateTaken);
        SimpleDateFormat dateFormat = new SimpleDateFormat(
                mActivity.getString(R.string.video_file_name_format));

        return dateFormat.format(date);
    }

    @Override
    public void init(CameraActivity activity, boolean isSecureCamera,
                     boolean isCaptureIntent) {
        mActivity = activity;
        // TODO: Need to look at the controller interface to see if we can get
        // rid of passing in the activity directly.
        mAppController = mActivity;

        int cameraId = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getDataModuleCamera().getInt(Keys.KEY_CAMERA_ID);

        DataStructSetting dataSetting = new DataStructSetting(
                DreamUtil.intToString(getMode()), DreamUtil.isFrontCamera(
                mAppController.getAndroidContext(), cameraId),
                mActivity.getCurrentModuleIndex(), cameraId);

        // change the data storage module
        DataModuleManager.getInstance(mAppController.getAndroidContext())
                .changeModuleStatus(dataSetting);

        mDataModule = DataModuleManager.getInstance(
                mAppController.getAndroidContext()).getDataModuleCamera();

        mDataModuleCurrent = DataModuleManager.getInstance(
                mAppController.getAndroidContext()).getCurrentDataModule();

        // get temp module of photo for some setting such as timestamp status
        DataStructSetting tempPhotoDataSetting = new DataStructSetting(
                DataConfig.CategoryType.CATEGORY_PHOTO,
                DreamUtil.isFrontCamera(mAppController.getAndroidContext(), cameraId),
                ""+1,
                cameraId);
        mTempModule = DataModuleManager
                .getInstance(mAppController.getAndroidContext())
                .getTempModule(tempPhotoDataSetting);

        mDataModuleCurrent.addListener(this);
        mDataModule.addListener(this);

        mAudioManager = AndroidServices.instance().provideAudioManager();

        mActivity.updateStorageSpaceAndHint(null);

        mUI = createUI(mActivity);
        mActivity.setPreviewStatusListener(mUI);

        SettingsManager settingsManager = mActivity.getSettingsManager();
        mCameraId = mDataModule.getInt(Keys.KEY_CAMERA_ID);

        /*
         * To reduce startup time, we start the preview in another thread. We
         * make sure the preview is started at the end of onCreate.
         */
        requestCamera(mCameraId);

        mContentResolver = mActivity.getContentResolver();

        // Surface texture is from camera screen nail and startPreview needs it.
        // This must be done before startPreview.
        mIsVideoCaptureIntent = isVideoCaptureIntent();

        mQuickCapture = mActivity.getIntent().getBooleanExtra(
                EXTRA_QUICK_CAPTURE, false);
        mLocationManager = mActivity.getLocationManager();

        mUI.setOrientationIndicator(0, false);
        setDisplayOrientation();

        mPendingSwitchCameraId = -1;

        mShutterIconId = CameraUtil.getCameraShutterIconId(
                mAppController.getCurrentModuleIndex(),
                mAppController.getAndroidContext());
    }

    @Override
    public boolean isUsingBottomBar() {
        return true;
    }

    private void initializeControlByIntent() {
        if (isVideoCaptureIntent()) {
            if (!mDontResetIntentUiOnResume) {
                mActivity.getCameraAppUI().transitionToIntentCaptureLayout();
            }
            // reset the flag
            mDontResetIntentUiOnResume = false;
        }
    }

    /* SPRD: fix bug 553567 slow motion does not support takeSnapShot and zoom @{ */
    private boolean isSlowMotionOn() {
        String slowMotionValue = mDataModuleCurrent
                .getString(Keys.KEY_VIDEO_SLOW_MOTION);
        int slow_motion = Integer
                .valueOf(slowMotionValue == null ? Keys.SLOWMOTION_DEFAULT_VALUE
                        : slowMotionValue);
        Log.i(TAG, "slow_motion = " + slow_motion + " boolean = "
                + (slow_motion == 1));
        return !(slow_motion == 1);
    }
    /* @} */

    private boolean isRecordingIn4k() {
        String videoQualityKey = isCameraFrontFacing() ? Keys.KEY_VIDEO_QUALITY_FRONT
                : Keys.KEY_VIDEO_QUALITY_BACK;
        String videoQuality = mDataModuleCurrent
                .getString(videoQualityKey);
        int quality = SettingsUtil.getVideoQuality(videoQuality, mCameraId);
        Log.d(TAG, "Selected video quality for '" + videoQuality + "' is " + quality);
        if (quality == CamcorderProfile.QUALITY_2160P) {
            return true;
        }
        return false;
    }

    /* SPRD:fix bug 615391 EOIS and 4k not support snap shot @ {*/
    private boolean isEOISOn() {
        String eoisKey = isCameraFrontFacing() ? Keys.KEY_EOIS_DV_FRONT
                : Keys.KEY_EOIS_DV_BACK;
        if (mDataModuleCurrent != null &&
                mDataModuleCurrent.isEnableSettingConfig(eoisKey)) {
            boolean eoisEnable = isCameraFrontFacing() ? CameraUtil.isEOISDvFrontEnabled()
                    : CameraUtil.isEOISDvBackEnabled();
            return eoisEnable && mDataModuleCurrent.getBoolean(eoisKey, false);
        }
        return false;
    }
    /* @} */

    @Override
    public void onSingleTapUp(View view, int x, int y) {
        /*
         * SPRD Bug:509945,not start preview,intercept snapshot@{ Original
         * Android code: if (mPaused || mCameraDevice == null) {
         */
        if (mPaused || mCameraDevice == null || !mPreviewing) {
        /* @} */
            return;
        }
        if (mMediaRecorderRecording) {
            /*
            if (!mSnapshotInProgress && !isSlowMotionOn() && !isRecordingIn4k()
                    && !(mCameraId == 0 && Keys.isEOISBackOn(mActivity.getSettingsManager()))
                    && !(mCameraId == 1 && Keys.isEOISFrontOn(mActivity.getSettingsManager()))) {
                takeASnapshot();
            } else if (isSlowMotionOn()) {
                Log.i(TAG, "slowMotionOn, do not takeASnapshot");
                // Show the toast.
                ToastUtil.showToast(mActivity, R.string.slow_motion_does_not_support_take_snap_shot,
                        ToastUtil.LENGTH_LONG);
            }
            */
            return;
        }
        // Check if metering area or focus area is supported.
        if (!mFocusAreaSupported && !mMeteringAreaSupported) {
            return;
        }
        // Tap to focus.
        mFocusManager.onSingleTapUp(x, y);
    }

    public void takeASnapshot() {
        // Only take snapshots if video snapshot is supported by device
        if (!mCameraCapabilities
                .supports(CameraCapabilities.Feature.VIDEO_SNAPSHOT)) {
            Log.w(TAG,
                    "Cannot take a video snapshot - not supported by hardware");
            return;
        }
        // Do not take the picture if there is not enough storage.
        if (mActivity.getStorageSpaceBytes() <= Storage.LOW_STORAGE_THRESHOLD_BYTES) {
            Log.i(TAG, "Not enough space or storage not ready. remaining="
                    + mActivity.getStorageSpaceBytes());
            return;
        }

        if (!mIsVideoCaptureIntent) {
            if (!mMediaRecorderRecording || mPaused || mSnapshotInProgress
                    || !mAppController.isShutterEnabled()
                    || mCameraDevice == null) {
                return;
            }

            Location loc = mLocationManager.getCurrentLocation();
            CameraUtil.setGpsParameters(mCameraSettings, loc);
            mCameraDevice.applySettings(mCameraSettings);

            // Set JPEG orientation. Even if screen UI is locked in portrait,
            // camera orientation
            // should
            // still match device orientation (e.g., users should always get
            // landscape photos while
            // capturing by putting device in landscape.)
            Characteristics info = mActivity.getCameraProvider()
                    .getCharacteristics(mCameraId);
            int sensorOrientation = info.getSensorOrientation();
            int deviceOrientation = mAppController.getOrientationManager()
                    .getDeviceOrientation().getDegrees();
            boolean isFrontCamera = info.isFacingFront();
            int jpegRotation = CameraUtil.getImageRotation(sensorOrientation,
                    deviceOrientation, isFrontCamera);
            Log.i(TAG, " sensorOrientation = " + sensorOrientation
                    + " ,deviceOrientation = " + deviceOrientation
                    + " isFrontCamera = " + isFrontCamera);
            mCameraDevice.setJpegOrientation(jpegRotation);

            Log.i(TAG, "Video snapshot start");
            mCameraDevice.takePicture(mHandler, null, null, null,
                    new JpegPictureCallback(loc));
            showVideoSnapshotUI(true);
            mSnapshotInProgress = true;
        }
    }

    private void updateAutoFocusMoveCallback() {
        if (mPaused || mCameraDevice == null) {
            return;
        }

        if (mCameraSettings.getCurrentFocusMode() == CameraCapabilities.FocusMode.CONTINUOUS_PICTURE) {
            mCameraDevice.setAutoFocusMoveCallback(mHandler,
                    (CameraAgent.CameraAFMoveCallback) mAutoFocusMoveCallback);
        } else {
            mCameraDevice.setAutoFocusMoveCallback(null, null);
        }
    }

    private void removeAutoFocusMoveCallback() {
        if (mCameraDevice != null) {
            mCameraDevice.setAutoFocusMoveCallback(null, null);
        }
    }

    /**
     * @return Whether the currently active camera is front-facing.
     */
    private boolean isCameraFrontFacing() {
        return mAppController.getCameraProvider().getCharacteristics(mCameraId)
                .isFacingFront();
    }

    /**
     * @return Whether the currently active camera is back-facing.
     */
    private boolean isCameraBackFacing() {
        return mAppController.getCameraProvider().getCharacteristics(mCameraId)
                .isFacingBack();
    }

    /**
     * The focus manager gets initialized after camera is available.
     */
    private void initializeFocusManager() {
        // Create FocusManager object. startPreview needs it.
        // if mFocusManager not null, reuse it
        // otherwise create a new instance
        if (mFocusManager != null) {
            mFocusManager.removeMessages();
        } else {
            mMirror = isCameraFrontFacing();
            String[] defaultFocusModesStrings = mActivity
                    .getResources()
                    .getStringArray(R.array.pref_camera_focusmode_default_array);
            CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                    .getStringifier();
            ArrayList<CameraCapabilities.FocusMode> defaultFocusModes = new ArrayList<CameraCapabilities.FocusMode>();
            for (String modeString : defaultFocusModesStrings) {
                CameraCapabilities.FocusMode mode = stringifier
                        .focusModeFromString(modeString);
                if (mode != null) {
                    defaultFocusModes.add(mode);
                }
            }
            mFocusManager = new FocusOverlayManager(mAppController,
                    defaultFocusModes, mCameraCapabilities, this, mMirror,
                    mActivity.getMainLooper(), mUI.getFocusRing());
        }
        mAppController.addPreviewAreaSizeChangedListener(mFocusManager);
    }

    @Override
    public void onOrientationChanged(OrientationManager orientationManager,
                                     OrientationManager.DeviceOrientation deviceOrientation) {
        mUI.onOrientationChanged(orientationManager, deviceOrientation);
    }

    @Override
    public void hardResetSettings(SettingsManager settingsManager) {
        // VideoModule does not need to hard reset any settings.
    }

    @Override
    public HardwareSpec getHardwareSpec() {
        if (mHardwareSpec == null) {
            mHardwareSpec = (mCameraSettings != null ? new HardwareSpecImpl(
                    getCameraProvider(), mCameraCapabilities,
                    mAppController.getCameraFeatureConfig(),
                    isCameraFrontFacing()) : null);
        }
        return mHardwareSpec;
    }

    @Override
    public CameraAppUI.BottomBarUISpec getBottomBarSpec() {
        CameraAppUI.BottomBarUISpec bottomBarSpec = new CameraAppUI.BottomBarUISpec();

        bottomBarSpec.enableCamera = true;
        bottomBarSpec.cameraCallback = mCameraCallback;
        bottomBarSpec.enableTorchFlash = true && !mIsBatteryLow;
        bottomBarSpec.flashCallback = mFlashCallback;
        bottomBarSpec.hideHdr = true;
        bottomBarSpec.enableGridLines = true;
        bottomBarSpec.enableExposureCompensation = false;
        bottomBarSpec.isExposureCompensationSupported = false;

        if (isVideoCaptureIntent()) {
            bottomBarSpec.showCancel = true;
            bottomBarSpec.cancelCallback = mCancelCallback;
            bottomBarSpec.showDone = true;
            bottomBarSpec.doneCallback = mDoneCallback;
            bottomBarSpec.showReview = true;
            bottomBarSpec.reviewCallback = mReviewCallback;
        }

        return bottomBarSpec;
    }

    @Override
    public void onCameraAvailable(CameraProxy cameraProxy) {
        Log.i(TAG, "onCameraAvailable cameraProxy = " + cameraProxy);
        if (cameraProxy == null) {
            Log.w(TAG, "onCameraAvailable returns a null CameraProxy object");
            return;
        }
        /* SPRD: fix bug549564  CameraProxy uses the wrong API @{ */
        if (getCameraProvider().isNewApi()) {
            resume();
            Log.d(TAG, "cameraProxy is error, resumed!");
            return;
        }
        /* @} */
        mCameraDevice = cameraProxy;
        mCameraCapabilities = mCameraDevice.getCapabilities();
        mAppController.getCameraAppUI().showAccessibilityZoomUI(
                mCameraCapabilities.getMaxZoomRatio());
        mCameraSettings = mCameraDevice.getSettings();
        mFocusAreaSupported = mCameraCapabilities
                .supports(CameraCapabilities.Feature.FOCUS_AREA);
        mMeteringAreaSupported = mCameraCapabilities
                .supports(CameraCapabilities.Feature.METERING_AREA);
        mMaxRatio = mCameraCapabilities.getMaxZoomRatio();
        readVideoPreferences();
        updateDesiredPreviewSize();
        resizeForPreviewAspectRatio();
        initializeFocusManager();
        // TODO: Having focus overlay manager caching the parameters is prone to
        // error,
        // we should consider passing the parameters to focus overlay to ensure
        // the
        // parameters are up to date.
        mFocusManager.updateCapabilities(mCameraCapabilities);

        startPreview();
        initializeVideoSnapshot();
        mUI.initializeZoom(mCameraSettings, mCameraCapabilities);
        if (isSlowMotionOn()) {
            mUI.hideZoomProcessorIfNeeded();
/*            if(!mActivity.isFilmstripVisible())
                ToastUtil.showToast(mActivity, R.string.slow_motion_does_not_support_zoom_change, ToastUtil.LENGTH_LONG);*/
        }
        initializeControlByIntent();

        mHardwareSpec = new HardwareSpecImpl(getCameraProvider(),
                mCameraCapabilities, mAppController.getCameraFeatureConfig(),
                isCameraFrontFacing());

        ButtonManager buttonManager = mActivity.getButtonManager();
        buttonManager.enableCameraButton();

        mCameraAvailable = true;

    }

    private void startPlayVideoActivity() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        intent.setDataAndType(mCurrentVideoUri,
                convertOutputFormatToMimeType(mProfile.fileFormat));

        // SPRD: add for mutex between float window and camera
        intent.putExtra(KEY_DISABLE_FLOAT_WINDOW, true);

        try {
            mActivity.launchActivityByIntent(intent);
            played = true;
        } catch (ActivityNotFoundException ex) {
            Log.e(TAG, "Couldn't view video " + mCurrentVideoUri, ex);
        }
    }

    @Override
    public void onReviewPlayClicked(View v) {
        startPlayVideoActivity();
    }

    @Override
    public void onReviewDoneClicked(View v) {
        mIsInReviewMode = false;
        doReturnToCaller(true);
    }

    @Override
    public void onReviewCancelClicked(View v) {
        // TODO: It should be better to not even insert the URI at all before we
        // confirm done in review, which means we need to handle temporary video
        // files in a quite different way than we currently had.
        // Make sure we don't delete the Uri sent from the video capture intent.
        if (mCurrentVideoUriFromMediaSaved) {
            mContentResolver.delete(mCurrentVideoUri, null, null);
        }
        mIsInReviewMode = false;
        doReturnToCaller(false);
    }

    @Override
    public boolean isInReviewMode() {
        return mIsInReviewMode;
    }

    private void onStopVideoRecording() {
        /* SPRD: fix for bug 535167 @{ */
        // mAppController.getCameraAppUI().enableCameraToggleButton();
        /* @} */
        mAppController.getCameraAppUI().setSwipeEnabled(true);
        boolean recordFail = stopVideoRecording();
        if (shouldShowResult()) {
            if (mQuickCapture) {
                doReturnToCaller(!recordFail);
            } else if (!recordFail) {
                showCaptureResult();
            }
        } else if (!recordFail) {
            // Start capture animation.
            if (!mPaused && ApiHelper.HAS_SURFACE_TEXTURE_RECORDING) {
                // The capture animation is disabled on ICS because we use
                // SurfaceView
                // for preview during recording. When the recording is done, we
                // switch
                // back to use SurfaceTexture for preview and we need to stop
                // then start
                // the preview. This will cause the preview flicker since the
                // preview
                // will not be continuous for a short period of time.
                mAppController.startFlashAnimation(false);
            }
        }
    }

    public void onVideoSaved() {
        if (shouldShowResult()) {
            showCaptureResult();
        }
    }

    public void onProtectiveCurtainClick(View v) {
        // Consume clicks
    }

    @Override
    public void onShutterButtonClick() {
        /*
         * SPRD Bug:509945,not start preview,intercept recording @{ Original
         * Android code: if (mSwitchingCamera) {
         */
        if (mSwitchingCamera || !mPreviewing) {
        /* @} */
            /* SPRD: fix bug 538868 video may not switch camera@{*/
            //mAppController.getCameraAppUI().enableCameraToggleButton();
            /* @} */
            return;
        }

        if (isPhoneCalling()) {
            Log.i(TAG, "video won't start due to telephone is running");
            ToastUtil.showToast(mActivity, R.string.phone_does_not_support_video,
                    Toast.LENGTH_LONG);
            return;
        }
        //SPRD:fix bug545455 video may not switch camera
        isShutterButtonClicked = true;
        boolean stop = mMediaRecorderRecording;
        if (stop) {
            // CameraAppUI mishandles mode option enable/disable
            // for video, override that
            // Sprd: Add for bug 529369 stop Video recording before switch
            // camera
            //mAppController.getCameraAppUI().enableCameraToggleButton();
            mAppController.getCameraAppUI().enableModeOptions();
            mAppController.getCameraAppUI().updateExtendPanelUI(View.VISIBLE);
            onStopVideoRecording();
        } else {
            if (mActivity.getStorageSpaceBytes() <= Storage.LOW_STORAGE_THRESHOLD_BYTES) {
                return;
            }

            mAppController.getCameraAppUI().disableModeOptions();
            startVideoRecording();
        }
        mAppController.setShutterEnabled(false);
        if (mCameraSettings != null) {
            mFocusManager.onShutterUp(mCameraSettings.getCurrentFocusMode());
        }

        // Keep the shutter button disabled when in video capture intent
        // mode and recording is stopped. It'll be re-enabled when
        // re-take button is clicked.
        if (!(shouldShowResult() && stop)) {
            mHandler.sendEmptyMessageDelayed(MSG_ENABLE_SHUTTER_BUTTON,
                    SHUTTER_BUTTON_TIMEOUT);
        }
    }

    private void updateMakeUpUI(int visible) {
        if (mDataModuleCurrent.getBoolean(Keys.KEY_VIDEO_BEAUTY_ENTERED) && mMakeUpController != null) {
            if (visible != View.VISIBLE) {
                mMakeUpController.pauseMakeupControllerView(false);
            } else {
                mMakeUpController.resumeMakeupControllerView();
            }
        }
    }

    @Override
    public void onShutterCoordinate(TouchCoordinate coord) {
        // Do nothing.
    }

    @Override
    public void onShutterButtonFocus(boolean pressed) {
        // TODO: Remove this when old camera controls are removed from the UI.
        // Sprd: Add for bug 529369 stop Video recording before switch camera
        //Log.d(TAG, "mMediaRecorderRecording = " + mMediaRecorderRecording
        //        + ",pressed" + pressed);
        //if (pressed && !mMediaRecorderRecording) {
        //    mAppController.getCameraAppUI().disableCameraToggleButton();
        ///*SPRD:fix bug545455 video may not switch camera@{*/
        //    isShutterButtonClicked = false;
        //} else if (!pressed && !mMediaRecorderRecording && !isShutterButtonClicked) {
        //    mAppController.getCameraAppUI().enableCameraToggleButton();
        ///*@}*/
        //}
    }

    private void readVideoPreferences() {
        // The preference stores values from ListPreference and is thus string
        // type for all values.
        // We need to convert it to int manually.
        String videoQualityKey = isCameraFrontFacing() ? Keys.KEY_VIDEO_QUALITY_FRONT
                : Keys.KEY_VIDEO_QUALITY_BACK;
        String videoQuality = mDataModuleCurrent.getString(videoQualityKey);
        int quality = SettingsUtil.getVideoQuality(videoQuality, mCameraId);
        Log.d(TAG, "Selected video quality for '" + videoQuality + "' is "
                + quality);

        // Set video quality.
        Intent intent = mActivity.getIntent();
        if (intent.hasExtra(MediaStore.EXTRA_VIDEO_QUALITY)) {
            int extraVideoQuality = intent.getIntExtra(
                    MediaStore.EXTRA_VIDEO_QUALITY, 0);
            if (extraVideoQuality > 0) {
                quality = CamcorderProfile.QUALITY_HIGH;
            } else { // 0 is mms.
                quality = CamcorderProfile.QUALITY_LOW;
            }
        }

        // Set video duration limit. The limit is read from the preference,
        // unless it is specified in the intent.
        if (intent.hasExtra(MediaStore.EXTRA_DURATION_LIMIT)) {
            int seconds = intent
                    .getIntExtra(MediaStore.EXTRA_DURATION_LIMIT, 0);
            mMaxVideoDurationInMs = 1000 * seconds;
        } else {
            mMaxVideoDurationInMs = SettingsUtil.getMaxVideoDuration(mActivity
                    .getAndroidContext());
        }

        // If quality is not supported, request QUALITY_HIGH which is always
        // supported.
        if (CamcorderProfile.hasProfile(mCameraId, quality) == false) {
            quality = CamcorderProfile.QUALITY_HIGH;
        }
        mProfile = CamcorderProfile.get(mCameraId, quality);
        mPreferenceRead = true;
    }

    /**
     * Calculates and sets local class variables for Desired Preview sizes. This
     * function should be called after every change in preview camera resolution
     * and/or before the preview starts. Note that these values still need to be
     * pushed to the CameraSettings to actually change the preview resolution.
     * Does nothing when camera pointer is null.
     */
    private void updateDesiredPreviewSize() {
        if (mCameraDevice == null) {
            return;
        }

        mCameraSettings = mCameraDevice.getSettings();
        Point desiredPreviewSize = getDesiredPreviewSize(mCameraCapabilities,
                mProfile, mUI.getPreviewScreenSize());
        mDesiredPreviewWidth = desiredPreviewSize.x;
        mDesiredPreviewHeight = desiredPreviewSize.y;
        mUI.setPreviewSize(mDesiredPreviewWidth, mDesiredPreviewHeight);
        Log.v(TAG, "Updated DesiredPreview=" + mDesiredPreviewWidth + "x"
                + mDesiredPreviewHeight);
    }

    private void resizeForPreviewAspectRatio() {
        mUI.setAspectRatio((float) mProfile.videoFrameWidth
                / mProfile.videoFrameHeight);
    }

    private void installIntentFilter() {
        // SPRD:fix bug599645 VideoModule recivedBroadcast later than CameraActivity, cause be killed
        // install an intent filter to receive SD card related events.
        // IntentFilter intentFilter = new IntentFilter(Intent.ACTION_MEDIA_EJECT);
        // intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_STARTED);
        // intentFilter.addDataScheme("file");
        mReceiver = new MyBroadcastReceiver();
        // mActivity.registerReceiver(mReceiver, intentFilter);
        mActivity.registerMediaBroadcastReceiver(mReceiver);
    }

    private void setDisplayOrientation() {
        mDisplayRotation = CameraUtil.getDisplayRotation();
        Characteristics info = mActivity.getCameraProvider()
                .getCharacteristics(mCameraId);
        mCameraDisplayOrientation = info
                .getPreviewOrientation(mDisplayRotation);
        // Change the camera display orientation
        if (mCameraDevice != null) {
            mCameraDevice.setDisplayOrientation(mDisplayRotation);
        }
        if (mFocusManager != null) {
            mFocusManager.setDisplayOrientation(mCameraDisplayOrientation);
        }
    }

    @Override
    public void updateCameraOrientation() {
        if (mMediaRecorderRecording) {
            return;
        }
        if (mDisplayRotation != CameraUtil.getDisplayRotation()) {
            setDisplayOrientation();
        }
    }

    @Override
    public void updatePreviewAspectRatio(float aspectRatio) {
        mAppController.updatePreviewAspectRatio(aspectRatio);
    }

    /**
     * Returns current Zoom value, with 1.0 as the value for no zoom.
     */
    private float currentZoomValue() {
        return mCameraSettings.getCurrentZoomRatio();
    }

    @Override
    public void onZoomChanged(float ratio) {
        // Not useful to change zoom value when the activity is paused.
        if (mPaused) {
            return;
        }
        if (isSlowMotionOn()) {
            mUI.hideZoomProcessorIfNeeded();
            ToastUtil.showToast(mActivity, R.string.slow_motion_does_not_support_zoom_change,
                    Toast.LENGTH_LONG);
            return;
        }
        mZoomValue = ratio;
        if (mCameraSettings == null || mCameraDevice == null) {
            return;
        }
        // Set zoom parameters asynchronously
        mCameraSettings.setZoomRatio(mZoomValue);
        mCameraDevice.applySettings(mCameraSettings);
    }

    private void startPreview() {
        Log.i(TAG, "startPreview");

        SurfaceTexture surfaceTexture = mActivity.getCameraAppUI()
                .getSurfaceTexture();
        if (!mPreferenceRead || surfaceTexture == null || mPaused == true
                || mCameraDevice == null) {
            return;
        }

        if (mPreviewing == true) {
            stopPreview();
        }

        setDisplayOrientation();
        mCameraDevice.setDisplayOrientation(mDisplayRotation);
        setCameraParameters();

        if (mFocusManager != null) {
            // If the focus mode is continuous autofocus, call cancelAutoFocus
            // to resume it because it may have been paused by autoFocus call.
            CameraCapabilities.FocusMode focusMode = mFocusManager
                    .getFocusMode(mCameraSettings.getCurrentFocusMode(), mDataModule.getString(Keys.KEY_FOCUS_MODE));
            if (focusMode == CameraCapabilities.FocusMode.CONTINUOUS_PICTURE) {
                mCameraDevice.cancelAutoFocus();
            }
        }

        // This is to notify app controller that preview will start next, so app
        // controller can set preview callbacks if needed. This has to happen
        // before
        // preview is started as a workaround of the framework issue related to
        // preview
        // callbacks that causes preview stretch and crash. (More details see
        // b/12210027
        // and b/12591410. Don't apply this to L, see b/16649297.
        if (!ApiHelper.isLOrHigher()) {
            Log.v(TAG, "calling onPreviewReadyToStart to set one shot callback");
            mAppController.onPreviewReadyToStart();
        } else {
            Log.v(TAG, "on L, no one shot callback necessary");
        }
        try {
            mCameraDevice.setPreviewTexture(surfaceTexture);
            mCameraDevice.startPreviewWithCallback(
                    new Handler(Looper.getMainLooper()),
                    new CameraAgent.CameraStartPreviewCallback() {
                        @Override
                        public void onPreviewStarted() {
                            VideoModule.this.onPreviewStarted();
                        }
                    });
            mPreviewing = true;
        } catch (Throwable ex) {
            closeCamera();
            throw new RuntimeException("startPreview failed", ex);
        }
    }

    private void onPreviewStarted() {
        mAppController.setShutterEnabled(true);
        mAppController.onPreviewStarted();
        if (mFocusManager != null) {
            mFocusManager.onPreviewStarted();
        }
    }

    @Override
    public void onPreviewInitialDataReceived() {
    }

    @Override
    public void stopPreview() {
        if (!mPreviewing) {
            Log.v(TAG, "Skip stopPreview since it's not mPreviewing");
            return;
        }
        if (mCameraDevice == null) {
            Log.v(TAG, "Skip stopPreview since mCameraDevice is null");
            return;
        }

        Log.v(TAG, "stopPreview");
        mCameraDevice.stopPreview();
        if (mFocusManager != null) {
            mFocusManager.onPreviewStopped();
        }
        mPreviewing = false;
    }

    private void closeCamera() {
        Log.i(TAG, "closeCamera");
        mCameraAvailable = false;
        if (mCameraDevice == null) {
            Log.d(TAG, "already stopped.");
            return;
        }
        mCameraDevice.setZoomChangeListener(null);
        mActivity.getCameraProvider()
                .releaseCamera(mCameraDevice.getCameraId());
        mCameraDevice = null;
        mPreviewing = false;
        mSnapshotInProgress = false;
        if (mFocusManager != null) {
            mFocusManager.onCameraReleased();
        }
    }

    @Override
    public boolean onBackPressed() {
        if (mPaused) {
            return true;
        }
        if (mMediaRecorderRecording) {
            onStopVideoRecording();
            return true;
        } else {
            return false;
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // Do not handle any key if the activity is paused.
        if (mPaused) {
            return true;
        }

        switch (keyCode) {
            case KeyEvent.KEYCODE_CAMERA:
            case KeyEvent.KEYCODE_VOLUME_UP:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
            /* SPRD:Bug 535058 New feature: volume @{ */
                int volumeStatus = getVolumeControlStatus(mActivity);
                if (volumeStatus == Keys.shutter || keyCode == KeyEvent.KEYCODE_CAMERA) {
                    return true;
                } else if (volumeStatus == Keys.zoom) {
                    if(mDataModuleCurrent != null &&
                            mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_DREAM_ZOOM_ENABLE_PHOTO_MODULE) &&
                            !mDataModuleCurrent.getBoolean(Keys.KEY_DREAM_ZOOM_ENABLE_PHOTO_MODULE)){
                        ToastUtil.showToast(mActivity, R.string.current_module_does_not_support_zoom_change,Toast.LENGTH_LONG);
                        return true;
                    }
                    float zoomValue;
                    if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
                        zoomValue = increaseZoomValue(mZoomValue);
                    } else {
                        zoomValue = reduceZoomValue(mZoomValue);
                    }
                    onZoomChanged(zoomValue);
                    mUI.setPreviewOverlayZoom(mZoomValue);
                    return true;
                } else if (volumeStatus == Keys.volume) {
                    return false;
                }
                return false;
            /* }@ */
            case KeyEvent.KEYCODE_DPAD_CENTER:
                if (event.getRepeatCount() == 0) {
                    onShutterButtonClick();
                    return true;
                }
            case KeyEvent.KEYCODE_MENU:
                // Consume menu button presses during capture.
                return mMediaRecorderRecording;
        }
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_CAMERA:
            case KeyEvent.KEYCODE_VOLUME_UP:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
            /*
             * SPRD:fix bug 546663 We will not handle the event because activity is in review
             * mode@{
             */
                if (isInReviewMode()) {
                    return true;
                }
            /* }@ */
            /*
             * SPRD:fix bug518054 ModeListView is appear when begin to capture using volume
             * key@{
             */
                mActivity.getCameraAppUI().hideModeList();
            /* }@ */
            /* SPRD:fix bug499275 Front DC can not capture through the Bluetooth@{ */
                mActivity.getCameraAppUI().closeModeOptions();
            /* }@ */
            /* SPRD:Bug 535058 New feature: volume @{ */
                int volumeStatus = getVolumeControlStatus(mActivity);
                if (volumeStatus == Keys.shutter || keyCode == KeyEvent.KEYCODE_CAMERA) {
                    onShutterButtonFocus(true);
                    onShutterButtonClick();
                    return true;
                } else if (volumeStatus == Keys.zoom) {
                    mUI.hideZoomUI();
                    return true;
                } else if (volumeStatus == Keys.volume) {
                    return false;
                }
                return false;
            /* }@ */
            case KeyEvent.KEYCODE_MENU:
                // Consume menu button presses during capture.
                return mMediaRecorderRecording;
        }
        return false;
    }

    @Override
    public boolean isVideoCaptureIntent() {
        String action = mActivity.getIntent().getAction();
        return (MediaStore.INTENT_ACTION_VIDEO_CAMERA.equals(action)
                || MediaStore.ACTION_VIDEO_CAPTURE.equals(action));
    }

    public boolean shouldShowResult() {
        String action = mActivity.getIntent().getAction();
        return (MediaStore.ACTION_VIDEO_CAPTURE.equals(action));
    }

    private void doReturnToCaller(boolean valid) {
        if(!played && mVideoFileDescriptor == null){
            Log.e(TAG, "VideoThumbnail loading is not complete");
            return;
        }
        Intent resultIntent = new Intent();
        int resultCode;
        if (valid) {
            resultCode = Activity.RESULT_OK;
            resultIntent.setData(mCurrentVideoUri);
            resultIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } else {
            resultCode = Activity.RESULT_CANCELED;
        }
        mActivity.setResultEx(resultCode, resultIntent);
        mActivity.finish();
    }

    private void cleanupEmptyFile() {
        if (mVideoFilename != null) {
            File f = new File(mVideoFilename);
            if (f.length() == 0 && f.delete()) {
                Log.v(TAG, "Empty video file deleted: " + mVideoFilename);
                mVideoFilename = null;
            }
        }
    }

    // Prepares media recorder.
    private void initializeRecorder() {
        Log.i(TAG, "initializeRecorder: " + Thread.currentThread());
        // If the mCameraDevice is null, then this activity is going to finish
        if (mCameraDevice == null) {
            Log.w(TAG, "null camera proxy, not recording");
            return;
        }

        updateCameraShutterSound();

        Intent intent = mActivity.getIntent();
        Bundle myExtras = intent.getExtras();

        long requestedSizeLimit = 0;
        closeVideoFileDescriptor();
        mCurrentVideoUriFromMediaSaved = false;
        if (mIsVideoCaptureIntent && myExtras != null) {
            Uri saveUri = (Uri) myExtras.getParcelable(MediaStore.EXTRA_OUTPUT);
            if (saveUri != null) {
                try {
                    mVideoFileDescriptor = mContentResolver.openFileDescriptor(
                            saveUri, "rw");
                    mCurrentVideoUri = saveUri;
                } catch (java.io.FileNotFoundException ex) {
                    // invalid uri
                    Log.e(TAG, ex.toString());
                }
            }
            requestedSizeLimit = myExtras.getLong(MediaStore.EXTRA_SIZE_LIMIT);
        }
        // SPRD: For bug 539723 add log
        Profile guard = Profilers.instance().guard("initializeRecorder");

        /*
         * SPRD Bug:590419 set 64-bit data file flag interface for camera use
         *
         * @{ Original Android code:
         * mMediaRecorder = new MediaRecorder();
         */
        mMediaRecorder = (MediaRecorder) DreamProxy.getMediaRecoder();
        /*
         * @}
         */
        // Unlock the camera object before passing it to media recorder.
        mCameraDevice.unlock();
        // We rely here on the fact that the unlock call above is synchronous
        // and blocks until it occurs in the handler thread. Thereby ensuring
        // that we are up to date with handler requests, and if this proxy had
        // ever been released by a prior command, it would be null.
        Camera camera = mCameraDevice.getCamera();
        Log.i(TAG, "camera = " + camera);
        // If the camera device is null, the camera proxy is stale and recording
        // should be ignored.
        if (camera == null) {
            Log.w(TAG, "null camera within proxy, not recording");
            return;
        }

        mMediaRecorder.setCamera(camera);

        /* SPRD: Fix bug 578587 that Video module does not support 4G+ video recording @{ */
        boolean vFatType = false;
        String storagePath = StorageUtil.getInstance().getFileDir();
        StorageManager storageManager = mActivity.getSystemService(StorageManager.class);
        List<VolumeInfo> vols = storageManager.getVolumes();

        for (VolumeInfo vol : vols) {
            if (vol != null) {
                String fsType = vol.fsType;
                String path = vol.path;
                if (null != path && "vfat".equals(fsType) && storagePath.startsWith(path)) {
                    showNoSupportHint();
                    vFatType = true;
                    break;
                }
            }
        }

        DreamProxy.setParam64BitFileOffset(!vFatType);
        /* @} */
        /*
         * SPRD Bug:474696 Feature:Slow-Motion. @{ Original Android code:
         */
         int audioSource = getRecorderAudioSoruceBySlowMotion(mIsVideoCaptureIntent);
         mMediaRecorder.setAudioSource(audioSource);
        /* @} */

        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);

        // SPRD Bug:474701 Feature:Video Encoding Type.
        mProfile.videoCodec = getVideoEncodeType();

        mMediaRecorder.setProfile(mProfile);
        mMediaRecorder.setVideoSize(mProfile.videoFrameWidth,
                mProfile.videoFrameHeight);
        mMediaRecorder.setMaxDuration(mMaxVideoDurationInMs);

        if (mCaptureTimeLapse) {
            double fps = 1000 / (double) mTimeBetweenTimeLapseFrameCaptureMs;
            setCaptureRate(mMediaRecorder, fps);
        }

        //Fix Bug631188 Video need know fps of slow motion
        String slowMotionValue = mDataModuleCurrent.getString(Keys.KEY_VIDEO_SLOW_MOTION);
        if (slowMotionValue != null) {
            int slow_motion = Integer.valueOf(slowMotionValue);
            double fps = slow_motion * 30;
            setSlowMotionRate(mMediaRecorder, fps);
        }
        setRecordLocation();
        // Set output file.
        // Try Uri in the intent first. If it doesn't exist, use our own
        // instead.
        if (mVideoFileDescriptor != null) {
            mMediaRecorder.setOutputFile(mVideoFileDescriptor
                    .getFileDescriptor());
        } else {
            generateVideoFilename(mProfile.fileFormat);
            mMediaRecorder.setOutputFile(mVideoFilename);
        }

        // Set maximum file size.
        long maxFileSize = mActivity.getStorageSpaceBytes()
                - Storage.LOW_STORAGE_THRESHOLD_BYTES;
        if (requestedSizeLimit > 0 && requestedSizeLimit < maxFileSize) {
            maxFileSize = requestedSizeLimit;
        }

        try {
            mMediaRecorder.setMaxFileSize(maxFileSize);
        } catch (RuntimeException exception) {
            // We are going to ignore failure of setMaxFileSize here, as
            // a) The composer selected may simply not support it, or
            // b) The underlying media framework may not handle 64-bit range
            // on the size restriction.
        }

        int sensorOrientation = mActivity.getCameraProvider()
                .getCharacteristics(mCameraId).getSensorOrientation();
        int deviceOrientation = mAppController.getOrientationManager()
                .getDeviceOrientation().getDegrees();
        int rotation = CameraUtil.getImageRotation(sensorOrientation,
                deviceOrientation, isCameraFrontFacing());
        mMediaRecorder.setOrientationHint(rotation);

        try {
            // SPRD: For bug 539723 add log
            guard.mark();
            mMediaRecorder.prepare();
            // SPRD: For bug 539723 add log
            guard.stop("MediaRecorder prepare");
        } catch (IOException e) {
            Log.e(TAG, "prepare failed for " + mVideoFilename, e);
            releaseMediaRecorder();
            mAppController.getCameraAppUI().setSwipeEnabled(true);// SPRD:fix
            // 527653
            throw new RuntimeException(e);
        }

        mMediaRecorder.setOnErrorListener(this);
        mMediaRecorder.setOnInfoListener(this);
        Log.i(TAG, "initializeRecorder end");
    }

    /* SPRD: Fix bug 625678 add hint when sd card supports 4k recording at the most in the first*/
    public void showNoSupportHint() {
        boolean shouldNoSupportHint = mDataModule.getBoolean(Keys.KEY_CAMERA_VIDEO_HINT);
        if (shouldNoSupportHint == true) {
            Toast.makeText(mActivity, R.string.video_maximum_size_limit, Toast.LENGTH_LONG).show();
            mDataModule.set(Keys.KEY_CAMERA_VIDEO_HINT, false);
        }
    }
    /* @} */

    private void setRecordLocation() {
        Location loc = mLocationManager.getCurrentLocation();
        if (loc != null) {
            mMediaRecorder.setLocation((float) loc.getLatitude(),
                    (float) loc.getLongitude());
        }
    }

    private void releaseMediaRecorder() {
        Log.i(TAG, "Releasing media recorder.");
        if (mMediaRecorder != null) {
            cleanupEmptyFile();
            mMediaRecorder.reset();
            mMediaRecorder.release();
            mMediaRecorder = null;
        }
        mVideoFilename = null;
    }

    private void generateVideoFilename(int outputFileFormat) {
        long dateTaken = System.currentTimeMillis();
        String title = createName(dateTaken);
        // Used when emailing.
        String filename = title
                + convertOutputFormatToFileExt(outputFileFormat);
        String mime = convertOutputFormatToMimeType(outputFileFormat);
        /*
         * SPRD: Change storage videopath for storage path Feature String path =
         * Storage.DIRECTORY + '/' + filename;
         */
        StorageUtil storageUtil = StorageUtil.getInstance();
        String path = storageUtil.getFileDir() + '/' + filename;

        String tmpPath = path + ".tmp";
        mCurrentVideoValues = new ContentValues(9);
        mCurrentVideoValues.put(Video.Media.TITLE, title);
        mCurrentVideoValues.put(Video.Media.DISPLAY_NAME, filename);
        mCurrentVideoValues.put(Video.Media.DATE_TAKEN, dateTaken);
        mCurrentVideoValues.put(MediaColumns.DATE_MODIFIED, dateTaken / 1000);
        mCurrentVideoValues.put(Video.Media.MIME_TYPE, mime);
        mCurrentVideoValues.put(Video.Media.DATA, path);
        mCurrentVideoValues.put(Video.Media.WIDTH, mProfile.videoFrameWidth);
        mCurrentVideoValues.put(Video.Media.HEIGHT, mProfile.videoFrameHeight);
        mCurrentVideoValues.put(
                Video.Media.RESOLUTION,
                Integer.toString(mProfile.videoFrameWidth) + "x"
                        + Integer.toString(mProfile.videoFrameHeight));
        Location loc = mLocationManager.getCurrentLocation();
        if (loc != null) {
            mCurrentVideoValues.put(Video.Media.LATITUDE, loc.getLatitude());
            mCurrentVideoValues.put(Video.Media.LONGITUDE, loc.getLongitude());
        }
        mVideoFilename = tmpPath;
        Log.v(TAG, "New video filename: " + mVideoFilename);
    }

    private void logVideoCapture(long duration) {
        String flashSetting = mActivity.getSettingsManager().getString(
                mAppController.getCameraScope(),
                Keys.KEY_VIDEOCAMERA_FLASH_MODE);
        boolean gridLinesOn = Keys.areGridLinesOn(mActivity
                .getSettingsManager());
        int width = (Integer) mCurrentVideoValues.get(Video.Media.WIDTH);
        int height = (Integer) mCurrentVideoValues.get(Video.Media.HEIGHT);
        long size = new File(mCurrentVideoFilename).length();
        String name = new File(
                mCurrentVideoValues.getAsString(Video.Media.DATA)).getName();
        UsageStatistics.instance().videoCaptureDoneEvent(name, duration,
                isCameraFrontFacing(), currentZoomValue(), width, height, size,
                flashSetting, gridLinesOn);
    }

    private void saveVideo() {
        if (mVideoFileDescriptor == null) {

            // SPRD Bug:474704 Feature:Video Recording Pause.
            calculatePauseDuration(mPauseRecorderRecording);

            long duration = SystemClock.uptimeMillis() - mRecordingStartTime - mAllPauseTime;
            if (duration > 0) {
                // TimeLapse
                if (mCaptureTimeLapse) {
                    duration = duration
                            * 1000
                            / (long) Math
                            .ceil(30 * mTimeBetweenTimeLapseFrameCaptureMs);
                    Log.d(TAG, "saveVideo duration=" + duration);
                } else {
                    String sSlowMotion = mDataModuleCurrent
                            .getString(Keys.KEY_VIDEO_SLOW_MOTION);
                    if (sSlowMotion != null) {
                        int slow_motion = Integer.valueOf(sSlowMotion);
                        if (slow_motion != 1) {
                            duration = duration / 1000 * 1000 * slow_motion;
                        }
                    }
                }

            } else {
                Log.w(TAG, "Video duration <= 0 : " + duration);
            }
            mCurrentVideoValues.put(Video.Media.SIZE, new File(
                    mCurrentVideoFilename).length());
            mCurrentVideoValues.put(Video.Media.DURATION, duration);
            // SPRD: fix for bug 534090 restart device video date changed
            mCurrentVideoValues.put(MediaColumns.DATE_MODIFIED, System.currentTimeMillis() / 1000);
            getServices().getMediaSaver().addVideo(mCurrentVideoFilename,
                    mCurrentVideoValues, mOnVideoSavedListener);
            logVideoCapture(duration);

            /* SPRD: Fix bug 650297 that video thumbnail is showed slowly @{ */
            final String finalName = mCurrentVideoValues.getAsString(Video.Media.DATA);
            File finalFile = new File(finalName);
            if (new File(mCurrentVideoFilename).renameTo(finalFile)) {
                // if there's snapshot, snapshot will show as thumbnail
                if (mSnapshotCount == 0) {
                    AsyncTask.THREAD_POOL_EXECUTOR.execute(new Runnable() {
                        @Override
                        public void run() {
                            mActivity.startPeekAnimation(
                                    FilmstripItemUtils.loadVideoThumbnail(finalName));
                        }
                    });
                }
            }
            /* @} */
        }
        mCurrentVideoValues = null;
    }

    private void deleteVideoFile(String fileName) {
        Log.v(TAG, "Deleting video " + fileName);
        File f = new File(fileName);
        if (!f.delete()) {
            Log.v(TAG, "Could not delete " + fileName);
        }
    }

    // from MediaRecorder.OnErrorListener
    @Override
    public void onError(MediaRecorder mr, int what, int extra) {
        Log.e(TAG, "MediaRecorder error. what=" + what + ". extra=" + extra);
        if (what == MediaRecorder.MEDIA_RECORDER_ERROR_UNKNOWN) {
            // We may have run out of space on the sdcard.
            stopVideoRecording();
            mActivity.updateStorageSpaceAndHint(null);
        }
    }

    // from MediaRecorder.OnInfoListener
    @Override
    public void onInfo(MediaRecorder mr, int what, int extra) {
        if (what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_DURATION_REACHED) {
            if (mMediaRecorderRecording) {
                onStopVideoRecording();
            }
        } else if (what == MediaRecorder.MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED) {
            if (mMediaRecorderRecording) {
                onStopVideoRecording();
            }

            // Show the toast.
            ToastUtil.showToast(mActivity, R.string.video_reach_size_limit, ToastUtil.LENGTH_LONG);
        }
        /* SPRD: Add for bug 559531 @{ */
//        else if (what == MediaRecorder.MEDIA_RECORDER_INFO_TRACKS_HAVE_DATA) {
//            if (mHandler.hasMessages(MSG_ENABLE_SHUTTER_PAUSE_BUTTON)) {
//                mHandler.removeMessages(MSG_ENABLE_SHUTTER_PAUSE_BUTTON);
//            }
//            mHandler.sendEmptyMessage(MSG_ENABLE_SHUTTER_PAUSE_BUTTON);
//        }
        /* @} */
    }

    /*
     * Make sure we're not recording music playing in the background, ask the
     * MediaPlaybackService to pause playback.
     */
    /*
     * SPRD: fix bug492439 Ture on FM, Camera can not record @{
     * original code

    private void silenceSoundsAndVibrations() {
        // Get the audio focus which causes other music players to stop.
        // SPRD:fix bug514208 when phone rings,camere can't record
        mAudioManager.requestAudioFocus(null, AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN);
        // Store current ringer mode so we can set it once video recording is
        // finished.
        mOriginalRingerMode = mAudioManager.getRingerMode();
        // Make sure no system sounds and vibrations happen during video
        // recording.
        mAudioManager.setRingerMode(AudioManager.RINGER_MODE_SILENT);
    }
     */
    private boolean silenceSoundsAndVibrations() {
        // Get the audio focus which causes other music players to stop.
        int ret = mAudioManager.requestAudioFocus(null, AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN);
        Log.i(TAG, "requestAudioFocus " + ret);
        if (ret != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            return false;
        }
        // Store current ringer mode so we can set it once video recording is
        // finished.
        mOriginalRingerMode = mAudioManager.getRingerMode();
        // Make sure no system sounds and vibrations happen during video
        // recording.
        mAudioManager.setRingerMode(AudioManager.RINGER_MODE_SILENT);

        return true;
    }
    /* @} */

    private void restoreRingerMode() {
        // First check if ringer mode was changed during the recording. If not,
        // re-set the mode that was set before video recording started.
        if (mAudioManager.getRingerMode() == AudioManager.RINGER_MODE_SILENT) {
            mAudioManager.setRingerMode(mOriginalRingerMode);
        }
    }

    // For testing.
    public boolean isRecording() {
        return mMediaRecorderRecording;
    }

    private void startVideoRecording() {
        Log.i(TAG, "startVideoRecording: " + Thread.currentThread());

         /*
          * SPRD Bug:474704 Feature:Video Recording Pause. @{
          * Original Android code: mUI.showRecordingUI(true);
          */
        mUI.showRecordingUI(true, mActivity
                .getOrientationManager()
                .getDisplayRotation().getDegrees());
         /* @} */

        mUI.cancelAnimations();
        /*
         * SPRD:fix 527653 Monkey is so fast that 2ms later after
         * startVideoRecording called, it shows the filmStrip, and then edit a
         * photo, which will take a long time, which caused ANR. setSwipeEnabled
         * at the begin of startVideoRecording to avoid ANR. @{
         */
        mAppController.getCameraAppUI().setSwipeEnabled(false);
        /* }@ */
        mUI.setSwipingEnabled(false);
        mUI.hidePassiveFocusIndicator();
        mAppController.getCameraAppUI().hideCaptureIndicator();
        mAppController.getCameraAppUI().setShouldSuppressCaptureIndicator(true);
        updateMakeUpUI(View.GONE);

        mActivity.updateStorageSpaceAndHint(new CameraActivity.OnStorageUpdateDoneListener() {
            @Override
            public void onStorageUpdateDone(long bytes) {
                if (bytes <= Storage.LOW_STORAGE_THRESHOLD_BYTES) {
                    Log.w(TAG, "Storage issue, ignore the start request");
                    /* SPRD: fix bug 538868 video may not switch camera@{*/
                    //mAppController.getCameraAppUI().enableCameraToggleButton();
                    /* @} */
                    mAppController.getCameraAppUI().setSwipeEnabled(true);//SPRD:fix 527653
                } else {
                    if (mCameraDevice == null) {
                        Log.v(TAG, "in storage callback after camera closed");
                        mAppController.getCameraAppUI().setSwipeEnabled(true);//SPRD:fix 527653
                        /* SPRD: fix bug 538868 video may not switch camera@{*/
                        //mAppController.getCameraAppUI().enableCameraToggleButton();
                        /* @} */
                        return;
                    }
                    if (mPaused == true) {
                        Log.v(TAG, "in storage callback after module paused");
                        mAppController.getCameraAppUI().setSwipeEnabled(true);//SPRD:fix 527653
                        /* SPRD: fix bug 538868 video may not switch camera@{*/
                        //mAppController.getCameraAppUI().enableCameraToggleButton();
                        /* @} */
                        return;
                    }

                    // Monkey is so fast so it could trigger startVideoRecording twice. To prevent
                    // app crash (b/17313985), do nothing here for the second storage-checking
                    // callback because recording is already started.
                    if (mMediaRecorderRecording) {
                        Log.v(TAG, "in storage callback after recording started");
                        mAppController.getCameraAppUI().setSwipeEnabled(true);//SPRD:fix 527653
                        /* SPRD: fix bug 538868 video may not switch camera@{*/
                        //mAppController.getCameraAppUI().enableCameraToggleButton();
                        /* @} */
                        return;
                    }

                    mCurrentVideoUri = null;

                    initializeRecorder();
                    if (mMediaRecorder == null) {
                        Log.e(TAG, "Fail to initialize media recorder");
                        mAppController.getCameraAppUI().setSwipeEnabled(true);//SPRD:fix 527653
                        /* SPRD: fix bug 538868 video may not switch camera@{*/
                        //mAppController.getCameraAppUI().enableCameraToggleButton();
                        /* @} */
                        return;
                    }

                    try {
                        /* SPRD:fix 520894 silence the value of STREAM_MUSIC @{*/
                        /* SPRD:fix bug492439 Ture on FM, Camera can not record@{*/
                        /* SPRD:fix bug514208 when phone rings,camere can't record@{*/
                        if (!silenceSoundsAndVibrations()) {
                            // Fix bug 540246 recording and music work together after we end call
                            mActivity.sendBroadcast(new Intent(PAUSE_ACTION));
                        }
                        /*}@*/
                        mMediaRecorder.start(); // Recording is now started
                    } catch (IllegalStateException exception) {
                        Log.e(TAG, "Could not start media recorder(start failed). ", exception);
                        mAppController.getCameraAppUI().setSwipeEnabled(true);
                        mAppController.getFatalErrorHandler().onMediaRecorderDisabledFailure();
                        releaseMediaRecorder();
                        restoreRingerMode();
                        mCameraDevice.lock();
                        return;
                    } catch (RuntimeException e) {
                        Log.e(TAG, "Could not start media recorder. ", e);
                        mAppController.getCameraAppUI().setSwipeEnabled(true);//SPRD:fix 527653
                        mAppController.getFatalErrorHandler().onGenericCameraAccessFailure();
                        releaseMediaRecorder();
                        /* SPRD:fix 520894 recover the value of STREAM_MUSIC @{*/
                        restoreRingerMode();
                        /*}@*/
                        // If start fails, frameworks will not lock the camera for us.
                        mCameraDevice.lock();
                        return;
                    }
                    // Make sure we stop playing sounds and disable the
                    // vibrations during video recording. Post delayed to avoid
                    // silencing the recording start sound.
                    /* SPRD:fix bug492439 Ture on FM, Camera can not record@{
                    mHandler.postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            silenceSoundsAndVibrations();
                        }
                    }, 250);
                    }@*/
                    /* SPRD:fix 527653 Start recording, Camera can not swipe. @{
                    mAppController.getCameraAppUI().setSwipeEnabled(false);
                    }@*/

                    // The parameters might have been altered by MediaRecorder already.
                    // We need to force mCameraDevice to refresh before getting it.
                    mCameraDevice.refreshSettings();
                    // The parameters may have been changed by MediaRecorder upon starting
                    // recording. We need to alter the parameters if we support camcorder
                    // zoom. To reduce latency when setting the parameters during zoom, we
                    // update the settings here once.
                    mCameraSettings = mCameraDevice.getSettings();

                    // SPRD Bug:474704 Feature:Video Recording Pause.
                    mResultTime = 0;
                    mAllPauseTime = 0;

                    mMediaRecorderRecording = true;

                            /*
                             * SPRD Bug:519334 Refactor Rotation UI of Camera.
                             *
                             * @{ Original Android code:
                             * mActivity.lockOrientation();
                             */

                    mRecordingStartTime = SystemClock.uptimeMillis();

                    // A special case of mode options closing: during
                    // capture it should
                    // not be possible to change mode state.
                    mAppController.getCameraAppUI().hideModeOptions();

                            /*
                             * SPRD Bug:529008 Animate just once. @{ Original
                             * Android code: mAppController.getCameraAppUI
                             * ().animateBottomBarToVideoStop
                             * (R.drawable.ic_stop);
                             */
                    // ui check 81
                    mAppController.getCameraAppUI()
                            .animateBottomBarToVideoStop(
                                    mShutterIconId, R.drawable.dream_ic_capture_video_stop);
                            /* @} */

                            /* SPRD: Fix bug 559531 @{ */
                    enableShutterAndPauseButton(false);
                    // bug 612193 pause video at 0 second can be done after stop video ant 1 second
                    if (mHandler.hasMessages(MSG_ENABLE_SHUTTER_PAUSE_BUTTON)) {
                        mHandler.removeMessages(MSG_ENABLE_SHUTTER_PAUSE_BUTTON);
                    }
                    mHandler.sendEmptyMessageDelayed(
                            MSG_ENABLE_SHUTTER_PAUSE_BUTTON, ENABLE_SHUTTER_PAUSE_BUTTON_TIMEOUT);

                    setFocusParameters();

                    updateRecordingTime();
                    mActivity.enableKeepScreenOn(true);
                    /* SPRD: Dream Camera ui check 71 @{ */
                    if (isSlowMotionOn() || isRecordingIn4k() || isEOISOn()) {//SPRD:fix bug 615391 EOIS and 4k not support snap shot
                        mAppController.getCameraAppUI().setSlowMotionCaptureButtonDisable();
                    }
                    /* @} */
                    // SPRD: Fix bug 650297 that video thumbnail is showed slowly
                    mSnapshotCount = 0;
                }
            }
        });

    }

    private Bitmap getVideoThumbnail() {
        Bitmap bitmap = null;
        if (mVideoFileDescriptor != null) {
            bitmap = Thumbnail.createVideoThumbnailBitmap(
                    mVideoFileDescriptor.getFileDescriptor(),
                    mDesiredPreviewWidth);
        } else if (mCurrentVideoUri != null) {
            try {
                mVideoFileDescriptor = mContentResolver.openFileDescriptor(
                        mCurrentVideoUri, "r");
                bitmap = Thumbnail.createVideoThumbnailBitmap(
                        mVideoFileDescriptor.getFileDescriptor(),
                        mDesiredPreviewWidth);
            } catch (java.io.FileNotFoundException ex) {
                // invalid uri
                Log.e(TAG, ex.toString());
            }
        }

        if (bitmap != null) {
            // MetadataRetriever already rotates the thumbnail. We should rotate
            // it to match the UI orientation (and mirror if it is front-facing
            // camera).
            bitmap = CameraUtil.rotateAndMirror(bitmap, 0,
                    isCameraFrontFacing());
        }
        return bitmap;
    }

    private void showCaptureResult() {
        mIsInReviewMode = true;
        played = false;
        Bitmap bitmap = getVideoThumbnail();
        if (bitmap != null) {
            mUI.showReviewImage(bitmap);
        }
        mUI.showReviewControls();
    }

    /* SPRD: Fix bug548010 Camera occurs error when connect USB during video recording @{ */
    private boolean stopVideoRecording() {
        return stopVideoRecording(true);
    }

    /* SPRD: Fix bug548010 Camera occurs error when connect USB during video recording @{
     * original code
    private boolean stopVideoRecording() {
     */
    private boolean stopVideoRecording(boolean shouldSaveVideo) {
    /* @} */
        // Do nothing if camera device is still capturing photo. Monkey test can trigger app crashes
        // (b/17313985) without this check. Crash could also be reproduced by continuously tapping
        // on shutter button and preview with two fingers.
        /*
         * SPRD Bug:510390 open error when snap shot is processing onpause@{
         * Original Android code: if (mSnapshotInProgress) {
         */
        /* SPRD: fix for bug 537147 video recording and pull SD card, camera can not switch the camera @{ */
        //mAppController.getCameraAppUI().enableCameraToggleButton();
        /* @} */
        if (mSnapshotInProgress && !mPaused) {
            /* @} */
            Log.v(TAG, "Skip stopVideoRecording since snapshot in progress");
            return true;
        }
        Log.v(TAG, "stopVideoRecording");

        // Re-enable sound as early as possible to avoid interfering with stop
        // recording sound.
        abandonAudioPlayback();
        restoreRingerMode();

        mUI.setSwipingEnabled(true);
        /*
         * SPRD:fix bug 501841 video record stop,focus ring shows
         * mUI.showPassiveFocusIndicator();
         */

        enableShutterAndPauseButton(true);
        mAppController.getCameraAppUI()
                .setShouldSuppressCaptureIndicator(false);
        updateMakeUpUI(View.VISIBLE);
        boolean fail = false;
        if (mMediaRecorderRecording) {
            boolean shouldAddToMediaStoreNow = false;

            try {
                mMediaRecorder.setOnErrorListener(null);
                mMediaRecorder.setOnInfoListener(null);

                // SPRD Bug:474704 Feature:Video Recording Pause.
                long beforStopTime = SystemClock.uptimeMillis();
                Log.i(TAG, "MediaRecorder.stop ......    starttime = "
                        + beforStopTime);

                mMediaRecorder.stop();

                // SPRD Bug:474704 Feature:Video Recording Pause.
                long afterStopTime = SystemClock.uptimeMillis();
                mResultTime += (afterStopTime - beforStopTime);
                Log.i(TAG, "MediaRecorder.stop ......    endtime = "
                        + (afterStopTime - beforStopTime));

                shouldAddToMediaStoreNow = true;
                mCurrentVideoFilename = mVideoFilename;
                Log.v(TAG, "stopVideoRecording: current video filename: "
                        + mCurrentVideoFilename);
            } catch (RuntimeException e) {
                Log.e(TAG, "stop fail", e);
                if (mVideoFilename != null) {
                    deleteVideoFile(mVideoFilename);
                }
                fail = true;
            }
            mMediaRecorderRecording = false;

            /*
             * SPRD Bug:519334 Refactor Rotation UI of Camera. @{ Original
             * Android code: mActivity.unlockOrientation();
             */

            // If the activity is paused, this means activity is interrupted
            // during recording. Release the camera as soon as possible because
            // face unlock or other applications may need to use the camera.
            if (mPaused) {
                // b/16300704: Monkey is fast so it could pause the module while
                // recording.
                // stopPreview should definitely be called before switching off.
                stopPreview();
                closeCamera();
            }

            /*
             * SPRD Bug:474704 Feature:Video Recording Pause. @{ Original
             * Android code: mUI.showRecordingUI(false);
             */
            mUI.showRecordingUI(false, mActivity.getOrientationManager()
                    .getDisplayRotation().getDegrees());
            /* @} */

            // The orientation was fixed during video recording. Now make it
            // reflect the device orientation as video recording is stopped.
            mUI.setOrientationIndicator(0, true);
            mActivity.enableKeepScreenOn(false);
            /* SPRD: fix bug548010 Camera occurs error when connect USB during video recording @{
            if (shouldAddToMediaStoreNow && !fail) {
             */
            if (shouldAddToMediaStoreNow && !fail && shouldSaveVideo) {
            /* @} */
                if (mVideoFileDescriptor == null) {
                    saveVideo();
                } else if (shouldShowResult()) {
                    // if no file save is needed, we can show the post capture
                    // UI now
                    showCaptureResult();
                }
            }
        }
        // release media recorder
        releaseMediaRecorder();

        mAppController.getCameraAppUI().showModeOptions();

        /*
         * SPRD Bug:529008 Animate just once. @{ Original Android code:
         * mAppController
         * .getCameraAppUI().animateBottomBarToFullSize(mShutterIconId);
         */

        mAppController.getCameraAppUI().animateBottomBarToFullSize(R.drawable.dream_ic_capture_video_stop,
                mShutterIconId);
        /* @} */

        mAppController.getCameraAppUI().setSwipeEnabled(true);// SPRD:Fix
        // bug391820

        if (!mPaused && mCameraDevice != null) {
            setFocusParameters();
            mCameraDevice.lock();
            if (!ApiHelper.HAS_SURFACE_TEXTURE_RECORDING) {
                stopPreview();
                // Switch back to use SurfaceTexture for preview.
                startPreview();
            }
            // Update the parameters here because the parameters might have been
            // altered
            // by MediaRecorder.
            mCameraSettings = mCameraDevice.getSettings();
        }

        // Check this in advance of each shot so we don't add to shutter
        // latency. It's true that someone else could write to the SD card
        // in the mean time and fill it, but that could have happened
        // between the shutter press and saving the file too.
        mActivity.updateStorageSpaceAndHint(null);

        // SPRD Bug:474704 Feature:Video Recording Pause.
        mPauseRecorderRecording = false;

        return fail;
    }

    private void updateRecordingTime() {
        if (!mMediaRecorderRecording) {
            return;
        }
        long now = SystemClock.uptimeMillis();

        /*
         * SPRD Bug:474704 Feature:Video Recording Pause. @{ Original Android
         * code: long delta = now - mRecordingStartTime;
         */
        long delta = now - mRecordingStartTime - mResultTime;
        /* @} */

        // Starting a minute before reaching the max duration
        // limit, we'll countdown the remaining time instead.
        boolean countdownRemainingTime = (mMaxVideoDurationInMs != 0 && delta >= mMaxVideoDurationInMs - 60000);

        long deltaAdjusted = delta;
        if (countdownRemainingTime) {
            deltaAdjusted = Math.max(0, mMaxVideoDurationInMs - deltaAdjusted) + 999;
        }
        String text;

        long targetNextUpdateDelay;

        text = millisecondToTimeString(deltaAdjusted, false);
        targetNextUpdateDelay = 1000;

        mUI.setRecordingTime(text);

        if (mRecordingTimeCountsDown != countdownRemainingTime) {
            // Avoid setting the color on every update, do it only
            // when it needs changing.
            mRecordingTimeCountsDown = countdownRemainingTime;

            int color = mActivity.getResources().getColor(
                    R.color.recording_time_remaining_text);

            mUI.setRecordingTimeTextColor(color);
        }

        long actualNextUpdateDelay = targetNextUpdateDelay
                - (delta % targetNextUpdateDelay);
        mHandler.sendEmptyMessageDelayed(MSG_UPDATE_RECORD_TIME,
                actualNextUpdateDelay);
    }
    /* @} */

    @Override
    public void onDreamSettingChangeListener(HashMap<String, String> keys) {
        Log.e(TAG, "dream video module onDreamSettingChangeListener  ");
        if (mCameraDevice == null) {
            return;
        }
        DataModuleManager.getInstance(
                mAppController.getAndroidContext()).getCurrentDataModule();

        for (String key : keys.keySet()) {
            Log.e(TAG,
                    "onSettingChanged key = " + key + " value = "
                            + keys.get(key));
            switch (key) {
                case Keys.KEY_VIDEO_QUALITY_BACK:
                case Keys.KEY_VIDEO_QUALITY_FRONT:
                /* nj dream camera test 78, 128 @{ */
                    restartPreview();
                    //@}
                    return;
                case Keys.KEY_VIDEO_ENCODE_TYPE:
                    break;
                case Keys.KEY_VIDEO_ANTIBANDING:
                    updateParametersAntibanding();
                    break;
//            case Keys.KEY_DREAM_COMPOSITION_LINE:
//                break;
                case Keys.KEY_VIDEO_WHITE_BALANCE:
                    updateParametersWhiteBalance();
                    break;
                case Keys.KEY_VIDEO_COLOR_EFFECT:
                    updateParametersColorEffect();
                    break;
//            case Keys.KEY_CAMERA_VIDEO_STABILIZATION:
//                break;
//            case Keys.KEY_CAMERA_MICROPHONE_SWITCH:
//                break;
                case Keys.KEY_VIDEO_SLOW_MOTION:
                    setSlowmotionParameters();
                    break;
                case Keys.KEY_VIDEO_TIME_LAPSE_FRAME_INTERVAL:
                    updateTimeLapse();// SPRD: for bug 509708 add time lapse
                    break;
//            case Keys.KEY_GIF_MODE_PIC_SIZE:
//                break;
//            case Keys.KEY_GIF_MODE_NUM_SIZE:
//                break;
                case Keys.KEY_VIDEOCAMERA_FLASH_MODE:
                    if (mPaused) {
                        break;
                    }
                    // Update flash parameters.
                    enableTorchMode(true);
                    break;
                case Keys.KEY_EOIS_DV_FRONT:
                case Keys.KEY_EOIS_DV_BACK:
                    //updateParametersEOIS();
                    restartPreview();
                    break;
                //nj dream camera test 70
                case Keys.KEY_CAMERA_GRID_LINES:
                    updateParametersGridLine();
                    break;
                case Keys.KEY_VIDEO_BEAUTY_ENTERED:
                    updateMakeLevel();
                    break;
                default:
                    break;
            }
        }

        if (mCameraDevice != null) {
            mCameraDevice.applySettings(mCameraSettings);
            mCameraSettings = mCameraDevice.getSettings();
        }

        mActivity.getCameraAppUI().initSidePanel();
        // Update UI based on the new parameters.
        mUI.updateOnScreenIndicators(mCameraSettings);
    }

    private void restartPreview() {
        stopPreview();
        if (!isVideoCaptureIntent())
        mAppController.getCameraAppUI().freezeScreenUntilPreviewReady();
        /* SPRD: fix bug620875 need make sure freeze screen draw on Time @{ */
        mHandler.post(new Runnable() {
            public void run() {
                if (!mPaused && mCameraDevice != null) {
                    startPreview();
                }
            }
        });
        /* @} */
    }

    @SuppressWarnings("deprecation")
    private void setCameraParameters() {
        SettingsManager settingsManager = mActivity.getSettingsManager();
        DataModuleManager.getInstance(
                mAppController.getAndroidContext()).getCurrentDataModule();
        // Update Desired Preview size in case video camera resolution has
        // changed.
        mCameraSettings.setDefault(mCameraId);//SPRD:fix bug616836 add for api1 use reconnect
        readVideoPreferences();
        updateDesiredPreviewSize();

        Size previewSize = new Size(mDesiredPreviewWidth, mDesiredPreviewHeight);
        mCameraSettings.setPreviewSize(previewSize.toPortabilitySize());
        // This is required for Samsung SGH-I337 and probably other Samsung S4
        // versions
        if (Build.BRAND.toLowerCase().contains("samsung")) {
            mCameraSettings.setSetting("video-size", mProfile.videoFrameWidth
                    + "x" + mProfile.videoFrameHeight);
        }
        int[] fpsRange = CameraUtil.getMaxPreviewFpsRange(mCameraCapabilities
                .getSupportedPreviewFpsRange());
        if (fpsRange.length > 0) {
            mCameraSettings.setPreviewFpsRange(fpsRange[0], fpsRange[1]);
        } else {
            mCameraSettings.setPreviewFrameRate(mProfile.videoFrameRate);
        }

        if (mActivity.getCameraAppUI().getFilmstripVisibility() != View.VISIBLE) {
            enableTorchMode(Keys.isCameraBackFacing(mDataModule));
        }

        // Set zoom.
        if (mCameraCapabilities.supports(CameraCapabilities.Feature.ZOOM)) {
            mCameraSettings.setZoomRatio(mZoomValue);
        }
        updateFocusParameters();

        /* SPRD Bug: 495676 update antibanding */
        updateParametersAntibanding();

        /* SPRD: Fix bug 535139, update color effect */
        updateParametersColorEffect();

        /* SPRD: Fix bug 535139, update white balance */
        updateParametersWhiteBalance();

        mCameraSettings.setRecordingHintEnabled(true);

        if (mCameraCapabilities
                .supports(CameraCapabilities.Feature.VIDEO_STABILIZATION)) {
            mCameraSettings.setVideoStabilization(true);
        }

        // Set picture size.
        // The logic here is different from the logic in still-mode camera.
        // There we determine the preview size based on the picture size, but
        // here we determine the picture size based on the preview size.
        List<Size> supported = Size.convert(mCameraCapabilities
                .getSupportedPhotoSizes());
        Size optimalSize = CameraUtil.getOptimalVideoSnapshotPictureSize(
                supported, mDesiredPreviewWidth, mDesiredPreviewHeight);
        Size original = new Size(mCameraSettings.getCurrentPhotoSize());
        if (!original.equals(optimalSize)) {
            mCameraSettings.setPhotoSize(optimalSize.toPortabilitySize());
            /* SPRD:fix bug 618724 need set thumbnail size for video snap shot @{ */
            mCameraSettings.setExifThumbnailSize(CameraUtil.getAdaptedThumbnailSize(optimalSize,
                    mAppController.getCameraProvider()).toPortabilitySize());
            /* @} */
        }

        Log.d(TAG, "Video snapshot size is " + optimalSize);

        // Set JPEG quality.
        int jpegQuality = CameraProfile.getJpegEncodingQualityParameter(
                mCameraId, CameraProfile.QUALITY_HIGH);
        mCameraSettings.setPhotoJpegCompressionQuality(jpegQuality);

        updateTimeLapse();// SPRD: for bug 509708 add time lapse
        updateTimeStamp();

        // SPRD : Fature : OIS && EIS
        updateParametersEOIS();

        // SPRD Bug:474696 Feature:Slow-Motion.
        setSlowmotionParameters();

        // SPRD： update makeup
        updateMakeLevel();

        if (mCameraDevice != null) {
            mCameraDevice.applySettings(mCameraSettings);
            // Nexus 5 through KitKat 4.4.2 requires a second call to
            // .setParameters() for frame rate settings to take effect.
            mCameraDevice.applySettings(mCameraSettings);
        }

        // SPRD Bug:474665 Feature Bug:Video Shutter Sound.
        updateCameraShutterSound();

        //nj dream camera test 70
        updateParametersGridLine();
        // Update UI based on the new parameters.
        mUI.updateOnScreenIndicators(mCameraSettings);

    }

    /* SPRD: New Feature EIS&OIS @{ */
    private void updateParametersEOIS() {
        // SPRD: add for NullPointerException when call isCameraFrontFacing at activity destroyed
        if (isCameraOpening() || mActivity.isDestroyed()) {
            return;
        }
        Log.d(TAG, "updateParametersEOIS video eois = " + CameraUtil.isEOISDvFrontEnabled() + isCameraFrontFacing());
        if (isCameraFrontFacing() && CameraUtil.isEOISDvFrontEnabled()) {
            if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_EOIS_DV_FRONT)) {
                return;
            }
            Log.d(TAG, "front video eois = " + mDataModuleCurrent.getBoolean(Keys.KEY_EOIS_DV_FRONT));
            mCameraSettings
                    .setEOISEnable(mDataModuleCurrent.getBoolean(Keys.KEY_EOIS_DV_FRONT));
            return;
        }

        if (!isCameraFrontFacing() && CameraUtil.isEOISDvBackEnabled()) {
            if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_EOIS_DV_BACK)) {
                return;
            }
            Log.d(TAG, "back video eois = " + mDataModuleCurrent.getBoolean(Keys.KEY_EOIS_DV_BACK));
            mCameraSettings
                    .setEOISEnable(mDataModuleCurrent.getBoolean(Keys.KEY_EOIS_DV_BACK));
            return;
        }
    }

    //nj dream camera test 70, 75
    private void updateParametersGridLine() {
        if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_GRID_LINES)) {
            return;
        }
        String grid = mDataModuleCurrent.getString(Keys.KEY_CAMERA_GRID_LINES);
        mAppController.getCameraAppUI().updateScreenGridLines(grid);
        Log.d(TAG, "updateParametersGridLine = " + grid);
    }

    //nj dream camera test 74
    private boolean updateParametersMircophone() {
        if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_MICROPHONE)) {
            return false;
        } else {
            return mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_MICROPHONE);
        }
    }

    private void updateFocusParameters() {
        // Set continuous autofocus. During recording, we use "continuous-video"
        // auto focus mode to ensure smooth focusing. Whereas during preview
        // (i.e.
        // before recording starts) we use "continuous-picture" auto focus mode
        // for faster but slightly jittery focusing.
        Set<CameraCapabilities.FocusMode> supportedFocus = mCameraCapabilities
                .getSupportedFocusModes();
        if (mMediaRecorderRecording) {
            if (mCameraCapabilities
                    .supports(CameraCapabilities.FocusMode.CONTINUOUS_VIDEO)) {
                mCameraSettings
                        .setFocusMode(CameraCapabilities.FocusMode.CONTINUOUS_VIDEO);
                mFocusManager
                        .overrideFocusMode(CameraCapabilities.FocusMode.CONTINUOUS_VIDEO);
            } else {
                mFocusManager.overrideFocusMode(null);
            }
        } else {
            // FIXME(b/16984793): This is broken. For some reasons,
            // CONTINUOUS_PICTURE is not on
            // when preview starts.
            mFocusManager.overrideFocusMode(null);
            if (mCameraCapabilities
                    .supports(CameraCapabilities.FocusMode.CONTINUOUS_PICTURE)) {
                mCameraSettings.setFocusMode(mFocusManager
                        .getFocusMode(mCameraSettings.getCurrentFocusMode(), mDataModule.getString(Keys.KEY_FOCUS_MODE)));
                if (mFocusAreaSupported) {
                    mCameraSettings
                            .setFocusAreas(mFocusManager.getFocusAreas());
                }
            }
        }
        updateAutoFocusMoveCallback();
    }

    @Override
    public void resume() {
        Log.i(TAG, "resume");
        if (isVideoCaptureIntent()) {
            mDontResetIntentUiOnResume = mPaused;
        }

        mPaused = false;
        mDataModuleCurrent.addListener(this);
        installIntentFilter();
        mAppController.setShutterEnabled(false);
        mZoomValue = 1.0f;

        OrientationManager orientationManager = mAppController
                .getOrientationManager();
        orientationManager.addOnOrientationChangeListener(this);
        mUI.onOrientationChanged(orientationManager,
                orientationManager.getDeviceOrientation());

        showVideoSnapshotUI(false);

        if (!mPreviewing) {
            // Bug #533869 new feature: check UI 27,28: dream camera of intent capture
            if (!mIsVideoCaptureIntent && mDataModule.getBoolean(Keys.KEY_INTENT_CAMERA_SWITCH) ||
                    mIsVideoCaptureIntent && mDataModule.getBoolean(Keys.KEY_CAMERA_SWITCH)) {
                requestCamera(mDataModule.getInt(Keys.KEY_CAMERA_ID));
            } else {
                requestCamera(mCameraId);
            }
        } else {
            // preview already started
            mAppController.setShutterEnabled(true);
        }

        if (mFocusManager != null) {
            // If camera is not open when resume is called, focus manager will
            // not
            // be initialized yet, in which case it will start listening to
            // preview area size change later in the initialization.
            mAppController.addPreviewAreaSizeChangedListener(mFocusManager);
        }

        if (mPreviewing) {
            mOnResumeTime = SystemClock.uptimeMillis();
            mHandler.sendEmptyMessageDelayed(MSG_CHECK_DISPLAY_ROTATION, 100);
        }
        getServices().getMemoryManager().addListener(this);
        /*
         * SPRD:Add for bug 496130 when recording video ,snap photo the
         * optionbar shows @{
         */
        getServices().getMediaSaver().setListener(
                new MediaSaverImpl.Listener() {
                    public void onHideBurstScreenHint() {
                        if (isRecording()) {
                            // SPRD: Fix bug 541750 The photo module can't change mode after take a picture.
                            showVideoSnapshotUI(false);
                            mAppController.getCameraAppUI().hideModeOptions();
                        } else {
                            mAppController.getCameraAppUI().showModeOptions();
                        }
                    }

                    public int getContinuousCaptureCount() {
                        return 0;
                    }
                });
        /* @} */

        //PAY ATTENTION: NEED NEXT DEBUG
        // SPRD BUG 532096: NO timestamp effect when take picture under videomodule
        /*CameraUtil.mTimeStamp = Keys.isTimeStampOn(mDataModuleCurrent));*/
    }

    @Override
    public void pause() {
        Log.i(TAG, "pause");
        /* SPRD: fix bug Bug 577352 Slow recording process, repeatedly tap the screen,the
         * phone interface will now show time "slow record does not support the camera @{ */
        ToastUtil.cancelToast();
        /* @} */
        mPaused = true;
        restoreToDefaultSettings();
        mDataModuleCurrent.removeListener(this);
        mDataModule.removeListener(this);
        mActivity.getCameraAppUI().dismissDialog(Keys.KEY_CAMERA_STORAGE_PATH);
        mUI.hideZoomProcessorIfNeeded();//SPRD:fix bug626587

        mAppController.getOrientationManager().removeOnOrientationChangeListener(this);
        /* SPRD: Fix bug 580448 slow should be closed beyond video @{ */
        if (mCameraSettings != null && mCameraDevice != null) {
            mCameraSettings.setVideoSlowMotion(Keys.SLOWMOTION_DEFAULT_VALUE);
            mCameraDevice.applySettings(mCameraSettings);
        }
        /* @} */
        if (mFocusManager != null) {
            // If camera is not open when resume is called, focus manager will
            // not
            // be initialized yet, in which case it will start listening to
            // preview area size change later in the initialization.
            mAppController.removePreviewAreaSizeChangedListener(mFocusManager);
            mFocusManager.removeMessages();
        }

        removeAutoFocusMoveCallback();

        if (mMediaRecorderRecording) {
            // Camera will be released in onStopVideoRecording.
            onStopVideoRecording();
        } else {
            // It may has changed ui in main thread but not started recording when onStorageUpdateDone.
            if (mUI != null && mActivity != null && mActivity.getOrientationManager() != null
                    && mActivity.getOrientationManager().getDisplayRotation() != null) {
                mUI.showRecordingUI(false, mActivity.getOrientationManager()
                        .getDisplayRotation().getDegrees());
            }
            /* SPRD:Fix bug 443439 @{ */
            if (mFocusManager != null) {
                if (mCameraDevice != null && mFocusManager.isInStateFocusing()) {
                    mCameraDevice.cancelAutoFocus();
                }
            }
            /* @} */
            stopPreview();
            closeCamera();
            releaseMediaRecorder();
        }

        closeVideoFileDescriptor();

        if (mReceiver != null) {
            // SPRD:fix bug599645 VideoModule recivedBroadcast later than CameraActivity, cause be killed
            // mActivity.unregisterReceiver(mReceiver);
            mActivity.unRegisterMediaBroadcastReceiver();
            mReceiver = null;
        }

        mHandler.removeMessages(MSG_CHECK_DISPLAY_ROTATION);
        mHandler.removeMessages(MSG_SWITCH_CAMERA);
        mHandler.removeMessages(MSG_SWITCH_CAMERA_START_ANIMATION);
        mHandler.removeMessages(MSG_ENABLE_SHUTTER_PAUSE_BUTTON);
        mPendingSwitchCameraId = -1;
        mSwitchingCamera = false;
        mPreferenceRead = false;
        getServices().getMemoryManager().removeListener(this);
        mUI.onPause();
    }

    public void restoreToDefaultSettings(){
        // UI part reset
        updateParametersGridLineToDefault();

        // camera device setting part reset
        updateParametersColorEffectToDefault();
        updateParametersWhiteBalanceToDefault();

        if (mCameraDevice != null) {
            mCameraDevice.applySettings(mCameraSettings);
        }
    }

    private void updateParametersGridLineToDefault() {
        if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_GRID_LINES)) {
            return;
        }
        String grid = mDataModuleCurrent.getStringDefault(Keys.KEY_CAMERA_GRID_LINES);
        mAppController.getCameraAppUI().updateScreenGridLines(grid);
        Log.d(TAG, "updateParametersGridLine = " + grid);
    }

    private void updateParametersWhiteBalanceToDefault() {
        if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_VIDEO_WHITE_BALANCE)) {
            return;
        }
        if (isCameraOpening()) {
            return;
        }
        String wb = mDataModuleCurrent.getStringDefault(Keys.KEY_VIDEO_WHITE_BALANCE);
        if (wb == null) {
            return;
        }
        CameraCapabilities.WhiteBalance whiteBalance = mCameraCapabilities.getStringifier()
                .whiteBalanceFromString(wb);
        if (mCameraCapabilities.supports(whiteBalance)) {
            mCameraSettings.setWhiteBalance(whiteBalance);
        }
    }

    private void updateParametersColorEffectToDefault() {
        if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_VIDEO_COLOR_EFFECT)) {
            return;
        }
        if (isCameraOpening()) {
            return;
        }
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities.getStringifier();
        String sColorEffect = mDataModuleCurrent.getStringDefault(Keys.KEY_VIDEO_COLOR_EFFECT);
        if (sColorEffect == null) {
            return;
        }
        Log.d(TAG, "update ColorEffect = " + sColorEffect);
        CameraCapabilities.ColorEffect colorEffect = stringifier
                .colorEffectFromString(sColorEffect);
        mCameraSettings.setColorEffect(colorEffect);
    }

    @Override
    public void destroy() {

    }

    @Override
    public void onLayoutOrientationChanged(boolean isLandscape) {
        setDisplayOrientation();
    }

    // TODO: integrate this into the SettingsManager listeners.
    public void onSharedPreferenceChanged() {

    }

    protected void switchCamera() {
        if (mPaused) {
            return;
        }
        /* SPRD:Fix bug 453868 @{ */
        int id = mCameraDevice.getCameraId();
        Log.i(TAG, "Start to switch camera. id=" + id
                + " mPendingSwitchCameraId=" + mPendingSwitchCameraId);
        if (id == mPendingSwitchCameraId) {
            return;
        }
        /* @} */
        SettingsManager settingsManager = mActivity.getSettingsManager();

        Log.d(TAG, "Start to switch camera.");
        mCameraId = mPendingSwitchCameraId;
        mPendingSwitchCameraId = -1;
        mDataModule.set(Keys.KEY_CAMERA_ID, mCameraId);

        if (mFocusManager != null) {
            mFocusManager.removeMessages();
        }

        /*
         * Sprd: Add for bug 529369 stop Video recording before switch camera
         *
         * @{
         */

        boolean stop = mMediaRecorderRecording;

        if (stop) {
            // CameraAppUI mishandles mode option enable/disable
            // for video, override that
            mAppController.getCameraAppUI().enableModeOptions();
            onStopVideoRecording();
        }
        /* @} */

        closeCamera();
        requestCamera(mCameraId);
        mMirror = isCameraFrontFacing();
        if (mFocusManager != null) {
            mFocusManager.setMirror(mMirror);
        }

        // From onResume
        mZoomValue = 1.0f;
        mUI.setOrientationIndicator(0, false);

        // Start switch camera animation. Post a message because
        // onFrameAvailable from the old camera may already exist.
        mHandler.sendEmptyMessage(MSG_SWITCH_CAMERA_START_ANIMATION);
        mUI.updateOnScreenIndicators(mCameraSettings);
    }

    private void initializeVideoSnapshot() {
        if (mCameraSettings == null) {
            return;
        }
    }

    void showVideoSnapshotUI(boolean enabled) {
        if (mCameraSettings == null) {
            return;
        }
        if (mCameraCapabilities
                .supports(CameraCapabilities.Feature.VIDEO_SNAPSHOT)
                && !mIsVideoCaptureIntent) {
            if (enabled) {
                mAppController.startFlashAnimation(false);
            } else {
                mUI.showPreviewBorder(enabled);
            }
            mAppController.setShutterEnabled(!enabled);
        }
    }

    /**
     * Used to update the flash mode. Video mode can turn on the flash as torch
     * mode, which we would like to turn on and off when we switching in and out
     * to the preview.
     *
     * @param enable Whether torch mode can be enabled.
     */
    private void enableTorchMode(boolean enable) {
        if (isCameraOpening()) {
            return;
        }
        if (mCameraSettings.getCurrentFlashMode() == null) {
            return;
        }
        //SPRD: Fix bug 631061 flash may be in error state when receive lowbattery broadcast
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_VIDEOCAMERA_FLASH_MODE)){
            return;
        }
        if (!mAppController.getButtonManager().isEnabled(ButtonManagerDream.BUTTON_VIDEO_FLASH_DREAM)) {
            return;
        }
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                .getStringifier();
        CameraCapabilities.FlashMode flashMode;
        if (enable) {
            flashMode = stringifier.flashModeFromString(mDataModuleCurrent
                    .getString(Keys.KEY_VIDEOCAMERA_FLASH_MODE));
        } else {
            flashMode = CameraCapabilities.FlashMode.OFF;
        }
        Log.d(TAG, "AA enableTorchMode " + flashMode);
        if (mCameraCapabilities.supports(flashMode)) {
            mCameraSettings.setFlashMode(flashMode);
        }
        /*
         * TODO: Find out how to deal with the following code piece: else {
         * flashMode = mCameraSettings.getCurrentFlashMode(); if (flashMode ==
         * null) { flashMode = mActivity.getString(
         * R.string.pref_camera_flashmode_no_flash);
         * mParameters.setFlashMode(flashMode); } }
         */
        if (mCameraDevice != null) {
            mCameraDevice.applySettings(mCameraSettings);
            mCameraSettings = mCameraDevice.getSettings();
        }
        mUI.updateOnScreenIndicators(mCameraSettings);
    }

    @Override
    public void onPreviewVisibilityChanged(int visibility) {
        if (mPreviewing) {
            /**
             * SPRD BUG 507795: won't open the flash when enlarge the picture in
             * picture preview UI @{ Original Code enableTorchMode(visibility ==
             * ModuleController.VISIBILITY_VISIBLE );
             */

            // SPRD:fix bug519856 return DV, torch will flash first,then turn on
            // enableTorchMode(visibility == ModuleController.VISIBILITY_VISIBLE
            // &&
            // mActivity.getCameraAppUI().getFilmstripVisibility() !=
            // View.VISIBLE);
            enableTorchMode(visibility != ModuleController.VISIBILITY_HIDDEN
                    && mActivity.getCameraAppUI().getFilmstripVisibility() != View.VISIBLE);
            /* @} */
        }
    }

    private void storeImage(final byte[] data, Location loc) {
        long dateTaken = System.currentTimeMillis();
        String title = CameraUtil.instance().createJpegName(dateTaken);
        ExifInterface exif = Exif.getExif(data);
        int orientation = Exif.getOrientation(exif);

        String flashSetting = mDataModuleCurrent.getString(
                Keys.KEY_VIDEOCAMERA_FLASH_MODE);
        //PAY ATTENTION: NEED NEXT DEBUG
        Boolean gridLinesOn = false;
        /*Boolean gridLinesOn = Keys.areGridLinesOn(mDataModuleCurrent);*/
        UsageStatistics.instance().photoCaptureDoneEvent(
                eventprotos.NavigationChange.Mode.VIDEO_STILL, title + ".jpeg",
                exif, isCameraFrontFacing(), false, currentZoomValue(),
                flashSetting, gridLinesOn, null, null, null, null, null, null,
                null);

        // Make sure next single tap we get the right memory.
        getServices().getMediaSaver().addImage(data, title, dateTaken, loc,
                orientation, exif, mOnPhotoSavedListener);
        mActivity.updateStorageSpaceAndHint(null);
        Log.i(TAG, "storeImage end!");
    }

    private String convertOutputFormatToMimeType(int outputFileFormat) {
        if (outputFileFormat == MediaRecorder.OutputFormat.MPEG_4) {
            return "video/mp4";
        }
        return "video/3gpp";
    }

    private String convertOutputFormatToFileExt(int outputFileFormat) {
        if (outputFileFormat == MediaRecorder.OutputFormat.MPEG_4) {
            return ".mp4";
        }
        return ".3gp";
    }

    private void closeVideoFileDescriptor() {
        if (mVideoFileDescriptor != null) {
            try {
                // SPRD: fix bug571335 camera anr
                long start = System.currentTimeMillis();
                mVideoFileDescriptor.close();
                /* SPRD: fix bug571335 camera anr @{ */
                long cost = System.currentTimeMillis() - start;
                if (cost > 100) {
                    Log.i(TAG, "closeVideoFileDescriptor cost: " + cost);
                }
                /* @} */
            } catch (IOException e) {
                Log.e(TAG, "Fail to close fd", e);
            }
            mVideoFileDescriptor = null;
        }
    }

    @Override
    public void onPreviewUIReady() {
        startPreview();
    }

    @Override
    public void onPreviewUIDestroyed() {
        stopPreview();
    }

    private void requestCamera(int id) {
        mActivity.getCameraProvider().requestCamera(id);
    }

    /* nj dream camera test 78, 128 @{ */
    protected void requestCameraOpen() {
        if (mCameraId != mDataModule.getInt(Keys.KEY_CAMERA_ID)) {
            mCameraId = mDataModule.getInt(Keys.KEY_CAMERA_ID);
        }
        Log.i(TAG, "requestCameraOpen mCameraId:" + mCameraId);
        requestCamera(mCameraId);
    }

    /* @} */
    @Override
    public void onMemoryStateChanged(int state) {
        mAppController.setShutterEnabled(state == MemoryManager.STATE_OK);
    }

    @Override
    public void onLowMemory() {
        // Not much we can do in the video module.
    }

    /*********************** FocusOverlayManager Listener ****************************/
    @Override
    public void autoFocus() {
        if (mCameraDevice != null) {
            mCameraDevice.autoFocus(mHandler, mAutoFocusCallback);
        }
    }

    @Override
    public void cancelAutoFocus() {
        if (mCameraDevice != null) {
            mCameraDevice.cancelAutoFocus();
            setFocusParameters();
        }
    }

    @Override
    public boolean capture() {
        return false;
    }

    @Override
    public void startFaceDetection() {

    }

    @Override
    public void stopFaceDetection() {

    }

    @Override
    public void setFocusParameters() {
        if (mCameraDevice != null) {
            updateFocusParameters();
            mCameraDevice.applySettings(mCameraSettings);
            mCameraSettings = mCameraDevice.getSettings();
        }
    }

    public void onPauseClicked() {
        Log.i(TAG, "onPauseClicked");
        long timePasedAfterRecording = SystemClock.uptimeMillis()
                - mRecordingStartTime;
        Log.d(TAG, "time passed after recording started: "
                + timePasedAfterRecording);
        if (mMediaRecorderRecording) {
            Log.d(TAG, String.format("mMediaRecorder execute %s",
                    (mMediaRecorderRecording ? "resume" : "pause")));
            // current is pause state
            if (mPauseRecorderRecording) {

                // Dependence:SPRD MediaRecorder.
                mMediaRecorder.resume();

                calculatePauseDuration(mPauseRecorderRecording);
                updateRecordingTime();
            }
            // current is recording state
            else {
                mHandler.removeMessages(MSG_UPDATE_RECORD_TIME);

                // Dependence:SPRD MediaRecorder.
                mMediaRecorder.pause();

                calculatePauseDuration(mPauseRecorderRecording);
            }
            // reverse pause state
            mPauseRecorderRecording = !mPauseRecorderRecording;
        }
        Log.d(TAG, "onPauseClicked mMediaRecorderRecording:"
                + mMediaRecorderRecording + ",mPauseRecorderRecording:"
                + mPauseRecorderRecording);
        mUI.onPauseClicked(mPauseRecorderRecording);
    }

    private void calculatePauseDuration(boolean isPause) {
        if (isPause) {
            mResumeTime = SystemClock.uptimeMillis();
            mResultTime += (mResumeTime - mPauseTime);
            mAllPauseTime += (mResumeTime - mPauseTime);
        } else {
            mPauseTime = SystemClock.uptimeMillis();
        }
    }

    /*
     * SPRD Bug:474701 Feature:Video Encoding Type. @{
     */
    public int getVideoEncodeType() {
        String str_encode_type = mDataModuleCurrent
                .getString(Keys.KEY_VIDEO_ENCODE_TYPE);
        int encodeType = MediaRecorder.VideoEncoder.H264;
        if (Keys.VAL_VIDEO_ENCODE_TYPE_H264.equals(str_encode_type)) {
            encodeType = MediaRecorder.VideoEncoder.H264;
        } else if (Keys.VAL_VIDEO_ENCODE_TYPE_MPEG.equals(str_encode_type)) {
            encodeType = MediaRecorder.VideoEncoder.MPEG_4_SP;
        }else if (Keys.VAL_VIDEO_ENCODE_TYPE_H265.equals(str_encode_type)) {
            encodeType = CameraUtil.H265;
        }
        Log.d(TAG, "setVideoEncodeType is " + encodeType);
        return encodeType;
    }

    /* SPRD: fix bug 474665 add shutter sound switch @{ */
    private void updateCameraShutterSound() {
        if (mCameraDevice != null) {
            mCameraDevice.enableShutterSound(mAppController.isPlaySoundEnable());
        }
    }

    // SPRD Bug:495676 update antibanding for DV
    private void updateParametersAntibanding() {
        if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_VIDEO_ANTIBANDING)) {
            return;
        }
        if (isCameraOpening()) {
            return;
        }

        CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                .getStringifier();

        String mAntibanding = mDataModuleCurrent
                .getString(Keys.KEY_VIDEO_ANTIBANDING);
        if (mAntibanding == null) {
            return;
        }
        mCameraSettings.setAntibanding(stringifier
                .antibandingModeFromString(mAntibanding));

    }
    /* @} */

    /* SPRD:fix bug492439 Ture on FM, Camera can not record @{ */
    private void abandonAudioPlayback() {
        mAudioManager.abandonAudioFocus(null);
    }

    /* @} */

    /*
     * SPRD Bug:509708 Feature:Time Lapse. @{
     */
    private void updateTimeLapse() {
        if (isCameraOpening()) {
            return;
        }
        if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_VIDEO_TIME_LAPSE_FRAME_INTERVAL)) {
            return;
        }
        String frameIntervalStr = mDataModuleCurrent
                .getString(Keys.KEY_VIDEO_TIME_LAPSE_FRAME_INTERVAL);
        Log.d(TAG, "updateTimeLapse frameIntervalStr=" + frameIntervalStr);
        if (frameIntervalStr == null) {
            return;
        }
        mTimeBetweenTimeLapseFrameCaptureMs = Integer
                .parseInt(frameIntervalStr);
        mCaptureTimeLapse = (mTimeBetweenTimeLapseFrameCaptureMs != 0);
    }

    /* @} */

    private void updateTimeStamp() {
        if (isCameraOpening()) {
            return;
        }
        if( mTempModule == null) {
            Log.e(TAG,"get tempModule failed");
            CameraUtil.mTimeStamp = false;
            return;
        }

        CameraUtil.mTimeStamp = Integer.parseInt(mTempModule.getString(
                DataConfig.SettingStoragePosition.POSITION_CATEGORY_BF,
                Keys.KEY_CAMERA_TIME_STAMP, "0")) != 0;
    }
    /*
     * SPRD Bug:474696 Feature:Slow-Motion. @{
     */
    private void setSlowmotionParameters() {
        if (isCameraOpening()) {
            return;
        }

        String slow_motion = mDataModuleCurrent
                .getString(Keys.KEY_VIDEO_SLOW_MOTION);
        if (mIsVideoCaptureIntent) {
            mCameraSettings.setVideoSlowMotion(Keys.SLOWMOTION_DEFAULT_VALUE);
        } else {
            mCameraSettings.setVideoSlowMotion(slow_motion);
        }
    }

    public int getRecorderAudioSoruceBySlowMotion(boolean isVideoCaptureIntent) {
        int result = MediaRecorder.AudioSource.CAMCORDER;
        if (mDataModuleCurrent != null) {
            String sSlowMotion = mDataModuleCurrent.getString(
                    Keys.KEY_VIDEO_SLOW_MOTION, Keys.SLOWMOTION_DEFAULT_VALUE);
            try {
                if ((Integer.parseInt(sSlowMotion) > 1 && !isVideoCaptureIntent)
                        || mCaptureTimeLapse || !updateParametersMircophone()) {
                    // Dependence:SPRD MediaRecorder.
                    // @Reference media/audio/include/system/audio.h AUDIO_SOURCE_RECORD_NO_AUDIO = 9
                    /* @}
                     * SPRD: Fix bug 607491 android N, change the slowmotion value to 1997
                     * #include "ARTPWriter.h
                     * -#define  AUDIO_SOURCE_RECORD_NO_AUDIO  9
                     * +#define  AUDIO_SOURCE_RECORD_NO_AUDIO  1997
                     @{ */
                    result = 1997;//MediaRecorder.AudioSource.RECORD_NO_AUDIO;
                }
            } catch (NumberFormatException e) {
                Log.e(TAG, "sSlowMotion is invalid.", e);
            }
        }
        return result;
    }

    /* @} */
    /* SPRD: Fix bug 535139, Add for video color effect @{ */
    private void updateParametersColorEffect() {
        if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_VIDEO_COLOR_EFFECT)) {
            return;
        }
        if (isCameraOpening()) {
            return;
        }
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities.getStringifier();
        String sColorEffect = mDataModuleCurrent.getString(Keys.KEY_VIDEO_COLOR_EFFECT);
        if (sColorEffect == null) {
            return;
        }
        Log.d(TAG, "update ColorEffect = " + sColorEffect);
        CameraCapabilities.ColorEffect colorEffect = stringifier
                .colorEffectFromString(sColorEffect);
        mCameraSettings.setColorEffect(colorEffect);
    }
    /* @} */

    /* SPRD: Fix bug 535139, Add for add whitebalance @{ */
    private void updateParametersWhiteBalance() {
        if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_VIDEO_WHITE_BALANCE)) {
            return;
        }
        if (isCameraOpening()) {
            return;
        }
        String wb = mDataModuleCurrent.getString(Keys.KEY_VIDEO_WHITE_BALANCE);
        if (wb == null) {
            return;
        }
        CameraCapabilities.WhiteBalance whiteBalance = mCameraCapabilities.getStringifier()
                .whiteBalanceFromString(wb);
        if (mCameraCapabilities.supports(whiteBalance)) {
            mCameraSettings.setWhiteBalance(whiteBalance);
        }
    }
    /* @} */

    /* SPRD: Add for bug 559531 @{ */
    private void enableShutterAndPauseButton(boolean enable) {
        if (mUI != null) {
            mUI.enablePauseButton(enable);
        }
        if (mAppController != null) {
            mAppController.setShutterEnabled(enable);
        }
    }

    /**
     * updateBatteryLevel
     * @param level the battery level when level changed
     */
    @Override
    public void updateBatteryLevel(int level) {
        String BEFORE_LOW_BATTERY = "_before_low_battery";
        String valueEffectScope = mAppController.getCameraScope();
        // the limen value which set up in system properties
        int batteryLevel = CameraUtil.getLowBatteryNoFlashLevel();
        String beforeMode = mDataModuleCurrent.getString(Keys.KEY_VIDEOCAMERA_FLASH_MODE + BEFORE_LOW_BATTERY);
        String currentMode = mDataModuleCurrent.getString(Keys.KEY_VIDEOCAMERA_FLASH_MODE);
        if (level <= batteryLevel && !isRecording()) {
            // step 1. save current value: on, off, torch
            if (TextUtils.isEmpty(beforeMode)) {
                mDataModuleCurrent.set(
                        Keys.KEY_VIDEOCAMERA_FLASH_MODE + BEFORE_LOW_BATTERY, currentMode);
            }
            // step 2. set flash mode off and write into sp
            /*SPRD: Fix bug 631061 flash may be in error state when receive lowbattery broadcast
            mDataModuleCurrent.set(Keys.KEY_VIDEOCAMERA_FLASH_MODE, "off");
            */
            // step 3. if flash is on, turn off the flash
            if (mCameraSettings != null && mCameraCapabilities != null) {
                enableTorchMode(false);
            }
            // step 4. set button disabled and show toast to users
            mAppController.getButtonManager().disableButton(ButtonManagerDream.BUTTON_VIDEO_FLASH_DREAM);
            Toast.makeText(mActivity, R.string.battery_level_low, Toast.LENGTH_LONG).show();
            mIsBatteryLow = true;
        } else if (level > batteryLevel && !isRecording()) {
            // never lower than limen
            if (TextUtils.isEmpty(beforeMode)) {
                return;
            }
            // step 1.set before state value to current value
            /*SPRD: Fix bug 631061 flash may be in error state when receive lowbattery broadcast
            mDataModuleCurrent.set(Keys.KEY_VIDEOCAMERA_FLASH_MODE, beforeMode);
            */
            // step 2.set before state value null
            mDataModuleCurrent.set(
                    Keys.KEY_VIDEOCAMERA_FLASH_MODE + BEFORE_LOW_BATTERY, null);
            // step 4.set button disabled or enabled
            mAppController.getButtonManager().enableButton(ButtonManagerDream.BUTTON_VIDEO_FLASH_DREAM);
            mIsBatteryLow = false;
            // step 3.according to before state value turn on flash
            //if (!"off".equals(beforeMode)) {
                // open video flash
                if (mCameraSettings != null && mCameraCapabilities != null) {
                    enableTorchMode(true);
                }
            //}
        }
    }

    public VideoUI createUI(CameraActivity activity) {
        return new VideoUI(activity, this, activity.getModuleLayoutRoot());
    }

    protected int getDeviceCameraId() {
        return mCameraDevice.getCameraId();
    }

    //dream test 11
    @Override
    public void updateParameter(String key) {
        switch (key) {
            case Keys.KEY_CAMERA_SHUTTER_SOUND:
                updateCameraShutterSound();
                break;
            default:
                break;
        }
    }

    //Bug#533869 add the feature of volume
    protected int getVolumeControlStatus(CameraActivity mActivity) {
        return mActivity.getVolumeControlStatus();
    }

    private boolean isCameraOpening() {
        return mCameraSettings == null || mCameraCapabilities == null;
    }

    /**
     * This Handler is used to post message back onto the main thread of the
     * application.
     */
    private class MainHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {

                case MSG_ENABLE_SHUTTER_BUTTON:
                    mAppController.setShutterEnabled(true);
                    break;

                case MSG_UPDATE_RECORD_TIME: {
                    updateRecordingTime();
                    break;
                }

                case MSG_CHECK_DISPLAY_ROTATION: {
                    // Restart the preview if display rotation has changed.
                    // Sometimes this happens when the device is held upside
                    // down and camera app is opened. Rotation animation will
                    // take some time and the rotation value we have got may be
                    // wrong. Framework does not have a callback for this now.
                    if ((CameraUtil.getDisplayRotation() != mDisplayRotation)
                            && !mMediaRecorderRecording && !mSwitchingCamera) {
                        startPreview();
                    }
                    if (SystemClock.uptimeMillis() - mOnResumeTime < 5000) {
                        mHandler.sendEmptyMessageDelayed(
                                MSG_CHECK_DISPLAY_ROTATION, 100);
                    }
                    break;
                }

                case MSG_SWITCH_CAMERA: {
                    switchCamera();
                    break;
                }

                case MSG_SWITCH_CAMERA_START_ANIMATION: {
                    // TODO:
                    // ((CameraScreenNail)
                    // mActivity.mCameraScreenNail).animateSwitchCamera();

                    // Enable all camera controls.
                    mSwitchingCamera = false;
                    break;
                }
                case MSG_ENABLE_SHUTTER_PAUSE_BUTTON: {
                    enableShutterAndPauseButton(true);
                    break;
                }
                default:
                    Log.v(TAG, "Unhandled message: " + msg.what);
                    break;
            }
        }
    }

    private class MyBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.v(TAG, "onReceive action = " + action);

            Uri uri = intent.getData();
            String path = uri.getPath();
            if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
                mActivity.getCameraAppUI().dismissDialog(Keys.KEY_CAMERA_STORAGE_PATH);
                String storagePath = DataModuleManager.getInstance(mActivity)
                        .getDataModuleCamera().getString(Keys.KEY_CAMERA_STORAGE_PATH);
                Log.d(TAG, "onReceive: storagePath = " + storagePath);
                String currentPath = null;
                if (MultiStorage.KEY_DEFAULT_INTERNAL.equals(storagePath)) {
                    currentPath = StorageUtilProxy.getInternalStoragePath().toString();
                } else if (MultiStorage.KEY_DEFAULT_EXTERNAL.equals(storagePath)) {
                    currentPath = StorageUtilProxy.getExternalStoragePath()
                            .toString();
                } else {
                    // SPRD: Fix bug 572473 add for usb storage support
                    currentPath = MultiStorage.getUsbStoragePath(storagePath);
                }
                Log.d(TAG, "onReceive: path = " + path + " currentPath = " + currentPath);
                if (path.equals(currentPath)) {
                    /* SPRD: fix bug548010 Camera occurs error when connect USB during video recording @{
                     * original code
                    stopVideoRecording();
                     */
                    if (mMediaRecorderRecording) {
                        stopVideoRecording(false);
                        /* @} */
                        //SPRD:fix bug538109 pull sdcard, recording stop in 4~5s
                        ToastUtil.showToast(mActivity, R.string.sdcard_remove, ToastUtil.LENGTH_LONG);
                    } else {
                        ToastUtil.showToast(mActivity, R.string.sdcard_changeto_phone, ToastUtil.LENGTH_LONG);
                    }
                } else {
                    ToastUtil.showToast(mActivity, R.string.externalstorage_removed, ToastUtil.LENGTH_LONG);
                }
            } else if (action.equals(Intent.ACTION_MEDIA_SCANNER_STARTED)) {
                ToastUtil.showToast(mActivity, R.string.wait, ToastUtil.LENGTH_LONG);
            }
            Log.d(TAG, "MyBroadcastReceiver onReceive: end");
        }
    }

    private final class JpegPictureCallback implements CameraPictureCallback {
        Location mLocation;

        public JpegPictureCallback(Location loc) {
            mLocation = loc;
        }

        @Override
        public void onPictureTaken(byte[] jpegData, CameraProxy camera) {
            Log.i(TAG, "Video snapshot taken.");
            mSnapshotInProgress = false;
            /*SPRD: Fix bug 541750 The photo module can't change mode after take a picture. @{
            showVideoSnapshotUI(false);
            @}*/
            storeImage(jpegData, mLocation);

            // SPRD: Fix bug 650297 that video thumbnail is showed slowly
            mSnapshotCount++;
        }
    }
    @Override
    public boolean isShutterClicked(){
        return mMediaRecorderRecording;
    }

    /* SPRD: fix bug 474672 add for ucam beauty 
     * @{ */

    public MakeupController mMakeUpController;

    @Override
    public void setMakeUpController(MakeupController makeUpController) {
        mMakeUpController = makeUpController;
    }

    @Override
    public void onBeautyValueChanged(int value) {
        Log.i(TAG, "onBeautyValueChanged setParameters Value = " + value);

        if (mCameraSettings != null) {
            mCameraSettings.setSkinWhitenLevel(value);
            if (mCameraDevice != null) {
                mCameraDevice.applySettings(mCameraSettings);
            }
        }
    }

    @Override
    public void onBeautyValueReset() {
        Log.i(TAG, "onBeautyValueReset");

        if (mCameraSettings != null) {
            mCameraSettings.setSkinWhitenLevel(0);
            if (mCameraDevice != null) {
                mCameraDevice.applySettings(mCameraSettings);
            }
        }
    }

    @Override
    public void updateMakeLevel() {
        Log.d(TAG,
                "initializeMakeupControllerView "
                        + DataModuleManager.getInstance(mActivity)
                                .getCurrentDataModule()
                                .getBoolean(Keys.KEY_VIDEO_BEAUTY_ENTERED));

        if (isMakeUpEnable() && mMakeUpController != null) {
            if (DataModuleManager.getInstance(mActivity).getCurrentDataModule()
                    .getBoolean(Keys.KEY_VIDEO_BEAUTY_ENTERED)) {
                mMakeUpController.resumeMakeupControllerView();
            } else {
                mMakeUpController.pauseMakeupControllerView();
            }
        }
    }

    @Override
    public boolean isMakeUpEnable(){
        return CameraUtil.isMakeupVideoEnable() && DataModuleManager.getInstance(mActivity).getCurrentDataModule()
                .isEnableSettingConfig(Keys.KEY_VIDEO_BEAUTY_ENTERED);
    }
    /* @{ */

    protected boolean mCameraAvailable = false;

    @Override
    public boolean isCameraAvailable(){
        return mCameraAvailable;
    }
}
