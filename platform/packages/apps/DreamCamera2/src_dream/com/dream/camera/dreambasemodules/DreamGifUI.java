
package com.dream.camera.dreambasemodules;

import android.view.View;
import android.view.ViewGroup;

import com.android.camera.ButtonManager;
import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.debug.Log;
import com.ucamera.ucam.modules.ugif.GifModule;
import com.ucamera.ucam.modules.ugif.GifUI;
import com.android.camera.settings.SettingsManager;

import com.android.camera2.R;

import com.dream.camera.ButtonManagerDream;
import com.dream.camera.DreamUI;
import com.dream.camera.dreambasemodules.DreamInterface;
import com.dream.camera.SlidePanelManager;
import com.dream.camera.settings.DreamUIPreferenceSettingLayout;
import com.dream.camera.settings.DreamUIPreferenceSettingLayout.SettingUIListener;
import com.dream.camera.util.DreamUtil;

public abstract class DreamGifUI extends GifUI implements DreamInterface, SettingUIListener{

    protected GifModule mBasicModule;

    public DreamGifUI(int cameraId, PhotoController baseController, CameraActivity activity,
            View parent) {
        super(cameraId, baseController, activity, parent);
        activity.getCameraAppUI().setDreamInterface(this);
    }

    @Override
    public void initUI() {

        mBasicModule = (GifModule) mController;

        // Generate a view to fit top panel.
        ViewGroup topPanelParent = (ViewGroup) mRootView.findViewById(R.id.top_panel_parent);
        topPanelParent.removeAllViews();
        updateTopPanelValue(mActivity);
        fitTopPanel(topPanelParent);

        // Update visibilities of state icons on side panel.
        updateSidePanel();

        // Generate views to fit extend panel.
        ViewGroup extendPanelParent = (ViewGroup) mRootView.findViewById(R.id.extend_panel_parent);
        extendPanelParent.removeAllViews();
        fitExtendPanel(extendPanelParent);

        // Update icons on bottom panel.
        updateBottomPanel();

        // Update item on slide panel.
        updateSlidePanel();
    }

    @Override
    public void updateSlidePanel() {
        SlidePanelManager.getInstance(mActivity).udpateSlidePanelShow(SlidePanelManager.FILTER,View.GONE);
        SlidePanelManager.getInstance(mActivity).focusItem(SlidePanelManager.CAPTURE, false);
    }

    @Override
    public void updateBottomPanel() {
        mActivity.getCameraAppUI().updateSwitchModeBtn(this);
        mBasicModule.onSwitchChanged(null,
                mActivity.getCurrentModule().getMode() == DreamUtil.VIDEO_MODE);
    }

    public void bindSettingsButton(View settingsButton) {
        if (settingsButton != null) {
            final DreamUIPreferenceSettingLayout dps = (DreamUIPreferenceSettingLayout) mRootView
                    .findViewById(R.id.dream_ui_preference_setting_layout);
            dps.changeModule(DreamGifUI.this);
            settingsButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    // mActivity.onSettingsSelected();
                    // update UI
                    dps.changeVisibilty(View.VISIBLE);
                    mActivity.getCameraAppUI().updatePreviewUI(View.GONE);
                }
            });
        }
    }

    public void bindCameraButton() {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity.getButtonManager();
        buttonManager.initializeButton(ButtonManagerDream.BUTTON_CAMERA_DREAM,
                mBasicModule.mCameraCallback);
    }

    protected int sidePanelMask;

    @Override
    public void updateSidePanel() {
        sidePanelMask = DreamUtil.SP_EXTERNAL_STORAGE | DreamUtil.SP_INTERNAL_STORAGE
                | DreamUtil.SP_USB_STORAGE | DreamUtil.SP_FACE_DETECT
                | DreamUtil.SP_LOCATE | DreamUtil.SP_TOUCH_CAPTURE
                | DreamUtil.SP_OPPOSITE | DreamUtil.SP_CA;
    }

    @Override
    public int getSidePanelMask() {
        return sidePanelMask;
    }

    @Override
    public void onSettingChanged(SettingsManager settingsManager, String key) {
    }

    /**
     * update preview ui after settings closed
     */
    public void onSettingUIHide() {
        mActivity.getCameraAppUI().updatePreviewUI(View.VISIBLE);
    }
    /* @} */

    public void updateGifSettingAndSwitch(boolean disable) {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity.getButtonManager();
        if (disable) {
            buttonManager.disableButton(ButtonManagerDream.BUTTON_SETTING_DREAM);
            buttonManager.disableCameraButtonAndBlock();
            updateGifFlashTopPanel(buttonManager,disable);
        } else {
            buttonManager.enableButton(ButtonManagerDream.BUTTON_SETTING_DREAM);
            buttonManager.enableCameraButton();
            updateGifFlashTopPanel(buttonManager,disable);
        }
    }

    @Override
    public int getUITpye() {
        return DreamUI.DREAM_GIF_UI;
    }

    /* SPRD: fix bug597435 add related logic so that flash is not available @{ */
    public void updateGifFlashTopPanel(ButtonManagerDream buttonManager,boolean disable){}
    /* @} */
}
