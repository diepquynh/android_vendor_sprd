/*
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

package com.android.gallery3d.util;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.Activity;
import android.app.ActivityManager;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;
import android.graphics.Color;
import android.media.AudioSystem;
import android.net.Uri;
import android.os.ConditionVariable;
import android.os.Environment;
import android.os.StatFs;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;
import android.widget.Toast;

import com.android.gallery3d.R;
import com.android.gallery3d.app.GalleryActivity;
import com.android.gallery3d.app.GalleryAppImpl;
import com.android.gallery3d.app.PackagesMonitor;
import com.android.gallery3d.common.ApiHelper;
import com.android.gallery3d.data.DataManager;
import com.android.gallery3d.data.MediaItem;
import com.android.gallery3d.ui.TiledScreenNail;
import com.android.gallery3d.util.ThreadPool.CancelListener;
import com.android.gallery3d.util.ThreadPool.JobContext;
import com.sprd.gallery3d.app.VideoActivity;

import java.io.File;
import java.lang.ref.WeakReference;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

public class GalleryUtils {
    private static final String TAG = "GalleryUtils";
    private static final String MAPS_PACKAGE_NAME = "com.google.android.apps.maps";
    private static final String MAPS_CLASS_NAME = "com.google.android.maps.MapsActivity";
    private static final String CAMERA_LAUNCHER_NAME = "com.android.camera.CameraLauncher";

    public static final String MIME_TYPE_IMAGE = "image/*";
    public static final String MIME_TYPE_VIDEO = "video/*";
    public static final String MIME_TYPE_PANORAMA360 = "application/vnd.google.panorama360+jpg";
    public static final String MIME_TYPE_ALL = "*/*";

    private static final String DIR_TYPE_IMAGE = "vnd.android.cursor.dir/image";
    private static final String DIR_TYPE_VIDEO = "vnd.android.cursor.dir/video";

    private static final String PREFIX_PHOTO_EDITOR_UPDATE = "editor-update-";
    private static final String PREFIX_HAS_PHOTO_EDITOR = "has-editor-";

    private static final String KEY_CAMERA_UPDATE = "camera-update";
    private static final String KEY_HAS_CAMERA = "has-camera";

    private static float sPixelDensity = -1f;
    private static boolean sCameraAvailableInitialized = false;
    private static boolean sCameraAvailable;
    public static boolean isLowRam = false;

    /* sprd: add to support play gif @{ */
    public static final String MIME_TYPE_IMAGE_GIF = "image/gif";
    public static final boolean SUPPORT_GIF = true;
    /* @} */
    // SPRD: fix bug 387548, WBMP don't support edit
    public static final String MIME_TYPE_IMAGE_WBMP = "image/vnd.wap.wbmp";
    /* SPRD: Fix Bug 535131, add slide music feature @{ */
    private static final String SLIDESHOW_MUSIC = "slideshow_music";
    private static final String SLIDESHOW_MUSIC_KEY = "slideshow_music_key";
    private static final String SLIDESHOW_MUSIC_DIALOG_SELECT = "slideshow_music_dialog_select";
    /* @} */

    public static final int DONT_SUPPORT_VIEW_PHOTOS = 1;
    public static final int DONT_SUPPORT_VIEW_VIDEO = 2;
    private static Toast mToast = null;

    public static void initialize(Context context) {
        DisplayMetrics metrics = new DisplayMetrics();
        WindowManager wm = (WindowManager)
                context.getSystemService(Context.WINDOW_SERVICE);
        wm.getDefaultDisplay().getMetrics(metrics);
        sPixelDensity = metrics.density;
        Resources r = context.getResources();
        TiledScreenNail.setPlaceholderColor(r.getColor(
                R.color.bitmap_screennail_placeholder));
        initializeThumbnailSizes(metrics, r);
        /* SPRD: for bug 530910, low ram phone limit Region Decoder @{ */
        final ActivityManager am = (ActivityManager) context.getSystemService(
                Context.ACTIVITY_SERVICE);
        isLowRam = am.isLowRamDevice();
        /* @} */
    }

    private static void initializeThumbnailSizes(DisplayMetrics metrics, Resources r) {
        int maxPixels = Math.max(metrics.heightPixels, metrics.widthPixels);

        // For screen-nails, we never need to completely fill the screen
        // Modify for bug#596005, to enhance image clarity.
        // original code: MediaItem.setThumbnailSizes(maxPixels / 2, maxPixels / 5);
        MediaItem.setThumbnailSizes(maxPixels, maxPixels / 5);
        TiledScreenNail.setMaxSide(maxPixels / 2);
    }

    public static float[] intColorToFloatARGBArray(int from) {
        return new float[] {
            Color.alpha(from) / 255f,
            Color.red(from) / 255f,
            Color.green(from) / 255f,
            Color.blue(from) / 255f
        };
    }

    public static float dpToPixel(float dp) {
        return sPixelDensity * dp;
    }

    public static int dpToPixel(int dp) {
        return Math.round(dpToPixel((float) dp));
    }

    public static int meterToPixel(float meter) {
        // 1 meter = 39.37 inches, 1 inch = 160 dp.
        return Math.round(dpToPixel(meter * 39.37f * 160));
    }

    public static byte[] getBytes(String in) {
        byte[] result = new byte[in.length() * 2];
        int output = 0;
        for (char ch : in.toCharArray()) {
            result[output++] = (byte) (ch & 0xFF);
            result[output++] = (byte) (ch >> 8);
        }
        return result;
    }

    // Below are used the detect using database in the render thread. It only
    // works most of the time, but that's ok because it's for debugging only.

    private static volatile Thread sCurrentThread;
    private static volatile boolean sWarned;

    public static void setRenderThread() {
        sCurrentThread = Thread.currentThread();
    }

    public static void assertNotInRenderThread() {
        if (!sWarned) {
            if (Thread.currentThread() == sCurrentThread) {
                sWarned = true;
                Log.w(TAG, new Throwable("Should not do this in render thread"));
            }
        }
    }

    private static final double RAD_PER_DEG = Math.PI / 180.0;
    private static final double EARTH_RADIUS_METERS = 6367000.0;

    public static double fastDistanceMeters(double latRad1, double lngRad1,
            double latRad2, double lngRad2) {
       if ((Math.abs(latRad1 - latRad2) > RAD_PER_DEG)
             || (Math.abs(lngRad1 - lngRad2) > RAD_PER_DEG)) {
           return accurateDistanceMeters(latRad1, lngRad1, latRad2, lngRad2);
       }
       // Approximate sin(x) = x.
       double sineLat = (latRad1 - latRad2);

       // Approximate sin(x) = x.
       double sineLng = (lngRad1 - lngRad2);

       // Approximate cos(lat1) * cos(lat2) using
       // cos((lat1 + lat2)/2) ^ 2
       double cosTerms = Math.cos((latRad1 + latRad2) / 2.0);
       cosTerms = cosTerms * cosTerms;
       double trigTerm = sineLat * sineLat + cosTerms * sineLng * sineLng;
       trigTerm = Math.sqrt(trigTerm);

       // Approximate arcsin(x) = x
       return EARTH_RADIUS_METERS * trigTerm;
    }

    public static double accurateDistanceMeters(double lat1, double lng1,
            double lat2, double lng2) {
        double dlat = Math.sin(0.5 * (lat2 - lat1));
        double dlng = Math.sin(0.5 * (lng2 - lng1));
        double x = dlat * dlat + dlng * dlng * Math.cos(lat1) * Math.cos(lat2);
        return (2 * Math.atan2(Math.sqrt(x), Math.sqrt(Math.max(0.0,
                1.0 - x)))) * EARTH_RADIUS_METERS;
    }


    public static final double toMile(double meter) {
        return meter / 1609;
    }

    // For debugging, it will block the caller for timeout millis.
    public static void fakeBusy(JobContext jc, int timeout) {
        final ConditionVariable cv = new ConditionVariable();
        jc.setCancelListener(new CancelListener() {
            @Override
            public void onCancel() {
                cv.open();
            }
        });
        cv.block(timeout);
        jc.setCancelListener(null);
    }

    public static boolean isEditorAvailable(Context context, String mimeType) {
        /* SPRD: add to support play gif @{ */
        if (GalleryUtils.MIME_TYPE_IMAGE_GIF.equals(mimeType)
                // SPRD: Add for bug421702, WBMP do not support edit
                || GalleryUtils.MIME_TYPE_IMAGE_WBMP.equals(mimeType)) {
            return false;
        }
        /* @} */
        int version = PackagesMonitor.getPackagesVersion(context);

        String updateKey = PREFIX_PHOTO_EDITOR_UPDATE + mimeType;
        String hasKey = PREFIX_HAS_PHOTO_EDITOR + mimeType;

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        if (prefs.getInt(updateKey, 0) != version) {
            PackageManager packageManager = context.getPackageManager();
            List<ResolveInfo> infos = packageManager.queryIntentActivities(
                    new Intent(Intent.ACTION_EDIT).setType(mimeType), 0);
            prefs.edit().putInt(updateKey, version)
                        .putBoolean(hasKey, !infos.isEmpty())
                        .commit();
        }

        return prefs.getBoolean(hasKey, true);
    }

    public static boolean isAnyCameraAvailable(Context context) {
        int version = PackagesMonitor.getPackagesVersion(context);
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        if (prefs.getInt(KEY_CAMERA_UPDATE, 0) != version) {
            PackageManager packageManager = context.getPackageManager();
            List<ResolveInfo> infos = packageManager.queryIntentActivities(
                    new Intent(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA), 0);
            prefs.edit().putInt(KEY_CAMERA_UPDATE, version)
                        .putBoolean(KEY_HAS_CAMERA, !infos.isEmpty())
                        .commit();
        }
        return prefs.getBoolean(KEY_HAS_CAMERA, true);
    }

    public static boolean isCameraAvailable(Context context) {
        if (sCameraAvailableInitialized) return sCameraAvailable;
        PackageManager pm = context.getPackageManager();
        Intent cameraIntent = IntentHelper.getCameraIntent(context);
        List<ResolveInfo> apps = pm.queryIntentActivities(cameraIntent, 0);
        sCameraAvailableInitialized = true;
        sCameraAvailable = !apps.isEmpty();
        return sCameraAvailable;
    }

    public static void startCameraActivity(Context context) {
        /* SPRD: bug 473267 add video entrance */
        /* old bug info: Bug 378480 it will start camera mode when open the camera from videoplayer.
        Intent intent = new Intent(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA)
                .setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP
                        | Intent.FLAG_ACTIVITY_NEW_TASK);
        */
        Intent intent = new Intent();
        if(context instanceof VideoActivity) {
            intent.setAction(MediaStore.INTENT_ACTION_VIDEO_CAMERA);
        } else {
            intent.setAction(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA);
        }
        intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        /* @} */
        try {
            context.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            // This will only occur if Camera was disabled while Gallery is open
            // since we cache our availability check. Just abort the attempt.
            Log.e(TAG, "Camera activity previously detected but cannot be found", e);
        }
    }

    public static void startGalleryActivity(Context context) {
        Intent intent = new Intent(context, GalleryActivity.class)
                .setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP
                | Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(intent);
    }

    public static boolean isValidLocation(double latitude, double longitude) {
        // TODO: change || to && after we fix the default location issue
        return (latitude != MediaItem.INVALID_LATLNG || longitude != MediaItem.INVALID_LATLNG);
    }

    public static String formatLatitudeLongitude(String format, double latitude,
            double longitude) {
        // We need to specify the locale otherwise it may go wrong in some language
        // (e.g. Locale.FRENCH)
        return String.format(Locale.ENGLISH, format, latitude, longitude);
    }

    public static void showOnMap(Context context, double latitude, double longitude) {
        try {
            // We don't use "geo:latitude,longitude" because it only centers
            // the MapView to the specified location, but we need a marker
            // for further operations (routing to/from).
            // The q=(lat, lng) syntax is suggested by geo-team.
            String uri = formatLatitudeLongitude("http://maps.google.com/maps?f=q&q=(%f,%f)",
                    latitude, longitude);
            ComponentName compName = new ComponentName(MAPS_PACKAGE_NAME,
                    MAPS_CLASS_NAME);
            Intent mapsIntent = new Intent(Intent.ACTION_VIEW,
                    Uri.parse(uri)).setComponent(compName);
            context.startActivity(mapsIntent);
        } catch (ActivityNotFoundException e) {
            // Use the "geo intent" if no GMM is installed
            Log.e(TAG, "GMM activity not found!", e);
            String url = formatLatitudeLongitude("geo:%f,%f", latitude, longitude);
            Intent mapsIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
            context.startActivity(mapsIntent);
        }
    }

    public static void setViewPointMatrix(
            float matrix[], float x, float y, float z) {
        // The matrix is
        // -z,  0,  x,  0
        //  0, -z,  y,  0
        //  0,  0,  1,  0
        //  0,  0,  1, -z
        Arrays.fill(matrix, 0, 16, 0);
        matrix[0] = matrix[5] = matrix[15] = -z;
        matrix[8] = x;
        matrix[9] = y;
        matrix[10] = matrix[11] = 1;
    }

    public static int getBucketId(String path) {
        return path.toLowerCase().hashCode();
    }

    // Return the local path that matches the given bucketId. If no match is
    // found, return null
    public static String searchDirForPath(File dir, int bucketId) {
        File[] files = dir.listFiles();
        if (files != null) {
            for (File file : files) {
                if (file.isDirectory()) {
                    String path = file.getAbsolutePath();
                    if (GalleryUtils.getBucketId(path) == bucketId) {
                        return path;
                    } else {
                        path = searchDirForPath(file, bucketId);
                        if (path != null) return path;
                    }
                }
            }
        }
        return null;
    }

    // Returns a (localized) string for the given duration (in seconds).
    public static String formatDuration(final Context context, int duration) {
        int h = duration / 3600;
        int m = (duration - h * 3600) / 60;
        int s = duration - (h * 3600 + m * 60);
        String durationValue;
        if (h == 0) {
            durationValue = String.format(context.getString(R.string.details_ms), m, s);
        } else {
            durationValue = String.format(context.getString(R.string.details_hms), h, m, s);
        }
        return durationValue;
    }

    @TargetApi(ApiHelper.VERSION_CODES.HONEYCOMB)
    public static int determineTypeBits(Context context, Intent intent) {
        int typeBits = 0;
        String type = intent.resolveType(context);

        if (MIME_TYPE_ALL.equals(type)) {
            typeBits = DataManager.INCLUDE_ALL;
        } else if (MIME_TYPE_IMAGE.equals(type) ||
                DIR_TYPE_IMAGE.equals(type)) {
            typeBits = DataManager.INCLUDE_IMAGE;
        } else if (MIME_TYPE_VIDEO.equals(type) ||
                DIR_TYPE_VIDEO.equals(type)) {
            typeBits = DataManager.INCLUDE_VIDEO;
        } else {
            typeBits = DataManager.INCLUDE_ALL;
        }

        if (ApiHelper.HAS_INTENT_EXTRA_LOCAL_ONLY) {
            if (intent.getBooleanExtra(Intent.EXTRA_LOCAL_ONLY, false)) {
                typeBits |= DataManager.INCLUDE_LOCAL_ONLY;
            }
        }

        return typeBits;
    }

    public static int getSelectionModePrompt(int typeBits) {
        if ((typeBits & DataManager.INCLUDE_VIDEO) != 0) {
            return (typeBits & DataManager.INCLUDE_IMAGE) == 0
                    ? R.string.select_video
                    : R.string.select_item;
        }
        return R.string.select_image;
    }

    public static boolean hasSpaceForSize(long size) {
        String state = Environment.getExternalStorageState();
        if (!Environment.MEDIA_MOUNTED.equals(state)) {
            return false;
        }

        String path = Environment.getExternalStorageDirectory().getPath();
        try {
            StatFs stat = new StatFs(path);
            return stat.getAvailableBlocks() * (long) stat.getBlockSize() > size;
        } catch (Exception e) {
            Log.i(TAG, "Fail to access external storage", e);
        }
        return false;
    }

    public static boolean isPanorama(MediaItem item) {
        if (item == null) return false;
        int w = item.getWidth();
        int h = item.getHeight();
        return (h > 0 && w / h >= 2);
    }
    /**SPRD:Bug510007  check storage permission  @{*/
    public static boolean checkStoragePermissions(Context context) {
        if (context.checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            return true;
        } else {
            return false;
        }
    }
    /** @ } */

    /** SPRD:Bug 474639 add phone call reaction @{ */
    public static boolean checkReadPhonePermissions(Context context) {
        if (context.checkSelfPermission(Manifest.permission.READ_PHONE_STATE) == PackageManager.PERMISSION_GRANTED) {
            return true;
        } else {
            return false;
        }
    }
    /** @}*/

    /* SPRD: Modify for bug576760, check access location permission for Gallery @{ */
    public static boolean checkLocationPermissions(Context context) {
        if (context.checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED) {
            return true;
        } else {
            return false;
        }
    }
    /* @} */

    /* SPRD: Modify for bug592606, check access sms permission for Gallery @{ */
    public static boolean checkSmsPermissions(Context context){
        if (context.checkSelfPermission(Manifest.permission.RECEIVE_SMS) == PackageManager.PERMISSION_GRANTED) {
            return true;
        } else {
            return false;
        }
    }
    /* Bug592606 end @} */

    /* SPRD: Fix Bug 535131, bug 569414 add slide music feature @{ */
    public static String getSlideMusicUri(Context context){
        String uriString = context.getSharedPreferences(SLIDESHOW_MUSIC, Context.MODE_PRIVATE).getString(SLIDESHOW_MUSIC_KEY, null);
        return uriString;
    }

    public static void setSlideMusicUri(Context context, String uriString){
        Editor editor = context.getSharedPreferences(SLIDESHOW_MUSIC, Context.MODE_PRIVATE).edit();
        if(editor != null){
            editor.putString(SLIDESHOW_MUSIC_KEY, uriString);
            editor.commit();
        }
    }
    /* @} */

    public static boolean isMonkey() {
        return ActivityManager.isUserAMonkey();
    }

    /*
     * SPRD:add interface for speaker switch function on videoplayer is invalid. @{
     * @param routing device is speaker or headset
     */
    public static void setSpeakerMediaOn(boolean on) {
        Log.d(TAG,"setSpeakerMediaOn on = "+on);
        if (on) {
            Log.d(TAG,"setSpeakerMediaOn AudioSystem.FOR_MEDIA AudioSystem.FORCE_SPEAKER = "+AudioSystem.FORCE_SPEAKER);
            AudioSystem.setForceUse(AudioSystem.FOR_MEDIA, AudioSystem.FORCE_SPEAKER);
        } else {
            Log.d(TAG,"setSpeakerMediaOn AudioSystem.FOR_MEDIA AudioSystem.FORCE_NONE= "+AudioSystem.FORCE_NONE);
            AudioSystem.setForceUse(AudioSystem.FOR_MEDIA, AudioSystem.FORCE_NONE);
        }
    }

    public static void saveSelected(Context context, int selected){
        Editor editor = context.getSharedPreferences(SLIDESHOW_MUSIC, Context.MODE_PRIVATE).edit();
        if (editor != null) {
            editor.putInt(SLIDESHOW_MUSIC_DIALOG_SELECT, selected);
            editor.commit();
        }
    }

    public static int getSelected(Context context){
        int selected = context.getSharedPreferences(SLIDESHOW_MUSIC, Context.MODE_PRIVATE).getInt(SLIDESHOW_MUSIC_DIALOG_SELECT, 0);
        return selected;
    }

    /* @} */
    public static void killActivityInMultiWindow(Activity context, int photoOrvideo){
        if (context.isInMultiWindowMode()){
            int id = 0;
            if (photoOrvideo == DONT_SUPPORT_VIEW_VIDEO){
                id = R.string.exit_multiwindow_video_tips;
            }else if(photoOrvideo == DONT_SUPPORT_VIEW_PHOTOS){
                id = R.string.exit_multiwindow_tips;
            }else {
                return;
            }

            if (mToast == null){
                mToast = Toast.makeText(GalleryAppImpl.getApplication(), id, Toast.LENGTH_SHORT);
            }else {
                mToast.setText(id);
            }
            mToast.show();
            Log.d(TAG, "killActivityInMultiWindow");
            context.finish();
        }
    }

    /* SPRD :bug 630329 file Non-existent,get permissions and open it crash. */
    public static boolean isValidUri(Activity activity, Uri uri) {
        if (uri == null) return false;
        try {
            AssetFileDescriptor f = activity.getContentResolver().openAssetFileDescriptor(uri, "r");
            f.close();
            return true;
        } catch (Throwable e) {
            Log.w(TAG, "cannot open uri: " + uri, e);
            return false;
        }
    }
    /* @} */

}
