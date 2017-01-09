/** Created by Spreadst */
package com.sprd.gallery3d.app;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteStatement;
import android.net.Uri;
import android.util.Log;

import android.content.Intent;
import android.widget.Toast;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

public class MovieViewContentProvider extends ContentProvider {

    private final static String TAG = "MovieViewContentProvider";

    public static final Uri CONTENT_URI = Uri
            .parse("content://com.sprd.gallery3d/movieview");

    public static final String _ID = "_id";

    public static final String NAME = "name";

    public static final String MAX_PORT = "max_port";

    public static final String MIN_PORT = "min_port";

    public static final String NET_INFO = "net_info";

    public static final String CONN_PROF = "conn_prof";

    public static final String HTTP_PROXY = "http_proxy";

    public static final String HTTP_PORT = "http_port";

    public static final String RTSP_PROXY = "rtsp_proxy";

    public static final String RTSP_PORT = "rtsp_port";

    public static final String BOOKMARK_TITLE = "title";

    public static final String BOOKMARK_URL = "url";

    private static final String BOOKMARKS_TABLE = "bookmarks";

    private static final String MOVIEVIEW_TABLE = "movieview_database";

    public static final Uri BOOKMARK_CONTENT_URI = Uri
            .parse("content://com.sprd.gallery3d/bookmarks");

    private static final int URI_MATCH_MOVIEVIEW = 0;

    private static final int URL_MATCH_MOVIEVIEW_ID = 1;

    private static final int URI_MATCH_BOOKMARKS = 2;

    private static final int URI_MATCH_BOOKMARKS_ID = 3;

    private static final UriMatcher URI_MATCHER;
    static {
        URI_MATCHER = new UriMatcher(UriMatcher.NO_MATCH);
        URI_MATCHER.addURI("com.sprd.gallery3d", "movieview",
                URI_MATCH_MOVIEVIEW);
        URI_MATCHER.addURI("com.sprd.gallery3d", "movieview" + "/#",
                URL_MATCH_MOVIEVIEW_ID);
        URI_MATCHER.addURI("com.sprd.gallery3d", "bookmarks",
                URI_MATCH_BOOKMARKS);
        URI_MATCHER.addURI("com.sprd.gallery3d", "bookmarks" + "/#",
                URI_MATCH_BOOKMARKS_ID);
    }

    private DatabaseHelper mDbHelper;

    public static final Uri PREFERRED_APN_URI = Uri
            .parse("content://telephony/carriers/preferapn");

    public static final Uri APN_LIST_URI = Uri
            .parse("content://telephony/carriers");

    public static final Uri CURRENT_APN_URI = Uri
            .parse("content://telephony/carriers/current");

    public class DatabaseHelper extends SQLiteOpenHelper {
        private static final int VERSION = 1;

        public DatabaseHelper(Context context, String name, int version) {
            super(context, name, null, version);
        }

        public DatabaseHelper(Context context, String name) {
            this(context, name, VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            try {
                db.execSQL("create table movieview_database("
                        + "_id INTEGER PRIMARY KEY," + "name TEXT,"
                        + "max_port TEXT," + "min_port TEXT,"
                        + "net_info TEXT," + "conn_prof TEXT,"
                        + "http_proxy TEXT," + "http_port TEXT,"
                        + "rtsp_proxy TEXT," + "rtsp_port TEXT,"
                        + "proxy_enable TEXT" + ");");
                SQLiteStatement statement = db
                        .compileStatement("insert "
                                + "into movieview_database(name,max_port,min_port,net_info,"
                                + "conn_prof,http_proxy,http_port,rtsp_proxy,rtsp_port,proxy_enable) "
                                + "values(?,?,?,?,?,?,?,?,?,?)");
                int index = 1;
                statement.bindString(index++, "Streaming");
                //SPRD:Bug 608339 set RTP port value between 1024~65535@{
                //statement.bindString(index++, "65534");
                //statement.bindString(index++, "8192");
                statement.bindString(index++, "65535");
                statement.bindString(index++, "1024");
               //Bug 608339 @}
                statement.bindString(index++, "EGPRS,10,100");
                statement.bindString(index++, "cmwap");
                statement.bindString(index++, "");
                statement.bindString(index++, "");
                statement.bindString(index++, "");
                statement.bindString(index++, "");
                statement.bindString(index++, "0");
                statement.execute();
                statement.close();

                db.execSQL("CREATE TABLE bookmarks ("
                        + "_id INTEGER PRIMARY KEY," + "title TEXT,"
                        + "url TEXT NOT NULL" + ");");

            } catch (IllegalArgumentException e) {
                Log.d(TAG, "tables create failed: " + e);
            }
        }

        @Override
        public void onUpgrade(SQLiteDatabase arg0, int arg1, int arg2) {
        }
    }

    public static String DmGetPreferredApn(Context context) {
        String apnString = null;
        Cursor cr = context.getContentResolver().query(PREFERRED_APN_URI, null,
                null, null, null);
        if (cr != null && cr.moveToNext()) {
            String id = cr.getString(cr.getColumnIndex("_id"));
            String apn = cr.getString(cr.getColumnIndex("apn"));
            String type = cr.getString(cr.getColumnIndex("type"));
            Log.i(TAG, "DmGetPreferredApn   apnId:" + id + "apnName:" + apn
                    + "apnType:" + type);
            apnString = apn;
        }
        if (cr != null) {
            cr.close();
        }
        return apnString;
    }

    public static void MovieViewDmHandleBroadcastReceiver(Context context,
            Intent intent) {

        if ("com.android.dm.vdmc".equals(intent.getAction())) {

            ContentValues values = new ContentValues();
            values.put(intent.getStringExtra("type"), intent
                    .getStringExtra("value"));
            Log.d("MovieViewDmHandleBroadcastReceiver", "DM update type="
                    + intent.getStringExtra("type") + "  value="
                    + intent.getStringExtra("value"));
            context.getContentResolver()
                    .update(CONTENT_URI, values, null, null);
            context.getContentResolver().notifyChange(CONTENT_URI, null);

            if (intent.getStringExtra("type").equals("conn_prof")) {
                Log.i(TAG, "DmSetPreferredApn" + intent.getStringExtra("type")
                        + intent.getStringExtra("value"));
                DmSetPreferredApn(context, intent.getStringExtra("value"));
            }
        }
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        Log.d(TAG, "delete : " + uri + ", selection: " + selection);
        final SQLiteDatabase db = mDbHelper.getWritableDatabase();
        int match = URI_MATCHER.match(uri);
        int deleted = 0;
        switch (match) {
        case URI_MATCH_BOOKMARKS_ID: {
            selection = DatabaseUtils.concatenateWhere(selection, "_id=?");
            selectionArgs = DatabaseUtils.appendSelectionArgs(selectionArgs,
                    new String[] { Long.toString(ContentUris.parseId(uri)) });
        }
        case URI_MATCH_BOOKMARKS: {
            deleted = db.delete(BOOKMARKS_TABLE, selection, selectionArgs);
        }
            break;
        default: {
            Log.e(TAG, "Unknown delete URI " + uri);
        }
        }
        if (deleted > 0) {
            getContext().getContentResolver().notifyChange(uri, null);
            Log.d(TAG, "delete db");
        }
        return deleted;
    }

    @Override
    public String getType(Uri uri) {
        Log.d("getType", "donothing");
        final int match = URI_MATCHER.match(uri);
        switch (match) {
        case URI_MATCH_BOOKMARKS:
            return "vnd.android.cursor.dir/video_bookmark";
        case URI_MATCH_BOOKMARKS_ID:
            return "vnd.android.cursor.item/video_bookmark";
        default:
            Log.e(TAG, "invalid uri: " + uri);
            return null;
        }
    }

    @Override
    public Uri insert(Uri uri, ContentValues contentValues) {
        Log.d(TAG, "insert : " + uri + ", valuse: " + contentValues);
        final SQLiteDatabase db = mDbHelper.getWritableDatabase();
        int match = URI_MATCHER.match(uri);
        Uri newUri = null;
        switch (match) {
        case URI_MATCH_BOOKMARKS: {
            long id = db.insert(BOOKMARKS_TABLE, null, contentValues);
            if (id > 0) {
                newUri = ContentUris.withAppendedId(uri, id);
            }
        }
            break;

        default: {
            Log.e(TAG, "Unknown insert URI " + uri);
        }
        }
        if (newUri != null) {
            getContext().getContentResolver().notifyChange(uri, null);
        }
        return newUri;
    }

    @Override
    public boolean onCreate() {
        mDbHelper = new DatabaseHelper(this.getContext(), "movieview_database");
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
            String[] selectionArgs, String sortOrder) {
        Log.d(TAG, "query : " + uri + ", selection: " + selection);
        final SQLiteDatabase db = mDbHelper.getReadableDatabase();
        int match = URI_MATCHER.match(uri);
        Cursor cursor = null;
        switch (match) {
        case URI_MATCH_BOOKMARKS_ID: {
            selection = DatabaseUtils.concatenateWhere(selection, "_id=?");
            selectionArgs = DatabaseUtils.appendSelectionArgs(selectionArgs,
                    new String[] { Long.toString(ContentUris.parseId(uri)) });
        }
        case URI_MATCH_BOOKMARKS: {
            cursor = db.query(BOOKMARKS_TABLE, projection, selection,
                    selectionArgs, null, null, sortOrder, null);
            if (cursor != null) {
                cursor.setNotificationUri(getContext().getContentResolver(),
                        BOOKMARK_CONTENT_URI);
            }
        }
            break;
        case URL_MATCH_MOVIEVIEW_ID: {
            selection = DatabaseUtils.concatenateWhere(selection, "_id=?");
            selectionArgs = DatabaseUtils.appendSelectionArgs(selectionArgs,
                    new String[] { Long.toString(ContentUris.parseId(uri)) });
        }
        case URI_MATCH_MOVIEVIEW: {
            cursor = db.query(MOVIEVIEW_TABLE, projection, selection,
                    selectionArgs, null, null, sortOrder, null);
        }
            break;
        default: {
            Log.e(TAG, "Unknown delete URI " + uri);
        }
        }

        return cursor;
    }

    @Override
    public int update(Uri uri, ContentValues contentValues, String selection,
            String[] selectionArgs) {
        final SQLiteDatabase db = mDbHelper.getReadableDatabase();
        int match = URI_MATCHER.match(uri);
        int count = 0;
        switch (match) {
        case URI_MATCH_BOOKMARKS_ID: {
            selection = DatabaseUtils.concatenateWhere(selection, "_id=?");
            selectionArgs = DatabaseUtils.appendSelectionArgs(selectionArgs,
                    new String[] { Long.toString(ContentUris.parseId(uri)) });
        }
        case URI_MATCH_BOOKMARKS: {
            count = db.update(BOOKMARKS_TABLE, contentValues, selection,
                    selectionArgs);
        }
            break;
        case URL_MATCH_MOVIEVIEW_ID: {
            selection = DatabaseUtils.concatenateWhere(selection, "_id=?");
            selectionArgs = DatabaseUtils.appendSelectionArgs(selectionArgs,
                    new String[] { Long.toString(ContentUris.parseId(uri)) });
        }
        case URI_MATCH_MOVIEVIEW: {
            count = db.update(MOVIEVIEW_TABLE, contentValues, selection,
                    selectionArgs);
        }
            break;
        default: {
            Log.e(TAG, "Unknown update URI " + uri);
        }
        }

        if (count > 0) {
            getContext().getContentResolver().notifyChange(uri, null);
        }
        return count;
    }

    public static void DmSetPreferredApn(Context context, String apn_name) {
        String cmwap_id = null;
        String prefer_apn = DmGetPreferredApn(context);
        String set_apn = apn_name != null ? apn_name.toLowerCase() : null;

        if (prefer_apn != null && set_apn != null
                && !prefer_apn.toLowerCase().equals(set_apn)) {
            Cursor cr = context.getContentResolver().query(CURRENT_APN_URI,
                    null, null, null, null);
            while (cr != null && cr.moveToNext()) {
                String id = cr.getString(cr.getColumnIndex("_id"));
                String apn = cr.getString(cr.getColumnIndex("apn"));
                String type = cr.getString(cr.getColumnIndex("type"));
                Log.i(TAG, "apnId:" + id + " apnName:" + apn + " apnType:"
                        + type);

                if (apn != null && apn.toLowerCase().equals(set_apn)) {
                    Log.i(TAG, "need to  set apnId :" + id);
                    cmwap_id = new String(id);
                    break;
                }
            }
            if (cr != null) {
                cr.close();
            }
            if (cmwap_id != null) {
                ContentValues values = new ContentValues();
                values.put("apn_id", cmwap_id);
                context.getContentResolver().update(PREFERRED_APN_URI, values,
                        null, null);
            } else {
                Toast.makeText(
                        context,
                        "current apn list has not  " + set_apn
                                + "apn ,set apn failured!", 1000).show();
            }
        }
    }
}
