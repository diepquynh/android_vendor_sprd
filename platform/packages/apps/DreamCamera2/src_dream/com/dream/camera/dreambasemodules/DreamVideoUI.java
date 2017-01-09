package com.dream.camera.dreambasemodules;

import android.graphics.drawable.Drawable;
import android.view.View;
import android.view.ViewGroup;

import com.android.camera.CameraActivity;
import com.android.camera.app.OrientationManager;
import com.android.camera.settings.SettingsManager;
import com.android.camera.ui.Rotatable;
import com.android.camera.MultiToggleImageButton;
import com.android.camera.VideoModule;
import com.android.camera.VideoUI;
import com.android.camera.VideoController;

import com.android.camera2.R;

import com.dream.camera.dreambasemodules.DreamInterface;
import com.dream.camera.ButtonManagerDream;
import com.dream.camera.SlidePanelManager;
import com.dream.camera.settings.DreamUIPreferenceSettingLayout;
import com.dream.camera.settings.DreamUIPreferenceSettingLayout.SettingUIListener;
import com.dream.camera.util.DreamUtil;
import com.dream.camera.DreamOrientation;

public abstract class DreamVideoUI extends VideoUI implements DreamInterface,
        SettingUIListener {

    protected VideoModule mBasicModule;

    public DreamVideoUI(CameraActivity activity, VideoController controller,
            View parent) {
        super(activity, controller, parent);
        activity.getCameraAppUI().setDreamInterface(this);
    }

    @Override
    public void initUI() {
        mBasicModule = (VideoModule) mController;
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
    public void updateBottomPanel() {
        mActivity.getCameraAppUI().updateSwitchModeBtn(this);
    }

    @Override
    public void updateSlidePanel() {
        SlidePanelManager.getInstance(mActivity).udpateSlidePanelShow(SlidePanelManager.FILTER,View.GONE);
        SlidePanelManager.getInstance(mActivity).focusItem(
                SlidePanelManager.CAPTURE, false);
    }

    protected int sidePanelMask;

    @Override
    public void updateSidePanel() {

        sidePanelMask = DreamUtil.SP_EXTERNAL_STORAGE | DreamUtil.SP_INTERNAL_STORAGE
                | DreamUtil.SP_USB_STORAGE | DreamUtil.SP_LOCATE | DreamUtil.SP_VA;
    }

    @Override
    public int getSidePanelMask() {
        return sidePanelMask;
    }

    @Override
    public void onSettingChanged(SettingsManager settingsManager, String key) {

    }

    public void bindSettingsButton(View settingsButton) {
        if (settingsButton != null) {
            final DreamUIPreferenceSettingLayout dps = (DreamUIPreferenceSettingLayout) mRootView
                    .findViewById(R.id.dream_ui_preference_setting_layout);
            dps.changeModule(DreamVideoUI.this);
            settingsButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if(mBasicModule.isShutterClicked()) {
                        return;
                    }
                    // mActivity.onSettingsSelected();
                    // update UI
                    if (!mRecordingStarted) {
                        dps.changeVisibilty(View.VISIBLE);
                        mActivity.getCameraAppUI().updatePreviewUI(View.GONE);
                    }
                }
            });
        }
    }

    public void bindFlashButton() {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity
                .getButtonManager();
        buttonManager.initializeButton(
                ButtonManagerDream.BUTTON_VIDEO_FLASH_DREAM,
                mBasicModule.mFlashCallback);
    }

    public void bindCameraButton() {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity
                .getButtonManager();
        buttonManager.initializeButton(ButtonManagerDream.BUTTON_CAMERA_DREAM,
                mBasicModule.mCameraCallback);
    }

    public void bindMakeupButton() {
        ButtonManagerDream buttonManager = (ButtonManagerDream) mActivity
                .getButtonManager();
        buttonManager.initializeButton(ButtonManagerDream.BUTTON_MAKE_UP_VIDEO_DREAM,
                mBasicModule.mMakeupCallback);
    }

    public void showRecordingUI(boolean recording, int orientation) {

        // SPRD Bug:474704 Feature:Video Recording Pause.
        if (recording == false)
            mPauseRecording = false;

        if (mActivity.getCameraAppUI().getPauseButton() != null) {
            mActivity.getCameraAppUI().getPauseButton()
                    .setImageResource(
                            mPauseRecording ? R.drawable.ic_start_sprd
                                    : R.drawable.ic_pause_sprd);
        }

        mRecordingStarted = recording;
        if (recording) {
            Drawable recordingIcon = mActivity.getResources().getDrawable(R.drawable.ic_recording_indicator);
            recordingIcon.setBounds(0, 0, recordingIcon.getMinimumWidth(), recordingIcon.getMinimumHeight());
            mRecordingTimeView.setCompoundDrawables(recordingIcon, null, null, null);
            mRecordingTimeView.setText("");
            mRecordingTimeView.setVisibility(View.VISIBLE);
            mRecordingTimeView
                    .announceForAccessibility(mActivity.getResources()
                            .getString(R.string.video_recording_started));
            mActivity.getCameraAppUI().setShutterPartInBottomBarShow(
                    View.VISIBLE, true);
            mActivity.getCameraAppUI().changeToRecordingUI();
            // nj dream camera test 66 - 68
            mActivity.getCameraAppUI().updateTopPanelUI(View.GONE);
            mActivity.getCameraAppUI().updateSidePanelUI(View.GONE);
            mActivity.getCameraAppUI().updateSlidePanelUI(View.GONE);

        } else {
            mRecordingTimeView
                    .announceForAccessibility(mActivity.getResources()
                            .getString(R.string.video_recording_stopped));
            mRecordingTimeView.setVisibility(View.GONE);
            mActivity.getCameraAppUI().setShutterPartInBottomBarShow(View.GONE,
                    true);
            mActivity.getCameraAppUI().changeToVideoReviewUI();
            // nj dream camera test 66 - 68
            mActivity.getCameraAppUI().updateTopPanelUI(View.VISIBLE);
            mActivity.getCameraAppUI().updateSidePanelUI(View.VISIBLE);
            mActivity.getCameraAppUI().updateSlidePanelUI(View.VISIBLE);

        }

    }

    public void onPauseClicked(boolean mPauseRecorderRecording) {
        // reset pause button icon
        mPauseRecording = mPauseRecorderRecording;
        mActivity
                .getCameraAppUI()
                .getPauseButton()
                .setImageResource(
                        mPauseRecorderRecording ? R.drawable.ic_start_sprd
                                : R.drawable.ic_pause_sprd);
        Drawable recording = mActivity.getResources().getDrawable(R.drawable.ic_recording_indicator);
        Drawable pasuing = mActivity.getResources().getDrawable(R.drawable.ic_recording_pause_indicator);
        if (mPauseRecorderRecording) {
            pasuing.setBounds(0, 0, pasuing.getMinimumWidth(), pasuing.getMinimumHeight());
            mRecordingTimeView.setCompoundDrawables(pasuing, null, null, null);
        } else {
            recording.setBounds(0, 0, recording.getMinimumWidth(), recording.getMinimumHeight());
            mRecordingTimeView.setCompoundDrawables(recording, null, null, null);
        }
    }

    /**
     * update preview ui after settings closed
     */
    public void onSettingUIHide() {
        if (!mRecordingStarted) {
            mActivity.getCameraAppUI().updatePreviewUI(View.VISIBLE);
        }
    }
    /* @} */

    /*
     * Add for ui check 122 @{
     */
    @Override
    public void onOrientationChanged(OrientationManager orientationManager,
            OrientationManager.DeviceOrientation deviceOrientation) {
        int orientation = deviceOrientation.getDegrees();
        DreamOrientation.setOrientation(mRootView, orientation, true);
    }
    /*
     * @}
     */

    public void onCloseModeListOrSettingLayout() {
        if (mRecordingStarted) {
            mActivity.getCameraAppUI().updateTopPanelUI(View.GONE);
            mActivity.getCameraAppUI().updateSidePanelUI(View.GONE);
            mActivity.getCameraAppUI().updateSlidePanelUI(View.GONE);
        }
    }
}
