/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto.util;

import java.util.Arrays;

import android.os.Build;


public abstract class Models {

    private static final String sModelName;
    static {
        sModelName = Build.MODEL.replace('-', '_').replace(' ', '_');
    }

    private Models() {
    }

    public static String getModel() {
        return sModelName;
    }

    public static boolean isSupportedEffect() {
        return !Arrays.asList(new String[]{Samsung_GT_S6358}).contains(getModel());
    }

    // ////////////////////////////////////////////////////////////////////////////////////
    // DETAIL MODELs
    // ////////////////////////////////////////////////////////////////////////////////////
    public static final String AMAZON_KFTT           = "KFTT";
    public static final String Samsung_GT_S6358      = "GT_S6358";
    public static final String Samsung_GT_I9508      = "GT_I9508";
    public static final String Samsung_GT_I9500      = "GT_I9500";
    public static final String HUAWEI_P6             = "HUAWEI_P6_T00";
    public static final String LAJIAO_LA3_W          = "LA3_W";
}
