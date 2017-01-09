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
import android.net.Uri;
import android.provider.MediaStore.Images;

import java.util.HashMap;

/**
 * An implementation of interface <code>IImageList</code> which contains only
 * one image.
 */
public class SingleImageList implements IImageList {

    @SuppressWarnings("unused")
    private static final String TAG = "BaseImageList";

    private IImage mSingleImage;

    private Uri mUri;

    private ContentResolver mContentResolver;

    /**
     *
     * @param resolver resolver
     * @param uri uri
     */
    public SingleImageList(ContentResolver resolver, Uri uri) {
        mUri = uri;
        mSingleImage = new UriImage(this, resolver, uri);
        mContentResolver = resolver;
    }

    /**
     * @return HashMap
     */
    public HashMap<String, String> getBucketIds() {
        throw new UnsupportedOperationException();
    }

    public HashMap<String, String> getBaseUri() {
        return null;
    };
    /**
     * @return int
     */
    public int getCount() {
        return isEmpty() ? 0 : 1;
    }

    /**
     * @return boolean
     */
    public boolean isEmpty() {
        return mSingleImage == null;
    }

    /**
     * @param image image
     * @return int
     */
    public int getImageIndex(IImage image) {
        return image == mSingleImage ? 0 : -1;
    }

    /**
     * @param i i
     * @return IImage
     */
    public IImage getImageAt(int i) {
        return i == 0 ? mSingleImage : null;
    }

    /**
     * @param image image
     * @return boolean
     */
    public boolean removeImage(IImage image) {
        if (image == mSingleImage && image != null) {
            mSingleImage = null;
            Uri uri = image.fullSizeImageUri();
            if (uri == null) {
                return false;
            }
            if(uri.getScheme().equals("file")) {
                String path = uri.getPath();
                if(path != null) {
                    StringBuffer buff = new StringBuffer();
                    buff.append("(").append(Images.ImageColumns.DATA).append("=").append("'" + path + "'").append(")");
                    Cursor cur = mContentResolver.query(Images.Media.EXTERNAL_CONTENT_URI,
                            new String[] { Images.ImageColumns._ID },buff.toString(), null, null);
                    int index = 0;
                    for(cur.moveToFirst(); !cur.isAfterLast(); cur.moveToNext()) {
                        index = cur.getInt(cur.getColumnIndex(Images.ImageColumns._ID));
                    }
                    cur.close();
                    if(index > 0) {
                        Uri contentUri = Uri.parse(Images.Media.EXTERNAL_CONTENT_URI.toString() + "/" + index);
                        if(mContentResolver.delete(contentUri, null, null) > 0) {
                            return true;
                        }
                    }
                }
            } else {
                if(mContentResolver.delete(uri, null, null) > 0) {
                    return true;
                }
            }
            return false;
        }
        return false;
    }

    /**
     * @param index index
     * @return boolean
     */
    public boolean removeImageAt(int index) {
        if (index == 0 && mSingleImage != null) {
            mSingleImage = null;
            return true;
        }
        return false;
    }

    /**
     * @param uri uri
     * @return IImage
     */
    public IImage getImageForUri(Uri uri) {
        return uri.equals(mUri) ? mSingleImage : null;
    }

    /**
     * close
     */
    public void close() {
        mSingleImage = null;
        mUri = null;
    }
}
