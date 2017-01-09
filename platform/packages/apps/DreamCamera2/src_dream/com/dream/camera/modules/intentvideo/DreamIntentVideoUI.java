
package com.dream.camera.modules.intentvideo;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.camera.CameraActivity;
import com.android.camera.VideoController;

import com.android.camera.settings.Keys;
import com.android.camera2.R;

import com.dream.camera.dreambasemodules.DreamVideoUI;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.util.DreamUtil;

public class DreamIntentVideoUI extends DreamVideoUI {

    private View topPanel;

    public DreamIntentVideoUI(CameraActivity activity, VideoController controller,
            View parent) {
        super(activity, controller, parent);
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {

        DreamUtil dreamUtil = new DreamUtil();
        if (DreamUtil.BACK_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.intentvideo_top_panel,
                        topPanelParent);
            }

            mActivity.getButtonManager().load(topPanel);

            bindFlashButton();

        } else {
            if (topPanel == null) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                topPanel = lf.inflate(R.layout.intentvideo_front_top_panel,
                        topPanelParent);
            }

            mActivity.getButtonManager().load(topPanel);

        }

        mActivity.getButtonManager().load(topPanel);

        bindCameraButton();
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {
    }

    @Override
    public void updateSlidePanel() {
        mActivity.getCameraAppUI().hideSlide();
    }

    @Override
    public void updateBottomPanel() {
        mActivity.getCameraAppUI().hideBottomPanelLeftRight();
    }

}
