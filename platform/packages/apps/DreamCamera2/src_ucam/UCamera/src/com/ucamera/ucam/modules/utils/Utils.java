/*
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.ucam.modules.utils;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Point;
import android.graphics.PorterDuffXfermode;
import android.graphics.PorterDuff.Mode;
import android.content.res.TypedArray;
import android.content.res.Resources;
import android.content.ContentResolver;
import android.database.Cursor;
import android.view.Display;
import android.view.Surface;
import android.view.OrientationEventListener;
import android.net.Uri;


import com.ucamera.ucam.modules.compatible.ResolutionSize;
import com.ucamera.ucam.modules.utils.LogUtils;
import com.android.ex.camera2.portability.CameraDeviceInfo.Characteristics;
import com.android.ex.camera2.portability.Size;
import com.android.camera.CameraActivity;
import android.hardware.Camera.CameraInfo;

import android.media.ExifInterface;

import java.io.ByteArrayOutputStream;
import java.io.Closeable;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.lang.reflect.Field;

public class Utils {
    private final static String TAG = "Utils";

    public static void closeSilently(Closeable c) {
        if (c == null)
            return;
        try {
            c.close();
        } catch (Throwable t) {
            // do nothing
        }
    }

    public static void recycleBitmap(Bitmap bitmap) {
        if (bitmap != null && !bitmap.isRecycled()) {
            bitmap.recycle();
        }
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
                LogUtils.error(TAG, "OutOfMemoryError "+ex);
                // We have no memory to rotate. Return the original bitmap.
            }
        }
        return b;
    }

    public static boolean calcNeedRevert(int cameraid) {
        boolean needRevert = false;
        int rotation = 0;
        try {
            CameraInfo info = getCameraInfoOrientation(cameraid);
            rotation = info.orientation;
        } catch (ArrayIndexOutOfBoundsException e) {
            e.printStackTrace();
            return needRevert;
        }

        needRevert = (rotation % 180 == 0);
        LogUtils.debug("Utils", "cameraId:%d, rotation:%d, calcNeedRevert:%b", cameraid, rotation, needRevert);
        return needRevert;
    }

    private static CameraInfo getCameraInfoOrientation(int cameraId) {
        CameraInfo info = new android.hardware.Camera.CameraInfo();
        android.hardware.Camera.getCameraInfo(cameraId, info);
        return info;
    }

    public static int getJpegRotation(int cameraId, int orientation, boolean mirror) {
        // See android.hardware.Camera.Parameters.setRotation for
        // documentation.
        int rotation = 0;
        if (orientation != OrientationEventListener.ORIENTATION_UNKNOWN) {
            CameraInfo info = getCameraInfoOrientation(cameraId);
            if (info.facing == CameraInfo.CAMERA_FACING_FRONT) {
                rotation = (info.orientation - orientation + 360) % 360;
            } else { // back-facing camera
                rotation = (info.orientation + orientation + 360) % 360;
            }
        }
        return rotation;
    }

    public static BitmapFactory.Options getNativeAllocOptions() {
        BitmapFactory.Options options = new BitmapFactory.Options();
        // options.inNativeAlloc = true;
        options.inInputShareable = true;
        options.inPurgeable = true;
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;
        return options;
    }

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

    public static int parseIntSafely(String content, int defaultValue) {
        if (content == null)
            return defaultValue;
        try {
            return Integer.parseInt(content);
        } catch (NumberFormatException e) {
            return defaultValue;
        }
    }

    public static List<ResolutionSize> filterSizeItem(List<ResolutionSize> sizes,
            ResolutionSize min, ResolutionSize max) {
        List<ResolutionSize> sizeList = new ArrayList<ResolutionSize>();
        if (sizes == null || sizes.size() == 0) {
            return sizeList;
        }

        for (ResolutionSize size : sizes) {
            int w = size.width;
            int h = size.height;

            if (w >= min.width && w <= max.width && h >= min.height && h <= max.height) {
                sizeList.add(size);
            }
        }
        return sizeList;
    }

    public static List<Size> filterSizeItem(List<Size> sizes,
            Size min, Size max) {
        List<Size> sizeList = new ArrayList<Size>();
        if (sizes == null || sizes.size() == 0) {
            return sizeList;
        }

        for (Size size : sizes) {
            int w = size.width();
            int h = size.height();

            if (w >= min.width() && w <= max.width() && h >= min.height() && h <= max.height()) {
                sizeList.add(size);
            }
        }
        return sizeList;
    }

    private static ResolutionSize getOptimalSize(ResolutionSize size,int relative_w,int relative_h) {
        /*
         * FIX BUG: 6048
         * BUG COMMENT: java.lang.ArithmeticException: divide by zero
         * DATE: 2014-03-04
         */
        int width = size != null ? size.width : 0;
        int height = size != null ? size.height : 0;

        if(width == 0 || height == 0) {
            return null;
        }
        int gcd = gcd(width, height);
        if(gcd == 0) {
            return null;
        }
        int relativeW = width / gcd;
        int relativeH = height / gcd;

        // if the reduction is too large, we will round it and recalculate
        while (relativeW > 20) {
            int unit;
            if (relativeW >= 100) {
                unit = 10;
            } else {
                unit = 4;
            }
            relativeW = roundby(relativeW, unit);
            relativeH = roundby(relativeH, unit);
            gcd = gcd(relativeW, relativeH);
            relativeW = relativeW / gcd;
            relativeH = relativeH / gcd;
        }

        if (width < height) {
            int tmp = relativeW;
            relativeW = relativeH;
            relativeH = tmp;
        }

        if (relativeW != relative_w || relativeH != relative_h) {
            size = null;
        }
        return size;
    }

    public static Size getOptimalSize(Size size,int relative_w,int relative_h) {
        /*
         * FIX BUG: 6048
         * BUG COMMENT: java.lang.ArithmeticException: divide by zero
         * DATE: 2014-03-04
         */
        int width = size != null ? size.width() : 0;
        int height = size != null ? size.height() : 0;

        if(width == 0 || height == 0) {
            return null;
        }
        int gcd = gcd(width, height);
        if(gcd == 0) {
            return null;
        }
        int relativeW = width / gcd;
        int relativeH = height / gcd;

        // if the reduction is too large, we will round it and recalculate
        while (relativeW > 20) {
            int unit;
            if (relativeW >= 100) {
                unit = 10;
            } else {
                unit = 4;
            }
            relativeW = roundby(relativeW, unit);
            relativeH = roundby(relativeH, unit);
            gcd = gcd(relativeW, relativeH);
            relativeW = relativeW / gcd;
            relativeH = relativeH / gcd;
        }

        if (width < height) {
            int tmp = relativeW;
            relativeW = relativeH;
            relativeH = tmp;
        }

        if (relativeW != relative_w || relativeH != relative_h) {
            size = null;
        }
        return size;
    }

    // round num by unit
    public static int roundby(int num, int unit) {
        int reminder = num % unit;
        int halfunit = unit / 2;

        if (reminder >= halfunit) {
            num = (num / unit + 1) * unit;
        } else {
            num = (num / unit) * unit;
        }
        return num;
    }

    // get the greatest common divider
    public static int gcd(int a, int b) {
        int c = 0;
        // guard a > b
        if (a < b) {
            c = a;
            a = b;
            b = c;
        }

        /*
         * FIX BUG: 5958
         * BUG COMMENT: java.lang.ArithmeticException: divide by zero
         * DATE: 2014-02-21
         */
        if(b == 0) {
            return a;
        }
        c = a % b;
        if (c == 0) {
            return b;
        } else {
            return gcd(b, c);
        }
    }

    public static Bitmap roundRectBitmap(Bitmap inBitmap, float r) {
        int width = inBitmap.getWidth();
        int height = inBitmap.getHeight();
        Bitmap retBitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(retBitmap);

        Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        canvas.drawRoundRect(new RectF(0, 0, width, height), r, r, paint);

        paint.setXfermode(new PorterDuffXfermode(Mode.SRC_IN));
        canvas.drawBitmap(inBitmap, 0, 0, paint);

        return retBitmap;
    }

    public static int getExifOrientation(String filepath) {
        int degree = 0;
        ExifInterface exif = null;
        try {
            exif = new ExifInterface(filepath);
        } catch (IOException ex) {
            LogUtils.debug("Utils", "cannot read exif", ex);
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

    public static int getPreviewOrientation(CameraActivity activity,int cameraId) {
        int degrees = 0;
        int result = 0;
        degrees = getDisplayRotation(activity);

        if (activity.getCameraProvider() == null) {
            return result;
        }
        Characteristics info =
                activity.getCameraProvider().getCharacteristics(cameraId);
        result = info.getPreviewOrientation(degrees);
        return result;
    }

    public static int getDisplayRotation(Activity activity) {
        int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();
        switch (rotation) {
        case Surface.ROTATION_0:
            return 0;
        case Surface.ROTATION_90:
            return 90;
        case Surface.ROTATION_180:
            return 180;
        case Surface.ROTATION_270:
            return 270;
        }
        return 0;
    }

    public static int nextPowerOf2(int n) {
        if (n <= 0 || n > (1 << 30)) {
            throw new IllegalArgumentException("n is invalid: " + n);
        }
        n -= 1;
        n |= n >> 16;
        n |= n >> 8;
        n |= n >> 4;
        n |= n >> 2;
        n |= n >> 1;
        return n + 1;
    }

    public static void assertTrue(boolean cond) {
        if (!cond) {
            throw new AssertionError();
        }
    }

    public static int prevPowerOf2(int n) {
        if (n <= 0)
            throw new IllegalArgumentException();
        return Integer.highestOneBit(n);
    }

    /**
     * recyle bitmaps
     * @param bitmaps which needed to recyled
     */
    public static void recycleBitmaps(Bitmap[] bitmaps){
        if (bitmaps != null) {
            for (int i = 0; i < bitmaps.length; i++) {
                recycleBitmap(bitmaps[i]);
            }
        }
    }

    public static Bitmap transformBufferToBitmap(byte[] jpegData) {
        Bitmap bitmap = null;
        try {
            BitmapFactory.Options options = Utils.getNativeAllocOptions();
            bitmap = BitmapFactory.decodeByteArray(jpegData, 0, jpegData.length, options);
        } catch (OutOfMemoryError ex) {
            // We have no memory to rotate. Return the original bitmap.
            bitmap = null;
            System.gc();
        }
        return bitmap;
    }

    public static byte[] transformBitmapToBuffer(Bitmap bitmap) {

        /*
         * BUGFIX: 1010 1273 FIX COMMENT: if bitmap is null or recycled, then
         * just return DATE: 2012-05-18 2012-07-15
         */
        if (bitmap == null || bitmap.isRecycled())
            return null;
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

 // get strings from array.xml
    public static String[] getIconStrings(Resources res, int iconsRes) {
        if (iconsRes == 0)
            return null;
        TypedArray array = res.obtainTypedArray(iconsRes);
        int n = array.length();
        String ids[] = new String[n];
        for (int i = 0; i < n; ++i) {
            ids[i] = res.getString(array.getResourceId(i, 0));
        }
        array.recycle();
        return ids;
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

    public static ArrayList<Integer> getIconValues(Resources res, int iconsRes) {
        if (iconsRes == 0)
            return null;
        ArrayList<Integer> ids = new ArrayList<Integer>();
        TypedArray array = res.obtainTypedArray(iconsRes);
        int n = array.length();
        for (int i = 0; i < n; ++i) {
            ids.add(array.getInt(i, 0));
        }
        array.recycle();
        return ids;
    }

    public static String getFilePathByUri(Uri uri, ContentResolver cr) {
        Cursor cursor = cr.query(uri, null, null, null, null);
        try {
            if (cursor == null || cursor.getCount() == 0 || cursor.getColumnCount() < 2) {
                return null;
            }
            cursor.moveToFirst();
            String name = cursor.getString(1);
            return name;
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
    }

    public static ResolutionSize getGifPreviewSize(List<ResolutionSize> supportedSizes) {
        List<ResolutionSize> only43List = new ArrayList<ResolutionSize>();
        /* SPRD:Fix bug The var is not sentenced to empty, resulting in a null pointer @{ */
        if (supportedSizes == null) {
            return null;
        }
        /* @} */
        for (ResolutionSize size : supportedSizes) {
            if (getOptimalSize(size,4,3) != null) {
                only43List.add(size);
            }
        }
        ResolutionSize previewSize = null;
        if (only43List.size() > 0) {
            ResolutionSize maxSize = new ResolutionSize(640, 480);
            ResolutionSize minSize = new ResolutionSize(320, 240);
            List<ResolutionSize> selectedList = filterSizeItem(only43List, minSize, maxSize);
            if (selectedList.size() > 0) {
                previewSize = selectedList.get(0);
            }
        }
        return previewSize;
    }

    public static ResolutionSize getOptimalPreviewSize(Activity currentActivity,
            List<ResolutionSize> sizes, double targetRatio) {
        // Use a very small tolerance because we want an exact match.
        final double ASPECT_TOLERANCE = 0.001;
        if (sizes == null)
            return null;

        ResolutionSize optimalSize = null;
        double minDiff = Double.MAX_VALUE;

        // Because of bugs of overlay and layout, we sometimes will try to
        // layout the viewfinder in the portrait orientation and thus get the
        // wrong size of preview surface. When we change the preview size, the
        // new overlay will be created before the old one closed, which causes
        // an exception. For now, just get the screen size.
        Point point = getDefaultDisplaySize(currentActivity, new Point());
        int targetHeight = Math.min(point.x, point.y);
        // Try to find an size match aspect ratio and size
        for (ResolutionSize size : sizes) {
            double ratio = (double) size.width / size.height;
            if (Math.abs(ratio - targetRatio) > ASPECT_TOLERANCE)
                continue;
            if (Math.abs(size.height - targetHeight) < minDiff) {
                optimalSize = size;
                minDiff = Math.abs(size.height - targetHeight);
            }
        }
        // Cannot find the one match the aspect ratio. This should not happen.
        // Ignore the requirement.
        if (optimalSize == null) {
            minDiff = Double.MAX_VALUE;
            for (ResolutionSize size : sizes) {
                if (Math.abs(size.height - targetHeight) < minDiff) {
                    optimalSize = size;
                    minDiff = Math.abs(size.height - targetHeight);
                }
            }
        }
        return optimalSize;
    }

    private static Point getDefaultDisplaySize(Activity activity, Point size) {
        Display d = activity.getWindowManager().getDefaultDisplay();
        d.getSize(size);
        return size;
    }

    public static boolean contains(List<ResolutionSize> sizes,ResolutionSize size){
        if(sizes == null || size == null) return false;
        for(ResolutionSize list : sizes){
            if(list.width == size.width && list.height == size.height){
                return true;
            }
        }
        return false;
    }

    public static List<ResolutionSize> getOrderedSizes(List<ResolutionSize> stringarrayList) {
        ResolutionSize[] stringarray = stringarrayList.toArray(new ResolutionSize[] {});
        // Sort DESC
        Arrays.sort(stringarray, new Comparator<ResolutionSize>() {
            @Override
            public int compare(ResolutionSize o1, ResolutionSize o2) {
                return o2.compareTo(o1);
            }
        });
        return Arrays.asList(stringarray);
    }

    public static int getIntFieldIfExists(Class<?> klass, String fieldName,
            Object obj, int defaultVal) {
        try {
            Field f = klass.getDeclaredField(fieldName);
            return f.getInt(obj);
        } catch (Exception e) {
            return defaultVal;
        }
    }

    public static final int ORIENTATION_HYSTERESIS = 5;

    public static int roundOrientation(int orientation, int orientationHistory) {
        boolean changeOrientation = false;
        if (orientationHistory == OrientationEventListener.ORIENTATION_UNKNOWN) {
            changeOrientation = true;
        } else {
            int dist = Math.abs(orientation - orientationHistory);
            dist = Math.min(dist, Math.abs(360 - dist));
            changeOrientation = (dist >= 45 + ORIENTATION_HYSTERESIS);
        }
        if (changeOrientation) {
            return ((orientation + 45) / 90 * 90) % 360;
        }
        return orientationHistory;
    }

}
