package com.thundersoft.advancedfilter;
import android.hardware.Camera;
import android.opengl.Matrix;

import com.android.camera.util.CameraUtil;

public class  FilterTranslationUtils {
    private static FilterTranslationUtils mInstances;
    private float[] mMMatrix = new float[16];
    private int mCameraId;
    private int mSensorOrientationFront = 0;
    private int mSensorOrientationBack = 0;

    public static FilterTranslationUtils getInstance(){
        if(mInstances==null){
            mInstances = new FilterTranslationUtils();
        }
        return mInstances;
    }

    public void setCameraId(int cameraId){
        mCameraId=cameraId;
    }

    public int getCameraId(){
        return mCameraId;
    }

    public float[] getMMatrix(){
        synchronized (mMMatrix) {
            return mMMatrix;
        }
    }
    /*
     * Set matrix.
     * First set matrix to the identity matrix,this must do.
     * Use matrix to change camera preview orientation, when switch camera facing or surface changed
     * @see android.opengl.Matrix
     *
     * When switch camera facing or surface changed, must call it.
     * Can be called in other thread or other local.
     */
    public void setMMatrix() {
        synchronized (mMMatrix) {
            float[] matrix1 = new float[16];
            /*
             * First sets matrix to the identity matrix. This must do.
             * @see android.opengl.Matrix
             */
            Matrix.setIdentityM(mMMatrix, 0);

            int displayOrientation = CameraUtil.getDisplayRotation();

            int cameraId = mCameraId;
            if (cameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
                /*
                 * camera facing is back
                 * Rotate matrix as Camera facing
                 *
                 * @see android.opengl.Matrix
                 */
                Matrix.setRotateM(matrix1, 0, 180, 1.0f, 0.0f, 0.0f); // mirror for back sensor by experience
                Matrix.setRotateM(mMMatrix, 0, (mSensorOrientationBack + displayOrientation + 180) % 360, 0.0f, 0, 1.0f);
                Matrix.multiplyMM(mMMatrix, 0, mMMatrix, 0, matrix1, 0);
            } else {
                /*
                 * camera facing is front
                 * Rotate matrix as Camera facing
                 *
                 * @see android.opengl.Matrix
                 */
                Matrix.setRotateM(mMMatrix, 0, (mSensorOrientationFront + displayOrientation + 180) % 360, 0.0f, 0, 1.0f);
            }
        }
    }

    public void setSensorOrientation(int sensorOrientationFront, int sensorOrientationBack) {
        mSensorOrientationFront = sensorOrientationFront;
        mSensorOrientationBack = sensorOrientationBack;
    }
}
