package com.dream.camera.settings;

import java.util.HashSet;

import com.android.camera.settings.SettingsScopeNamespaces;

public interface DataConfig {
    /**
     * constant variable that used to generate the preference file name
     */
    public interface DataStoragePath {
        // public storage for all camera category
        public static final String PRE_CAMERA_CATEGORY_PUBLIC_SETTING = "camera_camera_setting";

        // public storage for category photo
        public static final String PRE_CAMERA_CATEGORY_PHOTO_SETTING = "camera_category_photo_setting";

        // public storage for category video
        public static final String PRE_CAMERA_CATEGORY_VIDEO_SETTING = "camera_category_video_setting";

        // public storage for category photo front camera
        public static final String PRE_CAMERA_CATEGORY_PHOTO_FRONT_SETTING = "camera_category_photo_front_setting";

        // public storage for category photo back camera
        public static final String PRE_CAMERA_CATEGORY_PHOTO_BACK_SETTING = "camera_category_photo_back_setting";

        // public storage for category video front camera
        public static final String PRE_CAMERA_CATEGORY_VIDEO_FRONT_SETTING = "camera_category_video_front_setting";

        // public storage for category video back camera
        public static final String PRE_CAMERA_CATEGORY_VIDEO_BACK_SETTING = "camera_category_video_back_setting";

    }

    /**
     * classify five range of storage position the int value will be used in xml
     * configuration
     */
    public interface SettingStoragePosition {
        public static final int POSITION_ERROR = 0x10000000;
        public static final int POSITION_CAMERA_PUBLIC = 0x00000010;
        public static final int POSITION_CATEGORY = 0x00000001;
        public static final int POSITION_CATEGORY_BF = 0x00000002;
        public static final int POSITION_CATEGORY_BF_MODULE = 0x00000004;
        public static final int POSITION_CATEGORY_BF_MODULE_ID = 0x00000008;

        public static final int[] positionList = { POSITION_ERROR,
                POSITION_CAMERA_PUBLIC, POSITION_CATEGORY,
                POSITION_CATEGORY_BF, POSITION_CATEGORY_BF_MODULE,
                POSITION_CATEGORY_BF_MODULE_ID };
    }

    /**
     * define the category of setting
     */
    public interface CategoryType {
        public static final String CATEGORY_VIDEO = "category_video";
        public static final String CATEGORY_PHOTO = "category_photo";
        public static final String CATEGORY_CAMERA = "category_camera";
    }

    /**
     * child mode of photo
     */
    public interface PhotoModeType {
        public static final String PHOTO_MODE_BACK_AUTO = SettingsScopeNamespaces.AUTO_PHOTO;
        public static final String PHOTO_MODE_BACK_MANUAL = SettingsScopeNamespaces.MANUAL;
        public static final String PHOTO_MODE_BACK_PANORAMA = SettingsScopeNamespaces.DREAM_PANORAMA;
        public static final String PHOTO_MODE_BACK_REFOCUS = SettingsScopeNamespaces.REFOCUS;
        public static final String PHOTO_MODE_BACK_CONTINUE_PICTURE = SettingsScopeNamespaces.CONTINUE;
        public static final String PHOTO_MODE_BACK_SCENE = SettingsScopeNamespaces.SCENE;
        public static final String PHOTO_MODE_BACK_PIP_VIV = SettingsScopeNamespaces.PIP;
        public static final String PHOTO_MODE_BACK_SOUND_PICTURE = SettingsScopeNamespaces.AUDIO_PICTURE;
        public static final String PHOTO_MODE_BACK_UCAM_SECNE = SettingsScopeNamespaces.GCAM;
        public static final String PHOTO_MODE_BACK_UCAM_GIF = SettingsScopeNamespaces.GIF_PHOTO;
        public static final String PHOTO_MODE_BACK_UCAM_SCENERY = SettingsScopeNamespaces.SCENERY;
        public static final String PHOTO_MODE_BACK_UCAM_FILTER = SettingsScopeNamespaces.FILTER;
        public static final String PHOTO_MODE_BACK_INTENT_CAPTURE = SettingsScopeNamespaces.INTENTCAPTURE;
        public static final String PHOTO_MODE_BACK_QRCODE = SettingsScopeNamespaces.QR_CODE;
        public static final String PHOTO_MODE_FRONT_AUTO = SettingsScopeNamespaces.AUTO_PHOTO;
        public static final String PHOTO_MODE_FRONT_INTERVAL_PICTURE = SettingsScopeNamespaces.INTERVAL;
        public static final String PHOTO_MODE_FRONT_PIP_VIV = SettingsScopeNamespaces.PIP;
        public static final String PHOTO_MODE_FRONT_SOUND_PICTURE = SettingsScopeNamespaces.AUDIO_PICTURE;
        public static final String PHOTO_MODE_FRONT_UCAM_GIF = SettingsScopeNamespaces.GIF_PHOTO;
        public static final String PHOTO_MODE_FRONT_UCAM_SCENERY = SettingsScopeNamespaces.SCENERY;
        public static final String PHOTO_MODE_FRONT_UCAM_FILTER = SettingsScopeNamespaces.FILTER;
        public static final String PHOTO_MODE_FRONT_INTENT_CAPTURE = SettingsScopeNamespaces.INTENTCAPTURE;
        public static final String PHOTO_MODE_FRONT_VGESTURE = SettingsScopeNamespaces.VGESTURE;
    }

    /**
     * child mode of video
     */
    public interface VideoModeType {
        public static final String VIDEO_MODE_BACK_AUTO = SettingsScopeNamespaces.AUTO_VIDEO;
        public static final String VIDEO_MODE_BACK_TIME_LAPSE = SettingsScopeNamespaces.TIMELAPSE;
        public static final String VIDEO_MODE_BACK_SLOW_MOTION = SettingsScopeNamespaces.SLOWMOTION;
        public static final String VIDEO_MODE_BACK_PIP_VIV = SettingsScopeNamespaces.VIV;
        public static final String VIDEO_MODE_BACK_GIF = SettingsScopeNamespaces.GIF_VIDEO;
        public static final String VIDEO_MODE_FRONT_AUTO = SettingsScopeNamespaces.AUTO_VIDEO;
        public static final String VIDEO_MODE_FRONT_PIV_VIV = SettingsScopeNamespaces.VIV;
        public static final String VIDEO_MODE_FRONT_GIF = SettingsScopeNamespaces.GIF_VIDEO;
    }

    /**
     * add the features which user want to force removing whatever if the device
     * supported. developter can user this to debug
     */
    public static HashSet<String> FEATURE_SWITCHER = new HashSet<String>() {

    };

}
