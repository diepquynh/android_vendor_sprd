package com.dream.camera.vgesture;

import com.android.ex.camera2.portability.CameraAgent;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import android.os.Handler;
import android.view.View;
public interface VGestureInterface {

    public void onStartVGestureDetection(int orientation, boolean mirror,
            CameraAgent.CameraProxy cameraDevice, Handler h, int PreviewWidth, int PreviewHeight, View rootView);

    public void onStopVGestureDetection();

//    public void showGuide(boolean bShow);
//
//    public void showHelp(boolean bShow);

    public void showGuide();

    public void showHelp();

    public void setVGestureStart(boolean isStart);

    public void startTimer(int detectMode);

    public void stopTimer(boolean restart);

    public void startDetect();

    public void stopDetect();
}
