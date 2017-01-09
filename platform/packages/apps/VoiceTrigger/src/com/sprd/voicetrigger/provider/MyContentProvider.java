package com.sprd.voicetrigger.provider;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.net.Uri;
import android.util.Log;

import com.sprd.voicetrigger.SensibilityActivity;

public class MyContentProvider extends ContentProvider {
    private static final String TAG = "MyContentProvider";
    private static final int TABLE_DIR = 0;
    private static final int TABLE_ITEM = 1;
    private static UriMatcher matcher;

    static {
        matcher = new UriMatcher(UriMatcher.NO_MATCH);
        matcher.addURI(MyProviderMetaData.AUTHORITY, "config", TABLE_DIR);
        matcher.addURI(MyProviderMetaData.AUTHORITY, "config/#", TABLE_ITEM);
    }

    private MySqliteHelper mySqliteHelper;

    public MyContentProvider() {
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        return 0;
    }

    @Override
    public String getType(Uri uri) {
        switch (matcher.match(uri)) {
            case TABLE_DIR:
                return MyProviderMetaData.ConfigTableMetaData.CONTENT_TYPE;
            case TABLE_ITEM:
                return MyProviderMetaData.ConfigTableMetaData.CONTENT_ITEM_TYPE;
        }
        return null;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        Uri uriReturn = null;
        SQLiteDatabase mDatabase = mySqliteHelper.getWritableDatabase();
        switch (matcher.match(uri)) {
            case TABLE_DIR:
            case TABLE_ITEM:
                long newid = mDatabase.insert(MyProviderMetaData.ConfigTableMetaData.TABLE_NAME, null, values);
                uriReturn = uri.parse("content://" + MyProviderMetaData.AUTHORITY + "/config" + newid);
                break;
            default:
                break;
        }
        return uriReturn;
    }

    @Override
    public boolean onCreate() {
        mySqliteHelper = new MySqliteHelper(getContext(), MyProviderMetaData.DATABASE_NAME);
        ContentValues mContentValues = new ContentValues();
        mContentValues.put(MyProviderMetaData.ConfigTableMetaData.SENSIBILITY, SensibilityActivity.SENSIBILITY_DEFAULT_VALUE);
        mContentValues.put(MyProviderMetaData.ConfigTableMetaData.ISOPENSWITCH, false);
        mContentValues.put(MyProviderMetaData.ConfigTableMetaData.ISDEFAULTMODE, true);
        insert(MyProviderMetaData.ConfigTableMetaData.CONTENT_URI, mContentValues);
        Log.i(TAG, "provider create");
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
                        String[] selectionArgs, String sortOrder) {
        SQLiteDatabase db = mySqliteHelper.getReadableDatabase();
        Cursor c = null;
        switch (matcher.match(uri)) {
            case TABLE_DIR:
                c = db.query(MyProviderMetaData.ConfigTableMetaData.TABLE_NAME,
                        projection, selection, selectionArgs, null, null, sortOrder);
                break;
            case TABLE_ITEM:
                String id = uri.getPathSegments().get(1);
                c = db.query(MyProviderMetaData.ConfigTableMetaData.TABLE_NAME,
                        projection, "_id = ?", new String[]{id}, null, null, sortOrder);
                break;
            default:
                throw new IllegalArgumentException("Unknown URI" + uri);

        }
        c.setNotificationUri(getContext().getContentResolver(), uri);
        return c;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection,
                      String[] selectionArgs) {
        int updaterows = 0;
        SQLiteDatabase mDatabase = mySqliteHelper.getWritableDatabase();
        switch (matcher.match(uri)) {
            case TABLE_DIR:
                updaterows = mDatabase.update(MyProviderMetaData.ConfigTableMetaData.TABLE_NAME,
                        values, selection, selectionArgs);
                break;
            case TABLE_ITEM:
                String id = uri.getPathSegments().get(1);
                updaterows = mDatabase.update(MyProviderMetaData.ConfigTableMetaData.TABLE_NAME,
                        values, "_id = ?", new String[]{id});
                break;
            default:
                break;
        }
        return updaterows;
    }

    public class MySqliteHelper extends SQLiteOpenHelper {

        public MySqliteHelper(Context context, String name, SQLiteDatabase.CursorFactory factory, int version) {
            super(context, name, factory, version);
        }

        public MySqliteHelper(Context context, String name, int verson) {
            this(context, name, null, verson);
        }

        public MySqliteHelper(Context context, String name) {
            this(context, name, MyProviderMetaData.DATABASE_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL(MyProviderMetaData.ConfigTableMetaData.SQL_CREATE_TABLE);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            db.execSQL("DROP TABLE IF EXISTS " + MyProviderMetaData.ConfigTableMetaData.TABLE_NAME);
            onCreate(db);
        }
    }


}
