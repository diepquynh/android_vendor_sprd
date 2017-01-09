package com.ucamera.ugallery.gallery.privateimage.util;
/**
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */
import java.io.File;

import com.ucamera.ugallery.util.Models;

import android.os.Environment;

public class Constants {
    public static final File SDCARD_FILE = Environment.getExternalStorageDirectory();
    public static final String SD_PATH = SDCARD_FILE.getAbsolutePath();
    public static String STORE_DIR_LOCKED = SD_PATH + "/UCam/UcamSecretBak";
    public static String STORE_DIR_LOCKED_PHOTOGRAPY = SD_PATH + "/DCIM/UcamSecretBak";
    public static String STORE_DIR_LOCKED_DCIM = SD_PATH + "/DCIM/UcamSecretBak";
    public static final String STORE_DIR_BASE = SD_PATH + "/DCIM/Ucam";
    public static final String STORE_DIR_CACHE = STORE_DIR_BASE + "/.cache";
    public static final String ALL_VIDEOS_BUCKET_ID = "allvideos";
    public static final int NO_STRING = -1;
    static {
        if(Models.Oppo_X907.equals(Models.getModel())) {
            STORE_DIR_LOCKED = SD_PATH +"/我的照片/UcamSecretBak";
        }
    }
}
