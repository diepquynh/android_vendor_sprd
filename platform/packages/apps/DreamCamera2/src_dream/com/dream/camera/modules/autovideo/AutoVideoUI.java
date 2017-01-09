
package com.dream.camera.modules.autovideo;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;

import com.android.camera.settings.Keys;
import com.android.camera.util.CameraUtil;
import com.android.camera2.R;
import com.android.camera.CameraActivity;
import com.android.camera.VideoController;
import com.dream.camera.MakeupController;
import com.dream.camera.MakeupController.MakeupListener;
import com.dream.camera.dreambasemodules.DreamVideoUI;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.util.DreamUtil;

public class AutoVideoUI extends DreamVideoUI {

    private ImageButton mSettingsButton;
    private View topPanel;

    public AutoVideoUI(CameraActivity activity, VideoController controller, View parent) {
        super(activity, controller, parent);
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {

        DreamUtil dreamUtil = new DreamUtil();
        if (DreamUtil.BACK_CAMERA == dreamUtil.getRightCamera(DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID))) {

            if(mController.isMakeUpEnable()){
                if (topPanel == null) {
                    LayoutInflater lf = LayoutInflater.from(mActivity);
                    topPanel = lf.inflate(R.layout.autovideo_makeup_top_panel,
                            topPanelParent);
                }
            } else {
                if (topPanel == null) {
                    LayoutInflater lf = LayoutInflater.from(mActivity);
                    topPanel = lf.inflate(R.layout.autovideo_top_panel,
                            topPanelParent);
                }
            }
            mActivity.getButtonManager().load(topPanel);
            bindFlashButton();

        } else {

            if(mController.isMakeUpEnable()){
                if (topPanel == null) {
                    LayoutInflater lf = LayoutInflater.from(mActivity);
                    topPanel = lf.inflate(R.layout.autovideo_makeup_front_top_panel,
                            topPanelParent);
                }
            } else {
                if (topPanel == null) {
                    LayoutInflater lf = LayoutInflater.from(mActivity);
                    topPanel = lf.inflate(R.layout.autovideo_makeup_top_panel,
                            topPanelParent);
                }
            }

            mActivity.getButtonManager().load(topPanel);
        }

        mSettingsButton = (ImageButton) topPanel.findViewById(R.id.settings_button_dream);

        if(mController.isMakeUpEnable()){
            bindMakeupButton();
        }

        bindSettingsButton(mSettingsButton);

        bindCameraButton();
    }

    @Override
    public void updateSidePanel() {
        super.updateSidePanel();
    }

    @Override
    public void fitExtendPanel(ViewGroup extendPanelParent) {

        if (mController.isMakeUpEnable()) {
                LayoutInflater lf = LayoutInflater.from(mActivity);
                // mFreezeFrame = extendPanelParent;
                View extendPanel = lf.inflate(R.layout.video_extend_panel,
                        extendPanelParent);
                new MakeupController(extendPanel, mController,
                        Keys.KEY_MAKEUP_VIDEO_LEVEL, mActivity.getResources()
                                .getInteger(R.integer.ucam_makup_default_value));
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

}
