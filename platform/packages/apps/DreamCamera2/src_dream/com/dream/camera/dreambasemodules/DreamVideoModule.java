package com.dream.camera.dreambasemodules;

import com.android.camera.app.AppController;
import com.android.camera.CameraActivity;
import com.android.camera.VideoModule;
import com.android.camera.VideoUI;
import com.dream.camera.util.DreamUtil;

public abstract class DreamVideoModule extends VideoModule {

    public DreamVideoModule(AppController app) {
        super(app);
    }

    @Override
    public abstract VideoUI createUI(CameraActivity activity);

    @Override
    protected void switchCamera() {
        // super.switchCamera();
        mActivity.switchFrontAndBackMode();
        mActivity.getCameraAppUI().updateModeList();
    }

    @Override
    public int getMode() {
        return DreamUtil.VIDEO_MODE;
    }

    @Override
    public void pause() {
        //mAppController.getCameraAppUI().hideSettingUI();
        super.pause();
    }
}
