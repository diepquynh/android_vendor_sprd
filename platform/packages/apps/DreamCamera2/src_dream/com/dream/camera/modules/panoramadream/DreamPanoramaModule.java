
package com.dream.camera.modules.panoramadream;

import com.android.camera.CameraActivity;
import com.android.camera.app.AppController;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.dream.camera.util.DreamUtil;
import com.sprd.camera.panora.WideAnglePanoramaModule;
import com.sprd.camera.panora.WideAnglePanoramaUI;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleManager;
import android.view.ViewGroup;
import android.view.View;

public class DreamPanoramaModule extends WideAnglePanoramaModule {

    public DreamPanoramaModule(AppController app) {
        super(app);
    }

    @Override
    public WideAnglePanoramaUI createUI(CameraActivity activity) {
        return new DreamPanoramaUI(activity, this, (ViewGroup) activity.getCameraAppUI()
                .getModuleView());
    }

    @Override
    public int getMode() {
        return DreamUtil.PHOTO_MODE;
    }

    @Override
    protected void doSomethingWhenonShutterStateViewFinder() {
        ((DreamPanoramaUI) mUI).changeOtherUIVisible(true, View.INVISIBLE);
    }

    @Override
    protected void showSomethingWhenonShutterStateMosaic() {
        ((DreamPanoramaUI) mUI).changeSomethingUIVisible(false, View.VISIBLE);
        ((DreamPanoramaUI) mUI).changeSomethingUIVisible(false, View.VISIBLE);
        ((DreamPanoramaUI) mUI).changeSomethingUIVisible(false, View.VISIBLE);
    }
    @Override
    protected void hideSideAndSlideWhenSaveHighResMosaic() {
        ((DreamPanoramaUI) mUI).changeMosaicSideAndSlideVisible(true, View.INVISIBLE);
        ((DreamPanoramaUI) mUI).changeMosaicSideAndSlideVisible(true, View.INVISIBLE);
    }

}
