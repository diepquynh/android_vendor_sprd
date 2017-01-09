
package com.dream.camera.modules.gifvideo;

import com.android.camera.PhotoController;
import com.android.camera.app.AppController;
import com.android.camera.app.CameraAppUI;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.dream.camera.ButtonManagerDream;
import com.dream.camera.dreambasemodules.DreamGifModule;
import android.view.View;
import com.dream.camera.util.DreamUtil;
import com.ucamera.ucam.modules.ugif.GifUI;
import com.dream.camera.modules.gifvideo.GifVideoUI;
import com.dream.camera.settings.DataModuleBasic;
import com.android.ex.camera2.portability.CameraCapabilities;

public class GifVideoModule extends DreamGifModule {

    public GifVideoModule(AppController app) {
        super(app);
    }

    @Override
    public GifUI createUI(PhotoController controller, View parent) {
        return new GifVideoUI(mCameraId, controller, mActivity, parent);
    }

    @Override
    public String getFlashMode(DataModuleBasic dataModuleCurrent) {
        return dataModuleCurrent.getString(Keys.KEY_VIDEO_FLASH_MODE);
    }

    @Override
    public boolean isFlashEnable(){
        return mAppController.getButtonManager().isEnabled(ButtonManagerDream.BUTTON_VIDEO_FLASH_DREAM);
    }

    @Override
    public int getMode() {
        return DreamUtil.VIDEO_MODE;
    }
    @Override
    public CameraAppUI.BottomBarUISpec getBottomBarSpec() {
        CameraAppUI.BottomBarUISpec bottomBarSpec = new CameraAppUI.BottomBarUISpec();

        bottomBarSpec.enableCamera = true;
        bottomBarSpec.cameraCallback = mCameraCallback;
        bottomBarSpec.enableTorchFlash = true;
        bottomBarSpec.flashCallback = mFlashCallback;

        return bottomBarSpec;
    }

    /* SPRD:fix bug 595608 modify for flash always flash when back from thumbnail @{ */
    @Override
    public void enableTorchMode(boolean enable) {
        if (mParameters == null) {
            return;
        }
        CameraCapabilities.Stringifier stringifier = mCameraCapabilities
                .getStringifier();
        CameraCapabilities.FlashMode flashMode;

        if (enable && !mIsBatteryLow) {
            flashMode = stringifier.flashModeFromString(mDataModuleCurrent
                    .getString(Keys.KEY_VIDEOCAMERA_FLASH_MODE));
        } else {
            flashMode = CameraCapabilities.FlashMode.OFF;
        }

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
    }
    /* @} */
    @Override
    public void updateBatteryLevel(int level) {
        super.updateBatteryLevel(level, Keys.KEY_VIDEOCAMERA_FLASH_MODE, ButtonManagerDream.BUTTON_VIDEO_FLASH_DREAM);
    }
}
