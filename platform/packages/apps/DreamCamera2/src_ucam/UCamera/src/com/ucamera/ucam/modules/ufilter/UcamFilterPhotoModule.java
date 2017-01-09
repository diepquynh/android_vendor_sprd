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

package com.ucamera.ucam.modules.ufilter;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.hardware.Camera;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.View;
import android.widget.Toast;

import com.android.camera.CameraActivity;
import com.android.camera.module.ModuleController;
import com.android.camera.settings.SettingsScopeNamespaces;
import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.android.camera.PhotoController;
import com.android.camera.app.AppController;
import com.android.camera.app.CameraAppUI;
import com.android.camera.debug.Log;

import com.android.camera.module.ModuleController;
import com.android.camera.one.OneCamera;
import com.android.camera.one.OneCameraAccessException;
import com.android.camera.settings.CameraPictureSizesCacher;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.settings.SettingsScopeNamespaces;
import com.android.camera.settings.SettingsUtil;
import com.android.camera.util.CameraUtil;
import com.android.camera.util.Size;
import com.android.camera2.R;
import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.android.ex.camera2.portability.CameraDeviceInfo.Characteristics;
import com.dream.camera.ButtonManagerDream;
import com.thundersoft.advancedfilter.FilterTranslationUtils;
import com.thundersoft.advancedfilter.SmallAdvancedFilter;
import com.thundersoft.advancedfilter.TsAdvancedFilterGLView;
import com.ucamera.ucam.modules.BasicModule;
import com.ucamera.ucam.modules.ui.UcamFilterPhotoUI;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import com.android.ex.camera2.portability.CameraDeviceInfo.Characteristics;

public class UcamFilterPhotoModule extends BasicModule {

    private static final Log.Tag TAG = new Log.Tag(SettingsScopeNamespaces.FILTER);

    private Size mSize;
    private SettingsManager mSettingsManager;
    protected TsAdvancedFilterGLView mAdvancedFilterGLView;
    protected SmallAdvancedFilter mSmallAdvancedFilter;
    private UcamFilterPhotoUI mFilterUI;
    private HandlerThread mPreviewDataCallbackThread = null;

    // For calculate the best fps range for preview.
    private final static int MAX_PREVIEW_FPS_TIMES_1000 = 400000;
    private final static int PREFERRED_PREVIEW_FPS_TIMES_1000 = 20000;

    public UcamFilterPhotoModule(AppController app) {
        super(app);
    }

    @Override
    public void init(CameraActivity activity, boolean isSecureCamera, boolean isCaptureIntent) {
        super.init(activity, isSecureCamera, isCaptureIntent);

        FilterTranslationUtils.getInstance().setCameraId(mCameraId);
        FilterTranslationUtils.getInstance().setMMatrix();

        Characteristics infoF = activity.getCameraProvider()
                .getCharacteristics(Camera.CameraInfo.CAMERA_FACING_FRONT);
        Characteristics infoB = activity.getCameraProvider()
                .getCharacteristics(Camera.CameraInfo.CAMERA_FACING_BACK);
        FilterTranslationUtils.getInstance()
                .setSensorOrientation(infoF.getSensorOrientation(), infoB.getSensorOrientation());

        /* SPRD:Add for bug 458160 filter mode's resolution is not the same as photo mode's @{ */
        //Toast.makeText(mActivity, R.string.filter_warning, Toast.LENGTH_LONG).show();//SPRD:fix bug605019
        /* @} */
    }

    /**
     * initialize module controls
     */
    protected void initializeModuleControls() {
    }

    @Override
    public void makeModuleUI(PhotoController controller, View parent) {
        Log.d(TAG, "makeModuleUI E.");
        // initialize BaseUI object
        mUI = new UcamFilterPhotoUI(mCameraId, mActivity, controller, parent);
        mFilterUI = (UcamFilterPhotoUI) mUI;
        mActivity.setPreviewStatusListener(mUI);
        mAdvancedFilterGLView = mFilterUI.getAdvancedFilterGLView();
        mSmallAdvancedFilter = mFilterUI.getSmallAdvancedFilter();

        mSettingsManager = mActivity.getSettingsManager();
        mSettingsManager.setToDefault(mSettingsManager.SCOPE_GLOBAL, Keys.KEY_CAMERA_TIME_STAMP);
        mCurrentModule = SettingsScopeNamespaces.FILTER;
        initializeModuleControls();
        Log.d(TAG, "makeModuleUI X.");
    }

    /**
     * Uses the {@link CameraProvider} to open the currently-selected camera
     * device, using {@link GservicesHelper} to choose between API-1 and API-2.
     */
    @Override
    protected void requestCameraOpen() {
        Log.i(TAG, "requestCameraOpen mCameraId: " + mCameraId);
        mActivity.getCameraProvider().requestCamera(mCameraId, false);
        FilterTranslationUtils.getInstance().setCameraId(mCameraId);
        FilterTranslationUtils.getInstance().setMMatrix();
    }

    @Override
    public void updateFocusUI() {
        mFocusManager.updateFocusUI(); // Ensure focus indicator is hidden.
    }

    @Override
    public void setPreviewDataCallback(boolean does) {
        if (mCameraDevice == null) {
            return;
        }

        if (does) {
            mPreviewDataCallbackThread = new HandlerThread("preview thread");
            mPreviewDataCallbackThread.start();
            Handler handler = new Handler(mPreviewDataCallbackThread.getLooper());

            mCameraDevice.setPreviewDataCallback(handler, new MagiclensFrameCallback());
            mActivity.getCameraAppUI().resumeTextureViewRendering();
        } else {
            mCameraDevice.setPreviewDataCallback(null, null);
            if (mPreviewDataCallbackThread != null) {
                mPreviewDataCallbackThread.quitSafely();
                mPreviewDataCallbackThread = null;
            }
        }
    }

    private final class MagiclensFrameCallback implements CameraAgent.CameraPreviewDataCallback {
        @Override
        public void onPreviewFrame(byte[] data, CameraProxy camera) {
            if (mCameraDevice == null) {
                Log.i(TAG, "mCameraDevice==" + mCameraDevice);
                return;
            }

            mAdvancedFilterGLView.setPreviewSize(mCameraSettings.getCurrentPreviewSize());
            mAdvancedFilterGLView.setPreviewData(data);
            /* SPRD: no need to do this currently @{
            if (mSmallAdvancedFilter.isSupportRealPreviewThum()) {
                mSmallAdvancedFilter.setPreviewData(data, size.width(), size.height());
            }
            @} */
        }
    }

    public Bitmap getPreviewBitmap() {
        return mAdvancedFilterGLView.getPreviewData();
    }

    @Override
    public void freezeScreen(boolean needBlur, boolean needSwitch) {
        // disable preview data callback to pause preview
        setPreviewDataCallback(false);
        Bitmap freezeBitmap = null;
        freezeBitmap = getPreviewBitmap();
        if (needBlur || needSwitch) {
            freezeBitmap = CameraUtil.blurBitmap(CameraUtil.computeScale(freezeBitmap, 0.2f), (Context)mActivity);
        }

        if (needSwitch) {
            mAppController.getCameraAppUI().startSwitchAnimation(freezeBitmap);
        }
        // freezeScreen with the preview bitmap
        mAppController.freezeScreenUntilPreviewReady(freezeBitmap);

        // enable preview data callback to resume preview
        setPreviewDataCallback(true);
    }

    @Override
    public void onPreviewStartedAfter() {
        mActivity.getCameraAppUI().textureViewRequestLayout();
    }

    @Override
    public void initCameraID() {
        mSettingsManager.set(mAppController.getModuleScope(),
                Keys.KEY_CAMERA_ID, mActivity.getCameraId());
    }

    @Override
    public void setPhotoCameraID() {
        int photoIndex = mActivity.getResources().getInteger(R.integer.camera_mode_photo);
        mSettingsManager.set(mActivity.getModuleScope(photoIndex), Keys.KEY_CAMERA_ID, mCameraId);
    };

    @Override
    public void resume() {
        super.resume();
        mSmallAdvancedFilter.setUcamFilterPhotoModule(this);
        if (mActivity.getCameraAppUI() != null) {
            mActivity.getCameraAppUI().setPanelsVisibilityListener(mFilterUI);
        }
        setFilterHandle(true);
    }

    @Override
    public void pause() {
        if (mActivity.getCameraAppUI() != null) {
            mActivity.getCameraAppUI().setPanelsVisibilityListener(null);
            mActivity.getCameraAppUI().resumeTextureViewRendering();
        }
        super.pause();
        if (mActivity.getCameraAppUI() != null) {
            mActivity.getCameraAppUI().setPanelsVisibilityListener(null);
        }
        setFilterHandle(false);
    }

    @Override
    public void onShutterButtonClick() {
        super.onShutterButtonClick();
        if (mFilterUI != null) {
            mFilterUI.setFilterButtonEnabled(false);
        }
    }

    /* SPRD: Add for bug 567784 @{ */
    @Override
    protected void cancelCountDown() {
        super.cancelCountDown();
        if (mFilterUI != null) {
            mFilterUI.setFilterButtonEnabled(true);
        }
    }
    /* @} */

    @Override
    protected void setupPreview() {
        super.setupPreview();
        if (mFilterUI != null) {
            mFilterUI.setFilterButtonEnabled(true);
        }
        setFilterHandle(false);
    }

/*    public void hideFaceForSpecialFilter() {
        mSmallAdvancedFilter.hideFaceForSpecialFilter(true);
    }*/

    @Override
    public void initBottomBarSpec(CameraAppUI.BottomBarUISpec bottomBarSpec) {
        super.initBottomBarSpec(bottomBarSpec);
        bottomBarSpec.hideHdr = true;
    }

    @Override
    public void updateFreeze(Uri uri) {
        if (uri != null) {
            mActivity.notifyNewMedia(uri);
        }
    }

    /* SPRD: fix bug549564  CameraProxy uses the wrong API @{ */
    @Override
    public boolean checkCameraProxy() {
        return !getCameraProvider().isNewApi() &&
                (mCameraId == getCameraProvider().getCurrentCameraId().getLegacyValue());
    }
    /* @} */

    public void delayUiPause() {
        mFilterUI.delayUiPause();
    }

    @Override
    protected void updateCameraParametersInitialize() {
        /* SPRD: Fix bug 590671, select filter fps range @{ */
        if (CameraUtil.isFilterHighFpsEnable()) {
            List<int[]> fpsRanges = mCameraCapabilities.getSupportedPreviewFpsRange();
            int[] fpsRange = getFilterPreviewFpsRange(fpsRanges);
            if (fpsRange != null && fpsRange.length > 0) {
                mCameraSettings.setPreviewFpsRange(fpsRange[0], fpsRange[1]);
                Log.d(TAG, "preview fps: " + fpsRange[0] + ", " + fpsRange[1]);
                return;
            }
        }
        /* @} */
        // The list is sorted. Return the first element.
        List<int[]> fpsRange = mCameraCapabilities.getSupportedPreviewFpsRange();

        int minFps = (fpsRange.get(0))[Camera.Parameters.PREVIEW_FPS_MIN_INDEX];
        int maxFps = (fpsRange.get(0))[Camera.Parameters.PREVIEW_FPS_MAX_INDEX];
        mCameraSettings.setPreviewFpsRange(minFps, maxFps);
        Log.d(TAG, "preview fps: " + minFps + ", " + maxFps);
    }

    public static int[] getFilterPreviewFpsRange(List<int[]> frameRates) {
        if (frameRates.size() == 0) {
            Log.w(TAG, "No suppoted frame rates returned!");
            return null;
        }

        // Find range in supported ranges who can cover 20fps.
        List<int[]> selectedFrameRates = new ArrayList<>();
        for (int[] rate : frameRates) {
            int minFps = rate[0];
            int maxFps = rate[1];
            if (maxFps >= PREFERRED_PREVIEW_FPS_TIMES_1000 &&
            minFps <= PREFERRED_PREVIEW_FPS_TIMES_1000) {
                selectedFrameRates.add(rate);
            }
        }

        // pick the one with lowest max rate from selected ranges who cover 20fps
        int resultIndex = -1;
        int lowestMaxRate = MAX_PREVIEW_FPS_TIMES_1000;
        for (int i = 0; i < selectedFrameRates.size(); i++) {
            int[] rate = selectedFrameRates.get(i);
            int maxFps = rate[1];
            if (maxFps <= lowestMaxRate) {
                lowestMaxRate = maxFps;
                resultIndex = i;
            }
        }

        if (resultIndex >= 0) {
            return selectedFrameRates.get(resultIndex);
        }
        Log.w(TAG, "Can't find an appropiate frame rate range!");
        return null;
    }

    /* SPRD:Fix bug 558883 The filterUI is wrong.@{ */
    @Override
    public void onPreviewVisibilityChanged(int visibility) {
        if (mAdvancedFilterGLView == null || mSmallAdvancedFilter == null) {
            return;
        }
        if (mActivity.isFilmstripVisible() && visibility == ModuleController.VISIBILITY_HIDDEN) {
            mAdvancedFilterGLView.setVisibility(View.GONE);
        } else {
            mAdvancedFilterGLView.setVisibility(View.VISIBLE);
        }
    }
    /* @} */

    @Override
    protected void updateParametersPictureSize() {
        if (mCameraDevice == null) {
            Log.i(TAG, "attempting to set picture size without caemra device");
            return;
        }

        List<Size> supported = Size.convert(mCameraCapabilities.getSupportedPhotoSizes());
        CameraPictureSizesCacher.updateSizesForCamera(mAppController.getAndroidContext(),
                mCameraDevice.getCameraId(), supported);

        OneCamera.Facing cameraFacing =
                isCameraFrontFacing() ? OneCamera.Facing.FRONT : OneCamera.Facing.BACK;
        Size pictureSizeRestricted;
        try {
            // not bigger than 300 megapixels, hard-code max resolution here temporarily
            // Fix bug 563079 filter mode in not full screen
            SettingsManager settingsManager = mActivity.getSettingsManager();
            final String pictureSizeSettingKey = cameraFacing == OneCamera.Facing.FRONT ?
                    Keys.KEY_PICTURE_SIZE_FRONT : Keys.KEY_PICTURE_SIZE_BACK;
            Size size = SettingsUtil.sizeFromSettingString(
                    settingsManager.getString(SettingsManager.SCOPE_GLOBAL,
                            pictureSizeSettingKey));
            Size preferredMaxSize;
            if ((float) size.width() / (float) size.height() > 4.0f / 3.0f) {
                preferredMaxSize = new Size(2048, 1152);
            } else {
                preferredMaxSize = new Size(2048, 1536);
            }

            pictureSizeRestricted = mAppController.getResolutionSetting().getPictureSize(
                    mAppController.getCameraProvider().getCurrentCameraId(),
                    cameraFacing, preferredMaxSize);
        } catch (OneCameraAccessException ex) {
            mAppController.getFatalErrorHandler().onGenericCameraAccessFailure();
            return;
        }
        mCameraSettings.setPhotoSize(pictureSizeRestricted.toPortabilitySize());
        Log.i(TAG, "setting photo size: " + pictureSizeRestricted);

        // SPRD: add fix bug 555245 do not display thumbnail picture in MTP/PTP Mode at pc
        mCameraSettings.setExifThumbnailSize(CameraUtil.getAdaptedThumbnailSize(pictureSizeRestricted,
                mAppController.getCameraProvider()).toPortabilitySize());

        // Set a preview size that is closest to the viewfinder height and has
        // the right aspect ratio.
        List<Size> sizes = Size.convert(mCameraCapabilities.getSupportedPreviewSizes());
        Size optimalSize = CameraUtil.getOptimalPreviewSize(sizes,
                (double) pictureSizeRestricted.width() / pictureSizeRestricted.height());

        // Filter module does not support large sizes, we restrict them as follow.
        Size optimalSizeRestricted;
        if ((float) pictureSizeRestricted.width() / (float) pictureSizeRestricted.height() > 4.0f / 3.0f) {
            optimalSizeRestricted = new Size(1280, 720);
        } else {
            optimalSizeRestricted = new Size(1280, 960);
        }
        if (optimalSize.height() * optimalSize.width()
                > optimalSizeRestricted.height() * optimalSizeRestricted.width()) {
            if (sizes.contains(optimalSizeRestricted)) {
                optimalSize = optimalSizeRestricted;
            } else {
                Collections.sort(sizes, new Comparator<Size>() {
                    @Override
                    public int compare(Size lhs, Size rhs) {
                        // sorted in descending order
                        return rhs.height() * rhs.width() - lhs.height() * lhs.width();
                    }
                });
                for (Size size : sizes) {
                    if (size.height() * size.width()
                            > optimalSizeRestricted.height() * optimalSizeRestricted.width()) {
                        continue;
                    }
                    if (Math.abs((float)size.width() / (float)size.height()
                            - (float)pictureSizeRestricted.width() / (float)pictureSizeRestricted.height()) > 0.00001) {
                        continue;
                    }
                    optimalSize = size;
                }
            }
        }

        mCameraSettings.setPreviewSize(optimalSize.toPortabilitySize());
        mCameraDevice.applySettings(mCameraSettings);
        mCameraSettings = mCameraDevice.getSettings();

        if (optimalSize.width() != 0 && optimalSize.height() != 0) {
            mUI.updatePreviewAspectRatio((float) optimalSize.width()
                    / (float) optimalSize.height());
        }
        Log.d(TAG, "Preview size is " + optimalSize);
    }

    @Override
    public void updateBatteryLevel(int level) {
        super.updateBatteryLevel(level, Keys.KEY_FLASH_MODE, ButtonManagerDream.BUTTON_FLASH_DREAM);
    }
}
