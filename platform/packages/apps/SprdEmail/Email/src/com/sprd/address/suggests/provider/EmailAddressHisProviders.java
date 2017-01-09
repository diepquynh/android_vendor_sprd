
package com.sprd.address.suggests.provider;

import android.accounts.AccountManager;
import android.content.*;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.net.Uri;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Contacts.Data;
import android.provider.ContactsContract.Directory;
import android.provider.ContactsContract.DisplayNameSources;
import android.provider.ContactsContract.RawContacts;
import android.text.TextUtils;
import android.util.Log;
import com.android.email.activity.setup.AccountSettingsUtils;
import com.android.mail.utils.LogUtils;
import com.sprd.address.suggests.provider.EmailAddressContent.AddressColumns;

import java.util.HashMap;
import java.util.List;

/**
 * SPRD: 523599. Add EmailAddressHisProviders for query and insert
 */
public class EmailAddressHisProviders extends ContentProvider {
    private static final String TAG = "EmailAddressHisProvider";

    protected static final String DATABASE_NAME = "EmailAddressHisProviders.db";
    private static final int BASE_SHIFT = 12;
    private static final String DEFAULT_LOOKUP_LIMIT = "50";
    private static final int EXIST_ADDRESS_LIMIT_IN_DB = 100;

    private static final int ADDRESS_BASE = 0;
    private static final int ADDRESS = ADDRESS_BASE;
    private static final int ADDRESS_FILTER = ADDRESS_BASE + 1;
    private static final int ADDRESS_ID = ADDRESS_BASE + 2;
    private static final int CONTACT_FILTER = ADDRESS_BASE + 3;
    private static final int EMAIL_FILTER = ADDRESS_BASE + 4;
    private static final int DIRECTORIES = ADDRESS_BASE + 5;
    private static final int PHONE_FILTER = ADDRESS_BASE + 6;
    private static final int LOOKUP_KEY = ADDRESS_BASE + 7;
    private static final int LOOKUP_KEY_WITH_ID = ADDRESS_BASE + 8;

    public static final String MIME_ADDRESS = "vnd.android.cursor.dir/eah-address";

    private SQLiteDatabase mDb;
    // Determine whether support contacts and phone search address from Address Table.
    private static final boolean SUPPORT_CONTACTS_PHONE_FILTER = false;

    private static final String[] TABLE_NAMES = {
        EmailAddress.TABLE_NAME
    };

    private static final UriMatcher sURIMatcher = new UriMatcher(UriMatcher.NO_MATCH);

    private static final String MAIL_ADDRESS_SPLIT_PATTERN = ",|;";

    private static final long DEFAULT_CONTACT_ID = 1;

    static {
        UriMatcher matcher = sURIMatcher;
        // all address
        matcher.addURI(EmailAddressContent.AUTHORITY, "address", ADDRESS);
        // filter address
        matcher.addURI(EmailAddressContent.AUTHORITY, "address/filter/*", ADDRESS_FILTER);
        // special address
        matcher.addURI(EmailAddressContent.AUTHORITY, "address/#", ADDRESS_ID);
        matcher.addURI(EmailAddressContent.AUTHORITY, "contacts/filter/*", CONTACT_FILTER);
        matcher.addURI(EmailAddressContent.AUTHORITY, "data/emails/filter/*", EMAIL_FILTER);
        matcher.addURI(EmailAddressContent.AUTHORITY, "directories", DIRECTORIES);
        matcher.addURI(EmailAddressContent.AUTHORITY, "data/phones/filter/*", PHONE_FILTER);

        // add lookup key filter
        matcher.addURI(EmailAddressContent.AUTHORITY, "contacts/lookup/*/entities", LOOKUP_KEY);
        matcher.addURI(EmailAddressContent.AUTHORITY, "contacts/lookup/*/#/entities",
                LOOKUP_KEY_WITH_ID);
    }

    synchronized SQLiteDatabase getDatabase(Context context) {
        if (mDb != null) {
            return mDb;
        }
        DatabaseHelper DbHelper = new DatabaseHelper(getContext(), DATABASE_NAME);
        mDb = DbHelper.getWritableDatabase();
        return mDb;
    }

    @Override
    public String getType(Uri uri) {
        int match = findMatch(uri, "getType");
        switch (match) {
            case ADDRESS:
                return MIME_ADDRESS;
            default:
                return null;
        }
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        int match = findMatch(uri, "insert");
        Context context = getContext();
        SQLiteDatabase db = getDatabase(context);
        int table = match >> BASE_SHIFT;
        Uri resultUri = null;
        String emailAddress = values.getAsString(AddressColumns.EMAIL_ADDRESS);
        String addressType = values.getAsString(AddressColumns.ADDRESS_TYPE);
        if (TextUtils.isEmpty(emailAddress)) {
            return null;
        }
        emailAddress = emailAddress.trim();
        String[] emailPart = emailAddress.split(MAIL_ADDRESS_SPLIT_PATTERN);
        for (String dis_emailAddress : emailPart) {
            Long longId;
            if (EmailAddress.isValidEmailAddress(dis_emailAddress)) {
                if (!findExistingAddress(getContext(), dis_emailAddress, addressType)) {
                    Uri needDeletedUri = needDeletedAddressUri(getContext(), addressType);
                    if (needDeletedUri != null) {
                        int res = delete(needDeletedUri, null, null);
                        LogUtils.d(TAG, "Current address count is over limit, so delete uri: " + needDeletedUri + ", delete result :" + res);
                        if (res < 1) {
                            return null;
                        }
                    }
                    LogUtils.d(TAG, "needDeletedUri: " + needDeletedUri);
                    try {
                        ContentValues cv = new ContentValues();
                        cv.put(AddressColumns.EMAIL_ADDRESS, dis_emailAddress);
                        cv.put(AddressColumns.ADDRESS_TYPE, addressType);
                        switch (match) {
                            case ADDRESS:
                                longId = db.insert(TABLE_NAMES[table],
                                        AddressColumns.EMAIL_ADDRESS, cv);
                                resultUri = ContentUris.withAppendedId(uri, longId);
                                break;
                            default:
                                throw new IllegalArgumentException("Unknown URL ");
                        }

                    } catch (SQLiteException e) {
                        Log.e(TAG, "SQLiteException: " + e.getMessage());
                    }
                } else {
                    resultUri = null;
                }

            } else {
                resultUri = null;
            }
        }

        return resultUri;
    }

    @Override
    public boolean onCreate() {
        return false;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        Cursor c = null;
        int match = findMatch(uri, "query");
        Context context = getContext();
        SQLiteDatabase db = getDatabase(context);
        int table = match >> BASE_SHIFT;
        String id;
        String filter;
        LogUtils.v(TAG, "query: uri=" + uri);
        try {
            switch (match) {
                case ADDRESS_FILTER:
                    filter = uri.getLastPathSegment();
                    if (filter == null || filter.length() < 1) {
                        return null;
                    }
                    selection = AddressColumns.EMAIL_ADDRESS + " like ? escape '\\'";
                    if (filter.contains("_")) {
                        filter = filter.replace("_", "\\_");
                    } else if (filter.contains("%")) {
                        filter = filter.replace("%", "\\%");
                    }
                    String[] selectionArg = {
                        filter + "%"
                    };
                    c = db.query(TABLE_NAMES[table], projection, selection, selectionArg, null,
                            null, null, null);
                    break;
                case ADDRESS_ID:
                    id = uri.getPathSegments().get(1);
                    c = db.query(TABLE_NAMES[table], projection, whereWithId(id, selection),
                            selectionArgs, null, null, sortOrder, null);
                    break;
                case ADDRESS:
                    c = db.query(TABLE_NAMES[table], projection, selection, selectionArgs, null,
                            null, sortOrder, null);
                    break;
                case CONTACT_FILTER:
                case PHONE_FILTER:
                    if (!SUPPORT_CONTACTS_PHONE_FILTER) {
                        return null;
                    }
                case EMAIL_FILTER:
                    filter = uri.getLastPathSegment();
                    if (filter == null || filter.length() < 1) {
                        return null;
                    }
                    // Enforce a limit on the number of lookup responses
                    String limitString = uri.getQueryParameter(ContactsContract.LIMIT_PARAM_KEY);
                    if (limitString == null) {
                        limitString = DEFAULT_LOOKUP_LIMIT;
                    }
                    String dbSelection = AddressColumns.EMAIL_ADDRESS + " like ?";
                    String[] dbSelectionArgs = {
                        filter + "%"
                    };
                    c = db.query(TABLE_NAMES[table], EmailAddress.CONTENT_PROJECTION, dbSelection,
                            dbSelectionArgs, null, null, null, limitString);
                    if (c == null) {
                        Log.i(TAG, "EMAIL_FILTER return null because c is null.");
                        return null;
                    }
                    try {
                        return buildResultCursor(projection, c);
                    } finally {
                        c.close();
                    }
                case DIRECTORIES: {
                    android.accounts.Account[] accounts = AccountManager.get(getContext())
                            .getAccounts();
                    MatrixCursor cursor = new MatrixCursor(projection);
                    if (accounts != null && accounts.length > 0) {
                        android.accounts.Account account = accounts[0];
                        Object[] row = new Object[projection.length];
                        for (int i = 0; i < projection.length; i++) {
                            String column = projection[i];
                            if (column.equals(Directory.ACCOUNT_NAME)) {
                                row[i] = account.name;
                            } else if (column.equals(Directory.ACCOUNT_TYPE)) {
                                row[i] = account.type;
                            } else if (column.equals(Directory.TYPE_RESOURCE_ID)) {
                                row[i] = com.android.email.R.string.app_name;
                            } else if (column.equals(Directory.DISPLAY_NAME)) {
                                row[i] = account.name;
                            } else if (column.equals(Directory.EXPORT_SUPPORT)) {
                                row[i] = Directory.EXPORT_SUPPORT_SAME_ACCOUNT_ONLY;
                            } else if (column.equals(Directory.SHORTCUT_SUPPORT)) {
                                row[i] = Directory.SHORTCUT_SUPPORT_NONE;
                            }
                        }
                        cursor.addRow(row);
                    }
                    return cursor;
                }
                case LOOKUP_KEY:
                case LOOKUP_KEY_WITH_ID:
                    /* SPRD: Modify for bug 556153 {@ */
                    String address = uri.getQueryParameter(RawContacts.ACCOUNT_NAME);
                    /* @} */
                    if (address == null) {
                        return new MatrixCursor(projection);
                    }
                    HistoryProjection hisProjection = new HistoryProjection(projection);
                    MatrixCursor cursor = new MatrixCursor(projection);
                    // Handle the decomposition of the key into rows suitable for CP2
                    List<String> pathSegments = uri.getPathSegments();
                    final long contactId = (match == LOOKUP_KEY_WITH_ID) ? Long
                            .parseLong(pathSegments.get(3)) : DEFAULT_CONTACT_ID;
                    /* SPRD: Modify for bug 556153 {@ */
                    long lookupKey = 0;
                    if (contactId == 0) {
                        lookupKey= Long.parseLong(pathSegments.get(2));
                        selection = AddressColumns.ID + "=" + lookupKey;
                        try {
                            c = db.query(TABLE_NAMES[table], EmailAddress.CONTENT_PROJECTION, selection,
                                    null, null, null, null, null);
                            if (c != null && c.moveToNext()) {
                                String mAddress = c.getString(EmailAddress.CONTENT_ADDRESS_COLUMN);
                                if (!TextUtils.isEmpty(mAddress)) {
                                    address = mAddress;
                                }
                            }
                        } catch (SQLiteException e) {
                            Log.e(TAG, "SQLiteException: " + e.getMessage());
                        }
                    }
                    /* @} */

                    String displayName = address;
                    if (address.contains("@")) {
                        displayName = address.split("@")[0];
                    }
                    HistoryContactRow.addEmailAddress(cursor, hisProjection, contactId,
                            displayName, displayName, address, lookupKey);
                    HistoryContactRow.addNameRow(cursor, hisProjection, contactId, displayName,
                            displayName, displayName, displayName, lookupKey);
                    return cursor;
                default:
                    throw new UnsupportedOperationException("Unknown URI ");
            }
        } catch (SQLiteException e) {
            Log.e(TAG, "SQLiteException: " + e.getMessage());
        } finally {
            if (c == null) {
                LogUtils.d(TAG, "Query returning null for uri: " + uri + ", selection: " + selection);
            }
        }
        return c;
    }

    /* package */Cursor buildResultCursor(String[] projection, Cursor addressCursor) {
        int displayNameIndex = -1;
        int emailIndex = -1;
        int idIndex = -1;
        int lookupIndex = -1;
        int displayNameSource = -1;

        for (int i = 0; i < projection.length; i++) {
            String column = projection[i];
            if (Contacts.DISPLAY_NAME.equals(column)
                    || Contacts.DISPLAY_NAME_PRIMARY.equals(column)) {
                displayNameIndex = i;
            } else if (Email.ADDRESS.equals(column)) {
                emailIndex = i;
            } else if (Contacts._ID.equals(column)) {
                idIndex = i;
            } else if (Contacts.LOOKUP_KEY.equals(column)) {
                lookupIndex = i;
            } else if (Contacts.DISPLAY_NAME_SOURCE.equals(column)) {
                displayNameSource = i;
            }
        }

        Object[] row = new Object[projection.length];
        MatrixCursor cursor = new MatrixCursor(projection);
        while (addressCursor.moveToNext()) {
            long id = addressCursor.getLong(0);
            String address = addressCursor.getString(1);
            if (displayNameIndex != -1) {
                row[displayNameIndex] = address.split("@")[0];
            }
            if (emailIndex != -1) {
                row[emailIndex] = address;
            }
            if (idIndex != -1) {
                row[idIndex] = id;
            }
            if (lookupIndex != -1) {
                row[lookupIndex] = id;
            }
            if (displayNameSource != -1) {
                row[displayNameSource] = DisplayNameSources.STRUCTURED_NAME;
            }
            cursor.addRow(row);
        }
        return cursor;
    }

    private String whereWithId(String id, String selection) {
        StringBuilder sb = new StringBuilder(256);
        sb.append("_id=");
        sb.append(id);
        if (selection != null) {
            sb.append(" AND (");
            sb.append(selection);
            sb.append(')');
        }
        return sb.toString();
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        final int match = findMatch(uri, "delete");
        Context context = getContext();
        SQLiteDatabase db = getDatabase(context);
        int table = match >> BASE_SHIFT;
        int result = -1;
        String id = "0";
        switch (match) {
            case ADDRESS_ID:
                id = uri.getPathSegments().get(1);
                result = db.delete(TABLE_NAMES[table], whereWithId(id, selection), selectionArgs);
                break;
            default:
                throw new IllegalArgumentException("Unknown URI " + uri);
        }
        return result;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException();
    }

    private static int findMatch(Uri uri, String methodName) {
        int match = sURIMatcher.match(uri);
        /* SPRD:bug547595 Coverity defect begin. @{ */
        if (match < 0) {
            LogUtils.v(TAG, "uri cannot match. Unknown uri:" + uri);
        } else {
            LogUtils.v(TAG, methodName + ": filterStr=" + uri.getLastPathSegment() + ", match is " + match);
        }
        /* @} */
        return match;
    }

    public static boolean findExistingAddress(Context context, String allowAddress,
            String addressType) {
        boolean isExsit = false;
        ContentResolver resolver = context.getContentResolver();
        Cursor c = resolver.query(EmailAddress.CONTENT_URI, EmailAddress.CONTENT_PROJECTION, null,
                null, null);
        if (c == null) {
            return false;
        }
        try {
            while (c.moveToNext()) {
                if (allowAddress.equals(c.getString(1))) {
                    String addType = c.getString(2);
                    if (addressType.equals(addType)
                            || AccountSettingsUtils.FROM_ACCOUNT_ADD.equals(addType)) {
                        isExsit = true;
                    }
                    break;
                }
            }
        } catch (SQLiteException e) {
            Log.e(TAG, "SQLiteException: " + e.getMessage());
        } finally {
            c.close();
        }
        return isExsit;
    }

    public static Uri needDeletedAddressUri(Context context, String type) {
        Uri needDeletedUri = null;
        ContentResolver resolver = context.getContentResolver();
        Cursor c = resolver.query(EmailAddress.CONTENT_URI, new String[]{"_id"}, EmailAddress.ADDRESS_TYPE + "=?",
                new String[]{type}, null);
        if (c == null) {
            return null;
        }
        try {
            int count = c.getCount();
            if (count < EXIST_ADDRESS_LIMIT_IN_DB) {
                needDeletedUri = null;
            } else {
                if (c.moveToFirst()) {
                    String id = c.getInt(0) + "";
                    needDeletedUri = Uri.withAppendedPath(EmailAddress.CONTENT_URI, id);
                }
            }
        } catch (SQLiteException e) {
            Log.e(TAG, "SQLiteException: " + e.getMessage());
        } finally {
            c.close();
        }
        return needDeletedUri;
    }

    /**
     * History contact Projection for Lookup cursor.
     */
    static class HistoryProjection {
        final int mSize;

        final HashMap<String, Integer> mColumnMap = new HashMap<String, Integer>();

        HistoryProjection(String[] projection) {
            mSize = projection.length;
            for (int i = 0; i < projection.length; i++) {
                mColumnMap.put(projection[i], i);
            }
        }
    }

    /**
     * History contact row for Lookup uri cursor.
     */
    static class HistoryContactRow {
        private final HistoryProjection mProjection;

        private Object[] mRow;

        static long sDataID = 1;
        /* SPRD: Modify for bug 556153 {@ */
        HistoryContactRow(HistoryProjection projection, long contactId, String accountName,
                String displayName, long lookupKey) {
        /* @} */
            this.mProjection = projection;
            mRow = new Object[projection.mSize];

            put(Contacts.Entity.CONTACT_ID, contactId);

            // We only have one raw contact per aggregate, so they can have the same ID
            put(Contacts.Entity.RAW_CONTACT_ID, contactId);
            put(Contacts.Entity.DATA_ID, sDataID++);

            put(Contacts.DISPLAY_NAME, displayName);

            // TODO alternative display name
            put(Contacts.DISPLAY_NAME_ALTERNATIVE, displayName);
            /* SPRD: Modify for bug 556153 {@ */
            if (!TextUtils.isEmpty(displayName)) {
                put(Contacts.DISPLAY_NAME_SOURCE, DisplayNameSources.STRUCTURED_NAME);
            }
            put(Email._ID, lookupKey);
            /* @} */

            put(RawContacts.ACCOUNT_TYPE, "com.android.email");
            put(RawContacts.ACCOUNT_NAME, accountName);
            put(RawContacts.RAW_CONTACT_IS_READ_ONLY, 1);
            put(Data.IS_READ_ONLY, 1);
        }

        Object[] getRow() {
            return mRow;
        }

        void put(String columnName, Object value) {
            final Integer integer = mProjection.mColumnMap.get(columnName);
            if (integer != null) {
                mRow[integer] = value;
            } else {
                LogUtils.e(TAG, "Unsupported column: " + columnName);
            }
        }

        /* SPRD: Modify for bug 556153 {@ */
        static void addEmailAddress(MatrixCursor cursor, HistoryProjection galProjection,
                long contactId, String accountName, String displayName, String address,
                long lookupKey) {
            if (!TextUtils.isEmpty(address)) {
                final HistoryContactRow r = new HistoryContactRow(galProjection, contactId,
                        accountName, displayName, lookupKey);
                r.put(Data.MIMETYPE, Email.CONTENT_ITEM_TYPE);
                r.put(Email.TYPE, Email.TYPE_WORK);
                r.put(Email.ADDRESS, address);
                cursor.addRow(r.getRow());
            }
        }

        public static void addNameRow(MatrixCursor cursor, HistoryProjection galProjection,
                long contactId, String accountName, String displayName, String firstName,
                String lastName, long lookupKey) {
            final HistoryContactRow r = new HistoryContactRow(galProjection, contactId,
                    accountName, displayName, lookupKey);
            r.put(Data.MIMETYPE, StructuredName.CONTENT_ITEM_TYPE);
            r.put(StructuredName.GIVEN_NAME, firstName);
            r.put(StructuredName.FAMILY_NAME, lastName);
            r.put(StructuredName.DISPLAY_NAME, displayName);
            cursor.addRow(r.getRow());
        }
        /* @} */
    }
}
