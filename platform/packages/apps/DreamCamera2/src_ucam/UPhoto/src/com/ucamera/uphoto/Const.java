/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.os.Environment;

public class Const {

    public final static String mDecorDirectory = "decor";
    public final static String mPhotoFrameDirectory = "photoframe";
    public final static String mTextureDirectory = "texture";
    public final static String mMosaicDirectory = "mosaic";

    public static final String DCIM = Environment.getExternalStoragePublicDirectory(
            Environment.DIRECTORY_DCIM).toString();

    public static final String DIRECTORY = DCIM + "/UCam";

    public static final String DOWNLOAD_DIRECTORY = DCIM + "/UCam/download/";

    public final static String RESOURCE_DIRECTORY = DCIM + "/.UCam";

    public final static String PHOTO_FRAME_DIRECTORY = RESOURCE_DIRECTORY + "/"
            + mPhotoFrameDirectory;
    public final static String TEXTURE_DIRECTORY = RESOURCE_DIRECTORY + "/" + mTextureDirectory;

    public static final String NOTFOUNDERROR = "NOTFOUNDERROR";

    public static final String EXPOSURE_DEFAULT_VALUE = "0";

    public static final String KEY_UCAM_SELECT_PATH       = "sf_pref_ucam_select_path_key";

    // download center
    public static final String ACTION_DOWNLOAD_TYPE = "download_type";
    public static final String EXTRA_FRAME_VALUE = "frame";
    public static final String EXTRA_DECOR_VALUE = "decor";
    public static final String EXTRA_PHOTO_FRAME_VALUE = "photoframe";
    public static final String EXTRA_TEXTURE_VALUE = "texture";
    public static final String EXTRA_MOSAIC_VALUE = "mosaic";
    public static final String EXTRA_FONT_VALUE = "font";
    public static final String EXTRA_PUZZLE_VALUE = "puzzle";

    public static final String ACTION_SCREEN_DENSITY = "screen_density";
    public static final String EXTRA_DENSITY_HDPI = "hdpi";
    public static final String EXTRA_DENSITY_MDPI = "mdpi";

    public static final String EXTRA_OUT_CALL = "outCall";

    public static final int[][] EFFECT_CATEGORYS = {
            {
                    0, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150
            },// LOMO
            {
                    1, 16, 210, 211, 212, 213, 214
            },// HDR
            {
                    200, 201, 202, 203, 204, 205
            },// skin pretify
            {
                    180, 181, 182, 183, 184, 185, 186, 187
            },// Japanese series
            {
                    62, 61, 60, 7, 30, 31, 32, 33, 34, 35, 36, 37
            },// Sketch
            {
                    5, 6, 22, 23, 24, 25, 104, 105, 106, 107, 120, 121, 122, 123, 124, 125, 126,
                    127, 128, 129, 130
            },// Color
            {
                    3, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21
            },// Fun
            {
                    50, 51, 52, 53, 54, 55, 56, 57
            },// Retro
            {
                    4, 2, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170
            },// Black and White
            {
                    19, 40, 41, 42, 43, 44, 100, 101, 102, 103
            }
    // Deformation
    };
}
