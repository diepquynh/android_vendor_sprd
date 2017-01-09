package android.hardware.camera2;

import android.hardware.camera2.CameraCharacteristics.Key;
import android.hardware.camera2.impl.PublicKey;

/**
 * @hide
 */
public class SprdCameraCharacteristics {

    public SprdCameraCharacteristics() {
    }

    // SPRD: Bug 474722 Feature:Saturation.
    public static final Key<int[]> CONTROL_AVAILABLE_SATURATION =
            new Key<int[]>("com.addParameters.availableSaturation", int[].class);

    // SPRD: Bug 474718 Feature:Metering.
    public static final Key<int[]> CONTROL_AVAILABLE_METERING =
            new Key<int[]>("com.addParameters.availableMeteringMode", int[].class);

    // SPRD: Bug 474724 Feature:ISO.
    public static final Key<int[]> CONTROL_AVAILABLE_ISO =
            new Key<int[]>("com.addParameters.availableISO", int[].class);

    // SPRD: Bug 474715 Feature:Brightness.
    public static final Key<int[]> SPRD_OEM_AVAILABLE_BRIGHTNESS =
            new Key<int[]>("com.addParameters.availableBrightness", int[].class);
    
    // SPRD: Bug 474721 Feature:Contrast.
    public static final Key<int[]> CONTROL_AVAILABLE_CONTRAST =
            new Key<int[]>("com.addParameters.availableContrast", int[].class);

    // SPRD: Bug 548832 add for smile capture
    public static final Key<Integer> CONTROL_AVAILABLE_SMILEENABLE =
            new Key<Integer>("com.addParameters.availableSmileEnable", Integer.class);

    // SPRD: Bug 549740 add for auto antibanding
    public static final Key<Integer> CONTROL_AVAILABLE_ANTIBAND_AUTO =
            new Key<Integer>("com.addParameters.availableAntibandAutoSupported", Integer.class);
}
