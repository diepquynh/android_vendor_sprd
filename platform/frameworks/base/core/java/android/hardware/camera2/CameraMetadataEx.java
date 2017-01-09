package android.hardware.camera2;

/**
 * @hide
 */
public final class CameraMetadataEx {

    // SPRD: Bug 473462 Feature:Burst Capture.
    public static final int CONTROL_CAPTURE_MODE_ONE = 1;
    public static final int CONTROL_CAPTURE_MODE_THREE = 3;
    public static final int CONTROL_CAPTURE_MODE_SIX = 6;
    public static final int CONTROL_CAPTURE_MODE_TEN = 10;
    public static final int CONTROL_CAPTURE_MODE_NINETY_NINE = 99;

    // SPRD: Bug 474715 Feature:Brightness.
    public static final int CONTROL_BRIGHTNESS_ZERO = 0;
    public static final int CONTROL_BRIGHTNESS_ONE = 1;
    public static final int CONTROL_BRIGHTNESS_TWO = 2;
    public static final int CONTROL_BRIGHTNESS_THREE = 3;
    public static final int CONTROL_BRIGHTNESS_FOUR = 4;
    public static final int CONTROL_BRIGHTNESS_FIVE = 5;
    public static final int CONTROL_BRIGHTNESS_SIX = 6;

    // SPRD: Bug 474724 Feature:ISO.
    public static final int CONTROL_ISO_MODE_AUTO = 0;
    public static final int CONTROL_ISO_MODE_1600 = 5;
    public static final int CONTROL_ISO_MODE_800 = 4;
    public static final int CONTROL_ISO_MODE_400 = 3;
    public static final int CONTROL_ISO_MODE_200 = 2;
    public static final int CONTROL_ISO_MODE_100 = 1;

    // SPRD: Bug 474718 Feature:Metering.
    public static final int CONTROL_METERING_FRAMEAVERAGE = 0;
    public static final int CONTROL_METERING_CENTERWEIGHTED = 1;
    public static final int CONTROL_METERING_SPOTMETERING = 2;
}
