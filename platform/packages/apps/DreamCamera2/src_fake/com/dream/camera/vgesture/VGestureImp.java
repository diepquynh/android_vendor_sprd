package com.dream.camera.vgesture;

import com.android.camera.CameraActivity;
import com.android.camera.ui.FaceView;
import android.graphics.RectF;
import android.hardware.Camera.Face;
import com.android.camera.app.OrientationManager;

public class VGestureImp implements VGestureInterface/*,
        OrientationManager.OnOrientationChangeListener */{

    private static VGestureImp mInstance;

    public static synchronized VGestureImp getInstance(CameraActivity activity) {
        if (mInstance == null) {
            mInstance = new VGestureImp(activity);
        }
        return mInstance;
    }

    public VGestureImp(CameraActivity activity) {

    }

    public void onPreviewAreaChanged(RectF previewArea) {
    }

    public void setDisplayOrientation(int orientation) {
    }

    public void onPause() {

    }

    public boolean isNeedClearFaceView() {
        return false;
    }

    public boolean isDetectView() {
        return false;
    }

    public void doDectView(Face[] faces, FaceView faceView) {

    }
}
