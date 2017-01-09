package com.dream.camera.dreambasemodules;

import com.android.camera.app.AppController;
import com.android.camera.CameraActivity;
import com.android.camera.PhotoModule;
import com.android.camera.PhotoUI;
import com.android.camera.debug.Log;
import com.android.ex.camera2.portability.CameraCapabilities;

import com.dream.camera.dreambasemodules.DreamController;
import com.dream.camera.util.DreamUtil;

public abstract class DreamPhotoModule extends PhotoModule implements
        DreamController {

    private static final Log.Tag TAG = new Log.Tag("DreamPhotoModule");

    public DreamPhotoModule(AppController app) {
        super(app);
    }

    @Override
    public abstract PhotoUI createUI(CameraActivity activity);

    @Override
    protected void switchCamera() {
        // super.switchCamera();
        mActivity.switchFrontAndBackMode();

        mActivity.getCameraAppUI().updateModeList();

    }

    public void singleTapAEAF(int x, int y) {
        Log.d(TAG, "singleTapAEAF " + isSupportTouchAFAE());
        if (isSupportTouchAFAE()) {
            super.singleTapAEAF(x, y);
        }
    }


    @Override
    public int getMode() {
        return DreamUtil.PHOTO_MODE;
    }

    @Override
    public void pause() {
        //mAppController.getCameraAppUI().hideSettingUI();
        super.pause();
    }

}
