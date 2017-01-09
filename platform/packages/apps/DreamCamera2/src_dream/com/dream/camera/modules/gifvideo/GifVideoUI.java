
package com.dream.camera.modules.gifvideo;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;
import com.android.camera.ButtonManager;
import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.debug.Log;
import com.ucamera.ucam.modules.ugif.GifModule;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;

import com.android.camera2.R;

import com.dream.camera.ButtonManagerDream;
import com.dream.camera.dreambasemodules.DreamInterface;
import com.dream.camera.SlidePanelManager;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.util.DreamUtil;
import com.dream.camera.dreambasemodules.DreamGifUI;

public class GifVideoUI extends DreamGifUI {

    private ImageButton mSettingsButton;
    private View topPanel;
    private DreamUtil mDreamUtil = new DreamUtil();

    public GifVideoUI(int cameraId, PhotoController baseController, CameraActivity activity,
            View parent) {
        super(cameraId, baseController, activity, parent);
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {
        topPanelParent.removeAllViews();
        LayoutInflater lf = LayoutInflater.from(mActivity);
        if (DreamUtil.BACK_CAMERA == mDreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {
            topPanel = lf.inflate(R.layout.gifvideo_top_panel, topPanelParent);
        } else {
            topPanel = lf.inflate(R.layout.gifvideo_front_top_panel,
                    topPanelParent);
        }

        mActivity.getButtonManager().load(topPanel);

        if(DreamUtil.BACK_CAMERA == mDreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))){
            bindFlashButton();
        }

        mSettingsButton = (ImageButton) topPanel.findViewById(R.id.settings_button_dream);

        bindSettingsButton(mSettingsButton);

        bindCameraButton();
    }

    @Override
    public void updateSidePanel() {
        super.updateSidePanel();
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {

    }

    @Override
    public void updateBottomPanel() {
        super.updateBottomPanel();
    }

    @Override
    public void updateSlidePanel() {
        super.updateSlidePanel();
    }

    public void bindFlashButton() {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity.getButtonManager();
        buttonManager.initializeButton(ButtonManagerDream.BUTTON_VIDEO_FLASH_DREAM,
                mBasicModule.mFlashCallback);
    }

    @Override
    public void updateGifFlashTopPanel(ButtonManagerDream buttonManager,
            boolean disable) {
        if (DreamUtil.BACK_CAMERA == mDreamUtil
                .getRightCamera(DataModuleManager.getInstance(mActivity)
                        .getDataModuleCamera().getInt(Keys.KEY_CAMERA_ID))
                && DataModuleManager.getInstance(mActivity)
                        .getCurrentDataModule()
                        .isEnableSettingConfig(Keys.KEY_VIDEOCAMERA_FLASH_MODE)) {
            if (buttonManager != null && !mBasicModule.mIsBatteryLow) {
                buttonManager.updateGifFlashButton(ButtonManagerDream.BUTTON_VIDEO_FLASH_DREAM, disable);
            }
        }
    }
}
