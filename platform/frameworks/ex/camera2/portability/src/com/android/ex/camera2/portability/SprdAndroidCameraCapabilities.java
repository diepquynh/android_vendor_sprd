package com.android.ex.camera2.portability;

import android.hardware.Camera;
import android.hardware.SprdCamera;
import android.hardware.SprdCamera.SprdParameters;

import com.android.ex.camera2.portability.CameraCapabilities.Stringifier;
import com.android.ex.camera2.portability.SprdCameraCapabilities.Antibanding;
import com.android.ex.camera2.portability.SprdCameraCapabilities.ColorEffect;

import com.android.ex.camera2.portability.debug.Log;

import java.util.List;


public class SprdAndroidCameraCapabilities extends CameraCapabilities {

    private static Log.Tag TAG = new Log.Tag("SprdAndCamCapabs");

    /**
     * SPRD: fix bug 473462 add burst capture
     */
    public static final String VALUE_ZERO = "0";
    public static final String VALUE_ONE = "1";
    public static final String VALUE_TWO = "2";
    public static final String VALUE_THREE = "3";
    public static final String VALUE_FOUR = "4";
    public static final String VALUE_FIVE = "5";
    public static final String VALUE_SIX = "6";
    public static final String VALUE_TEN = "10";
    public static final String VALUE_NINETYNINE = "99";

    SprdAndroidCameraCapabilities(SprdAndroidCameraCapabilities src) {
        super(src);
    }

    SprdAndroidCameraCapabilities(Stringifier stringifier, Camera.Parameters p) {
        super(stringifier);

        if (p instanceof SprdCamera.SprdParameters) {
            SprdCamera.SprdParameters params = (SprdCamera.SprdParameters) p;
            buidAntibanding(params);// SPRD:Add for antibanding
            buildColorEffect(params);// SPRD:Add for color effect Bug 474727
            buildContrast(params);// SPRD Bug:474721 Feature:Contrast.
            buildBrightness(params);// SPRD Bug:474715 Feature:Brightness.
            // SPRD Bug:474724 Feature:ISO.
            buildISO(params);
            // SPRD Bug:474718 Feature:Metering.
            buildMetering(params);
            // SPRD Bug:474722 Feature:Saturation.
            buildSaturation(params);
            // SPRD Bug:474696 Feature:Slow-Motion.
            buildVideoSlowMotion(params);
        }
    }

    protected void buidAntibanding(SprdCamera.SprdParameters p) {
        List<String> supportedAntibanding = p.getSupportedAntibanding();
        Log.d(TAG, "supportedAntibanding" + supportedAntibanding);
        if (supportedAntibanding != null) {
            for (String antibanding : supportedAntibanding) {
                if (SprdCamera.SprdParameters.ANTIBANDING_AUTO.equals(antibanding)) {
                    mSupportedAntibanding.add(Antibanding.AUTO);
                } else if (SprdCamera.SprdParameters.ANTIBANDING_50HZ.equals(antibanding)) {
                    mSupportedAntibanding.add(Antibanding.ANTIBANDING_50HZ);
                } else if (SprdCamera.SprdParameters.ANTIBANDING_60HZ.equals(antibanding)) {
                    mSupportedAntibanding.add(Antibanding.ANTIBANDING_60HZ);
                } else if (SprdCamera.SprdParameters.ANTIBANDING_OFF.equals(antibanding)) {
                    mSupportedAntibanding.add(Antibanding.OFF);
                }
            }
        }
    }

    /* SPRD:Add for color effect Bug 474727 @{ */
    protected void buildColorEffect(SprdCamera.SprdParameters p) {
        List<String> supportedColorEffects = p.getSupportedColorEffects();
        Log.d(TAG, "supportedColorEffects" + supportedColorEffects);
        if (supportedColorEffects != null) {
            for (String colorEffect : supportedColorEffects) {
                Log.d(TAG, "colorEffect" + colorEffect);
                if (SprdCamera.SprdParameters.EFFECT_NONE.equals(colorEffect)) {
                    mSupportedColorEffects.add(ColorEffect.NONE);
                } else if (SprdCamera.SprdParameters.EFFECT_MONO.equals(colorEffect)) {
                    mSupportedColorEffects.add(ColorEffect.MONO);
                } else if (SprdCamera.SprdParameters.EFFECT_NEGATIVE.equals(colorEffect)) {
                    mSupportedColorEffects.add(ColorEffect.NEGATIVE);
                } else if (SprdCamera.SprdParameters.EFFECT_SEPIA.equals(colorEffect)) {
                    mSupportedColorEffects.add(ColorEffect.SEPIA);
                } else if (SprdCamera.SprdParameters.EFFECT_AQUA.equals(colorEffect)) {
                    mSupportedColorEffects.add(ColorEffect.COLD);
                } else if (SprdCamera.SprdParameters.EFFECT_SOLARIZE.equals(colorEffect)) {
                    mSupportedColorEffects.add(ColorEffect.ANTIQUE);
                }
            }
        }
    }
    /* @} */

    // SPRD Bug:474721 Feature:Contrast.
    private void buildContrast(SprdCamera.SprdParameters p) {
        List<String> supportedContrast = p.getSupportedContrast();
        Log.d(TAG, "supportedContrast" + supportedContrast);
        if (supportedContrast != null) {
            for (String contrast : supportedContrast) {
                if (VALUE_ZERO.equals(contrast)) {
                    mSupportedContrast.add(CameraCapabilities.Contrast.CONTRAST_ZERO);
                } else if (VALUE_ONE.equals(contrast)) {
                    mSupportedContrast.add(CameraCapabilities.Contrast.CONTRAST_ONE);
                } else if (VALUE_TWO.equals(contrast)) {
                    mSupportedContrast.add(CameraCapabilities.Contrast.CONTRAST_TWO);
                } else if (VALUE_THREE.equals(contrast)) {
                    mSupportedContrast.add(CameraCapabilities.Contrast.CONTRAST_THREE);
                } else if (VALUE_FOUR.equals(contrast)) {
                    mSupportedContrast.add(CameraCapabilities.Contrast.CONTRAST_FOUR);
                } else if (VALUE_FIVE.equals(contrast)) {
                    mSupportedContrast.add(CameraCapabilities.Contrast.CONTRAST_FIVE);
                } else if (VALUE_SIX.equals(contrast)) {
                    mSupportedContrast.add(CameraCapabilities.Contrast.CONTRAST_SIX);
                }
            }
        }
    }

    // SPRD Bug:474715 Feature:Brightness.
    private void buildBrightness(SprdCamera.SprdParameters p) {
        List<String> supportedBrightness = p.getSupportedBrightness();
        Log.d(TAG, "supportedBrightness" + supportedBrightness);
        if (supportedBrightness != null) {
            for (String brightness : supportedBrightness) {
                if (VALUE_ZERO.equals(brightness)) {
                    mSupportedBrightNess.add(CameraCapabilities.BrightNess.BRIGHTNESS_ZERO);
                } else if (VALUE_ONE.equals(brightness)) {
                    mSupportedBrightNess.add(CameraCapabilities.BrightNess.BRIGHTNESS_ONE);
                } else if (VALUE_TWO.equals(brightness)) {
                    mSupportedBrightNess.add(CameraCapabilities.BrightNess.BRIGHTNESS_TWO);
                } else if (VALUE_THREE.equals(brightness)) {
                    mSupportedBrightNess.add(CameraCapabilities.BrightNess.BRIGHTNESS_THREE);
                } else if (VALUE_FOUR.equals(brightness)) {
                    mSupportedBrightNess.add(CameraCapabilities.BrightNess.BRIGHTNESS_FOUR);
                } else if (VALUE_FIVE.equals(brightness)) {
                    mSupportedBrightNess.add(CameraCapabilities.BrightNess.BRIGHTNESS_FIVE);
                } else if (VALUE_SIX.equals(brightness)) {
                    mSupportedBrightNess.add(CameraCapabilities.BrightNess.BRIGHTNESS_SIX);
                }
            }
        }
    }

    // SPRD Bug:474724 Feature:ISO.
    private void buildISO(SprdCamera.SprdParameters p) {
        List<String> supportedISO = p.getSupportedISO();
        Log.d(TAG, "supportedISO" + supportedISO);
        if (supportedISO != null) {
            for (String iso : supportedISO) {
                if (iso != null) {
                    if (SprdCamera.SprdParameters.ISO_AUTO.equals(iso)) {
                        mSupportedISO.add(CameraCapabilities.ISO.AUTO);
                    } else if (SprdCamera.SprdParameters.ISO_100.equals(iso)) {
                        mSupportedISO.add(CameraCapabilities.ISO.ISO_100);
                    } else if (SprdCamera.SprdParameters.ISO_200.equals(iso)) {
                        mSupportedISO.add(CameraCapabilities.ISO.ISO_200);
                    } else if (SprdCamera.SprdParameters.ISO_400.equals(iso)) {
                        mSupportedISO.add(CameraCapabilities.ISO.ISO_400);
                    } else if (SprdCamera.SprdParameters.ISO_800.equals(iso)) {
                        mSupportedISO.add(CameraCapabilities.ISO.ISO_800);
                    } else if (SprdCamera.SprdParameters.ISO_1600.equals(iso)) {
                        mSupportedISO.add(CameraCapabilities.ISO.ISO_1600);
                    }
                }
            }
        }
    }

    // SPRD Bug:474718 Feature:Metering.
    private void buildMetering(SprdCamera.SprdParameters p) {
        List<String> supportedMetering = p.getSupportedMeteringMode();
        Log.d(TAG, "supportedMetering = " + supportedMetering);
        if (supportedMetering != null) {
            for (String me : supportedMetering) {
                if (SprdCamera.SprdParameters.AUTO_EXPOSURE_FRAME_AVG.equals(me)) {
                    mSupportedMetering.add(Metering.FRAMEAVERAGE);
                } else if (SprdCamera.SprdParameters.AUTO_EXPOSURE_CENTER_WEIGHTED.equals(me)) {
                    mSupportedMetering.add(Metering.CENTERWEIGHTED);
                } else if (SprdCamera.SprdParameters.AUTO_EXPOSURE_SPOT_METERING.equals(me)) {
                    mSupportedMetering.add(Metering.SPOTMETERING);
                }
            }
        }
    }

    // SPRD Bug:474722 Feature:Saturation.
    private void buildSaturation(SprdCamera.SprdParameters p) {
        List<String> supportedSaturation = p.getSupportedSaturation();
        Log.d(TAG, "supportedSaturation" + supportedSaturation);
        if (supportedSaturation != null) {
            for (String saturation : supportedSaturation) {
                if (VALUE_ZERO.equals(saturation)) {
                    mSupportedSaturation.add(CameraCapabilities.Saturation.SATURATION_ZERO);
                } else if (VALUE_ONE.equals(saturation)) {
                    mSupportedSaturation.add(CameraCapabilities.Saturation.SATURATION_ONE);
                } else if (VALUE_TWO.equals(saturation)) {
                    mSupportedSaturation.add(CameraCapabilities.Saturation.SATURATION_TWO);
                } else if (VALUE_THREE.equals(saturation)) {
                    mSupportedSaturation.add(CameraCapabilities.Saturation.SATURATION_THREE);
                } else if (VALUE_FOUR.equals(saturation)) {
                    mSupportedSaturation.add(CameraCapabilities.Saturation.SATURATION_FOUR);
                } else if (VALUE_FIVE.equals(saturation)) {
                    mSupportedSaturation.add(CameraCapabilities.Saturation.SATURATION_FIVE);
                } else if (VALUE_SIX.equals(saturation)) {
                    mSupportedSaturation.add(CameraCapabilities.Saturation.SATURATION_SIX);
                }
            }
        }
    }

    /*
     * SPRD Bug:474696 Feature:Slow-Motion. @{
     */
    private void buildVideoSlowMotion(SprdCamera.SprdParameters p) {
        List<String> supportedSlowMotion = p.getSupportedSlowmotion();
        Log.d(TAG, "supportedSlowMotion = " + supportedSlowMotion);
        if (supportedSlowMotion != null) {
            for (String slowmotion : supportedSlowMotion) {
                if (slowmotion != null) {
                    mSupportedSlowMotion.add(slowmotion);
                }
            }
        }
    }
    /* @} */
}
