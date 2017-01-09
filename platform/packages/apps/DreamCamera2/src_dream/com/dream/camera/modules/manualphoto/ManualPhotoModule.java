
package com.dream.camera.modules.manualphoto;

import com.android.camera.app.AppController;
import com.android.camera.CameraActivity;
import com.android.camera.PhotoUI;

import com.dream.camera.dreambasemodules.DreamPhotoModule;

public class ManualPhotoModule extends DreamPhotoModule {

    public ManualPhotoModule(AppController app) {
        super(app);
    }

    @Override
    public PhotoUI createUI(CameraActivity activity) {
        return new ManualPhotoUI(activity, this, activity.getModuleLayoutRoot());
    }

    public boolean isSupportTouchAFAE() {
        return true;
    }

    public boolean isSupportManualMetering() {
        return true;
    }
}
