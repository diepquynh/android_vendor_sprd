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
import android.graphics.Bitmap;
import com.ucamera.ugallery.provider.ThumbnailUtils;
import android.net.Uri;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Video;
import android.util.Log;

import java.io.IOException;
import java.io.InputStream;

import com.ucamera.ugallery.util.BitmapManager;
import com.ucamera.ugallery.util.Compatible;
import com.ucamera.ugallery.util.Util;

/**
 * Represents a particular video and provides access to the underlying data and
 * two thumbnail bitmaps as well as other information such as the id, and the
 * path to the actual video data.
 */
public class VideoObject extends BaseImage implements IImage {
    private static final String TAG = "VideoObject";
    private int mDuration;
    /**
     * Constructor.
     *
     * @param id        the image id of the image
     * @param cr        the content resolver
     */
    protected VideoObject(BaseImageList container, ContentResolver cr,
            long id, int index, Uri uri, String dataPath,
            String mimeType, long dateTaken, String title, String bucketId) {
        super(container, cr, id, index, uri, dataPath,
                mimeType, dateTaken, title, bucketId);
    }
    protected VideoObject(BaseImageList container, ContentResolver cr,
            long id, int index, Uri uri, String dataPath,
            String mimeType, long dateTaken, String title, String bucketId ,String resolution, int duration) {
        super(container, cr, id, index, uri, dataPath,
                mimeType, dateTaken, title, bucketId);
        mDuration = duration;
        parseResolution(resolution);
    }

    @Override
    public boolean equals(Object other) {
        if (other == null || !(other instanceof VideoObject)) return false;
        return fullSizeImageUri().equals(
                ((VideoObject) other).fullSizeImageUri());
    }

    @Override
    public int hashCode() {
        return fullSizeImageUri().toString().hashCode();
    }

    @Override
    public Bitmap fullSizeBitmap(int minSideLength, int maxNumberOfPixels,
            boolean rotateAsNeeded, boolean useNative) {
        /* FIX BUG : 4439
         * BUG CAUSE : some devices can not supported "setOrientationHint" function of MediaRecorder
         * BUG COMMENT : set correct orientation to thumbnail for this devices
         * DATE : 2013-07-01
         */
        Bitmap bitmap = android.media.ThumbnailUtils.createVideoThumbnail(mDataPath,Video.Thumbnails.MINI_KIND);
        if(!Compatible.instance().getOrientationRecordable()){
            bitmap = Util.rotate(bitmap, 90);
        }
        return bitmap;
    }

    @Override
    public InputStream fullSizeImageData() {
        try {
            InputStream input = mContentResolver.openInputStream(
                    fullSizeImageUri());
            return input;
        } catch (IOException ex) {
            return null;
        }
    }

    @Override
    public int getHeight() {
         return mHeight;
    }

    @Override
    public int getWidth() {
        return mWidth;
    }

    public int getDuration(){
        return mDuration;
    }
    public boolean isReadonly() {
        return false;
    }

    public boolean isDrm() {
        return false;
    }

    public boolean rotateImageBy(int degrees) {
       return false;
    }

    public Bitmap thumbBitmap(boolean rotateAsNeeded) {
        return fullSizeBitmap(THUMBNAIL_TARGET_SIZE, THUMBNAIL_MAX_NUM_PIXELS);
    }

    /*@Override
    public Bitmap miniThumbBitmap() {
        try {
            long id = mId;
            return BitmapManager.instance().getThumbnail(mContentResolver,
                    id, Images.Thumbnails.MICRO_KIND, null, true);
        } catch (Throwable ex) {
            Log.e(TAG, "miniThumbBitmap got exception", ex);
            return null;
        }
    }*/

    @Override
    public Object[] miniThumbBitmap() {
        Object[] obj = new Object[2];
        try {
            /*long id = mId;
            return BitmapManager.instance().getThumbnail(mContentResolver,
                    id, Images.Thumbnails.MICRO_KIND, null, true);*/
            /* FIX BUG : 5119
             * BUG CAUSE : The video picture become smaller when Drag the video;
             * DATE : 2013-10-29
             */
            obj[0] = true;
            obj[1] = BitmapManager.instance().getThumbnail(mContentResolver, mId, Images.Thumbnails.MINI_KIND, null, true);
            if(obj[1] == null) {
                obj[1] = android.media.ThumbnailUtils.createVideoThumbnail(mDataPath, Images.Thumbnails.MINI_KIND);
            }
            obj[1] = ThumbnailUtils.extractThumbnail((Bitmap)obj[1],
                    ThumbnailUtils.TARGET_SIZE_MINI_THUMBNAIL,
                    ThumbnailUtils.TARGET_SIZE_MINI_THUMBNAIL,
                    ThumbnailUtils.OPTIONS_RECYCLE_INPUT);
            return obj;
        } catch (Throwable ex) {
            Log.e(TAG, "miniThumbBitmap got exception", ex);
            return null;
        }
    }

    @Override
    public String toString() {
        return new StringBuilder("VideoObject").append(mId).toString();
    }

    @Override
    public long getImageId() {
        return mId;
    }

    private void parseResolution(String resolution) {
        if (resolution == null) return;
        int m = resolution.indexOf('x');
        if (m == -1) return;
        try {
            int w = Integer.parseInt(resolution.substring(0, m));
            int h = Integer.parseInt(resolution.substring(m + 1));
            mWidth = w;
            mHeight = h;
        } catch (Throwable t) {
            Log.w(TAG, t);
        }
    }
}
