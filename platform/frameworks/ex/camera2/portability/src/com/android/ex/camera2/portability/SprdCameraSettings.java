package com.android.ex.camera2.portability;


public abstract class SprdCameraSettings {
    protected CameraCapabilities.Antibanding mAntibanding;
    protected CameraCapabilities.ColorEffect mCurrentColorEffect;//SPRD:Add for color effect Bug 474727
    protected CameraCapabilities.BurstNumber mBurstNumber;//SPRD:fix 473462 bug add burst capture
    protected int mCurrentEnableZslMode = 0;//SPRD:fix bug 473462 add for burst capture or zsl
    protected CameraCapabilities.Contrast mCurrentContrast;
    protected int mUcamSkinWhitenLevel = 0;//SPRD:fix bug 474672 add for ucam beauty
    protected int mCurrentEnableHighISO = 0;//SPRD Add for highiso

    protected SprdCameraSettings() {

    }

    protected SprdCameraSettings(SprdCameraSettings src) {
        mAntibanding = src.mAntibanding;
        mCurrentColorEffect = src.mCurrentColorEffect;//SPRD:Add for color effect Bug 474727
        /*SPRD:fix 473462 bug add burst capture @{ */
        mBurstNumber = src.mBurstNumber;
        mCurrentEnableZslMode = src.mCurrentEnableZslMode;
        /* @}*/
        // SPRD Bug:474721 Feature:Contrast.
        mCurrentContrast = src.mCurrentContrast;
        mBrightNess = src.mBrightNess;// SPRD Bug:474715 Feature:Brightness.
        // SPRD Bug:474724 Feature:ISO.
        mISO = src.mISO;
        // SPRD Bug:474718 Feature:Metering.
        mMetering = src.mMetering;
        // SPRD Bug:474722 Feature:Saturation.
        mCurrentSaturation = src.mCurrentSaturation;
        mUcamSkinWhitenLevel = src.mUcamSkinWhitenLevel;//SPRD:fix bug474672 add for ucam beauty
        // SPRD Bug:474696 Feature:Slow-Motion.
        mCurrentSlowMotion = src.mCurrentSlowMotion;
        // SPRD Bug:500099 Feature:mirror.
        mFrontCameraMirror = src.mFrontCameraMirror;
        // SPRD  Feature: EOIS.
        mEOISEnable = src.mEOISEnable;

        mCurrentEnableHighISO = src.mCurrentEnableHighISO;
    }

    public void setAntibanding(CameraCapabilities.Antibanding antibanding) {
       mAntibanding = antibanding;
    }

    public CameraCapabilities.Antibanding getAntibanding() {
        return mAntibanding;
    }

    /* SPRD:Add for color effect Bug 474727 @{ */
    public void setColorEffect(CameraCapabilities.ColorEffect colorEffect) {
        mCurrentColorEffect = colorEffect;
    }

    public CameraCapabilities.ColorEffect getCurrentColorEffect() {
        return mCurrentColorEffect;
    }
    /* @} */

    /**
     * SPRD:fix bug 473462 add burst capture
     */
    public void setBurstPicNum(CameraCapabilities.BurstNumber count) {
        mBurstNumber = count;
    }

    public CameraCapabilities.BurstNumber getBurstPicNum() {
        return mBurstNumber;
    }

    /**
     * SPRD:fix bug 473462 add zsl mode for api2
     */
    public void setZslModeEnable(int enable) {
        mCurrentEnableZslMode = enable;
    }

    public int getZslModeEnable() {
        return mCurrentEnableZslMode;
    }

    /*
     * SPRD Bug:474721 Feature:Contrast. @{
     */
    public void setContrast(CameraCapabilities.Contrast contrast){
        mCurrentContrast = contrast;
    }

    public CameraCapabilities.Contrast getCurrentContrast() {
        return mCurrentContrast;
    }
    /* @} */

    /*
     * SPRD Bug:474715 Feature:Brightness. @{
     */
    protected CameraCapabilities.BrightNess mBrightNess;

    public void setBrightNess(CameraCapabilities.BrightNess brightness) {
        mBrightNess = brightness;
    }

    public CameraCapabilities.BrightNess getBrightNess() {
        return mBrightNess;
    }
    /* @} */

    /*
     * SPRD Bug:474724 Feature:ISO. @{
     */
    protected CameraCapabilities.ISO mISO;

    public void setISO(CameraCapabilities.ISO iso) {
        mISO = iso;
    }

    public CameraCapabilities.ISO getISO() {
        return mISO;
    }
    /* @} */

    /*
     * SPRD Bug:474718 Feature:Metering. @{
     */
    protected CameraCapabilities.Metering mMetering;

    public void setMetering(CameraCapabilities.Metering metering) {
        mMetering = metering;
    }

    public CameraCapabilities.Metering getMetering() {
        return mMetering;
    }
    /* @} */

    /*
     * SPRD Bug:474722 Feature:Saturation. @{
     */
    protected CameraCapabilities.Saturation mCurrentSaturation;

    public void setSaturation(CameraCapabilities.Saturation saturation){
        mCurrentSaturation = saturation;
    }

    public CameraCapabilities.Saturation getCurrentSaturation() {
        return mCurrentSaturation;
    }
    /* @} */

    /*SPRD: fix bug 474672 add for ucam beauty @{ */
    public void setSkinWhitenLevel(int level) {
        mUcamSkinWhitenLevel = level;
    }

    public int getSkinWhitenLevel() {
        return mUcamSkinWhitenLevel;
    }
    /* @} */

    /*
     * SPRD Bug:474696 Feature:Slow-Motion. @{
     */
    protected String mCurrentSlowMotion;

    public void setVideoSlowMotion(String slowmotion) {
        mCurrentSlowMotion = slowmotion;
    }

    public String getCurrentVideoSlowMotion() {
        return mCurrentSlowMotion;
    }
    /* @} */

    /*
     * SPRD Bug:500099 Feature:mirror. @{
     */
    protected boolean mFrontCameraMirror;

    public void setFrontCameraMirror(boolean frontCameraMirror) {
        mFrontCameraMirror = frontCameraMirror;
    }

    public boolean getfrontCameraMirror(){
        return mFrontCameraMirror;
    }
    /* @} */

    /*
     * SPRD Feature: EOIS. @{
     */
    protected boolean mEOISEnable;

    public void setEOISEnable(boolean eois) {
        mEOISEnable = eois;
    }

    public boolean getEOISEnable(){
        return mEOISEnable;
    }
    /* @} */

    /*SPRD Add for highiso 556862 @{*/
    public void setHighISOEnable(int enable) {
        mCurrentEnableHighISO = enable;
    }

    public int getHighISOEnable() {
        return mCurrentEnableHighISO;
    }
    /* @} */
}
