
package com.sprd.engineermode.debuglog.slogui;

/*
 append to AndroidManifest.xml
 <provider
 android:name="com.sprd.engineermode.debuglog.slogui.slogui.SlogProvider"
 android:authorities="com.sprd.engineermode"
 android:exported="false" >
 <grant-uri-permission android:pathPattern=".*" />
 </provider>
 */

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.provider.BaseColumns;
import android.util.Log;

public class SlogProvider extends ContentProvider {
    private static final String TAG = "Provider";

    public static final String SCHEME = "content://";
    public static final String AUTHORITY = "com.sprd.engineermode";

    public static final String CONTENT_TYPE = "vnd.spreadtrum.cursor.dir/vnd.spreadtrum.slog";
    public static final String CONTENT_ITEM_TYPE = "vnd.spreadtrum.cursor.item/vnd.spreadtrum.slog";

    public static final String PATH_ID_modes = "/modes/";
    public static final String PATH_modes = "/modes";
    public static final Uri URI_ID_MODES = Uri.parse(SCHEME + AUTHORITY
            + PATH_ID_modes);
    public static final Uri URI_MODES = Uri.parse(SCHEME + AUTHORITY
            + PATH_modes);
    private static final int N_modes = 0 * 2;
    private static final int N_ID_modes = 0 * 2 + 1;

    private DatabaseHelper mDb;
    private static final UriMatcher sUriMatcher;

    static {
        sUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
        sUriMatcher.addURI(AUTHORITY, "modes", N_modes);
        sUriMatcher.addURI(AUTHORITY, "modes/#", N_ID_modes);
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        SQLiteDatabase db = mDb.getWritableDatabase();
        String where = selection;
        int count = 0;

        switch (sUriMatcher.match(uri)) {
            case N_modes:
                count = db.delete(Contract.TABLE_modes, selection, selectionArgs);
                break;
            case N_ID_modes:
                where = Contract._ID + " = " + uri.getPathSegments().get(1);

                if (selection != null) {
                    where += " AND " + selection;
                }
                count = db.delete(Contract.TABLE_modes, where, selectionArgs);
                break;

            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
        }

        getContext().getContentResolver().notifyChange(uri, null);
        return count;
    }

    @Override
    public String getType(Uri uri) {
        switch (sUriMatcher.match(uri)) {
            case N_modes:
                return CONTENT_TYPE;

            case N_ID_modes:
                return CONTENT_ITEM_TYPE;

            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
        }
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        SQLiteDatabase db = mDb.getWritableDatabase();
        long rowId;
        Uri iuri;

        switch (sUriMatcher.match(uri)) {
            case N_modes:
                rowId = db.insert(Contract.TABLE_modes, Contract._ID, values);
                iuri = ContentUris.withAppendedId(URI_ID_MODES, rowId);
                break;

            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
        }

        if (rowId > 0) {
            getContext().getContentResolver().notifyChange(iuri, null);
            return iuri;
        }

        throw new SQLException("Failed to insert row into " + uri);
    }

    @Override
    public boolean onCreate() {
        Log.d(TAG, "=====> onCreate, " + getContext());
        mDb = new DatabaseHelper(getContext());
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
            String[] selectionArgs, String sortOrder) {
        SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
        String where = selection;

        switch (sUriMatcher.match(uri)) {
            case N_modes:
                qb.setTables(Contract.TABLE_modes);
                break;
            case N_ID_modes:
                if (uri.getPathSegments().size() > 1) {
                    String tid = uri.getPathSegments().get(1);
                    where = Contract._ID + " = " + tid;
                    if (selection != null) {
                        where += " AND " + selection;
                    }
                }
                qb.setTables(Contract.TABLE_modes);
                break;

            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
        }

        SQLiteDatabase db = mDb.getReadableDatabase();
        Cursor c = qb.query(db, projection, where, selectionArgs, null, null,
                sortOrder);
        c.setNotificationUri(getContext().getContentResolver(), uri);
        return c;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        SQLiteDatabase db = mDb.getWritableDatabase();
        int count = 0;
        String where;

        switch (sUriMatcher.match(uri)) {
            case N_modes:
                count = db.update(Contract.TABLE_modes, values, selection,
                        selectionArgs);
                break;

            case N_ID_modes: {
                String tid = uri.getPathSegments().get(1);
                where = Contract._ID + " = " + tid;
                if (selection != null) {
                    where += " AND " + selection;
                }
                count = db.update(Contract.TABLE_modes, values, where,
                        selectionArgs);
            }
                break;

            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
        }

        getContext().getContentResolver().notifyChange(uri, null);
        return count;
    }

    public static class Contract implements BaseColumns {
        public final static String DB_NAME = "slog.db";
        public final static int DB_VERSION = 3;
        public final static String TABLE_modes = "mode";
        public final static String COLUMN_DELETABLE = "deletable";

        public final static String COLUMN_MODE = "name";// "mode";
        public final static String COLUMN_GENERAL = "general";
        public final static String COLUMN_MAIN = SlogAction.MAINKEY.replace(
                "\t", "_");
        public final static String COLUMN_EVENT = SlogAction.EVENTKEY.replace(
                "\t", "_");
        public final static String COLUMN_RADIO = SlogAction.RADIOKEY.replace(
                "\t", "_");
        public final static String COLUMN_KERNEL = SlogAction.KERNELKEY
                .replace("\t", "_");
        public final static String COLUMN_SYSTEM = SlogAction.SYSTEMKEY
                .replace("\t", "_");
        public final static String COLUMN_MODEM = SlogAction.MODEMKEY.replace(
                "\t", "_");
//        public final static String COLUMN_CP0 = SlogAction.CP0KEY.replace(
//                "\t", "_" , "-", "_");
//        public final static String COLUMN_CP1 = SlogAction.CP1KEY.replace(
//                "\t", "_" , "-", "_");
//        public final static String COLUMN_CP2 = SlogAction.CP2KEY.replace(
//                "\t", "_" , "-", "_");
//        public final static String COLUMN_CP3 = SlogAction.CP3KEY.replace(
//                "\t", "_" , "-", "_");
//        public final static String COLUMN_CP4 = SlogAction.CP4KEY.replace(
//                "\t", "_" , "-", "_");
//        public final static String COLUMN_CP5 = SlogAction.CP5KEY.replace(
//                "\t", "_" , "-", "_");
        public static final String COLUMN_CP0 = "stream_cp_wcdma_";
        public static final String COLUMN_CP1 = "stream_cp_td_scdma_";
        public static final String COLUMN_CP2 = "stream_cp_wcn_";
        public static final String COLUMN_CP3 = "stream_cp_td_lte_";
        public final static String COLUMN_CP4 = "stream_cp_tdd_lte_"; 
        public final static String COLUMN_CP5 = "stream_cp_fdd_lte_";
        public final static String COLUMN_TCP = SlogAction.TCPKEY.replace("\t",
                "_");
        public final static String COLUMN_BLUETOOTH = SlogAction.BLUETOOTHKEY
                .replace("\t", "_");
        public final static String COLUMN_HWWATCHDOG = SlogAction.HWWATCHDOGKEY
                .replace("\t", "_");
        // public final static String COLUMN_MISC =
        // SlogAction.MISCKEY.replace("\t", "_");
        public final static String COLUMN_STORAGE = SlogAction.STORAGEKEY
                .replace("\t", "_");

        public final static String COLUMN_SYSDUMP = SlogAction.SYSDUMPKEY
                .replace("\t", "_");
        public final static String COLUMN_COREDUMP = SlogAction.COREDUMPKEY
                .replace("\t", "_");
        public final static String COLUMN_HPROFS = SlogAction.HPROFSKEY
                .replace("\t", "_");
        public final static String COLUMN_SPRDDEBUG = SlogAction.SPRDDEBUGKEY
                .replace("\t", "_");

        public final static int VALUE_GENERAL_ENABLE = 1;
        public final static int VALUE_GENERAL_DISABLE = 2;
        public final static int VALUE_GENERAL_LOW = 3;

    }

    private static class DatabaseHelper extends SQLiteOpenHelper {
        DatabaseHelper(Context context) {
            super(context, Contract.DB_NAME, null, Contract.DB_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {

            db.execSQL("CREATE TABLE " + Contract.TABLE_modes + " ("
                    + Contract.COLUMN_MODE + " TEXT NOT NULL,"
                    + Contract.COLUMN_DELETABLE + " INTEGER NOT NULL,"
                    + Contract.COLUMN_GENERAL + " TEXT NOT NULL,"
                    + Contract.COLUMN_MAIN + " INTEGER NOT NULL,"
                    + Contract.COLUMN_EVENT + " INTEGER NOT NULL,"
                    + Contract.COLUMN_RADIO + " INTEGER NOT NULL,"
                    + Contract.COLUMN_KERNEL + " INTEGER NOT NULL,"
                    + Contract.COLUMN_SYSTEM + " INTEGER NOT NULL,"
                    + Contract.COLUMN_MODEM + " INTEGER NOT NULL,"
                    + Contract.COLUMN_CP0 + " INTEGER NOT NULL,"
                    + Contract.COLUMN_CP1 + " INTEGER NOT NULL,"
                    + Contract.COLUMN_CP2 + " INTEGER NOT NULL,"
                    + Contract.COLUMN_CP3 + " INTEGER NOT NULL,"
                    + Contract.COLUMN_CP4 + " INTEGER NOT NULL,"
                    + Contract.COLUMN_CP5 + " INTEGER NOT NULL,"
                    + Contract.COLUMN_TCP
                    + " INTEGER NOT NULL,"
                    + Contract.COLUMN_BLUETOOTH
                    + " INTEGER NOT NULL,"
                    + Contract.COLUMN_HWWATCHDOG
                    + " INTEGER NOT NULL,"
                    // + Contract.COLUMN_MISC + " INTEGER NOT NULL,"
                    + Contract.COLUMN_STORAGE + " INTEGER NOT NULL,"
                    + Contract.COLUMN_SYSDUMP + " INTEGER NOT NULL,"
                    + Contract.COLUMN_COREDUMP + " INTEGER NOT NULL,"
                    + Contract.COLUMN_HPROFS + " INTEGER NOT NULL,"
                    + Contract.COLUMN_SPRDDEBUG + " INTEGER NOT NULL,"

                    + Contract._ID + " INTEGER PRIMARY KEY);");

            db.execSQL("INSERT INTO " + Contract.TABLE_modes + " VALUES ("
                    + "'All Logs'" + "," + 0 + ",'" + SlogAction.GENERALON + "'," + 1 + "," + 1 + ","
                    + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1
                    + "," + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1 + ","
                    + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1 + ","
                    + 1 + "," +  1
                    + ")");
            db.execSQL("INSERT INTO " + Contract.TABLE_modes + " VALUES ("
                    + "'Android Log Only'" + "," + 0 + ",'" + SlogAction.GENERALON  + "'," + 1 + "," + 1 + ","
                    + 1 + "," + 1 + "," + 1 + "," + 0 + "," + 0
                    + "," + 0 + "," + 0 + "," + 0 + "," + 0 + "," + 0 + "," + 0 + "," + 0 + ","
                    + 0 + "," + 1 + "," + 0 + "," + 0 + "," + 0 + ","
                    + 0 +"," + 2
                    + ")");
            db.execSQL("INSERT INTO " + Contract.TABLE_modes + " VALUES ("
                    + "'Modem Related'" + "," + 0 + ",'" + SlogAction.GENERALON  + "'," + 1 + "," + 1 + ","
                    + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1
                    + "," + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 0 + ","
                    + 1 + "," + 1 + "," + 0 + "," + 0 + "," + 0 + ","
                    + 0 +"," +   3
                    + ")");
            db.execSQL("INSERT INTO " + Contract.TABLE_modes + " VALUES ("
                    + "'Connectivity'" + "," + 0 + ",'" + SlogAction.GENERALON  + "'," + 1 + "," + 1 + ","
                    + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1
                    + "," + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1 + "," + 1 + ","
                    + 1 + "," + 1 + "," + 0 + "," + 0 + "," + 0 + ","
                    + 0 + "," + 4
                    + ")");
        }

        /*
         * db.execSQL("INSERT INTO " + Contract.TABLE_modes + " VALUES (" + name
         * + "," + deletable + "," + general + "," + mainlog + "," + eventlog +
         * "," + radiolog + "," + kernellog + "," + systemlog + "," + modemlog +
         * "," + cp0 + "," + cp1 + "," + cp2 + "," + cp3 + "," + caplog + "," +
         * bluetooth + "," + hwwatchdog + "," + storage + "," + systemdump + "," + hprofile + ","
         * + coredump + "," + sprddebug + "," + id + ")");
         */

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            db.execSQL("DROP TABLE IF EXISTS " + Contract.TABLE_modes);
            onCreate(db);
        }
    }
}
