
package com.dream.camera.modules.intentcapture;

import com.android.camera.app.AppController;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.util.GservicesHelper;
import com.android.camera.CameraActivity;
import com.android.camera.PhotoUI;
import android.os.Handler;
import android.os.Looper;
import com.android.ex.camera2.portability.CameraAgent;

import com.dream.camera.dreambasemodules.DreamPhotoModule;
import com.ucamera.ucam.modules.utils.UCamUtill;
import com.dream.camera.vgesture.VGestureController;
import android.hardware.Camera.Parameters;
import com.android.camera.CameraActivity;
import android.view.View;
import com.dream.camera.util.DreamUtil;
import com.dream.camera.settings.DataModuleManager;
import com.dream.camera.vgesture.VGestureOpenCameraInterface;

public class DreamIntentCaptureModule extends DreamPhotoModule {
    private static final Log.Tag TAG = new Log.Tag("DreamIntentCaptureModule");

    public DreamIntentCaptureModule(AppController app) {
        super(app);
    }

    @Override
    public PhotoUI createUI(CameraActivity activity) {
        // TODO Auto-generated method stub
        return new DreamIntentCaptureUI(activity, this, activity.getModuleLayoutRoot());
    }

    public boolean isSupportTouchAFAE() {
        return true;
    }

    public boolean isSupportManualMetering() {
        return false;
    }

    @Override
    public void doCloseCameraSpecial(CameraActivity activity, CameraAgent.CameraProxy cameraDevice) {
        if (mVGestureController != null) {
            mVGestureController.doCloseCameraSpecial(activity,cameraDevice);
            /*mVGestureController = null;*/
        }
    }

    @Override
    public void closeCamera(){
        super.closeCamera();
    }

    @Override
    public void requestCameraOpen(){
        super.requestCameraOpen();
    }
    /* nj dream camera test 24 */
    @Override
    public void pause() {
        if (mVGestureController != null) {
            mVGestureController.resetVGestureImp();
        }
        super.pause();
    }
    /* @} */
}
