package com.dream.camera.modules.scenephoto;

import java.util.HashMap;

import com.android.camera.CameraActivity;
import com.android.camera.PhotoUI;
import com.android.camera.app.AppController;
import com.android.camera.settings.Keys;
import com.dream.camera.dreambasemodules.DreamPhotoModule;

import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.settings.DataStructSetting;

public class ScenePhotoModule extends DreamPhotoModule {

    public ScenePhotoModule(AppController app) {
        super(app);
    }

    @Override
    public PhotoUI createUI(CameraActivity activity) {
        return new ScenePhotoUI(activity, this, activity.getModuleLayoutRoot());
    }

    public boolean isSupportTouchAFAE() {
        return true;
    }

    public boolean isSupportManualMetering() {
        return false;
    }
   //nj dream camera test 58
    @Override
    public void onDreamSettingChangeListener(HashMap<String, String> keys) {
        for (String key : keys.keySet()) {
            switch (key) {
            case Keys.KEY_SCENE_MODE:
                String sceneMode = DataModuleManager.getInstance(mActivity)
                        .getDataModulePhoto().getString(Keys.KEY_SCENE_MODE);
                ((ScenePhotoUI)mUI).updateUIDisplay(sceneMode);
                break;
            }
        }
        super.onDreamSettingChangeListener(keys);
    }
}
