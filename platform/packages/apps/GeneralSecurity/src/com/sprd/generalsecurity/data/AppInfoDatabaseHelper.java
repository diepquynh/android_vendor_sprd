package com.sprd.generalsecurity.data;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.lang.String;


public class AppInfoDatabaseHelper extends SQLiteOpenHelper {
    private static final String DATABASE_NAME = "appinfo.db";
    private static final int DATABASE_VERSION = 1;

    public AppInfoDatabaseHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        AppInfoTable.onCreate(db);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        AppInfoTable.onUpgrade(db, oldVersion, newVersion);

    }
}