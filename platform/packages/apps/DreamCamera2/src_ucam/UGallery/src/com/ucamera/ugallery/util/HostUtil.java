/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.util;

import android.app.ActivityManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Environment;
import android.preference.PreferenceManager;

import com.ucamera.ugallery.UGalleryConst;

import java.io.File;
import java.io.InputStream;
import java.util.Locale;
import java.util.TimeZone;

public class HostUtil {
    private static final TimeZone CURRENT_TIME_ZONE = TimeZone.getDefault();

    public static String runCmd(String[] cmd, String wdir) {
        InputStream in = null;
        try {
            ProcessBuilder pBuilder = new ProcessBuilder(cmd);
            if (wdir != null) {
                pBuilder.directory(new File(wdir));
                pBuilder.redirectErrorStream(true);

                Process process = pBuilder.start();
                in = process.getInputStream();

                StringBuilder sb = new StringBuilder();

                byte[] buffer = new byte[512];
                int length = -1;
                while ((length = in.read(buffer)) != -1) {
                    sb.append(new String(buffer, 0, length));
                }
                return sb.toString();
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        } finally {
            Util.closeSilently(in);
        }
        return "";
    }


    @SuppressWarnings("deprecation")
    public static String getSoftwareInfo(Context context) {
            return new StringBuilder()
                .append("TimeZone :").append(CURRENT_TIME_ZONE.getDisplayName())
                .append("\nUCam Ver : ").append(Util.getPackageVersion(context))
                .append("\nBrand : ").append(Build.BRAND)
                .append("\nDevice : ").append(Build.DEVICE)
                .append("\nModel :").append(Build.MODEL)
                .append("\nBuildID :").append(Build.ID)
                .append("\nSDK Ver :").append(Build.VERSION.SDK)
                .append("\nVersion :").append(Build.VERSION.RELEASE)
                .toString();
    }

    public static String makeTitleBase(Context context) {
        return new StringBuilder()
            .append("[V:").append(Util.getPackageVersion(context)).append("]")
            .append("[B:").append(Build.BRAND).append("]")
            .append("[M:").append(Build.MODEL).append("]")
            .append("[I:").append(Build.ID).append("]")
            .append("[L:").append(Locale.getDefault().toString()).append("]")
            .toString();
    }

    public static String getCpuInfo(Context context){
        String[] args = new String[]{"/system/bin/cat", "/proc/cpuinfo"};
        return HostUtil.runCmd(args,"/system/bin/");
    }

    public static String getMemoryInfo(Context context) {
        ActivityManager actManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo outInfo = new ActivityManager.MemoryInfo();
        actManager.getMemoryInfo(outInfo);

        String[] args = { "/system/bin/cat", "/proc/meminfo" };

        return new StringBuffer()
            .append("\nTotal Available Memory :")
            .append(outInfo.availMem >> 10).append("K")
            .append("\nTotal Available Memory :")
            .append(outInfo.availMem >> 20).append("K")
            .append("\nIn low memory situation:")
            .append(outInfo.lowMemory)
            .append(HostUtil.runCmd(args, "/system/bin/"))
            .toString();
    }

    public static String getSystemProps(Context context) {
        String[] args = new String[] {"/system/bin/getprop"};
        return runCmd(args, "/system/bin");
    }

    public static String getCameraParameters(Context context) {
        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        return new StringBuilder()
            .append("\n[BACK]\n")
            .append(sharedPrefs.getString(UGalleryConst.KEY_INIT_PARAM + "_" + UGalleryConst.CAMERA_BACK, "NO"))
            .append("\n[FRONT]\n")
            .append(sharedPrefs.getString(UGalleryConst.KEY_INIT_PARAM + "_" + UGalleryConst.CAMERA_FRONT, "NO"))
            .toString();
    }

    public static String zipCameraPreference(Context context) {
        try {
            String dataDir = context.getApplicationInfo().dataDir + File.separator + "shared_prefs";
            String zipFile = Environment.getExternalStorageDirectory().toString() + "/UCam/preferences.zip";
            ZipUtil.zipFile(dataDir, zipFile);
            return zipFile;
        }catch (Exception e) {
            return null;
        }
    }
}
