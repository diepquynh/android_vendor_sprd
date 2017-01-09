/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 *
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

package com.ucamera.ugallery.gallery;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.ExifInterface;
import android.net.Uri;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.ImageColumns;
import android.util.Log;

import com.ucamera.ugallery.util.BitmapManager;
import com.ucamera.ugallery.util.Util;

import java.io.IOException;
import java.lang.ref.WeakReference;

/**
 * The class for normal images in gallery.
 */
public class Image extends BaseImage implements IImage {
    private static final String TAG = "BaseImage";

    private ExifInterface mExif;

    private int mRotation;

    public Image(BaseImageList container, ContentResolver cr, long id, int index, Uri uri,
            String dataPath, String mimeType, long dateTaken, String title, int rotation, String bucketId) {
        super(container, cr, id, index, uri, dataPath, mimeType, dateTaken, title, bucketId);
        mRotation = rotation;
    }

    @Override
    public int getDegreesRotated() {
        return mRotation;
    }

    protected void setDegreesRotated(int degrees) {
        if (mRotation == degrees)
            return;
        mRotation = degrees;
        ContentValues values = new ContentValues();
        values.put(ImageColumns.ORIENTATION, mRotation);
        mContentResolver.update(mUri, values, null, null);

        // Consider invalidate the cursor in container
        // ((BaseImageList) getContainer()).invalidateCursor();
    }

    public boolean isReadonly() {
        String mimeType = getMimeType();
        return !"image/jpeg".equals(mimeType) && !"image/jpg".equals(mimeType) && !"image/png".equals(mimeType);
    }

    public boolean isDrm() {
        return false;
    }

    /**
     * Replaces the tag if already there. Otherwise, adds to the exif tags.
     *
     * @param tag
     * @param value
     */
    public void replaceExifTag(String tag, String value) {
        if (mExif == null) {
            loadExifData();
        }
        mExif.setAttribute(tag, value);
    }

    private void loadExifData() {
        try {
            mExif = new ExifInterface(mDataPath);
        } catch (IOException ex) {
            Log.e(TAG, "cannot read exif", ex);
        }
    }

    private void saveExifData() throws IOException {
        if (mExif != null) {
            mExif.saveAttributes();
        }
    }

    private void setExifRotation(int degrees) {
        try {
            degrees %= 360;
            if (degrees < 0)
                degrees += 360;

            int orientation = ExifInterface.ORIENTATION_NORMAL;
            switch (degrees) {
                case 0:
                    orientation = ExifInterface.ORIENTATION_NORMAL;
                    break;
                case 90:
                    orientation = ExifInterface.ORIENTATION_ROTATE_90;
                    break;
                case 180:
                    orientation = ExifInterface.ORIENTATION_ROTATE_180;
                    break;
                case 270:
                    orientation = ExifInterface.ORIENTATION_ROTATE_270;
                    break;
            }

            replaceExifTag(ExifInterface.TAG_ORIENTATION, Integer.toString(orientation));
            saveExifData();
        } catch (Exception ex) {
            Log.e(TAG, "unable to save exif data with new orientation " + fullSizeImageUri(), ex);
        }
    }

    /**
     * Save the rotated image by updating the Exif "Orientation" tag.
     *
     * @param degrees
     */
    public boolean rotateImageBy(int degrees) {
        int newDegrees = (getDegreesRotated() + degrees) % 360;
        setExifRotation(newDegrees);
        setDegreesRotated(newDegrees);

        return true;
    }

    public Bitmap thumbBitmap(boolean rotateAsNeeded) {
        Bitmap bitmap = null;
        BitmapFactory.Options options = Util.getNativeAllocOptions();
        options.inDither = false;
        options.inPreferredConfig = Bitmap.Config.RGB_565;
        bitmap = BitmapManager.instance().getThumbnail(mContentResolver, mId,
                Images.Thumbnails.MINI_KIND, options, false);
        if (bitmap != null && rotateAsNeeded) {
            bitmap = Util.rotate(bitmap, getDegreesRotated());
        }

        mThumbCache = new WeakReference<Bitmap>(bitmap);
        return bitmap;
    }

    @Override
    public Bitmap getCachedThumbBitmap() {
        if (mThumbCache != null) {
            return mThumbCache.get();
        }
        return null;
    }

    private WeakReference<Bitmap> mThumbCache;
}
