/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.downloadcenter;

import android.os.Environment;

public class Constants {
    public static final String THUMBNAILS_DIR = Environment.getExternalStorageDirectory().toString() + "/UCam/.thumbnails/";

    public static final String ACTION_DOWNLOAD_TYPE = "download_type";
    public static final String EXTRA_FRAME_VALUE = "frame";
    public static final String EXTRA_DECOR_VALUE = "decor";
    public static final String EXTRA_PHOTO_FRAME_VALUE = "photoframe";
    public static final String EXTRA_TEXTURE_VALUE = "texture";
    public static final String EXTRA_MOSAIC_VALUE = "mosaic";
    public static final String EXTRA_FONT_VALUE = "font";
    public static final String EXTRA_PUZZLE_VALUE = "puzzle";
    public static final String EXTRA_MANGA_FRAME_VALUE = "mangaframe";

    public static final String ACTION_SCREEN_DENSITY = "screen_density";
    public static final String EXTRA_DENSITY_HDPI = "hdpi";
    public static final String EXTRA_DENSITY_MDPI = "mdpi";

    public static final String DOWNLOAD_RES = "http://www.u-camera.com/api/resources.php";
    public static final String WEIBO_ACTIVITIES_RES = "http://www.u-camera.com/api/activities.php";

    public static final String DOWNLOAD_APP = "http://www.u-camera.com/api/hot-apps.php";
    public static final String DOWNLOAD_EVENT = "http://www.u-camera.com/api/activities.php";
    public static final String KEY_LOCAL_VERSION = "key_current_download_center_version";
    public static final int CURRENT_LOCAL_VERSION =  4;
}
