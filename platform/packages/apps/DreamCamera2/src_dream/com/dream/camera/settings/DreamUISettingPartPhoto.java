
package com.dream.camera.settings;

import android.content.Context;
import android.util.AttributeSet;

import com.android.camera.settings.Keys;
import com.android.camera.settings.PictureSizeLoader.PictureSizes;
import com.android.camera.util.CameraUtil;
import com.ucamera.ucam.modules.utils.UCamUtill;
import android.util.Log;

public class DreamUISettingPartPhoto extends DreamUISettingPartBasic {

    public DreamUISettingPartPhoto(Context context) {
        super(context, null);
    }

    public DreamUISettingPartPhoto(Context context, AttributeSet attrs) {
        super(context, attrs);

    }

    public DreamUISettingPartPhoto(Context context, AttributeSet attrs,
            int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public DreamUISettingPartPhoto(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    private static final String TAG = "DreamUISettingPartPhoto";

    @Override
    public void changContent() {
        mDataModule = DataModuleManager.getInstance(getContext())
                .getCurrentDataModule();
        super.changContent();
    }

    /*SPRD: fix bug 606536 not add ui change listener when back from secure camera @*/
    @Override
    public void addListener() {
        super.addListener();
    }
    /* @ */

    @Override
    protected void updatePreItemsAccordingProperties() {

        // update visibility of picturesize
        updateVisibilityPictureSizes();

        // update visibility of time stamp
        updateVisibilityTimeStamp();

        // update visibility of EIOS
        updateVisibilityEOIS();

       // update visibility of ZSL
        updateVisibilityZSL();

       // update visibility of HighISO
        updateVisibilityHighISO();

        // update visibility of mirror
        updateVisibilityMirror();

        updateVisibilityTouchPhotograph();
    }

    private void updateVisibilityTouchPhotograph() {
        if(!CameraUtil.isTouchPhotoEnable()){
            recursiveDelete(this, findPreference(Keys.KEY_CAMERA_TOUCHING_PHOTOGRAPH));
        }
    }

    private void updateVisibilityHighISO() {
        if(!CameraUtil.isHighISOEnable()){
            recursiveDelete(this, findPreference(Keys.KEY_HIGH_ISO));
        }
    }

    private void updateVisibilityZSL() {
        if(!CameraUtil.isZslEnable()){
            recursiveDelete(this, findPreference(Keys.KEY_CAMERA_ZSL_DISPLAY));
        }
    }

    private void updateVisibilityTimeStamp() {
        if (!UCamUtill.isTimeStampEnable()) {
            recursiveDelete(this, findPreference(Keys.KEY_CAMERA_TIME_STAMP));
        }
    }

    private void updateVisibilityPictureSizes() {
        PictureSizes mPictureSizes = ((DataModuleInterfacePV) mDataModule)
                .getPictureSizes();
        if(mPictureSizes != null){
            if (mPictureSizes.backCameraSizes != null && mPictureSizes.backCameraSizes.isEmpty()) {
                recursiveDelete(this, findPreference(Keys.KEY_PICTURE_SIZE_BACK));
            }
            if (mPictureSizes.frontCameraSizes != null && mPictureSizes.frontCameraSizes.isEmpty()) {
                recursiveDelete(this, findPreference(Keys.KEY_PICTURE_SIZE_FRONT));
            }
        }
    }

    private void updateVisibilityEOIS() {
        if (!CameraUtil.isEOISDcBackEnabled()) {
            recursiveDelete(this, findPreference(Keys.KEY_EOIS_DC_BACK));
        }
        if (!CameraUtil.isEOISDcFrontEnabled()) {
            recursiveDelete(this, findPreference(Keys.KEY_EOIS_DC_FRONT));
        }
    }

    /* SPRD: Fix bug 615081 that update mirror visibility according to property setting @{ */
    private void updateVisibilityMirror() {
        if (!CameraUtil.isFrontCameraMirrorEnable()) {
            recursiveDelete(this, findPreference(Keys.KEY_FRONT_CAMERA_MIRROR));
        }
    }
    /* @} */
}
