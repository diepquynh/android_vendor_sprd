package com.dream.camera.modules.continuephoto;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoController;

import com.android.camera2.R;

import com.dream.camera.dreambasemodules.DreamPhotoUI;
import android.widget.TextView;

public class ContinuePhotoUI extends DreamPhotoUI {

    private ImageButton mSettingsButton;
    private ImageButton mCameraToggleButton;
    private View topPanel;

    private TextView mCaptureAlready;
    private View mExtendPanel;

    public ContinuePhotoUI(CameraActivity activity, PhotoController controller,
            View parent) {
        super(activity, controller, parent);
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {

        if (topPanel == null) {
            LayoutInflater lf = LayoutInflater.from(mActivity);
            topPanel = lf.inflate(R.layout.continuephoto_top_panel,
                    topPanelParent);
        }

        mActivity.getButtonManager().load(topPanel);

        mSettingsButton = (ImageButton) topPanel
                .findViewById(R.id.settings_button_dream);

        bindSettingsButton(mSettingsButton);
        bindCameraButton();
    }

    @Override
    public void updateSidePanel() {
        super.updateSidePanel();
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {
        if (extendPanelParent != null) {
            LayoutInflater lf = LayoutInflater.from(mActivity);
            View extendPanel = lf.inflate(R.layout.continuephoto_extend_panel,
                    extendPanelParent);

            mCaptureAlready = (TextView) extendPanelParent
                    .findViewById(R.id.capture_already);
            mExtendPanel = (View) extendPanelParent
                    .findViewById(R.id.continuephoto_extend_panel);
        }
    }

    @Override
    public void updateBottomPanel() {
        super.updateBottomPanel();
    }

    @Override
    public void updateSlidePanel() {
        super.updateSlidePanel();
    }

    /**
     * update capture already ui
     */
    public void updateCaptureUI(String captureAlready) {
        if (mCaptureAlready != null) {
            mCaptureAlready.setText(captureAlready);
        }
    }

    public void changeExtendPanelUI(int visibility) {
        mExtendPanel.setVisibility(visibility);
    }

    public void changeOtherUIVisible(Boolean bursting, int visible) {
        mActivity.getCameraAppUI().setBursting(bursting);
        mActivity.getCameraAppUI().updateTopPanelUI(visible);
        mActivity.getCameraAppUI().setBottomBarLeftAndRightUI(visible);
        mActivity.getCameraAppUI().updateSidePanelUI(visible);
        mActivity.getCameraAppUI().updateSlidePanelUI(visible);
    }

    public boolean getExtendPanelVisibility() {
        return View.VISIBLE == mExtendPanel.getVisibility();
    }
}
