/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.util;

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

    public static boolean isLaJiaoPepper() {
        return Arrays.asList(new String[]{
                LAJIAO_LA3_W,
                LAJIAO_LA2_W,
                LAJIAO_LA3S
        }).contains(getModel());
    }
    // ////////////////////////////////////////////////////////////////////////////////////
    // DETAIL MODELs
    // ////////////////////////////////////////////////////////////////////////////////////
    public static final String SN_IS11CA = "IS11CA";
    public static final String SN_IS11N = "IS11N";
    public static final String SN_N06C  = "N06C";
    public static final String SN_N_03E = "N_03E";
    public static final String SN_N_04D = "N_04D";
    public static final String SN_N_04C = "N_04C";
    public static final String SN_N_06C = "N_06C";
    public static final String SN_N_07D = "N_07D";
    public static final String Oppo_X907             = "X907";
    public static final String AMAZON_KFTT           = "KFTT";
    public static final String Samsung_GT_I9500      = "GT_I9500";
    public static final String Samsung_GT_I9508      = "GT_I9508";
    public static final String HUAWEI_P6             = "HUAWEI_P6_T00";
    public static final String LAJIAO_LA3_W          = "LA3_W";
    public static final String LAJIAO_LA2_W          = "LA2_W";
    public static final String LAJIAO_LA3S           = "LA3S";
    public static final String Lenovo_K900           = "Lenovo_K900";
    public static final String HTC_DOPOD             = "HTC_Flyer_P512";
    public static final String GOOGLE_Nexus_7        = "Nexus_7";
}
