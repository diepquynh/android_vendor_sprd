package com.android.ex.camera2.portability;

import java.util.EnumSet;
import java.util.Locale;
import java.util.Set;
import java.util.HashSet;
import java.util.List;
import java.util.ArrayList;

import com.android.ex.camera2.portability.CameraCapabilities.Stringifier;
import com.android.ex.camera2.portability.debug.Log;

public class SprdCameraCapabilities {
    private static Log.Tag TAG = new Log.Tag("SprdCamCapabs");

    protected final EnumSet<Antibanding> mSupportedAntibanding= EnumSet.noneOf(Antibanding.class);
    // SPRD:Add for color effect Bug 474727
    protected final EnumSet<ColorEffect> mSupportedColorEffects = EnumSet.noneOf(ColorEffect.class);
    // SPRD: fix bug 473462 add burst capture
    protected final EnumSet<BurstNumber> mSupportedBurstNumber = EnumSet.noneOf(BurstNumber.class);
    protected final EnumSet<Contrast> mSupportedContrast = EnumSet.noneOf(Contrast.class);
    protected final EnumSet<BrightNess> mSupportedBrightNess = EnumSet.noneOf(BrightNess.class);

    /**
     * SPRD:Add for Antibanding.
     */
    public enum Antibanding {
        AUTO,
        ANTIBANDING_50HZ,
        ANTIBANDING_60HZ,
        OFF,
    }

    /* SPRD:Add for color effect Bug 474727 @{ */
    public enum ColorEffect {
        NONE,
        MONO,
        NEGATIVE,
        SEPIA,
        COLD,
        ANTIQUE,
    }
    /* @} */

    /**
     * SPRD: fix bug 473462 add for burst capture
     */
    public enum BurstNumber {
        ONE,
        THREE,
        SIX,
        TEN,
        NINETYNINE,
    }

    /*
     * SPRD Bug:474721 Feature:Contrast. @{
     */
    public enum Contrast {
        CONTRAST_ZERO,
        CONTRAST_ONE,
        CONTRAST_TWO,
        CONTRAST_THREE,
        CONTRAST_FOUR,
        CONTRAST_FIVE,
        CONTRAST_SIX,
    }

    public enum BrightNess {
        BRIGHTNESS_ZERO,
        BRIGHTNESS_ONE,
        BRIGHTNESS_TWO,
        BRIGHTNESS_THREE,
        BRIGHTNESS_FOUR,
        BRIGHTNESS_FIVE,
        BRIGHTNESS_SIX,
    }

    public boolean supports(Contrast contrast) {
        return (contrast != null && mSupportedContrast.contains(contrast));
    }

    public final Set<Contrast> getSupportedContrast() {
        return mSupportedContrast;
    }
    /* @} */

    /*
     * SPRD Bug:474724 Feature:ISO. @{
     */
    protected final EnumSet<ISO> mSupportedISO = EnumSet.noneOf(ISO.class);

    public enum ISO {
        AUTO,
        ISO_1600,
        ISO_800,
        ISO_400,
        ISO_200,
        ISO_100,
    }

    public boolean supports(ISO os) {
        return (os != null && mSupportedISO.contains(os));
    }

    public final Set<ISO> getSupportedISO() {
        return new HashSet<ISO>(mSupportedISO);
    }
    /* @} */

    /*
     * SPRD Bug:474718 Feature:Metering. @{
     */
    protected final EnumSet<Metering> mSupportedMetering = EnumSet.noneOf(Metering.class);

    public enum Metering {
        FRAMEAVERAGE,
        CENTERWEIGHTED,
        SPOTMETERING,
    }

    public boolean supports(Metering mt) {
        return (mt != null && mSupportedMetering.contains(mt));
    }

    public final Set<Metering> getSupportedMetering() {
        return new HashSet<Metering>(mSupportedMetering);
    }
    /* @} */

    /*
     * SPRD Bug:474722 Feature:Saturation. @{
     */
    protected final EnumSet<Saturation> mSupportedSaturation = EnumSet.noneOf(Saturation.class);

    public enum Saturation {
        SATURATION_ZERO,
        SATURATION_ONE,
        SATURATION_TWO,
        SATURATION_THREE,
        SATURATION_FOUR,
        SATURATION_FIVE,
        SATURATION_SIX,
    }

    public boolean supports(Saturation saturation) {
        return (saturation != null && mSupportedSaturation.contains(saturation));
    }

    public final Set<Saturation> getSupportedSaturation() {
        return mSupportedSaturation;
    }
    /* @} */

    /*
     * SPRD Bug:474696 Feature:Slow-Motion. @{
     */
    protected final ArrayList<String> mSupportedSlowMotion = new ArrayList<String>();

    public final List<String> getSupportedSlowMotion() {
        return mSupportedSlowMotion;
    }

    public final boolean supports(String value, List<String> supported) {
        return supported == null ? false : supported.indexOf(value) >= 0;
    }
    /* @} */

    SprdCameraCapabilities(Stringifier stringifier) {
    }

    public SprdCameraCapabilities(SprdCameraCapabilities src) {
        mSupportedAntibanding.addAll(src.mSupportedAntibanding);
        mSupportedColorEffects.addAll(src.mSupportedColorEffects);//SPRD:Add for color effect Bug 474727
        mSupportedBurstNumber.addAll(src.mSupportedBurstNumber);//SPRD: fix bug 473462 add burst capture
        mSupportedContrast.addAll(src.mSupportedContrast);// SPRD Bug:474721 Feature:Contrast.
        mSupportedBrightNess.addAll(src.mSupportedBrightNess);// SPRD Bug:474715 Feature:Brightness.
        // SPRD Bug:474724 Feature:ISO.
        mSupportedISO.addAll(src.mSupportedISO);
        // SPRD Bug:474718 Feature:Metering.
        mSupportedMetering.addAll(src.mSupportedMetering);
        // SPRD Bug:474722 Feature:Saturation.
        mSupportedSaturation.addAll(src.mSupportedSaturation);
        // SPRD Bug:474696 Feature:Slow-Motion.
        mSupportedSlowMotion.addAll(src.mSupportedSlowMotion);
    }

    public final boolean supports(Antibanding antibanding) {
        return (antibanding != null && mSupportedAntibanding.contains(antibanding));
    }

    public final Set<Antibanding> getSupportedAntibanding() {
        return mSupportedAntibanding;
    }

    /* SPRD:Add for color effect Bug 474727 @{ */
    public final Set<ColorEffect> getSupportedColorEffects() {
        return new HashSet<ColorEffect>(mSupportedColorEffects);
    }

    public final boolean supports(ColorEffect colorEffect) {
        return (colorEffect != null && mSupportedColorEffects.contains(colorEffect));
    }
    /* @} */

    /*
     * SPRD Bug:474715 Feature:Brightness. @{
     */

    public boolean supports(BrightNess bn) {
        return (bn != null && mSupportedBrightNess.contains(bn));
    }

    public final Set<BrightNess> getSupportedBrightNess() {
        return new HashSet<BrightNess>(mSupportedBrightNess);
    }
    /* @} */


    public static class SprdStringifier {
        public Antibanding antibandingModeFromString(String val) {
            if (val == null) {
                return Antibanding.values()[0];
            }
            try {
                return Antibanding.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                Log.e(TAG, "antibandingModeFromString IllegalArgumentException !");
                return Antibanding.values()[0];
            }
        }


        /**
         * Converts the string to underscore-delimited uppercase to match the enum constant names.
         *
         * @param apiCase An API-related string representation.
         * @return The converted string.
         */
        private static String toEnumCase(String apiCase) {
            return apiCase.toUpperCase(Locale.US).replaceAll("-", "_");
        }

        /**
         * Converts the string to hyphen-delimited lowercase for compatibility with multiple APIs.
         *
         * @param enumCase The name of an enum constant.
         * @return The converted string.
         */
        private static String toApiCase(String enumCase) {
            return enumCase.toLowerCase(Locale.US).replaceAll("_", "-");
        }

        /* SPRD:Add for color effect Bug 474727 @{ */
        public String stringify(ColorEffect colorEffect) {
            return toApiCase(colorEffect.name());
        }

        public ColorEffect colorEffectFromString(String val) {
            if (val == null) {
                return ColorEffect.values()[0];
            }
            try {
                return ColorEffect.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                return ColorEffect.values()[0];
            }
        }
        /* @} */

        /**
         * SPRD: fix bug 473462 add burst capture @{
         */
        public BurstNumber burstNumberFromString(String val) {
            if (val == null) {
                return BurstNumber.values()[0];
            }
            try {
                return BurstNumber.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                return BurstNumber.values()[0];
            }
        }
        /**
         * @}
         */

        /*
         * SPRD Bug:474721 Feature:Contrast. @{
         */
        public Contrast contrastFromString(String val) {
            if (val == null) {
                return Contrast.values()[0];
            }
            try {
                return Contrast.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                return Contrast.values()[0];
            }
        }

        public String stringify(Contrast contrast) {
            return toApiCase(contrast.name());
        }
        /* @} */

        /*
         * SPRD Bug:474715 Feature:Brightness. @{
         */
        public BrightNess brightnessFromString(String val) {
            if (val == null) {
                return BrightNess.values()[0];
            }
            try {
                return BrightNess.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                return BrightNess.values()[0];
            }
        }

        public String stringify(BrightNess brightness) {
            return toApiCase(brightness.name());
        }

        /*
         * SPRD Bug:474724 Feature:ISO. @{
         */
        public ISO isoModeFromString(String val) {
            if (val == null) {
                return ISO.values()[0];
            }
            try {
                return ISO.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                return ISO.values()[0];
            }
        }

        public String stringify(ISO iso) {
            return toApiCase(iso.name());
        }
        /* @} */

        /*
         * SPRD Bug:474718 Feature:Metering. @{
         */
        public Metering meteringFromString(String val) {
            if (val == null) {
                return Metering.values()[0];
            }
            try {
                return Metering.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                return Metering.values()[0];
            }
        }

        public String stringify(Metering metering) {
            return toApiCase(metering.name());
        }
        /* @} */

        /*
         * SPRD Bug:474722 Feature:Saturation. @{
         */
        public Saturation saturationFromString(String val) {
            if (val == null) {
                return Saturation.values()[0];
            }
            try {
                return Saturation.valueOf(toEnumCase(val));
            } catch (IllegalArgumentException ex) {
                return Saturation.values()[0];
            }
        }

        public String stringify(Saturation saturation) {
            return toApiCase(saturation.name());
        }
        /* @} */
    }
}
