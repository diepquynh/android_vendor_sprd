package com.android.ex.camera2.portability;

import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.SprdCamera;
import android.hardware.SprdCamera.SprdParameters;
import android.os.Handler;
import android.os.Looper;
import com.android.ex.camera2.portability.debug.Log;


public abstract class SprdAndroidCameraAgentImpl extends CameraAgent {
    private static final Log.Tag TAG = new Log.Tag("SprdAndroidCameraAgentImpl");

    public abstract class SprdAndroidCameraProxyImpl extends CameraAgent.CameraProxy {
        @Override
        public int cancelBurstCapture(CancelBurstCaptureCallback cb) {
            return -1;
        }
    }

    public class SprdCameraHandler extends HistoryHandler {

        SprdCameraHandler(Looper looper) {
            super(looper);
        }

        protected void applySuperSettingsToParameters(final CameraSettings settings,
                final Parameters parameters, AndroidCameraCapabilities capabilities) {
            if (parameters instanceof SprdParameters) {
                applySuperSettingsToParameters(settings, (SprdParameters)parameters, capabilities);
            }
        }

        protected void applySuperSettingsToParameters(final CameraSettings settings,
                final SprdParameters parameters, AndroidCameraCapabilities capabilities) {
            /* SPRD:Add for white balance */
            if (capabilities.supports(settings.getWhiteBalance())) {
                parameters.setWhiteBalance(getWhiteBalanceString(settings
                        .getWhiteBalance()));
            }

            /* SPRD:Add for color effect */
            if (capabilities.supports(settings.getCurrentColorEffect())) {
                parameters.setColorEffect(getColorEffectModeString(settings
                        .getCurrentColorEffect()));
            }

            /* SPRD:Add for antibanding */
            if (capabilities.supports(settings.getAntibanding())) {
                parameters.setAntibanding(getAntibandingString(settings
                        .getAntibanding()));
            }

            /* SPRD: fix bug 473462 add burst capture @ */
            if (settings.getBurstPicNum() != null) {
                parameters.setContinuousCount(getBursetCaptureString(settings
                        .getBurstPicNum()));
            }
            /* @} */

            // SPRD Bug:474721 Feature:Contrast.
            if (capabilities.supports(settings.getCurrentContrast())) {
                parameters.setContrast(getContrastString(settings.getCurrentContrast()));
            }

            // SPRD Bug:474715 Feature:Brightness.
            if (capabilities.supports(settings.getBrightNess())) {
                parameters.setBrightness(getBrightnessString(settings.getBrightNess()));
            }

            // SPRD Bug:474724 Feature:ISO.
            if (capabilities.supports(settings.getISO())) {
                parameters.setISO(getISOString(settings.getISO()));
            }

            // SPRD Bug:474718 Feature:Metering.
            if (capabilities.supports(settings.getMetering())) {
                parameters.setMeteringMode(getMeteringString(settings.getMetering()));
            }

            // SPRD Bug:474722 Feature:Saturation.
            if (capabilities.supports(settings.getCurrentSaturation())) {
                parameters.setSaturation(getSaturationString(settings.getCurrentSaturation()));
            }

            //SPRD:fix bug474672 add ucam beauty for api1
            parameters.set("perfectskinlevel", settings.getSkinWhitenLevel());
            Log.e(TAG, " Camera1 API setParameters = " + parameters.flatten());

            // SPRD Bug:474696 Feature:Slow-Motion.
            if (capabilities.supports(settings.getCurrentVideoSlowMotion(),
                    capabilities.getSupportedSlowMotion())) {
                parameters.setSlowmotion(settings.getCurrentVideoSlowMotion());
            }

            // SPRD Feature: EOIS
            parameters.setEOIS(settings.getEOISEnable());

            // SPRD Bug:500099 Feature:Mirror.
            parameters.set("mirror", settings.getfrontCameraMirror() ? "true" : "false");
        }

        /**
         * SPRD:Add for antibanding
         */
        public String getAntibandingString(
                CameraCapabilities.Antibanding antibanding) {
            String antibandingParameter = null;
            switch (antibanding) {
            case AUTO: {
                antibandingParameter = SprdCamera.SprdParameters.ANTIBANDING_AUTO;
                break;
            }
            case ANTIBANDING_50HZ: {
                antibandingParameter = SprdCamera.SprdParameters.ANTIBANDING_50HZ;
                break;
            }
            case ANTIBANDING_60HZ: {
                antibandingParameter = SprdCamera.SprdParameters.ANTIBANDING_60HZ;
                break;
            }
            case OFF: {
                antibandingParameter = SprdCamera.SprdParameters.ANTIBANDING_OFF;
                break;
            }
            default: {
                antibandingParameter = SprdCamera.SprdParameters.ANTIBANDING_AUTO;
                break;
            }
            }
            return antibandingParameter;
        }

        /**
         * SPRD:Add for coloreffect
         */
        private String getColorEffectModeString(
                CameraCapabilities.ColorEffect colorEffect) {
            String colorParametersString = null;
            switch (colorEffect) {
            case NONE: {
                colorParametersString = SprdCamera.SprdParameters.EFFECT_NONE;
                break;
            }
            case MONO: {
                colorParametersString = SprdCamera.SprdParameters.EFFECT_MONO;
                break;
            }
            case NEGATIVE: {
                colorParametersString = SprdCamera.SprdParameters.EFFECT_NEGATIVE;
                break;
            }
            case SEPIA: {
                colorParametersString = SprdCamera.SprdParameters.EFFECT_SEPIA;
                break;
            }
            case COLD: {
                colorParametersString = SprdCamera.SprdParameters.EFFECT_AQUA;
                break;
            }
            case ANTIQUE: {
                colorParametersString = SprdCamera.SprdParameters.EFFECT_SOLARIZE;
                break;
            }
            default: {
                colorParametersString = SprdCamera.SprdParameters.EFFECT_NONE;
                break;
            }
            }
            return colorParametersString;
        }

        /**
         * SPRD:Add for whitebalance
         */
        private String getWhiteBalanceString(
                CameraCapabilities.WhiteBalance whiteBalance) {
            String whiteBalanceParametersString = null;
            switch (whiteBalance) {
            case AUTO: {
                whiteBalanceParametersString = SprdCamera.SprdParameters.WHITE_BALANCE_AUTO;
                break;
            }
            case CLOUDY_DAYLIGHT: {
                whiteBalanceParametersString = SprdCamera.SprdParameters.WHITE_BALANCE_CLOUDY_DAYLIGHT;
                break;
            }
            case DAYLIGHT: {
                whiteBalanceParametersString = SprdCamera.SprdParameters.WHITE_BALANCE_DAYLIGHT;
                break;
            }
            case FLUORESCENT: {
                whiteBalanceParametersString = SprdCamera.SprdParameters.WHITE_BALANCE_FLUORESCENT;
                break;
            }
            case INCANDESCENT: {
                whiteBalanceParametersString = SprdCamera.SprdParameters.WHITE_BALANCE_INCANDESCENT;
                break;
            }
            case SHADE: {
                whiteBalanceParametersString = SprdCamera.SprdParameters.WHITE_BALANCE_SHADE;
                break;
            }
            case TWILIGHT: {
                whiteBalanceParametersString = SprdCamera.SprdParameters.WHITE_BALANCE_TWILIGHT;
                break;
            }
            case WARM_FLUORESCENT: {
                whiteBalanceParametersString = SprdCamera.SprdParameters.WHITE_BALANCE_WARM_FLUORESCENT;
                break;
            }
            default: {
                whiteBalanceParametersString = SprdCamera.SprdParameters.WHITE_BALANCE_AUTO;
                break;
            }
            }
            return whiteBalanceParametersString;
        }

        /**
         * SPRD:fix bug 473462 add burst capture
         */
        public String getBursetCaptureString(CameraCapabilities.BurstNumber burst) {
            String burstParameters = null;
            switch (burst) {
            case ONE: {
                burstParameters = SprdAndroidCameraCapabilities.VALUE_ONE;
                break;
            }
            case THREE: {
                burstParameters = SprdAndroidCameraCapabilities.VALUE_THREE;
                break;
            }
            case SIX: {
                burstParameters = SprdAndroidCameraCapabilities.VALUE_SIX;
                break;
            }
            case TEN: {
                burstParameters = SprdAndroidCameraCapabilities.VALUE_TEN;
                break;
            }
            default: {
                burstParameters = AndroidCameraCapabilities.VALUE_ONE;
                break;
            }
            }
            return burstParameters;
        }

        // SPRD Bug:474721 Feature:Contrast.
        public String getContrastString(CameraCapabilities.Contrast contrast) {
            String contrastParameter = null;
            switch (contrast) {
                case CONTRAST_ZERO: {
                    contrastParameter = AndroidCameraCapabilities.VALUE_ZERO;
                    break;
                }
                case CONTRAST_ONE: {
                    contrastParameter = AndroidCameraCapabilities.VALUE_ONE;
                    break;
                }
                case CONTRAST_TWO: {
                    contrastParameter = AndroidCameraCapabilities.VALUE_TWO;
                    break;
                }
                case CONTRAST_THREE: {
                    contrastParameter = AndroidCameraCapabilities.VALUE_THREE;
                    break;
                }
                case CONTRAST_FOUR: {
                    contrastParameter = AndroidCameraCapabilities.VALUE_FOUR;
                    break;
                }
                case CONTRAST_FIVE: {
                    contrastParameter = AndroidCameraCapabilities.VALUE_FIVE;
                    break;
                }
                case CONTRAST_SIX: {
                    contrastParameter = AndroidCameraCapabilities.VALUE_SIX;
                    break;
                }
                default: {
                    contrastParameter = AndroidCameraCapabilities.VALUE_THREE;
                }
            }
            return contrastParameter;
        }

        // SPRD Bug:474715 Feature:Brightness.
        public String getBrightnessString(CameraCapabilities.BrightNess brightness) {
            String brightnessParameter = null;
            switch (brightness) {
                case BRIGHTNESS_ZERO: {
                    brightnessParameter = AndroidCameraCapabilities.VALUE_ZERO;
                    break;
                }
                case BRIGHTNESS_ONE: {
                    brightnessParameter = AndroidCameraCapabilities.VALUE_ONE;
                    break;
                }
                case BRIGHTNESS_TWO: {
                    brightnessParameter = AndroidCameraCapabilities.VALUE_TWO;
                    break;
                }
                case BRIGHTNESS_THREE: {
                    brightnessParameter = AndroidCameraCapabilities.VALUE_THREE;
                    break;
                }
                case BRIGHTNESS_FOUR: {
                    brightnessParameter = AndroidCameraCapabilities.VALUE_FOUR;
                    break;
                }
                case BRIGHTNESS_FIVE: {
                    brightnessParameter = AndroidCameraCapabilities.VALUE_FIVE;
                    break;
                }
                case BRIGHTNESS_SIX: {
                    brightnessParameter = AndroidCameraCapabilities.VALUE_SIX;
                    break;
                }
                default: {
                    brightnessParameter = AndroidCameraCapabilities.VALUE_THREE;
                }
            }
            return brightnessParameter;
        }

        // SPRD Bug:474724 Feature:ISO.
        public String getISOString(CameraCapabilities.ISO iso) {
            String isoParameter = null;
            switch (iso) {
                case AUTO: {
                    isoParameter = SprdCamera.SprdParameters.ISO_AUTO;
                    break;
                }
                case ISO_1600: {
                    isoParameter = SprdCamera.SprdParameters.ISO_1600;
                    break;
                }
                case ISO_800: {
                    isoParameter = SprdCamera.SprdParameters.ISO_800;
                    break;
                }
                case ISO_400: {
                    isoParameter = SprdCamera.SprdParameters.ISO_400;
                    break;
                }
                case ISO_200: {
                    isoParameter = SprdCamera.SprdParameters.ISO_200;
                    break;
                }
                case ISO_100: {
                    isoParameter = SprdCamera.SprdParameters.ISO_100;
                    break;
                }
                default: {
                    isoParameter = SprdCamera.SprdParameters.ISO_AUTO;
                    break;
                }
            }
            return isoParameter;
        }

        // SPRD Bug:474718 Feature:Metering.
        public String getMeteringString(CameraCapabilities.Metering metering) {
            String meteringParameter = null;
            switch (metering) {
                case FRAMEAVERAGE: {
                    meteringParameter = SprdParameters.AUTO_EXPOSURE_FRAME_AVG;
                    break;
                }
                case CENTERWEIGHTED: {
                    meteringParameter = SprdParameters.AUTO_EXPOSURE_CENTER_WEIGHTED;
                    break;
                }
                case SPOTMETERING: {
                    meteringParameter = SprdParameters.AUTO_EXPOSURE_SPOT_METERING;
                    break;
                }
                default: {
                    meteringParameter = SprdParameters.AUTO_EXPOSURE_FRAME_AVG;
                    break;
                }
            }
            return meteringParameter;
        }

        // SPRD Bug:474722 Feature:Saturation.
        public String getSaturationString(CameraCapabilities.Saturation saturation) {
            String saturationParameter = null;
            switch (saturation) {
                case SATURATION_ZERO: {
                    saturationParameter = AndroidCameraCapabilities.VALUE_ZERO;
                    break;
                }
                case SATURATION_ONE: {
                    saturationParameter = AndroidCameraCapabilities.VALUE_ONE;
                    break;
                }
                case SATURATION_TWO: {
                    saturationParameter = AndroidCameraCapabilities.VALUE_TWO;
                    break;
                }
                case SATURATION_THREE: {
                    saturationParameter = AndroidCameraCapabilities.VALUE_THREE;
                    break;
                }
                case SATURATION_FOUR: {
                    saturationParameter = AndroidCameraCapabilities.VALUE_FOUR;
                    break;
                }
                case SATURATION_FIVE: {
                    saturationParameter = AndroidCameraCapabilities.VALUE_FIVE;
                    break;
                }
                case SATURATION_SIX: {
                    saturationParameter = AndroidCameraCapabilities.VALUE_SIX;
                    break;
                }
                default: {
                    saturationParameter = AndroidCameraCapabilities.VALUE_THREE;
                }
            }
            return saturationParameter;
        }
    }
}
