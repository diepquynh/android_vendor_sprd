package com.sprd.generalsecurity.data;

import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

import java.lang.String;


public class AppInfoTable {
    public static final String TABLE_APP_INFO = "app_info";
    public static final String COLUMN_ID = "_id";
    public static final String COLUMN_PKG_NAME = "packagename";
    public static final String COLUMN_UID = "uid";
    public static final String COLUMN_BLOCK_STATE = "blockstate";

    private static final String DATABASE_CREATE = "create table "
            + TABLE_APP_INFO
            + "("
            + COLUMN_ID + " integer primary key autoincrement, "
            + COLUMN_PKG_NAME + " text, "
            + COLUMN_UID + " integer, "
            + COLUMN_BLOCK_STATE + " integer "
            + ");";

    public static void onCreate(SQLiteDatabase db) {
        db.execSQL(DATABASE_CREATE);
    }

    public static void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {

    }
}