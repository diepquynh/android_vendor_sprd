
package com.dream.camera.modules.scenerydream;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;

import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera2.R;

import com.dream.camera.dreambasemodules.DreamBasicUI;
import com.dream.camera.dreambasemodules.DreamInterface;
import com.dream.camera.SlidePanelManager;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.util.DreamUtil;
import com.sprd.camera.aidetection.AIDetectionController;
import com.android.camera.debug.Log;
import com.android.camera.debug.Log.Tag;

public class DreamSceneryUI extends DreamBasicUI implements DreamInterface {
    private static final Tag TAG = new Tag("DreamSceneryUI");
    // top panel
    private View topPanel;
    private ImageButton tSettingsButton;

    public DreamSceneryUI( int cameraId, PhotoController baseController, CameraActivity activity, View parent) {
        super(cameraId, activity, baseController, parent);
        activity.getCameraAppUI().setDreamInterface(this);
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {
        Log.i(TAG, "fitTopPanel");
        DreamUtil dreamUtil = new DreamUtil();
        if (DreamUtil.BACK_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.sceneryphoto_top_panel,
                        topPanelParent);
            }
            bindTopButtons();
        } else {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.sceneryphoto_front_top_panel,
                        topPanelParent);
            }

            mActivity.getButtonManager().load(topPanel);

            tSettingsButton = (ImageButton) topPanel
                    .findViewById(R.id.settings_button_dream);

            bindSettingsButton(tSettingsButton);
            bindCountDownButton();
            bindCameraButton();
        }
    }

    private void bindTopButtons() {
        mActivity.getButtonManager().load(topPanel);

        tSettingsButton = (ImageButton) topPanel
                .findViewById(R.id.settings_button_dream);

        bindSettingsButton(tSettingsButton);
        bindFlashButton();
        bindCountDownButton();
        bindCameraButton();
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {

    }

    @Override
    public void updateBottomPanel() {
        mActivity.getCameraAppUI().updateSwitchModeBtn(this);
    }

    @Override
    public void updateSlidePanel() {
        SlidePanelManager.getInstance(mActivity).udpateSlidePanelShow(SlidePanelManager.FILTER,View.GONE);
        SlidePanelManager.getInstance(mActivity).focusItem(SlidePanelManager.CAPTURE, false);
    }

    protected int sidePanelMask;

    @Override
    public void updateSidePanel() {
        super.updateSidePanel();
    }

    @Override
    public void onSettingChanged(SettingsManager settingsManager, String key) {

    }
}
