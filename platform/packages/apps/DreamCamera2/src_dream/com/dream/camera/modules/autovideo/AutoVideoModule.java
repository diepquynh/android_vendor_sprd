
package com.dream.camera.modules.autovideo;

import com.android.camera.app.AppController;
import com.android.camera.CameraActivity;
import com.android.camera.VideoUI;

import com.dream.camera.dreambasemodules.DreamVideoModule;

public class AutoVideoModule extends DreamVideoModule {

    public AutoVideoModule(AppController app) {
        super(app);
    }

    @Override
    public VideoUI createUI(CameraActivity activity) {
        return new AutoVideoUI(activity, this, activity.getModuleLayoutRoot());
    }
}
