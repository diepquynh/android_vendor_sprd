/**
 * Created By Spreadst
 * */

package com.sprd.gallery3d.app;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

public class VideoBookmarkProvider extends ContentProvider {

    static final String _ID = "_id";
    static final String TITLE = "title";
    static final String BOOKMARK = "bookmark";
    static final String URI = "uri";
    static final String DURATION = "duration";
    static final String DATE = "date";
    static final int BOOKMARKS = 1;
    static final int BOOKMARKS_ID = 2;
    private static final UriMatcher uriMatcher;
    private static final String TAG = "VideoBookmarkProvider";
    static final String PROVIDER_NAME =
            "com.sprd.gallery3d.app.VideoBookmarkProvider";
    static final Uri CONTENT_URI =
            Uri.parse("content://" + PROVIDER_NAME + "/bookmarks");
    static {
        uriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
        uriMatcher.addURI(PROVIDER_NAME, "bookmarks", BOOKMARKS);
        uriMatcher.addURI(PROVIDER_NAME, "bookmarks/#", BOOKMARKS_ID);
    }
    private SQLiteDatabase bookmarksDB;
    static final String DATABASE_NAME = "VideoBookmarks.db";
    static final String DATABASE_TABLE = "Bookmark";
    static final int DATABASE_VERSION = 1;
    static final String DATABASE_CREATE =
            "create table "
                    + DATABASE_TABLE
                    + " (_id integer primary key autoincrement, "
                    + "title text not null, bookmark text not null,uri text not null,duration text not null,date text not null);";

    private class DatabaseHelper extends SQLiteOpenHelper {

        public DatabaseHelper(Context context, String name, CursorFactory factory, int version) {
            super(context, name, factory, version);
            // TODO Auto-generated constructor stub
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            Log.d(TAG, "onCreate");
            db.execSQL(DATABASE_CREATE);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            Log.d("Content provider database",
                    "Upgrading database from version " +
                            oldVersion + " to " + newVersion +
                            ", which will destroy all old data");
            db.execSQL("DROP TABLE IF EXISTS titles");
            onCreate(db);
        }
    }

    @Override
    public boolean onCreate() {
        Context context = getContext();
        DatabaseHelper dbHelper = new DatabaseHelper(context, DATABASE_NAME, null, DATABASE_VERSION);
        try {
            bookmarksDB = dbHelper.getWritableDatabase();
        } catch (SQLiteException e) {
            Log.w(TAG, "could not open database", e);
        }
        return (bookmarksDB == null) ? false : true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        SQLiteQueryBuilder sqlBuilder = new SQLiteQueryBuilder();
        sqlBuilder.setTables(DATABASE_TABLE);
        if (uriMatcher.match(uri) == BOOKMARKS_ID)
            // ---if getting a particular book---
            sqlBuilder.appendWhere(
                    _ID + " = " + uri.getPathSegments().get(1));
        if (sortOrder == null || sortOrder == "")
            sortOrder = TITLE;
        Cursor c = sqlBuilder.query(
                bookmarksDB,
                projection,
                selection,
                selectionArgs,
                null,
                null,
                sortOrder);
        // ---register to watch a content URI for changes---
        c.setNotificationUri(getContext().getContentResolver(), uri);
        return c;
    }

    @Override
    public String getType(Uri uri) {
        return "";
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        long rowId = bookmarksDB.insert(DATABASE_TABLE, "", values);
        // ---if added successfully---
        if (rowId > 0) {
            Uri _uri = ContentUris.withAppendedId(CONTENT_URI, rowId);
            Log.d(TAG, "insert successfully" + _uri);
            getContext().getContentResolver().notifyChange(_uri, null);
            return _uri;
        }
        throw new SQLException("Failed to insert row into " + uri);
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        int count = 0;
        switch (uriMatcher.match(uri)) {
            case BOOKMARKS:
                count = bookmarksDB.delete(
                        DATABASE_TABLE,
                        selection,
                        selectionArgs);
                break;
            case BOOKMARKS_ID:
                String id = uri.getPathSegments().get(1);
                count = bookmarksDB.delete(
                        DATABASE_TABLE,
                        _ID + " = " + id +
                                (!TextUtils.isEmpty(selection) ? " AND (" +
                                        selection + ')' : ""),
                        selectionArgs);
                break;
            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
        }
        getContext().getContentResolver().notifyChange(uri, null);
        Log.d(TAG, "delete " + count + " rows successfully");
        return count;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        // TODO Auto-generated method stub
        return 0;
    }

}
