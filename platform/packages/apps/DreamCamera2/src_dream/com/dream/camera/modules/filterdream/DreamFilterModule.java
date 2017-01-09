
package com.dream.camera.modules.filterdream;

import java.util.HashMap;

import com.android.camera.CameraActivity;
import com.android.camera.app.AppController;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.util.DreamUtil;
import com.dream.camera.dreambasemodules.DreamBasicUI;
import com.thundersoft.advancedfilter.TsAdvancedFilterNative;
import com.ucamera.ucam.modules.BasicModule;
import com.ucamera.ucam.modules.ufilter.UcamFilterPhotoModule;
import com.android.camera.PhotoController;
import android.view.View;
import android.widget.Toast;
import com.android.camera2.R;
import com.android.camera.debug.Log;
import com.android.camera.debug.Log.Tag;
import com.android.camera.one.OneCamera;
import com.android.camera.one.OneCameraAccessException;
import com.android.camera.settings.CameraPictureSizesCacher;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsScopeNamespaces;
import com.android.camera.util.CameraUtil;
import com.android.camera.util.GcamHelper;
import com.android.camera.util.Size;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

public class DreamFilterModule extends UcamFilterPhotoModule {
    public static final String DREAMSCENERY_MODULE_STRING_ID = "DreamFilterModule";
    private static final Tag TAG = new Tag(DREAMSCENERY_MODULE_STRING_ID);
    private DreamFilterModuleUI mFilterUI;

    @Override
    protected void switchCamera() {
        // super.switchCamera();
        // SPRD: ui check 213.
        /* SPRD: Fix bug 597195 the freeze screen for switch @{ */
        freezeScreen(CameraUtil.isFreezeBlurEnable(), CameraUtil.isSwitchAnimationEnable());
        mHandler.post(new Runnable() {

            @Override
            public void run() {
                if (!mPaused) {
                    mActivity.switchFrontAndBackMode();
                    mActivity.getCameraAppUI().updateModeList();
                }
            }
        });
        /* @} */

    }

    public DreamFilterModule(AppController app) {
        super(app);
    }

    @Override
    public void setPhotoCameraID() { };

    @Override
    public void makeModuleUI(PhotoController controller, View parent) {
        Log.d(TAG, "makeModuleUI E.");
        // initialize BaseUI object
        mUI = new DreamFilterModuleUI(mCameraId, mActivity, controller, parent);
        mFilterUI = (DreamFilterModuleUI) mUI;
        mActivity.setPreviewStatusListener(mUI);
        mAdvancedFilterGLView = mFilterUI.getAdvancedFilterGLView();
        mSmallAdvancedFilter = mFilterUI.getSmallAdvancedFilter();

        initializeModuleControls();
        Log.d(TAG, "makeModuleUI X.");
    }

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
    public void initCameraID() {
    }

    @Override
    public void delayUiPause() {
        mFilterUI.delayUiPause();
    }

    @Override
    public void pause() {
        // SPRD: nj dream camera debug 117
        //mDataModuleCurrent.set(Keys.KEY_CAMERA_FILTER_TYPE,mSmallAdvancedFilter.getFilterType());//SPRD:fix bug626090
        mDataModuleCurrent.destroy();
        super.pause();
    }

    /* SPRD:fix bug605522 filter mode will normal preview when back camera @{*/
    @Override
    public void destroy() {
        if (mFilterUI != null) {
            mFilterUI.destroy();
        }
    }
    /* @} */

    /*
     * SPRD: nj dream camera debug 117. @{
     */
    @Override
    public void onPreviewStartedAfter() {
        super.onPreviewStartedAfter();
        int type = mDataModuleCurrent.getInt(Keys.KEY_CAMERA_FILTER_TYPE);
        mSmallAdvancedFilter.setDreamFilterType(type);
    }
    /*
     * @}
     */

    @Override
    public void onDreamSettingChangeListener(HashMap<String, String> keys) {
        Log.e(TAG, "dreamPhotoonDreamSettingChangeListener  ");
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
            case Keys.KEY_PICTURE_SIZE_BACK:
            case Keys.KEY_PICTURE_SIZE_FRONT:
                freezeScreen(CameraUtil.isFreezeBlurEnable(),false);
                stopPreview();
                startPreview();
                break;
            case Keys.KEY_JPEG_QUALITY:
                updateParametersPictureQuality();
                break;
            case Keys.KEY_CAMERA_GRID_LINES:
                updateParametersGridLine();
                break;
            case Keys.KEY_CAMERA_AI_DATECT:
                updateFace();
                // SPRD:Modify for ai detect
                mUI.intializeAIDetection(mDataModuleCurrent);
                break;
            case Keys.KEY_CAMER_ANTIBANDING:
                updateParametersAntibanding();
                break;
            case Keys.KEY_FRONT_CAMERA_MIRROR:
                updateParametersMirror();
                break;
            case Keys.KEY_FLASH_MODE:
                if (mPaused
                        || mAppController.getCameraProvider()
                                .waitingForCamera()) {
                    return;
                }
                updateParametersFlashMode();
                break;
            case Keys.KEY_CAMERA_FILTER_TYPE:
                hideFaceForSpecialFilter();
                break;
            }
        }
         mActivity.getCameraAppUI().initSidePanel();

         if (mCameraDevice != null) {
             mCameraDevice.applySettings(mCameraSettings);
             mCameraSettings = mCameraDevice.getSettings();
         }
    }

    public void hideFaceForSpecialFilter() {
        if (!mAppController.getAndroidContext()
                .getResources().getString(R.string.pref_ai_detect_entry_value_face)
                .equals(mDataModuleCurrent.getString(Keys.KEY_CAMERA_AI_DATECT))) {
            return;
        }
        int mFilterType = mDataModuleCurrent.getInt(Keys.KEY_CAMERA_FILTER_TYPE);
        Log.e(TAG, "mFilterType  = " + mFilterType);
        if (mFilterType == 103 || mFilterType == 101 || mFilterType == 405) {
            stopFaceDetection();
        } else {
            startFaceDetection();
        }
    }

    /* SPRD: nj dream camera test debug 120 @{ */
    public void updateFilterPanelUI(int visible) {
        if (mSmallAdvancedFilter != null) {
            mSmallAdvancedFilter.setVisibility(visible);
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
            Size size = mAppController.getResolutionSetting().getPictureSize(
                    DataModuleManager.getInstance(mAppController.getAndroidContext()),
                    mAppController.getCameraProvider().getCurrentCameraId(), cameraFacing);
            Size preferredMaxSize;
            if ((float) size.width() / (float) size.height() > 4.0f / 3.0f) {
                preferredMaxSize = new Size(2048, 1152);
            } else {
                preferredMaxSize = new Size(2048, 1536);
            }
            pictureSizeRestricted = mAppController.getResolutionSetting()
                    .getPictureSize(
                            DataModuleManager.getInstance(mAppController
                                    .getAndroidContext()),
                            mAppController.getCameraProvider()
                                    .getCurrentCameraId(), cameraFacing,
                            preferredMaxSize);
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
    public int getModuleTpye() {
        return FILTER_MODULE;
    }
}
