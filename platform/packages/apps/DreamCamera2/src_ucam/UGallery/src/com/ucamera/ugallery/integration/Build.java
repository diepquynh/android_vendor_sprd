/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.integration;

public class Build {
    private static final String VARIANT = "UGallery";
    public static final boolean HIDE_ADVANCE_SETTINGS = false;
    public static final boolean SHOW_DOCOMO_PHOTO_COLLECTION = false;
    public static final boolean USE_LARGE_CAPACITY_IMAGE = false;
    public static boolean NEED_SECRET_ABLUM = true;
    public static final boolean isTelecomCloud()   {
        return "CT_CLOUDPHONE".equals(android.os.SystemProperties.get("Build.VERSION.INCREMENTAL"));
    }
//    public static final boolean IS_PRE_INSTALL = false;
    public static final boolean isDopod() {
        return VARIANT.equals("Dopod");
    }
    public static final boolean isHosin() {
        return VARIANT.equals("Hosin");
    }
    public static final boolean isSpeedUp() {
        return VARIANT.equals("SpeedUp");
    }
    public static final boolean isPreInstall() {
        return isDopod() || isHosin() || isSpeedUp();
    }
}
