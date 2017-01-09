/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.camera;

import android.app.ActivityManager;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.location.Location;
import android.net.Uri;
import android.os.AsyncTask;
import android.provider.MediaStore.Video;

import com.android.camera.app.MediaSaver;
import com.android.camera.data.FilmstripItemData;
import com.android.camera.debug.Log;
import com.android.camera.exif.ExifInterface;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.android.camera.settings.SettingsScopeNamespaces;
import com.android.camera.util.CameraUtil;
import com.sprd.camera.storagepath.StorageUtil;
import com.thundersoft.advancedfilter.TsAdvancedFilterNative;
import com.ucamera.ucam.jni.ImageProcessJni;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Locale;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Executor;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

// SPRD: Fix 474843 Add for Filter Feature

/**
 * A class implementing {@link com.android.camera.app.MediaSaver}.
 */
public class MediaSaverImpl implements MediaSaver {
    private static final Log.Tag TAG = new Log.Tag("MediaSaverImpl");
    private static final String VIDEO_BASE_URI = "content://media/external/video/media";

    /** The memory limit for unsaved image is 30MB. */
    // TODO: Revert this back to 20 MB when CaptureSession API supports saving
    // bursts.
    private static final int SAVE_TASK_MEMORY_LIMIT = 30 * 1024 * 1024;

    /* SPRD:Fix bug 555811 Can not slide around after taking a picture @{ */
    private static final int CPU_COUNT = Runtime.getRuntime().availableProcessors();
    private static final int CORE_POOL_SIZE = CPU_COUNT + 1;
    private static final int MAXIMUM_POOL_SIZE = CPU_COUNT * 2 + 1;
    private static final int KEEP_ALIVE = 1;
    private static final BlockingQueue<Runnable> sPoolWorkQueue =
            new LinkedBlockingQueue<Runnable>(128);
    private static final Executor THREAD_POOL_EXECUTOR
        = new ThreadPoolExecutor(CORE_POOL_SIZE, MAXIMUM_POOL_SIZE, KEEP_ALIVE,
            TimeUnit.SECONDS, sPoolWorkQueue, new ThreadPoolExecutor.DiscardOldestPolicy());
    /* @} */

    private final ContentResolver mContentResolver;

    /** Memory used by the total queued save request, in bytes. */
    private long mMemoryUse;

    // SPRD: Fix 474843 Add for Filter Feature
    private static final int MAX_IMAGE_COUNT_IN_LOW_RAM = 2;
    private int mCurrentImageCount = 0;
    /* @} */

    /** 
     * SPRD: fix bug 536179,synchronize the use of variable mMemoryUse
     */
    private Object mMemoryUseLock;

    private QueueListener mQueueListener;

    /* SPRD: Fix bug 547144 that app is killed because of low memory @{ */
    private final boolean IS_LOW_RAM = ActivityManager.isLowRamDeviceStatic();
    /* @} */

    // SPRD: Fix 474843 Add for Filter Feature
    private Context mAppContext = null;

    /**
     * @param contentResolver The {@link android.content.ContentResolver} to be
     *                 updated.
     */
    public MediaSaverImpl(ContentResolver contentResolver) {
        mContentResolver = contentResolver;
        mMemoryUse = 0;
        mMemoryUseLock = new Object();
    }

    /* SPRD: Fix 474843 Add for Filter Feature @{ */
    public MediaSaverImpl(Context context) {
        mAppContext = context;
        mContentResolver = context.getContentResolver();
        mMemoryUse = 0;
        mMemoryUseLock = new Object();
    }
    /* @} */

    /*SPRD:fix bug 388273 @{*/
    private Listener mListener;
    public interface Listener {//SPRD BUG:388273
        public void onHideBurstScreenHint();
        public int getContinuousCaptureCount();
    }

    @Override
    public void setListener(Listener l) {
        if (l == null) return;
        mListener = l;
    }

    @Override
    public boolean isEmptyQueue() {
        return mMemoryUse == 0;
    }
    /* @}*/

    @Override
    public boolean isQueueFull() {
        return (mMemoryUse >= SAVE_TASK_MEMORY_LIMIT);
    }

    /* SPRD: Fix 474843 Add for Filter Feature @{ */
    private boolean isQueueFull(boolean specialCase) {

        /* SPRD: Fix bug 547144 that app is killed because of low memory @{ */
        if (specialCase && IS_LOW_RAM
                && mCurrentImageCount >= MAX_IMAGE_COUNT_IN_LOW_RAM) {

            Log.i(TAG, "isQueueFull true");
            return true;
        }
        /* @} */

        return isQueueFull();
    }
    /* @} */

    @Override
    public void addImage(final byte[] data, String title, long date, Location loc, int width,
            int height, int orientation, ExifInterface exif, OnMediaSavedListener l) {
        /*
         * SPRD: Fix bug 535110, Photo voice record. @{
         * Original Android code :
        addImage(data, title, date, loc, width, height, orientation, exif, l,
                FilmstripItemData.MIME_TYPE_JPEG);
         */
        addImage(data, title, date, loc, width, height, orientation, exif, l,
                FilmstripItemData.MIME_TYPE_JPEG, null);
        /* @} */
    }

    /* SPRD: Fix bug 535110, Photo voice record. @{ */
    @Override
    public void addImage(final byte[] data, String title, long date,
            Location loc, int width, int height, int orientation,
            ExifInterface exif, OnMediaSavedListener l, String photoVoicePath) {
        addImage(data, title, date, loc, width, height, orientation, exif, l,
                FilmstripItemData.MIME_TYPE_JPEG, photoVoicePath);
    }
   /* @} */
    @Override
    /*
     * SPRD: Fix bug 535110, Photo voice record. @{
     * Original Android code :
    public void addImage(final byte[] data, String title, long date, Location loc, int width,
            int height, int orientation, ExifInterface exif, OnMediaSavedListener l,
            String mimeType) {
             */
    public void addImage(final byte[] data, String title, long date, Location loc, int width,
            int height, int orientation, ExifInterface exif, OnMediaSavedListener l,
            String mimeType, String photoVoicePath) {
           /* @} */
        /*
         * SPRD: Fix 474843 Add for Filter Feature
         * original code @{
         *
        if (isQueueFull()) {
            Log.e(TAG, "Cannot add image when the queue is full");
            return;
        }
         */
        if (isQueueFull(CameraUtil.mTimeStamp)) {
            // fix bug 388273, wont return
            Log.e(TAG, "Cannot add image when the queue is full");
        }
        /* @} */

        /*
         * SPRD: Fix bug 535110, Photo voice record. @{
         * Original Android code :
        ImageSaveTask t = new ImageSaveTask(data, title, date,
                (loc == null) ? null : new Location(loc),
                width, height, orientation, mimeType, exif, mContentResolver, l);
          */
        ImageSaveTask t = new ImageSaveTask(data, title, date,
                (loc == null) ? null : new Location(loc),
                width, height, orientation, mimeType, exif, mContentResolver, l, photoVoicePath);
        /* @} */

        /**
         * SPRD: fix bug 536179,synchronize the use of variable mMemoryUse @{
         * original code
        mMemoryUse += data.length;
        */
        synchronized (mMemoryUseLock) {
            mMemoryUse += data.length;
        }
        /* @} */

        /*
         * SPRD: Fix 474843 Add for Filter Feature
         * original code @{
         *
        if (isQueueFull()) {
            onQueueFull();
        }
         */
        mCurrentImageCount++;
        if (isQueueFull(CameraUtil.mTimeStamp)) {
            onQueueFull();
        }
        /* @} */

        /**
         * SPRD:fix bug 390702
        t.execute();
         */
        // SPRD:Fix bug 555811 Can not slide around after taking a picture
        t.executeOnExecutor(THREAD_POOL_EXECUTOR);
    }

    @Override
    public void addImage(final byte[] data, String title, long date, Location loc, int orientation,
            ExifInterface exif, OnMediaSavedListener l) {
        // When dimensions are unknown, pass 0 as width and height,
        // and decode image for width and height later in a background thread
        /*
         * SPRD: Fix bug 535110, Photo voice record. @{
         * Original Android code :
        addImage(data, title, date, loc, 0, 0, orientation, exif, l,
                FilmstripItemData.MIME_TYPE_JPEG);
         */
        addImage(data, title, date, loc, 0, 0, orientation, exif, l,
                FilmstripItemData.MIME_TYPE_JPEG, null);
        /* @} */
    }
    @Override
    public void addImage(final byte[] data, String title, Location loc, int width, int height,
            int orientation, ExifInterface exif, OnMediaSavedListener l) {
        /*
         * SPRD: Fix bug 535110, Photo voice record. @{
         * Original Android code :
        addImage(data, title, System.currentTimeMillis(), loc, width, height, orientation, exif, l,
                FilmstripItemData.MIME_TYPE_JPEG);
          */
        addImage(data, title, System.currentTimeMillis(), loc, width, height, orientation, exif, l,
                FilmstripItemData.MIME_TYPE_JPEG, null);
        /* @} */
    }

    /* SPRD: Fix bug 535110, Photo voice record. @{ */
    @Override
    public void addImage(final byte[] data, String title, Location loc,
            int width, int height, int orientation, ExifInterface exif,
            OnMediaSavedListener l, String photoVoicePath) {
        addImage(data, title, System.currentTimeMillis(), loc, width, height,
                orientation, exif, l, FilmstripItemData.MIME_TYPE_JPEG,
                photoVoicePath);
    }
    /* @} */

    /* SPRD: Fix 474843 Add for Filter Feature @{ */
    @Override
    public void addImage(byte[] data, String title, long date, Location loc, int width, int height,
            int orientation, ExifInterface exif, OnMediaSavedListener l,  String photoVoicePath, boolean filterHandle) {
        if (isQueueFull(filterHandle)) {
            Log.e(TAG, "Cannot add image when the queue is full");
        }
        ImageSaveTask t = new ImageSaveTask(data, title,  System.currentTimeMillis(),
                (loc == null) ? null : new Location(loc)
                , width, height, orientation, FilmstripItemData.MIME_TYPE_JPEG
                , exif, mContentResolver, l, photoVoicePath, filterHandle);

        mMemoryUse += data.length;

        mCurrentImageCount++;

        if (isQueueFull(filterHandle)) {
            onQueueFull();
        }
        // SPRD:Fix bug 555811 Can not slide around after taking a picture
        t.executeOnExecutor(THREAD_POOL_EXECUTOR);
    }
    /* @} */

    @Override
    public void addVideo(String path, ContentValues values, OnMediaSavedListener l) {
        // We don't set a queue limit for video saving because the file
        // is already in the storage. Only updating the database.
        new VideoSaveTask(path, values, l, mContentResolver).execute();
    }

    @Override
    public void setQueueListener(QueueListener l) {
        mQueueListener = l;
        if (l == null) {
            return;
        }
        l.onQueueStatus(isQueueFull());
    }

    private void onQueueFull() {
        if (mQueueListener != null) {
            mQueueListener.onQueueStatus(true);
        }
    }

    private void onQueueAvailable() {
        if (mQueueListener != null) {
            mQueueListener.onQueueStatus(false);
        }
    }

    private class ImageSaveTask extends AsyncTask <Void, Void, Uri> {
        private final byte[] data;
        private final String title;
        private final long date;
        private final Location loc;
        private int width, height;
        private final int orientation;
        private final String mimeType;
        private final ExifInterface exif;
        private final ContentResolver resolver;
        private final OnMediaSavedListener listener;
        // SPRD: Fix bug 474843, New feature of filter.
        private final boolean filterHandle;
        // SPRD: Fix bug 535110, Photo voice record.
        private final String photoVoicePath;

        /*
         * SPRD: Fix bug 535110, Photo voice record. @{
         * Original Android code :
        public ImageSaveTask(byte[] data, String title, long date, Location loc,
                             int width, int height, int orientation, String mimeType,
                             ExifInterface exif, ContentResolver resolver,
                             OnMediaSavedListener listener) {
          */
        public ImageSaveTask(byte[] data, String title, long date, Location loc,
                int width, int height, int orientation, String mimeType,
                ExifInterface exif, ContentResolver resolver,
                OnMediaSavedListener listener, String photoVoicePath) {
            this.data = data;
            this.title = title;
            this.date = date;
            this.loc = loc;
            this.width = width;
            this.height = height;
            this.orientation = orientation;
            this.mimeType = mimeType;
            this.exif = exif;
            this.resolver = resolver;
            this.listener = listener;
            // SPRD: Fix bug 535110, Photo voice record.
            this.photoVoicePath = photoVoicePath;

            // SPRD: Fix 474843 Add for Filter Feature
            this.filterHandle = false;
        }

        /* SPRD: Fix 474843 Add for Filter Feature @{ */
        public ImageSaveTask(byte[] data, String title, long date, Location loc,
                int width, int height, int orientation, String mimeType,
                ExifInterface exif, ContentResolver resolver,
                OnMediaSavedListener listener, String photoVoicePath, boolean filterHandle) {
            this.data = data;
            this.title = title;
            this.date = date;
            this.loc = loc;
            this.width = width;
            this.height = height;
            this.orientation = orientation;
            this.mimeType = mimeType;
            this.exif = exif;
            this.resolver = resolver;
            this.listener = listener;
            this.photoVoicePath = photoVoicePath;
            this.filterHandle = filterHandle;
        }
        /* @} */

        @Override
        protected void onPreExecute() {
            // do nothing.
        }

        @Override
        protected Uri doInBackground(Void... v) {
            if (width == 0 || height == 0) {
                // Decode bounds
                BitmapFactory.Options options = new BitmapFactory.Options();
                options.inJustDecodeBounds = true;
                BitmapFactory.decodeByteArray(data, 0, data.length, options);
                width = options.outWidth;
                height = options.outHeight;
            }
            try {
                byte[] newData = null;
                if (CameraUtil.mTimeStamp) {
                    //SimpleDateFormat dateFormat=new SimpleDateFormat(CameraUtil.getDateTimeFormat(resolver));
                    SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.ENGLISH);
                    String markDate = dateFormat.format(System.currentTimeMillis());
                    Log.i(TAG, "CML_addWatermarkForJpeg start " + markDate);

                    String timeStampPath = CameraUtil.setTimeStampPicPath();
                    if(timeStampPath == null){
                        return null;
                    }
                    newData = ImageProcessJni.ImageTimeStamp(data,data.length,markDate);
                    Log.i(TAG, "CML_addWatermarkForJpeg end");
                }
                /*
                 * SPRD: Fix 474843 Add for Filter Feature @{
                 * Original code:
                 return Storage.addImage(
                         resolver, title, date, loc, orientation, exif, data, width, height,
                         mimeType);
                 */
                if (CameraUtil.mTimeStamp && filterHandle) {
                    return addImage(resolver, title, date, loc, orientation, exif, newData, width, height, photoVoicePath);
                } else if (CameraUtil.mTimeStamp) {
                    return Storage.addImage(resolver, title, date, loc,
                            orientation, exif, newData, width, height, mimeType, photoVoicePath);
                } else if (filterHandle) {
                    return addImage(resolver, title, date, loc, orientation, exif, data, width, height, photoVoicePath);
                } else {
                    return Storage.addImage(resolver, title, date, loc,
                            orientation, exif, data, width, height, mimeType, photoVoicePath);
                }
                /* @} */

            } catch (IOException e) {
                Log.e(TAG, "Failed to write data", e);
                return null;
            }
        }

        @Override
        protected void onPostExecute(Uri uri) {
            if (listener != null) {
                listener.onMediaSaved(uri);
            }
            /*SPRD:modify for Coverity 109140
             * Orginal android code
            boolean previouslyFull = isQueueFull();
            */
            /**
             * SPRD: fix bug 536179,synchronize the use of variable mMemoryUse @{
             * original code
            mMemoryUse -= data.length;
            */
            synchronized (mMemoryUseLock) {
                mMemoryUse -= data.length;
            }
            /**@}*/

            // SPRD: Fix 474843 Add for Filter Feature
            mCurrentImageCount--;

            /**
             * SPRD: fix bug 388273 @{
            if (isQueueFull() != previouslyFull) {
                onQueueAvailable();
            }
            */
            Log.i(TAG, "mListener=" + mListener + "  ,mMemoryUse=" + mMemoryUse);
            if(mListener != null && mMemoryUse == 0 && mListener.getContinuousCaptureCount() == 0){//SPRD BUG:388273
                mListener.onHideBurstScreenHint();
                onQueueAvailable();
            }
        }
    }

    private class VideoSaveTask extends AsyncTask <Void, Void, Uri> {
        private String path;
        private final ContentValues values;
        private final OnMediaSavedListener listener;
        private final ContentResolver resolver;

        public VideoSaveTask(String path, ContentValues values, OnMediaSavedListener l,
                             ContentResolver r) {
            this.path = path;
            this.values = new ContentValues(values);
            this.listener = l;
            this.resolver = r;
        }

        @Override
        protected Uri doInBackground(Void... v) {
            Uri uri = null;
            try {
                Uri videoTable = Uri.parse(VIDEO_BASE_URI);
                long start = System.currentTimeMillis();
                uri = resolver.insert(videoTable, values);
                Log.i(TAG, "resolver.insert cost: " + (System.currentTimeMillis() - start));

                // Rename the video file to the final name. This avoids other
                // apps reading incomplete data.  We need to do it after we are
                // certain that the previous insert to MediaProvider is completed.
                String finalName = values.getAsString(Video.Media.DATA);
                File finalFile = new File(finalName);
                if (new File(path).renameTo(finalFile)) {
                    path = finalName;
                }
                start = System.currentTimeMillis();
                resolver.update(uri, values, null, null);
                Log.i(TAG, "resolver.update cost: " + (System.currentTimeMillis() - start));
            } catch (Exception e) {
                // We failed to insert into the database. This can happen if
                // the SD card is unmounted.
                Log.e(TAG, "failed to add video to media store", e);
                uri = null;
            } finally {
                Log.v(TAG, "Current video URI: " + uri);
            }
            return uri;
        }

        @Override
        protected void onPostExecute(Uri uri) {
            if (listener != null) {
                listener.onMediaSaved(uri);
            }
        }
    }

    /* SPRD: Fix 474843 Add for Filter Feature @{ */
    public Uri addImage(ContentResolver resolver, String title, long date,
            Location location, int orientation, ExifInterface exif, byte[] data, int width,
            int height,  String photoVoicePath) throws IOException {

        StorageUtil storageUtil = StorageUtil.getInstance();
        String path = storageUtil.generateFilePath(title , FilmstripItemData.MIME_TYPE_JPEG);

        SettingsManager settingManager = new SettingsManager(mAppContext);
        int filterType = settingManager.getInteger(SettingsManager.SCOPE_GLOBAL
                , Keys.KEY_CAMERA_FILTER_TYPE);
        int cameraId = settingManager.getInteger(settingManager
                .getModuleSettingScope(SettingsScopeNamespaces.FILTER)
                ,  Keys.KEY_CAMERA_ID);

        long start = System.currentTimeMillis();
        boolean result =  TsAdvancedFilterNative.takeFilterPicture(data, width, height
                , filterType, cameraId, null, path, true);
        long end = System.currentTimeMillis();
        Log.i(TAG, "cost: dofiltereffect = " + (end - start));

        Bitmap bitmap = (exif != null ? exif.getThumbnailBitmap() : null);
        if (bitmap != null) {
            File thumbFile = new File(path + ".thumb");
            File imageFile = new File(path);

            int thumbWidth = bitmap.getWidth();
            int thumbHeight = bitmap.getHeight();

            /* SPRD: Fix bug 578330, remove this extra rotation after the driver bug is fixed @{ */
            bitmap = CameraUtil.rotateAndMirror(bitmap, Exif.getOrientation(exif), false);
            exif.setCompressedThumbnail(bitmap);
            /* @} */

            bitmap.recycle();

            result = TsAdvancedFilterNative.takeFilterPicture(exif.getThumbnailBytes()
                    , thumbWidth, thumbHeight
                    , filterType, cameraId, null, thumbFile.getPath(), true);

            if (result) {
                FileInputStream thumbStream = null;
                FileInputStream imageStream = null;
                try {
                    thumbStream = new FileInputStream(thumbFile.getPath());
                    Bitmap thumbBitmapFiltered = BitmapFactory.decodeStream(thumbStream);
                    // SPRD: Fix bug 578330, remove this extra rotation after the driver bug is fixed
                    thumbBitmapFiltered = CameraUtil.rotate(thumbBitmapFiltered, (360 - Exif.getOrientation(exif)));
                    exif.setCompressedThumbnail(thumbBitmapFiltered);
                    thumbBitmapFiltered.recycle();

                    byte[] newData = new byte[(int) imageFile.length()];
                    imageStream = new FileInputStream(path);
                    imageStream.read(newData);
                    exif.writeExif(newData, path);

                } catch (Exception e) {
                    Log.i(TAG, "thumbnail write error", e);
                } finally {
                    if (thumbStream != null) {
                        thumbStream.close();
                    }
                    if (imageStream != null) {
                        imageStream.close();
                    }
                }
            }

            thumbFile.delete();
        }

        if (result) {
            return Storage.addImageToMediaStore(resolver, title, date, location, orientation, data.length,
                    path, width, height, FilmstripItemData.MIME_TYPE_JPEG, photoVoicePath);
        }

        return null;
    }
    /* @} */
}
