
package com.sprd.address.suggests.provider;

import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.net.Uri;
import android.util.Log;

/**
 * SPRD: 523599. Add EmailAddressContent for email address history
 */
public abstract class EmailAddressContent {

    public static final String AUTHORITY = "com.sprd.email.provider.history";

    public static final Uri CONTENT_URI = Uri.parse("content://" + AUTHORITY);

    public static final String TAG = "EmailAddrerssContent";

    // define the interface for using the name of the items
    public interface AddressColumns {
        String ID = "_id";

        String EMAIL_ADDRESS = "emailAddress";

        String ADDRESS_TYPE = "addressType";
    }

    // Write the Content into a ContentValues container
    public abstract ContentValues toContentValues();

    // Read the Content from a ContentCursor
    public abstract void restore(Cursor cursor);

}
