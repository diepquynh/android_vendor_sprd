package com.sprd.soundrecorder;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

public class RecordDatabaseHelper extends SQLiteOpenHelper {

    private static final String TAG = "RecordDatabase";
    private static final String DATABASE_NAME = "record.db";
    public static final String TABLE_WAVE = "waves";
    public static final String TABLE_TAG = "tags";
    public static final int DATABASE_VERSION = 1;

    static final String _ID = "_id";
    static final String RECORD_TITLE = "title";
    static final String RECORD_WAVE = "wave";
    static final String RECORD_TAG = "tag";
    static final String TAG_LOCATION = "location";

    private SQLiteDatabase db;

    public RecordDatabaseHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL("CREATE TABLE " + TABLE_WAVE + " (" +_ID +" INTEGER PRIMARY KEY," + RECORD_TITLE +" TEXT," + RECORD_WAVE + " REAL," + RECORD_TAG + " INTEGER);");
        db.execSQL("CREATE TABLE " + TABLE_TAG + " (" +_ID +" INTEGER PRIMARY KEY," + RECORD_TITLE +" TEXT," + RECORD_TAG + " INTEGER," + TAG_LOCATION + " INTEGER);");
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        db.execSQL("DROP TABLE IF EXISTS "+TABLE_WAVE+";");
        db.execSQL("DROP TABLE IF EXISTS "+TABLE_TAG+";");
        onCreate(db);
    }

    public long insert(String title, float wave, int tag) {
        db = this.getWritableDatabase();
        ContentValues cv = new ContentValues();
        cv.put(RECORD_TITLE, title);
        cv.put(RECORD_WAVE, wave);
        cv.put(RECORD_TAG, tag);
        long row = db.insert(TABLE_WAVE, null, cv);
        db.close();
        return row;
    }

    public long insertTAG(String title, int tag, int location) {
        db = this.getWritableDatabase();
        ContentValues cv = new ContentValues();
        cv.put(RECORD_TITLE, title);
        cv.put(RECORD_TAG, tag);
        cv.put(TAG_LOCATION, location);
        long row = db.insert(TABLE_TAG, null, cv);
        db.close();
        return row;
    }

    public void delete(String title) {
        db = this.getWritableDatabase();
        db.delete(TABLE_WAVE, RECORD_TITLE+"=?", new String[]{title});
        db.delete(TABLE_TAG, RECORD_TITLE+"=?", new String[]{title});
        db.close();
    }

    public void update(String oldTitle, String newTitle) {
        db = this.getWritableDatabase();
        ContentValues cv = new ContentValues();
        cv.put(RECORD_TITLE, newTitle);
        db.update(TABLE_WAVE, cv, RECORD_TITLE+"=?", new String[]{oldTitle});
        db.update(TABLE_TAG, cv, RECORD_TITLE+"=?", new String[]{oldTitle});
        db.close();
    }

    public Cursor query(String title) {
        db = this.getReadableDatabase();
        Cursor cursor = db.query(TABLE_WAVE, new String[]{RECORD_WAVE, RECORD_TAG}, RECORD_TITLE+"=?", new String[]{title}, null, null, null);
        //db.close();
        return cursor;
    }

    public Cursor queryTag(String title) {
        db = this.getReadableDatabase();
        Cursor cursor = db.query(TABLE_TAG, new String[]{RECORD_TAG, TAG_LOCATION}, RECORD_TITLE+"=?", new String[]{title}, null, null, null);
        //db.close();
        return cursor;
    }
}
