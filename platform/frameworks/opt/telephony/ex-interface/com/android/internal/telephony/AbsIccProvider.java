package com.android.internal.telephony;

import android.content.ContentValues;
import android.net.Uri;
import android.database.Cursor;

/**
 * {@hide}
 */
public class AbsIccProvider {

    public AbsIccProvider() {

    }

    public Cursor query (Uri url, String[] projection, String selection,
            String[] selectionArgs, String sort) {
        return null;
    }

    public Uri insert (Uri url, ContentValues initialValues) {
        return null;
    }

    public int delete (Uri url, String where, String[] whereArgs) {
        return 0;
    }

    public int update (Uri url, ContentValues values, String where, String[] whereArgs) {
        return 0;
    }

    public String getType (Uri url) {
        return null;
    }
}
