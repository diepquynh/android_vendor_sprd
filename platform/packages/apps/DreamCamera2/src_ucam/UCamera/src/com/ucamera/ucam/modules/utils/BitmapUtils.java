/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 *
 * Copyright (C) 2010 The Android Open Source Project
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

package com.ucamera.ucam.modules.utils;

import java.io.ByteArrayOutputStream;
import java.lang.reflect.Method;

import com.android.camera.CameraActivity;
import com.ucamera.ucam.modules.compatible.Models;

import java.io.InputStream;
import java.io.IOException;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.hardware.Camera.CameraInfo;
import android.net.Uri;
import android.os.Build;
import android.provider.MediaStore.Images.Media;
import android.util.FloatMath;
import android.util.Log;
import android.view.Surface;

public abstract class BitmapUtils {
    private static final String TAG = "BitmapUtils";

    private BitmapUtils(){}

    public static void recycleSilently(Bitmap bitmap) {
        if (bitmap == null) return;
        try {
            bitmap.recycle();
        } catch (Throwable t) {
            // ignore
        }
    }

    private static Bitmap.Config getConfig(Bitmap bitmap) {
        Bitmap.Config config = bitmap.getConfig();
        if (config == null) {
            config = Bitmap.Config.ARGB_8888;
        }
        return config;
    }


    /*
     * Compute the sample size as a function of minSideLength
     * and maxNumOfPixels.
     * minSideLength is used to specify that minimal width or height of a
     * bitmap.
     * maxNumOfPixels is used to specify the maximal size in pixels that is
     * tolerable in terms of memory usage.
     *
     * The function returns a sample size based on the constraints.
     * Both size and minSideLength can be passed in as -1
     * which indicates no care of the corresponding constraint.
     * The functions prefers returning a sample size that
     * generates a smaller bitmap, unless minSideLength = -1.
     *
     * Also, the function rounds up the sample size to a power of 2 or multiple
     * of 8 because BitmapFactory only honors sample size this way.
     * For example, BitmapFactory downsamples an image by 2 even though the
     * request is 3. So we round up the sample size to avoid OOM.
     */
    public static int computeSampleSize(BitmapFactory.Options options,
            int minSideLength, int maxNumOfPixels) {
        int initialSize = computeInitialSampleSize(options, minSideLength,
                maxNumOfPixels);

        int roundedSize;
        if (initialSize <= 8) {
            roundedSize = 1;
            while (roundedSize < initialSize) {
                roundedSize <<= 1;
            }
        } else {
            roundedSize = (initialSize + 7) / 8 * 8;
        }

        return roundedSize;
    }

    private static int computeInitialSampleSize(BitmapFactory.Options options,
            int minSideLength, int maxNumOfPixels) {
        double w = options.outWidth;
        double h = options.outHeight;

        int lowerBound = (maxNumOfPixels < 0) ? 1 :
                (int) Math.ceil(Math.sqrt(w * h / maxNumOfPixels));
        int upperBound = (minSideLength < 0) ? 128 :
                (int) Math.min(Math.floor(w / minSideLength),
                Math.floor(h / minSideLength));

        if (upperBound < lowerBound) {
            // return the larger one when there is no overlapping zone.
            return lowerBound;
        }

        if (maxNumOfPixels < 0 && minSideLength < 0) {
            return 1;
        } else if (minSideLength < 0) {
            return lowerBound;
        } else {
            return upperBound;
        }
    }

    /*
     * Compute the sample size as a function of minSideLength
     * and maxNumOfPixels.
     * minSideLength is used to specify that minimal width or height of a
     * bitmap.
     * maxNumOfPixels is used to specify the maximal size in pixels that is
     * tolerable in terms of memory usage.
     *
     * The function returns a sample size based on the constraints.
     * Both size and minSideLength can be passed in as UNCONSTRAINED,
     * which indicates no care of the corresponding constraint.
     * The functions prefers returning a sample size that
     * generates a smaller bitmap, unless minSideLength = UNCONSTRAINED.
     *
     * Also, the function rounds up the sample size to a power of 2 or multiple
     * of 8 because BitmapFactory only honors sample size this way.
     * For example, BitmapFactory downsamples an image by 2 even though the
     * request is 3. So we round up the sample size to avoid OOM.
     */
    public static int computeSampleSize(int width, int height,
            int minSideLength, int maxNumOfPixels) {
        int initialSize = computeInitialSampleSize(
                width, height, minSideLength, maxNumOfPixels);

        return initialSize <= 8
                ? Utils.nextPowerOf2(initialSize)
                : (initialSize + 7) / 8 * 8;
    }

    private static final int UNCONSTRAINED           = -1;
    private static int computeInitialSampleSize(int w, int h,
          int minSideLength, int maxNumOfPixels) {
        if (maxNumOfPixels == UNCONSTRAINED && minSideLength == UNCONSTRAINED) {
           return 1;
        }

        int lowerBound = (maxNumOfPixels == UNCONSTRAINED) ? 1 :
                (int) FloatMath.ceil(FloatMath.sqrt((float) (w * h) / maxNumOfPixels));

        if (minSideLength == UNCONSTRAINED) {
            return lowerBound;
        } else {
            int sampleSize = Math.min(w / minSideLength, h / minSideLength);
            return Math.max(sampleSize, lowerBound);
        }
    }

    // This computes a sample size which makes the longer side at least
    // minSideLength long. If that's not possible, return 1.
    public static int computeSampleSizeLarger(int w, int h, int minSideLength) {
        int initialSize = Math.max(w / minSideLength, h / minSideLength);
        if (initialSize <= 1) return 1;

        return initialSize <= 8
                ? Utils.prevPowerOf2(initialSize)
                : initialSize / 8 * 8;
    }

    // Find the min x that 1 / x >= scale
    public static int computeSampleSizeLarger(float scale) {
        int initialSize = (int) FloatMath.floor(1f / scale);
        if (initialSize <= 1) return 1;

        return initialSize <= 8
                ? Utils.prevPowerOf2(initialSize)
                : initialSize / 8 * 8;
    }

    // Find the max x that 1 / x <= scale.
    public static int computeSampleSize(float scale) {
        Utils.assertTrue(scale > 0);
        int initialSize = Math.max(1, (int) FloatMath.ceil(1 / scale));
        return initialSize <= 8
                ? Utils.nextPowerOf2(initialSize)
                : (initialSize + 7) / 8 * 8;
    }

    public static Bitmap resizeBitmapByScale(
            Bitmap bitmap, float scale, boolean recycle) {
        int width = Math.round(bitmap.getWidth() * scale);
        int height = Math.round(bitmap.getHeight() * scale);
        if (width == bitmap.getWidth()
                && height == bitmap.getHeight()) return bitmap;
        Bitmap target = Bitmap.createBitmap(width, height, getConfig(bitmap));
        Canvas canvas = new Canvas(target);
        canvas.scale(scale, scale);
        Paint paint = new Paint(Paint.FILTER_BITMAP_FLAG | Paint.DITHER_FLAG);
        canvas.drawBitmap(bitmap, 0, 0, paint);
        if (recycle) bitmap.recycle();
        return target;
    }

    public static Bitmap resizeDownBySideLength(
            Bitmap bitmap, int maxLength, boolean recycle) {
        int srcWidth = bitmap.getWidth();
        int srcHeight = bitmap.getHeight();
        float scale = Math.min(
                (float) maxLength / srcWidth, (float) maxLength / srcHeight);
        if (scale >= 1.0f) return bitmap;
        return resizeBitmapByScale(bitmap, scale, recycle);
    }

    public static Bitmap resizeAndCropCenter(Bitmap bitmap, int size, boolean recycle) {
        int w = bitmap.getWidth();
        int h = bitmap.getHeight();
        if (w == size && h == size) return bitmap;

        // scale the image so that the shorter side equals to the target;
        // the longer side will be center-cropped.
        float scale = (float) size / Math.min(w,  h);

        Bitmap target = Bitmap.createBitmap(size, size, getConfig(bitmap));
        int width = Math.round(scale * bitmap.getWidth());
        int height = Math.round(scale * bitmap.getHeight());
        Canvas canvas = new Canvas(target);
        canvas.translate((size - width) / 2f, (size - height) / 2f);
        canvas.scale(scale, scale);
        Paint paint = new Paint(Paint.FILTER_BITMAP_FLAG | Paint.DITHER_FLAG);
        canvas.drawBitmap(bitmap, 0, 0, paint);
        if (recycle) bitmap.recycle();
        return target;
    }


    public static boolean isRotationSupported(String mimeType) {
        if (mimeType == null) return false;
        mimeType = mimeType.toLowerCase();
        return mimeType.equals("image/jpeg");
    }

    // Rotates the bitmap by the specified degree.
    // If a new bitmap is created, the original bitmap is recycled.
    public static Bitmap rotate(Bitmap b, int degrees) {
        return rotateAndMirror(b, degrees, false);
    }

    public static Bitmap rotateBmpToDisplay(CameraActivity activity,Bitmap bmJpeg, int jpegDegree, int cameraid){
        int rotate  = Utils.getPreviewOrientation(activity,cameraid);
        /**
         * FIX BUG:5533
         * BUG CAUSE:get get image is inverted of Ugif
         * DATE: 2013.12.10
         */
        if(jpegDegree != 0){
            rotate = (rotate + jpegDegree + 360) % 360;
        }
        return rotateAndMirror(bmJpeg, rotate, false);
    }

    /* SPRD: Fix bug 595400 the freeze screen for gif @{ */
    public static Bitmap rotateBmpToDisplayandMirror(CameraActivity activity,Bitmap bmJpeg, int jpegDegree, int cameraid){
        int rotate  = Utils.getPreviewOrientation(activity,cameraid);
        /**
         * FIX BUG:5533
         * BUG CAUSE:get get image is inverted of Ugif
         * DATE: 2013.12.10
         */
        if(jpegDegree != 0){
            rotate = (rotate + jpegDegree + 360) % 360;
        }
        return rotateAndMirror(bmJpeg, rotate, true);
    }
    /* @} */

    // Rotates and/or mirrors the bitmap. If a new bitmap is created, the
    // original bitmap is recycled.
    public static Bitmap rotateAndMirror(Bitmap b, int degrees, boolean mirror) {
        if ((degrees != 0 || mirror) && b != null) {
            Matrix m = new Matrix();
            // Mirror first.
            // horizontal flip + rotation = -rotation + horizontal flip
            if (mirror) {
                m.postScale(-1, 1);
                degrees = (degrees + 360) % 360;
                if (degrees == 0 || degrees == 180) {
                    m.postTranslate(b.getWidth(), 0);
                } else if (degrees == 90 || degrees == 270) {
                    m.postTranslate(b.getHeight(), 0);
                } else {
                    throw new IllegalArgumentException("Invalid degrees=" + degrees);
                }
            }
            if (degrees != 0) {
                // clockwise
                m.postRotate(degrees,
                        (float) b.getWidth() / 2, (float) b.getHeight() / 2);
            }

            try {
                Bitmap b2 = Bitmap.createBitmap(
                        b, 0, 0, b.getWidth(), b.getHeight(), m, true);
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

    public static Bitmap createVideoThumbnail(String filePath) {
        // MediaMetadataRetriever is available on API Level 8
        // but is hidden until API Level 10
        Class<?> clazz = null;
        Object instance = null;
        try {
            clazz = Class.forName("android.media.MediaMetadataRetriever");
            instance = clazz.newInstance();

            Method method = clazz.getMethod("setDataSource", String.class);
            method.invoke(instance, filePath);

            // The method name changes between API Level 9 and 10.
            if (Build.VERSION.SDK_INT <= 9) {
                return (Bitmap) clazz.getMethod("captureFrame").invoke(instance);
            } else {
                byte[] data = (byte[]) clazz.getMethod("getEmbeddedPicture").invoke(instance);
                if (data != null) {
                    Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length);
                    if (bitmap != null) return bitmap;
                }
                return (Bitmap) clazz.getMethod("getFrameAtTime").invoke(instance);
            }
        } catch (IllegalArgumentException ex) {
            // Assume this is a corrupt video file
        } catch (RuntimeException ex) {
            // Assume this is a corrupt video file.
        } catch (Exception e) {
            Log.e(TAG, "createVideoThumbnail", e);
        } finally {
            try {
                if (instance != null) {
                    clazz.getMethod("release").invoke(instance);
                }
            } catch (Exception ignored) {
            }
        }
        return null;
    }

    private static final int DEFAULT_JPEG_QUALITY   = 90;
    public static byte[] compressToBytes(Bitmap bitmap) {
        return compressToBytes(bitmap, DEFAULT_JPEG_QUALITY);
    }

    public static byte[] compressToBytes(Bitmap bitmap, int quality) {
        ByteArrayOutputStream baos = new ByteArrayOutputStream(65536);
        bitmap.compress(CompressFormat.JPEG, quality, baos);
        return baos.toByteArray();
    }

    public static boolean isSupportedByRegionDecoder(String mimeType) {
        if (mimeType == null) return false;
        mimeType = mimeType.toLowerCase();
        return mimeType.startsWith("image/")
                && (!mimeType.equals("image/gif")
                && !mimeType.endsWith("bmp"));
    }

    public static Bitmap makeBitmap(byte[] jpegData, int maxNumOfPixels) {
        if(jpegData == null || jpegData.length <=0) {
            return null;
        }
        try {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inJustDecodeBounds = true;
            BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length, options);
            if (options.mCancel || options.outWidth == -1 || options.outHeight == -1) {
                return null;
            }
            options.inSampleSize = computeSampleSize( options, -1, maxNumOfPixels);
            options.inJustDecodeBounds = false;

            options.inDither = false;
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            return BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length, options);
        } catch (OutOfMemoryError ex) {
            Log.e(TAG, "Got oom exception ", ex);
            return null;
        }
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
            Utils.closeSilently(is);
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
                Utils.recycleBitmap(tmp);
            }

            int rotateDegree = getImageDegreeByUri(uri, context);
            if (rotateDegree != 0) {
                tmp = Utils.rotate(bitmap, rotateDegree);
                /*
                 * BUG FIX: 1486
                 * FIX COMMENT: some case bitmap == tmp, so dont recycle
                 * DATE: 2012-08-21
                 */
                if (tmp != bitmap){
                    Utils.recycleBitmap(bitmap);
                    bitmap = tmp;
                }
            }
            return bitmap;
        } catch (Exception e) {
            Log.e("BitmapUtil","Failt create bitmap",e);
            // some error occure
        }finally {
            Utils.closeSilently(is);
        }
        return null;
    }

    private static int getImageDegreeByUri(Uri target, Context context) {
        if (target == null) {
            return 0;
        }
        String filepath = getDefaultPathAccordUri(target, context);
        if (filepath != null) {
            return Utils.getExifOrientation(filepath);
        } else {
            Log.d("BitmapUtil", "The path in uri: " + target + " is null");
            return 0;
        }
    }

    private static String getDefaultPathAccordUri(Uri uri, Context context) {
        String strPath = null;
        if (ContentResolver.SCHEME_FILE.equals(uri.getScheme())) {
            strPath = uri.getPath();
        } else if (ContentResolver.SCHEME_CONTENT.equals(uri.getScheme())) {
            final String[] IMAGE_PROJECTION = new String[] {Media.DATA};
            Cursor cr = context.getContentResolver().query(uri, IMAGE_PROJECTION, null, null, null);
            /* SPRD:  CID 108009 : Resource leak (RESOURCE_LEAK) @{ */
            if(cr != null ) {
                if(cr.getCount() > 0 && cr.isBeforeFirst()) {
                    cr.moveToFirst();
                    strPath = cr.getString(cr.getColumnIndex(Media.DATA));
                }
                cr.close();
            }
            /**
            if(cr != null && cr.getCount() > 0) {
                if(cr.isBeforeFirst()) {
                    cr.moveToFirst();
                    strPath = cr.getString(cr.getColumnIndex(Media.DATA));
                }
                cr.close();
            }
            */
            /* @} */
        }
        return strPath;
    }
}
