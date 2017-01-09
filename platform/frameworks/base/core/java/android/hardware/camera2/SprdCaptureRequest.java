package android.hardware.camera2;

import android.hardware.camera2.CaptureRequest.Key;

/**
 * @hide
 */
public final class SprdCaptureRequest {

    private SprdCaptureRequest() {
    }

    // SPRD: Fix bug 474722 Feature:Saturation.
    public static final Key<Integer> CONTROL_SATURATION_MODE =
            new Key<Integer>("com.addParameters.saturation", int.class);

    // SPRD: Fix bug 474718 Feature:Metering.
    public static final Key<Integer> CONTROL_METERING_MODE =
            new Key<Integer>("com.addParameters.meteringMode", int.class);

    // SPRD: Fix bug 474724 Feature:ISO.
    public static final Key<Integer> CONTROL_ISO_MODE =
            new Key<Integer>("com.addParameters.iso", int.class);

    // SPRD: Fix bug 474715 Feature:Brightness.
    public static final Key<Integer> CONTROL_BRIGHTNESS_MODE =
            new Key<Integer>("com.addParameters.brightness", int.class);

    // SPRD: Fix bug 474721 Feature:Contrast.
    public static final Key<Integer> CONTROL_CONTRAST_MODE =
            new Key<Integer>("com.addParameters.contrast", int.class);

    // SPRD: Fix bug 474672 Feature:ucam beauty
    public static final Key<Integer> CONTROL_SKIN_WHITEN_MODE = new Key<Integer>(
            "com.addParameters.perfectskinlevel", int.class);

    // SPRD: Fix Bug 534257 Feature: EOIS
    public static final Key<Boolean> CONTROL_SPRD_EOIS_ENABLE = new Key<Boolean>(
            "com.addParameters.sprdEisEnabled", boolean.class);

    /*
     * SPRD: Fix bug 473462 add burst capture @{
     */
    public static final Key<Integer> SPRD_CAPTURE_MODE =
            new Key<Integer>("com.addParameters.capMode", int.class);

    public static final Key<Byte> ANDROID_SPRD_SENSOR_ORIENTATION = new Key<Byte>(
            "com.addParameters.sensororientation", byte.class);

    public static final Key<Integer> ANDROID_SPRD_SENSOR_ROTATION = new Key<Integer>(
            "com.addParameters.sensorrotation", int.class);
    /* @} */

    // SPRD: Fix bug 534917 Add for Feature:PIP/VIV.
    public static final Key<Integer> ANDROID_SPRD_PIPVIV_ENABLED = new Key<Integer>(
            "com.addParameters.sprdPipVivEnabled", int.class);

    // SPRD: Fix bug 534917 Add for Feature:HIGHISO.
    public static final Key<Integer> ANDROID_SPRD_HIGHISO_ENABLED = new Key<Integer>(
            "com.addParameters.sprdHighIsoEnabled", int.class);

    // SPRD: Fix bug 505155 add sprd zsl
    public static final Key<Integer> ANDROID_SPRD_ZSL_ENABLED = new Key<Integer>(
           "com.addParameters.sprdZslEnabled", int.class);

    // SPRD: Fix bug 500099 Feature:mirror
    public static final Key<Boolean> CONTROL_FRONT_CAMERA_MIRROR = new Key<Boolean>(
            "com.addParameters.mirror", boolean.class);
}
