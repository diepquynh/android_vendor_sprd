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
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore.Images;

import com.ucamera.ugallery.provider.ThumbnailUtils;
import com.ucamera.ugallery.provider.UCamData;
import com.ucamera.ugallery.util.BitmapManager;
import com.ucamera.ugallery.util.RotationUtil;
import com.ucamera.ugallery.util.Util;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

/**
 * Represents a particular image and provides access to the underlying bitmap
 * and two thumbnail bitmaps as well as other information such as the id, and
 * the path to the actual image data.
 */
public abstract class BaseImage implements IImage {
    private static final String TAG = "BaseImage";

    private static final int UNKNOWN_LENGTH = -1;

    protected ContentResolver mContentResolver;

    // Database field
    protected Uri mUri;

    protected long mId;

    protected String mDataPath;

    protected final int mIndex;

    protected String mMimeType;

    private final long mDateTaken;

    private String mTitle;

    private String mBucketId;

    protected BaseImageList mContainer;

    protected int mWidth = UNKNOWN_LENGTH;

    protected int mHeight = UNKNOWN_LENGTH;

    protected BaseImage(BaseImageList container, ContentResolver cr, long id, int index, Uri uri,
            String dataPath, String mimeType, long dateTaken, String title, String bucketId) {
        mContainer = container;
        mContentResolver = cr;
        mId = id;
        mIndex = index;
        mUri = uri;
        mDataPath = dataPath;
        mMimeType = mimeType;
        mDateTaken = dateTaken;
        mTitle = title;
        mBucketId = bucketId;
    }

    public String getDataPath() {
        return mDataPath;
    }

    @Override
    public boolean equals(Object other) {
        if (other == null || !(other instanceof Image))
            return false;
        return mUri.equals(((Image) other).mUri);
    }

    @Override
    public int hashCode() {
        return mUri.hashCode();
    }

    public Bitmap fullSizeBitmap(int minSideLength, int maxNumberOfPixels) {
        return fullSizeBitmap(minSideLength, maxNumberOfPixels, IImage.ROTATE_AS_NEEDED,
                IImage.NO_NATIVE);
    }

    public Bitmap fullSizeBitmap(int minSideLength, int maxNumberOfPixels, boolean rotateAsNeeded,
            boolean useNative) {
        Uri url = mContainer.contentUri(mId);
        if (url == null)
            return null;

        Bitmap b = Util.makeBitmap(minSideLength, maxNumberOfPixels, url, mContentResolver,
                useNative, mDataPath);
        if (b != null && rotateAsNeeded) {
            b = Util.rotate(b, getDegreesRotated());
        }

        return b;
    }

    public InputStream fullSizeImageData() {
        try {
            InputStream input = mContentResolver.openInputStream(mUri);
            return input;
        } catch (IOException ex) {
            return null;
        }
    }

    public Uri fullSizeImageUri() {
        return mUri;
    }

    public IImageList getContainer() {
        return mContainer;
    }

    public long getDateTaken() {
        return mDateTaken;
    }

    public int getDegreesRotated() {
        return 0;
    }

    public String getMimeType() {
        return mMimeType;
    }

    public String getTitle() {
        return mTitle;
    }
    public void rename(String title) {
        mTitle = title;
        /*
         * FIX BUG: 4883
         * BUG COMMENT: set new title to data path when the picture name has renamed
         * DATE: 2013-09-18
         */
        if(mDataPath != null) {
            int lastIndex = mDataPath.lastIndexOf("/");
            if(lastIndex == -1)
                return;
            mDataPath = mDataPath.substring(0, lastIndex + 1) + mTitle + ".jpg";
        }
    }
    public long getImageId() {
        return mId;
    }

    @Override
    public String getBucketId() {
        return mBucketId;
    }

    private void setupDimension() {
        ParcelFileDescriptor input = null;
        try {
            input = mContentResolver.openFileDescriptor(mUri, "r");
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inJustDecodeBounds = true;
            BitmapManager.instance().decodeFileDescriptor(input.getFileDescriptor(), options);
            mWidth = options.outWidth;
            mHeight = options.outHeight;
        } catch (FileNotFoundException ex) {
            mWidth = 0;
            mHeight = 0;
        } finally {
            Util.closeSilently(input);
        }
    }

    public int getWidth() {
        if (mWidth == UNKNOWN_LENGTH)
            setupDimension();
        return mWidth;
    }

    public int getHeight() {
        if (mHeight == UNKNOWN_LENGTH)
            setupDimension();
        return mHeight;
    }
    public Bitmap getMicroCachedThumbBitmap() {
        if(mThumbsCache != null && mThumbsCache.get(mDataPath) != null) {
            return mThumbsCache.get(mDataPath).get();
        }
        return null;
    }
    private Map<String, WeakReference<Bitmap>> mThumbsCache = new HashMap<String, WeakReference<Bitmap>>();
    public Object[] microBottomThubmBitmap() {
        Object[] obj = new Object[2];
        Bitmap b = null;

        Cursor cursor = mContentResolver.query(UCamData.Thumbnails.CONTENT_URI,
                new String[] {UCamData.Thumbnails.THUMB},
                UCamData.Thumbnails.THUMB_PATH + "=?",
                new String[]{mDataPath},
                null);
        if (cursor != null) {
            try {
                if (cursor.moveToFirst()) {
                    byte[] data = cursor.getBlob(0);
                    if (data != null) {
                        b = BitmapFactory.decodeByteArray(data, 0, data.length);
                        obj[0] = true;
                        obj[1] = b;
                        mThumbsCache.put(mDataPath, new WeakReference<Bitmap>(b));
                        return obj;
                    }
                }
            } finally {
                cursor.close();
            }
        }
        b = ThumbnailUtils.createImageBottomThumbnail(mDataPath, Images.Thumbnails.MICRO_KIND);
        if (b != null) {
            b = RotationUtil.rotate(b, getDegreesRotated());
        }

        obj[0] = false;
        obj[1] = b;
        mThumbsCache.put(mDataPath, new WeakReference<Bitmap>(b));

        return obj;
    }
    private WeakReference<Bitmap> mThumbCache;
    public Object[] miniThumbBitmap() {
        Object[] obj = new Object[2];
        Bitmap b = null;

        Cursor cursor = mContentResolver.query(UCamData.Thumbnails.CONTENT_URI,
                new String[] {UCamData.Thumbnails.THUMB},
                UCamData.Thumbnails.THUMB_PATH + "=?",
                new String[]{mDataPath},
                null);
        if (cursor != null) {
            try {
                if (cursor.moveToFirst()) {
                    byte[] data = cursor.getBlob(0);
                    if (data != null) {
                        b = BitmapFactory.decodeByteArray(data, 0, data.length);
                        obj[0] = true;
                        obj[1] = b;
                        mThumbCache = new WeakReference<Bitmap>(b);
                        return obj;
                    }
                }
            } finally {
                cursor.close();
            }
        }
        /*
         * FIX BUG: 5038
         * FIX COMMENT: Improve the thumbnail image quality ;
         * DATE: 2013-10-11
         */
        try {
            b = ThumbnailUtils.createImageThumbnail(mDataPath, Images.Thumbnails.MINI_KIND);
        } catch (Exception e) {
            android.util.Log.e(TAG, "miniThumbBitmap(): decode miniThumbBitmap fail", e);
        }

        if (b != null) {
            b = RotationUtil.rotate(b, getDegreesRotated());
        }

        obj[0] = false;
        obj[1] = b;

        mThumbCache = new WeakReference<Bitmap>(b);
        return obj;
    }

    protected void onRemove() {
    }

    @Override
    public String toString() {
        return mUri.toString();
    }

    @Override
    public Bitmap getCachedThumbBitmap() {
        if(mThumbCache != null) {
            return mThumbCache.get();
        }
        return null;
    }
}
