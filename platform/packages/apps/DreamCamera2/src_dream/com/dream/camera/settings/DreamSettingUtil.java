package com.dream.camera.settings;

import com.android.camera.debug.Log;
import com.android.camera.settings.Keys;
import com.android.camera2.R;
import com.dream.camera.settings.DataConfig;
import com.dream.camera.settings.DataModuleBasic.DataStorageStruct;

import android.R.integer;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.res.TypedArray;
import android.view.View;

public class DreamSettingUtil {

    private static final Log.Tag TAG = new Log.Tag("DreamSetting");

    /**
     * open preferences associated with file path
     * 
     * @param context
     * @param settingPath
     * @return
     */
    public static SharedPreferences openPreferences(Context context,
            String settingPath) {
        synchronized (DreamSettingUtil.class) {
            SharedPreferences preferences;
            preferences = context.getSharedPreferences(settingPath,
                    Context.MODE_PRIVATE);
            return preferences;
        }
    }

    public static int getSupportDataResourceID(DataStructSetting dataSetting) {
        int resourceID = -1;
        // camera configuration resource
        if (dataSetting.mCategory
                .equals(DataConfig.CategoryType.CATEGORY_CAMERA)) {
            resourceID = R.array.camera_public_setting;
            return resourceID;
        }

        // photo configuration resource
        if (dataSetting.mCategory
                .equals(DataConfig.CategoryType.CATEGORY_PHOTO)) {
            if (dataSetting.mIsFront) {
                switch (dataSetting.mMode) {
                    case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_INTERVAL_PICTURE:
                        resourceID = R.array.photo_front_interval_pic_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_AUTO:
                        resourceID = R.array.photo_front_auto_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_UCAM_GIF:
                        resourceID = R.array.photo_front_gif_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_UCAM_SCENERY:
                        resourceID = R.array.photo_front_mode_ucam_scenery_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_UCAM_FILTER:
                        resourceID = R.array.photo_front_mode_ucam_filter_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_INTENT_CAPTURE:
                        resourceID = R.array.photo_front_mode_intent_capture_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_SOUND_PICTURE:
                        resourceID = R.array.photo_front_mode_sound_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_VGESTURE:
                        resourceID = R.array.photo_front_vgesture_setting;
                        break;
                    default:
                        break;
                }
            } else {
                switch (dataSetting.mMode) {
                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_AUTO:
                        resourceID = R.array.photo_back_auto_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_MANUAL:
                        resourceID = R.array.photo_back_manual_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_CONTINUE_PICTURE:
                        resourceID = R.array.photo_back_continue_pic_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_PANORAMA:
                        resourceID = R.array.photo_back_panorama_setting;
                        break;

                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_SCENE:
                        resourceID = R.array.photo_back_scene_setting;
                        break;

                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_UCAM_GIF:
                        resourceID = R.array.photo_back_gif_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_UCAM_SCENERY:
                        resourceID = R.array.photo_back_mode_ucam_scenery_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_UCAM_FILTER:
                        resourceID = R.array.photo_back_mode_ucam_filter_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_INTENT_CAPTURE:
                        resourceID = R.array.photo_back_mode_intent_capture_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_QRCODE:
                        resourceID = R.array.photo_back_mode_qrcode_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_SOUND_PICTURE:
                        resourceID = R.array.photo_back_mode_sound_setting;
                        break;
                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_PIP_VIV:
                    case DataConfig.PhotoModeType.PHOTO_MODE_BACK_REFOCUS:
                        break;
                    default:
                        break;
                }
            }
            return resourceID;
        }

        // video configuration resource
        if (dataSetting.mCategory
                .equals(DataConfig.CategoryType.CATEGORY_VIDEO)) {
            if (dataSetting.mIsFront) {
                switch (dataSetting.mMode) {
                case DataConfig.VideoModeType.VIDEO_MODE_FRONT_AUTO:
                    resourceID = R.array.video_front_mode_auto_setting;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_FRONT_GIF:
                    resourceID = R.array.video_front_mode_gif_setting;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_FRONT_PIV_VIV:
                    break;
                default:
                    break;
                }
            } else {
                switch (dataSetting.mMode) {
                case DataConfig.VideoModeType.VIDEO_MODE_BACK_AUTO:
                    resourceID = R.array.video_back_mode_auto_setting;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_BACK_TIME_LAPSE:
                    resourceID = R.array.video_back_mode_time_lapse_setting;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_BACK_SLOW_MOTION:
                    resourceID = R.array.video_back_mode_slowmotion_setting;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_BACK_GIF:
                    resourceID = R.array.video_back_mode_gif_setting;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_BACK_PIP_VIV:
                    break;
                default:
                    break;
                }
            }
            return resourceID;
        }

        return resourceID;
    }

    public static int getPreferenceUIConfigureID(DataStructSetting dataSetting) {
        int resourceID = -1;
        // camera configuration resource
        if (dataSetting.mCategory
                .equals(DataConfig.CategoryType.CATEGORY_CAMERA)) {
            resourceID = R.array.camera_public_setting_display;
            return resourceID;
        }
        // photo configuration resource
        if (dataSetting.mCategory
                .equals(DataConfig.CategoryType.CATEGORY_PHOTO)) {
            if (dataSetting.mIsFront) {
                switch (dataSetting.mMode) {
                case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_AUTO:
                    resourceID = R.array.photo_front_mode_auto_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_INTERVAL_PICTURE:
                    resourceID = R.array.photo_front_mode_interval_picture_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_SOUND_PICTURE:
                    resourceID = R.array.photo_front_mode_sound_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_PIP_VIV:
                    resourceID = R.array.photo_front_mode_pip_viv_picture_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_UCAM_GIF:
                    resourceID = R.array.photo_back_mode_ucam_gif_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_UCAM_SCENERY:
                    resourceID = R.array.photo_front_mode_ucam_scenery_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_UCAM_FILTER:
                    resourceID = R.array.photo_front_mode_ucam_filter_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_INTENT_CAPTURE:
                    resourceID = R.array.photo_front_mode_intent_capture_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_FRONT_VGESTURE:
                    resourceID = R.array.photo_front_mode_vgesture_setting_display;
                    break;
                default:
                    break;
                }
            } else {
                switch (dataSetting.mMode) {
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_AUTO:
                    resourceID = R.array.photo_back_mode_auto_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_MANUAL:
                    resourceID = R.array.photo_back_mode_manual_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_PANORAMA:
                    resourceID = R.array.photo_back_mode_panorama_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_REFOCUS:
                    resourceID = R.array.photo_back_mode_refocus_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_CONTINUE_PICTURE:
                    resourceID = R.array.photo_back_mode_continue_pic_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_SCENE:
                    resourceID = R.array.photo_back_mode_scene_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_PIP_VIV:
                    resourceID = R.array.photo_back_mode_pip_viv_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_SOUND_PICTURE:
                    resourceID = R.array.photo_back_mode_sound_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_UCAM_SECNE:
                    resourceID = R.array.photo_back_mode_ucam_scene_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_UCAM_GIF:
                    resourceID = R.array.photo_back_mode_ucam_gif_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_UCAM_SCENERY:
                    resourceID = R.array.photo_back_mode_ucam_scenery_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_UCAM_FILTER:
                    resourceID = R.array.photo_back_mode_ucam_filter_setting_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_INTENT_CAPTURE:
                    resourceID = R.array.photo_back_mode_intent_capture_display;
                    break;
                case DataConfig.PhotoModeType.PHOTO_MODE_BACK_QRCODE:
                    resourceID = R.array.photo_back_mode_qrcode_display;
                    break;

                default:
                    break;
                }
            }
            return resourceID;
        }

        // video configuration resource
        if (dataSetting.mCategory
                .equals(DataConfig.CategoryType.CATEGORY_VIDEO)) {
            if (dataSetting.mIsFront) {
                switch (dataSetting.mMode) {
                case DataConfig.VideoModeType.VIDEO_MODE_FRONT_AUTO:
                    resourceID = R.array.video_front_mode_auto_setting_display;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_FRONT_PIV_VIV:
                    resourceID = R.array.video_front_mode_pip_viv_setting_display;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_FRONT_GIF:
                    resourceID = R.array.video_front_mode_gif_setting_display;
                    break;

                default:
                    break;
                }
            } else {
                switch (dataSetting.mMode) {
                case DataConfig.VideoModeType.VIDEO_MODE_BACK_AUTO:
                    resourceID = R.array.video_back_mode_auto_setting_display;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_BACK_TIME_LAPSE:
                    resourceID = R.array.video_back_mode_time_lapse_setting_display;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_BACK_SLOW_MOTION:
                    resourceID = R.array.video_back_mode_slow_motion_setting_display;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_BACK_PIP_VIV:
                    resourceID = R.array.video_back_mode_piv_viv_setting_display;
                    break;
                case DataConfig.VideoModeType.VIDEO_MODE_BACK_GIF:
                    resourceID = R.array.video_back_mode_gif_setting_display;
                    break;
                default:
                    break;
                }
            }
            return resourceID;
        }

        return resourceID;
    }

    /*
     * module int to settings string.
     */
    public static String changeChildModeToString(String mCategory,
            boolean mIsFront, int mMode) {
        Log.d(TAG, "changeChildModeToString = " + mCategory + "," + mIsFront
                + "," + mMode);
        // photo configuration resource
        if (mCategory.equals(DataConfig.CategoryType.CATEGORY_PHOTO)) {

            if (mIsFront) {
                switch (mMode) {
                case 0:
                    return DataConfig.PhotoModeType.PHOTO_MODE_FRONT_AUTO;
                case 3:
                    return DataConfig.PhotoModeType.PHOTO_MODE_FRONT_INTERVAL_PICTURE;
                case 9:
                    return DataConfig.PhotoModeType.PHOTO_MODE_FRONT_UCAM_GIF;
                case 10:
                    return DataConfig.PhotoModeType.PHOTO_MODE_FRONT_UCAM_SCENERY;
                case 16:
                    return DataConfig.PhotoModeType.PHOTO_MODE_FRONT_SOUND_PICTURE;
                case 17:
                    return DataConfig.PhotoModeType.PHOTO_MODE_FRONT_VGESTURE;
                case 19:
                    return DataConfig.PhotoModeType.PHOTO_MODE_FRONT_UCAM_FILTER;
                case 20:
                    return DataConfig.PhotoModeType.PHOTO_MODE_FRONT_INTENT_CAPTURE;

                default:
                    return DataConfig.PhotoModeType.PHOTO_MODE_FRONT_AUTO;
                    // TODO
                    // DataConfig.PhotoModeType.PHOTO_MODE_FRONT_SOUND_PICTURE;
                }

            } else {
                switch (mMode) {
                case 0:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_AUTO;
                case 1:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_MANUAL;
                case 2:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_CONTINUE_PICTURE;
                case 4:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_PANORAMA;
                case 5:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_REFOCUS;
                case 6:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_SCENE;
                case 7:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_PIP_VIV;
                case 9:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_UCAM_GIF;
                case 10:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_UCAM_SCENERY;
                case 16:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_SOUND_PICTURE;
                case 18:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_QRCODE;
                case 19:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_UCAM_FILTER;
                case 20:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_INTENT_CAPTURE;
                default:
                    return DataConfig.PhotoModeType.PHOTO_MODE_BACK_AUTO;
                }
            }
        }

        // video configuration resource
        if (mCategory.equals(DataConfig.CategoryType.CATEGORY_VIDEO)) {
            if (mIsFront) {
                switch (mMode) {
                case 11:
                    return DataConfig.VideoModeType.VIDEO_MODE_FRONT_AUTO;
                case 12:
                    return DataConfig.VideoModeType.VIDEO_MODE_FRONT_PIV_VIV;
                case 15:
                    return DataConfig.VideoModeType.VIDEO_MODE_FRONT_GIF;
                default:
                    return DataConfig.VideoModeType.VIDEO_MODE_FRONT_AUTO;
                }
            } else {
                switch (mMode) {
                case 11:
                    return DataConfig.VideoModeType.VIDEO_MODE_BACK_AUTO;
                case 12:
                    return DataConfig.VideoModeType.VIDEO_MODE_BACK_PIP_VIV;
                case 13:
                    return DataConfig.VideoModeType.VIDEO_MODE_BACK_TIME_LAPSE;
                case 14:
                    return DataConfig.VideoModeType.VIDEO_MODE_BACK_SLOW_MOTION;
                case 15:
                    return DataConfig.VideoModeType.VIDEO_MODE_BACK_GIF;
                default:
                    return DataConfig.VideoModeType.VIDEO_MODE_BACK_AUTO;
                }
            }
        }
        return DataConfig.PhotoModeType.PHOTO_MODE_BACK_AUTO;
    }

    public static int getMutexDataResourceID(DataStructSetting dataSetting) {

        int resourceID = -1;
        // camera configuration resource
        if (dataSetting.mCategory
                .equals(DataConfig.CategoryType.CATEGORY_CAMERA)) {
//            resourceID = R.array.camera_other_data_setting;
        }
        // photo configuration resource
        if (dataSetting.mCategory
                .equals(DataConfig.CategoryType.CATEGORY_PHOTO)) {
            if (dataSetting.mIsFront) {
                resourceID = R.array.photo_front_other_mutex_setting;
            }else {
                resourceID = R.array.photo_back_other_mutex_setting;
            }
        }

        // video configuration resource
        if (dataSetting.mCategory
                .equals(DataConfig.CategoryType.CATEGORY_VIDEO)) {
            if (dataSetting.mIsFront) {
                resourceID = R.array.video_front_other_mutex_setting;
            }else {
                resourceID = R.array.video_back_other_mutex_setting;
            }
        }
        return resourceID;

    }

    public static void showDialog(Context context, String key, int titleID,
            int drawableIDs, final DialogInterface.OnClickListener listener) {
        // title
        String title = context.getResources().getString(titleID);
        // drable list
        TypedArray types = context.getResources().obtainTypedArray(drawableIDs);
        int[] mDrawableIDs = new int[types.length()];
        if (types != null) {
            for (int i = 0; i < types.length(); i++) {
                mDrawableIDs[i] = types.getResourceId(i, -1);

            }
        }

        // key
        DataStorageStruct ds = (DataStorageStruct) DataModuleManager
                .getInstance(context).getCurrentDataModule().mSupportDataMap.get(key);

        if (ds == null) {
            return;
        }

        int selectedIndex = DataModuleManager.getInstance(context)
                .getCurrentDataModule().getIndexOfCurrentValue(key);

        AlertDialog.Builder builder = new AlertDialog.Builder(context,
                R.style.ThemeDeviceDefaultDialogAlert);
        DreamUIAdapterMeter adapter = new DreamUIAdapterMeter();
        adapter.setData(ds.mEntries, mDrawableIDs, selectedIndex);

        builder.setTitle(title)
                .setSingleChoiceItems(adapter, selectedIndex,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                                if (listener != null) {
                                    listener.onClick(dialog, which);
                                }
                            }
                        })
                .setPositiveButton(null, null)
                .setCancelable(true)
                .setNegativeButton(
                        context.getResources().getString(
                                R.string.cancel_button_description),
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                dialog.dismiss();
                            }
                        }).setPositiveButton(null, null);

        // builder.setTitle(title)
        // .setSingleChoiceItems(ds.mEntries, selectedIndex,
        // new DialogInterface.OnClickListener() {
        // public void onClick(DialogInterface dialog,
        // int which) {
        // dialog.dismiss();
        // if (listener != null) {
        // listener.onClick(dialog, which);
        // }
        // }
        // })
        // .setPositiveButton(null, null)
        // .setCancelable(true)
        // .setNegativeButton(
        // context.getResources().getString(
        // R.string.cancel_button_description),
        // new DialogInterface.OnClickListener() {
        // public void onClick(DialogInterface dialog,
        // int which) {
        // dialog.dismiss();
        // }
        // }).setPositiveButton(null, null);
        AlertDialog dialog = builder.create();
        dialog.setCanceledOnTouchOutside(true);

        dialog.show();
    }

    /**
     * Package private conversion method to turn ints into preferred String
     * storage format.
     * 
     * @param value
     *            int to be stored in Settings
     * @return String which represents the int
     */
    static String convert(int value) {
        return Integer.toString(value);
    }

    /**
     * Package private conversion method to turn booleans into preferred String
     * storage format.
     * 
     * @param value
     *            boolean to be stored in Settings
     * @return String which represents the boolean
     */
    static String convert(boolean value) {
        return value ? "1" : "0";
    }

    /**
     * Package private conversion method to turn String storage format into
     * ints.
     * 
     * @param value
     *            String to be converted to int
     * @return int value of stored String
     */
    static public int convertToInt(String value) {
        return Integer.parseInt(value);
    }

    /**
     * Package private conversion method to turn String storage format into
     * booleans.
     * 
     * @param value
     *            String to be converted to boolean
     * @return boolean value of stored String
     */
    static boolean convertToBoolean(String value) {
        return Integer.parseInt(value) != 0;
    }

}
