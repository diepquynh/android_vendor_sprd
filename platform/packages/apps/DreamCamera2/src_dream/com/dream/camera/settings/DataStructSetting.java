package com.dream.camera.settings;

import android.util.Log;

/**
 * used to configure the module, the member parameter mCategory decide which
 * module(photo/video/camera) to use, the member parameter mIsFront decide if
 * use front camera, the member parameter mMode used to which child
 * mode(autophoto/manualphoto/autovideo,etc) to use, the member parameter
 * mCameraID decide the camera id
 * 
 * @author SPREADTRUM\ying.sun
 * 
 */
public class DataStructSetting {
    private static final String TAG = "DreamSetting";
    // photo/video/camera
    public String mCategory;
    // front/back camera
    public boolean mIsFront;
    // mode
    public String mMode;
    // camera id
    public int mCameraID;

    /**
     * used for photo/video module
     * 
     * @param category
     * @param isFront
     * @param mode
     * @param cameraID
     */
    public DataStructSetting(String category, boolean isFront, String mode,
            int cameraID) {
        mCategory = category;
        mIsFront = isFront;
        mMode = mode;
        mCameraID = cameraID;
        Log.e(TAG, "DataStructSetting = " + mCategory + "," + mIsFront + ","
                + mMode + "," + mCameraID);
    }

    /**
     * 
     * @param category
     * @param isFront
     * @param mode
     * @param cameraID
     */
    public DataStructSetting(String category, boolean isFront, int mode,
            int cameraID) {
        this(category, isFront, DreamSettingUtil.changeChildModeToString(
                category, isFront, mode), cameraID);
    }

    public DataStructSetting() {
    }

    public boolean equals(DataStructSetting dataSetting) {
        if (mCategory == null || !mCategory.equals(dataSetting.mCategory)) {
            return false;
        }
        if (mIsFront != dataSetting.mIsFront) {
            return false;
        }
        if (mMode == null || !mMode.equals(dataSetting.mMode)) {
            return false;
        }
        if (mCameraID != dataSetting.mCameraID) {
            return false;
        }
        return true;
    }

}
