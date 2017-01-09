package com.dream.camera.util;

import android.content.Context;
import android.content.res.TypedArray;

import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera2.R;
import com.dream.camera.settings.DataConfig;
import com.dream.camera.settings.DataModuleBasic;

public class DreamUtil {
    public static final int BACK_CAMERA_PHOTO_MODE = 0;
    public static final int FRONT_CAMERA_PHOTO_MODE = 1;
    public static final int BACK_CAMERA_VIDEO_MODE = 2;
    public static final int FRONT_CAMERA_VIDEO_MODE = 3;
    public static final int PHOTO_MODE = 0;
    public static final int VIDEO_MODE = 1;
    public static final int BACK_CAMERA = 0;
    public static final int FRONT_CAMERA = 1;
    // side panel icon mask
    public static final int SP_EMPTY = 0;
    public static final int SP_EXTERNAL_STORAGE = 1 << 0;
    public static final int SP_INTERNAL_STORAGE = 1 << 1;
    // usb storage side panel icon mask
    public static final int SP_USB_STORAGE = 1 << 2;
    public static final int SP_FACE_DETECT = 1 << 3;
    public static final int SP_LOCATE = 1 << 4;
    public static final int SP_TOUCH_CAPTURE = 1 << 5;
    public static final int SP_FILTER = 1 << 6;
    public static final int SP_OPPOSITE = 1 << 7;
    public static final int SP_CA = 1 << 8;
    public static final int SP_VA = 1 << 9;
    private static final Log.Tag TAG = new Log.Tag("DreamUtil ");

    public static int getSwitchBtnResId(int mode) {
        switch (mode % 2) {
            case PHOTO_MODE:
                return R.drawable.ic_switch_to_camera_sprd;
            case VIDEO_MODE:
                return R.drawable.ic_switch_to_video_sprd;
            default:
                return R.drawable.ic_switch_to_video_sprd;
        }
    }

    /*
     * settings cameraId to boolean back/front.
     */
    public static Boolean isFrontCamera(Context context, int cameraId) {
        return DreamUtil.intToBoolean(DreamUtil.getRightCamera(cameraId));
    }

    /*
     * mode int to settings string.
     */
    public static String intToString(int mode) {
        switch (mode) {
            case PHOTO_MODE:
                return DataConfig.CategoryType.CATEGORY_PHOTO;
            case VIDEO_MODE:
                return DataConfig.CategoryType.CATEGORY_VIDEO;
            default:
                return DataConfig.CategoryType.CATEGORY_PHOTO;
        }

    }

    /*
     * back/front int to settings boolean.
     */
    public static boolean intToBoolean(int backOrFront) {
        switch (backOrFront) {
            case BACK_CAMERA:
                return false;
            case FRONT_CAMERA:
                return true;
            default:
                return false;
        }
    }

    public static int getRightCamera(int cameraId) {
        int result = BACK_CAMERA;
        switch (cameraId) {
            case 0:
                result = BACK_CAMERA;
                break;
            case 1:
                result = FRONT_CAMERA;
                break;
        }
        return result;
    }

    public static int getRightCameraId(int camera) {
        int result = 0;
        switch (camera) {
            case BACK_CAMERA:
                result = 0;
                break;
            case FRONT_CAMERA:
                result = 1;
                break;
        }
        return result;
    }

    /**
     * dream camera Gets the mode icon resource id of a specific mode.
     *
     * @param modeIndex index of the mode
     * @param context   current context
     * @return icon resource id if the index is valid, otherwise 0
     */
    public static int getCameraUnselectedModeIconResId(int modeIndex, Context context) {
        // Find the camera mode icon using id
        TypedArray cameraModesIcons = context.getResources()
                .obtainTypedArray(R.array.camera_mode_selected_icon);
        if (modeIndex >= cameraModesIcons.length() || modeIndex < 0) {
            // Mode index not found
            Log.e(TAG, "Invalid mode index: " + modeIndex);
            return 0;
        }
        return cameraModesIcons.getResourceId(modeIndex, 0);
    }

    public String getGlobleKey(int module, int cameraId) {
        String key = Keys.KEY_BACK_PHOTO_MODE;
        int backOrFront = getRightCamera(cameraId);
        switch (module) {
            case PHOTO_MODE:
                switch (cameraId) {
                    case BACK_CAMERA:
                        key = Keys.KEY_BACK_PHOTO_MODE;
                        break;
                    case FRONT_CAMERA:
                        key = Keys.KEY_FRONT_PHOTO_MODE;
                        break;
                }
                break;
            case VIDEO_MODE:
                switch (cameraId) {
                    case BACK_CAMERA:
                        key = Keys.KEY_BACK_VIDEO_MODE;
                        break;
                    case FRONT_CAMERA:
                        key = Keys.KEY_FRONT_VIDEO_MODE;
                        break;
                }
                break;
        }
        return key;
    }

    public int getDefaultValue(int module, int cameraId) {
        int value = 0;
        ;
        int backOrFront = getRightCamera(cameraId);
        switch (module) {
            case PHOTO_MODE:
                switch (cameraId) {
                    case BACK_CAMERA:
                        value = 0;
                        break;
                    case FRONT_CAMERA:
                        value = 0;
                        break;
                }
                break;
            case VIDEO_MODE:
                switch (cameraId) {
                    case BACK_CAMERA:
                        value = 11;
                        break;
                    case FRONT_CAMERA:
                        value = 11;
                        break;
                }
                break;
        }
        return value;
    }

    public int getRightMode(SettingsManager settingsManager, int module, int cameraId) {
        Log.d(TAG,
                "getRightMode cameraId=" + cameraId + ",module=" + module + ","
                        + getGlobleKey(module, cameraId) + "," + getDefaultValue(module, cameraId));
        return settingsManager.getInteger(SettingsManager.SCOPE_GLOBAL,
                getGlobleKey(module, cameraId), getDefaultValue(module, cameraId));
    }

    public int getRightMode(DataModuleBasic dataModule, int module, int cameraId) {
        Log.d(TAG,
                "getRightMode cameraId=" + cameraId + ",module=" + module);
        return dataModule.getInt(
                getGlobleKey(module, cameraId), getDefaultValue(module, cameraId));
    }

    /**
     * Every time mode selected, we should save to the corresponding camera mode
     * KEY_BACK_PHOTO_MODE/KEY_FRONT_PHOTO_MODE/KEY_BACK_VIDEO_MODE/KEY_FRONT_VIDEO_MODE
     *
     * @param context
     * @param settingsManager
     * @param cameraId        : 0/1 - back/front
     * @param saveMode        : current mode.
     */
    public void savaToCameraMode(Context context, SettingsManager settingsManager, int cameraId,
                                 int saveMode) {
        Log.d(TAG, "savaRightMode cameraId=" + cameraId + ",saveMode=" + saveMode);

        // Now in photo or video
        int[] modeSupportList = context.getResources().getIntArray(
                R.array.dream_module_mode_support_list);

        // save value to responding keys.
        settingsManager.set(SettingsManager.SCOPE_GLOBAL,
                getGlobleKey(modeSupportList[saveMode], cameraId), saveMode);
    }

    public void savaToCameraMode(Context context, DataModuleBasic dataModule, int cameraId,
                                 int saveMode) {
        Log.d(TAG, "savaRightMode cameraId=" + cameraId + ",saveMode=" + saveMode);

        // Now in photo or video
        int[] modeSupportList = context.getResources().getIntArray(
                R.array.dream_module_mode_support_list);

        // save value to responding keys.
        dataModule.set(getGlobleKey(modeSupportList[saveMode], cameraId), saveMode);
    }
}
