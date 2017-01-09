
package com.dream.camera.modules.slowmotionvideo;

import com.android.camera.CameraActivity;
import com.android.camera.VideoController;
import com.dream.camera.dreambasemodules.DreamVideoUI;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageButton;

import com.android.camera2.R;

public class SlowmotionVideoUI extends DreamVideoUI {

    private ImageButton mSettingsButton;
    private View topPanel;


    public SlowmotionVideoUI(CameraActivity activity, VideoController controller, View parent) {
        super(activity, controller, parent);
    }

    @Override
    public void fitTopPanel(ViewGroup topPanelParent) {
        if (topPanel == null) {
            LayoutInflater lf = LayoutInflater.from(mActivity);
            topPanel = lf.inflate(R.layout.autovideo_top_panel, topPanelParent);
        }

        mActivity.getButtonManager().load(topPanel);
        mSettingsButton = (ImageButton) topPanel.findViewById(R.id.settings_button_dream);

        bindSettingsButton(mSettingsButton);

        bindFlashButton();

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

}
