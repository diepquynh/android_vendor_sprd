/*
 * Copyright (C) 2008 Esmertec AG.
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.mmslib;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.net.Uri;
import android.util.Log;

//import com.android.util.LogUtil;

// Wrapper around content resolver methods to catch exceptions
public final class SqliteWrapper {
    private static final String TAG = "SqliteWrapper";

    private SqliteWrapper() {
        // Forbidden being instantiated.
    }

    public static Cursor query(Context context, ContentResolver resolver, Uri uri,
            String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        try {
            return resolver.query(uri, projection, selection, selectionArgs, sortOrder);
        } catch (SQLiteException e) {
            Log.e(TAG, "SqliteWrapper: catch an exception when query", e);
            return null;
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "SqliteWrapper: catch an exception when query", e);
            return null;
        }
    }

    public static int update(Context context, ContentResolver resolver, Uri uri,
            ContentValues values, String where, String[] selectionArgs) {
        try {
            return resolver.update(uri, values, where, selectionArgs);
        } catch (SQLiteException e) {
            Log.e(TAG, "SqliteWrapper: catch an exception when update", e);
            return -1;
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "SqliteWrapper: catch an exception when update", e);
            return -1;
        }
    }

    public static int delete(Context context, ContentResolver resolver, Uri uri,
            String where, String[] selectionArgs) {
        try {
            return resolver.delete(uri, where, selectionArgs);
        } catch (SQLiteException e) {
            Log.e(TAG, "SqliteWrapper: catch an exception when delete", e);
            return -1;
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "SqliteWrapper: catch an exception when delete", e);
            return -1;
        }
    }

    public static Uri insert(Context context, ContentResolver resolver,
            Uri uri, ContentValues values) {
        try {
            return resolver.insert(uri, values);
        } catch (SQLiteException e) {
            Log.e(TAG, "SqliteWrapper: catch an exception when insert", e);
            return null;
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "SqliteWrapper: catch an exception when insert", e);
            return null;
        }
    }
}
