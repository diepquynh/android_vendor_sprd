
package com.sprd.appbackup.appbackup;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ApplicationInfo;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.PixelFormat;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Environment;
import android.os.StatFs;
import android.os.SystemProperties;
import android.util.Base64;
import android.util.DisplayMetrics;
import android.util.Log;
import android.content.pm.PackageParser;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.lang.reflect.Field;
import java.util.Properties;
import com.sprd.appbackup.AppInfo;


public class Utils {

    private static final String TAG = "Utils";

    public static final int SUCCESS = 0;

    public static final int FAIL = -1;

    public static boolean fileExists(String filePath) {
        File file = new File(filePath);
        if (file != null && file.exists()) {
            return true;
        }
        return false;
    }

    public static void checkAndMakeFolder(String filePath) {
        File file = new File(filePath);
        if (file != null && !file.exists()) {
            file.mkdirs();
        }
    }

    public static void deletePath(File path) {
        if (path == null || !path.exists()) {
            return;
        }
        if (path.isFile()) {
            path.delete();
        } else {
            File[] list = path.listFiles();
            if (list == null)
                return;
            for (File f : list) {
                if (!f.delete()) {
                    deletePath(f);
                }
            }
            path.delete();
        }
    }

    public static boolean copyFileToDir(String appPath, String dirPath,
            String appName) {
        boolean ret = false;
        File dirFile = new File(dirPath);
        if (!dirFile.exists()) {
            dirFile.mkdirs();
        }
        Log.i(TAG, "appPath : " + appPath);
        File appFile = new File(appPath);
        Log.i(TAG, "appFile.length() : " + appFile.length());
        long time = appFile.lastModified();
        File backupAppFile = new File(dirPath + "/" + appName);
        FileInputStream in = null;
        FileOutputStream out = null;
        try {
            in = new FileInputStream(appFile);
            out = new FileOutputStream(backupAppFile);
            byte[] buffer = new byte[1024];
            int length;
            while ((length = in.read(buffer)) != -1) {
                out.write(buffer, 0, length);
            }
            backupAppFile.setLastModified(time);
            ret = true;
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            ret = false;
        } catch (IOException e) {
            e.printStackTrace();
            ret = false;
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e) {
                }
            }
            if (out != null) {
                try {
                    out.close();
                } catch (IOException e) {
                }
            }
        }
        return ret;
    }

    public static AppInfo packageInfoToAppInfo(PackageManager pm,
            PackageInfo info, Drawable drawable) {
        AppInfo appInfo = new AppInfo();
        if (info != null) {
            appInfo.setPackagePath(info.applicationInfo.sourceDir);
            appInfo.setPackageName(info.applicationInfo.packageName);
            appInfo.setVersionCode(info.versionCode);
            appInfo.setVersionName(info.versionName);
            appInfo.setName(pm.getApplicationLabel(info.applicationInfo)
                    .toString());
            appInfo.setApkSize(new File(info.applicationInfo.sourceDir)
                    .length());
            Log.d(TAG, "setPackagePath = "
                    + info.applicationInfo.sourceDir);
            if (drawable != null) {
                appInfo.setIcon(drawable);
            } else {
                appInfo.setIcon(pm.getApplicationIcon(info.applicationInfo));
            }
        }
        return appInfo;
    }

    public static Drawable getUninstalledAPKIcon(Context context, String apkPath) {
        PackageParser parser = new PackageParser();
        DisplayMetrics metrics = new DisplayMetrics();
        metrics.setToDefaults();

        PackageParser.Package pac = null;

        try {
            pac = parser.parsePackage(new File(apkPath), 0);
            if(pac == null){
                return null;
            }
        } catch (PackageParser.PackageParserException e) {
            return null;
        }

        ApplicationInfo info = pac.applicationInfo;
        AssetManager assetMag = new AssetManager();
        assetMag.addAssetPath(apkPath);
        Resources res = context.getResources();
        res = new Resources(assetMag, metrics, res.getConfiguration());
        if (info.icon != 0) {
            Drawable icon = res.getDrawable(info.icon);
            return icon;
        }
        return null;
    }
   //fix bug 200501 
    public static String getUninstalledAPKName(Context context, String apkPath) {

        PackageParser parser = new PackageParser();
        DisplayMetrics metrics = new DisplayMetrics();
        metrics.setToDefaults();

        PackageParser.Package pac = null;
        try {
            pac = parser.parsePackage(new File(apkPath), 0);
            if(pac == null){
                return null;
            }
        } catch (PackageParser.PackageParserException e) {
            return null;
        }

        ApplicationInfo info = pac.applicationInfo;
        AssetManager assetMag = new AssetManager();
        assetMag.addAssetPath(apkPath);
        Resources res = context.getResources();
        res = new Resources(assetMag, metrics, res.getConfiguration());
        CharSequence label = null;
        if (info.labelRes != 0) {
            try {
                label = res.getText(info.labelRes);
            } catch (Resources.NotFoundException e) {
            	return null;
            }
            return label.toString();
        }
        if (label == null) {
            label = (info.nonLocalizedLabel != null) ?
                    info.nonLocalizedLabel : info.packageName;
            return label.toString();
        }
        return null;
    }

}

