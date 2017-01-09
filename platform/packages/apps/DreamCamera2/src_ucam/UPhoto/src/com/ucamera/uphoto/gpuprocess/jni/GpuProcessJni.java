package com.ucamera.uphoto.gpuprocess.jni;

import android.graphics.Bitmap;
import android.util.Log;

/**
 * Copyright (C) 2013 Thundersoft Corporation
 * All rights Reserved
 */
public class GpuProcessJni {

    /*
    * Description: get the output bitmap which has processed by shader
    * src: input bitmap
    * out: output bitmap
    * texture: the resource of shader needed
    * see also setType function annotation
     */
    public static native int gpuProcess(Bitmap src, Bitmap out, Bitmap[] texture);
    /*
    * Description: release resources if needed
     */
    public static native void uninit();
    /*
    *  Description: set the process type of shader
    *  type: the id of effect
    *  vertex: the fragment content of shader
    *  WARNING: this must be invoked before gpuProcess
     */
    public static native int setType(String type, String fragment);

    public static native int init();

    static {
        try {
            System.loadLibrary("GpuProcessJni");
        }
        catch (UnsatisfiedLinkError e) {
            Log.e("GpuProcessJni", "GpuProcessJni library not found!");
        }
    }

}
