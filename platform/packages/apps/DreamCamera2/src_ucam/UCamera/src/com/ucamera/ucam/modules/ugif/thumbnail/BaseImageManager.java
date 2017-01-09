/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.thumbnail;

import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import com.ucamera.ucam.modules.utils.LogUtils;
import com.ucamera.ucam.modules.utils.UiUtils;
import com.ucamera.ucam.modules.utils.Utils;
import android.app.Activity;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.provider.MediaStore.Images;
import android.provider.MediaStore.Images.ImageColumns;
import android.provider.MediaStore.Images.Media;
import android.provider.MediaStore.Video.VideoColumns;
import android.util.Log;

public class BaseImageManager {

    private static final String TAG = "BaseImageManager";

    private static final int BUFSIZE = 4096;

    private static final int INDEX_ID                   = 0;
    private static final int INDEX_ORIENTATION          = 1;

    public static final String LAST_IMAGE_THUMB_FILENAME = "last_image_thumb";
    public static final String LAST_VIDEO_THUMB_FILENAME = "last_video_thumb";
    public static final String LAST_GIF_THUMB_FILENAME = "last_gif_thumb";

    private static final String WHERE_CLAUSE = "(" + Media.MIME_TYPE + " in (?, ?, ?, ?, ?, ?))";
    private static final String WHERE_CLAUSE_WITH_BUCKET_ID = WHERE_CLAUSE + " AND " + Media.BUCKET_ID + " = ?";

    private static final String[] ACCEPTABLE_IMAGE_TYPES = new String[] {
            "image/jpeg", "image/png", "image/x-ms-bmp", "image/gif",
            "image/raw", "image/vnd.wap.wbmp" };

    public static final int SORT_ASCENDING = 1;
    public static final int SORT_DESCENDING = 2;

    private boolean mIsFromFile = false;

    public boolean isFromFile() {
        return mIsFromFile;
    }

    static final String[] IMAGE_PROJECTION = new String[] {
        ImageColumns._ID,ImageColumns.ORIENTATION,ImageColumns.DATE_TAKEN
    };

    private static final String[] VIDEO_PROJECTION = new String[] {
        VideoColumns._ID,VideoColumns.DATA,VideoColumns.DATE_TAKEN
    };

    private BaseImage mBaseImage = null;

    public BaseImage getLastImage(Activity activity, ContentResolver cr,String bucketId){
        mBaseImage = loadFrom(cr, new File(activity.getFilesDir(), LAST_IMAGE_THUMB_FILENAME));
        if(mBaseImage != null) {
            return mBaseImage;
        }
        createImageCursor(cr, bucketId);
        return mBaseImage;
    }

    public BaseImage getLastVideo(Activity activity, ContentResolver cr,String bucketId){
        mBaseImage = loadFrom(cr, new File(activity.getFilesDir(), LAST_VIDEO_THUMB_FILENAME));
        if(mBaseImage != null) {
            mIsFromFile = true;
            return mBaseImage;
        }
        createVideoCursor(cr, bucketId);
        mIsFromFile = false;
        return mBaseImage;
    }

    public BaseImage getLastGif(Activity activity, ContentResolver cr,String bucketId){
        /*mBaseImage = loadFrom(new File(activity.getFilesDir(), LAST_GIF_THUMB_FILENAME));
        if(mBaseImage != null) {
            return mBaseImage;
        }*/
        createGifCursor(cr, bucketId);
        return mBaseImage;
    }

    protected void createImageCursor(ContentResolver cr,String bucketId) {
        Uri uri = Images.Media.EXTERNAL_CONTENT_URI;
        Uri query = uri.buildUpon().appendQueryParameter("limit", "1").build();
        Cursor c = Media.query(cr, query, IMAGE_PROJECTION, whereClause(bucketId),
                whereClauseArgs(bucketId), sortOrder(SORT_DESCENDING));
        loadImageFromCursor(cr, c,BaseImage.FILE_TYPE_IMAGE,uri);
        if(c != null){
            c.close();
        }
    }

    protected void createVideoCursor(ContentResolver cr,String bucketId) {
        Uri uri = Uri.parse("content://media/external/video/media");
        Uri query = uri.buildUpon().appendQueryParameter("limit", "1").build();
        Cursor c = Images.Media.query(cr, query, VIDEO_PROJECTION,
                Images.Media.BUCKET_ID + " = '" + bucketId + "'", null, sortOrder(SORT_DESCENDING));
        loadImageFromCursor(cr, c,BaseImage.FILE_TYPE_VIDEO,uri);
        if(c != null){
            c.close();
        }
    }

    protected void createGifCursor(ContentResolver cr,String bucketId) {
        Uri uri = Images.Media.EXTERNAL_CONTENT_URI;
        Uri query = uri.buildUpon().appendQueryParameter("limit", "1").build();
        Cursor c = Images.Media.query(cr, query, IMAGE_PROJECTION,
                Images.Media.BUCKET_ID + " = '" + bucketId + "' and " + Images.Media.MIME_TYPE + " = 'image/gif'", null, sortOrder(SORT_DESCENDING));
        loadImageFromCursor(cr, c,BaseImage.FILE_TYPE_GIF,uri);
        if(c != null){
            c.close();
        }
    }

    protected  String whereClause(String bucketId) {
        return bucketId == null ? WHERE_CLAUSE : WHERE_CLAUSE_WITH_BUCKET_ID;
    }

    protected static String[] whereClauseArgs(String bucketId) {
        // Since mBucketId won't change, we should keep the array.
        if (bucketId != null) {
            int count = ACCEPTABLE_IMAGE_TYPES.length;
            String[] result = new String[count + 1];
            System.arraycopy(ACCEPTABLE_IMAGE_TYPES, 0, result, 0, count);
            result[count] = bucketId;
            return result;
        }
        return ACCEPTABLE_IMAGE_TYPES;
    }

    protected  String sortOrder(int sort) {
        String ascending = (sort == SORT_ASCENDING) ? " ASC" : " DESC";

        // Use DATE_TAKEN if it's non-null, otherwise use DATE_MODIFIED.
        // DATE_TAKEN is in milliseconds, but DATE_MODIFIED is in seconds.
        String dateExpr = "case ifnull(datetaken,0)" + " when 0 then date_modified*1000"
                + " else datetaken" + " end";

        // Add id to the end so that we don't ever get random sorting
        // which could happen, I suppose, if the date values are the same.
        return dateExpr + ascending + ", _id" + ascending;
    }

    protected  void loadImageFromCursor(ContentResolver cr, Cursor cursor,int fileType,Uri storageUri) {
        if(cursor == null || cursor.getCount() < 1) {
            mBaseImage = null;
            return;
        }
        cursor.moveToFirst();
        long id = cursor.getLong(INDEX_ID);
        int orientation = 0;
        if(fileType == BaseImage.FILE_TYPE_IMAGE) {
            orientation = cursor.getInt(INDEX_ORIENTATION);
        }
        if(orientation < 0 || orientation % 90 != 0) {
            orientation = 0;
        }
        mBaseImage = new BaseImage(cr, id, contentUri(id,storageUri), orientation,fileType);
    }

    public Uri contentUri(long id,Uri storageUri) {
        // avoid using exception for most cases
        try {
            // does our uri already have an id (single image query)?
            // if so just return it
            long existingId = ContentUris.parseId(storageUri);
            if (existingId != id)
                LogUtils.debug("nanxn", "id mismatch");
            return storageUri;
        } catch (NumberFormatException ex) {
            // otherwise tack on the id
            return ContentUris.withAppendedId(storageUri, id);
        }
    }

    public BaseImage loadFrom(ContentResolver cr, File file) {
        Uri uri = null;
        Bitmap bitmap = null;
        FileInputStream f = null;
        BufferedInputStream b = null;
        DataInputStream d = null;
        try {
            f = new FileInputStream(file);
            b = new BufferedInputStream(f, BUFSIZE);
            d = new DataInputStream(b);
            uri = Uri.parse(d.readUTF());
            bitmap = BitmapFactory.decodeStream(d);
            d.close();
        } catch (IOException e) {
            Log.i(TAG, "Fail to load bitmap. " + e);
            return null;
        } finally {
            Utils.closeSilently(f);
            Utils.closeSilently(b);
            Utils.closeSilently(d);
        }
        if (bitmap != null && UiUtils.isUriValid(uri, cr)) {
            return new BaseImage(uri, bitmap);
        }
        return null;
    }
    /*
     * FIX BUG: 6189
     * FIX COMMENT: deleteThumb when change select path;
     * DATE: 2014-04-01
     */
    public static void deleteThumb(Context context){
        File fileImage = new File(context.getFilesDir(), LAST_IMAGE_THUMB_FILENAME);
        if (fileImage != null && fileImage.exists()) {
            fileImage.delete();
        }
        File fileVideo = new File(context.getFilesDir(), LAST_VIDEO_THUMB_FILENAME);
        if (fileVideo != null && fileVideo.exists()) {
            fileVideo.delete();
        }
    }
}
