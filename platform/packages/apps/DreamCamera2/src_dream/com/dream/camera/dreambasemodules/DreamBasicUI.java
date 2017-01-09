package com.dream.camera.dreambasemodules;

import android.view.View;
import android.view.ViewGroup;

import com.android.camera.ButtonManager;
import com.android.camera.CameraActivity;
import com.android.camera.debug.Log;
import com.android.camera.PhotoController;
import com.android.camera.PhotoUI;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;

import com.android.camera2.R;

import com.dream.camera.ButtonManagerDream;
import com.dream.camera.dreambasemodules.DreamInterface;
import com.dream.camera.SlidePanelManager;
import com.dream.camera.settings.DataConfig;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataStructSetting;
import com.dream.camera.settings.DreamUIPreferenceSettingLayout;
import com.dream.camera.settings.DreamUIPreferenceSettingLayout.SettingUIListener;
import com.dream.camera.util.DreamUtil;
import com.sprd.camera.aidetection.AIDetectionController;
import com.ucamera.ucam.modules.BasicModule;
import com.ucamera.ucam.modules.ui.BasicUI;
import com.dream.camera.DreamOrientation;

public abstract class DreamBasicUI extends BasicUI implements DreamInterface, SettingUIListener {
    private static final Log.Tag TAG = new Log.Tag("DreamBasicUI");
    protected BasicModule mBasicModule;

    public DreamBasicUI(int cameraId, CameraActivity activity, PhotoController controller,
            View parent) {
        super(cameraId, activity, controller, parent);
        activity.getCameraAppUI().setDreamInterface(this);
    }

    @Override
    public void initUI() {

        mBasicModule = (BasicModule) mController;

        // Generate a view to fit top panel.
        ViewGroup topPanelParent = (ViewGroup) mRootView
                .findViewById(R.id.top_panel_parent);
        topPanelParent.removeAllViews();
        updateTopPanelValue(mActivity);
        fitTopPanel(topPanelParent);

        // Update visibilities of state icons on side panel.
        updateSidePanel();

        // Generate views to fit extend panel.
        ViewGroup extendPanelParent = (ViewGroup) mRootView
                .findViewById(R.id.extend_panel_parent);
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
        SlidePanelManager.getInstance(mActivity).focusItem(
                SlidePanelManager.CAPTURE, false);
    }

    @Override
    public void updateBottomPanel() {
        mActivity.getCameraAppUI().updateSwitchModeBtn(this);
    }

    public void bindSettingsButton(View settingsButton) {
        if (settingsButton != null) {
            final DreamUIPreferenceSettingLayout dps = (DreamUIPreferenceSettingLayout) mRootView
                    .findViewById(R.id.dream_ui_preference_setting_layout);
            dps.changeModule(DreamBasicUI.this);
            settingsButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    /*SPRD:fix bug606617 setting can not click when capture @*/
                    if(mBasicModule.isShutterClicked()) {
                        return;
                    }
                    /* @} */
                    // mActivity.onSettingsSelected();
                    /*
                     * DataStructSetting dataSetting = new DataStructSetting(
                     * DataConfig.CategoryType.CATEGORY_PHOTO, false,
                     * DataConfig.PhotoModeType.PHOTO_MODE_BACK_REFOCUS, 1); //
                     * change the data storage module
                     * DataModuleManager.getInstance
                     * (mActivity).changeModuleStatus(dataSetting);
                     */
                    // update UI
                    dps.changeVisibilty(View.VISIBLE);
                    mActivity.getCameraAppUI().updatePreviewUI(View.GONE);
                }
            });
        }
    }

    public void bindFlashButton() {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity
                .getButtonManager();
        buttonManager.initializeButton(ButtonManagerDream.BUTTON_FLASH_DREAM,
                mBasicModule.mFlashCallback);
    }

    public void bindCountDownButton() {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity
                .getButtonManager();
        buttonManager.initializeButton(
                ButtonManagerDream.BUTTON_COUNTDOWN_DREAM, null);
    }

    public void bindHdrButton() {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity
                .getButtonManager();
        buttonManager.initializeButton(ButtonManagerDream.BUTTON_HDR_DREAM,
                mBasicModule.mHdrPlusCallback);
    }

    public void bindCameraButton() {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity
                .getButtonManager();
        buttonManager.initializeButton(ButtonManagerDream.BUTTON_CAMERA_DREAM,
                mBasicModule.mCameraCallback);
    }

    public void bindMeteringButton() {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity
                .getButtonManager();
        buttonManager.initializeButton(
                ButtonManagerDream.BUTTON_METERING_DREAM, null);
    }

    private ButtonManager.ButtonCallback getDisableCameraButtonCallback(
            final int conflictingButton) {
        return new ButtonManager.ButtonCallback() {
            @Override
            public void onStateChanged(int state) {
                mActivity.getButtonManager().disableButton(conflictingButton);
            }
        };
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
}
