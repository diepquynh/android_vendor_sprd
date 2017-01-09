
package com.sprd.note.data;

import java.util.Date;

import com.sprd.note.utils.LogUtils;

import android.content.ContentValues;
import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.os.Debug;
import android.util.Log;

public class DBOpenHelper extends SQLiteOpenHelper {
    private static final int DB_VERSION = 11;
    private static final String DB_NAME = "Notes.db";
    // table name in db
    public static final String TABLE_NAME = "items";

    public static final String ID = "_id";
    public static final String NOTE_CONTENT = "content";
    public static final String UPDATE_DATE = "cdate";
    public static final String UPDATE_TIME = "ctime";
    public static final String NOTE_IS_FOLDER = "isfilefolder";
    public static final String NOTE_PARENT_FOLDER = "parentfile";
    public static final String NOTE_UPDATE_DATE = "cdata_long";
    public static final String NOTE_TITLE = "title";

    public static final String[] NOTE_ALL_COLUMS = new String[] {
            ID,
            NOTE_CONTENT, NOTE_IS_FOLDER, NOTE_PARENT_FOLDER, NOTE_TITLE, NOTE_UPDATE_DATE
    };

    private static DBOpenHelper helper = null;

    public static synchronized DBOpenHelper getInstance(Context context){
        if (helper == null) {
            helper = new DBOpenHelper(context);
        }
        return helper;

    }

    public DBOpenHelper(Context context) {
        super(context, DB_NAME, null, DB_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {

        db.execSQL(" CREATE TABLE IF NOT EXISTS " + TABLE_NAME + " ( "
                + ID + " integer primary key autoincrement , "
                + NOTE_CONTENT + " text , "
                + NOTE_IS_FOLDER + " int , "
                + NOTE_PARENT_FOLDER + " varchar, "
                + NOTE_TITLE + " text, "
                + NOTE_UPDATE_DATE + " long);");
        if (LogUtils.DEBUG) {
            Log.v("you", "Create Table: " + TABLE_NAME);
        }
    }

    public static interface ColumnsIndex {
        public int ID = 0;
        public int NOTE_CONTENT = 1;
        public int NOTE_IS_FOLDER = 2;
        public int NOTE_PARENT_FOLDER = 3;
        public int NOTE_TITLE = 4;
        public int NOTE_UPDATE_DATE = 5;
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        db.execSQL(" DROP TABLE IF EXISTS " + TABLE_NAME);
        onCreate(db);
    }

}
