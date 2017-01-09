/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.utils;

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
    public static final String FULL_SIZE_FRAME_PATH = "/data/data/com.ucamera.ucam/frame/";
    public static final String EXTRA_FRAME_ORIGINAL_VALUE = EXTRA_FRAME_VALUE + "/frame";
    public static final String EXTRA_FRAME_THUMB_VALUE = EXTRA_FRAME_VALUE + "/frame_hdpi";
}
