/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 *
 *  Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.ucamera.uphoto;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Xfermode;
import android.graphics.Bitmap.Config;
import android.media.ExifInterface;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelFileDescriptor;
import android.preference.PreferenceManager;
import android.text.format.DateFormat;
import android.util.Log;
import android.util.Pair;
import android.view.View;
import android.view.Window;
import android.widget.Toast;
import android.os.Environment;
import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

import com.dream.camera.settings.DataConfig.DataStoragePath;
import com.dream.camera.settings.DreamSettingUtil;
import com.sprd.camera.storagepath.StorageUtilProxy;
import com.ucamera.uphoto.util.Models;

/**
 * Collection of utility functions used in this package.
 */
public class ImageEditOperationUtil {
    private final static String TAG = "ImageEditOperationUtil";
    public static final String DCIM = Environment
            .getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)
            .toString();
    private static final String DEFAULT_DIR = "/DCIM/Camera";
    public static final String DIRECTORY = DCIM + "/Camera";
    public static final String INTERNALDIR = StorageUtilProxy
            .getInternalStoragePath().toString() + DEFAULT_DIR;
    public static final String EXTERNALDIR = StorageUtilProxy
            .getExternalStoragePath().toString() + DEFAULT_DIR;
    public static final String KEY_DEFAULT_INTERNAL = "Internal";
    public static final String KEY_DEFAULT_EXTERNAL = "External";
    private ImageEditOperationUtil() {
    }

    public static byte[] transformBitmapToBuffer(Bitmap bitmap) {

        /*
         * BUGFIX: 1010 1273
         * FIX COMMENT: if bitmap is null or recycled, then just return
         * DATE: 2012-05-18 2012-07-15
         */
        if (bitmap == null || bitmap.isRecycled()) return null;
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        bitmap.compress(Bitmap.CompressFormat.JPEG, 100, buffer);
        byte[] jpegData = buffer.toByteArray();
        if (buffer != null) {
            try {
                buffer.close();
            } catch (Throwable t) {
                // do nothing
            }
        }
        return jpegData;
    }

    public static Bitmap transformBufferToBitmap(byte[] jpegData) {
        Bitmap bitmap = null;
        try {
            BitmapFactory.Options options = Utils.getNativeAllocOptions();
            bitmap = BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length, options);
        } catch (OutOfMemoryError ex) {
            // We have no memory to rotate. Return the original bitmap.
            bitmap = null;
            Log.w(TAG, "transformBufferToBitmap(): code has a memory leak is detected...");
            ImageEditDesc.getInstance().reorganizeQueue();
            System.gc();
            //transformBufferToBitmap(jpegData);
        }
        return bitmap;
    }

    // Rotates the bitmap by the specified degree.
    // if not rotate, return b itself
    public static Bitmap rotate(Bitmap b, int degrees) {
        degrees = degrees % 360;
        if (degrees == 0) return b;

        Bitmap b2 = null;
        if (b != null && !b.isRecycled()) {
            Matrix m = new Matrix();
            m.setRotate(degrees, (float) b.getWidth() / 2, (float) b.getHeight() / 2);
            try {
                b2 = Bitmap.createBitmap(b, 0, 0, b.getWidth(), b.getHeight(),m, true);
                Log.d(TAG, "rotate(): b = " + b + ", b2 = " + b2);
            } catch (OutOfMemoryError ex) {
                // We have no memory to rotate. Return the original bitmap.
                b2 = null;
                Log.w(TAG, "rotate(): code has a memory leak is detected...");
                ImageEditDesc.getInstance().reorganizeQueue();
                System.gc();
                //rotate(b, degrees);
            }
        }
        return b2;
    }

    public static RectF resizeRectF(Bitmap bitmap, float windowWidth,
            float windowHeight) {
        return resizeRectF(bitmap, windowWidth, windowHeight, false);
    }

    public static RectF regizeRect(Bitmap bitmap, float windowWidth,
            float windowHeight, Matrix matrix) {
        float srcWidth = bitmap.getWidth();
        float srcHeight = bitmap.getHeight();
        float left;
        float top;
        float right;
        float bottom;
        float[] values = new float[9];
        matrix.getValues(values);
        srcWidth *= values[0];
        srcHeight *= values[4];
        left = (windowWidth - srcWidth) / 2;
        top = (windowHeight - srcHeight) / 2;
        right = left + srcWidth;
        bottom = top + srcHeight;
        return new RectF(left, top, right, bottom);
    }
    public static RectF resizeRectF(Bitmap bitmap, float windowWidth,
            float windowHeight, boolean isScale) {
        float srcWidth = 0;
        float srcHeight = 0;
        float left;
        float top;
        float right;
        float bottom;
        double scale;
        float targetWidth;
        float targetHeight;

        /*
         * FIX BUG: 4968
         * FIX COMMENT: avoid null point exception
         * DATE: 2013-10-11
        */
        if(bitmap != null) {
            srcWidth = bitmap.getWidth();
            srcHeight = bitmap.getHeight();
        }
        if(srcWidth == 0 || srcHeight == 0) {
            return new RectF(windowWidth / 2, windowHeight / 2, windowWidth / 2, windowHeight / 2);
        }

        if(isScale == false && srcWidth < windowWidth && srcHeight < windowHeight){
            Log.d(TAG, "resizeRectF(): resizeRectF...");
            left = (windowWidth - srcWidth) / 2;
            top = (windowHeight - srcHeight) / 2;
            right = left + srcWidth;
            bottom = top + srcHeight;
            return new RectF(left, top, right, bottom);
        }

        RectF rectF = new RectF();
        if ((srcWidth / windowWidth) <= (srcHeight / windowHeight)) {
          scale = srcHeight / windowHeight;
          targetWidth = (float) (srcWidth / scale);
          left = (windowWidth - targetWidth) / 2;
          targetWidth = left + targetWidth;
          rectF =  new RectF(left, 0, targetWidth, windowHeight);
        } else {
          scale = srcWidth / windowWidth;
          targetHeight = (int) (srcHeight / scale);
          top = (windowHeight - targetHeight) / 2;
          targetHeight = top + targetHeight;
          rectF = new RectF(0, top, windowWidth, targetHeight);
        }
        return rectF;
    }

    public static Bitmap getDstBitmap(Bitmap srcBitmap, Matrix matrix, Paint paint, Config config) throws OutOfMemoryError{
        if (srcBitmap == null) {
            return null;
        }
        Bitmap bmp = null;
        if (config == null) {
            config = srcBitmap.getConfig();
        }
        /**
         * FIX BUG: 706 1249
         * BUG CAUSE: config is null;
         * FIX COMMENT: set the default Config.ARGB_8888
         * DATE: 2012-07-17
         */
        bmp = Bitmap.createBitmap(srcBitmap.getWidth(), srcBitmap.getHeight(), (config != null ? config : Config.ARGB_8888));
        if (bmp == null) {
            return null;
        }
        Canvas canvas = new Canvas(bmp);
        if (paint == null) {
            paint = new Paint();
        }

        if (matrix != null) {
            canvas.drawBitmap(srcBitmap, matrix, paint);
        } else {
            canvas.drawBitmap(srcBitmap, 0, 0, paint);
        }
        return bmp;
    }

    public static Bitmap operateFlipBitmap(Bitmap src, float scale) {
        if (src == null) {
            return null;
        }
        Bitmap b2 = null;
        final Matrix m = new Matrix();
        m.setScale(scale, -scale);
        try {
            b2 = Bitmap.createBitmap(src, 0, 0, src.getWidth(), src.getHeight(), m, true);
        } catch (OutOfMemoryError ex) {
            // We have no memory to rotate. Return the original bitmap.
            b2 = null;
            Log.w(TAG, "operateFlipBitmap(): code has a memory leak is detected...");
            ImageEditDesc.getInstance().reorganizeQueue();
            System.gc();
        }
        return b2;
    }

    public static Bitmap operateSymmetryBitmap(Bitmap bitmap,int clipX, int clipY, int clipWidth, int
             clipHeight, int dstX, int dstY, float scale, Xfermode xfermode) {
        Bitmap bottomLayer = null;
        Bitmap src = bitmap;
        try {
            final Matrix m = new Matrix();
            final Paint paint = new Paint();
            m.setScale(scale, -scale);
            Bitmap nestedBitmap = Bitmap.createBitmap(src, clipX, clipY , clipWidth , clipHeight, m, false);
            Config config = src.getConfig();
            bottomLayer =  Bitmap.createBitmap(src.getWidth(), src.getHeight(), (config != null? config: Config.ARGB_8888));
            m.reset();
            Canvas canvas = new Canvas(bottomLayer);
            canvas.drawBitmap(src, m, paint);
            paint.reset();
            paint.setXfermode(xfermode);
            canvas.drawBitmap(nestedBitmap, dstX, dstY, paint);
            Utils.recyleBitmap(nestedBitmap);
        } catch (OutOfMemoryError ex) {
            // We have no memory to rotate. Return the original bitmap.
            Log.w(TAG, "operateSymmetryBitmap(): code has a memory leak is detected...");
            ImageEditDesc.getInstance().reorganizeQueue();
            System.gc();
        }catch (Exception e) {
            e.printStackTrace();
        }
        Log.d(TAG, "operateSymmetryBitmap(): bottomLayer = " + bottomLayer);
        return bottomLayer;
    }

    /**
     * get the full path of selected photoframe
     * @return fullPath string
     */
    public static String getPhotoFileFullPath(String strFilename, String strDir, String strDownloadDir) {
        String fileName = null;
        //if photoframe/texture comes form SDcard, then the "temp" is photoframe/texture_hdpi/photoframe/texture_1xxx.jpg
        // or photoframe/texture_mdpi/photoframe/texture_1xxx.jpg
        //if photoframe/texture comes form application, then the temp is photoframe/texture_0xx.jpg
        if(strFilename.startsWith(strDir + "_") && !strFilename.startsWith(strDir + "_0")) {
//            fileName = CameraActivity.DEFAULT_DIRECTORY + strDir + "/" +
            fileName = Const.DOWNLOAD_DIRECTORY + strDir + "/" +
            strDir + "/" + strFilename.substring(strFilename.indexOf("/") + 1, strFilename.length());
        } else {
            fileName = strDownloadDir + "/" + strFilename;
        }

        return fileName;
    }

    public static <T> int indexOf(T[] array, T s) {
        for (int i = 0; i < array.length; i++) {
            if (array[i].equals(s)) {
                return i;
            }
        }
        return -1;
    }

    public static void closeSilently(Closeable c) {
        if (c == null)
            return;
        try {
            c.close();
        } catch (Throwable t) {
            // do nothing
        }
    }

    public static void closeSilently(ParcelFileDescriptor c) {
        if (c == null)
            return;
        try {
            c.close();
        } catch (Throwable t) {
            // do nothing
        }
    }

    public static void Assert(boolean cond) {
        if (!cond) {
            throw new AssertionError();
        }
    }

    public static void startBackgroundJob(final Activity activity,
            final String title, final String message, final Runnable job, final Handler handler) {
        // Make the progress dialog uncancelable, so that we can gurantee
        // the thread will be done before the activity getting destroyed.
        new AsyncTask<Void, Void, Void> () {
            private ProgressDialog mDialog;
            @Override
            protected void onPreExecute() {
                mDialog = showProgressDialog(activity, title, message, false, false);
            }

            @Override
            protected void onPostExecute(Void result) {
                // SPRD: add fix the bug 558976 not attached to window manager when activity is destroyed
                if (activity.isDestroyed()) {
                    return;
                }
                if (mDialog != null && mDialog.getWindow() != null) {
                    mDialog.dismiss();
                }
                handler.sendEmptyMessage(ImageEditConstants.ACTION_PREVIEW);
            }
            @Override
            protected Void doInBackground(Void ... args) {
                if (job != null) {
                    job.run();
                }
                return null;
            }
        }.execute();
    }

    private static ProgressDialog showProgressDialog(
            Activity activity, String title, String message, boolean indeterminate, boolean flag) {
        /* SPRD: Fix bug 559526 there may be leaked window @{ */
        if (activity.isDestroyed()) {
            return null;
        }
        /* @} */
        ProgressDialog dialog = new ProgressDialog(activity);
        dialog.requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
//        dialog.setTitle(title);
        dialog.setMessage(message);
        dialog.setIndeterminate(indeterminate);
        dialog.setCancelable(flag);
        dialog.show();
        if(Models.getModel().equals(Models.AMAZON_KFTT)) {
            dialog.getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        }
        return dialog;
    }

    public static void showToast(Context context,  int resId, int duration){
        Log.d(TAG, "showToast(): showToast, come from resId");
        Toast.makeText(context, resId, duration).show();
    }

    public static void showToast(Context context, CharSequence text, int duration){
        Log.d(TAG, "showToast(): showToast, come from text");
        Toast.makeText(context, text, duration).show();
    }

    public static void showHandlerToast(final Context context, final int resId, final int duration) {
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            public void run() {
                Toast.makeText(context, resId, duration).show();
            }
        });
    }

    public static String getStoragePath(Context context, String filePath) {
        String targetPath = null;
        SharedPreferences pref = DreamSettingUtil.openPreferences(
                context, DataStoragePath.PRE_CAMERA_CATEGORY_PUBLIC_SETTING);
        targetPath = pref.getString("pref_camera_storage_path",
                ImageManager.getCameraUCamPath());

        if (KEY_DEFAULT_INTERNAL.equals(targetPath)) {
            targetPath = INTERNALDIR;
        } else if (KEY_DEFAULT_EXTERNAL.equals(targetPath)) {
            targetPath = EXTERNALDIR;
        } else {
            targetPath = INTERNALDIR;
        }
        return targetPath;
    }

    public static Pair<Uri, String> saveOutput(Context context, Bitmap bitmap,
                                               String filePath, String fileName) {
        String targetPath = getStoragePath(context, filePath);
        Log.d(TAG, "saveOutput(): filePath is " + filePath
                + ", fileName is " + fileName
                + ", targetPath is " + targetPath);
        /*
         * try to delete exist record in Media database
         */
//        Uri editUri = ImageEditDesc.getInstance().getEditImageUri();

//        if( editUri != null ){
//            context.getContentResolver().delete(editUri, null, null);
//        }
        String title = null;
        if (fileName.lastIndexOf(".") > 0) {
            title = fileName.substring(0, fileName.lastIndexOf("."));
        }
        final long dateTaken = System.currentTimeMillis();
        boolean is24Hour = DateFormat.is24HourFormat(context);
        Date date = new Date(dateTaken);
        SimpleDateFormat dateFormat = is24Hour ? new SimpleDateFormat("yyyy:MM:dd HH:mm:ss") :
                new SimpleDateFormat("yyyy:MM:dd h:mm:ss a");
        Uri originalUri = ImageEditDesc.getInstance().getOriginalUri();

        Uri newUri = ImageManager.addImage(context.getContentResolver(),
                title,
                dateTaken,
                null,
                targetPath,
                fileName,
                null,
                transformBitmapToBuffer(bitmap),
                new int[]{0},
                true,
                originalUri
        );
        /*
         * FIX BUG: 6100
         * BUG COMMENT: remove last thumbnail of file dir after save image in uphoto
         * DATE: 2014-03-13
         */
        File file = new File(context.getFilesDir(), "last_image_thumb");
        if (file.exists() && file.isFile()) {
            file.delete();
        }

        if (newUri != null) {
            final String saveFullPath = targetPath + "/" + fileName;
            ImageEditOperationUtil.setExifAttribute(saveFullPath,
                    ExifInterface.TAG_DATETIME,
                    dateFormat.format(date));
            return Pair.create(newUri, saveFullPath);
        }
        return null;
    }

    public static void setExifAttribute(String fileName, String tag, String value){
        try {
            ExifInterface exif = new ExifInterface(fileName);
            exif.setAttribute(tag, value);
            exif.saveAttributes();
        } catch (IOException e) {
            Log.w(TAG, "setExifAttribute(): the exif is falded to read, fileName = " + fileName);
        }
    }

    /**
     * Compute the sample size to reduce the image size
     * @param options BitmapFactory.Options
     * @param newWidth target scale width
     * @param newHeight target scale height
     * @return reduce the image size parameter
     */
    public static int computeSampleSize(BitmapFactory.Options options, int newWidth, int newHeight) {
        int inSampleSize  = 1;
        int imageWidth = options.outWidth;
        int imageHeight = options.outHeight;

        if(newWidth >= imageHeight && newHeight >= imageWidth) {
            return inSampleSize;
        }

        while(imageWidth / inSampleSize > newWidth || imageHeight / inSampleSize > newHeight) {
            inSampleSize *= 2;
        }

        if(inSampleSize > 1) {
            return inSampleSize / 2;
        }

        return inSampleSize;
    }

    public static int computeBitmapSampleSize(BitmapFactory.Options options, int newWidth, int newHeight){
        int sampleSize  = 1;
        int bitmapWidth = options.outWidth;
        int bitmapHeight = options.outHeight;

        while (newWidth < (bitmapWidth / sampleSize) || newHeight < (bitmapHeight / sampleSize) ) {
            sampleSize <<= 1;
        }
        return sampleSize;
    }

    /**
     * scale suitable bitmap with target dimens
     *
     * @param tempBitmap need to be scaled bitmap that is less than original bitmap
     * @param options options BitmapFactory.Options
     * @param newWidth target scale width
     * @param newHeight target scale height
     * @return target bitmap
     */
    public static  Bitmap computeSuitableBitmap(Bitmap tempBitmap, BitmapFactory.Options options, int newWidth, int newHeight) {
        float imageWidth = options.outWidth;
        float imageHeight = options.outHeight;

        if(imageWidth <= newWidth && imageHeight <= newHeight) {
            return tempBitmap;
        }

        float scaleW = newWidth / imageWidth;
        float scaleH = newHeight / imageHeight;
        float scaleFactor = scaleW;
        if(scaleW > scaleH) {
            scaleFactor = scaleH;
        }

        int dstWidth = (int) (imageWidth * scaleFactor);
        int dstHeight = (int) (imageHeight * scaleFactor);

        Bitmap newBitmap = Bitmap.createScaledBitmap(tempBitmap, dstWidth, dstHeight, false);
        tempBitmap.recycle();
        return newBitmap;
    }

    /*
     * FIX BUG: 4215
     * ADD COMMENT:create new bitmap according to the config
     * DATE: 2013-06-20
     */
    public static Bitmap createBitmapFromConfig(Bitmap src,Bitmap.Config dstConfig){
        Bitmap.Config srcConfig=src.getConfig();
        if(dstConfig==srcConfig){
            return src;
        }
        int width=src.getWidth();
        int height=src.getHeight();
        Bitmap dst=Bitmap.createBitmap(width, height, dstConfig);
        Canvas canvas=new Canvas();
        canvas.setBitmap(dst);
        canvas.drawBitmap(src, 0, 0, null);
        return dst;
    }
}
