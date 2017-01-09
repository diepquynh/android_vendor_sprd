package com.dream.camera.vgesture;

import android.os.Handler;
import android.view.View;
import com.android.camera.CameraActivity;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;

public class VGestureController {

    public VGestureController(CameraActivity activity) {

    }

    public void startVGestureDetection(CameraActivity activity,
            CameraAgent.CameraProxy cameraDevice, Handler h, int displayOrientation,
            boolean mirror, View rootView) {

    }

    public void stopVGestureDetection(CameraActivity activity, CameraAgent.CameraProxy cameraDevice) {

    }

    public void startTimer(int detectMode) {

    }
    public static void setListenter(VGestureOpenCameraInterface vc){

    }
    public void doStartPreview(Handler h, CameraAgent.CameraStartPreviewCallback startPreviewCallback, CameraAgent.CameraProxy cameraDevice) {

    }
    public void doOnPreviewStartedSpecial(boolean isCameraIdle, boolean isHdrOn,
            CameraActivity activity, CameraAgent.CameraProxy cameraDevice, Handler h,
            int displayOrientation, boolean mirror, View rootView) {

    }

    public void doCaptureSpecial() {

    }

    public void doCloseCameraSpecial(CameraActivity activity, CameraAgent.CameraProxy cameraDevice) {

    }
}
