
package com.sprd.providers.contacts;

import android.accounts.Account;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.Groups;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.telephony.PhoneNumberUtilsEx;
import android.util.Log;

import java.util.regex.Pattern;
import com.sprd.providers.contacts.SimContactProxy.SimContactCache;
import com.sprd.providers.contacts.SimContactProxy.SimContactGroupCache;
import com.android.providers.contacts.ContactsDatabaseHelper.Views;
import com.android.providers.contacts.ContactsDatabaseHelper;

public class SimContactProxyUtil {

    private static final String TAG = "SimContactProxyUtil";

    private ContactsDatabaseHelper mContactsHelper;

    private static SimContactProxyUtil sInstance;
    private ContentResolver mResolver;

    private SimContactCache mSimContactCache;
    private SimContactGroupCache mSimContactGroupCache;
    private ContactProxyManager mContactProxyManager;
    private final Pattern mPhoneNumPattern = Pattern.compile("[^0-9\\+,;N\\*#]");

    private SimContactProxyUtil(ContactProxyManager contactProxyManager, Context context) {
        mSimContactCache = contactProxyManager.getProxySimContactCache();
        mSimContactGroupCache = contactProxyManager.getProxySimContactGroupCache();
        mContactProxyManager = contactProxyManager;
        mResolver = context.getContentResolver();

        mContactsHelper = getDatabaseHelper(context);

    }

    synchronized public static SimContactProxyUtil getInstance(
            ContactProxyManager contactProxyManager, Context context) {
        if (sInstance == null) {
            sInstance = new SimContactProxyUtil(contactProxyManager, context);
        }
        return sInstance;
    }

    private ContactsDatabaseHelper getDatabaseHelper(final Context context) {
        return ContactsDatabaseHelper.getInstance(context);
    }

    public boolean isMyContact(long rawContactId) {
        return checkIsSim(rawContactId);
    }

    public boolean isMyContactGroup(long groupRowId) {
        return checkIsSimGroup(groupRowId);
    }

    private boolean checkIsSimGroup(long groupRowId) {
        if (mSimContactGroupCache.isSimContactGroup(groupRowId) == -1) {
            return false;
        }

        if (mSimContactGroupCache.isSimContactGroup(groupRowId) == 0) {
            buildSimContactGroupCache(groupRowId);
            if (mSimContactGroupCache.isSimContactGroup(groupRowId) != 1) {
                return false;
            }
        }
        Log.e(TAG, ">>>isMyContactGroup");
        return true;
    }

    private boolean checkIsSim(long rawContactId) {
        if (mSimContactCache.isSimContact(rawContactId) == -1) {
            return false;
        }
        if (mSimContactCache.isSimContact(rawContactId) == 0) {
            buildSimContactCache(rawContactId);
            if (mSimContactCache.isSimContact(rawContactId) != 1) {
                return false;
            }
        }
        Log.e(TAG, ">>>isMyContact");
        return true;
    }

    private void buildSimContactGroupCache(long origGroupRowId) {

        final SQLiteDatabase db = mContactsHelper.getWritableDatabase();

        if (mContactProxyManager.isProxyImporting()) {
            // when importing, onImport will fill the cache
            return;
        }
        Log.e(TAG, ">>>buildSimContactGroupCache");
        mSimContactGroupCache.remove(origGroupRowId);

        String[] projection = new String[] {
                Groups._ID, Groups.SYNC1, Groups.SYNC2, Groups.ACCOUNT_NAME, Groups.ACCOUNT_TYPE
        };

        Cursor cursor = db.query(Views.GROUPS, projection,
                Groups.ACCOUNT_TYPE + " like 'sprd%sim' and " + Groups._ID + " = " + origGroupRowId
                        + " and " + Groups.DELETED + " = 0", null, null, null,
                null);

        try {
            if (cursor != null && cursor.moveToFirst()) {
                do {
                    long groupRowId = cursor.getLong(0);
                    String groupIndex = cursor.getString(1);
                    String iccGroupUri = cursor.getString(2);
                    String accountName = cursor.getString(3);
                    String accountType = cursor.getString(4);

                    mSimContactGroupCache.addEntry(groupRowId, groupIndex, iccGroupUri,
                            new Account(
                                    accountName, accountType));
                } while (cursor.moveToNext());
            } else {
                mSimContactGroupCache.addNonSimEntry(origGroupRowId);
            }
        } finally {
            if (cursor != null)
                cursor.close();
            Log.e(TAG, "<<<buildSimContactGroupCache");
        }

    }

    public void buildSimContactCache(long origRawContactId) {

        final SQLiteDatabase db = mContactsHelper.getWritableDatabase();
        Cursor c = null;
        if (mContactProxyManager.isProxyImporting()) {
            // when importing, onImport will fill the cache
            return;
        }
        // Log.e (TAG,">>>buildSimContactCache: "+origRawContactId);
        mSimContactCache.remove(origRawContactId);

        String[] projection = new String[] {
                RawContacts._ID, RawContacts.ACCOUNT_NAME, RawContacts.ACCOUNT_TYPE,
                RawContacts.SYNC1, RawContacts.SYNC2
        };

        Cursor cursor = db.query(Views.RAW_CONTACTS, projection,
                RawContacts.ACCOUNT_TYPE + " like 'sprd%sim' and " + RawContacts._ID + " = "
                        + origRawContactId
                        + " and " + RawContacts.DELETED + " = 0", null, null, null,
                null);
        try {

            if (cursor != null && cursor.moveToFirst()) {
                do {
                    long rawContactId = cursor.getLong(0);
                    String accountName = cursor.getString(1);
                    String accountType = cursor.getString(2);
                    String simIndex = cursor.getString(3);
                    String iccUri = cursor.getString(4);
                    /* sprd bug490245 read sne and aas for orange, add Data.DATA3 @{ */
                    String[] projection2 = new String[] {
                            Data.MIMETYPE, Data.DATA1, Data.DATA2, Data.DATA3
                    };
                    /* @} */

                    mSimContactCache.addEntry(rawContactId, simIndex, iccUri, new Account(
                            accountName,
                            accountType));
                    // onDataUpdate

                    c = db.query(true, Views.DATA + " data", projection2,
                            Data.RAW_CONTACT_ID + "=?", new String[] {
                                String.valueOf(rawContactId)
                            }, null, null, null,
                            null);

                    if (c != null && c.moveToFirst()) {
                        do {
                            ContentValues values = new ContentValues();
                            String mimeType = c.getString(0);
                            String data1 = c.getString(1);
                            String data2 = c.getString(2);
                            /* sprd bug490245 read sne and aas for orange @{ */
                            String data3 = c.getString(3);
                            /* @} */
                            values.put(Data.DATA1, data1);
                            values.put(Data.DATA2, data2);
                            /* sprd bug490245 read sne and aas for orange @{ */
                            values.put(Data.DATA3, data3);
                            /* @} */
                            if (mimeType.equals(Phone.CONTENT_ITEM_TYPE)) {
                                data1 =PhoneNumberUtilsEx.pAndwToCommaAndSemicolon(data1);
                                if (null != data1)
                                    values.put(Data.DATA1,
                                            mPhoneNumPattern.matcher(data1).replaceAll(""));
                            }
                            mContactProxyManager.onDataUpdate(rawContactId, values, mimeType);
                        } while (c.moveToNext());
                    }
                    mSimContactCache.setIsDirty(rawContactId, false);
                    if (c != null) {
                        c.close();
                    }
                } while (cursor.moveToNext());
            } else {
                mSimContactCache.addNonSimEntry(origRawContactId);
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
            if (c != null) {
                c.close();
            }
            // Log.e (TAG,"<<<buildSimContactCache");
        }
    }

}
