/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ugallery.provider;

import android.content.ContentProvider;
import android.content.ContentProviderOperation;
import android.content.ContentProviderResult;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.OperationApplicationException;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.text.TextUtils;
import android.util.Log;

import com.ucamera.ugallery.provider.UCamData.Albums;
import com.ucamera.ugallery.provider.UCamData.Thumbnails;
import com.ucamera.ugallery.util.Util;

import java.io.File;
import java.lang.reflect.Method;
import java.util.ArrayList;

public class UCamDataProvider extends ContentProvider {
    private static final String TAG = "UCamDataProvider";
    //database name;
    private static final int DATABASE_VERSION = 2;

    //table name;
    private static final String THUMB_NAME = "thumbnails";
    private static final int ALL = 0;
    private static final int THUMB = 1;
    private static final int THUMB_ID = 2;

    private static final String ALBUM_NAME = "albums";
    private static final int ALBUM = 3;
    private static final int ALBUM_ID = 4;

    private static final UriMatcher sUriMatcher;
    private static final String DATABASE_NAME = "ucam_data.db";

    private static final String CREATE_THUMB_TABLE = "CREATE TABLE IF NOT EXISTS "
        + THUMB_NAME + "("
        + Thumbnails._ID + " INTEGER PRIMARY KEY, "
        + Thumbnails.THUMB_ID + " INTEGER, "
        + Thumbnails.THUMB_PATH + " TEXT, "
        + Thumbnails.THUMB + " BLOB, "
        + Thumbnails.THUMB_DATE + " LONG );";

    private static final String CREATE_ALBUM_TABLE = "CREATE TABLE IF NOT EXISTS "
        + ALBUM_NAME + "("
        + Albums._ID + " INTEGER PRIMARY KEY, "
        + Albums.ALBUM_BUCKET + " TEXT, "
        + Albums.IMAGE_NAME + " TEXT, "
        + Albums.ALBUM + " BLOB );";

    static {
        sUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
        sUriMatcher.addURI(UCamData.AUTHORITY, "", ALL);
        sUriMatcher.addURI(UCamData.AUTHORITY, UCamData.THUMBNAILS, THUMB);
        sUriMatcher.addURI(UCamData.AUTHORITY, UCamData.THUMBNAILS + "/#", THUMB_ID);
        sUriMatcher.addURI(UCamData.AUTHORITY, UCamData.ALBUMS, ALBUM);
        sUriMatcher.addURI(UCamData.AUTHORITY, UCamData.ALBUMS + "/#", ALBUM_ID);
    }

    /**
     * This class helps open, create, and upgrade the database file.
     */
    private static class DatabaseHelper extends SQLiteOpenHelper {

        DatabaseHelper(Context context, String path, String dbName) {
            super(context, path + "/" + dbName, null, DATABASE_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL(CREATE_THUMB_TABLE);
            db.execSQL(CREATE_ALBUM_TABLE);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            Log.w(TAG, "Upgrading database from version " + oldVersion + " to "
                    + newVersion + ", which will destroy all old data");
            db.execSQL("DROP TABLE IF EXISTS " + THUMB_NAME);
            db.execSQL("DROP TABLE IF EXISTS " + ALBUM_NAME);
            onCreate(db);
        }
    }

    private DatabaseHelper mOpenHelper;

    @Override
    public boolean onCreate() {
        Log.d(TAG, "onCreate() start");
        File cacheDir = getContext().getExternalCacheDir();
        Log.i(TAG, "getExternalCacheDir end");
        if (cacheDir != null) {
            mOpenHelper = new DatabaseHelper(getContext(), cacheDir.getPath(),
                    DATABASE_NAME);
            Log.e(TAG, "onCreate() end cacheDir != null");
            return true;
        } else {
            mOpenHelper = new DatabaseHelper(getContext(), null, DATABASE_NAME);
            Log.e(TAG, "onCreate() end");
            return false;
        }
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        int count = 0;
        try{
            SQLiteDatabase db = mOpenHelper.getWritableDatabase();
            switch (sUriMatcher.match(uri)) {
            case THUMB:
                count = db.delete(THUMB_NAME, selection, selectionArgs);
                break;

            case THUMB_ID:
                String imageId = uri.getPathSegments().get(1);
                count = db.delete(THUMB_NAME, UCamData.Thumbnails.THUMB_ID + "=" + imageId
                        + (!TextUtils.isEmpty(selection) ? " AND (" + selection + ')' : ""), selectionArgs);
                break;
            case ALBUM:
                count = db.delete(ALBUM_NAME, selection, selectionArgs);
                break;
            case ALBUM_ID:
                String bucketId = uri.getPathSegments().get(1);
                count = db.delete(ALBUM_NAME, UCamData.Albums.ALBUM_BUCKET + "=" + bucketId
                        + (!TextUtils.isEmpty(selection) ? " AND (" + selection + ')' : ""), selectionArgs);
                break;
            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
            }

        }catch(SQLiteException e) {
            Log.d(TAG, "delete SQLiteException : " +e);
        }
        getContext().getContentResolver().notifyChange(uri, null);
        return count;
    }

    @Override
    public String getType(Uri uri) {
        return null;
    }

    @Override
    public Uri insert(Uri uri, ContentValues initialValues) {
        /*if(sUriMatcher.match(uri) != THUMB) {
            throw new IllegalArgumentException("Unknown URI " + uri);
        }

        ContentValues values;
        if (initialValues != null) {
            values = new ContentValues(initialValues);
        } else {
            values = new ContentValues();
        }

        if(values.containsKey(UCamData.Thumbnails.THUMB_ID) == false) {
            throw new SQLException("Failed to insert row into " + uri + " in THUMB_ID");
        }

        if(values.containsKey(UCamData.Thumbnails.THUMB_PATH) == false) {
            throw new SQLException("Failed to insert row into " + uri + " in THUMB_PATH");
        }

        if(values.containsKey(UCamData.Thumbnails.THUMB) == false) {
            throw new SQLException("Failed to insert row into " + uri + " in THUMB");
        }*/

        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        try {
            long rowId = 0;
            switch (sUriMatcher.match(uri)) {
                case THUMB:
                    rowId = db.insert(THUMB_NAME, null, initialValues);
                    if(rowId > 0) {
                        Uri thumbUri = ContentUris.withAppendedId(UCamData.Thumbnails.CONTENT_URI, rowId);
                        getContext().getContentResolver().notifyChange(thumbUri, null);

                        return thumbUri;
                    }
                    break;
                case ALBUM:
                    rowId = db.insert(ALBUM_NAME, null, initialValues);
                    if(rowId > 0) {
                        Uri albumUri = ContentUris.withAppendedId(UCamData.Albums.CONTENT_URI, rowId);
                        getContext().getContentResolver().notifyChange(albumUri, null);

                        return albumUri;
                    }
                    break;
                default:
                    break;
            }
        } catch(SQLiteException e){
            Log.d(TAG, "insert SQLiteException : " +e);
        } finally {
        }
        //throw new SQLException("Failed to insert row into " + uri);
        Log.d(TAG, "Failed to insert row into " + uri);
        return null;
    }

    @Override
    public ContentProviderResult[] applyBatch(ArrayList<ContentProviderOperation> operations)
            throws OperationApplicationException {
        long beginTime = System.currentTimeMillis();
        SQLiteDatabase db = null;
        try {
            /*
             * FIX BUG:5863
             * FIX COMMETN: avoid null point exception
             * DATE: 2014-01-21
             */
            if(mOpenHelper != null) {
                db = mOpenHelper.getWritableDatabase();
            }
            if(db == null) {
                return null;
            }
            db.beginTransaction();
            /*
             * FIX BUG: 1747 1758
             * BUG CAUSE: The operations list size is zero;
             * FIX COMMENT: To organizing data;
             * DATE: 2012-10-23
             */
            final int numOperations = operations.size();
            final ContentProviderResult[] results = new ContentProviderResult[numOperations];
            /*
             * FIX BUG: 1916
             * BUG CAUSE: Invalid index 1, size is 0 ;
             * FIX COMMENT: try catch();
             * DATE: 2012-11-16
             */
            try{
                for (int i = 0; i < numOperations; i++) {
                    if(operations.get(i) != null) {
                        results[i] = operations.get(i).apply(this, results, i);
                    }
                }
            } catch(IndexOutOfBoundsException ex){
                Log.w(TAG, "applyBatch()" + ex);
            }
            db.setTransactionSuccessful();
            return results;
        } catch(SQLiteException e) {
            Log.d(TAG, "applyBatch SQLiteException : " +e);
            return null;
            /*
             * FIX BUG: 6215
             * FIX COMMENT:Java.lang.IllegalStateException: no transaction pending
             * DATE: 2014-04-13
             */
        } catch (IllegalStateException e) {
            Log.d(TAG, "applyBatch finally IllegalStateException no transaction pending : " +e);
            return null;
        } finally {
            try{
                if(db != null) {
                    db.endTransaction();
                }
            }catch(SQLiteException e){
                Log.d(TAG, "applyBatch finally SQLiteException : " +e);
                return null;
            }
            Log.d(TAG, "applyBatch(): end Transaction time is " + (System.currentTimeMillis() - beginTime));
        }
    }

    @Override
    public int bulkInsert(Uri uri, ContentValues[] values) {
        SQLiteDatabase db = null;
        int numInserted = 0;
        try {
            db = mOpenHelper.getWritableDatabase();
            db.beginTransaction();
            int len = values.length;
            for (int i = 0; i < len; i++) {
                db.insert(THUMB_NAME, null, values[i]);
            }
            numInserted = len;
            db.setTransactionSuccessful();
        } catch(SQLiteException e) {
            Log.d(TAG, "bulkInsert SQLiteException : " +e);
        } finally {
            db.endTransaction();
        }
        getContext().getContentResolver().notifyChange(uri, null);
        return numInserted;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
        String orderBy = null;

        switch (sUriMatcher.match(uri)) {
            case THUMB:
                qb.setTables(THUMB_NAME);
                if (TextUtils.isEmpty(sortOrder)) {
                    orderBy = UCamData.Thumbnails.DEFAULT_SORT_ORDER;
                } else {
                    orderBy = sortOrder;
                }
                break;
            case THUMB_ID:
                qb.setTables(THUMB_NAME);
                qb.appendWhere(UCamData.Thumbnails._ID + "=" + uri.getPathSegments().get(1));
                break;
            case ALBUM:
                qb.setTables(ALBUM_NAME);
                if (TextUtils.isEmpty(sortOrder)) {
                    orderBy = UCamData.Albums.DEFAULT_SORT_ORDER;
                } else {
                    orderBy = sortOrder;
                }
                break;
            case ALBUM_ID:
                qb.setTables(ALBUM_NAME);
                qb.appendWhere(UCamData.Albums.ALBUM_BUCKET + "=" + uri.getPathSegments().get(1));
                break;
        }

        Cursor cursor = null;
        try{
            SQLiteDatabase db = mOpenHelper.getReadableDatabase();
            cursor = qb.query(db, projection, selection, selectionArgs, null, null, orderBy);
        }catch(SQLiteException e){
            Log.d(TAG, "SQLiteException: " + e);
            return null;
        }

        // Tell the cursor what uri to watch, so it knows when its source data changes
        cursor.setNotificationUri(getContext().getContentResolver(), uri);
        return cursor;

    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        int count = 0;
        try{
            SQLiteDatabase db = mOpenHelper.getWritableDatabase();
            switch (sUriMatcher.match(uri)) {
            case THUMB:
                count = db.update(THUMB_NAME, values, selection, selectionArgs);
                break;
            case THUMB_ID:
                String imageId = uri.getPathSegments().get(1);
                count = db.update(THUMB_NAME, values, UCamData.Thumbnails.THUMB_ID + "=" + imageId
                        + (!TextUtils.isEmpty(selection) ? " AND (" + selection + ')' : ""), selectionArgs);
                break;
            case ALBUM:
                count = db.update(ALBUM_NAME, values, selection, selectionArgs);
                break;
            case ALBUM_ID:
                String bucketId = uri.getPathSegments().get(1);
                count = db.update(ALBUM_NAME, values, UCamData.Albums.ALBUM_BUCKET + "=" + bucketId
                        + (!TextUtils.isEmpty(selection) ? " AND (" + selection + ')' : ""), selectionArgs);
                break;
            }
        } catch (SQLiteException e) {
            Log.d(TAG, "update SQLiteException: " + e);
        }
        getContext().getContentResolver().notifyChange(uri, null);

        return count;
    }

}
