/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle;

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

import android.app.ActivityManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.drawable.Drawable;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.Build;
import android.provider.MediaStore.Images.Media;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;

public class PuzzleUtils {
    // add for pip
    public static void recyleBitmap(Bitmap bitmap) {
        if (bitmap != null && !bitmap.isRecycled()) {
            bitmap.recycle();
        }
    }

    public static ArrayList<Integer> getIconIds(Resources res, int iconsRes) {
        if (iconsRes == 0)
            return null;
        ArrayList<Integer> ids = new ArrayList<Integer>();
        TypedArray array = res.obtainTypedArray(iconsRes);
        int n = array.length();
        for (int i = 0; i < n; ++i) {
            ids.add(array.getResourceId(i, 0));
        }
        array.recycle();
        return ids;
    }

    public static Bitmap createBitmap(Context context, Uri uri, int maxWidth, int maxHeight) {
        InputStream is = null;

        // decode bitmap bounds
        int bitmapWidth = 0;
        int bitmapHeight = 0;
        try {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inJustDecodeBounds = true;
            is = context.getContentResolver().openInputStream(uri);
            BitmapFactory.decodeStream(is,null,options);
            bitmapWidth = options.outWidth;
            bitmapHeight = options.outHeight;
        } catch (IOException e) {
            Log.e("BitmapUtil", "fail get bounds info",e);
            // fail get bounds info
            return null;
        } finally {
            closeSilently(is);
        }

        //calc sampleSize accounding width/height
        int sampleSize = 1;
        while (maxWidth < (bitmapWidth / sampleSize) || maxHeight < (bitmapHeight / sampleSize) ) {
            sampleSize <<= 1;
        }

        //decode the bitmap
        try {
            Bitmap tmp = null;
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inSampleSize = sampleSize;
            is = context.getContentResolver().openInputStream(uri);
            tmp = BitmapFactory.decodeStream(is,null,options);

            float scale = Math.min(1,Math.min((float)maxWidth/options.outWidth, (float)maxHeight/options.outHeight));
            int w = (int)(options.outWidth * scale);
            int h = (int)(options.outHeight*scale);
            Bitmap bitmap = Bitmap.createScaledBitmap(tmp,w,h, false);
            if (bitmap != tmp){
                recyleBitmap(tmp);
            }

            int rotateDegree = getImageDegreeByUri(uri, context);
            if (rotateDegree != 0) {
                tmp = rotate(bitmap, rotateDegree);
                /*
                 * BUG FIX: 1486
                 * FIX COMMENT: some case bitmap == tmp, so dont recycle
                 * DATE: 2012-08-21
                 */
                if (tmp != bitmap){
                    recyleBitmap(bitmap);
                    bitmap = tmp;
                }
            }
            return bitmap;
        } catch (Exception e) {
            Log.e("BitmapUtil","Failt create bitmap",e);
            // some error occure
        }finally {
            closeSilently(is);
        }
        return null;
    }

    public static Bitmap rotate(Bitmap b, int degrees) {
        if (degrees != 0 && b != null) {
            Matrix m = new Matrix();
            m.setRotate(degrees, (float) b.getWidth() / 2, (float) b.getHeight() / 2);
            try {
                Bitmap b2 = Bitmap.createBitmap(b, 0, 0, b.getWidth(), b.getHeight(), m, true);
                if (b != b2) {
                    b.recycle();
                    b = b2;
                }
            } catch (OutOfMemoryError ex) {
                // We have no memory to rotate. Return the original bitmap.
            }
        }
        return b;
    }

    private static int getImageDegreeByUri(Uri target, Context context) {
        if (target == null) {
            return 0;
        }
        String filepath = getDefaultPathAccordUri(target, context);
        if (filepath != null) {
            return getExifOrientation(filepath);
        } else {
            Log.d("BitmapUtil", "The path in uri: " + target + " is null");
            return 0;
        }
    }

    public static void closeSilently(Closeable c) {
        if (c == null)
            return;
        try {
            c.close();
        } catch (Throwable t) {
            // ignore
        }
    }

    private static String getDefaultPathAccordUri(Uri uri, Context context) {
        String strPath = null;
        if (ContentResolver.SCHEME_FILE.equals(uri.getScheme())) {
            strPath = uri.getPath();
        } else if (ContentResolver.SCHEME_CONTENT.equals(uri.getScheme())) {
            final String[] IMAGE_PROJECTION = new String[] {Media.DATA};
            Cursor cr = context.getContentResolver().query(uri, IMAGE_PROJECTION, null, null, null);
            if(cr != null && cr.getCount() > 0) {
                if(cr.isBeforeFirst()) {
                    cr.moveToFirst();
                    strPath = cr.getString(cr.getColumnIndex(Media.DATA));
                }
            /* SPRD:  CID 108996 : Resource leak (RESOURCE_LEAK) @{ */
                // cr.close();
            }
            if(cr != null){
                cr.close();
            }
            /* @} */
        }
        return strPath;
    }

    public static int getExifOrientation(String filepath) {
        int degree = 0;
        ExifInterface exif = null;
        try {
            exif = new ExifInterface(filepath);
        } catch (IOException ex) {
            Log.e("Utils", "cannot read exif", ex);
        }
        if (exif != null) {
            int orientation = exif.getAttributeInt(
                ExifInterface.TAG_ORIENTATION, -1);
            if (orientation != -1) {
                // We only recognize a subset of orientation tag values.
                switch(orientation) {
                    case ExifInterface.ORIENTATION_ROTATE_90:
                        degree = 90;
                        break;
                    case ExifInterface.ORIENTATION_ROTATE_180:
                        degree = 180;
                        break;
                    case ExifInterface.ORIENTATION_ROTATE_270:
                        degree = 270;
                        break;
                }

            }
        }
        return degree;
    }

    public static String getModle(){
        return Build.MODEL.replace('-', '_').replace(' ', '_');
    }
    public static boolean getGridPuzlePre(Context context) {
        SharedPreferences sp = context.getSharedPreferences("gridpuzzle", Context.MODE_PRIVATE);
        return sp.getBoolean("first", true);
    }
    public static void setGridPuzzlePre(Context context ,boolean isFirstUse) {
        SharedPreferences sp = context.getSharedPreferences("gridpuzzle", Context.MODE_PRIVATE);
        Editor editor = sp.edit();
        editor.putBoolean("first", isFirstUse);
        editor.commit();
    }
//    public static boolean getFreePuzzlePre(Context context) {
//        SharedPreferences sp = context.getSharedPreferences("freepuzzle", Context.MODE_PRIVATE);
//        return sp.getBoolean("first", true);
//    }
//    public static void setFreePuzzlePre(Context context ,boolean isFirstUse) {
//        SharedPreferences sp = context.getSharedPreferences("freepuzzle", Context.MODE_PRIVATE);
//        Editor editor = sp.edit();
//        editor.putBoolean("first", isFirstUse);
//        editor.commit();
//    }
    public static int getFreePuzzleLeft(Context context) {
        Drawable drawable = context.getResources().getDrawable(R.drawable.bt_arrow_right);
        return drawable == null ? 0 : drawable.getIntrinsicWidth();
    }
    public static int getPuzzleWidth(Context context) {
        DisplayMetrics metrics = new DisplayMetrics();
        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        wm.getDefaultDisplay().getMetrics(metrics);
        return metrics.widthPixels;
    }
    public static Bitmap createPUzzlePieceBitmap(Context context, int count, Uri uri) {
        final ActivityManager activityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
        activityManager.getMemoryInfo(memInfo);
        Log.d("PuzzleView", "availMem:" + memInfo.availMem + "threshold:" + memInfo.threshold);
        double availMem = memInfo.availMem * 0.5;
        double maxSize = (availMem/4) / count;
        int size = (int)Math.min(Math.sqrt(maxSize), 1024);
        Bitmap bitmap = null;
        android.util.Log.d("PuzzleView", "max bitmap size=" + size);
        while(bitmap == null && size > 50) {
            try {
                bitmap = createBitmap(context, uri,size,size);
                if (bitmap != null) {
                    break;
                }
                size /= 2;
            } catch(OutOfMemoryError oom) {
                android.util.Log.e("PuzzleView","Fail create bitmap",oom);
                size /= 2;
                bitmap = null;
            }
        }
        return bitmap;
    }
}
