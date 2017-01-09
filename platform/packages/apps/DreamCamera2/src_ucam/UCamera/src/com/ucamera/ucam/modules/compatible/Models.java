/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.compatible;

import android.os.Build;

import com.ucamera.ucam.modules.utils.LogUtils;

import java.util.Arrays;

public abstract class Models {

/////////////////////////////////////////////////////////////////////////////////////////
    private static final String sModelName;
    static {
        sModelName = Build.MODEL.replace('-', '_').replace(' ', '_');
        LogUtils.debug("Models", "ModeName:%s", sModelName);
    }

    private Models() {}

    public static String getModel()  {return sModelName; }

    //////////////////////////////////////////////////////////////////////////////////////
    // DETAIL MODELs
    //////////////////////////////////////////////////////////////////////////////////////
    public static final String SP_SP7730A             = "SP7730A";
}
