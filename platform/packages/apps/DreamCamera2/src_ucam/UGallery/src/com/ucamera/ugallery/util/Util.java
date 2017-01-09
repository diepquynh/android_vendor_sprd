/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.ucamera.ugallery.util;

import android.app.Activity;
import android.app.ActivityManager;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.Window;
import android.widget.TextView;
import android.widget.Toast;

import com.ucamera.ugallery.ImageListSpecial;
import com.ucamera.ugallery.MonitoredActivity;
import com.ucamera.ugallery.UGalleryConst;
import com.ucamera.ugallery.ViewImage;
import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.privateimage.util.Constants;

import android.content.pm.PackageManager;
import android.database.Cursor;

import java.io.Closeable;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.List;

import android.media.MediaMetadataRetriever;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.ParcelFileDescriptor;
import android.preference.PreferenceManager;
import android.provider.MediaStore.Images.ImageColumns;
import com.ucamera.ugallery.R;


/**
 * Collection of utility functions used in this package.
 */
public class Util {
    private static final String TAG = "Util";
    public static final int DIRECTION_LEFT = 0;
    public static final int DIRECTION_RIGHT = 1;
    public static final String REVIEW_ACTION = "com.cooliris.media.action.REVIEW";
    public static final String VIEW_ACTION = Intent.ACTION_VIEW;
    private final static String PACKAGE_NAME="com.ucamera.ugallery";
    // Whether we should recycle the input (unless the output is the input).
    public static final boolean RECYCLE_INPUT = true;
    public static final boolean NO_RECYCLE_INPUT = false;

    private Util() {
    }

    // get strings from array.xml

 // get strings from array.xml

    /*
     * Compute the sample size as a function of minSideLength
     * and maxNumOfPixels.
     * minSideLength is used to specify that minimal width or height of a
     * bitmap.
     * maxNumOfPixels is used to specify the maximal size in pixels that is
     * tolerable in terms of memory usage.
     *
     * The function returns a sample size based on the constraints.
     * Both size and minSideLength can be passed in as IImage.UNCONSTRAINED,
     * which indicates no care of the corresponding constraint.
     * The functions prefers returning a sample size that
     * generates a smaller bitmap, unless minSideLength = IImage.UNCONSTRAINED.
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

        int lowerBound = (maxNumOfPixels == IImage.UNCONSTRAINED) ? 1 :
                (int) Math.ceil(Math.sqrt(w * h / maxNumOfPixels));
        int upperBound = (minSideLength == IImage.UNCONSTRAINED) ? 128 :
                (int) Math.min(Math.floor(w / minSideLength),
                Math.floor(h / minSideLength));

        if (upperBound < lowerBound) {
            // return the larger one when there is no overlapping zone.
            return lowerBound;
        }

        if ((maxNumOfPixels == IImage.UNCONSTRAINED) &&
                (minSideLength == IImage.UNCONSTRAINED)) {
            return 1;
        } else if (minSideLength == IImage.UNCONSTRAINED) {
            return lowerBound;
        } else {
            return upperBound;
        }
    }

    public static <T>  int indexOf(T [] array, T s) {
        for (int i = 0; i < array.length; i++) {
            if (array[i].equals(s)) {
                return i;
            }
        }
        return -1;
    }

    public static void closeSilently(Closeable c) {
        if (c == null) return;
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

    public static boolean equals(String a, String b) {
        // return true if both string are null or the content equals
        return a == b || a.equals(b);
    }

    public static BitmapFactory.Options getNativeAllocOptions() {
        BitmapFactory.Options options = new BitmapFactory.Options();
        //options.inNativeAlloc = true;
        options.inInputShareable = true;
        options.inPurgeable = true;
        options.inPreferredConfig = Bitmap.Config.ARGB_8888;
        return options;
    }

    public static void recyleBitmap(Bitmap bitmap){
        if (bitmap != null && !bitmap.isRecycled()) {
            bitmap.recycle();
        }
    }

    /**
     * recyle bitmaps
     * @param bitmaps which needed to recyled
     */
    public static void recyleBitmaps(Bitmap[] bitmaps){
        if (bitmaps != null) {
            for (int i = 0; i < bitmaps.length; i++) {
                recyleBitmap(bitmaps[i]);
            }
        }
    }

    public static String getPackageVersion(Context context){
        String version = "";
        try{
            version = context.getPackageManager().getPackageInfo(PACKAGE_NAME, 0).versionName;
        }
        catch (PackageManager.NameNotFoundException e) {
            // do nothing
        }
        return version;
    }

    // Rotates the bitmap by the specified degree.
    // If a new bitmap is created, the original bitmap is recycled.
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

    public static Bitmap transform(Matrix scaler, Bitmap source, int targetWidth, int targetHeight,
            boolean scaleUp, boolean recycle) {
        if(source == null) {
            return null;
        }
        int deltaX = source.getWidth() - targetWidth;
        int deltaY = source.getHeight() - targetHeight;
        if (!scaleUp && (deltaX < 0 || deltaY < 0)) {
            /*
             * In this case the bitmap is smaller, at least in one dimension,
             * than the target. Transform it by placing as much of the image as
             * possible into the target and leaving the top/bottom or left/right
             * (or both) black.
             */
            Bitmap b2 = Bitmap.createBitmap(targetWidth, targetHeight, Bitmap.Config.ARGB_8888);
            Canvas c = new Canvas(b2);

            int deltaXHalf = Math.max(0, deltaX / 2);
            int deltaYHalf = Math.max(0, deltaY / 2);
            Rect src = new Rect(deltaXHalf, deltaYHalf, deltaXHalf
                    + Math.min(targetWidth, source.getWidth()), deltaYHalf
                    + Math.min(targetHeight, source.getHeight()));
            int dstX = (targetWidth - src.width()) / 2;
            int dstY = (targetHeight - src.height()) / 2;
            Rect dst = new Rect(dstX, dstY, targetWidth - dstX, targetHeight - dstY);
            c.drawBitmap(source, src, dst, null);
            if (recycle) {
                source.recycle();
            }
            return b2;
        }
        float bitmapWidthF = source.getWidth();
        float bitmapHeightF = source.getHeight();

        float bitmapAspect = bitmapWidthF / bitmapHeightF;
        float viewAspect = (float) targetWidth / targetHeight;

        if (bitmapAspect > viewAspect) {
            float scale = targetHeight / bitmapHeightF;
            if (scale < .9F || scale > 1F) {
                scaler.setScale(scale, scale);
            } else {
                scaler = null;
            }
        } else {
            float scale = targetWidth / bitmapWidthF;
            if (scale < .9F || scale > 1F) {
                scaler.setScale(scale, scale);
            } else {
                scaler = null;
            }
        }

        /*
         * FIX BUG: 1452
         * BUG CAUSE: May be OOM caused a null pointer exception;
         * FIX COMMENT: Catch OOM and avoid null pointer exception;
         * DATE: 2012-08-13
         */
        Bitmap b1 = null;
        if (scaler != null) {
            try {
            // this is used for minithumb and crop, so we want to filter here.
                b1 = Bitmap.createBitmap(source, 0, 0, source.getWidth(), source.getHeight(), scaler, true);
            } catch(OutOfMemoryError ex) {
                android.util.Log.w(TAG, "transform(): create bitmap oom", ex);
            }
        } else {
            b1 = source;
        }

        if (recycle && b1 != source) {
            source.recycle();
        }

        if(b1 == null) {
            return null;
        }
        int dx1 = Math.max(0, b1.getWidth() - targetWidth);
        int dy1 = Math.max(0, b1.getHeight() - targetHeight);

        Bitmap b2 = null;
        try {
            b2 = Bitmap.createBitmap(b1, dx1 / 2, dy1 / 2, targetWidth, targetHeight);
        } catch (OutOfMemoryError ex) {
            android.util.Log.w(TAG, "transform(): create bitmap(b2) oom", ex);
        }

        if (b2 != b1) {
            if (recycle || b1 != source) {
                b1.recycle();
            }
        }

        return b2;
    }

    public static Bitmap makeBitmap(int minSideLength, int maxNumOfPixels,
            ParcelFileDescriptor pfd, boolean useNative) {
        BitmapFactory.Options options = null;
        if (useNative) {
            options = createNativeAllocOptions();
        }
        return makeBitmap(minSideLength, maxNumOfPixels, null, null, pfd, options);
    }

    /**
     * Make a bitmap from a given Uri.
     *
     * @param uri
     */
    public static Bitmap makeBitmap(int minSideLength, int maxNumOfPixels, Uri uri,
            ContentResolver cr, boolean useNative, String dataPath) {
        ParcelFileDescriptor input = null;
        BitmapFactory.Options options = null;
        if (useNative) {
            options = createNativeAllocOptions();
        }
        try {
            input = cr.openFileDescriptor(uri, "r");
        } catch (IOException ex) {
            Log.e(TAG, "IOException "+ex);
        }

        if(input == null) {
            try{
                input = ParcelFileDescriptor.open(new File(dataPath),ParcelFileDescriptor.MODE_READ_ONLY);
            }catch(FileNotFoundException e) {
                Log.e(TAG, "IOException "+e);
                return null;
            }
        }
        return makeBitmap(minSideLength, maxNumOfPixels, uri, cr, input, options);
    }

    public static Bitmap makeBitmap(int minSideLength, int maxNumOfPixels, Uri uri,
            ContentResolver cr, ParcelFileDescriptor pfd, BitmapFactory.Options options) {
        try {
            if (pfd == null)
                pfd = makeInputStream(uri, cr);
            if (pfd == null)
                return null;
            if (options == null)
                options = Util.getNativeAllocOptions();;

            FileDescriptor fd = pfd.getFileDescriptor();
            options.inJustDecodeBounds = true;
            BitmapManager.instance().decodeFileDescriptor(fd, options);
            if (options.mCancel || options.outWidth == -1 || options.outHeight == -1) {
                return null;
            }
            options.inSampleSize = computeSampleSize(options, minSideLength, maxNumOfPixels);
            options.inJustDecodeBounds = false;

            options.inDither = false;
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
//            options.inPreferredConfig = Bitmap.Config.RGB_565;
            return BitmapManager.instance().decodeFileDescriptor(fd, options);
        } catch (OutOfMemoryError ex) {
            Log.e(TAG, "Got oom exception ", ex);
            return null;
        } finally {
            closeSilently(pfd);
        }
    }

    private static ParcelFileDescriptor makeInputStream(Uri uri, ContentResolver cr) {
        try {
            return cr.openFileDescriptor(uri, "r");
        } catch (IOException ex) {
            return null;
        }
    }

    // Returns Options that set the puregeable flag for Bitmap decode.
    public static BitmapFactory.Options createNativeAllocOptions() {
        return Util.getNativeAllocOptions();
    }

    /**
     * update the view status by selecting item
     * @param activity current activity
     * @param multiSelect current be selected item object.
     */
    public static void setViewStatus(Activity activity, List<?> multiSelect) {
        int[] viewIds = {R.id.btn_multi_confirm, R.id.btn_multi_cancel, R.id.btn_multi_collage};
        if(activity != null) {
            if(multiSelect != null) {
                int currentSize = multiSelect.size();
                boolean hasFile = currentSize > 0;
                for(int viewId : viewIds) {
                    /* FIX BUG: 5966
                     * BUG COMMENT: java.lang.NullPointerException
                     * DATE: 2014-02-24
                     */
                    if(activity.findViewById(viewId) != null) {
                        if(viewId == R.id.btn_multi_collage) {
                            activity.findViewById(viewId).setEnabled(currentSize > 1);
                        } else {
                            activity.findViewById(viewId).setEnabled(hasFile);
                        }
                    }
                }
            } else {
                for(int viewId : viewIds) {
                    if(activity.findViewById(viewId) != null) {
                        activity.findViewById(viewId).setEnabled(false);
                    }
                }
            }
        }
    }

    public static Bitmap createVideoThumbnail(FileDescriptor fd, int targetWidth) {
        Bitmap bitmap = null;
        MediaMetadataRetriever retriever = new MediaMetadataRetriever();
        try {
            retriever.setDataSource(fd);
            bitmap = retriever.getFrameAtTime(-1);
        } catch (IllegalArgumentException ex) {
        } catch (RuntimeException ex) {
        } finally {
            try {
                retriever.release();
            } catch (RuntimeException ex) {
            }
        }
        if (bitmap == null) {
            return null;
        }

        // Scale down the bitmap if it is bigger than we need.
        int width = bitmap.getWidth();
        int height = bitmap.getHeight();
        if (width > targetWidth) {
            float scale = (float) targetWidth / width;
            int w = Math.round(scale * width);
            int h = Math.round(scale * height);
            bitmap = Bitmap.createScaledBitmap(bitmap, w, h, true);
        }
        return bitmap;
    }

    public static boolean checkNetworkShowAlert(Context context) {
        if (Util.isNetworkAvailable(context))
            return true;

        Util.showAlert(context, context.getString(android.R.string.dialog_alert_title),
                    context.getString(R.string.sns_msg_network_unavailable));
        return false;
    }

    public static boolean isNetworkAvailable(Context context) {
        ConnectivityManager connManager =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = connManager.getActiveNetworkInfo();
        if (networkInfo != null) {
            return networkInfo.isAvailable();
        } else {
            return false;
        }
    }

    public static void setupNetwork(Activity activity){
        Intent intent=null;
      //Judge the version of the Android system,android.os.Build.VERSION.SDK_INT>10 is android 3.0 or above
      if(android.os.Build.VERSION.SDK_INT>10){
        intent = new Intent(android.provider.Settings.ACTION_SETTINGS);
      }else{
        intent = new Intent();
        ComponentName component = new ComponentName("com.android.settings","com.android.settings.Settings");
        intent.setComponent(component);
        intent.setAction("android.intent.action.VIEW");
      }
      activity.startActivity(intent);
    }

    public static boolean isWiFiNetworkAvilable(Activity activity) {
        boolean isWiFiAvilable = false;
        ConnectivityManager cm = (ConnectivityManager)activity.getSystemService(Context.CONNECTIVITY_SERVICE);
        //determine all network type
        /*NetworkInfo info = cm.getActiveNetworkInfo();
        if (info != null) {
            isNetworkUp = info.isAvailable();
        }*/
        //Only to determine the wifi
        if(cm.getNetworkInfo(ConnectivityManager.TYPE_WIFI).getState() == NetworkInfo.State.CONNECTED) {
            isWiFiAvilable = true;
        }

        return isWiFiAvilable;
    }
    /**
     * Display a simple alert dialog with the given text and title.
     *
     * @param context Android context in which the dialog should be displayed
     * @param title Alert dialog title
     * @param message Alert dialog message
     */
    public static void showAlert(Context context, String title, String message) {
        new AlertDialog.Builder(context)
                .setIcon(android.R.drawable.ic_dialog_alert)
                .setTitle(title)
                .setMessage(message)
                .show();
    }

    public static void upgradeGlobalPreferences(SharedPreferences pref) {
        int version;
        try {
            version = pref.getInt(UGalleryConst.KEY_VERSION, 0);
        } catch (Exception ex) {
            version = 0;
        }
        if (version == UGalleryConst.CURRENT_VERSION) return;

        SharedPreferences.Editor editor = pref.edit();
        editor.putInt(UGalleryConst.KEY_VERSION, UGalleryConst.CURRENT_VERSION);
        editor.commit();
    }

    public static void startBackgroundJob(MonitoredActivity activity, String title, String message, Runnable job, Handler handler) {
        // Make the progress dialog uncancelable, so that we can gurantee
        // the thread will be done before the activity getting destroyed.
//        ProgressDialog dialog = ProgressDialog.show(activity, title, message, true, false);
        ProgressDialog dialog = showProgressDialog(activity, title, message, false, false);
        new Thread(new BackgroundJob(activity, job, dialog, handler)).start();
    }

    private static ProgressDialog showProgressDialog(
            Activity activity, String title, String message, boolean indeterminate, boolean flag) {
        ProgressDialog dialog = new ProgressDialog(activity);
        dialog.requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
//        dialog.setTitle(title);
        dialog.setMessage(message);
        dialog.setIndeterminate(indeterminate);
        dialog.setCancelable(flag);
        dialog.show();
        return dialog;
    }

    public static String getExternalStorageDir() {
        String external = Environment.getExternalStorageDirectory().toString();
        if(Models.Samsung_GT_I9508.equals(Models.getModel()) || Models.Samsung_GT_I9500.equals(Models.getModel())) {
            external = createPath("storage", "extSdCard");
        } else if(Models.HUAWEI_P6.equals(Models.getModel()) || Models.isLaJiaoPepper()) {
            external = createPath("storage", "sdcard1");
        }
        return external;
    }
    /*
     * FIX BUG: 4844 4658 5115
     * BUG COMMENT: some devices have internal storage,we need get data from internal database for these devices
     * DATE: 2013-09-10 2013-11-04
     */
    public static String getInternalStorageDir(){
        String internalRootDir = null;
        if((!com.ucamera.ugallery.integration.Build.isTelecomCloud() && "QRD-8625DSDS".equals(Build.MODEL)) || "K723".equals(Build.MODEL)) {
            internalRootDir = createPath("storage","sdcard1");
        }else if("IS12S".equals(Build.MODEL)) {
            internalRootDir = "ext_card";
        }else if("GT-I9300".equals(Build.MODEL) ||
                "GT-N7100".equals(Build.MODEL) ||
                "GT-N7102".equals(Build.MODEL) ||
                "GT-N719".equals(Build.MODEL) ||
                "GT-I9502".equals(Build.MODEL)) {
            internalRootDir = createPath("storage","extSdCard");
        } else if("GT-I9508".equals(Build.MODEL) || "GT-I9500".equals(Build.MODEL) || Models.HUAWEI_P6.equals(Models.getModel())) {
            internalRootDir = createPath("storage","sdcard0");
        } else if(Environment.getExternalStorageDirectory().toString().startsWith(createPath("storage","emulated"))) {
            internalRootDir = createPath("storage","sdcard0");
        } else if(Models.isLaJiaoPepper()) {
            internalRootDir = Environment.getExternalStorageDirectory().toString();
        }

        if(internalRootDir != null) {
            File dir = new File(internalRootDir);
            if (dir.exists() && dir.canWrite()) {
                return internalRootDir;
            }
        }

        if(isOtherHasInternalStorageDevice()) {
            return createPath("mnt","sdcard1");
        }
        return null;
    }

    private static boolean isOtherHasInternalStorageDevice() {
        return Arrays.asList(new String[]{
                "HTC_X920e",
                "htc_butterfly",
                "SCH-I959"
        }).contains(Build.MODEL.replace('-', '_').replace(' ', '_'));
    }

    private static final String createPath(String... parts) {
        StringBuilder sb = new StringBuilder();
        for (String s : parts) {
            sb.append(File.separator).append(s);
        }
        return sb.toString();
    }

    // Returns an intent which is used for "set as" menu items.
    public static Intent createSetAsIntent(IImage image) {
        Uri u = image.fullSizeImageUri();
        Intent intent = new Intent(Intent.ACTION_ATTACH_DATA);
        intent.setDataAndType(u, image.getMimeType());
        intent.putExtra("mimeType", image.getMimeType());
        return intent;
    }
    public static void displayToast(Context context, String message) {
        Toast.makeText(context, message, Toast.LENGTH_SHORT).show();
    }
    public static void startCloudService(IImage image , Context context){
        String filePath = image.getDataPath();
        if(filePath != null) {
            SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
            String dcim = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM).toString();
            String imagePath = pref.getString("sf_pref_ucam_select_path_key", dcim+"/Camera");
            File file = new File(filePath);
            /**
             * FIX BUG: 6085
             * BUG CAUSE: java.lang.NullPointerException;
             * DATE: 2014-03-12
             */
            String dir = null;
            if(file.getParentFile() != null) {
                dir = file.getParentFile().getPath();
            }
            if(imagePath != null && dir != null && dir.startsWith(imagePath)) {
                Intent intent = new Intent();
                intent.setAction("telecom.mdesk.cloud.sys.ACTION_USER_PHOTO_CHANGED");
                intent.setPackage("telecom.mdesk.cloud.sys");
                intent.setClassName("telecom.mdesk.cloud.sys", "telecom.mdesk.cloud.sys.SysSyncUserPhotoChangedService");
                intent.putExtra("user_del_photo_name", filePath);
                context.startService(intent);
            }
        }
    }

    public static long getSystemTotalMemory(Context context){
        final ActivityManager actManager =
                (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
        actManager.getMemoryInfo(memInfo);
        return getFieldValueifExists(ActivityManager.MemoryInfo.class,
                "totalMem", memInfo, 0L);
    }

    public static <T> T getFieldValueifExists(Class<?> klass, String fieldName,
            Object obj, T defValue) {
        try {
            Field f = klass.getDeclaredField(fieldName);
            return (T) f.get(obj);
        } catch (Exception e) {
            return defValue;
        }
    }

    public static void setTextAndDrawableTop(Context context, TextView tv, int drawId, int textId){
        Drawable drawable= context.getResources().getDrawable(drawId);
        drawable.setBounds(0, 0, drawable.getMinimumWidth(), drawable.getMinimumHeight());
        tv.setCompoundDrawables(null, drawable, null, null);
        if(textId != Constants.NO_STRING) {
            tv.setText(textId);
        }
    }

    public static String getBurstPrefix(String title) {
        if(title != null && title.startsWith("Burst_")) {
            if(title.length() >= 21) {
                return title.substring(0, 21);
            } else {
                return title.substring(0,title.length());
            }
        }
        return null;
    }

    public static boolean isSelectPicMode(String entry) {
        if(ViewImage.IMAGE_ENTRY_NORMAL_VALUE.equals(entry) || ViewImage.IMAGE_ENTRY_UGALLERY_VALUE.equals(entry)) {
            return false;
        }
        return true;
    }

    public static boolean isBurstPicture(String title) {
        if(title != null && title.startsWith(ImageListSpecial.BURST_PREFIX)) {
            return true;
        }
        return false;
    }

    public static boolean isPanoramaPicture(String title) {
        if(title != null && title.startsWith(ImageListSpecial.PANORAMA_PREFIX)) {
            return true;
        }
        return false;
    }

    public static String getSuffix(String path) {
        if(path !=null && path.lastIndexOf(".")>0) {
            return path.substring(path.lastIndexOf("."));
        }
        return null;
    }

    public static int dip2px(Context context, float dpValue) {
        final float scale = context.getResources().getDisplayMetrics().density;
        return (int) (dpValue * scale + 0.5f);
    }

    private static class BackgroundJob extends MonitoredActivity.LifeCycleAdapter implements Runnable {

        private final MonitoredActivity mActivity;
        private final ProgressDialog mDialog;
        private final Runnable mJob;
        private final Handler mHandler;
        private final Runnable mCleanupRunner = new Runnable() {
            public void run() {
                mActivity.removeLifeCycleListener(BackgroundJob.this);
                if (mDialog.getWindow() != null)
                    mDialog.dismiss();
            }
        };

        public BackgroundJob(MonitoredActivity activity, Runnable job, ProgressDialog dialog, Handler handler) {
            mActivity = activity;
            mDialog = dialog;
            mJob = job;
            mActivity.addLifeCycleListener(this);
            mHandler = handler;
        }

        public void run() {
            try {
                mJob.run();
            } finally {
                mHandler.post(mCleanupRunner);
            }
        }

        @Override
        public void onActivityDestroyed(MonitoredActivity activity) {
            // We get here only when the onDestroyed being called before
            // the mCleanupRunner. So, run it now and remove it from the queue
            mCleanupRunner.run();
            mHandler.removeCallbacks(mCleanupRunner);
        }

        @Override
        public void onActivityStopped(MonitoredActivity activity) {
            mDialog.hide();
        }

        @Override
        public void onActivityStarted(MonitoredActivity activity) {
            mDialog.show();
        }
    }
    public static Bitmap decodeBitmapFromUri(Uri uri, Context context) {
        InputStream is = null;
        Bitmap bitmap = null;
        try {
            is = context.getContentResolver().openInputStream(uri);
            bitmap = BitmapFactory.decodeStream(is);
            if(bitmap != null) {
                int degrees = getOrientation(uri, context);
                bitmap = rotate(bitmap, degrees);
            }
        }catch (FileNotFoundException e) {
            Log.e(TAG, "FileNotFoundException: " + uri);
        } finally {
            closeSilently(is);
        }
        return bitmap;
    }
    private static final int INDEX_ORIENTATION = 0;
    private static final String[] IMAGE_PROJECTION = new String[] {
        ImageColumns.ORIENTATION
    };
    public static int getOrientation(Uri uri, Context context) {
        int orientation = 0;
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, IMAGE_PROJECTION, null, null, null);
            if ((cursor != null) && cursor.moveToNext()) {
                orientation = cursor.getInt(INDEX_ORIENTATION);
            }
        } catch (Exception e) {
            // Ignore error for no orientation column; just use the default orientation value 0.
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return orientation;
    }
}
