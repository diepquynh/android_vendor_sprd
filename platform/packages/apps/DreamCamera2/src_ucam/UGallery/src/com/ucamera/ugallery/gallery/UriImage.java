/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

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

package com.ucamera.ugallery.gallery;

import android.content.ContentResolver;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore;
import android.provider.MediaStore.Images;
import android.util.Log;

import com.ucamera.ugallery.util.BitmapManager;
import com.ucamera.ugallery.util.ImageManager;
import com.ucamera.ugallery.util.Util;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.lang.ref.WeakReference;

public class UriImage implements IImage {
    private static final String TAG = "UriImage";

    private final Uri mUri;

    private final IImageList mContainer;

    private final ContentResolver mContentResolver;

    public UriImage(IImageList container, ContentResolver cr, Uri uri) {
        mContainer = container;
        mContentResolver = cr;
        mUri = uri;
    }

    public int getDegreesRotated() {
        return ImageManager.getExifOrientation(getDataPath());
    }

    public String getDataPath() {
        if (mUri.getScheme().equals("file")) {
            return mUri.getPath();
        }

        return getRealPathFromURI(mUri);
        //return mUri.getPath();
    }

    private InputStream getInputStream() {
        try {
            if (mUri.getScheme().equals("file")) {
                return new java.io.FileInputStream(mUri.getPath());
            } else {
                return mContentResolver.openInputStream(mUri);
            }
        } catch (FileNotFoundException ex) {
            return null;
        }
    }

    private ParcelFileDescriptor getPFD() {
        try {
            if (mUri.getScheme().equals("file")) {
                String path = mUri.getPath();
                return ParcelFileDescriptor.open(new File(path),
                        ParcelFileDescriptor.MODE_READ_ONLY);
            } else {
                return mContentResolver.openFileDescriptor(mUri, "r");
            }
        } catch (FileNotFoundException ex) {
            return null;
        }
    }
    public Object[] microBottomThubmBitmap() {
        return null;
    }
    public Bitmap fullSizeBitmap(int minSideLength, int maxNumberOfPixels) {
        return fullSizeBitmap(minSideLength, maxNumberOfPixels, IImage.ROTATE_AS_NEEDED,
                IImage.NO_NATIVE);
    }

    public Bitmap fullSizeBitmap(int minSideLength, int maxNumberOfPixels, boolean rotateAsNeeded) {
        return fullSizeBitmap(minSideLength, maxNumberOfPixels, rotateAsNeeded, IImage.NO_NATIVE);
    }

    public Bitmap fullSizeBitmap(int minSideLength, int maxNumberOfPixels, boolean rotateAsNeeded,
            boolean useNative) {
        try {
            ParcelFileDescriptor pfdInput = getPFD();
            Bitmap b = Util.makeBitmap(minSideLength, maxNumberOfPixels, pfdInput, useNative);
            if (b != null && rotateAsNeeded) {
                b = Util.rotate(b, getDegreesRotated());
            }
            return b;
        } catch (Exception ex) {
            Log.e(TAG, "got exception decoding bitmap ", ex);
            return null;
        }
    }

    public Uri fullSizeImageUri() {
        return mUri;
    }

    public InputStream fullSizeImageData() {
        return getInputStream();
    }

    public Object[] miniThumbBitmap() {
        Object[] obj = new Object[2];
        try {
            /*long id = mId;
            return BitmapManager.instance().getThumbnail(mContentResolver,
                    id, Images.Thumbnails.MICRO_KIND, null, true);*/
            obj[0] = true;
            obj[1] = thumbBitmap(IImage.ROTATE_AS_NEEDED);
            return obj;
        } catch (Throwable ex) {
            Log.e(TAG, "miniThumbBitmap got exception", ex);
            return null;
        }
    }

    public String getTitle() {
        return mUri.toString();
    }

    public String getDisplayName() {
        return getTitle();
    }

    public String getArtist() {
        // by liwd
        return "";
    }

    public Bitmap thumbBitmap(boolean rotateAsNeeded) {
        Bitmap bitmap =  fullSizeBitmap(THUMBNAIL_TARGET_SIZE, THUMBNAIL_MAX_NUM_PIXELS, rotateAsNeeded);
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
    public Bitmap getMicroCachedThumbBitmap(){
        return null;
    }
    private WeakReference<Bitmap> mThumbCache;

    private BitmapFactory.Options snifBitmapOptions() {
        ParcelFileDescriptor input = getPFD();
        if (input == null) {
            return null;
        }
        try {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inJustDecodeBounds = true;
            BitmapManager.instance().decodeFileDescriptor(input.getFileDescriptor(), options);
            return options;
        } finally {
            Util.closeSilently(input);
        }
    }

    public String getMimeType() {
        BitmapFactory.Options options = snifBitmapOptions();
        return (options != null && options.outMimeType != null) ? options.outMimeType : "";
    }

    public int getHeight() {
        BitmapFactory.Options options = snifBitmapOptions();
        return (options != null) ? options.outHeight : 0;
    }

    public int getWidth() {
        BitmapFactory.Options options = snifBitmapOptions();
        return (options != null) ? options.outWidth : 0;
    }

    public long fullSizeImageId() {
        return 0;
    }

    public IImageList getContainer() {
        return mContainer;
    }

    public long getDateTaken() {
        return 0;
    }

    public boolean isReadonly() {
        return true;
    }

    public boolean isDrm() {
        return false;
    }

    public boolean rotateImageBy(int degrees) {
        return false;
    }

    @Override
    public long getImageId() {
        return 0;
    }

    @Override
    public String getBucketId() {
        return ImageManager.getBucketIdForImage(getRealPathFromURI(mUri));
    }
    private String getRealPathFromURI(Uri contentUri) {
        if (contentUri == null) return null;
        if ("file".equals(contentUri.getScheme())) {
            return contentUri.getPath();
        }
        // can post image
        String path = null;
        Cursor cursor = null;
        String[] proj = { MediaStore.Images.Media.DATA };
        /*
         * FIX BUG: 5439
         * BUG COMMENT: there is a IllegalArgumentException exception if the database of uri not contains "_data" field
         * DATE: 2013-11-27
         */
        try{
            cursor = this.mContentResolver.query(contentUri, proj, // Which columns to return
                    null, // WHERE clause; which rows to return (all rows)
                    null, // WHERE clause selection arguments (none)
                    null); // Order-by clause (ascending by name)
            if (cursor != null) {
                int column_index = cursor
                        .getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
                if (cursor.moveToFirst()) {
                    path = cursor.getString(column_index);
                }
            }
        }catch(IllegalArgumentException e) {
            Log.e(TAG, "IllegalArgumentException : "+e);
            return null;
        } finally {
            if(cursor != null) {
                cursor.close();
            }
        }
        return path;
    }

    @Override
    public void rename(String title) {
    }
}
