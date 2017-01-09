package com.sprd.engineermode.utils;

import android.util.Log;

public class EngineerModeNative {

    static {
        try {
            System.loadLibrary("jni_engineermode");
        } catch (UnsatisfiedLinkError e) {
            Log.d("EngineerModeNative", " #loadLibrary jni_engineermode failed  ");
            e.printStackTrace();
        }
    }

    /**
     * send AT cmd to modem
     *
     * @return (String: the return value of send cmd, "OK":sucess, "ERROR":fail)
     */
    public static native String native_sendATCmd(int phoneId, String cmd);

    public static native boolean native_hashValueWrited();

    public static native int native_get_rootflag();

    public static native int native_check_sdcard_mounted_default();

    public static native String native_get_sd_file_path();
}