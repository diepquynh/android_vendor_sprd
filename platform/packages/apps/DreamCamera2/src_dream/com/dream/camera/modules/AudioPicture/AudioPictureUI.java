package com.dream.camera.modules.AudioPicture;

import java.util.HashMap;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;
import com.android.camera.PhotoModule;

import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera2.R;

import android.widget.FrameLayout;

import com.dream.camera.MakeupController;
import com.dream.camera.SlidePanelManager;
import com.dream.camera.dreambasemodules.DreamPhotoUI;
import com.dream.camera.settings.DataModuleBasic.DreamSettingChangeListener;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataModuleManager.ResetListener;
import com.dream.camera.util.DreamUtil;
import android.hardware.Camera.Face;
import com.android.camera.ui.FaceView;
import android.graphics.RectF;
import com.dream.camera.vgesture.VGestureImp;
import com.ucamera.ucam.modules.utils.UCamUtill;
// SPRD: Fix bug 535110, Photo voice record.
import com.android.camera.ui.PhotoVoiceRecordProgress;

public class AudioPictureUI extends DreamPhotoUI implements ResetListener {
    private static final Log.Tag TAG = new Log.Tag("AudioPictureUI");
    private ImageButton mSettingsButton;
    private View topPanel;

    // SPRD: Fix bug 535110, Photo voice record.
    private PhotoVoiceRecordProgress mPhotoVoiceRecordProgress;

    public AudioPictureUI(CameraActivity activity, PhotoController controller,
            View parent) {
        super(activity, controller, parent);
        mPhotoVoiceRecordProgress = (PhotoVoiceRecordProgress) mRootView.findViewById(R.id.photo_voice_record_progress);
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {
        DreamUtil dreamUtil = new DreamUtil();
        if (DreamUtil.BACK_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.autophoto_top_panel,
                        topPanelParent);
            }

            mActivity.getButtonManager().load(topPanel);

            bindFlashButton();



        } else {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.autophoto_front_top_panel,
                        topPanelParent);
            }
            mActivity.getButtonManager().load(topPanel);
            bindMakeupButton();
        }

        mSettingsButton = (ImageButton) topPanel
                .findViewById(R.id.settings_button_dream);
        bindSettingsButton(mSettingsButton);
        bindCountDownButton();
        bindHdrButton();
        bindCameraButton();
    }

    @Override
    public void updateSidePanel() {
        super.updateSidePanel();
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {

        DreamUtil dreamUtil = new DreamUtil();
        if (DreamUtil.BACK_CAMERA != dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {
            LayoutInflater lf = LayoutInflater.from(mActivity);
            // mFreezeFrame = extendPanelParent;
            View extendPanel = lf.inflate(
                    R.layout.autophoto_front_extend_panel, extendPanelParent);
            initMakeupControl(extendPanelParent);
        }

    }

    @Override
    public void updateBottomPanel() {
        super.updateBottomPanel();
    }

    @Override
    public void updateSlidePanel() {
        SlidePanelManager.getInstance(mActivity)
                .udpateSlidePanelShow(SlidePanelManager.FILTER,View.GONE);
        SlidePanelManager.getInstance(mActivity).focusItem(
                SlidePanelManager.CAPTURE, false);
    }

    @Override
    public void onSettingReset() {
        DreamUtil dreamUtil = new DreamUtil();
        int cameraid = dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID));
        if (DreamUtil.BACK_CAMERA != cameraid) {
            mBasicModule.updateMakeLevel();
            ((PhotoModule)mController).updateVGesture();
        }
    }

    @Override
    public void onResume(){
        DataModuleManager.getInstance(mActivity).addListener(this);
    }

    @Override
    public void onPause() {
        super.onPause();
        DataModuleManager.getInstance(mActivity).removeListener(this);
    }


    public boolean isDetectView() {
        return false;
    }


    public boolean isInFrontCamera() {
        DreamUtil dreamUtil = new DreamUtil();
        return DreamUtil.FRONT_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID));
    }

    /* SPRD: Fix bug 535110, Photo voice record. @{ */
    public void showAudioNoteProgress() {
        mPhotoVoiceRecordProgress.startVoiceRecord();
        mActivity.getCameraAppUI().showStopRecordVoiceButton();
        mActivity.getCameraAppUI().hideCaptureIndicator();
    }

    public void hideAudioNoteProgress() {
        mPhotoVoiceRecordProgress.stopVoiceRecord();
        mActivity.getCameraAppUI().hideStopRecordVoiceButton();
        mPhotoVoiceRecordProgress.hideTip();
    }
    /* @} */
    public void setTopPanelVisible(int visible){
        if (topPanel != null)
            topPanel.setVisibility(visible);
    }
}
