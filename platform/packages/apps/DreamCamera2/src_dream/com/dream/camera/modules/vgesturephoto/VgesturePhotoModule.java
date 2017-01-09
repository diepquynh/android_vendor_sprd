package com.dream.camera.modules.vgesturephoto;

import java.util.HashMap;

import com.android.camera.app.AppController;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.util.GcamHelper;
import com.android.camera.util.GservicesHelper;
import com.android.camera.CameraActivity;
import com.android.camera.PhotoUI;

import android.os.Handler;
import android.os.Looper;

import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraCapabilities;
import com.android.ex.camera2.portability.CameraSettings;
import com.dream.camera.DreamModule;
import com.dream.camera.dreambasemodules.DreamPhotoModule;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.dream.camera.vgesture.VGestureController;

import android.view.View;

import com.dream.camera.util.DreamUtil;
import com.dream.camera.settings.DataModuleBasic;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.vgesture.VGestureOpenCameraInterface;
import com.android.camera2.R;

public class VgesturePhotoModule extends DreamPhotoModule implements VGestureOpenCameraInterface{
    private static final Log.Tag TAG = new Log.Tag("DreamVegistureModule");

    public VgesturePhotoModule(AppController app) {
        super(app);
        if (DreamUtil.FRONT_CAMERA == DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID) && UCamUtill.isVgestureEnnable()) {
           VGestureController.setListenter(this);
           mVGestureController = new VGestureController((CameraActivity)app);
        }
    }

    @Override
    public VgesturePhotoUI createUI(CameraActivity activity) {
        return new VgesturePhotoUI(activity, this, activity.getModuleLayoutRoot());
    }

    @Override
    public void init(CameraActivity activity, boolean isSecureCamera,
            boolean isCaptureIntent) {
        super.init(activity, isSecureCamera, isCaptureIntent);
        final View cancelButton = mActivity.findViewById(R.id.shutter_cancel_button);
        cancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // SPRD: Fix bug 537444 if paused, ignore this event
                if (mPaused) return;

                cancelVgestureCountDown(true);
            }
        });
    }

    public boolean isSupportTouchAFAE() {
        return true;
    }

    public boolean isSupportManualMetering() {
        return false;
    }
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

    /*SPRD:fix bug624871 set preview callback before startpreview @{ */
    @Override
    protected void doStartPreviewSpecial(boolean isCameraIdle, boolean isHdrOn,
            CameraActivity activity, CameraAgent.CameraProxy cameraDevice, Handler h,
            int displayOrientation, boolean mirror, View rootView, CameraSettings cameraSettings) {
        if (!mPaused && isShouldShowVGesture()) {
            Log.d(TAG,"doStartPreviewSpecial ");
            mVGestureController.doStartPreviewSpecial(isCameraIdle, isHdrOn, activity,
                    cameraDevice, h, displayOrientation, mirror, rootView, cameraSettings);
        }
    }
    /* @} */

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
            /*mVGestureController = null;*/
        }
    }

    public boolean isShouldShowVGesture(){
        boolean isFront = DreamUtil.FRONT_CAMERA == DataModuleManager
                .getInstance(mActivity).getDataModuleCamera()
                .getInt(Keys.KEY_CAMERA_ID);
        boolean isVOpen = DataModuleManager.getInstance(mActivity).getCurrentDataModule()
                .getBoolean(Keys.KEY_CAMERA_VGESTURE);
        boolean isVEnable = DataModuleManager.getInstance(mActivity).getCurrentDataModule()
                .getBoolean(Keys.KEY_CAMERA_VGESTURE);
        boolean result = isFront && isVOpen && isVEnable;
        Log.d(TAG,
                "isShouldShowVGesture " + isFront+","+isVOpen+","+isVEnable);
        return result;
    }
    @Override
    public void closeCamera(){
        super.closeCamera();
    }

    /*SPRD:fix bug 620875 avoid 3d photo use burst mode @{ */
    @Override
    public boolean checkCameraProxy() {
        boolean preferNewApi = GservicesHelper.useCamera2ApiThroughPortabilityLayer(
                mActivity.getContentResolver());
        return !getCameraProvider().isNewApi()
                && mCameraId == getCameraProvider().getCurrentCameraId().getLegacyValue();
    }
    /* @} */

    @Override
    public void requestCameraOpen(){
        super.requestCameraOpen();
    }

    @Override
    public void resume() {
        VGestureController.setListenter(this);
        super.resume();
    }

    /* nj dream camera test 24 */
    @Override
    public void pause() {
        if (mVGestureController != null) {
            mVGestureController.resetVGestureImp();
        }
        VGestureController.setListenter(null);
        super.pause();
    }
    /* @} */

    @Override
    public int getModuleTpye() {
        return DreamModule.VGESTURE_MODULE;
    }

    @Override
    public void onDreamSettingChangeListener(HashMap<String, String> keyList) {
        Log.e(TAG, "dreamPhotoonDreamSettingChangeListener  ");
        if (mCameraDevice == null) {
            return;
        }

        if(keyList.containsKey(Keys.KEY_CAMERA_VGESTURE_HELP)){
            updateUIVgestureHelp();
        }

        if(keyList.containsKey(Keys.KEY_CAMERA_VGESTURE_GUIDE)){
            updateUIVgestureGuide();
        }

        super.onDreamSettingChangeListener(keyList);
    }

    private void updateUIVgestureGuide() {
        ((VgesturePhotoUI)mUI).updateUIVgestureGuide();
    }

    private void updateUIVgestureHelp() {
        ((VgesturePhotoUI)mUI).updateUIVgestureHelp();
    }
    @Override
    public void doSometingWhenFilmStripShow(){
        if (mVGestureController != null && mVGestureController.isStartTimerOn()) {
            cancelVgestureCountDown(true);
        }
    }
}

