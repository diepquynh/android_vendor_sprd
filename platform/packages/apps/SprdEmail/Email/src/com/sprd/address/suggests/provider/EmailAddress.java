
package com.sprd.address.suggests.provider;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.text.TextUtils;
import android.util.Log;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * SPRD: 523599. Add EmailAddress for address table.
 */
public final class EmailAddress extends EmailAddressContent implements
        EmailAddressContent.AddressColumns {
    private static final String TAG = "EmailAddress";

    public static final String TABLE_NAME = "Address";

    public static final Uri CONTENT_URI = Uri.parse(EmailAddressContent.CONTENT_URI + "/address");

    // define the items of the address table
    public String mAddress;

    public static final int CONTENT_ADDRESS_COLUMN = 1;

    static String[] CONTENT_PROJECTION = new String[] {
            AddressColumns.ID, AddressColumns.EMAIL_ADDRESS, AddressColumns.ADDRESS_TYPE
    };

    public String getEmailAddress() {
        return mAddress;
    }

    public void setEmailAddress(String emailAddress) {
        mAddress = emailAddress;
    }

    @Override
    public ContentValues toContentValues() {
        ContentValues values = new ContentValues();
        values.put(AddressColumns.EMAIL_ADDRESS, mAddress);
        return values;
    }

    @Override
    public void restore(Cursor cursor) {
        mAddress = cursor.getString(CONTENT_ADDRESS_COLUMN);
    }

    public static boolean isValidEmailAddress(String address) {
        Pattern p = Pattern
                .compile("^((\\u0022.+?\\u0022@)|(([\\Q-!#$%&'*+/=?^`{}|~\\E\\w])+(\\.[\\Q-!#$%&'*+/=?^`{}|~\\E\\w]+)*@))"
                        + "((\\[(\\d{1,3}\\.){3}\\d{1,3}\\])|(((?=[0-9a-zA-Z])[-\\w]*(?<=[0-9a-zA-Z])\\.)+[a-zA-Z]{2,6}))$");
        Matcher m = p.matcher(address);
        return m.matches();
    }

    public static void saveAddress(Context context, String text, String type) {
        ContentValues cv = new ContentValues();
        cv.put(AddressColumns.EMAIL_ADDRESS, text);
        cv.put(AddressColumns.ADDRESS_TYPE, type);
        Uri res = context.getContentResolver().insert(CONTENT_URI, cv);
        if (res == null) {
            Log.v(TAG, "address is invalid");
        }
    }

    public static Cursor queryAddress(Context context, Uri uri, String emailType, String from) {
        StringBuilder selection = null;
        String[] selectionArgs = null;
        if (!TextUtils.isEmpty(from)) {
            selection = new StringBuilder();
            selection.append(AddressColumns.ADDRESS_TYPE + "=?");
            selectionArgs = new String[] {
                from
            };
        }
        if (!TextUtils.isEmpty(emailType)) {
            if (selection == null) {
                selection = new StringBuilder();
                selection.append(AddressColumns.EMAIL_ADDRESS + " LIKE ?");
            } else {
                selection.append(" and ");
                selection.append(AddressColumns.EMAIL_ADDRESS + " LIKE ?");
            }
            if (selectionArgs == null) {
                selectionArgs = new String[] {
                    "%" + emailType
                };
            } else {
                selectionArgs[1] = "%" + emailType;
            }

        }
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, EmailAddress.CONTENT_PROJECTION,
                    selection != null ? selection.toString() : null, selectionArgs, null);
        } catch (android.database.sqlite.SQLiteDiskIOException ex) {
            ex.printStackTrace();
        } finally {
            return cursor;
        }
    }
}
