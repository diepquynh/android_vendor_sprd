
package com.sprd.note.data;

import com.sprd.note.utils.LogUtils;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.os.Debug;
import android.util.Log;

public class NoteProvider extends ContentProvider {
    private static final String TAG = "NoteProvider";
    DBOpenHelper mHelper;
    SQLiteDatabase db;

    public static final String AUTHORITY = "com.sprd.note.data.NoteProvider";
    public static final Uri NOTE_CONTENT_URI = Uri.parse("content://" + AUTHORITY + "/items");

    private static final UriMatcher sURIMatcher = new UriMatcher(UriMatcher.NO_MATCH);
    private static final int NOTES_QUERY_CUSTOM = 1;
    private static final int NOTES_ID = 2;

    private static final boolean DEBUG = LogUtils.DEBUG;

    static {
        sURIMatcher.addURI(AUTHORITY, "items", NOTES_QUERY_CUSTOM);
        sURIMatcher.addURI(AUTHORITY, "*/items/#", NOTES_ID);
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        /* SPRD: bug 520197 sqlitefull @{ */
        try {
            db = mHelper.getWritableDatabase();
        } catch (Exception e) {
            Log.w(TAG, "delete" + e.toString());
            return -1;
        }
        /* @} */
        switch (sURIMatcher.match(uri)) {
            case NOTES_QUERY_CUSTOM:
                long rowId = db.delete(DBOpenHelper.TABLE_NAME, selection, selectionArgs);
                getContext().getContentResolver().notifyChange(uri, null);
                return (int) rowId;
            default:
                throw new IllegalArgumentException("Unknown Uri: " + uri);

        }
    }

    @Override
    public String getType(Uri uri) {

        switch (sURIMatcher.match(uri))
        {
            case NOTES_ID:
                return "vnd.android.cursor.item/com.sprd.note";
            case NOTES_QUERY_CUSTOM:
                return "vnd.android.cursor.dir/com.sprd.note";
            default:
                return null;
        }
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        /* SPRD: bug 520197 sqlitefull  @{ */
        try {
            db = mHelper.getWritableDatabase();
        } catch (Exception e) {
            Log.w(TAG, "insert" + e.toString());
            return null;
        }
        /* @} */
        switch (sURIMatcher.match(uri)) {
            case NOTES_QUERY_CUSTOM:
                long rowId = db.insert(DBOpenHelper.TABLE_NAME, null, values);
                getContext().getContentResolver().notifyChange(uri, null);
                return ContentUris.withAppendedId(uri, rowId);
            default:
                throw new IllegalArgumentException("Unknown Uri: " + uri);
        }
    }

    @Override
    public boolean onCreate() {
        Log.d("NoteProvider", "onCreate");
        mHelper = DBOpenHelper.getInstance(getContext());
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
            String[] selectionArgs, String sortOrder) {
        if (DEBUG) {
            Log.v(TAG, "query  uri-->" + uri);
        }

        Log.i("UriMatcher code === ", sURIMatcher.match(uri) + "");
        Log.i("UriMatcher code === ", sURIMatcher.toString());

        switch (sURIMatcher.match(uri)) {
            case NOTES_QUERY_CUSTOM:
                return customQuery(uri, projection, selection, selectionArgs, sortOrder);
            default:
                throw new IllegalArgumentException("Unknown Uri: " + uri);
        }
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        /* SPRD: bug 520197 sqlitefull @{ */
        try {
            db = mHelper.getWritableDatabase();
        } catch (Exception e) {
            Log.w(TAG, "update" + e.toString());
            return -1;
        }
        /* @} */
        switch (sURIMatcher.match(uri)) {
            case NOTES_QUERY_CUSTOM:
                long rowId = db.update(DBOpenHelper.TABLE_NAME, values, selection, selectionArgs);
                getContext().getContentResolver().notifyChange(uri, null);
                return (int) rowId;
            default:
                throw new IllegalArgumentException("Unknown Uri: " + uri);
        }
    }

    private Cursor customQuery(Uri uri, String[] projection, String selection,
            String[] selectionArgs, String sortOrder) {
        sortOrder = DBOpenHelper.NOTE_IS_FOLDER + "  desc , " + DBOpenHelper.NOTE_UPDATE_DATE + " desc";
        /* SPRD: bug 520197 sqlitefull @{ */
        Cursor c = null;
        try {
            c = mHelper.getWritableDatabase().query(DBOpenHelper.TABLE_NAME, null, selection, selectionArgs, null, null, sortOrder);
        } catch (Exception e) {
            Log.w(TAG, "customQuery" + e.toString());
        }
        /* @} */
        return c;

    }

}
