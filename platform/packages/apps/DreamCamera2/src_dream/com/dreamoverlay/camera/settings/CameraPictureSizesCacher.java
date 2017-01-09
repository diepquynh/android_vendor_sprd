/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.camera.settings;

import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.Camera;
import android.os.Build;
import android.preference.PreferenceManager;

import com.android.camera.settings.Keys;
import com.android.camera.util.Size;
import com.android.ex.camera2.portability.CameraAgent.CameraProxy;
import com.android.ex.camera2.portability.CameraCapabilities;
import android.graphics.ImageFormat;
import android.util.Log;
import com.android.camera.device.CameraId;
import com.android.camera.one.OneCameraCharacteristics;
import com.android.camera.one.OneCameraManager;
import com.android.camera.one.OneCameraModule;

import com.google.common.base.Optional;

import java.util.List;

/**
 * Facilitates caching of camera supported picture sizes, which is slow to
 * query. Will update cache if Build ID changes.
 */
public class CameraPictureSizesCacher {
    private static final String PICTURE_SIZES_BUILD_KEY = "CachedSupportedPictureSizes_Build_Camera";
    private static final String PICTURE_SIZES_SIZES_KEY = "CachedSupportedPictureSizes_Sizes_Camera";

    private static final String TAG = "CameraPictureSizesCacher";
    /**
     * Opportunistically update the picture sizes cache, if needed.
     * 
     * @param cameraId
     *            cameraID we have sizes for.
     * @param sizes
     *            List of valid sizes.
     */
    public static void updateSizesForCamera(Context context, int cameraId,
            List<Size> sizes) {
        String key_build = PICTURE_SIZES_BUILD_KEY + cameraId;
        SharedPreferences defaultPrefs = PreferenceManager
                .getDefaultSharedPreferences(context);
        String thisCameraCachedBuild = defaultPrefs.getString(key_build, null);
        // Write to cache.
        if (thisCameraCachedBuild == null) {
            String key_sizes = PICTURE_SIZES_SIZES_KEY + cameraId;
            SharedPreferences.Editor editor = defaultPrefs.edit();
            editor.putString(key_build, Build.DISPLAY);
            editor.putString(key_sizes, Size.listToString(sizes));
            editor.apply();
        }
    }

    /**
     * Return list of Sizes for provided cameraId. Check first to see if we have
     * it in the cache for the current android.os.Build. Note: This method calls
     * Camera.open(), so the camera must be closed before calling or null will
     * be returned if sizes were not previously cached.
     * 
     * @param cameraId
     *            cameraID we would like sizes for.
     * @param context
     *            valid android application context.
     * @return List of valid sizes, or null if the Camera can not be opened.
     */
    public static List<Size> getSizesForCamera(int cameraId, Context context) {
        Optional<List<Size>> cachedSizes = getCachedSizesForCamera(cameraId,
                context);
        if (cachedSizes.isPresent()) {
            return cachedSizes.get();
        }

        /**
         * SPRD: Fix bug 592976 that optimize the first boot time @{
         * Original Code
         *
        // No cached value, so need to query Camera API.
        Camera thisCamera;
        try {
            thisCamera = Camera.open(cameraId);
        } catch (RuntimeException e) {
            // Camera open will fail if already open.
            return null;
        }
        if (thisCamera != null) {
            String key_build = PICTURE_SIZES_BUILD_KEY + cameraId;
            String key_sizes = PICTURE_SIZES_SIZES_KEY + cameraId;
            SharedPreferences defaultPrefs = PreferenceManager
                    .getDefaultSharedPreferences(context);

            List<Size> sizes = Size.buildListFromCameraSizes(thisCamera
                    .getParameters().getSupportedPictureSizes());
            thisCamera.release();
            SharedPreferences.Editor editor = defaultPrefs.edit();
            editor.putString(key_build, Build.DISPLAY);
            editor.putString(key_sizes, Size.listToString(sizes));
            editor.apply();
            return sizes;
        }
        */
        try {
            String key_build = PICTURE_SIZES_BUILD_KEY + cameraId;
            String key_sizes = PICTURE_SIZES_SIZES_KEY + cameraId;
            SharedPreferences defaultPrefs = PreferenceManager
                    .getDefaultSharedPreferences(context);

            CameraId cameraIdIdentifier = CameraId.fromLegacyId(cameraId);
            OneCameraManager oneCameraManager = OneCameraModule.provideOneCameraManager();
            OneCameraCharacteristics cameraCharacteristicsFront =
                    oneCameraManager.getOneCameraCharacteristics(cameraIdIdentifier);
            List<Size> sizes = cameraCharacteristicsFront.getSupportedPictureSizes(ImageFormat.JPEG);

            SharedPreferences.Editor editor = defaultPrefs.edit();
            editor.putString(key_build, Build.DISPLAY);
            editor.putString(key_sizes, Size.listToString(sizes));
            editor.apply();
            return sizes;
        } catch (Exception e) {
            Log.w(TAG, e.getMessage());
        }
        return null;
    }

    public static List<Size> getSizesForCamera(int cameraId, Context context,
            CameraProxy thisCamera) {
        Optional<List<Size>> cachedSizes = getCachedSizesForCamera(cameraId,
                context);
        if (cachedSizes.isPresent()) {
            return cachedSizes.get();
        }

        if (thisCamera != null) {
            try {
                String key_build = PICTURE_SIZES_BUILD_KEY + cameraId;
                String key_sizes = PICTURE_SIZES_SIZES_KEY + cameraId;
                SharedPreferences defaultPrefs = PreferenceManager
                        .getDefaultSharedPreferences(context);

                CameraId cameraIdIdentifier = CameraId.fromLegacyId(cameraId);
                OneCameraManager oneCameraManager = OneCameraModule.provideOneCameraManager();
                OneCameraCharacteristics cameraCharacteristicsFront =
                        oneCameraManager.getOneCameraCharacteristics(cameraIdIdentifier);
                List<Size> sizes = cameraCharacteristicsFront.getSupportedPictureSizes(ImageFormat.JPEG);

                SharedPreferences.Editor editor = defaultPrefs.edit();
                editor.putString(key_build, Build.DISPLAY);
                editor.putString(key_sizes, Size.listToString(sizes));
                editor.apply();
                return sizes;
            } catch (Exception e) {
                Log.w(TAG, e.getMessage());
            }
        }
        return null;
    }

    /**
     * Returns the cached sizes for the current camera. See
     * {@link #getSizesForCamera} for details.
     * 
     * @param cameraId
     *            cameraID we would like sizes for.
     * @param context
     *            valid android application context.
     * @return Optional ist of valid sizes. Not present if the sizes for the
     *         given camera were not cached.
     */
    public static Optional<List<Size>> getCachedSizesForCamera(int cameraId,
            Context context) {
        String key_build = PICTURE_SIZES_BUILD_KEY + cameraId;
        String key_sizes = PICTURE_SIZES_SIZES_KEY + cameraId;
        SharedPreferences defaultPrefs = PreferenceManager
                .getDefaultSharedPreferences(context);
        // Return cached value for cameraId and current build, if available.
        String thisCameraCachedBuild = defaultPrefs.getString(key_build, null);
        if (thisCameraCachedBuild != null
                && thisCameraCachedBuild.equals(Build.DISPLAY)) {
            String thisCameraCachedSizeList = defaultPrefs.getString(key_sizes,
                    null);
            if (thisCameraCachedSizeList != null) {
                return Optional.of(Size.stringToList(thisCameraCachedSizeList));
            }
        }
        return Optional.absent();
    }

//    public static String getSlowMotionForCamera(Context context) {
//        Camera thisCamera;
//        try {
//            thisCamera = Camera.open(0);
//        } catch (RuntimeException e) {
//            // Camera open will fail if already open.
//            return null;
//        }
//        String slowMotion = null;
//        if (thisCamera != null) {
//            List<String> listSlowMotion = thisCamera.getParameters()
//                    .getSupportedSlowmotion();
//            thisCamera.release();
//            if (listSlowMotion != null) {
//                slowMotion = listSlowMotion.get(0);
//                for (int i = 1; i < listSlowMotion.size(); i++) {
//                    slowMotion = slowMotion + "," + listSlowMotion.get(i);
//                }
//            }
//            saveSlowMotion(context, slowMotion);
//        }
//        return slowMotion;
//    }

    public static String getSlowMotionForCamera(Context context,
            CameraProxy thisCamera) {
        String slowMotion = null;
        if (thisCamera != null) {
            List<String> listSlowMotion = thisCamera.getCapabilities()
                    .getSupportedSlowMotion();
            if (listSlowMotion != null && listSlowMotion.size() > 0) {
                slowMotion = listSlowMotion.get(0);
                for (int i = 1; i < listSlowMotion.size(); i++) {
                    slowMotion = slowMotion + "," + listSlowMotion.get(i);
                }
            }
            saveSlowMotion(context, slowMotion);
        }
        return slowMotion;
    }

    public static void saveSlowMotion(Context context, String slowMotion) {
        SharedPreferences defaultPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        SharedPreferences.Editor editor = defaultPrefs.edit();
        editor.putString(Keys.KEY_VIDEO_SLOW_MOTION_ALL, slowMotion);
        editor.apply();
    }

    public static String getCacheSlowMotionForCamera(Context context) {
        SharedPreferences defaultPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        String slowMotionValues = defaultPrefs.getString(Keys.KEY_VIDEO_SLOW_MOTION_ALL, null);
        return slowMotionValues;
    }
}
