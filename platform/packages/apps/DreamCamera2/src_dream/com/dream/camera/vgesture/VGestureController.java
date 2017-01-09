
package com.dream.camera.vgesture;

/*
 * New Feature for VGesture
 */
import android.hardware.Camera.Parameters;
import android.os.Handler;
import android.view.View;

import com.android.camera.CameraActivity;
import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.stats.SessionStatsCollector;
import com.android.ex.camera2.portability.CameraAgent;
import com.dream.camera.settings.DataModuleManager;
import com.android.ex.camera2.portability.CameraSettings;
import com.android.ex.camera2.portability.Size;
public class VGestureController {
    private static final Log.Tag TAG = new Log.Tag("VGestureController");
    /**
     * SPRD: Fix bug 474851, Add for new feature VGesture @{
     */
    private static VGestureController mInstance;
    public static boolean mVGestureStarted = false;
    protected Parameters mParameters;
    protected CameraSettings mCameraSettings;
    private boolean mCameraOpen = false;
    private boolean mDetectStarted = false;
    private VGestureImp mVGestureImp;
    private static VGestureOpenCameraInterface mVgestureChangeCamera;
    private boolean mIsStartTimerOn = false;

    public VGestureController(CameraActivity activity) {
        mVGestureImp = VGestureImp.getInstance(activity);
    }

    public void doCaptureSpecial() {
        /* SPRD: New feature vgesture detect @{ */
        if (mVGestureStarted && mDetectStarted) {
            stopDetect();
        }
        /* @} */
    }

    public void doCloseCameraSpecial(CameraActivity activity, CameraAgent.CameraProxy cameraDevice) {
        /* SPRD: Fix bug 474851, Add for new feature VGesture @{ */
        mCameraOpen = false;
        /* @} */
        /* SPRD: Fix bug 474851, Add for new feature VGesture @{ */
        stopDetect();
        stopVGestureDetection(activity,cameraDevice);
        /* @} */
    }

    public void startVGestureDetection(CameraActivity activity,
            CameraAgent.CameraProxy cameraDevice, Handler h, int displayOrientation,
            boolean mirror, View rootView) {
        Log.d(TAG,"startVGestureDetection mVGestureStarted="+mVGestureStarted+","+cameraDevice);
        mIsStartTimerOn = false;
        if (mVGestureStarted || cameraDevice == null) {
            return;
        }
        Log.i(TAG,".....startVGestureStarted.....");
        if (activity.getCameraProvider().isNewApi()||mCameraSettings==null) {
            // SPRD: Fix bug 581647, After opening gesture camera preview interface now short black
            activity.getCameraAppUI().freezeScreenUntilPreviewReady();
            mVgestureChangeCamera.closeCamera();
            mVgestureChangeCamera.requestCameraOpen();
            mCameraOpen = true;
            return;
        }

        // ui check 181
        /*DataModuleManager.getInstance(activity).getCurrentDataModule().changeSettings(Keys.KEY_CAMERA_AI_DATECT, "face"); */
        mVGestureStarted = true;
        mDetectStarted = true;
        //android.hardware.Camera.Size size = mParameters.getPreviewSize();
        Size size = mCameraSettings.getCurrentPreviewSize();
        if(size == null){
            return;
        }
        mVGestureImp.onStartVGestureDetection(displayOrientation, mirror,
                cameraDevice,
                h, size.width(), size.height(), rootView);
        mVGestureImp.showGuide();
        mVGestureImp.showHelp();
        mVGestureImp.setVGestureStart(mVGestureStarted);
        SessionStatsCollector.instance().faceScanActive(true);
    }

    public void refreshParamaters( CameraActivity activity,
            CameraAgent.CameraProxy cameraDevice, Handler h,
            int displayOrientation, boolean mirror, View rootView) {
        if (cameraDevice != null ) {

            stopVGestureDetection(activity, cameraDevice);
            startVGestureDetection(activity, cameraDevice, h,
                    displayOrientation, mirror, rootView);

        }
    }

    public void stopVGestureDetection(CameraActivity activity, CameraAgent.CameraProxy cameraDevice) {
        Log.d(TAG,"stopVGestureDetection mVGestureStarted="+mVGestureStarted+","+cameraDevice);
        if (!mVGestureStarted || cameraDevice == null) {
            return;
        }
        Log.i(TAG, "     stopVGestureStarted.....    ");
        // ui check 181
        /*DataModuleManager.getInstance(activity).getCurrentDataModule().changeSettings(Keys.KEY_CAMERA_AI_DATECT,
                Keys.CAMERA_AI_DATECT_VAL_OFF);*/
        mVGestureStarted = false;
        mDetectStarted = false;
        cameraDevice.setFaceDetectionCallback(null, null);
        cameraDevice.stopFaceDetection();
        mVGestureImp.onStopVGestureDetection();
        mVGestureImp.setVGestureStart(mVGestureStarted);
        SessionStatsCollector.instance().faceScanActive(false);

        if (!activity.getCameraProvider().isNewApi() && cameraDevice != null && mCameraOpen) {
            // SPRD: Fix bug 581647, After opening gesture camera preview interface now short black
            activity.getCameraAppUI().freezeScreenUntilPreviewReady();
            mVgestureChangeCamera.closeCamera();
            mVgestureChangeCamera.requestCameraOpen();
        }

    }

    public void updateVGesture(boolean isCameraIdle, boolean isHdrOn, CameraActivity activity,
            CameraAgent.CameraProxy cameraDevice, Handler h, int displayOrientation, boolean mirror, View rootView) {

        refreshParamaters(activity, cameraDevice, h,
                displayOrientation, mirror, rootView);

//        Log.d(TAG,"updateVGesture "+","+isCameraIdle+","+isHdrOn);
//        if (DataModuleManager
//                .getInstance(activity).getCurrentDataModule()
//                .getBoolean(Keys.KEY_CAMERA_VGESTURE) && isCameraIdle && !isHdrOn) {
//            startVGestureDetection(activity, cameraDevice, h,
//                    displayOrientation, mirror, rootView);
//        } else {
//            stopVGestureDetection(activity, cameraDevice);
//        }
    }

    public void startTimer(int detectMode) {
        mIsStartTimerOn = true;
        mVGestureImp.startTimer(detectMode);
    }

    public void stopTimer(boolean restart){
        mIsStartTimerOn = false;
        mVGestureImp.stopTimer(restart);
    }

    public boolean isStartTimerOn(){
         return mIsStartTimerOn;
    }

    public void startDetect() {
        mDetectStarted = true;
        mVGestureImp.startDetect();
    }

    public void stopDetect() {
        mDetectStarted = false;
        mVGestureImp.stopDetect();
    }

    public void doStartPreview(Handler h, CameraAgent.CameraStartPreviewCallback startPreviewCallback, CameraAgent.CameraProxy cameraDevice) {
        cameraDevice.startPreviewWithCallback(h,
                startPreviewCallback);
        //mParameters = cameraDevice.getParameters();
    }

    /*SPRD:fix bug624871 set preview callback before startpreview @{ */
    public void doStartPreviewSpecial(boolean isCameraIdle, boolean isHdrOn,
            CameraActivity activity, CameraAgent.CameraProxy cameraDevice, Handler h,
            int displayOrientation, boolean mirror, View rootView, CameraSettings cameraSettings) {
        Log.d(TAG,"doStartPreviewSpecial mVGestureStarted="+mVGestureStarted+","+mDetectStarted);

        mCameraSettings = cameraSettings;
        if (mVGestureStarted && !mDetectStarted) {
            startDetect();
        }
        updateVGesture(isCameraIdle, isHdrOn, activity, cameraDevice, h,
                displayOrientation, mirror, rootView);
    }
    /* @} */

    public static void setListenter(VGestureOpenCameraInterface vc){
        mVgestureChangeCamera = vc;
    }

    /* nj dream camera test 24 */
    public void resetVGestureImp() {
        mVGestureImp.resetVGestureImp();
    }
    /* @} */
}
