/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucam.jni;

import android.util.Log;

public class SkiaSerialize {
    private static String TAG = "SkiaSerialize";
    public static native String GetPathParam(int path);
    public static native String GetPaintParam(int paint);
    static {
    try {
        System.loadLibrary("SkiaSerialize");
    } catch (UnsatisfiedLinkError e) {
        Log.e(TAG, "SkiaSerialize library not found!");
    }
}
}