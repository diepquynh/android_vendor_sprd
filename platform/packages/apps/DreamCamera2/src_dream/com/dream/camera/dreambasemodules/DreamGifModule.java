
package com.dream.camera.dreambasemodules;

import com.ucamera.ucam.modules.ugif.GifModule;
import com.ucamera.ucam.modules.ui.Switcher;
import com.android.camera.MultiToggleImageButton;
import com.android.camera.app.AppController;
import com.android.camera.ui.Rotatable;
import com.android.camera.util.CameraUtil;
import com.dream.camera.DreamOrientation;
import android.util.Log;

public abstract class DreamGifModule extends GifModule {
    private static final String TAG = "DreamGifModule";

    public DreamGifModule(AppController app) {
        super(app);
    }

    @Override
    public void switchCamera() {
        // super.switchCamera();
        Log.i(TAG, "AAA switchCamera");
        /* SPRD: Fix bug 595400 the freeze screen for gif @{ */
        freezeScreen(CameraUtil.isFreezeBlurEnable(), false);
        /* @} */

    }

    @Override
    protected void initializeCpatureBottomBar() {
    }

    @Override
    protected void changeCaptureBottomBarUI(int visible, boolean capturing) {
        // setCaptureBottomBarShow(!capturing);
        mActivity.getCameraAppUI().setShutterPartInBottomBarShow(visible, true);
        /* SPRD: ui check 49 gifModule ui @{ */
        mActivity.getCameraAppUI().updateGifCancelAndFinish(visible);
        /* @} */
    }

    @Override
    protected void updateCaptureBottomBarUI(int orientation, int rotation) {

    }

    @Override
    protected void onCaptureBottomBarSwitchChanged(Switcher source, boolean onOff) {

    }
    /*
     * Add for ui check 122 @{
     */
    protected void updateRotationUI(int orientation) {
        DreamOrientation.setOrientation(mRootView, orientation, true);
    }
    /* @} */
    @Override
    protected void changeCaptureTopPanelUI(boolean disable) {
        ((DreamGifUI)mUI).updateGifSettingAndSwitch(disable);
    }

    @Override
    public int getModuleTpye() {
        return GIF_MODULE;
    }
}
