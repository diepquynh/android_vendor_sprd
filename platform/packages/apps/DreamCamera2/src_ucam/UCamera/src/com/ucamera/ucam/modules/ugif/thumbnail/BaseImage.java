/*
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucam.modules.ugif.thumbnail;

import com.ucamera.ucam.modules.utils.BitmapUtils;
import com.ucamera.ucam.utils.LogUtils;

import android.content.ContentResolver;
import android.graphics.Bitmap;
import android.net.Uri;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Video;

public class BaseImage {

    public static final int FILE_TYPE_IMAGE = 0;
    public static final int FILE_TYPE_VIDEO = 1;
    public static final int FILE_TYPE_GIF = 2;

    protected ContentResolver mContentResolver;

    protected long mId;

    protected Uri mUri;

    private int mOrientation;

    private Bitmap mBitmap;

    public BaseImage(ContentResolver cr, long id, Uri uri, int orientation,int fileType) {
        mContentResolver = cr;
        mId = id;
        mUri = uri;
        mOrientation = orientation;
        generateBitmap(fileType);
    }

    public BaseImage(Uri uri, Bitmap bitmap) {
        mUri = uri;
        mBitmap = bitmap;
    }

    private void generateBitmap(int fileType){
        if(fileType == FILE_TYPE_IMAGE || fileType == FILE_TYPE_GIF){
            //mBitmap = ThumbnailUtils.createImageThumbnail(mDataPath, Images.Thumbnails.MICRO_KIND);
            mBitmap = Images.Thumbnails.getThumbnail(mContentResolver, mId,Images.Thumbnails.MICRO_KIND, null);
            if (mBitmap != null) {
                mBitmap = BitmapUtils.rotate(mBitmap, mOrientation);
            }
        }else if(fileType == FILE_TYPE_VIDEO){
            //mBitmap = ThumbnailUtils.createVideoThumbnail(mDataPath);
            mBitmap = Video.Thumbnails.getThumbnail(mContentResolver, mId,Video.Thumbnails.MICRO_KIND, null);
        }
    }

    public Uri getUri() {
        return mUri;
    }

    public Bitmap getBitmap(){
        return mBitmap;
    }
}
