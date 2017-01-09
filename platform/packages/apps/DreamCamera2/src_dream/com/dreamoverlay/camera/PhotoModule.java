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

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.SurfaceTexture;
import android.location.Location;
import android.media.AudioManager;
import android.media.CameraProfile;
import android.media.MediaActionSound;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.MessageQueue;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Toast;

import com.android.camera.PhotoModule.NamedImages.NamedEntity;
import com.android.camera.PhotoUI;
import com.android.camera.app.AppController;
import com.android.camera.app.CameraAppUI;
import com.android.camera.app.CameraProvider;
import com.android.camera.app.MediaSaver;
import com.android.camera.app.MemoryManager;
import com.android.camera.app.MemoryManager.MemoryListener;
import com.android.camera.app.MotionManager;
import com.android.camera.app.OrientationManager;
import com.android.camera.debug.Log;
import com.android.camera.exif.ExifInterface;
import com.android.camera.exif.ExifTag;
import com.android.camera.exif.Rational;
import com.android.camera.hardware.HardwareSpec;
import com.android.camera.hardware.HardwareSpecImpl;
import com.android.camera.hardware.HeadingSensor;
import com.android.camera.module.ModuleController;
import com.android.camera.one.OneCamera;
import com.android.camera.one.OneCameraAccessException;
import com.android.camera.one.OneCameraException;
import com.android.camera.one.OneCameraManager;
import com.android.camera.one.OneCameraModule;
import com.android.camera.remote.RemoteCameraModule;
import com.android.camera.settings.CameraPictureSizesCacher;
import com.android.camera.settings.CameraSettingsActivity.CameraSettingsFragment;
import com.android.camera.settings.Keys;
import com.android.camera.settings.ResolutionUtil;
import com.android.camera.settings.SettingsManager;
import com.android.camera.stats.SessionStatsCollector;
import com.android.camera.stats.UsageStatistics;
import com.android.camera.ui.CountDownView;
import com.android.camera.ui.TouchCoordinate;
import com.android.camera.util.AndroidServices;
import com.android.camera.util.ApiHelper;
import com.android.camera.util.CameraUtil;
import com.android.camera.util.GcamHelper;
import com.android.camera.util.GservicesHelper;
import com.android.camera.util.Size;
import com.android.camera.util.ToastUtil;
import com.android.camera2.R;
import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraAgent.CameraAFCallback;
import com.android.ex.camera2.portability.CameraAgent.CameraAFMoveCallback;
import com.android.ex.camera2.portability.CameraAgent.CameraPictureCallback;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.android.ex.camera2.portability.CameraAgent.CameraShutterCallback;
import com.android.ex.camera2.portability.CameraAgent.CancelBurstCaptureCallback;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.ex.camera2.portability.CameraDeviceInfo.Characteristics;
import com.android.ex.camera2.portability.CameraSettings;
import com.dream.camera.ButtonManagerDream;
import com.dream.camera.DreamModule;
import com.dream.camera.MakeupController;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleBasic.DreamSettingChangeListener;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataModuleManager.ResetListener;
import com.dream.camera.settings.DataStructSetting;
import com.dream.camera.util.DreamUtil;
import com.dream.camera.vgesture.VGestureController;
import com.google.common.logging.eventprotos;
import com.sprd.camera.freeze.FreezeFrameDisplayControl;
import com.sprd.camera.storagepath.MultiStorage;
import com.sprd.camera.storagepath.StorageUtilProxy;
import com.sprd.camera.voice.PhotoVoiceMessage;
import com.sprd.hz.selfportrait.MSG;
import com.ucamera.ucam.modules.utils.UCamUtill;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Vector;

public class PhotoModule extends CameraModule implements PhotoController,
        ModuleController, MemoryListener, FocusOverlayManager.Listener,
        SettingsManager.OnSettingChangedListener, RemoteCameraModule,
        /**
         * SPRD: fix bug 388273 CountDownView.OnCountDownStatusListener {
         */
        CountDownView.OnCountDownStatusListener,
        FreezeFrameDisplayControl.Listener, MediaSaverImpl.Listener,
        View.OnTouchListener, CancelBurstCaptureCallback,
        DreamSettingChangeListener,
        OrientationManager.OnOrientationChangeListener,
        FocusOverlayManager.TouchListener{

    private static final Log.Tag TAG = new Log.Tag("PhotoModule");

    // We number the request code from 1000 to avoid collision with Gallery.
    private static final int REQUEST_CROP = 1000;

    // Messages defined for the UI thread handler.
    private static final int MSG_FIRST_TIME_INIT = 1;
    private static final int MSG_SET_CAMERA_PARAMETERS_WHEN_IDLE = 2;
    public static final int BUTST_START_CAPTURE = 3;// SPRD:fix bug 473462 add
                                                    // for burst capture

    // The subset of parameters we need to update in setCameraParameters().
    private static final int UPDATE_PARAM_INITIALIZE = 1;
    private static final int UPDATE_PARAM_ZOOM = 2;
    private static final int UPDATE_PARAM_PREFERENCE = 4;
    private static final int UPDATE_PARAM_ALL = -1;

    private static final String DEBUG_IMAGE_PREFIX = "DEBUG_";

    protected CameraActivity mActivity;
    protected CameraProxy mCameraDevice;
    protected int mCameraId;
    protected CameraCapabilities mCameraCapabilities;
    protected CameraSettings mCameraSettings;
    private HardwareSpec mHardwareSpec;
    protected boolean mPaused;
    private boolean mCancelBurst = false;
    private boolean mThumbnailHasInvalid = false;

    protected PhotoUI mUI;

    // The activity is going to switch to the specified camera id. This is
    // needed because texture copy is done in GL thread. -1 means camera is not
    // switching.
    protected int mPendingSwitchCameraId = -1;

    // When setCameraParametersWhenIdle() is called, we accumulate the subsets
    // needed to be updated in mUpdateSet.
    private int mUpdateSet;

    private float mZoomValue; // The current zoom ratio.
    private int mTimerDuration;
    /** Set when a volume button is clicked to take photo */
    private boolean mVolumeButtonClickedFlag = false;

    private boolean mCameraButtonClickedFlag = false;// SPRD: fix bug 473602 add
                                                     // for half-press

    private boolean mFocusAreaSupported;
    private boolean mMeteringAreaSupported;
    private boolean mAeLockSupported;
    private boolean mAwbLockSupported;
    private boolean mContinuousFocusSupported;

    private static final String sTempCropFilename = "crop-temp";

    public boolean mFaceDetectionStarted = false;

    // mCropValue and mSaveUri are used only if isImageCaptureIntent() is true.
    private String mCropValue;
    private Uri mSaveUri;

    private Uri mDebugUri;

    // Add for dream settings.
    protected DataModuleBasic mDataModule;
    protected DataModuleBasic mDataModuleCurrent;

    // We use a queue to generated names of the images to be used later
    // when the image is ready to be saved.
    private NamedImages mNamedImages;
    private boolean isHdrOn = true;

    /*SPRD: fix bug 599542 add for touch capture and count down @{*/
    private boolean isOnSingleTapUp = false;
    private int mTapUpX= 0;
    private int mTapUpY= 0;
    /**
     * SPRD: Fix bug 474851, Add for new feature VGesture @{
     */
    public VGestureController mVGestureController;
    /**
     * @}
     */

    private final Runnable mDoSnapRunnable = new Runnable() {
        @Override
        public void run() {
            onShutterButtonClick();
        }
    };

    /**
     * An unpublished intent flag requesting to return as soon as capturing is
     * completed. TODO: consider publishing by moving into MediaStore.
     */
    private static final String EXTRA_QUICK_CAPTURE = "android.intent.extra.quickCapture";

    // The display rotation in degrees. This is only valid when mCameraState is
    // not PREVIEW_STOPPED.
    private int mDisplayRotation;
    // The value for UI components like indicators.
    private int mDisplayOrientation;
    // The value for cameradevice.CameraSettings.setPhotoRotationDegrees.
    private int mJpegRotation;
    // Indicates whether we are using front camera
    private boolean mMirror;
    private boolean mFirstTimeInitialized;
    private boolean mIsImageCaptureIntent;

    protected int mCameraState = PREVIEW_STOPPED;
    private boolean mSnapshotOnIdle = false;

    private ContentResolver mContentResolver;

    protected AppController mAppController;
    private OneCameraManager mOneCameraManager;

    private final PostViewPictureCallback mPostViewPictureCallback = new PostViewPictureCallback();
    private final RawPictureCallback mRawPictureCallback = new RawPictureCallback();
    private final AutoFocusCallback mAutoFocusCallback = new AutoFocusCallback();
    private final Object mAutoFocusMoveCallback = ApiHelper.HAS_AUTO_FOCUS_MOVE_CALLBACK ? new AutoFocusMoveCallback()
            : null;

    private long mFocusStartTime;
    private long mShutterCallbackTime;
    private long mPostViewPictureCallbackTime;
    private long mRawPictureCallbackTime;
    private long mJpegPictureCallbackTime;
    private long mOnResumeTime;
    private byte[] mJpegImageData;
    /** Touch coordinate for shutter button press. */
    private TouchCoordinate mShutterTouchCoordinate;

    // These latency time are for the CameraLatency test.
    public long mAutoFocusTime;
    public long mShutterLag;
    public long mShutterToPictureDisplayedTime;
    public long mPictureDisplayedToJpegCallbackTime;
    public long mJpegCallbackFinishTime;
    public long mCaptureStartTime;

    // This handles everything about focus.
    private FocusOverlayManager mFocusManager;

    private final int mGcamModeIndex;
    private SoundPlayer mCountdownSoundPlayer;

    private CameraCapabilities.SceneMode mSceneMode;

    /* SPRD: fix bug 473462 add burst capture @{ */
    private CameraCapabilities.BurstNumber mButstNumber;
    private int mContinueTakePictureCount;
    private int mContinuousCaptureCount;
    private int mHasCaputureCount = 0;
    private int mTotalCaputureCount;
    private int mCaptureCount = 0;

    private boolean mShutterSoundEnabled = false;// SPRD:fix bug 474665
    public boolean mBurstMode = false;
    // SPRD: Fix bug 581173 when burst capture, the shutter sound should be the same
    private boolean mIsBurstStarted = false;
    private boolean mIsContinousCaptureFinish = false;
    private boolean mHasStartCapture = false;
    private static final int PICTURE_SIZE_13M = 4160 * 3120;
    /* @} */

    protected final Handler mHandler = new MainHandler(this);

    private boolean mQuickCapture;

    /** Used to detect motion. We use this to release focus lock early. */
    private MotionManager mMotionManager;
    private HeadingSensor mHeadingSensor;

    /** True if all the parameters needed to start preview is ready. */
    /**
     * SPRD: Fix bug 572631, optimize camera launch time
     * Original Code
     *
    private boolean mCameraPreviewParamsReady = false;
     */

    private CameraCapabilities.Antibanding mAntibanding;// SPRD:Add for
                                                        // antibanding
    protected String mFace;// SPRD:Add for ai detect
    private String mDetectValueBeforeSceneMode;// SPRD:Add for ai detect
    private CameraCapabilities.ColorEffect mColorEffect;// SPRD:Add for color
                                                        // effect Bug 474727

    protected boolean mBurstNotShowShutterButton = false;

    protected int mBurstCount;

    protected boolean mIsBatteryLow = false;

    private final MediaSaver.OnMediaSavedListener mOnMediaSavedListener = new MediaSaver.OnMediaSavedListener() {

        @Override
        public void onMediaSaved(Uri uri) {
            /* SPRD: Add for FreezeViewDisplay if MediaSave Finish */
            if ((sFreezeFrameControl != null && sFreezeFrameControl
                    .isFreezeFrame(isSetFreezeFrameDisplay())) || mIsImageCaptureIntent) {
                if (uri != null) {
                    sFreezeFrameControl.proxyRunLoadProxy(uri);
                } else {
                    sFreezeFrameControl.proxyDoneClicked();
                }
            } else if (uri != null) {

                /*
                 * Add For Dream Camera in IntervalPhotoModule
                 * 
                 * @{
                 */
                mediaSaved(uri);
                /* @} */

                mActivity.notifyNewMedia(uri);
            } else {
                /* SPRD: Fix bug 547952, wont show error dialog, not serious problem @{
                 * orginal code
                onError();
                */
                Log.w(TAG, "null uri got");
               /* @} */
            }
        }
    };

    /**
     * Displays error dialog and allows use to enter feedback. Does not shut
     * down the app.
     */
    private void onError() {
        mAppController.getFatalErrorHandler().onMediaStorageFailure();
    }

    private boolean mShouldResizeTo16x9 = false;

    /**
     * We keep the flash setting before entering scene modes (HDR) and restore
     * it after HDR is off. SPRD:MUTEX
     */
    private String PREF_BEFORE = "PREF_BEFORE";
    private String PREF_BEFORE_SWITCH = "PREF_BEFORE_SWITCH";
    private String mFlashModeBeforeSceneMode;
    private String mExposureCompensationBefore;
    private String mSceneModeBefore;
    private String mCountineCaptureBefore;
    private String mWhiteBalanceBefore;
    private String mColorEffectBefore;
    private String mISOBefore;
    private String mContrastBefore;
    private String mSaturationBefore;
    private String mBrightnessBefore;
    private boolean mAutoExposureLockBefore;
    private boolean isToastExit = false;

    private void checkDisplayRotation() {
        // Need to just be a no-op for the quick resume-pause scenario.
        if (mPaused) {
            return;
        }
        // Set the display orientation if display rotation has changed.
        // Sometimes this happens when the device is held upside
        // down and camera app is opened. Rotation animation will
        // take some time and the rotation value we have got may be
        // wrong. Framework does not have a callback for this now.
        if (CameraUtil.getDisplayRotation() != mDisplayRotation) {
            setDisplayOrientation();
        }
        if (SystemClock.uptimeMillis() - mOnResumeTime < 5000) {
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    checkDisplayRotation();
                }
            }, 100);
        }
    }

    /**
     * This Handler is used to post message back onto the main thread of the
     * application
     */
    /**
     * SPRD: fix bug 473462 private static class MainHandler extends Handler {
     */
    private class MainHandler extends Handler {
        private final WeakReference<PhotoModule> mModule;

        public MainHandler(PhotoModule module) {
            super(Looper.getMainLooper());
            mModule = new WeakReference<PhotoModule>(module);
        }

        @Override
        public void handleMessage(Message msg) {
            PhotoModule module = mModule.get();
            if (module == null) {
                return;
            }
            Log.d(TAG, "handleMsg " + msg.what);
            switch (msg.what) {
            case MSG_FIRST_TIME_INIT: {
                module.initializeFirstTime();
                break;
            }

            case MSG_SET_CAMERA_PARAMETERS_WHEN_IDLE: {
                module.setCameraParametersWhenIdle(0);
                break;
            }

            case BUTST_START_CAPTURE: {
               //SPRD:fix bug531893 volume is pressed in Burst mode
               if(mVolumeButtonClickedFlag){
                    break;
               }

               /* SPRD: fix bug 640448 beauty(makeup) module do not support batch shooting @{ */
               if(mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_BEAUTY_ENTERED) && mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_BEAUTY_ENTERED)){
                   return;
               }
              /* @} */
                Log.d(TAG, "mActivity =" + mActivity + "isBurstOn()="
                        + isBurstOn());
                if (mActivity != null && isBurstOn()) {
                    Log.d(TAG, "start continue capture");
                    mCancelBurst = false;
                    mBurstMode = true;
                    mIsContinousCaptureFinish = false;
                    mCaptureCount = 0;
                    /* SPRD: Fix bug 581173 when burst capture, the shutter sound should be the same @{ */
                    if(mAppController.isPlaySoundEnable()) {
                        AndroidServices.instance().provideAudioManager().requestAudioFocus(null,
                                AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
                    }
                    mIsBurstStarted = true;
                    /* @} */
                    onShutterButtonClick();
                    doSomethingWhenonBrustStart();
                }
                break;
            }

            /* SPRD: New feature vgesture detect @{ */
            case MSG.CAMERA_SHUTTER: {
                if (mAppController.getCameraAppUI().isSettingLayoutOpen()){
                    break;
                }
                if (mAppController.getCameraAppUI().getFilmstripVisibility() == View.VISIBLE){
                    break;
                }
                mAppController.getCameraAppUI().transitionToCancel();
                mAppController.getCameraAppUI().hideModeOptions();
                mAppController.getCameraAppUI().updatePreviewUI(View.GONE);
                mVGestureController.startTimer(msg.arg1);
                break;
            }

            case MSG.CAMERA_FOCUS_CAPTURE: {
                Log.i(TAG,"CAMERA_FOCUS_CAPTURE");
                module.onShutterButtonClick();
                if(getModuleTpye() == DreamModule.VGESTURE_MODULE){
                    cancelVgestureCountDown(true);
                }
                mCaptureCount = 0;
                break;
            }
            /* @} */

            /* SPRD: Fix bug 535110, Photo voice record. @{ */
            case PhotoVoiceMessage.MSG_RECORD_AUDIO: {
                module.startAudioRecord();
                break;
            }

            case PhotoVoiceMessage.MSG_RECORD_STOPPED: {
                  module.onAudioRecordStopped();
                  break;
            }
            /* @} */
        }
    }
    }

    /*SPRD:Fix bug 543925 @{*/
    private BroadcastReceiver mReceiver = null;

    private class MyBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.i(TAG,"onReceive action="+action);
            if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
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
                        currentPath = StorageUtilProxy.getExternalStoragePath().toString();
                    }
                    if (path.equals(currentPath)) {
                            Toast.makeText(
                                    mActivity,
                                    mActivity.getResources()
                                            .getString(R.string.sdcard_changeto_phone),
                                    Toast.LENGTH_LONG).show();
                    } else {
                        Toast.makeText(
                                mActivity,
                                mActivity.getResources()
                                        .getString(R.string.externalstorage_removed),
                                Toast.LENGTH_LONG).show();
                    }
                } else if (action.equals(Intent.ACTION_MEDIA_SCANNER_STARTED)) {
                    Toast.makeText(mActivity,
                            mActivity.getResources().getString(R.string.wait), Toast.LENGTH_LONG)
                            .show();
                }
            }
        }
    }
    private void installIntentFilter() {
        // install an intent filter to receive SD card related events.
        //IntentFilter intentFilter =
        //        new IntentFilter(Intent.ACTION_MEDIA_EJECT);
        //intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_STARTED);
        //intentFilter.addDataScheme("file");
        mReceiver = new MyBroadcastReceiver();
        //mActivity.registerReceiver(mReceiver, intentFilter);
        mActivity.registerMediaBroadcastReceiver(mReceiver);
    }
    /* @} */

    public boolean isBurstOn() {
        return "ninetynine".equals(mDataModuleCurrent.getString(Keys.KEY_CAMERA_CONTINUE_CAPTURE));
    }

    private void switchToGcamCapture() {
        if (mActivity != null && mGcamModeIndex != 0) {
            SettingsManager settingsManager = mActivity.getSettingsManager();
            settingsManager.set(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_HDR_PLUS, true);

            // Disable the HDR+ button to prevent callbacks from being
            // queued before the correct callback is attached to the button
            // in the new module. The new module will set the enabled/disabled
            // of this button when the module's preferred camera becomes
            // available.
            ButtonManager buttonManager = mActivity.getButtonManager();

            buttonManager.disableButtonClick(ButtonManager.BUTTON_HDR_PLUS);

            mAppController.getCameraAppUI().freezeScreenUntilPreviewReady();

            // Do not post this to avoid this module switch getting interleaved
            // with
            // other button callbacks.
            mActivity.onModeSelected(mGcamModeIndex);

            buttonManager.enableButtonClick(ButtonManager.BUTTON_HDR_PLUS);
        }
    }

    /**
     * Constructs a new photo module.
     */
    public PhotoModule(AppController app) {
        super(app);

        mGcamModeIndex = app.getAndroidContext().getResources()
                .getInteger(R.integer.camera_mode_gcam);
    }

    @Override
    public String getPeekAccessibilityString() {
        return mAppController.getAndroidContext().getResources()
                .getString(R.string.photo_accessibility_peek);
    }

    @Override
    public void init(CameraActivity activity, boolean isSecureCamera,
            boolean isCaptureIntent) {
        Log.i(TAG, "init Camera");

        mActivity = activity;
        // TODO: Need to look at the controller interface to see if we can get
        // rid of passing in the activity directly.
        mAppController = mActivity;

        /*
         * sync settings
         * 
         * @{
         */
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

        //DataModuleManager.getInstance(mActivity).addListener(this);
        mDataModuleCurrent.addListener(this);

        mActivity.getCameraAppUI().initSidePanel();

        /* @} */

        // SPRD initialize mJpegQualityController.
        mJpegQualityController = new JpegQualityController();

        mUI = createUI(mActivity);
        mActivity.setPreviewStatusListener(mUI);

        SettingsManager settingsManager = mActivity.getSettingsManager();
        // TODO: Move this to SettingsManager as a part of upgrade procedure.
        // Aspect Ratio selection dialog is only shown for Nexus 4, 5 and 6.
        if (mAppController.getCameraAppUI().shouldShowAspectRatioDialog()) {
            // Switch to back camera to set aspect ratio.
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_ID);
        }
        mCameraId = mDataModule.getInt(Keys.KEY_CAMERA_ID);

        Log.i(TAG, " init " + mAppController.getModuleScope() + ","
                + mCameraId);
        mContentResolver = mActivity.getContentResolver();

        // Surface texture is from camera screen nail and startPreview needs it.
        // This must be done before startPreview.
        mIsImageCaptureIntent = isImageCaptureIntent();
        mUI.setCountdownFinishedListener(this);

        mQuickCapture = mActivity.getIntent().getBooleanExtra(
                EXTRA_QUICK_CAPTURE, false);
        mHeadingSensor = new HeadingSensor(AndroidServices.instance()
                .provideSensorManager());

        /*
         * SPRD Bug:517483 SoundPool leaks. @{ Original Android code:
         * mCountdownSoundPlayer = new
         * SoundPlayer(mAppController.getAndroidContext());
         */

        try {
            mOneCameraManager = OneCameraModule.provideOneCameraManager();
        } catch (OneCameraException e) {
            Log.e(TAG, "Hardware manager failed to open.");
        }

        // TODO: Make this a part of app controller API.
        View cancelButton = mActivity.findViewById(R.id.shutter_cancel_button);
        cancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // SPRD: Fix bug 537444 if paused, ignore this event
                if (mPaused) return;

                cancelCountDown();
            }
        });
        // SPRD:Add for freeze_display
        sFreezeFrameControl = new FreezeFrameDisplayControl(mActivity, mUI,
                mIsImageCaptureIntent, mDataModuleCurrent.getBoolean(
                        Keys.KEY_FREEZE_FRAME_DISPLAY, true));
        sFreezeFrameControl.setListener(PhotoModule.this);
    }

    @Override
    public void onDreamSettingChangeListener(HashMap<String, String> keyList) {
        Log.e(TAG, "dreamPhotoonDreamSettingChangeListener  ");
        if (mCameraDevice == null) {
            return;
        }
        DataModuleBasic dataModulePhoto = DataModuleManager.getInstance(
                mAppController.getAndroidContext()).getCurrentDataModule();

        HashMap<String, String> keys = new HashMap<String, String>();
        keys.putAll(keyList);

        for (String key : keys.keySet()) {
            Log.e(TAG,
                    "onSettingChanged key = " + key + " value = "
                            + keys.get(key));
            switch (key) {
            case Keys.KEY_PICTURE_SIZE_BACK:
                restartPreview(true);
                return;
            case Keys.KEY_PICTURE_SIZE_FRONT:
                restartPreview(true);
                return;
            case Keys.KEY_JPEG_QUALITY:
                updateParametersPictureQuality();
                break;
            case Keys.KEY_CAMERA_COMPOSITION_LINE:
                break;
            case Keys.KEY_CAMERA_AI_DATECT:
                updateParametersAiDetect();
                break;
            case Keys.KEY_CAMER_ANTIBANDING:
                updateParametersAntibanding();
                break;
            case Keys.KEY_CAMERA_COLOR_EFFECT:
                updateParametersColorEffect();
                break;
            case Keys.KEY_CAMERA_PHOTOGRAPH_STABILIZATION:
                break;
            case Keys.KEY_CAMERA_HDR_NORMAL_PIC:
                break;
            case Keys.KEY_CAMERA_GRADIENTER_KEY:
                break;
            case Keys.KEY_CAMERA_TOUCHING_PHOTOGRAPH:
                break;
            case Keys.KEY_CAMERA_TIME_STAMP:
                updateTimeStamp();
                break;
            case Keys.KEY_FREEZE_FRAME_DISPLAY:
                break;
            case Keys.KEY_CAMERA_ZSL_DISPLAY:
                restartPreview(false);
                return;
            case Keys.KEY_CAMERA_VGESTURE:
                restartPreview(true);
                return;
            case Keys.KEY_CAMERA_BEAUTY_ENTERED:
                if (mPaused
                        || mAppController.getCameraProvider()
                                .waitingForCamera()) {
                    return;
                }
                updateMakeLevel();
                break;
            case Keys.KEY_FLASH_MODE:
                if (mPaused
                        || mAppController.getCameraProvider()
                                .waitingForCamera()) {
                    return;
                }
                updateParametersFlashMode();
                break;
            case Keys.KEY_COUNTDOWN_DURATION:
                break;
            case Keys.KEY_CAMERA_HDR:
                if (GcamHelper.hasGcamAsSeparateModule(mAppController
                        .getCameraFeatureConfig())) {
                    // Set the camera setting to default backfacing.
                    mDataModule.set(Keys.KEY_CAMERA_ID,
                            mDataModule.getStringDefault(Keys.KEY_CAMERA_ID));
                    switchToGcamCapture();
                } else {
                    updateParametersHDR();
                }
                break;
            case Keys.KEY_CAMER_METERING:
                updateParametersMetering();
                break;
            case Keys.KEY_EXPOSURE:
                updateParametersExposureCompensation();
                break;
            case Keys.KEY_CAMERA_ISO:
                updateParametersISO();
                break;
            case Keys.KEY_WHITE_BALANCE:
                updateParametersWhiteBalance();
                break;
            case Keys.KEY_CAMERA_CONTRAST:
                updateParametersContrast();
                break;
            case Keys.KEY_CAMERA_SATURATION:
                updateParametersSaturation();
                break;
            case Keys.KEY_CAMERA_BRIGHTNESS:
                updateParametersBrightness();
                break;
            case Keys.KEY_SCENE_MODE:
                updateParametersSceneMode();
                break;
            case Keys.KEY_FRONT_CAMERA_MIRROR:
                restartPreview(false);
                mActivity.getCameraAppUI().initSidePanel();
                return;
            case Keys.KEY_CAMERA_GRID_LINES:
                updateParametersGridLine();
                break;

            }
        }

        mActivity.getCameraAppUI().initSidePanel();

        if (mCameraDevice != null) {
            mCameraDevice.applySettings(mCameraSettings);
            mCameraSettings = mCameraDevice.getSettings();
        }
    }

    private void restartPreview(boolean freezeScreen) {
        stopPreview();
        if(freezeScreen){
            mAppController.getCameraAppUI().freezeScreenUntilPreviewReady();
        }
        /* SPRD: fix bug620875 need make sure freeze screen draw on Time @{ */
        mHandler.post(new Runnable() {
            public void run() {
                if (!mPaused && mCameraDevice != null) {
                    startPreview(false);
                    if (mVGestureController != null
                            && mDataModuleCurrent
                                    .isEnableSettingConfig(Keys.KEY_CAMERA_VGESTURE)) {
                        mVGestureController.refreshParamaters(mActivity, mCameraDevice,
                                mHandler, mDisplayOrientation, isCameraFrontFacing(),
                                mUI.getRootView());
                    }
                }
            }
        });
        /* @ } */
    }

    public void restoreToDefaultSettings(){
        // UI part reset
        updateUIGridLineToDefault();

        // camera device setting part reset
        /**
        if (mCameraDevice != null) {
            mCameraDevice.applySettings(mCameraSettings);
        }
        */
    }

    private void updateUIGridLineToDefault() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_GRID_LINES)){
            return;
        }

        String grid = mDataModuleCurrent.getStringDefault(Keys.KEY_CAMERA_GRID_LINES);
        mAppController.getCameraAppUI().updateScreenGridLines(grid);
        Log.d(TAG, "updateUIGridLineToDefault = " + grid);
    }

    protected void cancelCountDown() {
        if (mUI.isCountingDown()) {
            // Cancel on-going countdown.
            mUI.cancelCountDown();
            mUI.enablePreviewOverlayHint(true);//SPRD:fix bug648477
        }
        if (!mActivity.getCameraAppUI().isInIntentReview()// SPRD:Fix bug 497077
                && !mActivity.getCameraAppUI().isInFreezeReview()){// SPRD:Fix bug 398341
            mAppController.getCameraAppUI().transitionToCapture();
            mAppController.getCameraAppUI().showModeOptions();
            mAppController.setShutterEnabled(true);
            // SPRD:cancel CountDown cannot swipe
            mAppController.getCameraAppUI().setSwipeEnabled(true);
        }
        /* SPRD: fix bug 473462 add burst capture @{ */
        if (mIsContinousCaptureFinish) {
            mIsContinousCaptureFinish = false;
        }
        /* @} */
        /*SPRD: fix bug 599542 add for touch capture and count down @{*/
        isOnSingleTapUp = false;
        mTapUpX = 0;
        mTapUpY = 0;
        /* @} */
    }


    protected void cancelVgestureCountDown(boolean restart) {

        if(mVGestureController != null){
            mVGestureController.stopTimer(restart);
        }
        mAppController.getCameraAppUI().updatePreviewUI(View.VISIBLE);
        mAppController.getCameraAppUI().transitionToCapture();
        mAppController.getCameraAppUI().showModeOptions();
        mAppController.getCameraAppUI().setSwipeEnabled(true);

    }

    @Override
    public boolean isUsingBottomBar() {
        return true;
    }

    private void initializeControlByIntent() {
        if (mIsImageCaptureIntent) {
            /*
             * SPRD: fix bug 497077 If the mode of bottom bar is
             * MODE_INTENT_REVIEW, then the IntentReviewLayout should be shown
             * 
             * @{
             */
            if (!mActivity.getCameraAppUI().isInIntentReview()) {
                mActivity.getCameraAppUI().transitionToIntentCaptureLayout();
            } else {
                mActivity.getCameraAppUI().transitionToIntentReviewLayout();
            }
            /* @} */
            setupCaptureParams();
        }
    }

    private void onPreviewStarted() {
        mAppController.onPreviewStarted();
        /**
         * SPRD: fix bug 473462 add burst capture @{
         * mAppController.setShutterEnabled(true); setCameraState(IDLE);
         */
        Log.i(TAG, "onPreviewStarted mCameraState=" + mCameraState
                + " mContinuousCaptureCount=" + mContinuousCaptureCount);
        if (mContinuousCaptureCount <= 0 || !isBurstCapture()
                || (mCameraState == PREVIEW_STOPPED)) {// SPRD:Fix
                                                       // bug
                                                       // 388289
            if (mIsImageCaptureIntent) {// SPRD:Fix bug 391829
                if (mUI.isReviewShow()) {// SPRD BUG: 402084
                    mAppController.getCameraAppUI().hideModeOptions();
                } else {
                    mAppController.getCameraAppUI().showModeOptions();
                }
            }
            setCameraState(IDLE);
        }

        doOnPreviewStartedSpecial(isCameraIdle(), isHdr(), mActivity, mCameraDevice, mHandler,
                mDisplayOrientation, isCameraFrontFacing(), mUI.getRootView());
        /*
         * SPRD:Modify for ai detect @{ startFaceDetection();
         */
        updateFace();
        /* @} */

        /* SPRD: Fix bug 572631, optimize camera launch time @{ */
        if (mCountdownSoundPlayer == null) {
            mCountdownSoundPlayer = new SoundPlayer(mAppController.getAndroidContext());

            mCountdownSoundPlayer.loadSound(R.raw.timer_final_second);
            mCountdownSoundPlayer.loadSound(R.raw.timer_increment);
        }

        if (mCameraSound == null) {
            mCameraSound = new MediaActionSound();
            // Not required, but reduces latency when playback is requested later
            mCameraSound.load(MediaActionSound.SHUTTER_CLICK);
        }
        /* @} */
    }

    @Override
    public void onPreviewUIReady() {
        Log.i(TAG, "onPreviewUIReady");
        startPreview(true);
    }

    @Override
    public void onPreviewUIDestroyed() {
        Log.i(TAG, "onPreviewUIDestroyed,CameraDevice = " + mCameraDevice);
        if (mCameraDevice == null) {
            return;
        }
        mCameraDevice.setPreviewTexture(null);
        stopPreview();
    }

    @Override
    public void startPreCaptureAnimation() {
        mAppController.startFlashAnimation(false);
    }

    private void onCameraOpened() {
        openCameraCommon();
        initializeControlByIntent();
    }

    protected void switchCamera() {
        Log.i(TAG, "switchCamera start");
        if (mPaused) {
            return;
        }

        /* SPRD:Fix bug 453868 @{ */
        int cameraId = -2;
        if (mCameraDevice != null) {// SPRD:Fix bug 459316.
            cameraId = mCameraDevice.getCameraId();
        }
        Log.i(TAG, " Start to switch camera. id=" + cameraId
                + " mPendingSwitchCameraId=" + mPendingSwitchCameraId);
        if (cameraId == mPendingSwitchCameraId) {
            return;
        }
        /* @} */
        /* SPRD:Fix bug 445882 @{ */
        if (mCameraDevice != null && mFocusManager.isInStateFocusing()) {
            mCameraDevice.cancelAutoFocus();
        }
        /* @} */
        cancelCountDown();
        cancelVgestureCountDown(false);

        mAppController.freezeScreenUntilPreviewReady();
        SettingsManager settingsManager = mActivity.getSettingsManager();

        Log.i(TAG, "Start to switch camera. id=" + mPendingSwitchCameraId);
        closeCamera();
        mCameraId = mPendingSwitchCameraId;

        mDataModule.set(Keys.KEY_CAMERA_ID, mCameraId);

        requestCameraOpen();

        mUI.clearFaces();
        if (mFocusManager != null) {
            mFocusManager.removeMessages();
        }

        mMirror = isCameraFrontFacing();
        mFocusManager.setMirror(mMirror);
        // Start switch camera animation. Post a message because
        // onFrameAvailable from the old camera may already exist.
        //SPRD:fix bug534665 add some mutex about scene mode
        setExposureIfNecessary();
        Log.i(TAG, "switchCamera end");
    }

    protected void mustDo() {
        mCameraId = mPendingSwitchCameraId;
        /*
         * mActivity.getSettingsManager().set(mAppController.getModuleScope(),
         * Keys.KEY_CAMERA_ID, mCameraId);
         */
        mMirror = isCameraFrontFacing();
        mFocusManager.setMirror(mMirror);
    }

    /**
     * Uses the {@link CameraProvider} to open the currently-selected camera
     * device, using {@link GservicesHelper} to choose between API-1 and API-2.
     */
    protected void requestCameraOpen() {
        /**
         * SPRD: fix bug47362 Log.v(TAG, "requestCameraOpen");
         */
        /*
         * SPRD:Fix bug 513841 KEY_CAMERA_ID may be changed during the camera in
         * background if user do switch camera action in contacts, and then we
         * resume the camera, the init function will not be called, so the
         * mCameraId will not be the latest one. Which will cause the switch
         * camera function can not be used. @{
         */
        SettingsManager settingsManager = mActivity.getSettingsManager();
        if (mCameraId != mDataModule.getInt(Keys.KEY_CAMERA_ID)) {
            mCameraId = mDataModule.getInt(Keys.KEY_CAMERA_ID);
        }
        /* @} */
        Log.i(TAG, "requestCameraOpen mCameraId:" + mCameraId);
        /**
         * SPRD: Change for New Feature VGesture and dream camera
         * original code
         * @{
        mActivity.getCameraProvider().requestCamera(
                mCameraId,
                GservicesHelper.useCamera2ApiThroughPortabilityLayer(mActivity
                         .getContentResolver()));
          */
        doCameraOpen(mCameraId);
       /**
         * @}
         */
    }

    protected void doCameraOpen(int cameraId){
        mActivity.getCameraProvider().requestCamera(mCameraId,
                GservicesHelper.useCamera2ApiThroughPortabilityLayer(mActivity
                        .getContentResolver()));
    }

    public final ButtonManager.ButtonCallback mCameraCallback = new ButtonManager.ButtonCallback() {
        @Override
        public void onStateChanged(int state) {
            // At the time this callback is fired, the camera id
            // has be set to the desired camera.

            if (mPaused
                    || mAppController.getCameraProvider().waitingForCamera()) {
                return;
            }
            // If switching to back camera, and HDR+ is still on,
            // switch back to gcam, otherwise handle callback normally.
            /*
             * SettingsManager settingsManager = mActivity.getSettingsManager();
             * if (Keys.isCameraBackFacing(settingsManager)) { if
             * (Keys.requestsReturnToHdrPlus(settingsManager,
             * mAppController.getModuleScope())) { switchToGcamCapture();
             * return; } }
             */

            ButtonManager buttonManager = mActivity.getButtonManager();
            buttonManager.disableCameraButtonAndBlock();

            mPendingSwitchCameraId = state;

            Log.d(TAG, "bbbb Start to switch camera. cameraId=" + state);
            // We need to keep a preview frame for the animation before
            // releasing the camera. This will trigger
            // onPreviewTextureCopied.
            // TODO: Need to animate the camera switch
            switchCamera();
        }
    };

    public final ButtonManager.ButtonCallback mHdrPlusCallback = new ButtonManager.ButtonCallback() {
        @Override
        public void onStateChanged(int state) {
            Log.v(TAG, "mHdrPlusCallback, onStateChanged state=" + state);
            SettingsManager settingsManager = mActivity.getSettingsManager();
            if (GcamHelper.hasGcamAsSeparateModule(mAppController
                    .getCameraFeatureConfig())) {
                // Set the camera setting to default backfacing.
                settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                        Keys.KEY_CAMERA_ID);
                switchToGcamCapture();
            } else {
                boolean sceneShowToast = false;
                if (Keys.isHdrOn(settingsManager)) {
                    String sceneMode = settingsManager.getString(
                            settingsManager.SCOPE_GLOBAL, Keys.KEY_SCENE_MODE);
                    if (!"auto".equals(sceneMode)) {
                        sceneShowToast = true;
                    }
                    settingsManager.set(
                            SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_SCENE_MODE,
                            mCameraCapabilities.getStringifier().stringify(
                                    CameraCapabilities.SceneMode.HDR));
                } else {
                    settingsManager.set(
                            SettingsManager.SCOPE_GLOBAL,
                            Keys.KEY_SCENE_MODE,
                            mCameraCapabilities.getStringifier().stringify(
                                    CameraCapabilities.SceneMode.AUTO));
                }
                updateParametersSceneMode();
                if (mCameraDevice != null) {
                    mCameraDevice.applySettings(mCameraSettings);
                }
                /*
                 * SPRD: All MUTEX OPERATION in onSettingsChanged function.
                 * updateSceneMode();
                 */
                /*
                 * scene mode mutex with hdr is special, so, if hdr on and toast
                 * did not show just now, which isToastExit = false, here, we
                 * should show the toast. otherwise, if the toast has been shown
                 * just now, which isToastExit = true we don't show toast any
                 * more.
                 */
                if (!isToastExit && sceneShowToast) {
                    Toast.makeText(
                            mActivity,
                            mActivity.getResources().getString(
                                    R.string.hdr_mutex), Toast.LENGTH_LONG)
                            .show();
                    isToastExit = false;
                }
            }
        }
    };

    public final ButtonManager.ButtonCallback mFlashCallback = new ButtonManager.ButtonCallback() {
        @Override
        public void onStateChanged(int state) {

            if (mPaused
                    || mAppController.getCameraProvider().waitingForCamera()) {
                return;
            }

            Log.d(TAG, "mFlashCallback=" + state);

            // updateParametersFlashMode();
            // if (mCameraDevice != null) {
            // mCameraDevice.applySettings(mCameraSettings);
            // }
        }
    };

    public final ButtonManager.ButtonCallback mMakeupCallback = new ButtonManager.ButtonCallback() {
        @Override
        public void onStateChanged(int state) {
            // At the time this callback is fired, the camera id
            // has be set to the desired camera.

            if (mPaused
                    || mAppController.getCameraProvider().waitingForCamera()) {
                return;
            }
            if (mUI != null) {
                updateMakeLevel();
            }
        }
    };

    private final View.OnClickListener mCancelCallback = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            onCaptureCancelled();
        }
    };

    private final View.OnClickListener mDoneCallback = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            onCaptureDone();
        }
    };

    private final View.OnClickListener mRetakeCallback = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            mActivity.getCameraAppUI().transitionToIntentCaptureLayout();
            onCaptureRetake();
        }
    };

    @Override
    public void hardResetSettings(SettingsManager settingsManager) {
        // PhotoModule should hard reset HDR+ to off,
        // and HDR to off if HDR+ is supported.
        settingsManager.set(SettingsManager.SCOPE_GLOBAL,
                Keys.KEY_CAMERA_HDR_PLUS, false);
        if (GcamHelper.hasGcamAsSeparateModule(mAppController
                .getCameraFeatureConfig())) {
            settingsManager.set(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_HDR, false);
        }
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
        bottomBarSpec.enableFlash = !mAppController.getSettingsManager()
                .getBoolean(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_HDR) && !mIsBatteryLow;
        bottomBarSpec.enableHdr = true;
        bottomBarSpec.hdrCallback = mHdrPlusCallback;
        bottomBarSpec.enableGridLines = false;
        if (mCameraCapabilities != null) {
            bottomBarSpec.enableExposureCompensation = true;
            /*
            bottomBarSpec.exposureCompensationSetCallback = new CameraAppUI.BottomBarUISpec.ExposureCompensationSetCallback() {
                @Override
                public void setExposure(int value) {
                    //SPRD:fix bug534665 add some mutex about scene mode
                    // EXPOSURE - SCENE MODE
                    if (value != 0) {
                       SettingsManager settingsManager = mActivity.getSettingsManager();
                       String sceneMode = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                                Keys.KEY_SCENE_MODE);
                        if (!"auto".equals(sceneMode)) {
                            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL, Keys.KEY_SCENE_MODE);
                            updateParametersSceneMode();
                            Toast.makeText(mActivity,
                                    mActivity.getResources().getString(R.string.exposure_mutex),
                                    Toast.LENGTH_LONG).show();
                        }
                    }
                    setExposureCompensation(value);
                }
            };
            */
            bottomBarSpec.minExposureCompensation = mCameraCapabilities
                    .getMinExposureCompensation();
            bottomBarSpec.maxExposureCompensation = mCameraCapabilities
                    .getMaxExposureCompensation();
            bottomBarSpec.exposureCompensationStep = mCameraCapabilities
                    .getExposureCompensationStep();
        }

        bottomBarSpec.enableSelfTimer = true;
        bottomBarSpec.showSelfTimer = true;

        if (isImageCaptureIntent()) {
            bottomBarSpec.showCancel = true;
            bottomBarSpec.cancelCallback = mCancelCallback;
            bottomBarSpec.showDone = true;
            bottomBarSpec.doneCallback = mDoneCallback;
            bottomBarSpec.showRetake = true;
            bottomBarSpec.retakeCallback = mRetakeCallback;
        }

        return bottomBarSpec;
    }

    // either open a new camera or switch cameras
    private void openCameraCommon() {
        mUI.onCameraOpened(mCameraCapabilities, mCameraSettings);
        if (mIsImageCaptureIntent) {
            // Set hdr plus to default: off.
            SettingsManager settingsManager = mActivity.getSettingsManager();
            settingsManager.setToDefault(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_HDR_PLUS);
        }
        /*
         * SPRD: All MUTEX OPERATION in onSettingsChanged function.
         * updateSceneMode();
         */
    }

    /* SPRD: fix bug 474665 add shutter sound switch @{ */
    private void updateCameraShutterSound() {
        Log.d(TAG, "updateCameraShutterSound mShutterSoundEnabled ="
                + mShutterSoundEnabled);
        if (mCameraDevice != null) {
            mCameraDevice.enableShutterSound(mShutterSoundEnabled);
        }
    }

    /* @} */

    @Override
    public void updatePreviewAspectRatio(float aspectRatio) {
        mAppController.updatePreviewAspectRatio(aspectRatio);
    }

    private void resetExposureCompensation() {
        SettingsManager settingsManager = mActivity.getSettingsManager();
        if (settingsManager == null) {
            Log.e(TAG, "Settings manager is null!");
            return;
        }
        settingsManager.setToDefault(mAppController.getCameraScope(),
                Keys.KEY_EXPOSURE);
    }

    // Snapshots can only be taken after this is called. It should be called
    // once only. We could have done these things in onCreate() but we want to
    // make preview screen appear as soon as possible.
    private void initializeFirstTime() {
        if (mFirstTimeInitialized || mPaused) {
            return;
        }

        mUI.initializeFirstTime();

        // We set the listener only when both service and shutterbutton
        // are initialized.
        getServices().getMemoryManager().addListener(this);

        mNamedImages = new NamedImages();

        mFirstTimeInitialized = true;
        addIdleHandler();

        /**
         * SPRD: Fix bug 572631, optimize camera launch time,
         * Original Code
         *
        mActivity.updateStorageSpaceAndHint(null);
         */
    }

    // If the activity is paused and resumed, this method will be called in
    // onResume.
    private void initializeSecondTime() {
        getServices().getMemoryManager().addListener(this);
        mNamedImages = new NamedImages();
        mUI.initializeSecondTime(mCameraCapabilities, mCameraSettings);
    }

    private void addIdleHandler() {
        MessageQueue queue = Looper.myQueue();
        queue.addIdleHandler(new MessageQueue.IdleHandler() {
            @Override
            public boolean queueIdle() {
                Storage.ensureOSXCompatible();
                return false;
            }
        });
    }

    /**
     * The method implement from FocusOverlayManager.Listener interface;
     * To controller Face detection function start by call CameraProxy.startFaceDetection method
     */
    @Override
    public void startFaceDetection() {
        if (mFaceDetectionStarted || mCameraDevice == null) {
            return;
        }
        Log.d(TAG, "startFaceDetection ");
        /*
         * SPRD:Modify for ai detect @{
         * if (mCameraCapabilities.getMaxNumOfFacesSupported() > 0) {
         */
        mCameraDevice.setFaceDetectionCallback(mHandler, mUI);
        mCameraDevice.startFaceDetection();
        mFaceDetectionStarted = true;
        mUI.onStartFaceDetection(mDisplayOrientation, isCameraFrontFacing());
        SessionStatsCollector.instance().faceScanActive(true);
        /*
         * }
         * 
         * @}
         */
    }

    @Override
    public void stopFaceDetection() {
        mUI.clearFaces();
        Log.d(TAG, "stopFaceDetection mFaceDetectionStarted=" + mFaceDetectionStarted);
        if (mCameraDevice == null) {//SPRD:fix bug625571 for OOM
            return;
        }
        /*
         * SPRD:Modify for ai detect @{ if
         * (mCameraCapabilities.getMaxNumOfFacesSupported() > 0) {
         */
        mCameraDevice.setFaceDetectionCallback(null, null);
        mCameraDevice.stopFaceDetection();
        mFaceDetectionStarted = false;
        /* SPRD:fix bug501085 Click screen,the face detect still appear@{ */
        mUI.pauseFaceDetection();
        /* }@ */
        mUI.clearFaces();
        SessionStatsCollector.instance().faceScanActive(false);
        /*
         * }
         * 
         * @}
         */
    }

    private final class ShutterCallback implements CameraShutterCallback {

        private final boolean mNeedsAnimation;

        public ShutterCallback(boolean needsAnimation) {
            mNeedsAnimation = needsAnimation;
        }

        @Override
        public void onShutter(CameraProxy camera) {
            mShutterCallbackTime = System.currentTimeMillis();
            mShutterLag = mShutterCallbackTime - mCaptureStartTime;
            mHasStartCapture = true;
            Log.i(TAG, "mShutterLag = " + mShutterLag + "ms");
            if (mNeedsAnimation) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        animateAfterShutter();
                    }
                });
            }
        }
    }

    private final class PostViewPictureCallback implements CameraPictureCallback {
        @Override
        public void onPictureTaken(byte[] data, CameraProxy camera) {
            mPostViewPictureCallbackTime = System.currentTimeMillis();
            Log.v(TAG, "mShutterToPostViewCallbackTime = "
                    + (mPostViewPictureCallbackTime - mShutterCallbackTime)
                    + "ms");
        }
    }

    private final class RawPictureCallback implements CameraPictureCallback {
        @Override
        public void onPictureTaken(byte[] rawData, CameraProxy camera) {
            mRawPictureCallbackTime = System.currentTimeMillis();
            Log.v(TAG, "mShutterToRawCallbackTime = "
                    + (mRawPictureCallbackTime - mShutterCallbackTime) + "ms");
        }
    }

    private static class ResizeBundle {
        byte[] jpegData;
        float targetAspectRatio;
        ExifInterface exif;
    }

    /**
     * @return Cropped image if the target aspect ratio is larger than the jpeg
     *         aspect ratio on the long axis. The original jpeg otherwise.
     */
    private ResizeBundle cropJpegDataToAspectRatio(ResizeBundle dataBundle) {

        final byte[] jpegData = dataBundle.jpegData;
        final ExifInterface exif = dataBundle.exif;
        float targetAspectRatio = dataBundle.targetAspectRatio;

        Bitmap original = BitmapFactory.decodeByteArray(jpegData, 0,
                jpegData.length);
        int originalWidth = original.getWidth();
        int originalHeight = original.getHeight();
        int newWidth;
        int newHeight;

        if (originalWidth > originalHeight) {
            newHeight = (int) (originalWidth / targetAspectRatio);
            newWidth = originalWidth;
        } else {
            newWidth = (int) (originalHeight / targetAspectRatio);
            newHeight = originalHeight;
        }
        int xOffset = (originalWidth - newWidth) / 2;
        int yOffset = (originalHeight - newHeight) / 2;

        if (xOffset < 0 || yOffset < 0) {
            return dataBundle;
        }

        Bitmap resized = Bitmap.createBitmap(original, xOffset, yOffset,
                newWidth, newHeight);
        exif.setTagValue(ExifInterface.TAG_PIXEL_X_DIMENSION, new Integer(
                newWidth));
        exif.setTagValue(ExifInterface.TAG_PIXEL_Y_DIMENSION, new Integer(
                newHeight));

        ByteArrayOutputStream stream = new ByteArrayOutputStream();

        resized.compress(Bitmap.CompressFormat.JPEG, 90, stream);
        dataBundle.jpegData = stream.toByteArray();
        return dataBundle;
    }

    private MediaActionSound mCameraSound;// SPRD:fix bug473462

    private final class JpegPictureCallback implements CameraPictureCallback {
        Location mLocation;

        public JpegPictureCallback(Location loc) {
            mLocation = loc;
            mHasCaputureCount = 0;
            //mTotalCaputureCount = 0;
        }

        @Override
        public void onPictureTaken(final byte[] originalJpegData,
                final CameraProxy camera) {
            /**
             * SPRD:fix bug 473462 add burst capture @{ Log.i(TAG,
             * "onPictureTaken");
             */
            Log.i(TAG, "onPictureTaken mCameraState=" + mCameraState
                    + " mContinuousCaptureCount = " + mContinuousCaptureCount
                    + " isBurstCapture()= " + isBurstCapture()
                    + " mIsContinousCaptureFinish ="
                    + mIsContinousCaptureFinish + "mHasCaputureCount = "
                    + mHasCaputureCount);
            mThumbnailHasInvalid = false;
            /*
             * Add For Dream Camera in IntervalPhotoModule/ContinuePhotoModule
             * 
             * @{
             */
            if (!shutterAgain()) {
                if(!mBurstNotShowShutterButton) {
                    mAppController.setShutterEnabled(true);
                }
                /* dream test 50 @{ */
                doSomethingWhenonPictureTaken(); // for dream camera if it want to do something
                /* SPRD: bug 517578 @{ */
                if (isUcamBeautyCanBeUsed()) {
                    mUI.setFilterMakeupButtonEnabled(true);
                }
                /* @} */
            }

            /* @} */
            mAppController.getCameraAppUI().setBottomPanelLeftRightClickable(true);
            if (mPaused) {
                return;
            }

            /* SPRD:fix bug 473462 add burst capture @ */
            if (mContinuousCaptureCount > 0) {
                mContinuousCaptureCount--;
            }
            mHasCaputureCount++;

            if (!mIsImageCaptureIntent
                    && !isSetFreezeFrameDisplay()
                    //&& getModuleTpye() != DreamModule.INTERVAL_MODULE
                    && (!isBurstCapture()
                            || (mContinuousCaptureCount > 1 && mIsContinousCaptureFinish)
                            || mHasCaputureCount == getCurrentBurstCountFromSettings())) {
                /* SPRD:fix bug620386 move up the thumbnail at Interval module @{ */
                if (getModuleTpye() != DreamModule.INTERVAL_MODULE) {
                    mThumbnailHasInvalid = true;
                }
                /* @} */
                startPeekAnimation(originalJpegData);
            }

            if (mAppController.isPlaySoundEnable()) {//SPRD:fix bug 618933 api1 not need shutter sound
                mCameraSound.play(MediaActionSound.SHUTTER_CLICK);
                /* SPRD: Fix bug 581173 when burst capture, the shutter sound should be the same @{ */
                if (mIsBurstStarted && (getContinuousCaptureCount() == 0 || mIsContinousCaptureFinish)) {
                    AndroidServices.instance().provideAudioManager().abandonAudioFocus(null);
                    mIsBurstStarted = false;
                    }
                /* @}*/
            }
            /* @} */
            if (mIsImageCaptureIntent) {
                stopPreview();
            }
            if (mSceneMode == CameraCapabilities.SceneMode.HDR) {
                mUI.setSwipingEnabled(true);
            }

            /* SPRD:fix bug 473462 add burst capture @ */
            if (mIsContinousCaptureFinish) {
                Log.d(TAG, "isShutterButtonPressed");
                mContinuousCaptureCount = 0;
                mIsContinousCaptureFinish = false;
                mBurstMode = false;
                // mUI.showBurstScreenHint(mHasCaputureCount);//SPRD:fix bug
                // 497854 when cancel 10
                // burst capture,the count of pics saveing is wrong
            } else if (isBurstCapture() && mContinuousCaptureCount == 0) {
                mIsContinousCaptureFinish = false;
                mBurstMode = false;
                mUI.showBurstScreenHint(mHasCaputureCount);
            }
            mNamedImages.nameNewImage(mCaptureStartTime);
            /* @} */

            mJpegPictureCallbackTime = System.currentTimeMillis();
            // If postview callback has arrived, the captured image is displayed
            // in postview callback. If not, the captured image is displayed in
            // raw picture callback.
            if (mPostViewPictureCallbackTime != 0) {
                mShutterToPictureDisplayedTime = mPostViewPictureCallbackTime
                        - mShutterCallbackTime;
                mPictureDisplayedToJpegCallbackTime = mJpegPictureCallbackTime
                        - mPostViewPictureCallbackTime;
            } else {
                mShutterToPictureDisplayedTime = mRawPictureCallbackTime
                        - mShutterCallbackTime;
                mPictureDisplayedToJpegCallbackTime = mJpegPictureCallbackTime
                        - mRawPictureCallbackTime;
            }
            Log.v(TAG, "mPictureDisplayedToJpegCallbackTime = "
                    + mPictureDisplayedToJpegCallbackTime + "ms");

            /**
             * SPRD: fix bug 473462 add burst capture if
             * (!mIsImageCaptureIntent) {
             */
            if (!mIsImageCaptureIntent
                    && (GservicesHelper
                            .useCamera2ApiThroughPortabilityLayer(mActivity
                                    .getContentResolver()) ? true
                            : (mContinuousCaptureCount <= 0))) {
                setupPreview();
            }

            long now = System.currentTimeMillis();
            mJpegCallbackFinishTime = now - mJpegPictureCallbackTime;
            Log.v(TAG, "mJpegCallbackFinishTime = " + mJpegCallbackFinishTime
                    + "ms");
            mJpegPictureCallbackTime = 0;

            final ExifInterface exif = Exif.getExif(originalJpegData);
            final NamedEntity name = mNamedImages.getNextNameEntity();
            if (mShouldResizeTo16x9) {
                Log.i(TAG, "mShouldResizeTo16x9");
                final ResizeBundle dataBundle = new ResizeBundle();
                dataBundle.jpegData = originalJpegData;
                dataBundle.targetAspectRatio = ResolutionUtil.NEXUS_5_LARGE_16_BY_9_ASPECT_RATIO;
                dataBundle.exif = exif;
                new AsyncTask<ResizeBundle, Void, ResizeBundle>() {

                    @Override
                    protected ResizeBundle doInBackground(
                            ResizeBundle... resizeBundles) {
                        return cropJpegDataToAspectRatio(resizeBundles[0]);
                    }

                    @Override
                    protected void onPostExecute(ResizeBundle result) {
                        saveFinalPhoto(result.jpegData, name, result.exif,
                                camera);
                    }
                }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, dataBundle);

            } else {
                saveFinalPhoto(originalJpegData, name, exif, camera);
            }

            /* SPRD:Fix bug 411062 @{ */
            if (getContinuousCount() <= 1 && getModuleTpye() != DreamModule.AUDIOPICTURE_MODULE) {
                mUI.enablePreviewOverlayHint(true);
                Log.i(TAG, "onPictureTaken enablePreviewOverlayHint!");
            }
            /* @} */
        }

        void saveFinalPhoto(final byte[] jpegData, NamedEntity name,
                final ExifInterface exif, CameraProxy camera) {
            Log.i(TAG, "saveFinalPhoto start!");
            int orientation = Exif.getOrientation(exif);

            float zoomValue = 1.0f;
            if (mCameraCapabilities.supports(CameraCapabilities.Feature.ZOOM)) {
                zoomValue = mCameraSettings.getCurrentZoomRatio();
            }
            boolean hdrOn = CameraCapabilities.SceneMode.HDR == mSceneMode;
            String flashSetting = mActivity.getSettingsManager().getString(
                    mAppController.getCameraScope(), Keys.KEY_FLASH_MODE);
            boolean gridLinesOn = Keys.areGridLinesOn(mActivity
                    .getSettingsManager());
            Log.i(TAG, "saveFinalPhoto title=" + name.title + ".jpg");// SPRD:Fix
                                                                      // bug
                                                                      // 419844
            UsageStatistics.instance().photoCaptureDoneEvent(
                    eventprotos.NavigationChange.Mode.PHOTO_CAPTURE,
                    name.title + ".jpg", exif, isCameraFrontFacing(), hdrOn,
                    zoomValue, flashSetting, gridLinesOn,
                    (float) mTimerDuration, null, mShutterTouchCoordinate,
                    mVolumeButtonClickedFlag, null, null, null);
            mShutterTouchCoordinate = null;
            mVolumeButtonClickedFlag = false;

            Log.i(TAG, "saveFinalPhoto mIsImageCaptureIntent="
                    + mIsImageCaptureIntent);
            if (!mIsImageCaptureIntent) {
                // Calculate the width and the height of the jpeg.
                Integer exifWidth = exif
                        .getTagIntValue(ExifInterface.TAG_PIXEL_X_DIMENSION);
                Integer exifHeight = exif
                        .getTagIntValue(ExifInterface.TAG_PIXEL_Y_DIMENSION);
                int width, height;
                if (mShouldResizeTo16x9 && exifWidth != null
                        && exifHeight != null) {
                    width = exifWidth;
                    height = exifHeight;
                } else {
                    Size s = new Size(mCameraSettings.getCurrentPhotoSize());
                    if ((mJpegRotation + orientation) % 180 == 0) {
                        width = s.width();
                        height = s.height();
                    } else {
                        width = s.height();
                        height = s.width();
                    }
                }
                String title = (name == null) ? null : name.title;
                long date = (name == null) ? -1 : name.date;

                // Handle debug mode outputs
                if (mDebugUri != null) {
                    // If using a debug uri, save jpeg there.
                    saveToDebugUri(jpegData);

                    // Adjust the title of the debug image shown in mediastore.
                    if (title != null) {
                        title = DEBUG_IMAGE_PREFIX + title;
                    }
                }

                if (title == null) {
                    Log.e(TAG, "Unbalanced name/data pair");
                } else {
                    if (date == -1) {
                        date = mCaptureStartTime;
                    }
                    int heading = mHeadingSensor.getCurrentHeading();
                    if (heading != HeadingSensor.INVALID_HEADING) {
                        // heading direction has been updated by the sensor.
                        ExifTag directionRefTag = exif.buildTag(
                                ExifInterface.TAG_GPS_IMG_DIRECTION_REF,
                                ExifInterface.GpsTrackRef.MAGNETIC_DIRECTION);
                        ExifTag directionTag = exif.buildTag(
                                ExifInterface.TAG_GPS_IMG_DIRECTION,
                                new Rational(heading, 1));
                        exif.setTag(directionRefTag);
                        exif.setTag(directionTag);
                    }
                    /*
                     * SPRD: Fix bug 535110, Photo voice record. @{
                     * Original Android code :
                    getServices().getMediaSaver().addImage(jpegData, title,
                            date, mLocation, width, height, orientation, exif,
                            mOnMediaSavedListener);
                     */
                    if (mActivity.getCurrentModuleIndex() == mActivity
                            .getResources().getInteger(
                                    R.integer.camera_mode_audio_picture)) {
                        initData(jpegData, title, date, width, height,
                                orientation, exif, mLocation,
                                mOnMediaSavedListener);
                    } else {
                        getServices().getMediaSaver().addImage(jpegData, title,
                                date, mLocation, width, height, orientation,
                                exif, mOnMediaSavedListener);
                        mUI.setPictureInfo(width, height,orientation);//SPRD:fix bug 625571
                    }
                    /* @} */
                }
                // Animate capture with real jpeg data instead of a preview
                // frame.
                /**
                 * SPRD:fix bug 473462 add burs capture
                 * mUI.animateCapture(jpegData, orientation, mMirror);
                 */
            } else {
                Log.i(TAG, "saveFinalPhoto mQuickCapture=" + mQuickCapture);
                mJpegImageData = jpegData;
                if (!mQuickCapture) {
                    Log.v(TAG, "showing UI");
                    mUI.showCapturedImageForReview(jpegData, orientation,
                            mMirror);
                    mAppController.getCameraAppUI().hideModeOptions();// SPRD
                                                                      // BUG:
                                                                      // 402084
                } else {
                    onCaptureDone();
                }
            }
            /* SPRD: ADD for FreezeDisplay */
            if (!mIsImageCaptureIntent && sFreezeFrameControl != null
                    && sFreezeFrameControl.isFreezeFrame(isSetFreezeFrameDisplay())) {
                sFreezeFrameControl.proxyAnimation(true, mDisplayOrientation);
            }
            // Send the taken photo to remote shutter listeners, if any are
            // registered.
            getServices().getRemoteShutterListener().onPictureTaken(jpegData);

            // Check this in advance of each shot so we don't add to shutter
            // latency. It's true that someone else could write to the SD card
            // in the mean time and fill it, but that could have happened
            // between the shutter press and saving the JPEG too.
            /* SPRD: fix bug620875 continue caputre not need more AsyncTask @{ */
            if (!isBurstCapture()
                || (mContinuousCaptureCount > 1 && mIsContinousCaptureFinish)
                || mHasCaputureCount == getCurrentBurstCountFromSettings()
                || (mHasCaputureCount % 10)  == 0)
            mActivity.updateStorageSpaceAndHint(null);
            /* @} */
            Log.i(TAG, "saveFinalPhoto end! mCameraState=" + mCameraState
                    + " mContinuousCaptureCount=" + mContinuousCaptureCount);
            installIntentFilter();
        }
    }


    private void startPeekAnimation(final byte[] jpegData) {
        Log.i(TAG, "startPeekAnimation");
        ExifInterface exif = Exif.getExif(jpegData);
        Bitmap bitmap = exif.getThumbnailBitmap();
        /* remove this extra rotation after the driver bug is fixed @{ */
        if (bitmap != null) {
            int orientation = Exif.getOrientation(exif);
            // if orientation is zero, createBitmap will return original bitmap
            // but not create a new bitmap
            // then we call rotatedBitmap.recycle(), which makes the variable
            // bitmap recycled
            if (orientation != 0) {
                Matrix matrix = new Matrix();
                matrix.setRotate(orientation);
                Bitmap rotatedBitmap = bitmap;
                bitmap = Bitmap.createBitmap(rotatedBitmap, 0, 0,
                        bitmap.getWidth(), bitmap.getHeight(), matrix, true);
                rotatedBitmap.recycle();
            }
        }
        /* @} */
        /* SPRD:fix bug620386 move up the thumbnail at Interval module @{ */
        if (getModuleTpye() == DreamModule.INTERVAL_MODULE) {
            if (bitmap != null) {
                updateIntervalThumbnail(bitmap);
            }
        } else {
            mActivity.startPeekAnimation(bitmap);
        }
        /* @} */
    }

    private final class AutoFocusCallback implements CameraAFCallback {
        @Override
        public void onAutoFocus(boolean focused, CameraProxy camera) {
            SessionStatsCollector.instance().autofocusResult(focused);
            if (mPaused) {
                return;
            }

            mAutoFocusTime = System.currentTimeMillis() - mFocusStartTime;
            Log.i(TAG, "mAutoFocusTime = " + mAutoFocusTime + "ms   focused = "
                    + focused);
            setCameraState(IDLE);
            mFocusManager.onAutoFocus(focused, false);
        }
    }

    private final class AutoFocusMoveCallback implements CameraAFMoveCallback {
        @Override
        public void onAutoFocusMoving(boolean moving, CameraProxy camera) {
            // SPRD: ui check 210
            if (isAudioRecording()) return;
            mFocusManager.onAutoFocusMoving(moving);
            SessionStatsCollector.instance().autofocusMoving(moving);
        }
    }

    /**
     * This class is just a thread-safe queue for name,date holder objects.
     */
    public static class NamedImages {
        private final Vector<NamedEntity> mQueue;

        public NamedImages() {
            mQueue = new Vector<NamedEntity>();
        }

        public void nameNewImage(long date) {
            NamedEntity r = new NamedEntity();
            r.title = CameraUtil.instance().createJpegName(date);
            r.date = date;
            mQueue.add(r);
        }

        public NamedEntity getNextNameEntity() {
            synchronized (mQueue) {
                if (!mQueue.isEmpty()) {
                    return mQueue.remove(0);
                }
            }
            return null;
        }

        public static class NamedEntity {
            public String title;
            public long date;
        }
    }

    private void setCameraState(int state) {
        mCameraState = state;
        switch (state) {
        case PREVIEW_STOPPED:
        case SNAPSHOT_IN_PROGRESS:
        case SWITCHING_CAMERA:
            // TODO: Tell app UI to disable swipe
            break;
        case PhotoController.IDLE:
            // TODO: Tell app UI to enable swipe
            break;
        }
    }

    private void animateAfterShutter() {
        // Only animate when in full screen capture mode
        // i.e. If monkey/a user swipes to the gallery during picture taking,
        // don't show animation
        if (!mIsImageCaptureIntent) {
            mUI.animateFlash();
        }
    }

    @Override
    public boolean capture() {
        Log.i(TAG, "capture");
        // If we are already in the middle of taking a snapshot or the image
        // save request is full then ignore.
        if (mCameraDevice == null || mCameraState == SNAPSHOT_IN_PROGRESS
                || mCameraState == SWITCHING_CAMERA) {
            return false;
        }
        setCameraState(SNAPSHOT_IN_PROGRESS);

        mCaptureStartTime = System.currentTimeMillis();

        mPostViewPictureCallbackTime = 0;
        mJpegImageData = null;


        /* SPRD: add hide shutter animation after capture when HDR is on
        final boolean animateBefore = (mSceneMode == CameraCapabilities.SceneMode.HDR);
        if (animateBefore) {
            animateAfterShutter();
        }
        */

        Location loc = mActivity.getLocationManager().getCurrentLocation();
        CameraUtil.setGpsParameters(mCameraSettings, loc);
        boolean isHasEnteredBeauty = mDataModule.getBoolean(Keys.KEY_CAMERA_BEAUTY_ENTERED);
        if (!isHasEnteredBeauty) {
            setParametersHighISO(true);
        }
        mCameraDevice.applySettings(mCameraSettings);

        // Set JPEG orientation. Even if screen UI is locked in portrait, camera
        // orientation should
        // still match device orientation (e.g., users should always get
        // landscape photos while
        // capturing by putting device in landscape.)
        Characteristics info = mActivity.getCameraProvider()
                .getCharacteristics(mCameraId);
        int sensorOrientation = info.getSensorOrientation();
        int deviceOrientation = mAppController.getOrientationManager()
                .getDeviceOrientation().getDegrees();
        boolean isFrontCamera = info.isFacingFront();
        mJpegRotation = CameraUtil.getImageRotation(sensorOrientation,
                deviceOrientation, isFrontCamera);
        Log.i(TAG, " sensorOrientation = " + sensorOrientation
                + " ,deviceOrientation = " + deviceOrientation
                + " isFrontCamera = " + isFrontCamera);
        mCameraDevice.setJpegOrientation(mJpegRotation);
        if (isUcamBeautyCanBeUsed()) {
            mUI.setFilterMakeupButtonEnabled(false);// SPRD: fix bug474672
        }
        Log.i(TAG, "takePicture start!");
        if (mReceiver != null) {
            //mActivity.unregisterReceiver(mReceiver);
            mActivity.unRegisterMediaBroadcastReceiver();
            mReceiver = null;
        }

        mCameraDevice.takePicture(mHandler,
        /**
         * SPRD: fix bug462021 remove capture animation
         * 
         * @{ new ShutterCallback(!animateBefore),
         */
        new ShutterCallback(false),
        /**
         * @}
         */
        mRawPictureCallback, mPostViewPictureCallback, new JpegPictureCallback(loc));

        /**
         * SPRD: fix bug 473462 add for burst capture
         * mNamedImages.nameNewImage(mCaptureStartTime);
         */

        mFaceDetectionStarted = false;
        doCaptureSpecial();
        return true;
    }

    @Override
    public void setFocusParameters() {
        setCameraParameters(UPDATE_PARAM_PREFERENCE);
    }
    /* SPRD: All MUTEX OPERATION in onSettingsChanged function.
    private void updateSceneMode() {
        // If scene mode is set, we cannot set flash mode, white balance, and
        // focus mode, instead, we read it from driver. Some devices don't have
        // any scene modes, so we must check both NO_SCENE_MODE in addition to
        // AUTO to check where there is no actual scene mode set.
        if (!(CameraCapabilities.SceneMode.AUTO == mSceneMode ||
                CameraCapabilities.SceneMode.NO_SCENE_MODE == mSceneMode)) {
            overrideCameraSettings(mCameraSettings.getCurrentFlashMode(),
                    mCameraSettings.getCurrentFocusMode());
        }
    }

    private void overrideCameraSettings(CameraCapabilities.FlashMode flashMode,
            CameraCapabilities.FocusMode focusMode) {
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities.getStringifier();
        SettingsManager settingsManager = mActivity.getSettingsManager();
        if ((flashMode != null) && (!CameraCapabilities.FlashMode.NO_FLASH.equals(flashMode))) {
            String flashModeString = stringifier.stringify(flashMode);
            Log.v(TAG, "override flash setting to: " + flashModeString);
            settingsManager.set(mAppController.getCameraScope(), Keys.KEY_FLASH_MODE,
                    flashModeString);
        } else {
            Log.v(TAG, "skip setting flash mode on override due to NO_FLASH");
        }
        if (focusMode != null) {
            String focusModeString = stringifier.stringify(focusMode);
            Log.v(TAG, "override focus setting to: " + focusModeString);
            settingsManager.set(mAppController.getCameraScope(), Keys.KEY_FOCUS_MODE,
                    focusModeString);
        }
    }
    */

    /*SPRD:fix bug 620875 avoid 3d photo use burst mode @{ */
    public boolean checkCameraProxy() {
        boolean preferNewApi = GservicesHelper.useCamera2ApiThroughPortabilityLayer(
                mActivity.getContentResolver());
        return (preferNewApi == getCameraProvider().isNewApi())
                && mCameraId == getCameraProvider().getCurrentCameraId().getLegacyValue();
    }
    /* @} */

    @Override
    public void onCameraAvailable(CameraProxy cameraProxy) {
        Log.i(TAG, "onCameraAvailable");
        if (mPaused) {
            return;
        }

        /*SPRD:fix bug 620875 avoid 3d photo use burst mode @{ */
        if (!checkCameraProxy()) {
            resume();
            Log.d(TAG, "cameraProxy is error, resumed!");
            return;
        }
        /* @} */

        mCameraDevice = cameraProxy;
        initializeCapabilities();
        if (!mCameraDevice.getCapabilities().supports(CameraCapabilities.SceneMode.HDR)) {
            mUI.setButtonVisibility(ButtonManagerDream.BUTTON_HDR_DREAM,View.GONE);
        } else {
            mUI.setButtonVisibility(ButtonManagerDream.BUTTON_HDR_DREAM,View.VISIBLE);
        }
        // mCameraCapabilities is guaranteed to initialized at this point.
        mAppController.getCameraAppUI().showAccessibilityZoomUI(
                mCameraCapabilities.getMaxZoomRatio());

        // Reset zoom value index.
        mZoomValue = 1.0f;
        if (mFocusManager == null) {
            initializeFocusManager();
        }
        mFocusManager.updateCapabilities(mCameraCapabilities);

        // Do camera parameter dependent initialization.
        mCameraSettings = mCameraDevice.getSettings();
        // Set a default flash mode and focus mode
        if (mCameraSettings.getCurrentFlashMode() == null) {
            mCameraSettings.setFlashMode(CameraCapabilities.FlashMode.NO_FLASH);
        }
        if (mCameraSettings.getCurrentFocusMode() == null) {
            mCameraSettings.setFocusMode(CameraCapabilities.FocusMode.AUTO);
        }

        /**
         * SPRD: Fix bug 572631, optimize camera launch time,
         * Original Code
         *
        setCameraParameters(UPDATE_PARAM_ALL);
         */
        // Set a listener which updates camera parameters based
        // on changed settings.
        SettingsManager settingsManager = mActivity.getSettingsManager();
        settingsManager.addListener(this);
        /* SPRD:fix bug529235 hdr and flash are both on@{ */
        if (isHdr() && !isFlashOff()) {
            Log.e(TAG, "Mutex error, HDR && Flash are both On!");
            onSettingChanged(settingsManager, Keys.KEY_CAMERA_HDR);
        }
        /* @} */
        /**
         * SPRD: Fix bug 572631, optimize camera launch time
         * Original Code
         *
        mCameraPreviewParamsReady = true;
         */

        startPreview(true);

        onCameraOpened();

        mHardwareSpec = new HardwareSpecImpl(getCameraProvider(),
                mCameraCapabilities, mAppController.getCameraFeatureConfig(),
                isCameraFrontFacing());

        ButtonManager buttonManager = mActivity.getButtonManager();
        buttonManager.enableCameraButton();
        mBurstCount = getCurrentBurstCountFromSettings();

        mCameraAvailable = true;
    }

    @Override
    public void onCaptureCancelled() {
        mActivity.setResultEx(Activity.RESULT_CANCELED, new Intent());
        mActivity.finish();
    }

    @Override
    public void onCaptureRetake() {
        Log.i(TAG, "onCaptureRetake");
        if (mPaused) {
            return;
        }
        mUI.hidePostCaptureAlert();
        mUI.hideIntentReviewImageView();
        mAppController.getCameraAppUI().showModeOptions();// SPRD BUG: 402084
        setupPreview();
    }

    @Override
    public void onCaptureDone() {
        Log.i(TAG, "onCaptureDone");
        /*
         * SPRD: onCaptureDone could be called in monkey test while image capture
         * has not completed, then NullPointerException happens @{
         * orginal code
        if (mPaused) {
            return;
        }
         */

        if (mPaused || mJpegImageData == null) {
            return;
        }
        /* @} */


        byte[] data = mJpegImageData;

        if (mCropValue == null) {
            // First handle the no crop case -- just return the value. If the
            // caller specifies a "save uri" then write the data to its
            // stream. Otherwise, pass back a scaled down version of the bitmap
            // directly in the extras.
            if (mSaveUri != null) {
                OutputStream outputStream = null;
                try {
                    /*
                     * SPRD: fix bug 568162 If ContentResolver open URI failed,
                     * try to use new file to save data.@{
                     * original code
                     *
                     outputStream = mContentResolver.openOutputStream(mSaveUri);
                     */
                    final String scheme = mSaveUri.getScheme();
                    if ("content".equals(scheme)) {
                        outputStream = mContentResolver.openOutputStream(mSaveUri);
                    } else if ("file".equals(scheme)) {
                        File picFile = new File(mSaveUri.getPath());
                        File picParentFile = picFile.getParentFile();
                        if (picParentFile.exists() || picParentFile.mkdirs()) {
                            outputStream = new FileOutputStream(picFile);
                        } else {
                            throw new IOException("mkdirs failed: " + mSaveUri);
                        }
                    } else {
                        throw new IOException("unSupported URI: " + mSaveUri);
                    }
                    /* @} */
                    outputStream.write(data);
                    outputStream.close();

                    Log.i(TAG, "saved result to URI: " + mSaveUri);
                    mActivity.setResultEx(Activity.RESULT_OK);
                    mActivity.finish();
                } catch (IOException ex) {
                    onError();
                } finally {
                    CameraUtil.closeSilently(outputStream);
                }
            } else {
                ExifInterface exif = Exif.getExif(data);
                int orientation = Exif.getOrientation(exif);
                Bitmap bitmap = CameraUtil.makeBitmap(data, 50 * 1024);
                bitmap = CameraUtil.rotate(bitmap, orientation);
                Log.v(TAG, "inlined bitmap into capture intent result");
                mActivity.setResultEx(Activity.RESULT_OK, new Intent(
                        "inline-data").putExtra("data", bitmap));
                mActivity.finish();
            }
        } else {
            // Save the image to a temp file and invoke the cropper
            Uri tempUri = null;
            FileOutputStream tempStream = null;
            try {
                File path = mActivity.getFileStreamPath(sTempCropFilename);
                path.delete();
                tempStream = mActivity.openFileOutput(sTempCropFilename, 0);
                tempStream.write(data);
                tempStream.close();
                tempUri = Uri.fromFile(path);
                Log.i(TAG, "wrote temp file for cropping to: "
                        + sTempCropFilename);
            } catch (FileNotFoundException ex) {
                Log.w(TAG, "error writing temp cropping file to: "
                        + sTempCropFilename, ex);
                mActivity.setResultEx(Activity.RESULT_CANCELED);
                onError();
                return;
            } catch (IOException ex) {
                Log.w(TAG, "error writing temp cropping file to: "
                        + sTempCropFilename, ex);
                mActivity.setResultEx(Activity.RESULT_CANCELED);
                onError();
                return;
            } finally {
                CameraUtil.closeSilently(tempStream);
            }

            Bundle newExtras = new Bundle();
            if (mCropValue.equals("circle")) {
                newExtras.putString("circleCrop", "true");
            }
            if (mSaveUri != null) {
                Log.i(TAG, "setting output of cropped file to: " + mSaveUri);
                newExtras.putParcelable(MediaStore.EXTRA_OUTPUT, mSaveUri);
            } else {
                newExtras.putBoolean(CameraUtil.KEY_RETURN_DATA, true);
            }
            if (mActivity.isSecureCamera()) {
                newExtras.putBoolean(CameraUtil.KEY_SHOW_WHEN_LOCKED, true);
            }

            // TODO: Share this constant.
            final String CROP_ACTION = "com.android.camera.action.CROP";
            Intent cropIntent = new Intent(CROP_ACTION);

            cropIntent.setData(tempUri);
            cropIntent.putExtras(newExtras);
            Log.i(TAG, "starting CROP intent for capture");
            mActivity.startActivityForResult(cropIntent, REQUEST_CROP);
        }
    }

    @Override
    public void onShutterCoordinate(TouchCoordinate coord) {
        mShutterTouchCoordinate = coord;
    }

    /*
     * SPRD: fix bug 498954 If the function of flash is on and the camera is
     * counting down, the flash should not run here but before the capture.@{
     */
    private boolean isCountDownRunning() {
        if (mActivity != null) {
            int countDownDuration = 0;
            if(mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_COUNTDOWN_DURATION)){
                countDownDuration = mDataModuleCurrent
                        .getInt(Keys.KEY_COUNTDOWN_DURATION);
            }
            Log.i(TAG, "isCountDownRunning countDownDuration = "
                    + countDownDuration);
            if (countDownDuration > 0) {
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    /* @} */

    @Override
    public void onShutterButtonFocus(boolean pressed) {
        // Do nothing. We don't support half-press to focus anymore.
        /* SPRD: fix bug 473602 add for half-press @{ */
        Log.i(TAG, "onShutterButtonFocus pressed = " + pressed + ",isFlashOn="
                + isFlashOn());
        if (mPaused || (mCameraState == SWITCHING_CAMERA)
                || (mCameraState == PREVIEW_STOPPED) || !isShutterEnabled()
                || isFreezeFrameDisplay()
                || mCameraState == SNAPSHOT_IN_PROGRESS
                || (!isFlashOn() && !mCameraButtonClickedFlag && !isForceFocus())
                || isFocusModeFixed()
                /*
                 * SPRD: fix bug 498954 If the function of flash is on and the
                 * camera is counting down, the flash should not run here but
                 * before the capture.@{
                 */
                || (!isFlashOff() && isCountDownRunning())
                || (!mIsImageCaptureIntent && mActivity.getStorageSpaceBytes() <= Storage.LOW_STORAGE_THRESHOLD_BYTES)) {
            /* @} */
            if (!pressed) {
                mCaptureCount = 0;
            }
            return;
        }

        if (pressed) {
            mFocusManager.onShutterDown(CameraCapabilities.FocusMode.AUTO);
        } else {
            // for countdown mode, we need to postpone the shutter release
            // i.e. lock the focus during countdown.
            if (mCameraButtonClickedFlag) {
                mCameraButtonClickedFlag = false;
            }
            mCaptureCount = 0;
            if (!mUI.isCountingDown()) {
                mFocusManager.onShutterUp(CameraCapabilities.FocusMode.AUTO);
            }
        }
        /* @} */
    }

    /* SPRD:Fix bug 473602 CAF do not need AF only flash on @{ */
    private boolean isFlashOn() {
        String flashValues = mDataModuleCurrent.getString(Keys.KEY_FLASH_MODE);
        return "on".equals(flashValues);
    }

    public boolean isShutterEnabled() {
        return mAppController.isShutterEnabled();
    }

    /* @} */
    @Override
    public boolean isShutterClicked(){
        return !mAppController.isShutterEnabled() || mUI.isCountingDown();
    }

    /* @} */
    /*SPRD:fix bug529235 hdr and flash are both on@{*/
    private boolean isFlashOff() {
        String flashValues = mDataModuleCurrent.getString(Keys.KEY_FLASH_MODE);
        return "off".equals(flashValues);
    }
    /* @} */
    /**
     * SPRD:Fix bug 531648 CAF and flash not off need force Focus
     * @return
     */
    private boolean isForceFocus(){
        String flashValues = mDataModuleCurrent.getString(Keys.KEY_FLASH_MODE);
        String focusMode = mDataModule.getString(Keys.KEY_FOCUS_MODE);
        return !"off".equals(flashValues) && "continuous-picture".equals(focusMode);
    }

    /**
     * Add For Dream Camera in IntervalPhotoModule
     * 
     * @return true means will shutter again; false means won't shutter again.
     * @{
     */
    public boolean shutterAgain() {
        return false;
    }

    public boolean isInShutter() {
        return false;
    }

    /* @} */

    @Override
    public void onShutterButtonClick() {
        /**
         * SPRD: fix bug 476432 add for burst capture @{ if (mPaused ||
         * (mCameraState == SWITCHING_CAMERA) || (mCameraState ==
         * PREVIEW_STOPPED) || !mAppController.isShutterEnabled()) {
         * mVolumeButtonClickedFlag = false; return; }
         */
        Log.i(TAG, "onShutterButtonClick mPaused:" + mPaused + ",mCameraState:"
                + mCameraState + " isShutterEnabled=" + isShutterEnabled()
                + "isFreezeFrameDisplay=" + isFreezeFrameDisplay()
                + " mCaptureCount = " + mCaptureCount);
        if (mPaused
                || (mCameraState == SWITCHING_CAMERA)
                || (mCameraState == PREVIEW_STOPPED)
                || (!shutterAgain() && !isShutterEnabled()) // Add
                                                            // For
                                                            // Dream
                                                            // Camera
                                                            // in
                                                            // IntervalPhotoModule
                || isFreezeFrameDisplay()
                || mActivity.isFilmstripCoversPreview()
                || (++mCaptureCount) > 1
                || mUI.isCountingDown()
                || isAudioRecording()
                || mAppController.getCameraAppUI().isModeListOpen()) {// SPRD:Fix bug399745
            Log.i(TAG, "onShutterButtonClick is return !");
            if (mCaptureCount > 1) {
                mAppController.getCameraAppUI().enableModeOptions();
            }
            mVolumeButtonClickedFlag = false;
            if (mActivity.isFilmstripCoversPreview()
                    && mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_VGESTURE)
                    && mVGestureController.isStartTimerOn()) {
                Log.i(TAG, " isFilmstripCoversPreview and then startDetect");
                mVGestureController.startDetect();
            }
            return;
        }
        /**
         * @}
         */
        // Do not take the picture if there is not enough storage.
        if (!mIsImageCaptureIntent && mActivity.getStorageSpaceBytes() <= Storage.LOW_STORAGE_THRESHOLD_BYTES) {
            Log.i(TAG, "Not enough space or storage not ready. remaining="
                    + mActivity.getStorageSpaceBytes());
            mVolumeButtonClickedFlag = false;
            return;
        }

        if (isBurstCapturing()){
            Log.i(TAG, "burst capture is running, can not perform onShutterButtonClick!");
            mVolumeButtonClickedFlag = false;
            return;
        }
        //Sprd Fix Bug: 665197
        if(mUI.isZooming()){
            Log.i(TAG, "camera is zooming,can not perform onShutterbuttonClick");
            return;
        }
        /**
         * SPRD: fix bug 388808 Log.d(TAG, "onShutterButtonClick: mCameraState="
         * + mCameraState + " mVolumeButtonClickedFlag=" +
         * mVolumeButtonClickedFlag); mAppController.setShutterEnabled(false);
         */

        doSomethingWhenonShutterButtonClick();
        /* SPRD: fix bug 476432 add for burst capture @{ */
        if (!mIsImageCaptureIntent) {
            updateParametersBurstCount();
        }
        if (getContinuousCount() == 99) {
            updateParametersBurstPictureSize();
        }
        if (!mIsImageCaptureIntent) {// SPRD BUG:389377
            mContinuousCaptureCount = getContinuousCount();
        }
        Log.i(TAG, "onShutterButtonClick: mCameraState=" + mCameraState
                + " mVolumeButtonClickedFlag=" + mVolumeButtonClickedFlag
                + " mContinuousCaptureCount=" + mContinuousCaptureCount);
        /*
         * SPRD:fix bug498569 When capture with hdr, the key of ... can be
         * clicked@{
         */
        mAppController.getCameraAppUI().disableModeOptions();
        /* }@ */

        mAppController.getCameraAppUI().setSwipeEnabled(false);
        if (getContinuousCount() > 1) {
            mAppController.getCameraAppUI().hideModeOptions();
            mAppController.getCameraAppUI().hideCaptureIndicator();
        }
        /* @} */

        /*
         * SPRD:Fix bug 502060 close the function of Zoom only during the
         * Continuous Capture@{
         * SPRD:fix bug 596882/617069/648477 disable zoom when capture, if zoom in zsl capture, next capture will fail
         */
        mUI.enablePreviewOverlayHint(false);// SPRD:Fix bug 411062
        mUI.hideZoomProcessorIfNeeded();
        /* @} */

        int countDownDuration  = 0;
        if(mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_COUNTDOWN_DURATION)){
            countDownDuration = mDataModuleCurrent
                    .getInt(Keys.KEY_COUNTDOWN_DURATION);
        }

        mTimerDuration = countDownDuration;
        if (countDownDuration > 0) {
            // Start count down.
            mAppController.getCameraAppUI().transitionToCancel();
            mAppController.getCameraAppUI().hideModeOptions();
            mUI.startCountdown(countDownDuration);
            return;
        } else {
            /* SPRD: Bug385148 Disable shuterbutton after click */
            if (!isBurstCapture()) {
                mAppController.setShutterEnabled(false);
            }

            if (isUcamBeautyCanBeUsed()) {
                mUI.setFilterMakeupButtonEnabled(false);// SPRD:fix bug474672
            }
            mAppController.getCameraAppUI().setBottomPanelLeftRightClickable(false);
            focusAndCapture();
        }
    }

    private void focusAndCapture() {
        Log.i(TAG, "focusAndCapture");
        if (mSceneMode == CameraCapabilities.SceneMode.HDR) {
            mUI.setSwipingEnabled(false);
        }
        // If the user wants to do a snapshot while the previous one is still
        // in progress, remember the fact and do it after we finish the previous
        // one and re-start the preview. Snapshot in progress also includes the
        // state that autofocus is focusing and a picture will be taken when
        // focus callback arrives.
        if ((mFocusManager.isFocusingSnapOnFinish() || mCameraState == SNAPSHOT_IN_PROGRESS)) {
            if (!mIsImageCaptureIntent) {
                mSnapshotOnIdle = true;
            }
            return;
        }

        mSnapshotOnIdle = false;
        /*
         * SPRD: fix bug 498954 If the function of flash is on and the camera is
         * counting down, the flash should not run here but before the
         * capture.@{
         */
        /*
         * SPRD: fix bug 502445 And judgement of whether the phone support Focus
         * function or not.
         */
        if (!isFlashOff() && mTimerDuration > 0 && !isFocusModeFixed()) {
            mFocusManager.focusAfterCountDownFinishWhileFlashOn();
            return;
        }
        /* @} */
        mFocusManager.focusAndCapture(mCameraSettings.getCurrentFocusMode());
    }

    @Override
    public void onRemainingSecondsChanged(int remainingSeconds) {
        /* SPRD:fix bug 474665 @{ */
        if (remainingSeconds == 1 && mAppController.isPlaySoundEnable()) {
            mCountdownSoundPlayer.play(R.raw.timer_final_second, 0.6f);
        } else if ((remainingSeconds == 2 || remainingSeconds == 3) && mAppController.isPlaySoundEnable()) {
            mCountdownSoundPlayer.play(R.raw.timer_increment, 0.6f);
            /* @} */
            /**
             * SRPD: fix bug 388289 }
             */
        } else if (remainingSeconds == 0) {
            if (!mIsContinousCaptureFinish) {
                if (!isBurstCapture()) {
                    mAppController.setShutterEnabled(false);
                }
                if (isUcamBeautyCanBeUsed()) {
                    mUI.setFilterMakeupButtonEnabled(false);// SPRD:fix
                                                            // bug474672
                }
            }
        }
    }

    /* SPRD:fix bug 550298 @{ */
    private boolean isInSilentMode() {
        AudioManager mAudioManager = (AudioManager)mAppController.getAndroidContext().getSystemService(mAppController.getAndroidContext().AUDIO_SERVICE);
        int ringerMode = mAudioManager.getRingerMode();
        return (AudioManager.RINGER_MODE_SILENT == ringerMode || AudioManager.RINGER_MODE_VIBRATE == ringerMode);
    }
    /* @} */

    @Override
    public void onCountDownFinished() {
        mAppController.getCameraAppUI().transitionToCapture();
        /**
         * SPRD: fix bug 473462
         * mAppController.getCameraAppUI().showModeOptions(); if (mPaused) {
         * return; }
         */
        if (mPaused || mIsContinousCaptureFinish) {
            mAppController.getCameraAppUI().setSwipeEnabled(true);
            mAppController.getCameraAppUI().showModeOptions();
            mIsContinousCaptureFinish = false;
            return;
        }
        /*SPRD: fix bug 599542 add for touch capture and count down @{*/
        if (isOnSingleTapUp) {
            mFocusManager.onSingleTapUp(mTapUpX, mTapUpY);
            mTapUpX = 0;
            mTapUpY = 0;
            return;
        }
        /* @} */
        /* SPRD: fix bug 634947. In monkey test, mCameraState maybe wrongly changed to PREVIEW_STOPPED during countdown @{ */
        if (mCameraState == PREVIEW_STOPPED) {
            Log.i(TAG, "State is wrongly change to PREVIEW_STOPPED during countdown.");
            return;
        }
        /* @} */
        focusAndCapture();
    }

    @Override
    public void resume() {
        Log.i(TAG, "resume start!");
        mPaused = false;
        installIntentFilter();
        /**
         * SPRD: Fix bug 572631, optimize camera launch time
         * Original Code
         *
        mCountdownSoundPlayer.loadSound(R.raw.timer_final_second);
        mCountdownSoundPlayer.loadSound(R.raw.timer_increment);
         */
        //DataModuleManager.getInstance(mActivity).addListener(this);
        mDataModuleCurrent.addListener(this);
        if (mFocusManager != null) {
            // If camera is not open when resume is called, focus manager will
            // not be initialized yet, in which case it will start listening to
            // preview area size change later in the initialization.
            mAppController.addPreviewAreaSizeChangedListener(mFocusManager);
        }
        mAppController.addPreviewAreaSizeChangedListener(mUI);
        mActivity.getModuleLayoutRoot().findViewById(R.id.shutter_button)
                .setOnTouchListener(this);// SPRD:
                                          // fix
                                          // bug473462

        CameraProvider camProvider = mActivity.getCameraProvider();
        if (camProvider == null) {
            // No camera provider, the Activity is destroyed already.
            return;
        }

        OrientationManager orientationManager = mAppController
                .getOrientationManager();
        orientationManager.addOnOrientationChangeListener(this);
        mUI.onOrientationChanged(orientationManager,
                orientationManager.getDeviceOrientation());
        requestCameraOpen();
        mUI.intializeAIDetection(mDataModuleCurrent);// SPRD:Modify
                                                     // for ai
                                                     // detect

        mJpegPictureCallbackTime = 0;
        mZoomValue = 1.0f;

        mOnResumeTime = SystemClock.uptimeMillis();
        checkDisplayRotation();

        // If first time initialization is not finished, put it in the
        // message queue.
        if (!mFirstTimeInitialized) {
            mHandler.sendEmptyMessage(MSG_FIRST_TIME_INIT);
        } else {
            initializeSecondTime();
        }

        mHeadingSensor.activate();

        getServices().getRemoteShutterListener().onModuleReady(this);
        SessionStatsCollector.instance().sessionActive(true);

        /* SPRD:fix bug 473462 add for burst capture @{ */
        getServices().getMediaSaver().setListener(this);// SPRD BUG:388273

        mUI.onResume();
    }

    /**
     * @return Whether the currently active camera is front-facing.
     */
    private boolean isCameraFrontFacing() {
        return mAppController.getCameraProvider().getCharacteristics(mCameraId)
                .isFacingFront();
    }

    /**
     * The focus manager is the first UI related element to get initialized, and
     * it requires the RenderOverlay, so initialize it here
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
            ArrayList<CameraCapabilities.FocusMode> defaultFocusModes = new ArrayList<CameraCapabilities.FocusMode>();
            CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                    .getStringifier();
            for (String modeString : defaultFocusModesStrings) {
                CameraCapabilities.FocusMode mode = stringifier
                        .focusModeFromString(modeString);
                if (mode != null) {
                    defaultFocusModes.add(mode);
                }
            }
            mFocusManager = new FocusOverlayManager(mAppController,
                    defaultFocusModes, mCameraCapabilities, this, mMirror,
                    mActivity.getMainLooper(), mUI.getFocusRing(),this);
            mMotionManager = getServices().getMotionManager();
            if (mMotionManager != null) {
                mMotionManager.addListener(mFocusManager);
            }
        }
        mAppController.addPreviewAreaSizeChangedListener(mFocusManager);
    }

    /**
     * @return Whether we are resuming from within the lockscreen.
     */
    private boolean isResumeFromLockscreen() {
        String action = mActivity.getIntent().getAction();
        return (MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA.equals(action) || MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA_SECURE
                .equals(action));
    }

    @Override
    public void pause() {
        Log.i(TAG, "pause");
        mPaused = true;
        /* SPRD: Fix bug 474851, Add for new feature VGesture @{ */
        mHandler.removeMessages(MSG.CAMERA_SHUTTER);
        mHandler.removeMessages(MSG.CAMERA_FOCUS_CAPTURE);
        /* @} */
        /* SPRD: fix bug 473462 add burst capture @{ */
        if (mBurstMode){
            cancelBurstCapture();
            mBurstMode = false;
        }
        mUI.dismissBurstScreenHit();
        mUI.enablePreviewOverlayHint(true);// SPRD:Fix bug 467044
        mUI.hideZoomProcessorIfNeeded();//SPRD:fix bug626587
        /* @} */
        getServices().getRemoteShutterListener().onModuleExit();
        SessionStatsCollector.instance().sessionActive(false);

        mHeadingSensor.deactivate();
        mActivity.getCameraAppUI().dismissDialog(Keys.KEY_CAMERA_STORAGE_PATH);
        // Reset the focus first. Camera CTS does not guarantee that
        // cancelAutoFocus is allowed after preview stops.
        if (mCameraDevice != null && mCameraState != PREVIEW_STOPPED) {
            mCameraDevice.cancelAutoFocus();
        }
        removeAutoFocusMoveCallback();
        restoreToDefaultSettings();
        // If the camera has not been opened asynchronously yet,
        // and startPreview hasn't been called, then this is a no-op.
        // (e.g. onResume -> onPause -> onResume).
        stopPreview();
        cancelCountDown();
        if(getModuleTpye() == DreamModule.VGESTURE_MODULE){
            cancelVgestureCountDown(false);
        }
        dosomethingWhenPause();
        recoverToPreviewUI();
        /**
         * SPRD: Fix bug 572631, optimize camera launch time
         * Original Code
         *
        mCountdownSoundPlayer.unloadSound(R.raw.timer_final_second);
        mCountdownSoundPlayer.unloadSound(R.raw.timer_increment);
         */

        /* SPRD:Fix bug 499558 @{ */
        if (!mActivity.getCameraAppUI().isInIntentReview()) {
            mNamedImages = null;
            // If we are in an image capture intent and has taken
            // a picture, we just clear it in onPause.
            mJpegImageData = null;
        }
        /* @} */

        // Remove the messages and runnables in the queue.
        mHandler.removeCallbacksAndMessages(null);

        if (mMotionManager != null) {
            mMotionManager.removeListener(mFocusManager);
            mMotionManager = null;
        }

        closeCamera();
        mActivity.enableKeepScreenOn(false);
        mUI.onPause();

        if (mReceiver != null) {
            //mActivity.unregisterReceiver(mReceiver);
            mActivity.unRegisterMediaBroadcastReceiver();
            mReceiver = null;
        }

        mPendingSwitchCameraId = -1;
        if (mFocusManager != null) {
            mFocusManager.removeMessages();
        }
        getServices().getMemoryManager().removeListener(this);
        mAppController.removePreviewAreaSizeChangedListener(mFocusManager);
        mAppController.removePreviewAreaSizeChangedListener(mUI);
        mActivity.getModuleLayoutRoot().findViewById(R.id.shutter_button)
                .setOnTouchListener(null);// SPRD:
                                          // fix
                                          // bug
                                          // 473462

        SettingsManager settingsManager = mActivity.getSettingsManager();
        settingsManager.removeListener(this);

        mDataModuleCurrent.removeListener(this);
        mDataModule.removeListener(this);
        //DataModuleManager.getInstance(mActivity).removeListener(this);

        if (mAppController != null && mAppController.getOrientationManager() != null)
            mAppController.getOrientationManager().removeOnOrientationChangeListener(this);

        mAppController.getCameraAppUI().setBottomPanelLeftRightClickable(true);//Fix bug 649158/628424
        Log.i(TAG, "pause end!");
        /* @} */
    }

    private void recoverToPreviewUI() {
        mActivity.getCameraAppUI().updatePreviewUI(View.VISIBLE);
    }

    @Override
    public void destroy() {
        /*
         * SPRD: Fix bug 572631, optimize camera launch time
         * Original Android code:
        mCountdownSoundPlayer.release();
         */
        if (mCountdownSoundPlayer != null) {
            mCountdownSoundPlayer.unloadSound(R.raw.timer_final_second);
            mCountdownSoundPlayer.unloadSound(R.raw.timer_increment);
            mCountdownSoundPlayer.release();
            mCountdownSoundPlayer = null;
        }

        if (mCameraSound != null) {
            mCameraSound.release();
            mCameraSound = null;
        }
        /* @} */
        Log.i(TAG, "destroy");
    }

    @Override
    public void onLayoutOrientationChanged(boolean isLandscape) {
        setDisplayOrientation();
        // SPRD: Update freezeFrame display when change screen configuration
        if (sFreezeFrameControl != null && sFreezeFrameControl.mFreezeVisible)
            sFreezeFrameControl.updateFreezeUi();

        if (isUcamBeautyCanBeUsed()) {
            mUI.setButtonOrientation(CameraUtil.getDisplayRotation());// SPRD:fix
                                                                      // bug474672
                                                                      // add
                                                                      // for
                                                                      // ucam
        }
    }

    @Override
    public void updateCameraOrientation() {
        if (mDisplayRotation != CameraUtil.getDisplayRotation()) {
            setDisplayOrientation();
        }
    }

    private boolean canTakePicture() {
        Log.i(TAG, "canTakePicture");
        return isCameraIdle()
                && (mActivity.getStorageSpaceBytes() > Storage.LOW_STORAGE_THRESHOLD_BYTES);
    }

    @Override
    public void autoFocus() {
        if (mCameraDevice == null) {
            return;
        }
        Log.i(TAG, "Starting auto focus");
        mFocusStartTime = System.currentTimeMillis();
        mCameraDevice.autoFocus(mHandler, mAutoFocusCallback);
        SessionStatsCollector.instance().autofocusManualTrigger();
        setCameraState(FOCUSING);
        Log.i(TAG, "autoFocus end!");
    }

    @Override
    public void cancelAutoFocus() {
        if (mCameraDevice == null) {
            return;
        }
        mCameraDevice.cancelAutoFocus();
        setCameraState(IDLE);
        if (mFace != null && !mFace.equals(Keys.CAMERA_AI_DATECT_VAL_OFF)
                && isCameraIdle() /* && !isHdr() SPRD:Fix bug 459572*/) {
            // now face is not mutex with ai in UE's doc.
            startFaceDetection();
        }

        setCameraParameters(UPDATE_PARAM_PREFERENCE);
    }

    @Override
    public void onSingleTapUp(View view, int x, int y) {
        if (mPaused || mCameraDevice == null || !mFirstTimeInitialized
                || mCameraState == SNAPSHOT_IN_PROGRESS
                || mCameraState == SWITCHING_CAMERA
                || mCameraState == PREVIEW_STOPPED
                || sFreezeFrameControl.mFreezeVisible) {
            Log.i(TAG, "onSingleTapUp return!");
            return;
        }
        // Check if metering area or focus area is supported.
        /*SPRD:fix bug533976 add touch AE for FF@{
         * android original code
        if (!mFocusAreaSupported && !mMeteringAreaSupported) {
            return;
        }
        */

        /* SPRD:fix bug 600946 add touch capture for FF and front camera, Dream Camera test ui check 20 @{*/
        if(mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_TOUCHING_PHOTOGRAPH)
                && mFocusManager.getFocusMode(mCameraSettings.getCurrentFocusMode(),mDataModule.getString(Keys.KEY_FOCUS_MODE)) == CameraCapabilities.FocusMode.FIXED){
            touchandCapture();
            return;
        }
        /* @} */
        if (!mMeteringAreaSupported) {
            return;
        }
        /*@}*/

        /*SPRD: fix bug 599542 add for touch capture and count down @{*/
        if (isCountDownRunning() && mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_TOUCHING_PHOTOGRAPH)) {
            isOnSingleTapUp = true;
            mTapUpX = x;
            mTapUpY = y;
            touchandCapture();
            return;
        }
        /* @} */
        if (mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_TOUCHING_PHOTOGRAPH)) {
            isOnSingleTapUp = true;
            mUI.enablePreviewOverlayHint(false);// SPRD:Fix bug 653186
            mUI.hideZoomProcessorIfNeeded();
        }
        mFocusManager.onSingleTapUp(x, y);
    }

    private void touchandCapture(){
        if (mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_TOUCHING_PHOTOGRAPH)) {
            onShutterButtonClick();
            setCaptureCount(0);
        }
    }

    @Override
    public void touchCapture(){
        if (mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_TOUCHING_PHOTOGRAPH) && isOnSingleTapUp) {
            isOnSingleTapUp = false;
            capture();
        }
    }

    @Override
    public boolean onBackPressed() {

        if(sFreezeFrameControl.onBackPressed()){
            return true;
        }

        return mUI.onBackPressed();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
        case KeyEvent.KEYCODE_VOLUME_UP:
        case KeyEvent.KEYCODE_VOLUME_DOWN:
        case KeyEvent.KEYCODE_FOCUS:
            if (/* TODO: mActivity.isInCameraApp() && */mFirstTimeInitialized &&
                    !mActivity.getCameraAppUI().isInIntentReview()) {
                /* SPRD:Bug 535058 New feature: volume */
                int volumeStatus = getVolumeControlStatus(mActivity);
                if (volumeStatus == Keys.shutter || keyCode == KeyEvent.KEYCODE_FOCUS) {
                    // SPRD: Freeze display don't action the key_down event
                    if (sFreezeFrameControl != null &&
                            sFreezeFrameControl.mFreezeVisible ||
                            mBurstNotShowShutterButton) {
                        return true;
                    }
                    if (event.getRepeatCount() == 0) {
                        /* SPRD:fix bug 473602 add for half-press @{ */
                        if (keyCode == KeyEvent.KEYCODE_FOCUS) {
                            mCameraButtonClickedFlag = true;
                        }
                        /* @} */
                        if (keyCode == KeyEvent.KEYCODE_FOCUS) {
                            if (!(mUI.isCountingDown() && mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_TOUCHING_PHOTOGRAPH))) {
                                onShutterButtonFocus(true);
                            }
                        } else {
                            if (!mBurstMode) {
                                handleActionDown(keyCode);
                            }
                        }
                    }
                    return true;
                } else if (volumeStatus == Keys.zoom) {
                    if(mDataModuleCurrent != null &&
                            mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_DREAM_ZOOM_ENABLE_PHOTO_MODULE) &&
                            !mDataModuleCurrent.getBoolean(Keys.KEY_DREAM_ZOOM_ENABLE_PHOTO_MODULE)){
                        ToastUtil.showToast(mActivity, R.string.current_module_does_not_support_zoom_change,
                                Toast.LENGTH_LONG);
                        return true;
                    }
                    if (mBurstMode || !mAppController.isShutterEnabled() || mUI.isCountingDown()) {//SPRD:fix bug655910
                        return true;
                    }
                    float zoomValue;
                    if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
                        zoomValue=increaseZoomValue(mZoomValue);
                    } else {
                        zoomValue=reduceZoomValue(mZoomValue);
                    }
                    onZoomChanged(zoomValue);
                    mUI.setPreviewOverlayZoom(mZoomValue);
                    return true;
                } else if (volumeStatus == Keys.volume) {
                    return false;
                }
            }
            return false;
        case KeyEvent.KEYCODE_CAMERA:
            // SPRD: Freeze display don't action the key_camera event
            if (sFreezeFrameControl != null
                    && sFreezeFrameControl.mFreezeVisible
                    || mBurstNotShowShutterButton) {
                return true;
            }
            if (mFirstTimeInitialized && event.getRepeatCount() == 0) {
                if (!mBurstMode) {
                    handleActionDown(keyCode);
                }
            }
            return true;
        case KeyEvent.KEYCODE_DPAD_CENTER:
            // If we get a dpad center event without any focused view, move
            // the focus to the shutter button and press it.
            if (mFirstTimeInitialized && event.getRepeatCount() == 0) {
                // Start auto-focus immediately to reduce shutter lag. After
                // the shutter button gets the focus, onShutterButtonFocus()
                // will be called again but it is fine.
                onShutterButtonFocus(true);
            }
            return true;
        }
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        switch (keyCode) {
        case KeyEvent.KEYCODE_VOLUME_UP:
        case KeyEvent.KEYCODE_VOLUME_DOWN:
        case KeyEvent.KEYCODE_CAMERA:
            int volumeStatus = getVolumeControlStatus(mActivity);
            /*
             * SPRD:fix bug518054 ModeListView is appear when begin to capture using volume
             * key@{
             */
            mActivity.getCameraAppUI().hideModeList();
            /* }@ */
            if (/* mActivity.isInCameraApp() && */mFirstTimeInitialized &&
                    !mActivity.getCameraAppUI().isInIntentReview()) {
                /* SPRD:Bug 535058 New feature: volume */
                if (volumeStatus == Keys.shutter || keyCode == KeyEvent.KEYCODE_CAMERA) {
                    if(getModuleTpye() == DreamModule.INTERVAL_MODULE && !canShutter()){
                        return true;
                    }
                    // SPRD: Freeze display don't action the key_up event
                    if (sFreezeFrameControl != null &&
                            sFreezeFrameControl.mFreezeVisible ||
                            mBurstNotShowShutterButton) {
                        return true;
                    }
                    if (mUI.isCountingDown()) {
                        cancelCountDown();
                        mBurstCaptureType = -1;
                    } else if (getModuleTpye() == DreamModule.VGESTURE_MODULE
                            && mVGestureController != null
                            && mVGestureController.isStartTimerOn()) {
                        cancelVgestureCountDown(true);
                    } else {
                        mVolumeButtonClickedFlag = true;
                        /*
                         * SPRD:fix bug499275 Front DC can not capture through
                         * the Bluetooth@{
                         */
                        mActivity.getCameraAppUI().closeModeOptions();
                        /* }@ */
                        if (mBurstCaptureType == keyCode){
                            handleActionUp();
                        } else if (!mIsImageCaptureIntent) {
                            Log.e(TAG, "burst capture is triggered by other key, can not cancel burst capture!");
                            return true;
                        }
                        if (!mBurstMode){
                            onShutterButtonFocus(true);
                            onShutterButtonClick();
                        }
                    }
                    /* SPRD:fix bug 496864 Can not capture twice using key of volumn @{ */
                    onShutterButtonFocus(false);
                    /* }@ */
                    return true;
                } else if (volumeStatus == Keys.zoom) {
                    mUI.hideZoomUI();
                    return true;
                } else if (volumeStatus == Keys.volume) {
                    return false;
                }
            }
            return false;
        case KeyEvent.KEYCODE_FOCUS:
            if(getModuleTpye() == DreamModule.INTERVAL_MODULE && !canShutter()){
                return true;
            }

            if (mFirstTimeInitialized) {
                // SPRD: Freeze display don't action the KEYCODE_FOCUS event
                if (sFreezeFrameControl != null
                        && sFreezeFrameControl.mFreezeVisible) {
                    return true;
                }

                onShutterButtonFocus(false);
            }
            return true;
        }
        return false;
    }

    protected void closeCamera() {
        Log.i(TAG, "closeCamera will! mCameraDevice=" + mCameraDevice);
        mCameraAvailable = false;
        /**
         * SPRD: fix bug 434570 if (mCameraDevice != null) {
         * stopFaceDetection(); mCameraDevice.setZoomChangeListener(null);
         * mCameraDevice.setFaceDetectionCallback(null, null);
         * mFaceDetectionStarted = false;
         * mActivity.getCameraProvider().releaseCamera
         * (mCameraDevice.getCameraId()); mCameraDevice = null;
         * setCameraState(PREVIEW_STOPPED); mFocusManager.onCameraReleased(); }
         */

        /*
         * SPRD: fix bug 496029 If turn off the screen during the Continuous
         * Capture, the mContinuousCaptureCount should set to be 0. @{
         */
        mContinuousCaptureCount = 0;
        /* @} */

        if (mCameraDevice == null) {
            Log.i(TAG, "already stopped.");
            Log.i(TAG, "closeCamera end!");
            return;
        }
        stopFaceDetection();
        doCloseCameraSpecial(mActivity, mCameraDevice);
        mCameraDevice.setZoomChangeListener(null);
        // already call by stopFaceDetection
        // mCameraDevice.setFaceDetectionCallback(null, null);
        // mFaceDetectionStarted = false;
        mActivity.getCameraProvider().releaseCamera(mCameraDevice.getCameraId());
        mCameraDevice = null;
        setCameraState(PREVIEW_STOPPED);

        if (mFocusManager != null)
            mFocusManager.onCameraReleased();

        Log.i(TAG, "closeCamera end!");
    }

    private void setDisplayOrientation() {
        mDisplayRotation = CameraUtil.getDisplayRotation();
        Characteristics info = mActivity.getCameraProvider()
                .getCharacteristics(mCameraId);
        mDisplayOrientation = info.getPreviewOrientation(mDisplayRotation);
        mUI.setDisplayOrientation(mDisplayOrientation);
        if (mFocusManager != null) {
            mFocusManager.setDisplayOrientation(mDisplayOrientation);
        }
        // Change the camera display orientation
        if (mCameraDevice != null) {
            mCameraDevice.setDisplayOrientation(mDisplayRotation);
        }
        Log.v(TAG, "setDisplayOrientation (screen:preview) " + mDisplayRotation
                + ":" + mDisplayOrientation);
    }

    /** Only called by UI thread. */
    private void setupPreview() {
        Log.i(TAG, "setupPreview");
        mFocusManager.resetTouchFocus();
        if (mCameraDevice != null && mCameraSettings != null) {
            setParametersHighISO(false);
            mCameraDevice.applySettings(mCameraSettings);
        }
        startPreview(true);
    }

    /**
     * Returns whether we can/should start the preview or not.
     */
    private boolean checkPreviewPreconditions() {
        if (mPaused) {
            return false;
        }

        if (mCameraDevice == null) {
            Log.w(TAG, "startPreview: camera device not ready yet.");
            return false;
        }

        SurfaceTexture st = mActivity.getCameraAppUI().getSurfaceTexture();
        if (st == null) {
            Log.w(TAG, "startPreview: surfaceTexture is not ready.");
            return false;
        }

        /**
         * SPRD: Fix bug 572631, optimize camera launch time,
         * Original Code
         *
        if (!mCameraPreviewParamsReady) {
            Log.w(TAG, "startPreview: parameters for preview is not ready.");
            return false;
        }
         */
        return true;
    }

    /**
     * The start/stop preview should only run on the UI thread.
     */
    private void startPreview(boolean optimize) {
        if (mCameraDevice == null) {
            Log.i(TAG, "attempted to start preview before camera device");
            // do nothing
            return;
        }

        if (!checkPreviewPreconditions()) {
            return;
        }

        /* SPRD: add for bug 380597/642171: switch camera preview has a frame error @{ */
        mActivity.getCameraAppUI().resetPreview(!optimize);
        /* @} */
        /*SPRD: fix bug606414 burst mode not need to set display orientation, because it will modify jpeg orientation @ */
        if (!mBurstMode) {
            setDisplayOrientation();
        }
        /* @}*/

        if (!mSnapshotOnIdle) {
            // If the focus mode is continuous autofocus, call cancelAutoFocus
            // to resume it because it may have been paused by autoFocus call.
            if (mFocusManager.getFocusMode(mCameraSettings
                    .getCurrentFocusMode(),mDataModule.getString(Keys.KEY_FOCUS_MODE)) == CameraCapabilities.FocusMode.CONTINUOUS_PICTURE) {
                mCameraDevice.cancelAutoFocus();
            }
            mFocusManager.setAeAwbLock(false); // Unlock AE and AWB.
        }

        // Nexus 4 must have picture size set to > 640x480 before other
        // parameters are set in setCameraParameters, b/18227551. This call to
        // updateParametersPictureSize should occur before setCameraParameters
        // to address the issue.
        updateParametersPictureSize();

        setCameraParameters(UPDATE_PARAM_ALL);

        if(optimize){
            mCameraDevice.setPreviewTexture(mActivity.getCameraAppUI()
                    .getSurfaceTexture());
        }else {
            mCameraDevice.setPreviewTextureWithoutOptimize(mActivity.getCameraAppUI()
                    .getSurfaceTexture());
        }

        Log.i(TAG, "startPreview");
        // If we're using API2 in portability layers, don't use
        // startPreviewWithCallback()
        // b/17576554
        CameraAgent.CameraStartPreviewCallback startPreviewCallback = new CameraAgent.CameraStartPreviewCallback() {
            @Override
            public void onPreviewStarted() {
                mFocusManager.onPreviewStarted();
                PhotoModule.this.onPreviewStarted();
                SessionStatsCollector.instance().previewActive(true);
                if (mSnapshotOnIdle) {
                    mHandler.post(mDoSnapRunnable);
                }
            }
        };
        /**
         * SPRD: Change for New Feature VGesture
         * original code
         * @{
        if (GservicesHelper.useCamera2ApiThroughPortabilityLayer(mActivity
                .getContentResolver())) {
           mCameraDevice.startPreview();
           startPreviewCallback.onPreviewStarted();
        } else {
           mCameraDevice.startPreviewWithCallback(
                   new Handler(Looper.getMainLooper()), startPreviewCallback);
        }
        */

        doStartPreviewSpecial(isCameraIdle(), isHdr(), mActivity, mCameraDevice, mHandler,
                mDisplayOrientation, isCameraFrontFacing(), mUI.getRootView(), mCameraSettings);//SPRD:fix bug624871
        doStartPreview(startPreviewCallback,mCameraDevice);
        mAppController.getCameraAppUI().setBottomPanelLeftRightClickable(true);
        Log.i(TAG, "startPreview end!");
    }

    @Override
    public void stopPreview() {
        Log.i(TAG, "stopPreview start!mCameraDevice=" + mCameraDevice);
        if (mCameraDevice != null && mCameraState != PREVIEW_STOPPED) {
            Log.i(TAG, "stopPreview");
            mCameraDevice.stopPreview();
            mFaceDetectionStarted = false;
        }
        setCameraState(PREVIEW_STOPPED);
        if (mFocusManager != null) {
            mFocusManager.onPreviewStopped();
        }
        SessionStatsCollector.instance().previewActive(false);
        Log.i(TAG, "stopPreview end!");
    }

    /*
     * SPRD: onSettingChanged method is mainly do the Mutex in the preview
     * screen @{
     */
    @Override
    public void onSettingChanged(SettingsManager settingsManager, String key) {
//        Log.d(TAG, "onSettingChanged key=" + key);
//        if (key.equals(Keys.KEY_FLASH_MODE)) {
//            updateParametersFlashMode();
//        }
//        if (key.equals(Keys.KEY_CAMERA_HDR)) {
//            if (settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL,
//                    Keys.KEY_CAMERA_HDR)) {
//                Log.d(TAG, "onSettingChanged hdr on");
//                // HDR is on.
//                int showToast = 0;
//                // SPRD: HDR MUTEX WITH FLASH BEGIN 1/2
//                mAppController.getButtonManager().disableButton(
//                        ButtonManager.BUTTON_FLASH);
//                mFlashModeBeforeSceneMode = settingsManager.getString(
//                        mAppController.getCameraScope(), Keys.KEY_FLASH_MODE);
//                settingsManager.set(mAppController.getCameraScope(),
//                        Keys.KEY_FLASH_MODE + PREF_BEFORE,
//                        mFlashModeBeforeSceneMode);
//                settingsManager.set(mAppController.getCameraScope(),
//                        Keys.KEY_FLASH_MODE, "off");
//                updateParametersFlashMode();
//                // SPRD: HDR MUTEX WITH FLASH END 1/2
//
//                // SPRD: HDR MUTEX WITH EXPOSURE BEGIN 1/2
//                if (settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_EXPOSURE_COMPENSATION_ENABLED)) {
//                    Log.d(TAG,
//                            "onSettingChanged KEY_EXPOSURE_COMPENSATION_ENABLED ");
//                    mAppController.getButtonManager().disableButton(
//                            ButtonManager.BUTTON_EXPOSURE_COMPENSATION);
//                    mExposureCompensationBefore = settingsManager.getString(
//                            mAppController.getCameraScope(), Keys.KEY_EXPOSURE);
//                    Log.d(TAG, "onSettingChanged mExposureCompensationBefore="
//                            + mExposureCompensationBefore);
//                    settingsManager.set(mAppController.getCameraScope(),
//                            Keys.KEY_EXPOSURE + PREF_BEFORE,
//                            mExposureCompensationBefore);
//                    setExposureCompensation(0);
//                }
//                // SPRD: HDR MUTEX WITH EXPOSURE END 1/2
//
//                // SPRD: HDR MUTEX WITH AeLock BEGIN 1/2
//                if (mAeLockSupported) {
//                    mAutoExposureLockBefore = mFocusManager.getAeAwbLock();
//                    mCameraSettings.setAutoExposureLock(false);
//                }
//                // SPRD: HDR MUTEX WITH AeLock END 1/2
//
//                // SPRD: HDR MUTEX WITH CONTINUE_CAPTURE BEGIN
//                mCountineCaptureBefore = settingsManager.getString(
//                        SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_CAMERA_CONTINUE_CAPTURE);
//                Log.d(TAG, "onSettingChanged mCountineCaptureBefore="
//                        + mCountineCaptureBefore);
//                settingsManager.set(
//                        SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_CAMERA_CONTINUE_CAPTURE,
//                        mActivity.getAndroidContext().getString(
//                                R.string.pref_camera_burst_entry_defaultvalue));
//                updateParametersBurstCount();
//                if (!mActivity
//                        .getAndroidContext()
//                        .getString(
//                                R.string.pref_camera_burst_entry_defaultvalue)
//                        .equals(mCountineCaptureBefore)) {
//                    showToast++;
//                }
//                // SPRD: HDR MUTEX WITH CONTINUE_CAPTURE END
//
//                // SPRD: HDR MUTEX WITH WHITE BALANCE BEGIN
//                mWhiteBalanceBefore = settingsManager.getString(
//                        SettingsManager.SCOPE_GLOBAL, Keys.KEY_WHITE_BALANCE);
//                Log.d(TAG,
//                        "onSettingChanged mWhiteBalanceBefore="
//                                + mWhiteBalanceBefore
//                                + ",DEFAULT"
//                                + mActivity
//                                        .getAndroidContext()
//                                        .getString(
//                                                R.string.pref_camera_whitebalance_default));
//                settingsManager.set(
//                        SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_WHITE_BALANCE,
//                        mActivity.getAndroidContext().getString(
//                                R.string.pref_camera_whitebalance_default));
//                updateParametersWhiteBalance();
//                if (!mActivity.getAndroidContext()
//                        .getString(R.string.pref_camera_whitebalance_default)
//                        .equals(mWhiteBalanceBefore)) {
//                    showToast++;
//                }
//                // SPRD: HDR MUTEX WITH WHITE BALANCE BEGIN
//
//                // SPRD: HDR MUTEX WITH COLOR EFFECT BEGIN
//                mColorEffectBefore = settingsManager.getString(
//                        SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_CAMERA_COLOR_EFFECT);
//                Log.d(TAG, "onSettingChanged mColorEffectBefore="
//                        + mColorEffectBefore);
//                settingsManager
//                        .set(SettingsManager.SCOPE_GLOBAL,
//                                Keys.KEY_CAMERA_COLOR_EFFECT,
//                                mActivity
//                                        .getAndroidContext()
//                                        .getString(
//                                                R.string.pref_camera_color_effect_entry_value_none));
//                updateParametersColorEffect();
//                if (!mActivity
//                        .getAndroidContext()
//                        .getString(
//                                R.string.pref_camera_color_effect_entry_value_none)
//                        .equals(mColorEffectBefore)) {
//                    showToast++;
//                }
//                // SPRD: HDR MUTEX WITH COLOR EFFECT BEGIN
//
//                // SPRD: HDR MUTEX WITH ISO BEGIN
//                mISOBefore = settingsManager.getString(
//                        SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_ISO);
//                Log.d(TAG, "onSettingChanged mISOBefore=" + mISOBefore);
//                settingsManager.set(SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_CAMERA_ISO, mActivity.getAndroidContext()
//                                .getString(R.string.pref_entry_value_auto));
//                updateParametersISO();
//                if (!mActivity.getAndroidContext()
//                        .getString(R.string.pref_entry_value_auto)
//                        .equals(mISOBefore)) {
//                    showToast++;
//                }
//                // SPRD: HDR MUTEX WITH ISO END
//
//                // SPRD: HDR MUTEX WITH CONTRAST BEGIN
//                mContrastBefore = settingsManager.getString(
//                        SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_CONTRAST);
//                Log.d(TAG, "onSettingChanged mContrastBefore="
//                        + mContrastBefore);
//                settingsManager.set(
//                        SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_CAMERA_CONTRAST,
//                        mActivity.getAndroidContext().getString(
//                                R.string.pref_contrast_entry_defaultvalue));
//                updateParametersContrast();
//                if (!mActivity.getAndroidContext()
//                        .getString(R.string.pref_contrast_entry_defaultvalue)
//                        .equals(mContrastBefore)) {
//                    showToast++;
//                }
//                // SPRD: HDR MUTEX WITH CONTRAST END
//
//                // SPRD: HDR MUTEX WITH SATURATION BEGIN
//                mSaturationBefore = settingsManager.getString(
//                        SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_CAMERA_SATURATION);
//                Log.d(TAG, "onSettingChanged mSaturationBefore="
//                        + mSaturationBefore);
//                settingsManager.set(
//                        SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_CAMERA_SATURATION,
//                        mActivity.getAndroidContext().getString(
//                                R.string.pref_saturation_entry_defaultvalue));
//                updateParametersSaturation();
//                if (!mActivity.getAndroidContext()
//                        .getString(R.string.pref_saturation_entry_defaultvalue)
//                        .equals(mSaturationBefore)) {
//                    showToast++;
//                }
//                // SPRD: HDR MUTEX WITH SATURATION END
//
//                // SPRD: HDR MUTEX WITH BRIGHTNESS BEGIN
//                mBrightnessBefore = settingsManager.getString(
//                        SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_CAMERA_BRIGHTNESS);
//                Log.d(TAG, "onSettingChanged mBrightnessBefore="
//                        + mBrightnessBefore);
//                settingsManager.set(
//                        SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_CAMERA_BRIGHTNESS,
//                        mActivity.getAndroidContext().getString(
//                                R.string.pref_brightness_entry_defaultvalue));
//                updateParametersBrightness();
//                if (!mActivity.getAndroidContext()
//                        .getString(R.string.pref_brightness_entry_defaultvalue)
//                        .equals(mBrightnessBefore)) {
//                    showToast++;
//                }
//                // SPRD: HDR MUTEX WITH BRIGHTNESS END
//                Log.d(TAG, "onSettingChanged showToast = " + showToast);
//                if (showToast != 0) {
//                    Toast.makeText(
//                            mActivity,
//                            mActivity.getResources().getString(
//                                    R.string.hdr_mutex), Toast.LENGTH_LONG)
//                            .show();
//                    isToastExit = true;
//                }
//                // SPRD: MUTEX END
//            } else {
//                Log.d(TAG, "onSettingChanged hdr off");
//                // SPRD: HDR MUTEX WITH FLASH BEGIN 2/2
//                mFlashModeBeforeSceneMode = settingsManager.getString(
//                        mAppController.getCameraScope(), Keys.KEY_FLASH_MODE
//                                + PREF_BEFORE, null);
//                settingsManager.set(mAppController.getCameraScope(),
//                        Keys.KEY_FLASH_MODE + PREF_BEFORE, null);
//                Log.d(TAG, "onSettingChanged mFlashModeBeforeSceneMode="
//                        + mFlashModeBeforeSceneMode);
//                if (mFlashModeBeforeSceneMode != null) {
//                    settingsManager.set(mAppController.getCameraScope(),
//                            Keys.KEY_FLASH_MODE, mFlashModeBeforeSceneMode);
//                    updateParametersFlashMode();
//                    mFlashModeBeforeSceneMode = null;
//                }
//                mAppController.getButtonManager().enableButton(
//                        ButtonManager.BUTTON_FLASH);
//                // SPRD: HDR MUTEX WITH FLASH BEGIN 2/2
//
//                // SPRD: HDR MUTEX WITH EXPOSURE BEGIN 2/2
//                if (settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_EXPOSURE_COMPENSATION_ENABLED)) {
//                    mExposureCompensationBefore = settingsManager.getString(
//                            mAppController.getCameraScope(), Keys.KEY_EXPOSURE
//                                    + PREF_BEFORE, null);
//                    Log.d(TAG, "onSettingChanged mExposureCompensationBefore="
//                            + mExposureCompensationBefore);
//                    settingsManager.set(mAppController.getCameraScope(),
//                            Keys.KEY_EXPOSURE + PREF_BEFORE, null);
//                    if (mExposureCompensationBefore != null) {
//                        settingsManager.set(mAppController.getCameraScope(),
//                                Keys.KEY_EXPOSURE, mExposureCompensationBefore);
//                        updateParametersExposureCompensation();
//                        mExposureCompensationBefore = null;
//                    }
//                    mAppController.getButtonManager().enableButton(
//                            ButtonManager.BUTTON_EXPOSURE_COMPENSATION);
//                }
//                // SPRD: HDR MUTEX WITH EXPOSURE END 2/2
//
//                // SPRD: HDR MUTEX WITH AeLock BEGIN 2/2
//                if (mAeLockSupported) {
//                    mCameraSettings
//                            .setAutoExposureLock(mAutoExposureLockBefore);
//                    mAutoExposureLockBefore = false;
//                }
//                // SPRD: HDR MUTEX WITH AeLock END 2/2
//            }
//        }
//
//        // FLASH
//        if (key.equals(Keys.KEY_FLASH_MODE)
//                && !"off".equals(settingsManager.getString(
//                        mAppController.getCameraScope(), Keys.KEY_FLASH_MODE))) {
//            Log.d(TAG, "onSettingsChanged flash is not off ");
//            int showFlashToast = 0;
//            // SPRD: FLASH MUTEX WITH SCENE MODE BEGIN
//            String sceneModeBefore = settingsManager.getString(
//                    SettingsManager.SCOPE_GLOBAL, Keys.KEY_SCENE_MODE);
//            Log.d(TAG, " sceneModeBefore=" + sceneModeBefore);
//            if (!"auto".equals(sceneModeBefore)) {
//                settingsManager.set(
//                        SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_SCENE_MODE,
//                        mCameraCapabilities.getStringifier().stringify(
//                                CameraCapabilities.SceneMode.AUTO));
//                updateParametersSceneMode();
//                showFlashToast++;
//            }
//            // SPRD: FLASH MUTEX WITH SCENE MODE END
//
//            // SPRD: FLASH MUTEX WITH CONTINUE_CAPTURE BEGIN
//            String countineCaptureBefore = settingsManager.getString(
//                    SettingsManager.SCOPE_GLOBAL,
//                    Keys.KEY_CAMERA_CONTINUE_CAPTURE);
//            Log.d(TAG,
//                    " countineCaptureBefore="
//                            + countineCaptureBefore
//                            + ","
//                            + mActivity
//                                    .getAndroidContext()
//                                    .getString(
//                                            R.string.pref_camera_burst_entry_defaultvalue));
//            if (!mActivity.getAndroidContext()
//                    .getString(R.string.pref_camera_burst_entry_defaultvalue)
//                    .equals(countineCaptureBefore)) {
//                settingsManager.set(
//                        SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_CAMERA_CONTINUE_CAPTURE,
//                        mActivity.getAndroidContext().getString(
//                                R.string.pref_camera_burst_entry_defaultvalue));
//                updateParametersBurstCount();
//                showFlashToast++;
//            }
//            // SPRD: FLASH MUTEX WITH CONTINUE_CAPTURE END
//            Log.d(TAG, "showFlashToast = " + showFlashToast);
//            if (showFlashToast != 0) {
//                Toast.makeText(
//                        mActivity,
//                        mActivity.getResources()
//                                .getString(R.string.flash_mutex),
//                        Toast.LENGTH_LONG).show();
//            }
//        }// FLASH END
//
//        // SMILE MUTEX COUNTDOWN BEGIN
//        if (key.equals(Keys.KEY_COUNTDOWN_DURATION)) {
//            String mface = settingsManager.getString(
//                    SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_AI_DATECT);
//            Log.d(TAG, "mutex mface=" + mface);
//            if ("smile".equals(mface)) {
//                settingsManager.set(SettingsManager.SCOPE_GLOBAL,
//                        Keys.KEY_CAMERA_AI_DATECT,
//                        Keys.CAMERA_AI_DATECT_VAL_OFF);
//                mFaceDetectionStarted = true;
//                stopFaceDetection();
//                setCaptureCount(0);
//                Toast.makeText(
//                        mActivity,
//                        mActivity.getResources().getString(
//                                R.string.count_down_mutex), Toast.LENGTH_LONG)
//                        .show();
//            }
//        }
//        // SMILE MUTEX COUNTDOWN END
//        if (mCameraDevice != null) {
//            mCameraDevice.applySettings(mCameraSettings);
//        }
    }

    /* @} */

    private void updateCameraParametersInitialize() {
        // Reset preview frame rate to the maximum because it may be lowered by
        // video camera application.
        int[] fpsRange = CameraUtil
                .getPhotoPreviewFpsRange(mCameraCapabilities);
        if (fpsRange != null && fpsRange.length > 0) {
            mCameraSettings.setPreviewFpsRange(fpsRange[0], fpsRange[1]);
        }

        mCameraSettings.setRecordingHintEnabled(false);

        if (mCameraCapabilities
                .supports(CameraCapabilities.Feature.VIDEO_STABILIZATION)) {
            mCameraSettings.setVideoStabilization(false);
        }
    }

    private void updateCameraParametersZoom() {
        // Set zoom.
        if (mCameraCapabilities.supports(CameraCapabilities.Feature.ZOOM)) {
            mCameraSettings.setZoomRatio(mZoomValue);
        }
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    private void setAutoExposureLockIfSupported() {
        if (mAeLockSupported) {
            mCameraSettings.setAutoExposureLock(mFocusManager.getAeAwbLock());
        }
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    private void setAutoWhiteBalanceLockIfSupported() {
        if (mAwbLockSupported) {
            mCameraSettings.setAutoWhiteBalanceLock(mFocusManager
                    .getAeAwbLock());
        }
    }

    private void setFocusAreasIfSupported() {
        if (mFocusAreaSupported) {
            mCameraSettings.setFocusAreas(mFocusManager.getFocusAreas());
        }
    }

    private void setMeteringAreasIfSupported() {
        if (mMeteringAreaSupported) {
            mCameraSettings.setMeteringAreas(mFocusManager.getMeteringAreas());
        }
    }

    private void updateCameraParametersPreference() {
        DataModuleManager.getInstance(
                mAppController.getAndroidContext()).getCurrentDataModule();
        // some monkey tests can get here when shutting the app down
        // make sure mCameraDevice is still valid, b/17580046
        if (mCameraDevice == null) {
            return;
        }

        mCameraSettings.setDefault(mCameraId);//SPRD:fix bug616836 add for photo use api1 or api2 use reconnect
        setAutoExposureLockIfSupported();
        setAutoWhiteBalanceLockIfSupported();
        setFocusAreasIfSupported();
        setMeteringAreasIfSupported();

        // Initialize focus mode.
        mFocusManager.overrideFocusMode(null);
        mCameraSettings.setFocusMode(mFocusManager.getFocusMode(mCameraSettings
                .getCurrentFocusMode(),mDataModule.getString(Keys.KEY_FOCUS_MODE)));
        SessionStatsCollector
                .instance()
                .autofocusActive(
                        mFocusManager.getFocusMode(mCameraSettings
                                .getCurrentFocusMode(),mDataModule.getString(Keys.KEY_FOCUS_MODE)) == CameraCapabilities.FocusMode.CONTINUOUS_PICTURE);

        if (mIsImageCaptureIntent) {
            updateParametersFlashMode();
            mDataModuleCurrent.set(Keys.KEY_CAMERA_HDR, false);
        } else {
            Log.d(TAG,
                    "updateCameraParametersPreference CameraSettingsFragment.mNeedCheckMutex="
                            + CameraSettingsFragment.mNeedCheckMutex);

            /*
             * SPRD: mutex - Premise: Exposure =3, HDR = on; Action: Set
             * sceneMode = action; Result: HDR off. And Exposure = 0. Expected:
             * Exposure = 3. Every time entry
             * CameraSettingsFragment,mNeedCheckMutex must be reset to false,
             * just as what here does. This is why add
             * CameraSettingsFragment.mNeedCheckMutex. After last modify.
             * Premise: Exposure =3, flash on, HDR = on, change to front camera;
             * Action: Set contrast = 3; Result: front camera is right: HDR off,
             * flash off. Exposure = -3. BUT change to back camera: HDR off,
             * flash off, exposure = 0. Expected: Exposure = 3. flash = on. This
             * is why add CameraSettingsFragment.mNeedCheckMutex.
             */
            if (CameraSettingsFragment.mNeedCheckMutex
                    && Keys.isCameraBackFacing(mActivity.getSettingsManager())) {
                onSettingChanged(mActivity.getSettingsManager(),
                        Keys.KEY_CAMERA_HDR);
                CameraSettingsFragment.mNeedCheckMutex = false;
            }

            // SPRD:Add for antibanding
            updateParametersAntibanding();

            // Set JPEG quality.
            updateParametersPictureQuality();

            // For the following settings, we need to check if the settings are
            // still supported by latest driver, if not, ignore the settings.
            // Set exposure compensation
            updateParametersExposureCompensation();

            // Set the scene mode: also sets flash and white balance.
            updateParametersSceneMode();

            // SPRD:Modify for add whitebalance bug 474737
            updateParametersWhiteBalance();

            // SPRD:Add for color effect Bug 474727
            updateParametersColorEffect();

            /*
             * SPRD:Add for ai detect @{ now face is not mutex with ai in UE's
             * doc. faceDatectMutex();
             * 
             * @}
             */

            updateParametersBurstCount();

            updateParametersFlashMode();

            // SPRD Bug:474721 Feature:Contrast.
            updateParametersContrast();

            // SPRD Bug:474715 Feature:Brightness.
            updateParametersBrightness();

            // SPRD Bug:474724 Feature:ISO.
            updateParametersISO();

            updateParametersMetering();

            // SPRD Bug: 505155 Feature: zsl
            updateParametersZsl();

            // SPRD:Add for mirror
            updateParametersMirror();

            // SPRD : Fature : OIS && EIS
            updateParametersEOIS();

            // SPRD : Fature : TimeStamp
            updateTimeStamp();

            // SPRD: Feature HDR
            updateParametersHDR();
        }

        // SPRD Bug:474722 Feature:Saturation.
        updateParametersSaturation();

        // SPRD:fix 501883 After closing shutter sound in camera ,it plays sound
        // when takepictures
        // from contact enter camera
        updateCameraShutterSound();

        updateParametersGridLine();

        if (mContinuousFocusSupported && ApiHelper.HAS_AUTO_FOCUS_MOVE_CALLBACK) {
            updateAutoFocusMoveCallback();
        }
        updateMakeLevel();
    }

    /* SPRD: Fix Bug 534257 New Feature EIS&OIS @{ */
    private void updateParametersEOIS() {
        if (isCameraFrontFacing() && CameraUtil.isEOISDcFrontEnabled()) {
            Log.i(TAG,
                    "front camera eois = " + mDataModuleCurrent.getBoolean(Keys.KEY_EOIS_DC_FRONT));
            if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_EOIS_DC_FRONT)){
                return;
            }
            mCameraSettings.setEOISEnable(mDataModuleCurrent.getBoolean(Keys.KEY_EOIS_DC_FRONT));
            return;
        }

        if (!isCameraFrontFacing() && CameraUtil.isEOISDcBackEnabled()) {
            Log.i(TAG, "back camera eois = " + mDataModuleCurrent.getBoolean(Keys.KEY_EOIS_DC_BACK));
            if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_EOIS_DC_BACK)){
                return;
            }
            mCameraSettings
                    .setEOISEnable(mDataModuleCurrent.getBoolean(Keys.KEY_EOIS_DC_BACK));
            return;
        }
    }
    /* @} */

    /**
     * This method sets picture size parameters. Size parameters should only be
     * set when the preview is stopped, and so this method is only invoked in
     * {@link #startPreview()} just before starting the preview.
     */
    private void updateParametersPictureSize() {
        if (mCameraDevice == null) {
            Log.w(TAG, "attempting to set picture size without caemra device");
            return;
        }

        List<Size> supported = Size.convert(mCameraCapabilities
                .getSupportedPhotoSizes());
        CameraPictureSizesCacher.updateSizesForCamera(
                mAppController.getAndroidContext(),
                mCameraDevice.getCameraId(), supported);

        OneCamera.Facing cameraFacing = isCameraFrontFacing() ? OneCamera.Facing.FRONT
                : OneCamera.Facing.BACK;
        Size pictureSize;
        try {
            pictureSize = mAppController.getResolutionSetting()
                    .getPictureSize(DataModuleManager.getInstance(mAppController.getAndroidContext()),
                            mAppController.getCameraProvider()
                                    .getCurrentCameraId(), cameraFacing);
        } catch (OneCameraAccessException ex) {
            mAppController.getFatalErrorHandler()
                    .onGenericCameraAccessFailure();
            return;
        }
        mCameraSettings.setPhotoSize(pictureSize.toPortabilitySize());

        if (ApiHelper.IS_NEXUS_5) {
            if (ResolutionUtil.NEXUS_5_LARGE_16_BY_9.equals(pictureSize)) {
                mShouldResizeTo16x9 = true;
            } else {
                mShouldResizeTo16x9 = false;
            }
        }

        // SPRD: add fix bug 555245 do not display thumbnail picture in MTP/PTP Mode at pc
        mCameraSettings.setExifThumbnailSize(CameraUtil.getAdaptedThumbnailSize(pictureSize,
                mAppController.getCameraProvider()).toPortabilitySize());

        // Set a preview size that is closest to the viewfinder height and has
        // the right aspect ratio.
        List<Size> sizes = Size.convert(mCameraCapabilities
                .getSupportedPreviewSizes());
        Size optimalSize = CameraUtil.getOptimalPreviewSize(sizes,
                (double) pictureSize.width() / pictureSize.height());
        Size original = new Size(mCameraSettings.getCurrentPreviewSize());
        if (!optimalSize.equals(original)) {
            Log.i(TAG, "setting preview size. optimal: " + optimalSize
                    + "original: " + original);
            mCameraSettings.setPreviewSize(optimalSize.toPortabilitySize());

        }

        if (optimalSize.width() != 0 && optimalSize.height() != 0) {
            Log.i(TAG, "updating aspect ratio");
            mUI.updatePreviewAspectRatio((float) optimalSize.width()
                    / (float) optimalSize.height());
        }
        Log.d(TAG, "Preview size is " + optimalSize);
    }

    private void updateParametersAiDetect() {
        mUI.intializeAIDetection(mDataModuleCurrent);
        updateFace();
    }

    /*
     * SPRD:Modify for jpegquality @{ private void
     * updateParametersPictureQuality() { int jpegQuality =
     * CameraProfile.getJpegEncodingQualityParameter(mCameraId,
     * CameraProfile.QUALITY_HIGH);
     * mCameraSettings.setPhotoJpegCompressionQuality(jpegQuality); }
     */
    private void updateParametersPictureQuality() {
        if (mJpegQualityController != null) {
            if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_JPEG_QUALITY)){
                return;
            }
            String quality = mDataModuleCurrent
                    .getString(Keys.KEY_JPEG_QUALITY);
            int jpegQuality = mJpegQualityController.findJpegQuality(quality);
            mCameraSettings.setPhotoJpegCompressionQuality(jpegQuality);
        }
    }

    /* @} */

    // SPRD:Add for antibanding
    private void updateParametersAntibanding() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMER_ANTIBANDING)){
            return;
        }
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                .getStringifier();
        String mAntibanding = mDataModuleCurrent
                .getString(Keys.KEY_CAMER_ANTIBANDING);
        if (mAntibanding == null) {
            return;
        }
        mCameraSettings.setAntibanding(stringifier
                .antibandingModeFromString(mAntibanding));
    }

    /* SPRD:Add for color effect Bug 474727 @{ */
    private void updateParametersColorEffect() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_COLOR_EFFECT)){
            return;
        }
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                .getStringifier();
        String colorEffect = mDataModuleCurrent
                .getString(Keys.KEY_CAMERA_COLOR_EFFECT);
        if (colorEffect == null) {
            return;
        }
        mColorEffect = stringifier.colorEffectFromString(colorEffect);

        Log.d(TAG, "update ColorEffect = " + mColorEffect);
        mCameraSettings.setColorEffect(mColorEffect);
    }

    public void updateParametersExposureCompensation() {
        if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_EXPOSURE)) {
            return;
        }
        int value = mDataModuleCurrent.getInt(Keys.KEY_EXPOSURE);
        Log.i(TAG, "Keys.KEY_EXPOSURE = " + value);
        setExposureCompensation(value);
    }

    private void updateParametersHDR() {
        /* SPRD:fix bug619441 HDR is one of scene mode, not need to effect scene mode setting @ */
        if (mCameraDevice == null || !mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_HDR)) {
            return;
        }
        /* @}　*/
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                .getStringifier();

        Log.d(TAG, "updateParametersHDR mSceneMode=" + mSceneMode + ","
                + isHdr());
        if (isHdr()) {
            mSceneMode = CameraCapabilities.SceneMode.HDR;
        } else {
            mSceneMode = stringifier.sceneModeFromString("auto");
        }

        Log.d(TAG, "updateParametersSceneMode mSceneMode=" + mSceneMode);

        if (mCameraCapabilities.supports(mSceneMode)) {
            Log.d(TAG, "updateParametersHDR support currentSceneMode="
                    + mCameraSettings.getCurrentSceneMode());
            if (mCameraSettings.getCurrentSceneMode() != mSceneMode) {
                mCameraSettings.setSceneMode(mSceneMode);

                // Setting scene mode will change the settings of flash mode,
                // white balance, and focus mode. Here we read back the
                // parameters, so we can know those settings.
                // mCameraDevice.applySettings(mCameraSettings);
                // mCameraSettings = mCameraDevice.getSettings();
            }
        } else {
            mSceneMode = mCameraSettings.getCurrentSceneMode();
            if (mSceneMode == null) {
                mSceneMode = CameraCapabilities.SceneMode.AUTO;
            }
        }
    }

    private void updateParametersSceneMode() {
        /* SPRD:fix bug517304 NullPointer Exception */
        if (mCameraDevice == null) {
            return;
        }

        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_SCENE_MODE)){
            return;
        }

        CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                .getStringifier();
        String sceneMode = mDataModuleCurrent.getString(Keys.KEY_SCENE_MODE);

        if (sceneMode == null) {
            return;
        }
        if ("normal".equals(sceneMode)) {// Fix bug 411026
            sceneMode = "auto";
        }
        mSceneMode = stringifier.sceneModeFromString(sceneMode);
        Log.d(TAG, "updateParametersSceneMode mSceneMode=" + mSceneMode);
        /* @} */
        if (mCameraCapabilities.supports(mSceneMode)) {
            Log.d(TAG, "updateParametersSceneMode support currentSceneMode="
                    + mCameraSettings.getCurrentSceneMode());
            if (mCameraSettings.getCurrentSceneMode() != mSceneMode) {
                mCameraSettings.setSceneMode(mSceneMode);
            }
        } else {
            mSceneMode = mCameraSettings.getCurrentSceneMode();
            if (mSceneMode == null) {
                mSceneMode = CameraCapabilities.SceneMode.AUTO;
            }
        }

    }

    /* SPRD:Modify for add whitebalance bug 474737 @{ */
    public void updateParametersWhiteBalance() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_WHITE_BALANCE)){
            return;
        }
        String wb = mDataModuleCurrent.getString(Keys.KEY_WHITE_BALANCE);
        Log.d(TAG, "updateParametersWhiteBalance = " + wb);
        if (wb == null) {
            return;
        }
        CameraCapabilities.WhiteBalance whiteBalance = mCameraCapabilities
                .getStringifier().whiteBalanceFromString(wb);

        if (mCameraCapabilities.supports(whiteBalance)) {
            mCameraSettings.setWhiteBalance(whiteBalance);
        }
    }

    /* @} */

    private void updateParametersFlashMode() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_FLASH_MODE)){
            return;
        }
        String flash = mDataModuleCurrent.getString(Keys.KEY_FLASH_MODE);
        if (flash == null) {
            return;
        }
        //SPRD: Fix bug 631061 flash may be in error state when receive lowbattery broadcast
        if (!mAppController.getButtonManager().isEnabled(ButtonManagerDream.BUTTON_FLASH_DREAM)) {
            return;
        }
        if (mIsBatteryLow && !"off".equals(flash)) {
            flash = "off";
            //mDataModuleCurrent.changeSettings(Keys.KEY_FLASH_MODE, flash);
        }

        /*
         * the mutex was implemented in datamodulephoto
        if(!flash.equals("on")){
            mDataModuleCurrent.changeSettings(Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE, "off");
        }else {
            mDataModuleCurrent.changeSettings(Keys.KEY_DREAM_FLASH_GIF_PHOTO_MODE, "torch");
        }
         */

        CameraCapabilities.FlashMode flashMode = mCameraCapabilities
                .getStringifier().flashModeFromString(flash);
        if (mCameraCapabilities.supports(flashMode)) {
            mCameraSettings.setFlashMode(flashMode);
        }

        Log.d(TAG, "updateParametersFlashMode = " + flashMode);
    }

    //nj dream camera test 70, 75
    private void updateParametersGridLine() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_GRID_LINES)){
            return;
        }
        String grid = mDataModuleCurrent.getString(Keys.KEY_CAMERA_GRID_LINES);
        mAppController.getCameraAppUI().updateScreenGridLines(grid);
        Log.d(TAG, "updateParametersGridLine = " + grid);
    }

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    private void updateAutoFocusMoveCallback() {
        if (mCameraDevice == null) {
            return;
        }
        if (mCameraSettings.getCurrentFocusMode() == CameraCapabilities.FocusMode.CONTINUOUS_PICTURE) {
            mCameraDevice.setAutoFocusMoveCallback(mHandler,
                    (CameraAFMoveCallback) mAutoFocusMoveCallback);
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
     * Sets the exposure compensation to the given value and also updates
     * settings.
     * 
     * @param value
     *            exposure compensation value to be set
     */
    public void setExposureCompensation(int value) {
        int max = mCameraCapabilities.getMaxExposureCompensation();
        int min = mCameraCapabilities.getMinExposureCompensation();
        Log.d(TAG, "setExposureCompensation vaule" + value);
        if (value >= min && value <= max) {
            mCameraSettings.setExposureCompensationIndex(value);
            mDataModuleCurrent.set(Keys.KEY_EXPOSURE, value);
        } else {
            Log.w(TAG, "invalid exposure range: " + value);
        }
    }

    // We separate the parameters into several subsets, so we can update only
    // the subsets actually need updating. The PREFERENCE set needs extra
    // locking because the preference can be changed from GLThread as well.
    private void setCameraParameters(int updateSet) {
        if ((updateSet & UPDATE_PARAM_INITIALIZE) != 0) {
            updateCameraParametersInitialize();
        }

        if ((updateSet & UPDATE_PARAM_ZOOM) != 0) {
            updateCameraParametersZoom();
        }

        if ((updateSet & UPDATE_PARAM_PREFERENCE) != 0) {
            updateCameraParametersPreference();
        }

        Log.d(TAG, "setCameraParameters mCameraDevice = " + mCameraDevice);
        if (mCameraDevice != null) {
            mCameraDevice.applySettings(mCameraSettings);
        }
    }

    // If the Camera is idle, update the parameters immediately, otherwise
    // accumulate them in mUpdateSet and update later.
    private void setCameraParametersWhenIdle(int additionalUpdateSet) {
        mUpdateSet |= additionalUpdateSet;
        if (mCameraDevice == null) {
            // We will update all the parameters when we open the device, so
            // we don't need to do anything now.
            mUpdateSet = 0;
            return;
        } else if (isCameraIdle()) {
            setCameraParameters(mUpdateSet);
            /*
             * SPRD: All MUTEX OPERATION in onSettingsChanged function.
             * updateSceneMode();
             */
            mUpdateSet = 0;
        } else {
            if (!mHandler.hasMessages(MSG_SET_CAMERA_PARAMETERS_WHEN_IDLE)) {
                mHandler.sendEmptyMessageDelayed(
                        MSG_SET_CAMERA_PARAMETERS_WHEN_IDLE, 1000);
            }
        }
    }

    @Override
    public boolean isCameraIdle() {
        return (mCameraState == IDLE)
                || (mCameraState == PREVIEW_STOPPED)
                || ((mFocusManager != null) && mFocusManager.isFocusCompleted() && (mCameraState != SWITCHING_CAMERA));
    }

    @Override
    public boolean isImageCaptureIntent() {
        String action = mActivity.getIntent().getAction();
        return (MediaStore.ACTION_IMAGE_CAPTURE.equals(action) || CameraActivity.ACTION_IMAGE_CAPTURE_SECURE
                .equals(action));
    }

    private void setupCaptureParams() {
        Bundle myExtras = mActivity.getIntent().getExtras();
        if (myExtras != null) {
            mSaveUri = (Uri) myExtras.getParcelable(MediaStore.EXTRA_OUTPUT);
            mCropValue = myExtras.getString("crop");
        }
    }

    private void initializeCapabilities() {
        mCameraCapabilities = mCameraDevice.getCapabilities();
        mFocusAreaSupported = mCameraCapabilities
                .supports(CameraCapabilities.Feature.FOCUS_AREA);
        mMeteringAreaSupported = mCameraCapabilities
                .supports(CameraCapabilities.Feature.METERING_AREA);
        mAeLockSupported = mCameraCapabilities
                .supports(CameraCapabilities.Feature.AUTO_EXPOSURE_LOCK);
        mAwbLockSupported = mCameraCapabilities
                .supports(CameraCapabilities.Feature.AUTO_WHITE_BALANCE_LOCK);
        mContinuousFocusSupported = mCameraCapabilities
                .supports(CameraCapabilities.FocusMode.CONTINUOUS_PICTURE);
        mMaxRatio = mCameraCapabilities.getMaxZoomRatio();
    }

    @Override
    public void onZoomChanged(float ratio) {
        // Not useful to change zoom value when the activity is paused.
        if (mPaused) {
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

    @Override
    public int getCameraState() {
        return mCameraState;
    }

    @Override
    public void onMemoryStateChanged(int state) {
        /**
         * SPRD: fix bug 473462 add for burst capture @{
         * mAppController.setShutterEnabled(state == MemoryManager.STATE_OK);
         */
        Log.i(TAG, "onMemoryStateChanged,(state == MemoryManager.STATE_OK)"
                + (state == MemoryManager.STATE_OK)
                + " mContinuousCaptureCount = " + mContinuousCaptureCount);
        if (mContinuousCaptureCount <= 0 ) {
            if(!mBurstNotShowShutterButton) {
                /* SPRD:fix bug623289 burst capture not work @{ */
                if (!isShutterEnabled()) {
                    mBurstCaptureType = -1;
                }
                /* @} */
                mAppController.setShutterEnabled(state == MemoryManager.STATE_OK);
            }
            /* SPRD: Fix bug 547144 that app is killed because of low memory @{ */
            if (state != MemoryManager.STATE_OK) {
                mAppController.getCameraAppUI().disableModeOptions();
            }
            /* @} */
            if (isUcamBeautyCanBeUsed()) {
                mUI.setFilterMakeupButtonEnabled(state == MemoryManager.STATE_OK);// SPRD:fix
                                                                                  // bug474672
            }
            if (!(state == MemoryManager.STATE_OK) && mActivity != null) {
                Toast.makeText(mActivity,
                        R.string.message_save_task_memory_limit,
                        Toast.LENGTH_LONG).show();
            }

            mHasStartCapture = false;
        }
    }

    @Override
    public void onLowMemory() {
        // Not much we can do in the photo module.
    }

    // For debugging only.
    public void setDebugUri(Uri uri) {
        mDebugUri = uri;
    }

    // For debugging only.
    private void saveToDebugUri(byte[] data) {
        if (mDebugUri != null) {
            OutputStream outputStream = null;
            try {
                outputStream = mContentResolver.openOutputStream(mDebugUri);
                outputStream.write(data);
                outputStream.close();
            } catch (IOException e) {
                Log.e(TAG, "Exception while writing debug jpeg file", e);
            } finally {
                CameraUtil.closeSilently(outputStream);
            }
        }
    }

    @Override
    public void onRemoteShutterPress() {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                focusAndCapture();
            }
        });
    }

    /***********************
     * SPRD: Add Listener Implements for FreezeDisplay*************************
     **/

    private FreezeFrameDisplayControl sFreezeFrameControl;

    @Override
    public void proxyStartPreview() {
        // TODO Auto-generated method stub
        startPreview(true);
    }

    @Override
    public void proxySetCameraState(int state) {
        // TODO Auto-generated method stub
        setCameraState(state);
    }

    @Override
    public void proxystartFaceDetection() {
        // TODO Auto-generated method stub
        startFaceDetection();
    }

    @Override
    public void proxyCaptureCancelled() {
        // TODO Auto-generated method stub
        onCaptureCancelled();
    }

    @Override
    public void proxyCaptureDone() {
        // TODO Auto-generated method stub
        onCaptureDone();
    }

    @Override
    public boolean proxyIsContinueTakePicture() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean proxyIsPauseStatus() {
        // TODO Auto-generated method stub
        return mPaused;
    }

    @Override
    public void proxySetImageData() {
        // TODO Auto-generated method stub
        mJpegImageData = null;
    }

    public boolean isFreezeFrameDisplay() {
        if (sFreezeFrameControl != null) {
            return sFreezeFrameControl.proxyDisplay();
        }
        return false;
    }

    /***** Add new feature for Freeze end ************************/

    /* SPRD: porting new feature JPEG quality start @{ */
    private JpegQualityController mJpegQualityController;

    private class JpegQualityController {
        private static final String LOG_TAG = "CameraJpegQualityController";
        private final Log.Tag TAG = new Log.Tag("PhotoModule");
        private static final String VAL_NORMAL = "normal";
        private static final String VAL_HIGHT = "hight";
        private static final String VAL_SUPER = "super";
        private static final int VAL_DEFAULT_QUALITY = 85;

        // default construct
        /* package */
        JpegQualityController() {
        }

        public int findJpegQuality(String quality) {
            Log.d(TAG, "findJpegQuality");
            int convertQuality = getConvertJpegQuality(quality);
            int result = VAL_DEFAULT_QUALITY;
            Log.d(TAG, "findJpegQuality convertQuality = " + convertQuality
                    + " VAL_DEFAULT_QUALITY != convertQuality = "
                    + (VAL_DEFAULT_QUALITY != convertQuality));
            if (VAL_DEFAULT_QUALITY != convertQuality) {
                result = CameraProfile.getJpegEncodingQualityParameter(convertQuality);
                Log.d(TAG, "findJpegQuality result = " + result);
            }
            Log.d(TAG, "findJpegQuality result = " + result);
            return result;
        }

        private int getConvertJpegQuality(String quality) {
            Log.d(TAG, "getConvertJpegQuality");
            int result = VAL_DEFAULT_QUALITY;
            if (quality != null) {
                if (VAL_NORMAL.equals(quality))
                    result = CameraProfile.QUALITY_LOW;
                else if (VAL_HIGHT.equals(quality))
                    result = CameraProfile.QUALITY_MEDIUM;
                else if (VAL_SUPER.equals(VAL_SUPER))
                    result = CameraProfile.QUALITY_HIGH;
            }
            Log.d(TAG, "getConvertJpegQuality result = " + result);
            return result;
        }
    }

    /* }@ dev new feature jpeg quality end */

    /* SPRD:Add for ai detect @{ */
    private void updateFace() {
       if (mCameraDevice == null) {
            Log.i(TAG, "mCameraDevice is null ");
            return;
        }
       if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_AI_DATECT)){
           return;
       }
        mFace = mDataModuleCurrent.getString(Keys.KEY_CAMERA_AI_DATECT);
        if (mFace == null) {
            Log.i(TAG, "mFace is null ");
            return;
        }
        Log.d(TAG, "face = " + mFace + "; hdrState = " + isHdr());
        if (!mFace.equals(Keys.CAMERA_AI_DATECT_VAL_OFF) && isCameraIdle()) {
//            mDataModuleCurrent.set(Keys.KEY_CAMERA_AI_DATECT,mFace);
            startFaceDetection();
        } else if (mFace.equals(Keys.CAMERA_AI_DATECT_VAL_OFF)|| !isCameraIdle()) {
//            mDataModuleCurrent.set(Keys.KEY_CAMERA_AI_DATECT,mFace);
            stopFaceDetection();
        }
        mCameraDevice.applySettings(mCameraSettings); // SPRD: BUG 531871 Smile capture is invalid
    }

    /*
     * now face is not mutex with ai in UE's doc. private void faceDatectMutex()
     * { SettingsManager settingsManager = mActivity.getSettingsManager();
     * String mface = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
     * Keys.KEY_CAMERA_AI_DATECT); if (isHdr() &&
     * !mface.equals(Keys.CAMERA_AI_DATECT_VAL_OFF)) {
     * settingsManager.set(SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_HDR,
     * false); } }
     */

    private void openFace() {
        SettingsManager settingsManager = mActivity.getSettingsManager();
        String mFace = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                Keys.KEY_CAMERA_AI_DATECT);
        SharedPreferences defaultPrefs = PreferenceManager
                .getDefaultSharedPreferences(mActivity.getAndroidContext());
        mDetectValueBeforeSceneMode = defaultPrefs.getString(
                Keys.KEY_CAMERA_AI_DATECT + 1, null);
        if (mFace != null && mFace.equals(Keys.CAMERA_AI_DATECT_VAL_OFF)) {
            settingsManager.set(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_AI_DATECT, mDetectValueBeforeSceneMode);
            startFaceDetection();
        }
    }

    private void closeFace() {
        SettingsManager settingsManager = mActivity.getSettingsManager();
        String mFace = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                Keys.KEY_CAMERA_AI_DATECT);
        SharedPreferences defaultPrefs = PreferenceManager
                .getDefaultSharedPreferences(mActivity.getAndroidContext());
        SharedPreferences.Editor editor = defaultPrefs.edit();
        mDetectValueBeforeSceneMode = settingsManager.getString(
                SettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_AI_DATECT);
        editor.putString(Keys.KEY_CAMERA_AI_DATECT + 1,
                mDetectValueBeforeSceneMode);
        editor.apply();
        if (mFace != null && !mFace.equals(Keys.CAMERA_AI_DATECT_VAL_OFF)) {
            settingsManager.set(SettingsManager.SCOPE_GLOBAL,
                    Keys.KEY_CAMERA_AI_DATECT, Keys.CAMERA_AI_DATECT_VAL_OFF);
            stopFaceDetection();
        }
    }

    private boolean isHdr() {
        // SettingsManager settingsManager = mActivity.getSettingsManager();
        // return settingsManager.getBoolean(SettingsManager.SCOPE_GLOBAL,
        // Keys.KEY_CAMERA_HDR);
        return mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_HDR);
    }

    // SPRD:Add for ai detect
    public void setCaptureCount(int count) {
        Log.d(TAG, "setCaptureCount count=" + count);
        mCaptureCount = count;
    }

    /* @} */

    /* SPRD: BUG 397228 stop or start face detection start @{ */
    @Override
    public void onPreviewVisibilityChanged(int visibility) {
        // SPRD BUG: 397097
        if (visibility == ModuleController.VISIBILITY_VISIBLE) {
            mUI.resumeFaceDetection();
        } else {
            mUI.pauseFaceDetection();
            mUI.clearFaces();
        }
    }

    /* }@ BUG 397228 end */

    /*
     * SPRD: Fix 473602 bug: flash is on while shutter in photoModule then the
     * process is slowly
     * 
     * @{
     */
    private boolean isFocusModeFixed() {
        if (mCameraSettings == null
                || mCameraSettings.getCurrentFocusMode() == CameraCapabilities.FocusMode.FIXED) {
            return true;
        }
        return false;
    }

    /*
     * /** SPRD: fix bug 473462 add for burst capture @{
     * 
     * @ {
     */
    @Override
    public boolean onTouch(View v, MotionEvent event) {
        int action = event.getActionMasked();
        Log.i(TAG, " onTouch action = " + action);
        if (mUI != null && !mUI.isOnTouchInside(event)) {
            Log.i(TAG, " onTouch out side action = " + action);
            if (action == MotionEvent.ACTION_MOVE) {
                if(mBurstCaptureType == 0)
                    handleActionUp();
            }
            if (!isActionUp(action)) {
                return false;
            }
        }
        switch (v.getId()) {
        case R.id.shutter_button:
            if (action == MotionEvent.ACTION_DOWN) {
                if (!mBurstMode) {
                    handleActionDown(0);
                }
            } else if (isActionUp(action)) {
                if(mBurstCaptureType == 0) {
                    handleActionUp();
                }
            }
            break;
        }
        return false;
    }

    /* SPRD:fix bug471950 add burst for Camera Key @{ */
    protected void handleActionDown(int action) {
        Log.d(TAG, "mBurstMode =" + mBurstMode + "," + mIsImageCaptureIntent
                + "," + checkStorage());
        if (!mBurstMode && checkStorage() && !mIsImageCaptureIntent) {
            mBurstCaptureType = action;
            mHandler.sendEmptyMessageDelayed(BUTST_START_CAPTURE, 500);
        }
    }

    /* @} */

    private boolean isActionUp(int action) {
        return action == MotionEvent.ACTION_UP
                || /* action == MotionEvent.ACTION_POINTER_UP || */action == MotionEvent.ACTION_CANCEL;
    }

    protected void handleActionUp() {
        Log.i(TAG, " handleActionUp ");
        if (mActivity != null
                && isBurstOn()) {
            Log.i(TAG, " handleActionUp enter mBurstMode = " + mBurstMode);
            if (mBurstMode) {
                mBurstMode = false;
                mBurstNotShowShutterButton = true;
                mActivity.getCameraAppUI().setShutterButtonEnabled(false);
                cancelBurstCapture();
            } else {
                mHandler.removeMessages(BUTST_START_CAPTURE);
            }
        }
        mBurstCaptureType = -1;
    }

    private boolean checkStorage() {
        if (mActivity == null) {
            return false;
        }
        if (mActivity.getStorageSpaceBytes() <= Storage.LOW_STORAGE_THRESHOLD_BYTES) {
            Log.i(TAG, "Not enough space or storage not ready.");
            return false;
        }
        return true;
    }

    private void cancelBurstCapture() {
        Log.i(TAG, "cancelBurstCapture mCameraState = " + mCameraState
                + " mHasStartCapture = " + mHasStartCapture
                + " mHasCaputureCount = " + mHasCaputureCount
                + " mCameraDevice = " + mCameraDevice);
        mCancelBurst = true;
        if (mCameraState == SNAPSHOT_IN_PROGRESS || mCameraState == FOCUSING) {
            if (mHasStartCapture && mHasCaputureCount > 0) {
                if (mCameraDevice != null) {
                    mIsContinousCaptureFinish = true;
                    // SPRD:fix bug 497854 when cancel 10 burst capture,the count of pics saveing is wrong
                    mHasCaputureCount = mCameraDevice.cancelBurstCapture(this);
                }
            } else {
                mHandler.postDelayed(new Runnable() {

                    @Override
                    public void run() {
                        if (mCameraDevice != null) {
                            mIsContinousCaptureFinish = true;
                           // SPRD:fix bug 497854 when cancel 10 burst capture,the count of pics saveing is wrong
                            mCameraDevice.cancelBurstCapture(PhotoModule.this);
                        }
                    }
                }, 1000);
            }
        } else {
            mIsContinousCaptureFinish = true;
            return;
        }
    }

    /*
     * SPRD:fix bug 497854 when cancel 10 burst capture,the count of pics
     * saveing is wrong @{
     */
    @Override
    public void onCanceled(int count) {
        /*
        mHasCaputureCount = getCurrentBurstCountFromSettings() - count;
        mTotalCaputureCount = mHasCaputureCount;
        Log.d(TAG, "onCanceled " + getCurrentBurstCountFromSettings() + ","
                + count);
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mUI.showBurstScreenHint(mTotalCaputureCount);
            }
        });
        */
    }

    /* @} */

    public int getBurstHasCaptureCount() {
        return mHasCaputureCount;
    }

    public void onHideBurstScreenHint() {
        mUI.dismissBurstScreenHit();
        if (mAppController.getCameraAppUI().isInFreezeReview()) {// SPRD :BUG
                                                                 // 398284
            mAppController.getCameraAppUI().setSwipeEnabled(false);
            mAppController.getCameraAppUI().onShutterButtonClick();
        } else {
            mAppController.getCameraAppUI().setSwipeEnabled(true);
            mAppController.getCameraAppUI().showModeOptions();
        }
    }

    public int getContinuousCaptureCount() {
        return mContinuousCaptureCount;
    }

    public int getContinuousCount() {
        if (mButstNumber == null) {
            return 1;
        }
        if ("ONE".equals(mButstNumber.toString())) {
            mContinueTakePictureCount = 1;
        } else if ("THREE".equals(mButstNumber.toString())) {
            mContinueTakePictureCount = 3;
        } else if (("SIX".equals(mButstNumber.toString()))) {
            mContinueTakePictureCount = 6;
        } else if ("TEN".equals(mButstNumber.toString())) {
            mContinueTakePictureCount = 10;
        } else if ("NINETYNINE".equals(mButstNumber.toString())) {
            mContinueTakePictureCount = 99;
        } else {
            mContinueTakePictureCount = 1;
        }
        return mContinueTakePictureCount;
    }

    protected int getCurrentBurstCountFromSettings() {
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                .getStringifier();
        String burstNumber = mDataModuleCurrent.getString(Keys.KEY_CAMERA_CONTINUE_CAPTURE);

        CameraCapabilities.BurstNumber bn;
        int num;
        bn = stringifier.burstNumberFromString(burstNumber);
        if (bn == null) {
            return 1;
        }
        if ("ONE".equals(bn.toString())) {
            num = 1;
        } else if ("THREE".equals(bn.toString())) {
            num = 3;
        } else if (("SIX".equals(bn.toString()))) {
            num = 6;
        } else if ("TEN".equals(bn.toString())) {
            num = 10;
        } else if ("NINETYNINE".equals(bn.toString())) {
            num = 99;
        } else {
            num = 1;
        }
        return num;
    }

    private boolean isBurstCapture() {
        if (getContinuousCount() > 1) {
            return true;
        }
        return false;
    }

    private void updateParametersBurstCount() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_CONTINUE_CAPTURE)){
            return;
        }
        Log.i(TAG, "updateParametersBurstCount mBurstMode = " + mBurstMode);
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                .getStringifier();
        String burstNumber = mDataModuleCurrent.getString(Keys.KEY_CAMERA_CONTINUE_CAPTURE);
        Log.d(TAG, "updateParametersBurstCount burstNumber=" + burstNumber);
        /**
         * the logic if use zsl will update in method updateParametersZsl()
        if (CameraUtil.isZslBurstEnabled()) {
            if (!"one".equals(burstNumber)) {
                Log.i(TAG, "setZslModeEnable : 1");
                mCameraSettings.setZslModeEnable(1);
            } else {
                Log.i(TAG, "setZslModeEnable : 0");
                mCameraSettings.setZslModeEnable(0);
            }
        }
         */
        mButstNumber = stringifier.burstNumberFromString(!mBurstMode ? "one"
                : burstNumber);
        mCameraSettings.setBurstPicNum(mButstNumber);
    }

    private void updateParametersBurstPictureSize() {
        Log.i(TAG, "updateParametersBurstPictureSize");
        if (!isCameraFrontFacing()) {
            Size selectSize = new Size(mCameraSettings.getCurrentPhotoSize());
            if (selectSize != null
                    && selectSize.width() * selectSize.height() >= PICTURE_SIZE_13M) {
                List<Size> supported = Size.convert(mCameraCapabilities
                        .getSupportedPhotoSizes());
                List<Size> pictureSizes = ResolutionUtil
                        .getDisplayableSizesFromSupported(supported, true);
                if (pictureSizes.size() >= 2) {
                    Size secondSize = pictureSizes.get(1);
                    mCameraSettings
                            .setPhotoSize(secondSize.toPortabilitySize());
                    Log.i(TAG, " secondSize = " + secondSize);
                }
                int jpegQuality = mJpegQualityController
                        .findJpegQuality("normal");
                mCameraSettings.setPhotoJpegCompressionQuality(jpegQuality);
            }
        }
    }

    /**
     * @}
     */

    // SPRD Bug:474721 Feature:Contrast.
    public void updateParametersContrast() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_CONTRAST)){
            return;
        }
        String sContrast = mDataModuleCurrent.getString(Keys.KEY_CAMERA_CONTRAST);
        Log.d(TAG, "updateParametersContrast = " + sContrast);
        if (sContrast == null) {
            return;
        }
        CameraCapabilities.Contrast contrast = mCameraCapabilities
                .getStringifier().contrastFromString(sContrast);
        if (mCameraCapabilities.supports(contrast)) {
            mCameraSettings.setContrast(contrast);
        }
    }

    /*
     * SPRD Bug:474715 Feature:Brightness. @{
     */
    private CameraCapabilities.BrightNess mBrightness;

    public void updateParametersBrightness() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_BRIGHTNESS)){
            return;
        }
        String brightess = mDataModuleCurrent.getString(Keys.KEY_CAMERA_BRIGHTNESS);
        Log.d(TAG, "updateParametersBrightness = " + brightess);
        if (brightess == null) {
            return;
        }
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                .getStringifier();
        mBrightness = stringifier.brightnessFromString(brightess);
        mCameraSettings.setBrightNess(mBrightness);
    }

    /* SPRD: fix bug 474672 add for ucam beauty @{ */
    @Override
    public void onBeautyValueChanged(int value) {
        Log.i(TAG, "onBeautyValueChanged setParameters Value = " + value);
        /* SPRD: save makeup level @{ */
//        DataModuleManager.getInstance(mAppController.getAndroidContext())
//                .getCurrentDataModule()
//                .set(Keys.KEY_MAKEUP_MODE_LEVEL, value);
        /*
         * Add mCameraSettings != null reason: not like other updateParametersXX
         * functions sunch as updateParametersBrightness(), onBeautyValueChanged
         * is Initialized in such a order: CameraActivity.java onCreateTasks():
         * init-> PhotoModule.java init(): PhotoUI->PhotoUI.java PhotoUI()
         * Actually here exits a risk: camera is not be opened now. Which means
         * onBeautyValueChanged() can be trigger before camera open. In other
         * words, mCameraSettings equals null for onCameraAvailable is not
         * called.
         */
        if (mCameraSettings != null) {
            mCameraSettings.setSkinWhitenLevel(value);
            if (mCameraDevice != null) {
                mCameraDevice.applySettings(mCameraSettings);
            }
            /* @} */
        }
    }

    /* @} */

    /**
     * SPRD: Fix bug 513768 makeup module switch to normal photo module, makeup
     * effect still works. @{
     */
    @Override
    public void onBeautyValueReset() {
        Log.i(TAG, "onBeautyValueReset");
        /*
         * Add mCameraSettings != null reason: not like other updateParametersXX
         * functions sunch as updateParametersBrightness(), onBeautyValueChanged
         * is Initialized in such a order: CameraActivity.java onCreateTasks():
         * init-> PhotoModule.java init(): PhotoUI->PhotoUI.java PhotoUI()
         * Actually here exits a risk: camera is not be opened now. Which means
         * onBeautyValueChanged() can be trigger before camera open. In other
         * words, mCameraSettings equals null for onCameraAvailable is not
         * called.
         */
        if (mCameraSettings != null) {
            mCameraSettings.setSkinWhitenLevel(0);
            if (mCameraDevice != null) {
                mCameraDevice.applySettings(mCameraSettings);
            }
        }
    }

    /* @} */

    // SPRD Bug:474724 Feature:ISO.
    public void updateParametersISO() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_ISO)){
            return;
        }
        String sISO = mDataModuleCurrent.getString(Keys.KEY_CAMERA_ISO);
        Log.d(TAG, "updateParametersISO = " + sISO);
        if (sISO == null) {
            return;
        }
        CameraCapabilities.ISO iso = mCameraCapabilities.getStringifier()
                .isoModeFromString(sISO);
        if (mCameraCapabilities.supports(iso)) {
            mCameraSettings.setISO(iso);
        }
    }

    // SPRD Bug:474718 Feature:Metering.
    protected void updateParametersMetering() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMER_METERING)){
            return;
        }
        Log.i(TAG, "updateParametersMetering");
        String sMetering = mDataModuleCurrent.getString(Keys.KEY_CAMER_METERING);
        if (sMetering == null) {
            return;
        }
        CameraCapabilities.Metering metering = mCameraCapabilities
                .getStringifier().meteringFromString(sMetering);
        if (mCameraCapabilities.supports(metering)) {
            mCameraSettings.setMetering(metering);
        }
    }

    // SPRD Bug:474722 Feature:Saturation.
    public void updateParametersSaturation() {
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_SATURATION)){
            return;
        }
        String sSaturation = mDataModuleCurrent.getString(Keys.KEY_CAMERA_SATURATION);
        if (sSaturation == null) {
            return;
        }
        CameraCapabilities.Saturation saturation = mCameraCapabilities
                .getStringifier().saturationFromString(sSaturation);
        if (mCameraCapabilities.supports(saturation)) {
            mCameraSettings.setSaturation(saturation);
        }
    }

    // SPRD Bug:514488 Click Button when rotation.
    public void postDelayed(Runnable r, long delayMillis) {
        mHandler.postDelayed(r, delayMillis);
    }

    public PhotoUI createUI(CameraActivity activity) {
        return new PhotoUI(activity, this, activity.getModuleLayoutRoot());
    }

    public void singleTapAEAF(int x, int y) {
        mFocusManager.onSingleTapUp(x, y);
    }

    // SPRD Bug:505155 Feature:zsl.
    private void updateParametersZsl() {
        if (!CameraUtil.isZslEnable()) {
            return;
        }

        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_ZSL_DISPLAY)){
            return;
        }

        if (mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_ZSL_DISPLAY)) {
            Log.i(TAG, "updateParametersZsl setZslModeEnable : 1");
            mCameraSettings.setZslModeEnable(1);
        } else {
            Log.i(TAG, "updateParametersZsl setZslModeEnable : 0");
            mCameraSettings.setZslModeEnable(0);
        }
    }

    // SPRD Add for highiso
    private void setParametersHighISO(boolean highisoenable) {
        if (!CameraUtil.isHighISOEnable()) {
            return;
        }
        if (!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_HIGH_ISO)) {
            return;
        }
        if (mDataModuleCurrent.getBoolean(Keys.KEY_HIGH_ISO)) {
            if (highisoenable) {
                Log.i(TAG, "updateParametersHighISO setHighISOModeEnable : 1");
                mCameraSettings.setHighISOEnable(1);
            } else {
                Log.i(TAG, "updateParametersHighISO setHighISOModeEnable : 0");
                mCameraSettings.setHighISOEnable(0);
            }
        }
    }

//SPRD:fix bug534665 add some mutex about scene mode
    public void setExposureIfNecessary() {
        SettingsManager settingsManager = mActivity.getSettingsManager();
        if (settingsManager == null) {
            Log.e(TAG, "Settings manager is null when setExposureIfNecessary!");
            return;
        }
        String sceneMode = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                Keys.KEY_SCENE_MODE);
        if (!sceneMode.equals("auto")) {
            int exposureValue = settingsManager.getInteger(mAppController.getCameraScope(),
                    Keys.KEY_EXPOSURE);
            if (exposureValue != 0) {
                settingsManager.set(mAppController.getCameraScope(), Keys.KEY_EXPOSURE, "0");
            }
        }
    }

    public void applySettings() {
        if (mCameraDevice != null) {
            mCameraDevice.applySettings(mCameraSettings);
        }
    }

    protected void mediaSaved(Uri uri) {

    }

    protected int getDeviceCameraId() {
        return mCameraDevice.getCameraId();
    }


    /* SPRD: New feature vgesture detect @{ */
    public final ButtonManager.ButtonCallback mVGestureCallback =
            new ButtonManager.ButtonCallback() {
                @Override
                public void onStateChanged(int state) {
                    // At the time this callback is fired, the camera id
                    // has be set to the desired camera.

                    if (mPaused || mAppController.getCameraProvider().waitingForCamera()) {
                        return;
                    }
                    // If switching to back camera, and HDR is still on,
                    // switch back to gcam, otherwise handle callback normally.

                    if (mVGestureController != null) {
                        if (mDataModuleCurrent
                                .getBoolean(Keys.KEY_CAMERA_VGESTURE)) {
                            mVGestureController.startVGestureDetection(mActivity, mCameraDevice,
                                    mHandler, mDisplayOrientation, isCameraFrontFacing(),
                                    mUI.getRootView());
                        } else {
                            mVGestureController.stopVGestureDetection(mActivity, mCameraDevice);
                        }
                    }
                }
            };

    public void doOnPreviewStartedSpecial(boolean isCameraIdle, boolean isHdrOn,
            CameraActivity activity, CameraAgent.CameraProxy cameraDevice, Handler h,
            int displayOrientation, boolean mirror, View rootView) {
    }

    public void doCaptureSpecial() {
    }

    public void doCloseCameraSpecial(CameraActivity activity, CameraProxy cameraDevice) {
    }

    protected void doStartPreviewSpecial(boolean isCameraIdle, boolean isHdrOn,
            CameraActivity activity, CameraAgent.CameraProxy cameraDevice, Handler h,
            int displayOrientation, boolean mirror, View rootView, CameraSettings cameraSettings) {
    }

    protected void doStartPreview(CameraAgent.CameraStartPreviewCallback startPreviewCallback, CameraAgent.CameraProxy cameraDevice) {
        if (GservicesHelper.useCamera2ApiThroughPortabilityLayer(mActivity.getContentResolver())) {
            mCameraDevice.startPreview();
            startPreviewCallback.onPreviewStarted();
        } else {
            mCameraDevice.startPreviewWithCallback(new Handler(Looper.getMainLooper()),
                    startPreviewCallback);
        }
    }
    /* @} */

    /* SPRD:fix bug519299 Name of photo is not consistent with capture time */
    private void updateTimeStamp() {
        CameraUtil.initializeTimeFormat(mActivity);
        if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_CAMERA_TIME_STAMP)){
            CameraUtil.mTimeStamp = false;
            return;
        }
        CameraUtil.mTimeStamp = mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_TIME_STAMP);
        Log.i(TAG, "updateTimeStamp = " + CameraUtil.mTimeStamp);
    }
    /* @} */

    /* SPRD:Add for mirror @{ */
    private void updateParametersMirror() {
        if (isCameraFrontFacing()) {
            if(!mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_FRONT_CAMERA_MIRROR)){
                return;
            }
            mCameraSettings.setFrontCameraMirror(mDataModuleCurrent
                    .getBoolean(Keys.KEY_FRONT_CAMERA_MIRROR));
        }
    }

    /* @} */
    /* SPRD:Add for VGesture @{ */
    public void updateVGesture() {
        Log.d(TAG, "HELLO updateVGesture " + mVGestureController + ","
                + mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_VGESTURE));
        if (mVGestureController != null
                && mDataModuleCurrent
                        .isEnableSettingConfig(Keys.KEY_CAMERA_VGESTURE)) {
            if (mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_VGESTURE)) {
                mVGestureController.startVGestureDetection(mActivity,
                        mCameraDevice, mHandler, mDisplayOrientation,
                        isCameraFrontFacing(), mUI.getRootView());
                // SPRD: Fix bug 591461 VGesture and Self portrait is mutex
                if (mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_FRONT_CAMERA_MIRROR)) {
                    mDataModuleCurrent.set(Keys.KEY_FRONT_CAMERA_MIRROR, false);
                }
            } else {
                mVGestureController.stopVGestureDetection(mActivity,
                        mCameraDevice);
            }
        }
    }

    /* @} */

    /*
     * for dream camera if it want to do something
     * @{
     */
    /* dream test 50 @{ */
    protected void doSomethingWhenonPictureTaken() {
        mActivity.getCameraAppUI().updatePreviewUI(View.VISIBLE);
    }
    /* @} */

    protected void doSomethingWhenonShutterButtonClick() {

    }

    protected void doSomethingWhenonBrustStart(){

    }
    /* @} */
    /*
     * Add for ui check 122 @{
     */
    @Override
    public void onOrientationChanged(OrientationManager orientationManager,
            OrientationManager.DeviceOrientation deviceOrientation) {
        mUI.onOrientationChanged(orientationManager, deviceOrientation);
    }
    /*
     * @}
     */
    //dream test 11
    @Override
    public void updateParameter(String key) {
        switch(key){
            case Keys.KEY_CAMERA_SHUTTER_SOUND:
                updateCameraShutterSound();
                break;
            default:
                    break;
        }
    }
    /* dream test 84 @{ */
    protected void dosomethingWhenPause() {
    }
    /* @} */

    /* SPRD: Fix bug 535110, Photo voice record. @{ */
    protected void startAudioRecord() {}

    protected void onAudioRecordStopped() {}

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
        String beforeMode = mDataModuleCurrent.getString(Keys.KEY_FLASH_MODE + BEFORE_LOW_BATTERY);
        String currentMode = mDataModuleCurrent.getString(Keys.KEY_FLASH_MODE);
        if (level <= batteryLevel) {
            // step 1. save current value: on, off, torch
            if (TextUtils.isEmpty(beforeMode)) {
                mDataModuleCurrent.set(Keys.KEY_FLASH_MODE + BEFORE_LOW_BATTERY, currentMode);
            }
            // step 2. set flash mode off and write into sp
            /*
             * SPRD: Fix bug 631061 flash may be in error state when receive lowbattery broadcast
            mDataModuleCurrent.set(Keys.KEY_FLASH_MODE, "off");
             */
            // step 3. set batterylow flag
            mIsBatteryLow = true;
            // step 4. if flash is on, turn off the flash
            /* Fix Bug631122 this setting may be applied during capturing which cause error
             */
            if (!"off".equals(currentMode) && mCameraSettings != null
                    && mCameraCapabilities != null) {
                if (!isPhotoFocusing()) {
                    updateParametersFlashMode();
                }
            }
            // step 5. set button disabled and show toast to users
            mAppController.getButtonManager().disableButton(ButtonManagerDream.BUTTON_FLASH_DREAM);
            Toast.makeText(mActivity, R.string.battery_level_low, Toast.LENGTH_LONG).show();
        } else {
            // never lower than limen
            if (TextUtils.isEmpty(beforeMode)) {
                return;
            }
            // step 1.set before state value to current value
            /*
             * SPRD: Fix bug 631061 flash may be in error state when receive lowbattery broadcast
            mDataModuleCurrent.set(Keys.KEY_FLASH_MODE, beforeMode);
             */
            // step 2.set before state value null
            mDataModuleCurrent.set(Keys.KEY_FLASH_MODE + BEFORE_LOW_BATTERY, null);
            // step 3.set button disabled or enabled and set BatteryLow flag
            mAppController.getButtonManager().enableButton(ButtonManagerDream.BUTTON_FLASH_DREAM);
            mIsBatteryLow = false;
            // step 4.according to before state value turn on flash
            // SPRD: Fix bug 580978 HDR and flash is effective at the same time
            boolean hdrState = mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_HDR);
            // if (!"off".equals(beforeMode) && !hdrState
            if (!hdrState
                    && mCameraSettings != null && mCameraCapabilities != null) {
                // open video flash
                updateParametersFlashMode();
            }
        }
        //Fix Bug631122 this setting may be applied during capturing which cause error
        if (mCameraDevice != null) {
            if (!isPhotoFocusing()) {
                mCameraDevice.applySettings(mCameraSettings);
            }
        }
    }

    protected Handler getHandler() {
        return mHandler;
    }

    protected void initData(byte[] jpegData, String title, long date,
            int width, int height, int orientation, ExifInterface exif,
            Location location,
            MediaSaver.OnMediaSavedListener onMediaSavedListener) {
    }
    /* @} */
    //Bug#533869 add the feature of volume
    protected int getVolumeControlStatus(CameraActivity mActivity) {
        return mActivity.getVolumeControlStatus();
    }

    /* SPRD: Fix bug 592600, InterValPhotoModule Thumbnail display abnormal. @{ */
    public void updateIntervalThumbnail(final Bitmap indicator){}
    /* @} */

    public boolean isSetFreezeFrameDisplay() {
        if (mDataModuleCurrent != null
                && mDataModuleCurrent.isEnableSettingConfig(Keys.KEY_FREEZE_FRAME_DISPLAY)
                && mDataModuleCurrent.getBoolean(Keys.KEY_FREEZE_FRAME_DISPLAY))
            return true;

        return false;
    }
    protected int mBurstCaptureType = -1;

    public boolean isFreezeFrameDisplayShow() {
        if(sFreezeFrameControl != null && sFreezeFrameControl.mFreezeVisible)
            return true;

        return false;
    }

    public boolean isBurstCapturing() {return false;}

    /*
     * fix bug 601158 thumbnail does not generate and display
     */
    public boolean isBurstThumbnailNotInvalid() {
        return mCancelBurst && !mThumbnailHasInvalid;
    }
    public void restoreCancelBurstTag() {
        mCancelBurst = false;
        mThumbnailHasInvalid = false;
    }

    private MakeupController mMakeupController;

    @Override
    public void setMakeUpController(MakeupController makeUpController) {
        mMakeupController = makeUpController;
    }

    public boolean isUcamBeautyCanBeUsed() {
        return UCamUtill.isUcamBeautyEnable() && !isImageCaptureIntent();
    }

    @Override
    public void updateMakeLevel() {
        /* SPRD: fix bug 474672 add for beauty @{ */

        Log.d(TAG,
                "updateMakeLevel "
                        + DataModuleManager.getInstance(mActivity)
                                .getCurrentDataModule()
                                .getBoolean(Keys.KEY_CAMERA_BEAUTY_ENTERED));
        // mMakeupSeekBar != null means this module contains makeupToggleButton
        // in top panel
        if (isUcamBeautyCanBeUsed()
                && DataModuleManager.getInstance(mActivity)
                        .getCurrentDataModule()
                        .isEnableSettingConfig(Keys.KEY_CAMERA_BEAUTY_ENTERED)) {
            if (mMakeupController == null) {
                return;
            }
            if (mDataModuleCurrent.getBoolean(Keys.KEY_CAMERA_BEAUTY_ENTERED)) {
                mMakeupController.resumeMakeupControllerView();
            } else {
                mMakeupController.pauseMakeupControllerView();
            }
        }
        /* @} */
    }
    @Override
    public boolean isPhotoFocusing(){
        return mCameraState == FOCUSING;
    }

    protected boolean mCameraAvailable = false;

    @Override
    public boolean isCameraAvailable(){
        return mCameraAvailable;
    }
}
