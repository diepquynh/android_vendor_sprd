
package com.dream.camera.modules.autophoto;

import com.android.camera.app.AppController;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.util.GservicesHelper;
import com.android.camera.CameraActivity;
import com.android.camera.PhotoUI;
import android.os.Handler;
import android.os.Looper;
import com.android.ex.camera2.portability.CameraAgent;

import com.dream.camera.dreambasemodules.DreamPhotoModule;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.dream.camera.vgesture.VGestureController;
import android.view.View;
import com.dream.camera.util.DreamUtil;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.vgesture.VGestureOpenCameraInterface;

public class AutoPhotoModule extends DreamPhotoModule {
    private static final Log.Tag TAG = new Log.Tag("AutoPhotoModule");

    public AutoPhotoModule(AppController app) {
        super(app);
//        if (DreamUtil.FRONT_CAMERA == DataModuleManager
//                .getInstance(mActivity).getDataModuleCamera()
//                .getInt(Keys.KEY_CAMERA_ID) && UCamUtill.isVgestureEnnable()) {
//           VGestureController.setListenter(this);
//           mVGestureController = new VGestureController((CameraActivity)app);
//        }
    }

    @Override
    public PhotoUI createUI(CameraActivity activity) {
        return new AutoPhotoUI(activity, this, activity.getModuleLayoutRoot());
    }

    public boolean isSupportTouchAFAE() {
        return true;
    }

    public boolean isSupportManualMetering() {
        return false;
    }

    /**
    @Override
    protected void doCameraOpen(int cameraId) {
        if (isShouldShowVGesture()) {
            Log.d(TAG,"doCameraOpen "+cameraId);
            mActivity.getCameraProvider().requestCamera(cameraId, false);
        }
        else {
            super.doCameraOpen(cameraId);
        }

    }

    @Override
    protected void doStartPreview(CameraAgent.CameraStartPreviewCallback startPreviewCallback, CameraAgent.CameraProxy cameraDevice) {
        if (isShouldShowVGesture()) {
            Log.d(TAG,"doCameraOpen ");
            mVGestureController.doStartPreview(new Handler(Looper.getMainLooper()),startPreviewCallback, cameraDevice);
        } else {
            super.doStartPreview(startPreviewCallback, cameraDevice);
        }
    }

    @Override
    public void doOnPreviewStartedSpecial(boolean isCameraIdle, boolean isHdrOn,
            CameraActivity activity, CameraAgent.CameraProxy cameraDevice, Handler h, int displayOrientation,
            boolean mirror, View rootView) {
        if (isShouldShowVGesture()) {
            Log.d(TAG,"doOnPreviewStartedSpecial ");
            mVGestureController.doOnPreviewStartedSpecial(isCameraIdle, isHdrOn, activity,
                    cameraDevice, h, displayOrientation, mirror, rootView);
        }
    }

    @Override
    public void doCaptureSpecial() {
        if (isShouldShowVGesture()) {
            mVGestureController.doCaptureSpecial();
        }
    }

    @Override
    public void doCloseCameraSpecial(CameraActivity activity, CameraAgent.CameraProxy cameraDevice) {
        if (mVGestureController != null) { // nj dream camera test 46
            mVGestureController.doCloseCameraSpecial(activity,cameraDevice);
            // mVGestureController = null;
        }
    }

    public boolean isShouldShowVGesture(){
        boolean isFront = DreamUtil.FRONT_CAMERA == DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID);
        boolean isVOpen = DataModuleManager.getInstance(mActivity).getCurrentDataModule()
                .getBoolean(Keys.KEY_CAMERA_VGESTURE);
        boolean isVEnable = UCamUtill.isVgestureEnnable();
        boolean isVConfigureEnable = DataModuleManager.getInstance(mActivity)
                .getCurrentDataModule()
                .isEnableSettingConfig(Keys.KEY_CAMERA_VGESTURE);

        boolean result = isVConfigureEnable && isFront && isVOpen && isVEnable;
        Log.d(TAG,
                "isShouldShowVGesture " + isFront+","+isVOpen+","+isVEnable + "," + isVConfigureEnable);
        return result;
    }
    @Override
    public void closeCamera(){
        super.closeCamera();
    }

    @Override
    public void requestCameraOpen(){
        super.requestCameraOpen();
    }
    // nj dream camera test 24
    @Override
    public void pause() {
        if (mVGestureController != null) {
            mVGestureController.resetVGestureImp();
        }
        super.pause();
    }
    /* @} */

    /* SPRD: optimize camera launch time @{ */
    public boolean useNewApi() {
        // judge VGesture enable state first will be better, like:
        // if (isShouldShowVGesture()) return false;
        // but isShouldShowVGesture will throw exception if useNewApi() is
        // called before module initialized, and no negative effect is found
        // until now, so just ignore this judgement temporarily.
        return GservicesHelper.useCamera2ApiThroughPortabilityLayer(null);
    }
    /* @} */
}
