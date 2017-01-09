/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto.exception;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Environment;

import com.ucamera.uphoto.Utils;

import java.io.File;
import java.io.InputStream;
import java.util.Locale;
import java.util.TimeZone;

public class HostUtil {
    private static final TimeZone CURRENT_TIME_ZONE = TimeZone.getDefault();
    public static final int CAMERA_BACK = 0;
    public static final int CAMERA_FRONT = 1;
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
                    /*SPRD: CID 109145 (#1 of 1): Dm: Dubious method used (FB.DM_DEFAULT_ENCODING) @{
                    sb.append(new String(buffer, 0, length));
                    */
                    sb.append(new String(buffer, 0, length , "utf-8"));
                    /* @} */
                }
                return sb.toString();
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        } finally {
            Utils.closeSilently(in);
        }
        return "";
    }


    @SuppressWarnings("deprecation")
    public static String getSoftwareInfo(Context context) {
            return new StringBuilder()
                .append("TimeZone :").append(CURRENT_TIME_ZONE.getDisplayName())
                .append("\nUCam Ver : ").append(getPackageVersion(context))
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
            .append("[V:").append(getPackageVersion(context)).append("]")
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
        return "";
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

    public static String getPackageVersion(Context context) {
        String version = "";
        try {
            version = context.getPackageManager().getPackageInfo(context.getPackageName(), 0).versionName;
        } catch (PackageManager.NameNotFoundException e) {
            // do nothing
        }
        return version;
    }
}
