/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.util;

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

    // ////////////////////////////////////////////////////////////////////////////////////
    // DETAIL MODELs
    // ////////////////////////////////////////////////////////////////////////////////////

    public static final String HTC_DOPOD             = "HTC_Flyer_P512";
}
