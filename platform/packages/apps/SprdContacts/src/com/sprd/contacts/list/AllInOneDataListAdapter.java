
package com.sprd.contacts.list;

import android.content.ContentUris;
import android.content.Context;
import android.content.CursorLoader;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.preference.PreferenceManager;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.Directory;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;

import com.android.contacts.common.list.ContactListFilter;
import com.android.contacts.common.list.ContactListItemView;
import com.android.contacts.common.list.ContactEntryListAdapter;
import com.sprd.contacts.list.AllInOneDataListAdapter.DataQuery;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.preference.ContactsPreferences;
import com.sprd.contacts.util.AccountRestrictionUtils;
import com.android.contacts.common.util.Constants;
import com.android.contacts.common.list.IndexerListAdapter.Placement;
import com.android.contacts.common.ContactPhotoManager.DefaultImageRequest;
import com.android.contacts.common.model.AccountTypeManager;
import java.util.ArrayList;
import java.util.List;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

public class AllInOneDataListAdapter extends ContactEntryListAdapter {
    private static final String TAG = AllInOneDataListAdapter.class.getSimpleName();

    protected static class DataQuery {
        private static final String[] PROJECTION_PRIMARY = new String[] {
                Data._ID,
                Data.DATA1,
                Data.CONTACT_ID,
                Data.LOOKUP_KEY,
                Data.PHOTO_ID,
                Data.PHOTO_URI,
                Data.DISPLAY_NAME_PRIMARY,
                Contacts.DISPLAY_ACCOUNT_TYPE,
                Contacts.DISPLAY_ACCOUNT_NAME,
                Data.MIMETYPE,
                Contacts.DISPLAY_NAME_SOURCE,
                RawContacts.SYNC1,
        };

        public static final int DATA_ID = 0;
        public static final int DATA_DATA1 = 1;
        public static final int DATA_CONTACT_ID = 2;
        public static final int DATA_LOOKUP_KEY = 3;
        public static final int DATA_PHOTO_ID = 4;
        public static final int DATA_PHOTO_URI = 5;
        public static final int DATA_DISPLAY_NAME = 6;
        public static final int CONTACT_DISPLAY_ACCOUNT_TYPE = 7;
        public static final int CONTACT_DISPLAY_ACCOUNT_NAME = 8;
        public static final int DATA_MIMETYPE = 9;
        public static final int DISPLAY_NAME_SOURCE = 10;
        public static final int RAWCONTACTS_SYNC1 = 11;
    }

    private final CharSequence mUnknownNameText;

    private List<String> mCascadingData;
    private String mDataSelection;
    private boolean filterable = true;

    private ContactListItemView.PhotoPosition mPhotoPosition;

    public AllInOneDataListAdapter(Context context) {
        super(context);
        mUnknownNameText = context.getText(android.R.string.unknownName);
    }

    protected CharSequence getUnknownNameText() {
        return mUnknownNameText;
    }

    @Override
    public void configureLoader(CursorLoader loader, long directoryId) {
        Uri uri;

        if (directoryId != Directory.DEFAULT) {
            Log.w(TAG, "AllInOneDataListAdapter is not ready for non-default directory ID ("
                    + "directoryId: " + directoryId + ")");
        }
        if (isSearchMode()) {
            String query = getQueryString();
            if (!TextUtils.isEmpty(query)) {
                uri = Data.CONTENT_URI.buildUpon()
                        .appendPath("filter")
                        .appendPath(query)
                        .build();
            } else {
                uri = Data.CONTENT_URI.buildUpon()
                        .appendPath("filter")
                        .build();
            }

        } else {
            uri = Data.CONTENT_URI.buildUpon().appendQueryParameter(
                    ContactsContract.DIRECTORY_PARAM_KEY, String.valueOf(Directory.DEFAULT))
                    .build();
        }
        if (isSectionHeaderDisplayEnabled()) {
            uri = buildSectionIndexerUri(uri);
        }
        configureSelection(loader, directoryId, getFilter());

        // Remove duplicates when it is possible.
        uri = uri.buildUpon()
                .appendQueryParameter(ContactsContract.REMOVE_DUPLICATE_ENTRIES, "true")
                .build();
        loader.setUri(uri);

        // TODO a projection that includes the search snippet
        loader.setProjection(DataQuery.PROJECTION_PRIMARY);

        /**
         * Bug449857,Bug509397 Set CursorLoader's sort order.
         * @{
         */
        if (getSortOrder() == ContactsPreferences.SORT_ORDER_PRIMARY) {
            loader.setSortOrder(Phone.SORT_KEY_PRIMARY + " , " + Phone.RAW_CONTACT_ID);
        } else {
            loader.setSortOrder(Phone.SORT_KEY_ALTERNATIVE + " , " + Phone.RAW_CONTACT_ID);
        }
        /**
         * @}
         */
    }

    private void configureSelection(
            CursorLoader loader, long directoryId, ContactListFilter filter) {
        if (filter == null || directoryId != Directory.DEFAULT) {
            return;
        }

        final StringBuilder selection = new StringBuilder();
        final List<String> selectionArgs = new ArrayList<String>();

        selection.append(Data.MIMETYPE + " in (");
        boolean isInit = true;
        for (String mimeType : mCascadingData) {
            if (!isInit) {
                selection.append(",");
            }
            isInit = false;
            selection.append(" \"" + mimeType + "\"");
        }
        selection.append(")");
        if ("star".equals(mDataSelection)) {
            selection.append(" AND ");
            selection.append(Contacts.STARRED + "!=0");
            filterable = false;
        }
        if (mCascadingData != null && mCascadingData.size() == 1
                && mCascadingData.contains(Email.CONTENT_ITEM_TYPE)) {
            filterable = false;
        }
        switch (filter.filterType) {
            case ContactListFilter.FILTER_TYPE_CUSTOM: {
                selection.append(" AND ");
                selection.append(Contacts.IN_VISIBLE_GROUP + "=1");
                break;
            }
            case ContactListFilter.FILTER_TYPE_ACCOUNT: {
                selection.append(" AND ");
                selection.append("(");

                selection.append(RawContacts.ACCOUNT_TYPE + "=?"
                        + " AND " + RawContacts.ACCOUNT_NAME + "=?");
                selectionArgs.add(filter.accountType);
                selectionArgs.add(filter.accountName);
                if (filter.dataSet != null) {
                    selection.append(" AND " + RawContacts.DATA_SET + "=?");
                    selectionArgs.add(filter.dataSet);
                } else {
                    selection.append(" AND " + RawContacts.DATA_SET + " IS NULL");
                }
                selection.append(")");
                break;
            }
            case ContactListFilter.FILTER_TYPE_ALL_ACCOUNTS:
            case ContactListFilter.FILTER_TYPE_DEFAULT:
                break; // No selection needed.
            case ContactListFilter.FILTER_TYPE_WITH_PHONE_NUMBERS_ONLY:
                /**
                 * SPRD bug498383 import simcard only has name and type of other_type
                 * to local by *#*#4636#*#*, add this phone into blacklist, phone crash
                 *
                 * @{
                 */
                selection.append(" AND ");
                selection.append(Contacts.HAS_PHONE_NUMBER + "=1");
                selection.append(" AND ");
                selection.append(Data.DATA1 + " NOT LIKE '%,%'");
                /**
                 * @}
                 */
                break; // This adapter is always "phone only", so no selection
                       // needed either.
            case ContactListFilter.FILTER_TYPE_GROUP_MEMBER_ONLY_PHONE_NUMBER:
                filterable = false;
                selection.append(" AND " +
                        Data.RAW_CONTACT_ID + " IN ("
                        + "SELECT " + "data." + Data.RAW_CONTACT_ID
                        + " FROM " + "data "
                        + "JOIN mimetypes ON (data.mimetype_id = mimetypes._id)"
                        + " WHERE " + Data.MIMETYPE + "='"
                        + GroupMembership.CONTENT_ITEM_TYPE
                        + "' AND " + Contacts.HAS_PHONE_NUMBER + "=1"
                        + " AND " + GroupMembership.GROUP_ROW_ID +
                        "=?)");
                // selection.append(" AND " + RawContacts.DELETED + "=0 )");
                selectionArgs.add(String.valueOf(filter.groupId));
                break;
            case ContactListFilter.FILTER_TYPE_GROUP_MEMBER:
                filterable = false;
                selection.append(" AND " +
                        Data.RAW_CONTACT_ID + " IN ("
                        + "SELECT " + "data." + Data.RAW_CONTACT_ID
                        + " FROM " + "data "
                        + "JOIN mimetypes ON (data.mimetype_id = mimetypes._id)"
                        + " WHERE " + Data.MIMETYPE + "='"
                        + GroupMembership.CONTENT_ITEM_TYPE
                        + "' AND " + GroupMembership.GROUP_ROW_ID +
                        "=?)");
                // selection.append(" AND " + RawContacts.DELETED + "=0 )");
                selectionArgs.add(String.valueOf(filter.groupId));
                break;
            default:
                Log.w(TAG, "Unsupported filter type came " +
                        "(type: " + filter.filterType + ", toString: " + filter + ")" +
                        " showing all contacts.");
                // No selection.
                break;
        }
        if (isCustomFilterForPhoneNumbersOnly() && filterable) {
            selection.append(" AND " + Contacts.HAS_PHONE_NUMBER + "=1");
        }
        // add selection for AllInOneData
        if (Constants.DEBUG)
            Log.d(TAG, "selection: " + selection.toString());
        loader.setSelection(selection.toString());
        loader.setSelectionArgs(selectionArgs.toArray(new String[0]));
    }

    protected static Uri buildSectionIndexerUri(Uri uri) {
        return uri.buildUpon()
                .appendQueryParameter(Contacts.EXTRA_ADDRESS_BOOK_INDEX, "true").build();
}

    @Override
    public String getContactDisplayName(int position) {
        return ((Cursor) getItem(position)).getString(DataQuery.DATA_DISPLAY_NAME);
    }

    /**
     * Builds a {@link Data#CONTENT_URI} for the given cursor position.
     *
     * @return Uri for the data. may be null if the cursor is not ready.
     */
    public Uri getDataUri(int position) {
        Cursor cursor = ((Cursor) getItem(position));
        if (cursor != null) {
            long id = cursor.getLong(DataQuery.DATA_ID);
            return ContentUris.withAppendedId(Data.CONTENT_URI, id);
        } else {
            Log.w(TAG, "Cursor was null in getDataUri() call. Returning null instead.");
            return null;
        }
    }

    /**
     * Builds a {@link Data#CONTENT_URI} for the given cursor position.
     *
     * @return Uri for the contact. may be null if the cursor is not ready.
     */
    public Uri getContactUri(int position) {
        Cursor cursor = ((Cursor) getItem(position));
        if (cursor != null) {
            int partitionIndex = getPartitionForPosition(position);
            return getContactUri(partitionIndex, cursor, DataQuery.DATA_CONTACT_ID,
                    DataQuery.DATA_LOOKUP_KEY);
        } else {
            Log.w(TAG, "Cursor was null in getContactUri() call. Returning null instead.");
            return null;
        }
    }

    @Override
    protected ContactListItemView newView(Context context, int partition, Cursor cursor, int position,ViewGroup parent) {
        final ContactListItemView view = new ContactListItemView(context, null);
        view.setUnknownNameText(mUnknownNameText);
        view.setQuickContactEnabled(isQuickContactEnabled());
        view.setPhotoPosition(mPhotoPosition);
        /* SPRD: add for bug 381608 add the first letter in the listView  @{ */
        view.setIsSectionHeaderEnabled(isSectionHeaderDisplayEnabled());
        view.setAdjustSelectionBoundsEnabled(isAdjustSelectionBoundsEnabled());
        /* @} */
        return view;
    }

    @Override
    protected void bindView(View itemView, int partition, Cursor cursor, int position) {
        ContactListItemView view = (ContactListItemView) itemView;
        /* SPRD: add for bug 384274 the icon is not right after search  @{ */
        view.setIsSectionHeaderEnabled(isSectionHeaderDisplayEnabled());
        /* @} */
        // Look at elements before and after this position, checking if contact
        // IDs are same.
        // If they have one same contact ID, it means they can be grouped.
        //
        // In one group, only the first entry will show its photo and its name,
        // and the other
        // entries in the group show just their data (e.g. phone number, email
        // address).
        cursor.moveToPosition(position);
        boolean isFirstEntry = true;
        boolean showBottomDivider = true;
        final long currentContactId = cursor.getLong(DataQuery.DATA_CONTACT_ID);
        if (cursor.moveToPrevious() && !cursor.isBeforeFirst()) {
            final long previousContactId = cursor.getLong(DataQuery.DATA_CONTACT_ID);
            if (currentContactId == previousContactId) {
                isFirstEntry = false;
            }
        }
        cursor.moveToPosition(position);
        if (cursor.moveToNext() && !cursor.isAfterLast()) {
            final long nextContactId = cursor.getLong(DataQuery.DATA_CONTACT_ID);
            if (currentContactId == nextContactId) {
                // The following entry should be in the same group, which means
                // we don't want a
                // divider between them.
                // TODO: we want a different divider than the divider between
                // groups. Just hiding
                // this divider won't be enough.
                showBottomDivider = false;
            }
        }
        cursor.moveToPosition(position);

        bindSectionHeaderAndDivider(view, position);
        if (isFirstEntry) {
            bindName(view, cursor);
            if (isQuickContactEnabled()) {
                bindQuickContact(view, partition, cursor, DataQuery.DATA_PHOTO_ID,
                        DataQuery.DATA_PHOTO_URI,
                        DataQuery.DATA_CONTACT_ID, DataQuery.DATA_LOOKUP_KEY,
                        DataQuery.CONTACT_DISPLAY_ACCOUNT_TYPE,
                        DataQuery.CONTACT_DISPLAY_ACCOUNT_NAME,
                        DataQuery.DATA_DISPLAY_NAME);
            } else {
                bindPhoto(view, cursor);
            }
        } else {
            unbindName(view);

            view.removePhotoView(true, false);
        }
        bindAllInOneData(view, cursor);
        if (isMultiPickerSupported()) {
            // note: position is NOT the real position in the adapter, in face,
            // it is the relative offset in the partition
            /**
             * SPRD:Bug549112 Can't choose or delete in multi tab
             * @{
             */
            final int pos = getRealPosition(partition, position);
            view.getCheckBox().setOnCheckedChangeListener(
                    new OnCheckedChangeListener() {
                        @Override
                        public void onCheckedChanged(CompoundButton buttonView,
                                boolean isChecked) {
                            setChecked(pos, isChecked);
                            if (null != onListCheckedChangeListener) {
                                onListCheckedChangeListener
                                        .onListCheckedChanged();
                            }
                        }
                    });
            /**
             * @}
             */
            bindCheckbox(view, getRealPosition(partition, position));
        }
        view.setDividerVisible(showBottomDivider);
    }

    protected void bindCheckbox(final ContactListItemView view, int position) {
        view.showCheckbox(isChecked(position));
    }

    protected void bindAllInOneData(ContactListItemView view, Cursor cursor) {
        // TODO: bind mime type to label
        String mimeType = cursor.getString(DataQuery.DATA_MIMETYPE);
        int resId = -1;
        if (mimeType != null) {
            resId = AccountRestrictionUtils.get(getContext()).mimeToRes(mimeType);
        }
        if (resId != -1) {
            view.setLabel(getContext().getText(resId));
        }
        view.showData(cursor, DataQuery.DATA_DATA1);
    }

    protected void bindSectionHeaderAndDivider(final ContactListItemView view, int position) {
        if (isSectionHeaderDisplayEnabled()) {
            Placement placement = getItemPlacementInSection(position);
            view.setSectionHeader(placement.firstInSection ? placement.sectionHeader : null);
            view.setDividerVisible(!placement.lastInSection);
        } else {
            view.setSectionHeader(null);
            view.setDividerVisible(true);
        }
    }

    protected void bindName(final ContactListItemView view, Cursor cursor) {
        view.showDisplayName(cursor, DataQuery.DATA_DISPLAY_NAME, getContactNameDisplayOrder());
        // Note: we don't show phonetic names any more (see issue 5265330)
    }

    protected void unbindName(final ContactListItemView view) {
        view.hideDisplayName();
    }

    protected void bindPhoto(final ContactListItemView view, Cursor cursor) {
        long photoId = 0;
        if (!cursor.isNull(DataQuery.DATA_PHOTO_ID)) {
            photoId = cursor.getLong(DataQuery.DATA_PHOTO_ID);
        }

        DefaultImageRequest request = null;
        if (photoId == 0) {
             request = getDefaultImageRequestFromCursor(cursor, DataQuery.DATA_DISPLAY_NAME,
                     DataQuery.DATA_LOOKUP_KEY);
        }
        /* SPRD: add for bug 390202 the photo in the list of Sent via SMS is square
         *  getPhotoLoader().loadThumbnail(view.getPhotoView(), photoId, false, false,request);
         *  @{ */
        getPhotoLoader().loadThumbnail(view.getPhotoView(), photoId, false,true,request);
        /* end */
    }

    public void setPhotoPosition(ContactListItemView.PhotoPosition photoPosition) {
        mPhotoPosition = photoPosition;
    }

    public ContactListItemView.PhotoPosition getPhotoPosition() {
        return mPhotoPosition;
    }

    void setCascadingData(List<String> data) {
        mCascadingData = data;
    }

    private boolean isCustomFilterForPhoneNumbersOnly() {
        // TODO: this flag should not be stored in shared prefs. It needs to be
        // in the db.
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getContext());
        return prefs.getBoolean(ContactsPreferences.PREF_DISPLAY_ONLY_PHONES,
                ContactsPreferences.PREF_DISPLAY_ONLY_PHONES_DEFAULT);
    }

    public void setLoaderSelection(String selection) {
        mDataSelection = selection;
    }
}
