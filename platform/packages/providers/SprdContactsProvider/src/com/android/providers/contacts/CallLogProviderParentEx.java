package com.android.providers.contacts;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.provider.CallLog;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.DataUsageFeedback;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.StatusUpdates;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import android.util.Log;

import com.android.common.content.ProjectionMap;
import com.android.providers.contacts.ContactsDatabaseHelper;
import com.android.providers.contacts.ContactsDatabaseHelper.AggregatedPresenceColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.ContactsColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.ContactsStatusUpdatesColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.DataColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.DataUsageStatColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.PhoneLookupColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.RawContactsColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.SearchIndexColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.StatusUpdatesColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.Tables;
import com.android.providers.contacts.ContactsDatabaseHelper.Views;
import com.android.providers.contacts.ContactsProvider2;
import com.android.providers.contacts.SearchIndexManager;
import com.android.providers.contacts.SearchIndexManager.FtsQueryBuilder;

public class CallLogProviderParentEx extends ContentProvider {
    private static final String TAG = "CallLogProviderParent";
    private static final boolean DBG = false;

    protected static final String TIME_SINCE_LAST_USED = "(strftime('%s', 'now') - "
            + DataUsageStatColumns.LAST_TIME_USED + "/1000)";

    // Recent contacts - those contacted within the last 30 days (in seconds)
    protected static final long EMAIL_FILTER_RECENT = 30 * 24 * 60 * 60;

    // Current contacts - those contacted within the last 3 days (in seconds)
    protected static final long EMAIL_FILTER_CURRENT = 3 * 24 * 60 * 60;

    protected static final String EMAIL_FILTER_SORT_ORDER = Contacts.STARRED
            + " DESC, " + Contacts.IN_VISIBLE_GROUP + " DESC, " + "(CASE WHEN "
            + TIME_SINCE_LAST_USED + " < " + EMAIL_FILTER_CURRENT + " THEN 0 "
            + " WHEN " + TIME_SINCE_LAST_USED + " < " + EMAIL_FILTER_RECENT
            + " THEN 1 " + " ELSE 2 END), " + DataUsageStatColumns.TIMES_USED
            + " DESC, " + Contacts.DISPLAY_NAME + ", " + Data.CONTACT_ID + ", "
            + Data.IS_SUPER_PRIMARY + " DESC, " + Data.IS_PRIMARY + " DESC";

    /** Currently same as {@link #EMAIL_FILTER_SORT_ORDER} */
    protected static final String PHONE_FILTER_SORT_ORDER = EMAIL_FILTER_SORT_ORDER;

    private static final ProjectionMap sDataColumns = ProjectionMap.builder()
            .add(Data.DATA1)
            .add(Data.DATA2)
            .add(Data.DATA3)
            .add(Data.DATA4)
            .add(Data.DATA5)
            .add(Data.DATA6)
            .add(Data.DATA7)
            .add(Data.DATA8)
            .add(Data.DATA9)
            .add(Data.DATA10)
            .add(Data.DATA11)
            .add(Data.DATA12)
            .add(Data.DATA13)
            .add(Data.DATA14)
            .add(Data.DATA15)
            .add(Data.DATA_VERSION)
            .add(Data.IS_PRIMARY)
            .add(Data.IS_SUPER_PRIMARY)
            .add(Data.MIMETYPE)
            .add(Data.RES_PACKAGE)
            .add(Data.SYNC1)
            .add(Data.SYNC2)
            .add(Data.SYNC3)
            .add(Data.SYNC4)
            .add(GroupMembership.GROUP_SOURCE_ID)
            .build();

    private static final ProjectionMap sDataPresenceColumns = ProjectionMap.builder()
            .add(Data.PRESENCE, Tables.PRESENCE + "." + StatusUpdates.PRESENCE)
            .add(Data.CHAT_CAPABILITY, Tables.PRESENCE + "." + StatusUpdates.CHAT_CAPABILITY)
            .add(Data.STATUS, StatusUpdatesColumns.CONCRETE_STATUS)
            .add(Data.STATUS_TIMESTAMP, StatusUpdatesColumns.CONCRETE_STATUS_TIMESTAMP)
            .add(Data.STATUS_RES_PACKAGE, StatusUpdatesColumns.CONCRETE_STATUS_RES_PACKAGE)
            .add(Data.STATUS_LABEL, StatusUpdatesColumns.CONCRETE_STATUS_LABEL)
            .add(Data.STATUS_ICON, StatusUpdatesColumns.CONCRETE_STATUS_ICON)
            .build();

    private static final ProjectionMap sContactsColumns = ProjectionMap.builder()
            .add(Contacts.CUSTOM_RINGTONE)
            .add(Contacts.DISPLAY_NAME)
            .add(RawContacts.ACCOUNT_TYPE)
            .add(RawContacts.ACCOUNT_NAME)
            .add(Contacts.DISPLAY_NAME_ALTERNATIVE)
            .add(Contacts.DISPLAY_NAME_SOURCE)
            .add(Contacts.IN_VISIBLE_GROUP)
            .add(Contacts.LAST_TIME_CONTACTED)
            .add(Contacts.LOOKUP_KEY)
            .add(Contacts.PHONETIC_NAME)
            .add(Contacts.PHONETIC_NAME_STYLE)
            .add(Contacts.PHOTO_ID)
            .add(Contacts.PHOTO_FILE_ID)
            .add(Contacts.PHOTO_URI)
            .add(Contacts.PHOTO_THUMBNAIL_URI)
            .add(Contacts.SEND_TO_VOICEMAIL)
            .add(Contacts.SORT_KEY_ALTERNATIVE)
            .add(Contacts.SORT_KEY_PRIMARY)
            .add(Contacts.STARRED)
            .add(Contacts.TIMES_CONTACTED)
            .add(Contacts.HAS_PHONE_NUMBER)
            .build();

    private static final ProjectionMap sContactPresenceColumns = ProjectionMap.builder()
            .add(Contacts.CONTACT_PRESENCE,
                    Tables.AGGREGATED_PRESENCE + '.' + StatusUpdates.PRESENCE)
            .add(Contacts.CONTACT_CHAT_CAPABILITY,
                    Tables.AGGREGATED_PRESENCE + '.' + StatusUpdates.CHAT_CAPABILITY)
            .add(Contacts.CONTACT_STATUS,
                    ContactsStatusUpdatesColumns.CONCRETE_STATUS)
            .add(Contacts.CONTACT_STATUS_TIMESTAMP,
                    ContactsStatusUpdatesColumns.CONCRETE_STATUS_TIMESTAMP)
            .add(Contacts.CONTACT_STATUS_RES_PACKAGE,
                    ContactsStatusUpdatesColumns.CONCRETE_STATUS_RES_PACKAGE)
            .add(Contacts.CONTACT_STATUS_LABEL,
                    ContactsStatusUpdatesColumns.CONCRETE_STATUS_LABEL)
            .add(Contacts.CONTACT_STATUS_ICON,
                    ContactsStatusUpdatesColumns.CONCRETE_STATUS_ICON)
            .build();

    /** Contains columns from the data view */
    private static final ProjectionMap sDistinctDataProjectionMap = ProjectionMap.builder()
            .add(Data._ID, "MIN(" + Data._ID + ")")
            .add(RawContacts.CONTACT_ID)
            .add(RawContacts.RAW_CONTACT_IS_USER_PROFILE)
            .addAll(sDataColumns)
            .addAll(sDataPresenceColumns)
            .addAll(sContactsColumns)
            .addAll(sContactPresenceColumns)
            .build();

    private static final int CONTACTS_AND_CALLLOG = 100;

    /**
     * Stores mapping from type Strings exposed via {@link DataUsageFeedback} to
     * type integers in {@link DataUsageStatColumns}.
     */
    protected static final Map<String, Integer> sDataUsageTypeMap;

    static {
        // map from contacts
        HashMap<String, Integer> tmpTypeMap = new HashMap<String, Integer>();
        tmpTypeMap.put(DataUsageFeedback.USAGE_TYPE_CALL,
                DataUsageStatColumns.USAGE_TYPE_INT_CALL);
        tmpTypeMap.put(DataUsageFeedback.USAGE_TYPE_LONG_TEXT,
                DataUsageStatColumns.USAGE_TYPE_INT_LONG_TEXT);
        tmpTypeMap.put(DataUsageFeedback.USAGE_TYPE_SHORT_TEXT,
                DataUsageStatColumns.USAGE_TYPE_INT_SHORT_TEXT);
        sDataUsageTypeMap = Collections.unmodifiableMap(tmpTypeMap);
    }

    private static final UriMatcher mMatcher = new UriMatcher(UriMatcher.NO_MATCH);
    static {
        mMatcher.addURI(CallLog.AUTHORITY, "calls/contacts/*", CONTACTS_AND_CALLLOG);
    }

    protected  ContactsDatabaseHelper mContactsDbHelper;

    @Override
    public boolean onCreate() {
        final Context context = getContext();
        mContactsDbHelper = ContactsDatabaseHelper.getInstance(context);
        return false;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
                        String[] selectionArgs, String sortOrder) {
        SQLiteQueryBuilder qb = new SQLiteQueryBuilder();
        String groupBy = null;
        String limit = getLimit(uri);

        final int match = mMatcher.match(uri);

        switch (match) {
            case CONTACTS_AND_CALLLOG: {

                mContactsDbHelper = ContactsDatabaseHelper.getInstance(getContext());
                String typeParam = uri.getQueryParameter(DataUsageFeedback.USAGE_TYPE);
                Integer typeInt = sDataUsageTypeMap.get(typeParam);
                if (typeInt == null) {
                    typeInt = DataUsageStatColumns.USAGE_TYPE_INT_CALL;
                }
                setTablesAndProjectionMapForData(qb, uri, projection, typeInt);
                qb.appendWhere(" AND " + DataColumns.MIMETYPE_ID + " = " + mContactsDbHelper.getMimeTypeIdForPhone());
                if (uri.getPathSegments().size() > 2) {
                    String filterParam = uri.getLastPathSegment();
                    StringBuilder sb = new StringBuilder();
                    sb.append(" AND (");

                    boolean hasCondition = false;
                    boolean orNeeded = false;
                    final String ftsMatchQuery = SearchIndexManager.getFtsMatchQuery(filterParam,
                            FtsQueryBuilder.UNSCOPED_NORMALIZING);
                    if (ftsMatchQuery.length() > 0) {
                        sb.append(Data.RAW_CONTACT_ID + " IN " + "(SELECT "
                                + RawContactsColumns.CONCRETE_ID + " FROM "
                                + Tables.SEARCH_INDEX + " JOIN "
                                + Tables.RAW_CONTACTS + " ON ("
                                + Tables.SEARCH_INDEX + "."
                                + SearchIndexColumns.CONTACT_ID + "="
                                + RawContactsColumns.CONCRETE_CONTACT_ID + ")"
                                + " WHERE " + SearchIndexColumns.NAME + " MATCH '");
                        sb.append(ftsMatchQuery);
                        sb.append("' AND "+RawContactsColumns.CONCRETE_DELETED+"=0 ");
                        sb.append("limit 20)");
                        orNeeded = true;
                        hasCondition = true;
                    }

                    String number = PhoneNumberUtils.normalizeNumber(filterParam);
                    if (!TextUtils.isEmpty(number)) {
                        if (orNeeded) {
                            sb.append(" OR ");
                        }
                        sb.append(Data._ID + " IN (SELECT DISTINCT "
                                + PhoneLookupColumns.DATA_ID + " FROM "
                                + Tables.PHONE_LOOKUP  + " JOIN "
                                + Tables.RAW_CONTACTS + " ON ("
                                + Tables.PHONE_LOOKUP + "."
                                + Data.RAW_CONTACT_ID  + "="
                                + Tables.RAW_CONTACTS + "." + RawContacts._ID + ")"
                                + " WHERE " + PhoneLookupColumns.NORMALIZED_NUMBER + " LIKE '");
                        sb.append("%");
                        sb.append(number);
                        sb.append("%'");
                        sb.append(" AND "+RawContactsColumns.CONCRETE_DELETED+"=0 ");
                        sb.append("limit 20)");
                        hasCondition = true;
                    }


                    if (!hasCondition) {
                        // If it is neither a phone number nor a name, the query
                        // should return
                        // an empty cursor. Let's ensure that.
                        sb.append("0");
                    }
                    sb.append(")");
                    qb.appendWhere(sb);
                }
                groupBy = "(CASE WHEN " + Phone.NORMALIZED_NUMBER
                        + " IS NOT NULL THEN " + Phone.NORMALIZED_NUMBER
                        + " ELSE " + Phone.NUMBER + " END), "
                        + RawContacts.CONTACT_ID;
                if (sortOrder == null) {
                    final String accountPromotionSortOrder = getAccountPromotionSortOrder(uri);
                    if (!TextUtils.isEmpty(accountPromotionSortOrder)) {
                        sortOrder = accountPromotionSortOrder + ", "
                                + PHONE_FILTER_SORT_ORDER;
                    } else {
                        sortOrder = PHONE_FILTER_SORT_ORDER;
                    }
                }

                // start query
                qb.setStrict(true);
                SQLiteDatabase db = mContactsDbHelper.getReadableDatabase();
                String sql = qb.buildQuery(projection, selection, groupBy, null,sortOrder, limit);
                if(DBG) Log.d(TAG, "filter : " + sql);
                Cursor cursor = query(db, qb, projection, selection, selectionArgs,
                        sortOrder, groupBy, limit);
                return cursor;
            }
            default:
                throw new IllegalArgumentException("Unknown URL " + uri);
        }
    }

    private Cursor query(final SQLiteDatabase db, SQLiteQueryBuilder qb,
                         String[] projection, String selection, String[] selectionArgs,
                         String sortOrder, String groupBy, String limit) {
        final Cursor c = qb.query(db, projection, selection, selectionArgs,
                groupBy, null, sortOrder, limit);
        if (c != null) {
            c.setNotificationUri(getContext().getContentResolver(),CallLog.CONTENT_URI);
        }
        return c;
    }

    @Override
    public String getType(Uri uri) {
        return null;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        return null;
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        return 0;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection,
                      String[] selectionArgs) {
        return 0;
    }

    private void setTablesAndProjectionMapForData(SQLiteQueryBuilder qb,
                                                  Uri uri, String[] projection, Integer usageType) {

        StringBuilder sb = new StringBuilder();
        /**
        * SPRD:Bug474803 Sync Facebook contacts.
        *
        * Original Android code:
        sb.append(Views.DATA);
        * @{
        */
        String dataView = mContactsDbHelper.getDataView(ContactsDatabaseHelper.canAccessRestrictedData(getContext()));
        sb.append(dataView);
        /**
        *
        * @}
        */
        sb.append(" data");

        appendContactPresenceJoin(sb, projection, RawContacts.CONTACT_ID);
        appendContactStatusUpdateJoin(sb, projection,
                ContactsColumns.LAST_STATUS_UPDATE_ID);
        appendDataPresenceJoin(sb, projection, DataColumns.CONCRETE_ID);
        appendDataStatusUpdateJoin(sb, projection, DataColumns.CONCRETE_ID);

        if (usageType != null) {
            appendDataUsageStatJoin(sb, usageType, DataColumns.CONCRETE_ID);
        }

        qb.setTables(sb.toString());
        qb.setDistinct(true);
        qb.setProjectionMap(sDistinctDataProjectionMap);
        appendAccountFromParameter(qb, uri);
    }

    private void appendContactPresenceJoin(StringBuilder sb,
                                           String[] projection, String contactIdColumn) {
        if (mContactsDbHelper.isInProjection(projection, Contacts.CONTACT_PRESENCE,
                Contacts.CONTACT_CHAT_CAPABILITY)) {
            sb.append(" LEFT OUTER JOIN " + Tables.AGGREGATED_PRESENCE
                    + " ON (" + contactIdColumn + " = "
                    + AggregatedPresenceColumns.CONCRETE_CONTACT_ID + ")");
        }
    }

    private void appendContactStatusUpdateJoin(StringBuilder sb,
                                               String[] projection, String lastStatusUpdateIdColumn) {
        if (mContactsDbHelper.isInProjection(projection, Contacts.CONTACT_STATUS,
                Contacts.CONTACT_STATUS_RES_PACKAGE,
                Contacts.CONTACT_STATUS_ICON, Contacts.CONTACT_STATUS_LABEL,
                Contacts.CONTACT_STATUS_TIMESTAMP)) {
            sb.append(" LEFT OUTER JOIN " + Tables.STATUS_UPDATES + " "
                    + ContactsStatusUpdatesColumns.ALIAS + " ON ("
                    + lastStatusUpdateIdColumn + "="
                    + ContactsStatusUpdatesColumns.CONCRETE_DATA_ID + ")");
        }
    }

    private void appendDataPresenceJoin(StringBuilder sb, String[] projection,
                                        String dataIdColumn) {
        if (mContactsDbHelper.isInProjection(projection, Data.PRESENCE,
                Data.CHAT_CAPABILITY)) {
            sb.append(" LEFT OUTER JOIN " + Tables.PRESENCE + " ON ("
                    + StatusUpdates.DATA_ID + "=" + dataIdColumn + ")");
        }
    }

    private void appendDataStatusUpdateJoin(StringBuilder sb,
                                            String[] projection, String dataIdColumn) {
        if (mContactsDbHelper.isInProjection(projection, StatusUpdates.STATUS,
                StatusUpdates.STATUS_RES_PACKAGE, StatusUpdates.STATUS_ICON,
                StatusUpdates.STATUS_LABEL, StatusUpdates.STATUS_TIMESTAMP)) {
            sb.append(" LEFT OUTER JOIN " + Tables.STATUS_UPDATES + " ON ("
                    + StatusUpdatesColumns.CONCRETE_DATA_ID + "="
                    + dataIdColumn + ")");
        }
    }

    private void appendDataUsageStatJoin(StringBuilder sb, int usageType, String dataIdColumn) {
        sb.append(" LEFT OUTER JOIN " + Tables.DATA_USAGE_STAT +
                " ON (" + DataUsageStatColumns.CONCRETE_DATA_ID + "=" + dataIdColumn +
                " AND " + DataUsageStatColumns.CONCRETE_USAGE_TYPE + "=" + usageType + ")");
    }

    private void appendAccountFromParameter(SQLiteQueryBuilder qb, Uri uri) {
        final String accountName = getQueryParameter(uri, RawContacts.ACCOUNT_NAME);
        final String accountType = getQueryParameter(uri, RawContacts.ACCOUNT_TYPE);
        final String dataSet = getQueryParameter(uri, RawContacts.DATA_SET);

        final boolean partialUri = TextUtils.isEmpty(accountName) ^ TextUtils.isEmpty(accountType);
        if (partialUri) {
            // Throw when either account is incomplete
            throw new IllegalArgumentException(mContactsDbHelper.exceptionMessage(
                    "Must specify both or neither of ACCOUNT_NAME and ACCOUNT_TYPE", uri));
        }

        // Accounts are valid by only checking one parameter, since we've
        // already ruled out partial accounts.
        final boolean validAccount = !TextUtils.isEmpty(accountName);
        if (validAccount) {
            String toAppend = RawContacts.ACCOUNT_NAME + "="
                    + DatabaseUtils.sqlEscapeString(accountName) + " AND "
                    + RawContacts.ACCOUNT_TYPE + "="
                    + DatabaseUtils.sqlEscapeString(accountType);
            if (dataSet == null) {
                toAppend += " AND " + RawContacts.DATA_SET + " IS NULL";
            } else {
                toAppend += " AND " + RawContacts.DATA_SET + "=" +
                        DatabaseUtils.sqlEscapeString(dataSet);
            }
            qb.appendWhere(toAppend);
        } else {
            qb.appendWhere("1");
        }
    }

    protected String getQueryParameter(Uri uri, String parameter) {
        return ContactsProvider2.getQueryParameter(uri, parameter);
    }

    /**
     * Returns a sort order String for promoting data rows (email addresses,
     * phone numbers, etc.) associated with a primary account. The primary
     * account should be supplied from applications with
     * {@link ContactsContract#PRIMARY_ACCOUNT_NAME} and
     * {@link ContactsContract#PRIMARY_ACCOUNT_TYPE}. Null will be returned when
     * the primary account isn't available.
     */
    private String getAccountPromotionSortOrder(Uri uri) {
        final String primaryAccountName = uri
                .getQueryParameter(ContactsContract.PRIMARY_ACCOUNT_NAME);
        final String primaryAccountType = uri
                .getQueryParameter(ContactsContract.PRIMARY_ACCOUNT_TYPE);

        // Data rows associated with primary account should be promoted.
        if (!TextUtils.isEmpty(primaryAccountName)) {
            StringBuilder sb = new StringBuilder();
            sb.append("(CASE WHEN " + RawContacts.ACCOUNT_NAME + "=");
            DatabaseUtils.appendEscapedSQLString(sb, primaryAccountName);
            if (!TextUtils.isEmpty(primaryAccountType)) {
                sb.append(" AND " + RawContacts.ACCOUNT_TYPE + "=");
                DatabaseUtils.appendEscapedSQLString(sb, primaryAccountType);
            }
            sb.append(" THEN 0 ELSE 1 END)");
            return sb.toString();
        } else {
            return null;
        }
    }

    /**
     * Gets the value of the "limit" URI query parameter.
     *
     * @return A string containing a non-negative integer, or <code>null</code>
     *         if the parameter is not set, or is set to an invalid value.
     */
    private String getLimit(Uri uri) {
        String limitParam = getQueryParameter(uri, ContactsContract.LIMIT_PARAM_KEY);
        if (limitParam == null) {
            return null;
        }

        // make sure that the limit is a non-negative integer
        try {
            int l = Integer.parseInt(limitParam);
            if (l < 0) {
                Log.w(TAG, "Invalid limit parameter: " + limitParam);
                return null;
            }
            return String.valueOf(l);
        } catch (NumberFormatException ex) {
            Log.w(TAG, "Invalid limit parameter: " + limitParam);
            return null;
        }
    }

}
