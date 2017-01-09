/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.contacts.common.list;

import android.content.ContentUris;
import android.content.Context;
import android.content.CursorLoader;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.Uri;
import android.net.Uri.Builder;
import android.preference.PreferenceManager;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Directory;
import android.provider.ContactsContract.SearchSnippets;
import android.support.annotation.VisibleForTesting;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.graphics.drawable.Drawable;

import com.android.contacts.common.Experiments;
import com.android.contacts.common.compat.ContactsCompat;
import com.android.contacts.common.preference.ContactsPreferences;
import com.android.contacts.commonbind.experiments.Flags;

import java.util.ArrayList;
import java.util.List;

//sprd add
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;

import com.android.contacts.common.model.account.AccountType;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.model.AccountTypeManager;
/**
 * A cursor adapter for the {@link ContactsContract.Contacts#CONTENT_TYPE} content type.
 */
public class DefaultContactListAdapter extends ContactListAdapter {

    private static final String TAG = "DefaultContactListAdapter";
    public static final char SNIPPET_START_MATCH = '[';
    public static final char SNIPPET_END_MATCH = ']';

    // Contacts contacted within the last 3 days (in seconds)
    private static final long LAST_TIME_USED_3_DAYS_SEC = 3L * 24 * 60 * 60;

    // Contacts contacted within the last 7 days (in seconds)
    private static final long LAST_TIME_USED_7_DAYS_SEC = 7L * 24 * 60 * 60;

    // Contacts contacted within the last 14 days (in seconds)
    private static final long LAST_TIME_USED_14_DAYS_SEC = 14L * 24 * 60 * 60;

    // Contacts contacted within the last 30 days (in seconds)
    private static final long LAST_TIME_USED_30_DAYS_SEC = 30L * 24 * 60 * 60;

    private static final String TIME_SINCE_LAST_USED_SEC =
            "(strftime('%s', 'now') - " + Contacts.LAST_TIME_CONTACTED + "/1000)";

    private static final String STREQUENT_SORT =
            "(CASE WHEN " + TIME_SINCE_LAST_USED_SEC + " < " + LAST_TIME_USED_3_DAYS_SEC +
                    " THEN 0 " +
                    " WHEN " + TIME_SINCE_LAST_USED_SEC + " < " + LAST_TIME_USED_7_DAYS_SEC +
                    " THEN 1 " +
                    " WHEN " + TIME_SINCE_LAST_USED_SEC + " < " + LAST_TIME_USED_14_DAYS_SEC +
                    " THEN 2 " +
                    " WHEN " + TIME_SINCE_LAST_USED_SEC + " < " + LAST_TIME_USED_30_DAYS_SEC +
                    " THEN 3 " +
                    " ELSE 4 END), " +
                    Contacts.TIMES_CONTACTED + " DESC, " +
                    Contacts.STARRED + " DESC";

    public DefaultContactListAdapter(Context context) {
        super(context);
    }

    @Override
    public void configureLoader(CursorLoader loader, long directoryId) {
        if (loader instanceof ProfileAndContactsLoader) {
            ((ProfileAndContactsLoader) loader).setLoadProfile(shouldIncludeProfile());
        }

        String sortOrder = null;
        ContactListFilter filter = getFilter();
        if (isSearchMode()) {
            final Flags flags = Flags.getInstance(mContext);
            String query = getQueryString();
            if (query == null) query = "";
            query = query.trim();
            if (TextUtils.isEmpty(query)) {
                // Regardless of the directory, we don't want anything returned,
                // so let's just send a "nothing" query to the local directory.
                loader.setUri(Contacts.CONTENT_URI);
                loader.setProjection(getProjection(false));
                loader.setSelection("0");
            } else {
                final Builder builder = ContactsCompat.getContentUri().buildUpon();
                appendSearchParameters(builder, query, directoryId);
                loader.setUri(builder.build());
                loader.setProjection(getProjection(true));
                if (flags.getBoolean(Experiments.FLAG_SEARCH_STREQUENTS_FIRST, false)) {
                    sortOrder = STREQUENT_SORT;
                }
                /**
                * SPRD:Bug474736 contacts to display.
                *
                * @{
                */
                configureSelection(loader, directoryId, filter, true);
                /**
                * @}
                */
            }
        } else {
            configureUri(loader, directoryId, filter);
            loader.setProjection(getProjection(false));
            configureSelection(loader, directoryId, filter);
        }

        if (getSortOrder() == ContactsPreferences.SORT_ORDER_PRIMARY) {
            if (sortOrder == null) {
                sortOrder = Contacts.SORT_KEY_PRIMARY;
            } else {
                sortOrder += ", " + Contacts.SORT_KEY_PRIMARY;
            }
        } else {
            if (sortOrder == null) {
                sortOrder = Contacts.SORT_KEY_ALTERNATIVE;
            } else {
                sortOrder += ", " + Contacts.SORT_KEY_ALTERNATIVE;
            }
        }
        loader.setSortOrder(sortOrder);
    }

    private void appendSearchParameters(Builder builder, String query, long directoryId) {
        builder.appendPath(query); // Builder will encode the query
        builder.appendQueryParameter(ContactsContract.DIRECTORY_PARAM_KEY,
                String.valueOf(directoryId));
        if (directoryId != Directory.DEFAULT && directoryId != Directory.LOCAL_INVISIBLE) {
            builder.appendQueryParameter(ContactsContract.LIMIT_PARAM_KEY,
                    String.valueOf(getDirectoryResultLimit(getDirectoryById(directoryId))));
        }
        builder.appendQueryParameter(SearchSnippets.DEFERRED_SNIPPETING_KEY, "1");
    }

    protected void configureUri(CursorLoader loader, long directoryId, ContactListFilter filter) {
        Uri uri = Contacts.CONTENT_URI;
        if (filter != null && filter.filterType == ContactListFilter.FILTER_TYPE_SINGLE_CONTACT) {
            String lookupKey = getSelectedContactLookupKey();
            if (lookupKey != null) {
                uri = Uri.withAppendedPath(Contacts.CONTENT_LOOKUP_URI, lookupKey);
            } else {
                uri = ContentUris.withAppendedId(Contacts.CONTENT_URI, getSelectedContactId());
            }
        }

        if (directoryId == Directory.DEFAULT && isSectionHeaderDisplayEnabled()) {
            uri = ContactListAdapter.buildSectionIndexerUri(uri);
        }

        // The "All accounts" filter is the same as the entire contents of Directory.DEFAULT
        if (filter != null
                && filter.filterType != ContactListFilter.FILTER_TYPE_CUSTOM
                && filter.filterType != ContactListFilter.FILTER_TYPE_SINGLE_CONTACT) {
            final Uri.Builder builder = uri.buildUpon();
            builder.appendQueryParameter(
                    ContactsContract.DIRECTORY_PARAM_KEY, String.valueOf(Directory.DEFAULT));
            if (filter.filterType == ContactListFilter.FILTER_TYPE_ACCOUNT) {
                filter.addAccountQueryParameterToUrl(builder);
            }
            uri = builder.build();
        }

        loader.setUri(uri);
    }

    /**
    * SPRD:
    *   Defer the action to make the window properly repaint.
    *
    * Original Android code:
    *     private void configureSelection(
            CursorLoader loader, long directoryId, ContactListFilter filter) {
    *
    * @{
    */
    private void configureSelection(
            CursorLoader loader, long directoryId, ContactListFilter filter) {
        configureSelection(loader, directoryId, filter, false);
    }
    /**
    * @}
    */
    /**
    * SPRD:
    *  add isSearchMode parameter
    *
    * Original Android code:
    * private void configureSelection(
            CursorLoader loader, long directoryId, ContactListFilter filter)
    *
    * @{
    */

    private void configureSelection(
            CursorLoader loader, long directoryId, ContactListFilter filter, boolean isSearchMode) {
        /**
         * @}
         */
        if (filter == null) {
            return;
        }

        if (directoryId != Directory.DEFAULT) {
            return;
        }

        StringBuilder selection = new StringBuilder();
        List<String> selectionArgs = new ArrayList<String>();

        switch (filter.filterType) {
            case ContactListFilter.FILTER_TYPE_ALL_ACCOUNTS: {
                // We have already added directory=0 to the URI, which takes care of this
                // filter
                /**
                * SPRD:Bug474736 Contacts to display.
                *
                * @{
                */
                // sunway: add additional filter according to AccountTypeManager
                // state, those account not "syncable" will not be shown in
                // ALL_ACCOUNT;
                AccountTypeManager am = AccountTypeManager.getInstance(getContext());
                List<AccountWithDataSet> accounts = am.getAccounts(false);

                selection.append(
                        " EXISTS ("
                                + "SELECT DISTINCT " + RawContacts.CONTACT_ID
                                + " FROM view_raw_contacts"
                                + " WHERE ( " + "view_contacts." + Contacts._ID + "="
                                + RawContacts.CONTACT_ID
                                + "  AND (("
                                + RawContacts.ACCOUNT_TYPE + " is NULL "
                                + " AND " + RawContacts.ACCOUNT_NAME + " is NULL )");
                for (AccountWithDataSet account : accounts) {
                    selection.append(" or ( " + RawContacts.ACCOUNT_TYPE + "=?" +
                            " AND " + RawContacts.ACCOUNT_NAME + "=?" +
                            ") "
                            );
                    selectionArgs.add(account.type);
                    selectionArgs.add(account.name);
                    if(mListRequestModeSelection != null
                        && ("mode_delete".equals(mListRequestModeSelection) || "mode_copyto".equals(mListRequestModeSelection))
                        && ("sprd.com.android.account.usim".equals(account.type)
                            || "sprd.com.android.account.sim".equals(account.type))) {
                        selection.append(" AND " + RawContacts.SYNC1 + "!='sdn'");
                    }
                }
                selection.append(")");
                if(mExcludeReadOnly){
                    selection.append(" AND " + RawContacts.RAW_CONTACT_IS_READ_ONLY + "=0");
                }
                selection.append("))");
                if (isCustomFilterForPhoneNumbersOnly()) {
                    selection.append(" AND " + Contacts.HAS_PHONE_NUMBER + "=1");
                }
                /**
                * @}
                */
                break;
            }
            case ContactListFilter.FILTER_TYPE_SINGLE_CONTACT: {
                // We have already added the lookup key to the URI, which takes care of this
                // filter
                break;
            }
            case ContactListFilter.FILTER_TYPE_STARRED: {
                selection.append(Contacts.STARRED + "!=0");
                break;
            }
            case ContactListFilter.FILTER_TYPE_WITH_PHONE_NUMBERS_ONLY: {
                selection.append(Contacts.HAS_PHONE_NUMBER + "=1");
                break;
            }
            case ContactListFilter.FILTER_TYPE_CUSTOM: {
                selection.append(Contacts.IN_VISIBLE_GROUP + "=1");
                if (isCustomFilterForPhoneNumbersOnly()) {
                    selection.append(" AND " + Contacts.HAS_PHONE_NUMBER + "=1");
                }
                if(mListRequestModeSelection != null &&
                        ("mode_delete".equals(mListRequestModeSelection) || "mode_copyto".equals(mListRequestModeSelection))) {
                    /*SPRD: 552449 The DUT fails to show contacts while deleting contacts
                     in custom filter.*/
                    selection.append(" AND (" + RawContacts.SYNC1 + " IS NOT " + "'sdn')");
                }

                /**
                 * SPRD:Bug556834 During custom view, default contact could be delete through batch operation
                 * @{
                 */
                if (mExcludeReadOnly) {
                    selection.append(" AND " + Data.IS_READ_ONLY + "=0");
                }
                /**
                 * @}
                 */

                break;
            }
            /**
             * SPRD:Bug474736 Contacts to display.
             * @{
             */
            case ContactListFilter.FILTER_TYPE_ACCOUNTS: {
                // TODO: avoid the use of private API
                selection.append(
                        " EXISTS ("
                                + "SELECT DISTINCT " + RawContacts.CONTACT_ID
                                + " FROM view_raw_contacts WHERE ( "
                                + "view_contacts." + Contacts._ID + "=" + RawContacts.CONTACT_ID
                                + " AND (");
                boolean init = true;
                if (filter.accounts == null || filter.accounts.size() == 0) {
                    selection.append("(" + RawContacts.ACCOUNT_TYPE + "=\"" + NO_ACCOUNT + "\"");
                    selection.append(" AND ");
                    selection.append(RawContacts.ACCOUNT_NAME + "=\"" + NO_ACCOUNT
                            + "\")");
                } else {
                    //SPRD:Bug400108 Remove the starred contacts by account sync from Batch starred list.
                    selection.append("(");
                    for (AccountWithDataSet account : filter.accounts) {
                        if (!init) {
                            selection.append(" OR ");
                        }
                        init = false;
                        selection.append("(" + RawContacts.ACCOUNT_TYPE + "=\"" + account.type
                                + "\"");
                        selection.append(" AND ");
                        selection.append(RawContacts.ACCOUNT_NAME + "=\"" + account.name + "\")");
                        if(mListRequestModeSelection != null &&
                            ("mode_delete".equals(mListRequestModeSelection) || "mode_copyto".equals(mListRequestModeSelection))
                            && ("sprd.com.android.account.usim".equals(account.type)
                                || "sprd.com.android.account.sim".equals(account.type))) {
                            selection.append(" AND " + RawContacts.SYNC1 + " != 'sdn'");
                        }
                        //SPRD: add for bug617830/621857, add fdn feature
                        if(mListRequestModeSelection != null &&
                                ("shortcut_contact".equals(mListRequestModeSelection)
                                        || "star_contact".equals(mListRequestModeSelection)
                                        || "mode_copyto".equals(mListRequestModeSelection))) {
                            selection.append(" AND (" + RawContacts.SYNC2 + " NOT LIKE " + "'fdn%' OR " + RawContacts.SYNC2 + " IS NULL)");
                        }
                    }
                    //add for Bug400108
                    selection.append(")");
                }
                if (isCustomFilterForPhoneNumbersOnly()) {
                    selection.append(" AND " + Contacts.HAS_PHONE_NUMBER + "=1");
                }
                if(mIsStarMemSelection){
                    selection.append(" AND " + Contacts.STARRED + "=0");
                }
                /**
                 * SPRD:Bug535983 The default contact should not be showed when setting head image.
                 * @{
                 */
                if (mExcludeReadOnly) {
                    selection.append(" AND " + RawContacts.RAW_CONTACT_IS_READ_ONLY + "=0");
                }
                /**
                 * @}
                 */
                selection.append(")))");
                break;
            }
            /**
            * @}
            */
            case ContactListFilter.FILTER_TYPE_ACCOUNT: {
                // We use query parameters for account filter, so no selection to add here.
                /**
                 * SPRD:Bug474736 Contacts to display.
                 *
                 * @{
                 */
                if (isSearchMode) {
                    selection.append(
                            " EXISTS ("
                                    + "SELECT DISTINCT " + RawContacts.CONTACT_ID
                                    + " FROM view_raw_contacts"
                                    + " WHERE ( " + "view_contacts." + Contacts._ID + "="
                                    + RawContacts.CONTACT_ID + " AND " + RawContacts.ACCOUNT_TYPE
                                    + "=?"
                                    + " AND " + RawContacts.ACCOUNT_NAME + "=? )");
                    selectionArgs.add(filter.accountType);
                    selectionArgs.add(filter.accountName);
                    selection.append(")");
                } else {
                    selection.append(
                            " EXISTS ("
                                    + "SELECT DISTINCT " + RawContacts.CONTACT_ID
                                    + " FROM view_raw_contacts"
                                    + " WHERE ( " + "view_contacts." + Contacts._ID + "="
                                    + RawContacts.CONTACT_ID );
                    if(mExcludeReadOnly){
                        selection.append(" AND " + RawContacts.RAW_CONTACT_IS_READ_ONLY + "=0");
                    }
                    selection.append("))");
                }
                if (isCustomFilterForPhoneNumbersOnly()) {
                    selection.append(" AND " + Contacts.HAS_PHONE_NUMBER + "=1");
                }
                if (mAddGroupMemSelection != null && !mAddGroupMemSelection.isEmpty()) {
                    selection.append(" AND " + Contacts._ID);
                    selection.append(" NOT IN (" + mAddGroupMemSelection + ")");
                }
                if (mListRequestModeSelection != null
                        && ("mode_group_select".equals(mListRequestModeSelection)
                            || "mode_copyto".equals(mListRequestModeSelection)
                            || "mode_delete".equals(mListRequestModeSelection))
                        && ("sprd.com.android.account.usim".equals(filter.accountType)
                            || "sprd.com.android.account.sim".equals(filter.accountType))) {
                    selection.append(" AND " + RawContacts.SYNC1);
                    selection.append("!='sdn'");
                }
                /**
                * @}
                */
                //SPRD: add for bug617830, add fdn feature
                selection.append(" AND (" + RawContacts.SYNC2 + " NOT LIKE " + "'fdn%' OR " + RawContacts.SYNC2 + " IS NULL)");
                break;
            }
            /**
            * SPRD:
            *
            * @{
            */
            case ContactListFilter.FILTER_TYPE_GROUP_MEMBER: {
                Long groupId = filter.groupId;
                selection.append(
                        Contacts._ID + " IN ("
                                + "SELECT " + RawContacts.CONTACT_ID
                                + " FROM " + "raw_contacts"
                                + " WHERE " + "raw_contacts._id" + " IN "
                                + "(SELECT " + "data." + Data.RAW_CONTACT_ID
                                + " FROM " + "data "
                                + "JOIN mimetypes ON (data.mimetype_id = mimetypes._id)"
                                + " WHERE " + Data.MIMETYPE + "='"
                                + GroupMembership.CONTENT_ITEM_TYPE
                                + "' AND " + GroupMembership.GROUP_ROW_ID +
                                "=?)");
                selection.append(" AND " + RawContacts.DELETED + "=0 )");
                selectionArgs.add(String.valueOf(filter.groupId));
            }
            /**
            * @}
            */
        }
        loader.setSelection(selection.toString());
        loader.setSelectionArgs(selectionArgs.toArray(new String[0]));
    }

    @Override
    protected void bindView(View itemView, int partition, Cursor cursor, int position) {
        super.bindView(itemView, partition, cursor, position);
        final ContactListItemView view = (ContactListItemView)itemView;

        view.setHighlightedPrefix(isSearchMode() ? getUpperCaseQueryString() : null);

        if (isSelectionVisible()) {
            view.setActivated(isSelectedContact(partition, cursor));
        }

        bindSectionHeaderAndDivider(view, position, cursor);

        if (isQuickContactEnabled()) {
            /**
            * SPRD:
            *
            *
            * Original Android code:
            bindQuickContact(view, partition, cursor, ContactQuery.CONTACT_PHOTO_ID,
                    ContactQuery.CONTACT_PHOTO_URI, ContactQuery.CONTACT_ID,
                    ContactQuery.CONTACT_LOOKUP_KEY, ContactQuery.CONTACT_DISPLAY_NAME);
            /*
            * @{
            */
            bindQuickContact(view, partition, cursor, ContactQuery.CONTACT_PHOTO_ID,
                    ContactQuery.CONTACT_PHOTO_URI, ContactQuery.CONTACT_ID,
                    ContactQuery.CONTACT_LOOKUP_KEY, ContactQuery.CONTACT_DISPLAY_ACCOUNT_TYPE,
                    ContactQuery.CONTACT_DISPLAY_ACCOUNT_NAME, ContactQuery.CONTACT_DISPLAY_NAME);
            /**
            * @}
            */

        } else {
            if (getDisplayPhotos()) {
                bindPhoto(view, partition, cursor);
            }
        }

        bindNameAndViewId(view, cursor);
       /**
        * SPRD:Bug 474752 Add features with multiSelection activity in Contacts.
        *
        * @{
        */
        if (isMultiPickerSupported()) {
           bindCheckbox(view, getRealPosition(partition, position));
        }
        /**
        * @}
        */

        bindPresenceAndStatusMessage(view, cursor);

       /**
        * SPRD Bug505150 after updating contact with facebook, but these contacts'
        * icons is default, but not facebook
        */
        if (!isMultiPickerSupported()) {
            Drawable drawable = null;
            String accountType = cursor.getString(ContactQuery.CONTACT_DISPLAY_ACCOUNT_TYPE);
            String accountName = cursor.getString(ContactQuery.CONTACT_DISPLAY_ACCOUNT_NAME);

            if (accountType != null
                    && accountName != null
                    && !(accountName.equals("Phone") || accountType
                            .equals("sprd.com.android.account.phone"))) {
                AccountWithDataSet account = new AccountWithDataSet(accountName, accountType, null);
                boolean isSdn = false;
                /**
                 * SPRD:Bug 559634 adjust the size of contact account icon.
                 * SPRD:AndroidN porting add sim icon feature.
                 * @{
                 */
                String sdnStr = cursor.getString(cursor.getColumnIndex(RawContacts.SYNC1));
                if (accountType.equalsIgnoreCase(AccountTypeManager.ACCOUNT_SIM)
                        || accountType.equalsIgnoreCase(AccountTypeManager.ACCOUNT_USIM)) {
                    if ("sdn".equals(sdnStr)) {
                        isSdn = true;
                    }
                    AccountType simAccountType = AccountTypeManager.getInstance(mContext)
                            .getAccountType(accountType, null);
                    if (simAccountType != null) {
                        drawable = AccountTypeManager.getInstance(mContext).getListSimIcon(accountType, accountName,
                                isSdn);
                    }
                } else {
                    drawable = AccountTypeManager.getInstance(mContext).getAccountIcon(account,
                            isSdn);
                }
                /**
                 * @}
                 */
                view.setAccountView(drawable);
            } else {
                /* SPRD: add for bug617830/621828, add fdn feature @}*/
                String fdnStr = cursor.getString(cursor.getColumnIndex(RawContacts.SYNC2));
                boolean isFdn = false;
                int phoneId = -1;
                if (fdnStr != null && fdnStr != "" && fdnStr.startsWith("fdn")) {
                    isFdn = true;
                    phoneId = Integer.parseInt(fdnStr.substring(3));
                }
                Log.d(TAG, "fdnStr = " + fdnStr);
                if (accountName != null && accountName.equals("Phone") && isFdn && phoneId != -1) {
                    drawable = AccountTypeManager.getInstance(mContext).getListFdnIcon(phoneId);
                    view.setAccountView(drawable);
                } else {
                    view.setAccountView(null);
                }
                /* @}*/
            }
        }

        if (isSearchMode()) {
            bindSearchSnippet(view, cursor);
        } else {
            view.setSnippet(null);
        }
    }

    private boolean isCustomFilterForPhoneNumbersOnly() {
        // TODO: this flag should not be stored in shared prefs.  It needs to be in the db.
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(getContext());
        return prefs.getBoolean(ContactsPreferences.PREF_DISPLAY_ONLY_PHONES,
                ContactsPreferences.PREF_DISPLAY_ONLY_PHONES_DEFAULT);
    }



    /**
    * SPRD:Bug 474752 Add features with multiSelection activity in Contacts.
    *
    * @{
    */
    public static final String NO_ACCOUNT = "sprd.com.android.account.null";
    private String mAddGroupMemSelection;
    private String mListRequestModeSelection;
    private boolean mIsStarMemSelection = false;
    private boolean mExcludeReadOnly = false;

    public void setAddGroupMemSelection(String selection) {
        mAddGroupMemSelection = selection;
    }

    public void setListRequestModeSelection(String selection) {
        mListRequestModeSelection = selection;
    }

    public void setStarMemSelection(){
        mIsStarMemSelection = true;
    }
    /**
    * @}
    */
    public void setExcludeReadOnly(boolean excludeReadOnly){
        mExcludeReadOnly = excludeReadOnly;
    }
}
