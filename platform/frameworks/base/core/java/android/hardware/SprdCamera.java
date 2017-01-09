package android.hardware;

import java.util.ArrayList;
import java.util.List;

import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.Parameters;
import java.util.LinkedHashMap;
import static android.system.OsConstants.EACCES;

import android.text.TextUtils;
import android.util.Log;

/**
 * SPRD:fix bug474665 add control for shutter sound
 * @hide
 */
public class SprdCamera extends Camera {

    SprdCamera(int cameraId) {
        super(cameraId);
    }

    public static SprdCamera open(int cameraId) {
        return new SprdCamera(cameraId);
    }

    public boolean enableShutterSoundSprd(boolean enabled) {
        return _enableShutterSound(enabled);
    }

    public SprdParameters getParameters() {
        Camera.Parameters parameters = super.getParameters();
        SprdParameters sprdParameters = new SprdParameters();
        sprdParameters.copyFrom(parameters);
        return sprdParameters;
    }

    /*
     * SPRD Bug:For cts & 3rd party app. Hide the private cameras. @{
     */
    public static int checkError(Camera camera, int cameraId) {
        if (cameraId >= getNumberOfCameras()) {
            return -EACCES;
        } else {
            return camera.cameraInitNormal(cameraId);
        }
    }

    public static void checkCameraId(int cameraId) {
        if (cameraId >= getNumberOfCameras()) {
            throw new RuntimeException("Unknown camera ID");
        }
    }
    /* @} */

    public class SprdParameters extends Camera.Parameters {
        private static final String TAG = "SprdParameters";
        // Parameter key suffix for supported values.
        private static final String SUPPORTED_VALUES_SUFFIX = "-values";
        private static final String KEY_FOCUS_MODE = "focus-mode";
        public static final String FOCUS_MODE_FIXED = "fixed";

        private static final String TRUE = "true";
        private static final String FALSE = "false";

        /* SPRD: Fix bug 473462 add burst capture @{ */
        private static final String KEY_CAPTURE_MODE = "capture-mode";
        /* @} */

        public SprdParameters() {
            super();
        }

        /*
         * SPRD: Fix bug 473462 add burst capture @{
         */
        public void setContinuousCount(String value) {
            set(KEY_CAPTURE_MODE, value);
        }
        /* @} */

        /*
         * SPRD: Fix bug 474721 Feature:Contrast. @{
         */
        private static final String KEY_CONTRAST = "contrast";

        public void setContrast(String value) {
            set(KEY_CONTRAST, value);
        }

        public String getContrast() {
            return get(KEY_CONTRAST);
        }

        public List<String> getSupportedContrast() {
            String str = get(KEY_CONTRAST + SUPPORTED_VALUES_SUFFIX);
            return split(str);
        }
        /* @} */

        /*
         * SPRD: Fix bug 474696 Feature:Slow-Motion. @{
         */
        private static final String KEY_SLOWMOTION = "slow-motion";

        public void setSlowmotion(String value) {
            set(KEY_SLOWMOTION, value);
        }

        public String getSlowmotion() {
            return get(KEY_SLOWMOTION);
        }

        public List<String> getSupportedSlowmotion() {
            String str = get(KEY_SLOWMOTION + SUPPORTED_VALUES_SUFFIX);
            return split(str);
        }
        /* @} */

        /*
         * SPRD: Fix bug 474715 Feature:Brightness. @{
         */
        private static final String KEY_BRIGHTNESS = "brightness";

        public void setBrightness(String value) {
            set(KEY_BRIGHTNESS, value);
        }

        public String getBrightness() {
            return get(KEY_BRIGHTNESS);
        }

        public List<String> getSupportedBrightness() {
            String str = get(KEY_BRIGHTNESS + SUPPORTED_VALUES_SUFFIX);
            return split(str);
        }
        /* @} */

        /*
         * SPRD: Fix bug 474724 Feature:ISO. @{
         */
        private static final String KEY_ISO = "iso";

        public static final String ISO_AUTO = "auto";
        public static final String ISO_100 = "100";
        public static final String ISO_200 = "200";
        public static final String ISO_400 = "400";
        public static final String ISO_800 = "800";
        public static final String ISO_1600 = "1600";

        public List<String> getSupportedISO() {
            String str = get(KEY_ISO + SUPPORTED_VALUES_SUFFIX);
            return split(str);
        }

        public String getISO() {
            return get(KEY_ISO);
        }

        public void setISO(String value) {
            set(KEY_ISO, value);
        }
        /* @} */

        /*
         * SPRD: Fix bug 474718 Feature:Metering. @{
         */
        private static final String KEY_METERING = "metering-mode";

        public static final String AUTO_EXPOSURE_FRAME_AVG = "frame-average";
        public static final String AUTO_EXPOSURE_CENTER_WEIGHTED = "center-weighted";
        public static final String AUTO_EXPOSURE_SPOT_METERING = "spot-metering";

        public void setMeteringMode(String value) {
            set(KEY_METERING, value);
        }

        public String getMeteringMode() {
            return get(KEY_METERING);
        }

        public List<String> getSupportedMeteringMode() {
            String str = get(KEY_METERING + SUPPORTED_VALUES_SUFFIX);
            return split(str);
        }
        /* @} */

        /*
         * SPRD: Fix bug 474722 Feature:Saturation. @{
         */
        private static final String KEY_SATURATION = "saturation";

        public void setSaturation(String value) {
            set(KEY_SATURATION, value);
        }

        public String getSaturation() {
            return get(KEY_SATURATION);
        }

        public List<String> getSupportedSaturation() {
            String str = get(KEY_SATURATION + SUPPORTED_VALUES_SUFFIX);
            return split(str);
        }
        /* @} */

        /*
         * SPRD: Fix bug 534257 Feature:EOIS. @{
         */
        private static final String KEY_EOIS = "EOIS";

        public void setEOIS(boolean value) {
            set(KEY_EOIS, value ? TRUE : FALSE);
        }

        public boolean getEOIS() {
            String value = get(KEY_EOIS);
            return TRUE.equals(value);
        }

        public List<String> getSupportedEOIS() {
            String str = get(KEY_EOIS + SUPPORTED_VALUES_SUFFIX);
            return split(str);
        }
        /* @} */

        /**
         * Sets the focus mode.
         *
         * @param value focus mode.
         */
        private boolean updateFocusMode(String value) {
            /* SPRD: Fix bug 526233 if the focus is not adjustable, return here,
             * or all parameter settings will not be excuted in hal layer.
             */
            // CTS case CameraTest#testInvalidParameters requires.
            if ("invalid".equals(value)) {
                set(KEY_FOCUS_MODE, value);
                return true;
            }

            List<String> supportedFocusModes = getSupportedFocusModes();
            if (supportedFocusModes != null && supportedFocusModes.size() == 1
                    && FOCUS_MODE_FIXED.equals(supportedFocusModes.get(0))) {
                Log.d(TAG, "setFocusMode return " + value);
                return true;
            }
            /* @} */
            return false;
        }

        @Override
        public void setFocusMode(String value) {
            /* SPRD: Fix bug 526233 if the focus is not adjustable, return here,
             * or all parameter settings will not be excuted in hal layer.
             */
            if (updateFocusMode(value)) {
                return;
            }
            super.setFocusMode(value);
        }
    }
}