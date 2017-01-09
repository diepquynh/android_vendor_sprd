package com.android.ex.camera2.portability;

import com.android.ex.camera2.portability.debug.Log;

import android.hardware.SprdCamera;
import android.hardware.SprdCamera.SprdParameters;

public abstract class SprdAndroidCameraSettings extends CameraSettings {
    private static final Log.Tag TAG = new Log.Tag("SprdAndCamSet");

    protected SprdAndroidCameraSettings() {

    }

    protected SprdAndroidCameraSettings(SprdAndroidCameraSettings src) {
        super(src);
    }
    
    public SprdAndroidCameraSettings(CameraCapabilities capabilities, SprdCamera.Parameters params) {
        if (params instanceof SprdParameters) {
            initAndroidCameraSettings(capabilities, (SprdCamera.SprdParameters)params);
        }
    }

    private void initAndroidCameraSettings(CameraCapabilities capabilities, SprdCamera.SprdParameters params) {
        if (params == null) {
            Log.w(TAG, "Settings ctor requires a non-null SprdCamera.SprdParameters.");
            return;
        }

        CameraCapabilities.Stringifier stringifier = capabilities.getStringifier();

        //SPRD:Add for antibanding
        setAntibanding(getAntibandingFromParameters(params.getAntibanding()));
        //SPRD:Add for whitebalance
        setWhiteBalance(stringifier.whiteBalanceFromString(params.getWhiteBalance()));
        // SPRD Bug:474721 Feature:Contrast.
        setContrast(getContrastFromParameters(params.getContrast()));
        // SPRD Bug:474715 Feature:Brightness.
        setBrightNess(getBrightNessFromParameters(params.getBrightness()));
        // SPRD Bug:474724 Feature:ISO.
        setISO(getISOFromParameters(params.getISO()));
        // SPRD Bug:474718 Feature:Metering.
        setMetering(getMeteringFromParameters(params.getMeteringMode()));
        // SPRD Bug:474722 Feature:Saturation.
        setSaturation(getSaturationFromParameters(params.getSaturation()));
        // SPRD Bug:474696 Feature:Slow-Motion.
        setVideoSlowMotion(params.getSlowmotion());
        // SPRD Bug:534257 eis/ois
        setEOISEnable(params.getEOIS());
    }

    protected CameraCapabilities.Antibanding getAntibandingFromParameters(String param) {
        if (SprdCamera.SprdParameters.ANTIBANDING_AUTO.equals(param)) {
            return CameraCapabilities.Antibanding.AUTO;
        } else if (SprdCamera.SprdParameters.ANTIBANDING_50HZ.equals(param)) {
            return CameraCapabilities.Antibanding.ANTIBANDING_50HZ;
        } else if (SprdCamera.SprdParameters.ANTIBANDING_60HZ.equals(param)) {
            return CameraCapabilities.Antibanding.ANTIBANDING_60HZ;
        } else if (SprdCamera.SprdParameters.ANTIBANDING_OFF.equals(param)) {
            return CameraCapabilities.Antibanding.OFF;
        } else {
            return null;
        }
    }

    /*
     * SPRD Bug:474721 Feature:Contrast. @{
     */
    private CameraCapabilities.Contrast getContrastFromParameters(String param) {
        if (AndroidCameraCapabilities.VALUE_ZERO.equals(param)) {
            return CameraCapabilities.Contrast.CONTRAST_ZERO;
        } else if (AndroidCameraCapabilities.VALUE_ONE.equals(param)) {
            return CameraCapabilities.Contrast.CONTRAST_ONE;
        } else if (AndroidCameraCapabilities.VALUE_TWO.equals(param)) {
            return CameraCapabilities.Contrast.CONTRAST_TWO;
        } else if (AndroidCameraCapabilities.VALUE_THREE.equals(param)) {
            return CameraCapabilities.Contrast.CONTRAST_THREE;
        } else if (AndroidCameraCapabilities.VALUE_FOUR.equals(param)) {
            return CameraCapabilities.Contrast.CONTRAST_FOUR;
        } else if (AndroidCameraCapabilities.VALUE_FIVE.equals(param)) {
            return CameraCapabilities.Contrast.CONTRAST_FIVE;
        } else if (AndroidCameraCapabilities.VALUE_SIX.equals(param)) {
            return CameraCapabilities.Contrast.CONTRAST_SIX;
        } else {
            return null;
        }
    }
    /* @} */

    // SPRD Bug:474715 Feature:Brightness.
    private CameraCapabilities.BrightNess getBrightNessFromParameters(String param) {
        if (AndroidCameraCapabilities.VALUE_ZERO.equals(param)) {
            return CameraCapabilities.BrightNess.BRIGHTNESS_ZERO;
        } else if (AndroidCameraCapabilities.VALUE_ONE.equals(param)) {
            return CameraCapabilities.BrightNess.BRIGHTNESS_ONE;
        } else if (AndroidCameraCapabilities.VALUE_TWO.equals(param)) {
            return CameraCapabilities.BrightNess.BRIGHTNESS_TWO;
        } else if (AndroidCameraCapabilities.VALUE_THREE.equals(param)) {
            return CameraCapabilities.BrightNess.BRIGHTNESS_THREE;
        } else if (AndroidCameraCapabilities.VALUE_FOUR.equals(param)) {
            return CameraCapabilities.BrightNess.BRIGHTNESS_FOUR;
        } else if (AndroidCameraCapabilities.VALUE_FIVE.equals(param)) {
            return CameraCapabilities.BrightNess.BRIGHTNESS_FIVE;
        } else if (AndroidCameraCapabilities.VALUE_SIX.equals(param)) {
            return CameraCapabilities.BrightNess.BRIGHTNESS_SIX;
        } else {
            return null;
        }
    }
    /* @} */

    /*
     * SPRD Bug:474724 Feature:ISO. @{
     */
    private CameraCapabilities.ISO getISOFromParameters(String param) {
        if (SprdCamera.SprdParameters.ISO_AUTO.equals(param)) {
            return CameraCapabilities.ISO.AUTO;
        } else if (SprdCamera.SprdParameters.ISO_100.equals(param)) {
            return CameraCapabilities.ISO.ISO_100;
        } else if (SprdCamera.SprdParameters.ISO_200.equals(param)) {
            return CameraCapabilities.ISO.ISO_200;
        } else if (SprdCamera.SprdParameters.ISO_400.equals(param)) {
            return CameraCapabilities.ISO.ISO_400;
        } else if (SprdCamera.SprdParameters.ISO_800.equals(param)) {
            return CameraCapabilities.ISO.ISO_800;
        } else if (SprdCamera.SprdParameters.ISO_1600.equals(param)) {
            return CameraCapabilities.ISO.ISO_1600;
        } else {
            return null;
        }
    }
    /* @} */

    /*
     * SPRD Bug:474718 Feature:Metering. @{
     */
    private CameraCapabilities.Metering getMeteringFromParameters(String param) {
        if (SprdCamera.SprdParameters.AUTO_EXPOSURE_FRAME_AVG.equals(param)) {
            return CameraCapabilities.Metering.FRAMEAVERAGE;
        } else if (SprdCamera.SprdParameters.AUTO_EXPOSURE_CENTER_WEIGHTED.equals(param)) {
            return CameraCapabilities.Metering.CENTERWEIGHTED;
        } else if (SprdCamera.SprdParameters.AUTO_EXPOSURE_SPOT_METERING.equals(param)) {
            return CameraCapabilities.Metering.SPOTMETERING;
        } else {
            return null;
        }
    }
    /* @} */

    /*
     * SPRD Bug:474722 Feature:Saturation. @{
     */
    private CameraCapabilities.Saturation getSaturationFromParameters(String param) {
        if (AndroidCameraCapabilities.VALUE_ZERO.equals(param)) {
            return CameraCapabilities.Saturation.SATURATION_ZERO;
        } else if (AndroidCameraCapabilities.VALUE_ONE.equals(param)) {
            return CameraCapabilities.Saturation.SATURATION_ONE;
        } else if (AndroidCameraCapabilities.VALUE_TWO.equals(param)) {
            return CameraCapabilities.Saturation.SATURATION_TWO;
        } else if (AndroidCameraCapabilities.VALUE_THREE.equals(param)) {
            return CameraCapabilities.Saturation.SATURATION_THREE;
        } else if (AndroidCameraCapabilities.VALUE_FOUR.equals(param)) {
            return CameraCapabilities.Saturation.SATURATION_FOUR;
        } else if (AndroidCameraCapabilities.VALUE_FIVE.equals(param)) {
            return CameraCapabilities.Saturation.SATURATION_FIVE;
        } else if (AndroidCameraCapabilities.VALUE_SIX.equals(param)) {
            return CameraCapabilities.Saturation.SATURATION_SIX;
        } else {
            return null;
        }
    }
    /* @} */
}
