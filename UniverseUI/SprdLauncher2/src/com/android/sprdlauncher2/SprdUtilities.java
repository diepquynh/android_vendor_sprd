package com.android.sprdlauncher2;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.net.URISyntaxException;
import java.util.List;
import java.util.Random;

import com.android.gallery3d.common.Utils;

import android.app.ActivityManager;
import android.app.WallpaperInfo;
import android.app.WallpaperManager;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources.NotFoundException;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Bitmap.Config;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.PaintDrawable;
import android.net.Uri;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;

/* SPRD: bug332730 2014-07-14 use MediaFile to get mime type from file path. @{ */
import android.media.MediaFile;
import android.media.MediaFile.MediaFileType;
/* SPRD: bug332730 2014-07-14 use MediaFile to get mime type from file path. @} */

/**
 * Various utilities shared amongst the Launcher's classes.
 */
final class SprdUtilities {
    private static final String TAG = "Launcher.SprdUtilities";

    /**
     * whether shortcut exists
     * @param context
     * @param className the class name of the component representing the intent
     * @param name the shortcut name
     * @return
     */
    static boolean shortcutExist(Context context, ComponentName className, String name){
        final ContentResolver contentResolver = context.getContentResolver();
        final Uri contentUri = LauncherSettings.Favorites.CONTENT_URI;
        final Cursor c = contentResolver.query(contentUri, null, null, null, null);
        final int intentIndex = c.getColumnIndexOrThrow(LauncherSettings.Favorites.INTENT);
        final int itemTypeIndex = c.getColumnIndexOrThrow(LauncherSettings.Favorites.ITEM_TYPE);
        final int titleIndex = c.getColumnIndexOrThrow(LauncherSettings.Favorites.TITLE);
        String title;
        String intentDescription;
        Intent it;
        int itemType = -1;
        boolean shortcutExist = false;
        while(c.moveToNext()){
            title = c.getString(titleIndex);
            intentDescription = c.getString(intentIndex);
            itemType = c.getInt(itemTypeIndex);
            if(itemType == LauncherSettings.Favorites.ITEM_TYPE_APPLICATION){
                try {
                    it = Intent.parseUri(intentDescription, 0);
                    ComponentName cn = it.getComponent();
                    if(cn.equals(className) && (name == null ? title == null : name.equals(title))){
                        Log.d(TAG, "component already exist:"+cn);
                        shortcutExist = true;
                        break;
                    }
                } catch (URISyntaxException e) {
                    e.printStackTrace();
                }
            }
        }
        Log.d(TAG, "component not exist:"+className);
        if (c != null) c.close();
        return shortcutExist;
    }

    /**
     * Set the window full screen parameters based on isFullScreen
     * @param window
     * @param isFullScreen
     */
    static void setWindowFullScreen(Window window, boolean isFullScreen){
        WindowManager.LayoutParams winParams = window.getAttributes();
        final int bits = WindowManager.LayoutParams.FLAG_FULLSCREEN;
        if(isFullScreen)
            winParams.flags |= bits;
        else
            winParams.flags &= ~bits;
        window.setAttributes(winParams);
    }

    /**
     * Set wallpaper specified by target id.
     *
     * @param target WALLPAPER_ALL_TYPE/WALLPAPER_DEFAULT_TYPE/WALLPAPER_LOCKSCREEN_TYPE.
     * @param data wallpaper bitmap data.
     * @param wpm WallpaperManager
     * @return true if set wallpaper ok, otherwise set wallpaper fail.
     */
    public static boolean setWallpaperByTarget(int target, InputStream data, WallpaperManager wpm) {

        if (data == null || wpm == null) {
            Log.e(TAG, "setWallpaperByTarget, params data and wpm must not be null.");
            return false;
        }

        boolean setResult = true;
        try {
            /* SPRD: bug310310 2014-05-09 let lockscreen wallpaper the same size with dispaly size. @{ */
            switch (target) {
                // currently target won't be WallpaperInfo.WALLPAPER_ALL_TYPE,
                // so we comment the next line
                //  case WallpaperInfo.WALLPAPER_ALL_TYPE:
                case WallpaperInfo.WALLPAPER_DEFAULT_TYPE:
                case WallpaperInfo.WALLPAPER_LOCKSCREEN_TYPE:
                case WallpaperInfo.WALLPAPER_MAINMENU_TYPE:
                    wpm.setStream(data, target);
                    break;

                default:
                    Log.e(TAG, "setWallpaperByTarget unknown target id.");
                    setResult = false;
                    break;
            }
            /* SPRD: bug310310 2014-05-09 let lockscreen wallpaper the same size with dispaly size. @} */
        } catch (IOException e) {
            setResult = false;
        } finally {
            Utils.closeSilently(data);
        }

        return setResult;
    }

    /* SPRD: bug328858 2014-07-01 code refactor. @{ */
    public static Bitmap resizeImage(Bitmap bitmap, int destW, int destH) {

        // load the origial Bitmap
        int width = bitmap.getWidth();
        int height = bitmap.getHeight();

        if (width <= destW && height <= destH) {
            return bitmap;
        }

        // calculate the scale
        float scaleWidth = ((float) destW) / width;
        float scaleHeight = ((float) destH) / height;

        // create a matrix for the manipulation
        Matrix matrix = new Matrix();
        matrix.postScale(scaleWidth, scaleHeight);

        Bitmap resizedBitmap = Bitmap.createBitmap(bitmap, 0, 0, width,
                height, matrix, true);

        return resizedBitmap;

    }

    public static BitmapDrawable getCompoundedDrawable(Bitmap layer1, Bitmap layer2) {
        int src1W = layer1.getWidth();
        int src1H = layer1.getHeight();

        int src2W = layer2.getWidth();
        int src2H = layer2.getHeight();

        int destW = src2W;
        int destH = src2H;

        float src1X = 0;
        float src1Y = 0;
        float src2X = 0;
        float src2Y = 0;

        if (src2W < src1W) {
            src2X = (src1W - src2W) / 2;
            destW = src1W;
        }

        if (src2H < src1H) {
            src2Y = (src1H - src2H) / 2;
            destH = src1H;
        }

        if (src1W < src2W) {
            src1X = (src2W - src1W) / 2;
        }

        if (src1H < src2H) {
            src1Y = (src2H - src1H) / 2;
        }

        Bitmap compoundBmp = Bitmap.createBitmap(destW, destH, Config.ARGB_8888);

        Canvas canvas = new Canvas(compoundBmp);
        canvas.drawBitmap(layer1, src1X, src1Y, null);
        canvas.drawBitmap(layer2, src2X, src2Y, null);
        return new BitmapDrawable(compoundBmp);
    }

    static public BitmapDrawable getDestDrawale(BitmapDrawable drawable, int width,
            int height) {

        if (drawable == null) {
            return null;
        }

        Bitmap bitmap = drawable.getBitmap();
        int sourceWidth = bitmap.getWidth();
        int sourceHeight = bitmap.getHeight();

        if (sourceWidth <= width || sourceHeight <= height) {
            return drawable;
        }

        int destX = (sourceWidth - width) / 2;
        int dextY = (sourceHeight - height) / 2;

        Bitmap destBitmap = Bitmap.createBitmap(bitmap, destX, dextY, width,
                height);

        return new BitmapDrawable(destBitmap);
    }

    public static BitmapDrawable compoundPreBitmap(Bitmap src, Bitmap pre) {

        if (src == null) {
            return null;
        }

        int srcW = src.getWidth();
        int srcH = src.getHeight();
        int preW = pre.getWidth();
        int preH = pre.getHeight();

        int rgb1[] = new int[srcW * srcH];
        int rgb2[] = new int[preW * preH];
        src.getPixels(rgb1, 0, srcW, 0, 0, srcW, srcH);
        pre.getPixels(rgb2, 0, preW, 0, 0, preW, preH);

        int minLength = Math.min(rgb1.length, rgb2.length);
        int alphaV = 0;
        int colorV = 0;
        for (int i=0; i<minLength; i++) {
            if (rgb2[i] == 0) {
                rgb1[i] = 0; // set it to transparent
            } else {
                alphaV = (rgb2[i] & 0xff000000);
                if (alphaV != 0xff000000 && alphaV != 0x00000000) {
                    colorV = (rgb1[i] & 0x00ffffff);
                    rgb1[i] =  alphaV | colorV;
                }
            }
        }

        for (int i=minLength; i<rgb1.length; i++) {
            rgb1[i] = 0; // set it to transparent
        }

        Bitmap dest = Bitmap.createBitmap(rgb1, srcW, srcH, Config.ARGB_8888);
        return new BitmapDrawable(dest);
    }
    /* SPRD: bug328858 2014-07-01 code refactor. @} */

    /* SPRD: add for 262515 @{ */
    private static final String LAUNCHER2_PROCESS = "com.android.sprdlauncher2";

    public static boolean isLauncherBackground(Context context) {
        ActivityManager amgr = (ActivityManager) context
                .getSystemService(Context.ACTIVITY_SERVICE);
        List<RunningAppProcessInfo> apps = amgr.getRunningAppProcesses();
        for (RunningAppProcessInfo app : apps) {
            if (app.processName.equals(LAUNCHER2_PROCESS)) {
                if (app.importance == RunningAppProcessInfo.IMPORTANCE_BACKGROUND) {
                    return true;
                }
            }
        }
        return false;
    }

    /* @} */
    /* SPRD : fix bug280707 @{ */
    public static int getStatusbarHeight(Context context) {
        Class<?> c = null;
        Object obj = null;
        Field field = null;
        int x = 0, sbar = 0;
        try {
            c = Class.forName("com.android.internal.R$dimen");
            obj = c.newInstance();
            field = c.getField("status_bar_height");
            x = Integer.parseInt(field.get(obj).toString());
            sbar = context.getResources().getDimensionPixelSize(x);
        } catch (NumberFormatException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (NotFoundException e) {
            e.printStackTrace();
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        } catch (InstantiationException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        }
        return sbar;
    }
    /* @} */

    /* SPRD: bug332730 2014-07-14 use MediaFile to get mime type from file path. @{ */
    public static String getMimeTypeBaseOnFileExtension(String filePath) {
        MediaFileType mft = MediaFile.getFileType(filePath);
        if (mft != null) {
            return mft.mimeType;
        }
        return "";
    }
    /* SPRD: bug332730 2014-07-14 use MediaFile to get mime type from file path. @} */
}
