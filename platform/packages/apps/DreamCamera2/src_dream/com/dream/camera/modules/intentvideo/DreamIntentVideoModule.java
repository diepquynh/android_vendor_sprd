
package com.dream.camera.modules.intentvideo;

import com.android.camera.app.AppController;
import com.android.camera.CameraActivity;
import com.android.camera.VideoUI;

import com.dream.camera.dreambasemodules.DreamVideoModule;

public class DreamIntentVideoModule extends DreamVideoModule {

    public DreamIntentVideoModule(AppController app) {
        super(app);
    }

    public VideoUI createUI(CameraActivity activity) {
        return new DreamIntentVideoUI(activity, this, activity.getModuleLayoutRoot());
    }

}
