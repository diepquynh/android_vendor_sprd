/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.contacts.editor;

import com.android.contacts.common.logging.ScreenEvent.ScreenType;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Lists;
import com.android.contacts.ContactSaveService;
import com.android.contacts.GroupMetaDataLoader;
import com.android.contacts.R;
import com.android.contacts.activities.ContactEditorAccountsChangedActivity;
import com.android.contacts.activities.ContactEditorBaseActivity;
import com.android.contacts.activities.ContactEditorBaseActivity.ContactEditor;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.Contact;
import com.android.contacts.common.model.ContactLoader;
import com.android.contacts.common.model.RawContact;
import com.android.contacts.common.model.RawContactDelta;
import com.android.contacts.common.model.RawContactDeltaList;
import com.android.contacts.common.model.RawContactModifier;
import com.android.contacts.common.model.ValuesDelta;
import com.android.contacts.common.model.account.AccountType;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.util.ImplicitIntentsUtil;
import com.android.contacts.common.util.MaterialColorMapUtils;
import com.android.contacts.editor.AggregationSuggestionEngine.Suggestion;
import com.android.contacts.interactions.ContactDeletionInteraction;
import com.android.contacts.list.UiIntentActions;
import com.android.contacts.quickcontact.QuickContactActivity;
import com.android.contacts.util.HelpUtils;
import com.android.contacts.util.PhoneCapabilityTester;
import com.android.contacts.util.UiClosables;

import android.accounts.Account;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.LoaderManager;
import android.content.ActivityNotFoundException;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.CursorLoader;
import android.content.Intent;
import android.content.Loader;
import android.database.Cursor;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.SystemClock;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Event;
import android.provider.ContactsContract.CommonDataKinds.Organization;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.CommonDataKinds.StructuredPostal;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Intents;
import android.provider.ContactsContract.RawContacts;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.LinearLayout;
import android.widget.ListPopupWindow;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
/**
 * SPRD:
 *
 * @{
 */
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Message;
import android.provider.ContactsContract.CommonDataKinds;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.Data;
import com.android.contacts.common.util.Constants;
import com.sprd.contacts.util.AccountRestrictionUtils;
import com.sprd.contacts.util.AccountsForMimeTypeUtils;
import com.sprd.contacts.common.model.account.SimAccountType;
import com.sprd.contacts.common.model.account.USimAccountType;
import com.sprd.contacts.common.model.account.PhoneAccountType;
import java.util.Set;
import android.Manifest.permission;
import android.os.Trace;
import android.content.pm.PackageManager;
import com.android.contacts.interactions.JoinContactsDialogFragment;
/**
  * @}
  */
import android.database.ContentObserver;
import android.os.Handler;
import android.content.ContentUris;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.content.SharedPreferences.Editor;
/**
 * Base Fragment for contact editors.
 */
abstract public class ContactEditorBaseFragment extends Fragment implements
        ContactEditor, SplitContactConfirmationDialogFragment.Listener,
        JoinContactConfirmationDialogFragment.Listener,
        AggregationSuggestionEngine.Listener, AggregationSuggestionView.Listener,
        CancelEditDialogFragment.Listener {

    static final String TAG = "ContactEditor";

    protected static final int LOADER_CONTACT = 1;
    protected static final int LOADER_GROUPS = 2;

    private static final List<String> VALID_INTENT_ACTIONS = new ArrayList<String>() {{
        add(Intent.ACTION_EDIT);
        add(Intent.ACTION_INSERT);
        add(ContactEditorBaseActivity.ACTION_EDIT);
        add(ContactEditorBaseActivity.ACTION_INSERT);
        add(ContactEditorBaseActivity.ACTION_SAVE_COMPLETED);
    }};

    /**
     * SPRD:Bug610452 remember selected station and restore the
     * station when split-screen
     * @{
     */
    public static int dataItem = -1;
    /**
     * @}
     */
    /**
     * SPRD:Bug612672 The editor is closed when rotate the screen
     * @{
     */
    public boolean sameDataChanged = false;
    public boolean isSave = false;
    /**
     * @}
     */
    /**
     * SPRD:Bug617126 Conflict prompt have occured when Edit and save
     * contacts twice,and press home
     * @{
     */
    private static final String KEY_URI_CURRENT = "uriCurrent";
    /**
     * @}
     */
    private static final String KEY_ACTION = "action";
    private static final String KEY_URI = "uri";
    private static final String KEY_AUTO_ADD_TO_DEFAULT_GROUP = "autoAddToDefaultGroup";
    private static final String KEY_DISABLE_DELETE_MENU_OPTION = "disableDeleteMenuOption";
    private static final String KEY_NEW_LOCAL_PROFILE = "newLocalProfile";
    private static final String KEY_MATERIAL_PALETTE = "materialPalette";
    private static final String KEY_PHOTO_ID = "photoId";

    private static final String KEY_VIEW_ID_GENERATOR = "viewidgenerator";

    private static final String KEY_RAW_CONTACTS = "rawContacts";

    private static final String KEY_EDIT_STATE = "state";
    private static final String KEY_STATUS = "status";

    private static final String KEY_HAS_NEW_CONTACT = "hasNewContact";
    private static final String KEY_NEW_CONTACT_READY = "newContactDataReady";

    private static final String KEY_IS_EDIT = "isEdit";
    private static final String KEY_EXISTING_CONTACT_READY = "existingContactDataReady";

    private static final String KEY_RAW_CONTACT_DISPLAY_ALONE_IS_READ_ONLY = "isReadOnly";

    // Phone option menus
    private static final String KEY_SEND_TO_VOICE_MAIL_STATE = "sendToVoicemailState";
    private static final String KEY_ARE_PHONE_OPTIONS_CHANGEABLE = "arePhoneOptionsChangable";
    private static final String KEY_CUSTOM_RINGTONE = "customRingtone";

    private static final String KEY_IS_USER_PROFILE = "isUserProfile";

    private static final String KEY_ENABLED = "enabled";

    // Aggregation PopupWindow
    private static final String KEY_AGGREGATION_SUGGESTIONS_RAW_CONTACT_ID =
            "aggregationSuggestionsRawContactId";

    // Join Activity
    private static final String KEY_CONTACT_ID_FOR_JOIN = "contactidforjoin";

    private static final String KEY_READ_ONLY_DISPLAY_NAME = "readOnlyDisplayName";
    // SPRD Bug597956 add
    private static final String KEY_ACCOUNT_WITH_DATASET = "accountWithDataSet";

    protected static final int REQUEST_CODE_JOIN = 0;
    protected static final int REQUEST_CODE_ACCOUNTS_CHANGED = 1;
    protected static final int REQUEST_CODE_PICK_RINGTONE = 2;

    private static final int CURRENT_API_VERSION = android.os.Build.VERSION.SDK_INT;

    /**
     * An intent extra that forces the editor to add the edited contact
     * to the default group (e.g. "My Contacts").
     */
    public static final String INTENT_EXTRA_ADD_TO_DEFAULT_DIRECTORY = "addToDefaultDirectory";

    public static final String INTENT_EXTRA_NEW_LOCAL_PROFILE = "newLocalProfile";

    public static final String INTENT_EXTRA_DISABLE_DELETE_MENU_OPTION =
            "disableDeleteMenuOption";

    /**
     * Intent key to pass the photo palette primary color calculated by
     * {@link com.android.contacts.quickcontact.QuickContactActivity} to the editor and between
     * the compact and fully expanded editors.
     */
    public static final String INTENT_EXTRA_MATERIAL_PALETTE_PRIMARY_COLOR =
            "material_palette_primary_color";

    /**
     * Intent key to pass the photo palette secondary color calculated by
     * {@link com.android.contacts.quickcontact.QuickContactActivity} to the editor and between
     * the compact and fully expanded editors.
     */
    public static final String INTENT_EXTRA_MATERIAL_PALETTE_SECONDARY_COLOR =
            "material_palette_secondary_color";

    /**
     * Intent key to pass the ID of the photo to display on the editor.
     */
    public static final String INTENT_EXTRA_PHOTO_ID = "photo_id";

    /**
     * Intent key to pass the ID of the raw contact id that should be displayed in the full editor
     * by itself.
     */
    public static final String INTENT_EXTRA_RAW_CONTACT_ID_TO_DISPLAY_ALONE =
            "raw_contact_id_to_display_alone";

    /**
     * Intent key to pass the boolean value of if the raw contact id that should be displayed
     * in the full editor by itself is read-only.
     */
    public static final String INTENT_EXTRA_RAW_CONTACT_DISPLAY_ALONE_IS_READ_ONLY =
            "raw_contact_display_alone_is_read_only";

    /**
     * Intent extra to specify a {@link ContactEditor.SaveMode}.
     */
    public static final String SAVE_MODE_EXTRA_KEY = "saveMode";
    //Bug440418 Save the selected sim number on replaceDialog  when rotating the screen.
    private static final String SELECTED_DATA_ID = "selectDataId";
    private static final String SELECTED_DATA_ITEM = "selectDataItem";
    /**
     * Intent extra key for the contact ID to join the current contact to after saving.
     */
    public static final String JOIN_CONTACT_ID_EXTRA_KEY = "joinContactId";
    // Add for Bug535284 More menu disappear while power off and on while edit sim contact
    public static boolean mContactFound = false;

    //SPRD: add for bug615040, limit the join contacts to 10.
    private static final int MAX_JOIN_CONTACTS_NUMBER = 10;

    /**
     * Callbacks for Activities that host contact editors Fragments.
     */
    public interface Listener {

        /**
         * Contact was not found, so somehow close this fragment. This is raised after a contact
         * is removed via Menu/Delete
         */
        void onContactNotFound();

        /**
         * Contact was split, so we can close now.
         *
         * @param newLookupUri The lookup uri of the new contact that should be shown to the user.
         *                     The editor tries best to chose the most natural contact here.
         */
        void onContactSplit(Uri newLookupUri);

        /**
         * User has tapped Revert, close the fragment now.
         */
        void onReverted();

        /**
         * Contact was saved and the Fragment can now be closed safely.
         */
        void onSaveFinished(Intent resultIntent);

        /**
         * User switched to editing a different contact (a suggestion from the
         * aggregation engine).
         */
        void onEditOtherContactRequested(Uri contactLookupUri,
                ArrayList<ContentValues> contentValues);

        /**
         * Contact is being created for an external account that provides its own
         * new contact activity.
         */
        void onCustomCreateContactActivityRequested(AccountWithDataSet account,
                Bundle intentExtras);

        /**
         * The edited raw contact belongs to an external account that provides
         * its own edit activity.
         *
         * @param redirect indicates that the current editor should be closed
         *                 before the custom editor is shown.
         */
        void onCustomEditContactActivityRequested(AccountWithDataSet account, Uri rawContactUri,
                Bundle intentExtras, boolean redirect);

        /**
         * User has requested that contact be deleted.
         */
        void onDeleteRequested(Uri contactUri);
    }

    /**
     * Adapter for aggregation suggestions displayed in a PopupWindow when
     * editor fields change.
     */
    protected static final class AggregationSuggestionAdapter extends BaseAdapter {
        private final LayoutInflater mLayoutInflater;
        private final boolean mSetNewContact;
        private final AggregationSuggestionView.Listener mListener;
        private final List<AggregationSuggestionEngine.Suggestion> mSuggestions;

        public AggregationSuggestionAdapter(Activity activity, boolean setNewContact,
                AggregationSuggestionView.Listener listener, List<Suggestion> suggestions) {
            mLayoutInflater = activity.getLayoutInflater();
            mSetNewContact = setNewContact;
            mListener = listener;
            mSuggestions = suggestions;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final Suggestion suggestion = (Suggestion) getItem(position);
            final AggregationSuggestionView suggestionView =
                    (AggregationSuggestionView) mLayoutInflater.inflate(
                            R.layout.aggregation_suggestions_item, null);
            suggestionView.setNewContact(mSetNewContact);
            suggestionView.setListener(mListener);
            suggestionView.bindSuggestion(suggestion);
            return suggestionView;
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public Object getItem(int position) {
            return mSuggestions.get(position);
        }

        @Override
        public int getCount() {
            return mSuggestions.size();
        }
    }

    protected Context mContext;
    protected Listener mListener;

    //
    // Views
    //
    protected LinearLayout mContent;
    protected View mAggregationSuggestionView;
    protected ListPopupWindow mAggregationSuggestionPopup;

    //
    // Parameters passed in on {@link #load}
    //
    protected String mAction;
    protected Uri mLookupUri;
    protected Bundle mIntentExtras;
    protected boolean mAutoAddToDefaultGroup;
    protected boolean mDisableDeleteMenuOption;
    protected boolean mNewLocalProfile;
    protected MaterialColorMapUtils.MaterialPalette mMaterialPalette;
    protected long mPhotoId = -1;

    //
    // Helpers
    //
    protected ContactEditorUtils mEditorUtils;
    protected RawContactDeltaComparator mComparator;
    protected ViewIdGenerator mViewIdGenerator;
    private AggregationSuggestionEngine mAggregationSuggestionEngine;

    //
    // Loaded data
    //
    // Used to store existing contact data so it can be re-applied during a rebind call,
    // i.e. account switch.  Only used in {@link ContactEditorFragment}.
    /*
     * SPRD:
     * Original Android code:
    protected ImmutableList<RawContact> mRawContacts;
    SPRD: Bug 606497 The DUT crashes after select replace original contact
     * @{
     */
    protected static ImmutableList<RawContact> mRawContacts;
    protected static ImmutableList<RawContact> sRawContactsforReplaceDialog;
    /*
     * @}
     */
    /**
     * SPRD:Bug498039 Dismiss the custom label dialog, when lock the screen.
     * @{
     */
    protected View mContactEditor;
    /**
     * @}
     */
    protected Cursor mGroupMetaData;

    //
    // Editor state
    //
    protected RawContactDeltaList mState;
    protected int mStatus;
    protected long mRawContactIdToDisplayAlone = -1;
    protected boolean mRawContactDisplayAloneIsReadOnly = false;

    // Whether to show the new contact blank form and if it's corresponding delta is ready.
    protected boolean mHasNewContact;
    protected AccountWithDataSet mAccountWithDataSet;
    protected boolean mNewContactDataReady;
    protected boolean mNewContactAccountChanged;

    // Whether it's an edit of existing contact and if it's corresponding delta is ready.
    protected boolean mIsEdit;
    protected boolean mExistingContactDataReady;

    // Whether we are editing the "me" profile
    protected boolean mIsUserProfile;

    // Phone specific option menu items
    private boolean mSendToVoicemailState;
    private boolean mArePhoneOptionsChangable;
    private String mCustomRingtone;

    // Whether editor views and options menu items should be enabled
    private boolean mEnabled = true;

    // Aggregation PopupWindow
    private long mAggregationSuggestionsRawContactId;

    // Join Activity
    protected long mContactIdForJoin;

    // Full resolution photo URIs
    protected Bundle mUpdatedPhotos = new Bundle();

    // Used to pre-populate the editor with a display name when a user edits a read-only contact.
    protected String mReadOnlyDisplayName;

    //
    // Not saved/restored on rotates
    //

    // The name editor view for the new raw contact that was created so that the user can
    // edit a read-only contact (to which the new raw contact was joined)
    protected StructuredNameEditorView mReadOnlyNameEditorView;

    /**
     * The contact data loader listener.
     */
    protected final LoaderManager.LoaderCallbacks<Contact> mContactLoaderListener =
            new LoaderManager.LoaderCallbacks<Contact>() {

                protected long mLoaderStartTime;

                @Override
                public Loader<Contact> onCreateLoader(int id, Bundle args) {
                    mLoaderStartTime = SystemClock.elapsedRealtime();
                    return new ContactLoader(mContext, mLookupUri, true);
                }

                @Override
                public void onLoadFinished(Loader<Contact> loader, Contact contact) {
                    final long loaderCurrentTime = SystemClock.elapsedRealtime();
                    Log.v(TAG, "Time needed for loading: " + (loaderCurrentTime-mLoaderStartTime));
                    if (!contact.isLoaded()) {
                        // Item has been deleted. Close activity without saving again.
                        Log.i(TAG, "No contact found. Closing activity");
                        mStatus = Status.CLOSING;
                        if (mListener != null) mListener.onContactNotFound();
                        return;
                    }

                    mStatus = Status.EDITING;
                    mLookupUri = contact.getLookupUri();
                    final long setDataStartTime = SystemClock.elapsedRealtime();
                    setState(contact);
                    setStateForPhoneMenuItems(contact);
                    final long setDataEndTime = SystemClock.elapsedRealtime();

                    Log.v(TAG, "Time needed for setting UI: " + (setDataEndTime - setDataStartTime));
                    // Add for Bug535284 More menu disappear while power off and on while edit
                    if (!contact.isNotFound() && !contact.isError()) {
                        mContactFound = true;
                    }

                }

                @Override
                public void onLoaderReset(Loader<Contact> loader) {
                }
            };

    /**
     * The groups meta data loader listener.
     */
    protected final LoaderManager.LoaderCallbacks<Cursor> mGroupsLoaderListener =
            new LoaderManager.LoaderCallbacks<Cursor>() {

                @Override
                public CursorLoader onCreateLoader(int id, Bundle args) {
                    return new GroupMetaDataLoader(mContext, ContactsContract.Groups.CONTENT_URI);
                }

                @Override
                public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
                    mGroupMetaData = data;
                    setGroupMetaData();
                }

                @Override
                public void onLoaderReset(Loader<Cursor> loader) {
                }
            };
            /**
             * SPRD:Bug612672 The editor is closed when rotate the screen
             * @{
             */
            public final ContentObserver mObserver = new ContentObserver(new Handler())  {
                @Override
                public void onChange(boolean selfChange, Uri uri){
                  if ((!isSave)&&isSimType()) {
                     SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(mContext);
                     String newuri = pref.getString(KEY_URI_CURRENT, null);
                     if(mLookupUri != null){
                       if (newuri != null && newuri.equals(mLookupUri.toString())) {
                           sameDataChanged = true;
                           Toast.makeText(mContext,
                             R.string.editor_data_changed,Toast.LENGTH_LONG).show();
                           onCancelEditConfirmed();
                       }
                     }
                  }
                }
            };
            /**
             * @}
             */

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        mContext = activity;
        mEditorUtils = ContactEditorUtils.getInstance(mContext);
        mComparator = new RawContactDeltaComparator(mContext);
    }

    @Override
    public void onCreate(Bundle savedState) {
        if (savedState != null) {
            // Restore mUri before calling super.onCreate so that onInitializeLoaders
            // would already have a uri and an action to work with
            mAction = savedState.getString(KEY_ACTION);
            mLookupUri = savedState.getParcelable(KEY_URI);
        }
        super.onCreate(savedState);

        /**
         * SPRD:Bug612672 The editor is closed when rotate the screen
         * @{
         */
        sameDataChanged = false;
        if(mContext != null){
           mContext.getContentResolver().registerContentObserver(RawContacts.CONTENT_URI, true, mObserver);
        }
        /**
         * @}
         */
        /**
         * SPRD:Bug494562 When clear the info of contact and then delete it, don't save it again.
         *
         * @{
         */
        ContactDeletionInteraction.doDeleteContact = false;
        /**
         * @}
         */
        /**
         * SPRD:Bug610452 remember selected station and restore the
         * station when split-screen
         * @{
         */
        if(ConfirmReplaceListDialogFragment.mSelectDataId != null){
           this.dataItem = ConfirmReplaceListDialogFragment.mSelectDataItem;
        }
        /**
         * @}
         */
        if (savedState == null) {
            mViewIdGenerator = new ViewIdGenerator();
        } else {
            mViewIdGenerator = savedState.getParcelable(KEY_VIEW_ID_GENERATOR);

            mAutoAddToDefaultGroup = savedState.getBoolean(KEY_AUTO_ADD_TO_DEFAULT_GROUP);
            mDisableDeleteMenuOption = savedState.getBoolean(KEY_DISABLE_DELETE_MENU_OPTION);
            mNewLocalProfile = savedState.getBoolean(KEY_NEW_LOCAL_PROFILE);
            mMaterialPalette = savedState.getParcelable(KEY_MATERIAL_PALETTE);
            mPhotoId = savedState.getLong(KEY_PHOTO_ID);

            mRawContacts = ImmutableList.copyOf(savedState.<RawContact>getParcelableArrayList(
                    KEY_RAW_CONTACTS));
            // NOTE: mGroupMetaData is not saved/restored

            // Read state from savedState. No loading involved here
            mState = savedState.<RawContactDeltaList> getParcelable(KEY_EDIT_STATE);
            mStatus = savedState.getInt(KEY_STATUS);
            mRawContactDisplayAloneIsReadOnly = savedState.getBoolean(
                    KEY_RAW_CONTACT_DISPLAY_ALONE_IS_READ_ONLY);

            mHasNewContact = savedState.getBoolean(KEY_HAS_NEW_CONTACT);
            mNewContactDataReady = savedState.getBoolean(KEY_NEW_CONTACT_READY);

            mIsEdit = savedState.getBoolean(KEY_IS_EDIT);
            mExistingContactDataReady = savedState.getBoolean(KEY_EXISTING_CONTACT_READY);

            mIsUserProfile = savedState.getBoolean(KEY_IS_USER_PROFILE);

            // Phone specific options menus
            mSendToVoicemailState = savedState.getBoolean(KEY_SEND_TO_VOICE_MAIL_STATE);
            mArePhoneOptionsChangable = savedState.getBoolean(KEY_ARE_PHONE_OPTIONS_CHANGEABLE);
            mCustomRingtone = savedState.getString(KEY_CUSTOM_RINGTONE);

            mEnabled = savedState.getBoolean(KEY_ENABLED);

            // Aggregation PopupWindow
            mAggregationSuggestionsRawContactId = savedState.getLong(
                    KEY_AGGREGATION_SUGGESTIONS_RAW_CONTACT_ID);

            // Join Activity
            mContactIdForJoin = savedState.getLong(KEY_CONTACT_ID_FOR_JOIN);

            mReadOnlyDisplayName = savedState.getString(KEY_READ_ONLY_DISPLAY_NAME);
        }

        // mState can still be null because it may not have have finished loading before
        // onSaveInstanceState was called.
        if (mState == null) {
            mState = new RawContactDeltaList();
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        validateAction(mAction);

        /**
         * SPRD:Bug597956 The DUT shows wrong contact account list after retating the screen.
         * @{
         */
        if (savedInstanceState != null) {
            mAccountWithDataSet = savedInstanceState.getParcelable(KEY_ACCOUNT_WITH_DATASET);
        }
        /*
         * @}
         */

        if (mState.isEmpty()) {
            // The delta list may not have finished loading before orientation change happens.
            // In this case, there will be a saved state but deltas will be missing.  Reload from
            // database.
            if (Intent.ACTION_EDIT.equals(mAction) ||
                    ContactEditorBaseActivity.ACTION_EDIT.equals(mAction)) {
                // Either...
                // 1) orientation change but load never finished.
                // 2) not an orientation change so data needs to be loaded for first time.
                getLoaderManager().initLoader(LOADER_CONTACT, null, mContactLoaderListener);
                getLoaderManager().initLoader(LOADER_GROUPS, null, mGroupsLoaderListener);
            }
        } else {
            // Orientation change, we already have mState, it was loaded by onCreate
            /**
             * SPRD: bug260683
             * Original Android code:
            bindEditors();
             * @{
             */
            if (!mReplaceDialogShowing) {
                bindEditors();
            }
            /**
             * @}
             */
        }
        // Handle initial actions only when existing state missing
        if (savedInstanceState == null) {
            if (Intent.ACTION_EDIT.equals(mAction) ||
                    ContactEditorBaseActivity.ACTION_EDIT.equals(mAction)) {
                mIsEdit = true;
            /**
             * SPRD:No need select account when from compact edit fragment.
             * Orig:
             *
            } else if (Intent.ACTION_INSERT.equals(mAction) ||
                    ContactEditorBaseActivity.ACTION_INSERT.equals(mAction)) {
             * @{
             */
            } else if (Intent.ACTION_INSERT.equals(mAction)){
            /**
             * @}
             */
                mHasNewContact = true;
                /**
                 * SPRD:Bug495782 Use the wrong type to forced type conversion.
                 * Original Android code:
                final Account account = mIntentExtras == null ? null :
                        (Account) mIntentExtras.getParcelable(Intents.Insert.EXTRA_ACCOUNT);
                 * @{
                 */
                final AccountWithDataSet account = mIntentExtras == null ? null :
                    (AccountWithDataSet) mIntentExtras.getParcelable(Intents.Insert.EXTRA_ACCOUNT);
                /**
                 * @}
                 */
                final String dataSet = mIntentExtras == null ? null :
                        mIntentExtras.getString(Intents.Insert.EXTRA_DATA_SET);
                if (account != null) {
                    // Account specified in Intent
                    createContact(new AccountWithDataSet(account.name, account.type, dataSet));
                } else {
                    // No Account specified. Let the user choose
                    // Load Accounts async so that we can present them
                    selectAccountAndCreateContact();
                }
            /**
             * SPRD:No need select account when from compact edit fragment.
             * @{
             */
            } else if (ContactEditorBaseActivity.ACTION_INSERT.equals(mAction)) {
                String accountName=mIntentExtras.getString("accountName");
                String accountType=mIntentExtras.getString("accountType");
                final String dataSet = mIntentExtras == null ? null :
                    mIntentExtras.getString(Intents.Insert.EXTRA_DATA_SET);
                createContact(new AccountWithDataSet(accountName,accountType,dataSet));
            }
            /**
             * @}
             */
        }
    }

    /**
     * Checks if the requested action is valid.
     *
     * @param action The action to test.
     * @throws IllegalArgumentException when the action is invalid.
     */
    private void validateAction(String action) {
        if (VALID_INTENT_ACTIONS.contains(action)) {
            return;
        }
        /**
         * SPRD:Bug492887 Remove exception throw,finish this activity.
         * Original code:
        throw new IllegalArgumentException(
                "Unknown action " + action + "; Supported actions: " + VALID_INTENT_ACTIONS);
        * @{
        */
        Log.d(TAG, "Unknown action " + action + "; Supported actions: " + VALID_INTENT_ACTIONS);
        this.getActivity().finish();
        /**
         * @}
         */
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        outState.putString(KEY_ACTION, mAction);
        outState.putParcelable(KEY_URI, mLookupUri);
        outState.putBoolean(KEY_AUTO_ADD_TO_DEFAULT_GROUP, mAutoAddToDefaultGroup);
        outState.putBoolean(KEY_DISABLE_DELETE_MENU_OPTION, mDisableDeleteMenuOption);
        outState.putBoolean(KEY_NEW_LOCAL_PROFILE, mNewLocalProfile);
        if (mMaterialPalette != null) {
            outState.putParcelable(KEY_MATERIAL_PALETTE, mMaterialPalette);
        }
        outState.putLong(KEY_PHOTO_ID, mPhotoId);

        outState.putParcelable(KEY_VIEW_ID_GENERATOR, mViewIdGenerator);

        outState.putParcelableArrayList(KEY_RAW_CONTACTS, mRawContacts == null ?
                Lists.<RawContact>newArrayList() : Lists.newArrayList(mRawContacts));
        // NOTE: mGroupMetaData is not saved

        if (hasValidState()) {
            // Store entities with modifications
            outState.putParcelable(KEY_EDIT_STATE, mState);
        }
        outState.putInt(KEY_STATUS, mStatus);
        outState.putBoolean(KEY_HAS_NEW_CONTACT, mHasNewContact);
        outState.putBoolean(KEY_NEW_CONTACT_READY, mNewContactDataReady);
        outState.putBoolean(KEY_IS_EDIT, mIsEdit);
        outState.putBoolean(KEY_EXISTING_CONTACT_READY, mExistingContactDataReady);
        outState.putBoolean(KEY_RAW_CONTACT_DISPLAY_ALONE_IS_READ_ONLY,
                mRawContactDisplayAloneIsReadOnly);

        outState.putBoolean(KEY_IS_USER_PROFILE, mIsUserProfile);

        // Phone specific options
        outState.putBoolean(KEY_SEND_TO_VOICE_MAIL_STATE, mSendToVoicemailState);
        outState.putBoolean(KEY_ARE_PHONE_OPTIONS_CHANGEABLE, mArePhoneOptionsChangable);
        outState.putString(KEY_CUSTOM_RINGTONE, mCustomRingtone);

        outState.putBoolean(KEY_ENABLED, mEnabled);

        // Aggregation PopupWindow
        outState.putLong(KEY_AGGREGATION_SUGGESTIONS_RAW_CONTACT_ID,
                mAggregationSuggestionsRawContactId);

        // Join Activity
        outState.putLong(KEY_CONTACT_ID_FOR_JOIN, mContactIdForJoin);

        outState.putString(KEY_READ_ONLY_DISPLAY_NAME, mReadOnlyDisplayName);
        /**
         * SPRD:
         *   fix bug426810 Add a number replace dialog when add a number to an existing contact in sms screen.
         * @{
         */
         outState.putBundle(KEY_INTENT_EXTRAS, mIntentExtras);
         if (mExistedDataIdList != null && mExistedDataList != null) {
             outState.putStringArrayList(KEY_DATA_LIST, mExistedDataList);
             outState.putStringArrayList(KEY_DATA_ID_LIST, mExistedDataIdList);
         }
         /**
          * @}
          */
        /**
         * SPRD: bug260683
         *
         * @{
         */
        outState.putBoolean(KEY_DIALOG_SHOWING, mReplaceDialogShowing);
        /**
         * @}
         */
        /**
         * SPRD:Bug597956 The DUT shows wrong contact account list after retating the screen.
         * @{
         */
        outState.putParcelable(KEY_ACCOUNT_WITH_DATASET, mAccountWithDataSet);
        /*
         * @}
         */
        super.onSaveInstanceState(outState);
    }

    @Override
    public void onStop() {
        super.onStop();

        UiClosables.closeQuietly(mAggregationSuggestionPopup);
        /**
         * SPRD:Bug498039 Dismiss the custom label dialog, when lock the screen.
         * @{
         */
        if(mContactEditor != null) {
            LabeledEditorView editorView = null;
            if(mContactEditor instanceof RawContactEditorView) {
                RawContactEditorView editor = (RawContactEditorView)mContactEditor;
                editorView = editor.getNameEditor();
            } else if (mContactEditor instanceof CompactRawContactsEditorView) {
                CompactRawContactsEditorView editor =
                                                 (CompactRawContactsEditorView)mContactEditor;
                /**
                 * SPRD: Bug597877 edit one linked contact, into the multi screen mode, the list popup window
                 * shows abnormal
                 * @{
                 */
                UiClosables.closeQuietly(editor.mRawContactAccountPopup);
                editorView = editor.getPrimaryNameEditorView();
                /**
                 * @}
                 */
            }
            if(editorView != null) {
                editorView.dissmissDialog();
            }
        }
        /**
         * @}
         */
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mAggregationSuggestionEngine != null) {
            mAggregationSuggestionEngine.quit();
        }
        /**
         * SPRD:Bug600523 The DUT can not switch to sim account after editing a sim contact.
         * @{
         */
        mRawContacts = null;
        /*
         * @}
         */
        /**
         * SPRD:Bug610452 remember selected station and restore the
         * station when split-screen
         * @{
         */
        this.dataItem = -1;
        /**
         * @}
         */
        /**
         * SPRD:Bug612672 The editor is closed when rotate the screen
         * @{
         */
        if (mObserver != null) {
           mContext.getContentResolver().unregisterContentObserver(mObserver);
        }
        /**
         * @}
         */
        /**
         * SPRD:Bug617126 Conflict prompt have occured when Edit and save
         * contacts twice,and press home
         * @{
         */
        SharedPreferences pref = PreferenceManager
                .getDefaultSharedPreferences(getActivity());
        if(pref != null){
            Editor editor = pref.edit();
            if(editor != null){
                editor.putString(KEY_URI_CURRENT, null);
                editor.commit();
            }
        }
        /**
         * @}
         */
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case REQUEST_CODE_JOIN: {
                // Ignore failed requests
                if (resultCode != Activity.RESULT_OK) return;
                if (data != null) {
                    final long contactId = ContentUris.parseId(data.getData());
                    int contactJoinNum = getSelectedContactJoinNum(contactId);
                    if (contactJoinNum > MAX_JOIN_CONTACTS_NUMBER) {
                        Toast.makeText(mContext, getString(R.string.over_max_join_number, contactJoinNum), Toast.LENGTH_LONG).show();
                        break;
                    }
                    if (hasPendingChanges()) {
                        // Ask the user if they want to save changes before doing the join
                        JoinContactConfirmationDialogFragment.show(this, contactId);
                    } else {
                        // Do the join immediately
                        joinAggregate(contactId);
                    }
                }
                mJoinContacts = false;
                break;
            }
            case REQUEST_CODE_ACCOUNTS_CHANGED: {
                // Bail if the account selector was not successful.
                if (resultCode != Activity.RESULT_OK) {
                    if (mListener != null) {
                        mListener.onReverted();
                    }
                    return;
                }
                // If there's an account specified, use it.
                if (data != null) {
                    AccountWithDataSet account = data.getParcelableExtra(
                            Intents.Insert.EXTRA_ACCOUNT);
                    if (account != null) {
                        createContact(account);
                        return;
                    }
                }
                // If there isn't an account specified, then this is likely a phone-local
                // contact, so we should continue setting up the editor by automatically selecting
                // the most appropriate account.
                /**
                 * SPRD: Bug542592 The DUT shows wrong account information after adding a new
                 * account and creating a contact of the newly added account.
                 * Android original code:
                createContact();
                 * @{
                 *
                 */
                if (data == null) {
                    Intent intent = new Intent(mContext, ContactEditorAccountsChangedActivity.class);
                    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                    mStatus = Status.SUB_ACTIVITY;
                    startActivityForResult(intent, REQUEST_CODE_ACCOUNTS_CHANGED);
                }
                /*
                 * @}
                 */
                break;
            }
            case REQUEST_CODE_PICK_RINGTONE: {
                if (data != null) {
                    final Uri pickedUri = data.getParcelableExtra(
                            RingtoneManager.EXTRA_RINGTONE_PICKED_URI);
                    onRingtonePicked(pickedUri);
                }
                break;
            }
        }
    }

    //SPRD: add for bug615040, limit the join contacts to 10.
    private int getSelectedContactJoinNum(long contactId) {
        int joinNum = 0;
        ContentResolver resolver = mContext.getContentResolver();
        Cursor cursor = resolver.query(
                ContactsContract.RawContacts.CONTENT_URI,
                new String[] {ContactsContract.RawContacts._ID, ContactsContract.RawContacts.CONTACT_ID},
                "contact_id is " + contactId, null, null);
        if (cursor != null && mState != null) {
            joinNum = cursor.getCount() + mState.size();
        } else if (cursor == null && mState != null) {
            joinNum = mState.size();
        } else if (cursor != null && mState == null) {
            joinNum = cursor.getCount();
        }
        if(cursor != null){
            cursor.close();
        }
        Log.d(TAG, "getSelectedContactJoinNum, joinNum = " + joinNum);
        return joinNum;
    }

    private void onRingtonePicked(Uri pickedUri) {
        mCustomRingtone = EditorUiUtils.getRingtoneStringFromUri(pickedUri, CURRENT_API_VERSION);
        Intent intent = ContactSaveService.createSetRingtone(
                mContext, mLookupUri, mCustomRingtone);
        mContext.startService(intent);
    }

    //
    // Options menu
    //

    private void setStateForPhoneMenuItems(Contact contact) {
        if (contact != null) {
            mSendToVoicemailState = contact.isSendToVoicemail();
            mCustomRingtone = contact.getCustomRingtone();
            mArePhoneOptionsChangable = !contact.isDirectoryEntry()
                    && PhoneCapabilityTester.isPhone(mContext);
        }
    }

    /**
     * Invalidates the options menu if we are still associated with an Activity.
     */
    protected void invalidateOptionsMenu() {
        final Activity activity = getActivity();
        if (activity != null) {
            activity.invalidateOptionsMenu();
        }
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, final MenuInflater inflater) {
        inflater.inflate(R.menu.edit_contact, menu);
    }

    @Override
    public void onPrepareOptionsMenu(Menu menu) {
        // This supports the keyboard shortcut to save changes to a contact but shouldn't be visible
        // because the custom action bar contains the "save" button now (not the overflow menu).
        // TODO: Find a better way to handle shortcuts, i.e. onKeyDown()?
        final MenuItem saveMenu = menu.findItem(R.id.menu_save);
        final MenuItem splitMenu = menu.findItem(R.id.menu_split);
        final MenuItem joinMenu = menu.findItem(R.id.menu_join);
        final MenuItem helpMenu = menu.findItem(R.id.menu_help);
        final MenuItem sendToVoiceMailMenu = menu.findItem(R.id.menu_send_to_voicemail);
        final MenuItem ringToneMenu = menu.findItem(R.id.menu_set_ringtone);
        final MenuItem deleteMenu = menu.findItem(R.id.menu_delete);

        // Set visibility of menus

        // help menu depending on whether this is inserting or editing
        if (isInsert(mAction) || mRawContactIdToDisplayAlone != -1) {
            HelpUtils.prepareHelpMenuItem(mContext, helpMenu, R.string.help_url_people_add);
            splitMenu.setVisible(false);
            joinMenu.setVisible(false);
            deleteMenu.setVisible(false);
        } else if (isEdit(mAction)) {
            HelpUtils.prepareHelpMenuItem(mContext, helpMenu, R.string.help_url_people_edit);
            splitMenu.setVisible(canUnlinkRawContacts());
            // Cannot join a user profile
            joinMenu.setVisible(!isEditingUserProfile());
            /**
             * SPRD: Bug594218 Can't add photo image and delete contact in UserProfile
             * @{
             */
//            deleteMenu.setVisible(!mDisableDeleteMenuOption && !isEditingUserProfile());
            deleteMenu.setVisible(!mDisableDeleteMenuOption);
            /*
             * @}
             */
        } else {
            // something else, so don't show the help menu
            helpMenu.setVisible(false);
        }
		// Save menu is invisible when there's only one read only contact in the editor.
        saveMenu.setVisible(!mRawContactDisplayAloneIsReadOnly);
        /**
         * SPRD: sim contact should not set ringtone and voicemail
         * @{
         */
        if (isSimType() || mRawContactIdToDisplayAlone != -1 || mIsUserProfile) {
            sendToVoiceMailMenu.setVisible(false);
            ringToneMenu.setVisible(false);
            joinMenu.setVisible(false);
        } else {
            // Hide telephony-related settings (ringtone, send to voicemail)
            // if we don't have a telephone or are editing a new contact.
            sendToVoiceMailMenu.setChecked(mSendToVoicemailState);
            sendToVoiceMailMenu.setVisible(mArePhoneOptionsChangable);
            ringToneMenu.setVisible(mArePhoneOptionsChangable);
        }
        /**
         * @}
         */
        int size = menu.size();
        for (int i = 0; i < size; i++) {
            menu.getItem(i).setEnabled(mEnabled);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        final Activity activity = getActivity();
        if (activity == null || activity.isFinishing() || activity.isDestroyed()) {
            // If we no longer are attached to a running activity want to
            // drain this event.
            return true;
        }

        switch (item.getItemId()) {
            case R.id.menu_save:
            /**
             * SPRD:Bug520249 add toast while can't save contact during joining
             *
             * @{
             */
            if (JoinContactsDialogFragment.isContactsJoining) {
                Toast.makeText(mContext,
                        R.string.toast_batchoperation_is_running,
                        Toast.LENGTH_LONG).show();
            }
            /**
             * @}
             */
            /**
             * SPRD:Bug612672 The editor is closed when rotate the screen
             * @{
             */
                if (save(SaveMode.CLOSE)) {
                    isSave = true;
                    SharedPreferences pref = PreferenceManager
                      .getDefaultSharedPreferences(getActivity());
                    Editor editor = pref.edit();
                    if(mLookupUri!=null){
                      editor.putString(KEY_URI_CURRENT, mLookupUri.toString());
                      editor.commit();
                    }
                   return true;
                }else{
                   return false;
                }
                /**
                 * @}
                 */
            case R.id.menu_delete:
                if (mListener != null) mListener.onDeleteRequested(mLookupUri);
                return true;
            case R.id.menu_split:
                return doSplitContactAction();
            case R.id.menu_join:
                //SPRD: add for bug615040, limit the join contacts to 10.
                if (mState != null && mState.size() >= MAX_JOIN_CONTACTS_NUMBER) {
                    Toast.makeText(mContext, getString(R.string.over_max_join_number, mState.size()), Toast.LENGTH_LONG).show();
                    return false;
                } else {
                    return doJoinContactAction();
                }
            case R.id.menu_set_ringtone:
                doPickRingtone();
                return true;
            case R.id.menu_send_to_voicemail:
                // Update state and save
                mSendToVoicemailState = !mSendToVoicemailState;
                item.setChecked(mSendToVoicemailState);
                final Intent intent = ContactSaveService.createSetSendToVoicemail(
                        mContext, mLookupUri, mSendToVoicemailState);
                mContext.startService(intent);
                return true;
        }

        return false;
    }

    @Override
    public boolean revert() {
        if (mState.isEmpty() || !hasPendingChanges()) {
            onCancelEditConfirmed();
        } else {
            CancelEditDialogFragment.show(this);
        }
        return true;
    }

    @Override
    public void onCancelEditConfirmed() {
        // When this Fragment is closed we don't want it to auto-save
        mStatus = Status.CLOSING;
        if (mListener != null) {
            mListener.onReverted();
        }
    }

    @Override
    public void onSplitContactConfirmed(boolean hasPendingChanges) {
        if (mState.isEmpty()) {
            // This may happen when this Fragment is recreated by the system during users
            // confirming the split action (and thus this method is called just before onCreate()),
            // for example.
            Log.e(TAG, "mState became null during the user's confirming split action. " +
                    "Cannot perform the save action.");
            return;
        }

        if (!hasPendingChanges && mHasNewContact) {
            // If the user didn't add anything new, we don't want to split out the newly created
            // raw contact into a name-only contact so remove them.
            final Iterator<RawContactDelta> iterator = mState.iterator();
            while (iterator.hasNext()) {
                final RawContactDelta rawContactDelta = iterator.next();
                if (rawContactDelta.getRawContactId() < 0) {
                    iterator.remove();
                }
            }
        }
        mState.markRawContactsForSplitting();
        save(SaveMode.SPLIT);
    }

    private boolean doSplitContactAction() {
        if (!hasValidState()) return false;

        SplitContactConfirmationDialogFragment.show(this, hasPendingChanges());
        return true;
    }

    private boolean doJoinContactAction() {
        if (!hasValidState() || mLookupUri == null) {
            return false;
        }

        // If we just started creating a new contact and haven't added any data, it's too
        // early to do a join
        if (mState.size() == 1 && mState.get(0).isContactInsert()
                && !hasPendingChanges()) {
            Toast.makeText(mContext, R.string.toast_join_with_empty_contact,
                    Toast.LENGTH_LONG).show();
            return true;
        }

        showJoinAggregateActivity(mLookupUri);
        return true;
    }

    @Override
    public void onJoinContactConfirmed(long joinContactId) {
        doSaveAction(SaveMode.JOIN, joinContactId);
    }

    private void doPickRingtone() {
        final Intent intent = new Intent(RingtoneManager.ACTION_RINGTONE_PICKER);
        // Allow user to pick 'Default'
        intent.putExtra(RingtoneManager.EXTRA_RINGTONE_SHOW_DEFAULT, true);
        // Show only ringtones
        intent.putExtra(RingtoneManager.EXTRA_RINGTONE_TYPE, RingtoneManager.TYPE_RINGTONE);
        // Allow the user to pick a silent ringtone
        intent.putExtra(RingtoneManager.EXTRA_RINGTONE_SHOW_SILENT, true);

        final Uri ringtoneUri = EditorUiUtils.getRingtoneUriFromString(mCustomRingtone,
                CURRENT_API_VERSION);

        // Put checkmark next to the current ringtone for this contact
        intent.putExtra(RingtoneManager.EXTRA_RINGTONE_EXISTING_URI, ringtoneUri);

        // Launch!
        try {
            startActivityForResult(intent, REQUEST_CODE_PICK_RINGTONE);
        } catch (ActivityNotFoundException ex) {
            Toast.makeText(mContext, R.string.missing_app, Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    public boolean save(int saveMode) {
        if (!hasValidState() || mStatus != Status.EDITING) {
            return false;
        }

        // If we are about to close the editor - there is no need to refresh the data
        if (saveMode == SaveMode.CLOSE || saveMode == SaveMode.COMPACT
                || saveMode == SaveMode.SPLIT) {
            getLoaderManager().destroyLoader(LOADER_CONTACT);
        }

        mStatus = Status.SAVING;

        if (!hasPendingChanges()) {
            if (mLookupUri == null && saveMode == SaveMode.RELOAD) {
                // We don't have anything to save and there isn't even an existing contact yet.
                // Nothing to do, simply go back to editing mode
                mStatus = Status.EDITING;
                return true;
            }
            onSaveCompleted(/* hadChanges =*/ false, saveMode,
                    /* saveSucceeded =*/ mLookupUri != null, mLookupUri, /* joinContactId =*/ null);
            return true;
        }
        /**
         * SPRD: bug398899, check whether the saved data complies with account restriction
         * Original Android code:
        setEnabled(false);
         * @{
         */
        String mimeType = AccountRestrictionUtils.get(mContext).violateFieldLengthRestriction(
                mState);

        if (mimeType != null) {
            int resId = AccountRestrictionUtils.get(mContext).mimeToRes(mimeType);
            String text = getString(R.string.field_too_long, getString(resId));
            mIsSaveFailure = true;
            Toast.makeText(mContext, text, Toast.LENGTH_LONG).show();
            mStatus = Status.EDITING;
            return true;
        }
        boolean isEmailAddress = AccountRestrictionUtils.get(mContext)
                .violateEmailFormatRestriction(mState);
        if (!isEmailAddress) {
            int toastId = R.string.email_format_fail;
            Toast.makeText(mContext, toastId, Toast.LENGTH_LONG).show();
            mStatus = Status.EDITING;
            mIsSaveFailure = true;
            return true;
        }
        /**
         * @}
         */

        /**
         * SPRD: Bug457662 Cannot switch the phone type spinner when add contacts from dialer.
         * @{
         */
        if (isSimType()) {
            // Bug 539814 If there's only a fixed number,the contacts can't save
            int toastId = AccountRestrictionUtils.get(mContext)
                    .violatePhoneNumberType(mState);
            if (toastId != -1) {
                Toast.makeText(mContext, toastId, Toast.LENGTH_LONG).show();
                /**
                 * SPRD: Bug482536 @
                 */
                if (saveMode == SaveMode.RELOAD) {
                    getLoaderManager().destroyLoader(LOADER_CONTACT);
                    if (mListener != null)
                        mListener.onSaveFinished(null);
                    mStatus = Status.CLOSING;
                }
                /**
                 * @
                 */
                /**
                 * SPRD:Bug494116 After being prevented to save two same type number to sim,
                 * you can't save again.
                 *
                 * @{
                 */
                mStatus = Status.EDITING;
                /**
                 * @}
                 */
                mIsSaveFailure = true;
                return true;
            }
        }
        /**
         * @}
         */
        return doSaveAction(saveMode, /* joinContactId */ null);
    }

    /**
     * Persist the accumulated editor deltas.
     *
     * @param joinContactId the raw contact ID to join the contact being saved to after the save,
     *         may be null.
     */
    abstract protected boolean doSaveAction(int saveMode, Long joinContactId);

    protected boolean startSaveService(Context context, Intent intent, int saveMode) {
        final boolean result = ContactSaveService.startService(
                context, intent, saveMode);
        if (!result) {
            onCancelEditConfirmed();
        }
        return result;
    }

    //
    // State accessor methods
    //

    /**
     * Check if our internal {@link #mState} is valid, usually checked before
     * performing user actions.
     */
    protected boolean hasValidState() {
        return mState.size() > 0;
    }

    protected boolean isEditingUserProfile() {
        /**
         * SPRD: Make sure my phone info account display when create local rawContact.
         * Orig:
         *
        return mNewLocalProfile || mIsUserProfile;
         * @{
         */
        Boolean isUserProfile = false;
        if (mIntentExtras != null) {
            isUserProfile = mIntentExtras.getBoolean("isUserProfile");
        }
        return mNewLocalProfile || mIsUserProfile || isUserProfile;
        /**
         * @}
         */
    }

    /**
     * Whether the contact being edited spans multiple raw contacts.
     * The may also span multiple accounts.
     */
    public boolean isEditingMultipleRawContacts() {
        return mState.size() > 1;
    }

    /**
     * Whether the contact being edited is composed of a single read-only raw contact
     * aggregated with a newly created writable raw contact.
     */
    protected boolean isEditingReadOnlyRawContactWithNewContact() {
        return mHasNewContact && mState.size() == 2;
    }

    /**
     * Return true if there are any edits to the current contact which need to
     * be saved.
     */
    protected boolean hasPendingRawContactChanges(Set<String> excludedMimeTypes) {
        final AccountTypeManager accountTypes = AccountTypeManager.getInstance(mContext);
        return RawContactModifier.hasChanges(mState, accountTypes, excludedMimeTypes);
    }

    /**
     * We allow unlinking only if there is more than one raw contact, it is not a user-profile,
     * and unlinking won't result in an empty contact.  For the empty contact case, we only guard
     * against this when there is a single read-only contact in the aggregate.  If the user
     * has joined >1 read-only contacts together, we allow them to unlink it, even if they have
     * never added their own information and unlinking will create a name only contact.
     */
    protected boolean canUnlinkRawContacts() {
        return isEditingMultipleRawContacts()
                && !isEditingUserProfile()
                && !isEditingReadOnlyRawContactWithNewContact();
    }

    /**
     * Determines if changes were made in the editor that need to be saved, while taking into
     * account that name changes are not real for read-only contacts.
     * See go/editing-read-only-contacts
     */
    protected boolean hasPendingChanges() {
        if (mReadOnlyNameEditorView != null && mReadOnlyDisplayName != null) {
            // We created a new raw contact delta with a default display name.
            // We must test for pending changes while ignoring the default display name.
            final String displayName = mReadOnlyNameEditorView.getDisplayName();
            if (mReadOnlyDisplayName.equals(displayName)) {
                final Set<String> excludedMimeTypes = new HashSet<>();
                excludedMimeTypes.add(StructuredName.CONTENT_ITEM_TYPE);
                return hasPendingRawContactChanges(excludedMimeTypes);
            }
            return true;
        }
        return hasPendingRawContactChanges(/* excludedMimeTypes =*/ null);
    }

    /**
     * Whether editor inputs and the options menu should be enabled.
     */
    protected boolean isEnabled() {
        return mEnabled;
    }

    /**
     * Returns the palette extra that was passed in.
     */
    protected MaterialColorMapUtils.MaterialPalette getMaterialPalette() {
        return mMaterialPalette;
    }

    //
    // Account creation
    //

    private void selectAccountAndCreateContact() {
        // If this is a local profile, then skip the logic about showing the accounts changed
        // activity and create a phone-local contact.
        if (mNewLocalProfile) {
            /**
             * SPRD: set account as sprd Phone, bug421665
             * Original Android code:
            createContact(null);
             * @{
             */
            createContact(AccountTypeManager.getInstance(mContext).getPhoneAccount());
            /**
             * @}
             */
            return;
        }

        /**
         * SPRD: Bug 595155 androidN adds the default account setting, the newly created contact
         * should be saved in default account.
         */
        // If there is no default account or the accounts have changed such that we need to
        // prompt the user again, then launch the account prompt.
        if (mEditorUtils.shouldShowAccountChangedNotification()) {
            Intent intent = new Intent(mContext, ContactEditorAccountsChangedActivity.class);
            // Prevent a second instance from being started on rotates
            intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            mStatus = Status.SUB_ACTIVITY;
            startActivityForResult(intent, REQUEST_CODE_ACCOUNTS_CHANGED);
        } else {
            // Otherwise, there should be a default account. Then either create a local contact
            // (if default account is null) or create a contact with the specified account.
            AccountWithDataSet defaultAccount = mEditorUtils.getDefaultAccount();
            createContact(defaultAccount);
        }
    }

    /**
     * Create a contact by automatically selecting the first account. If there's no available
     * account, a device-local contact should be created.
     */
    protected void createContact() {
        /*
         * SPRD: bug391835, created new account back to editor fragment,default storeDir displayed sim1.
         *
         * Original Android code:
        final List<AccountWithDataSet> accounts =
                AccountTypeManager.getInstance(mContext).getAccounts(true);
         * @{
         */
        final List<AccountWithDataSet> accounts = AccountTypeManager.getInstance(mContext).getAccountsWithNoSim(true);
        // No Accounts available. Create a phone-local contact.
        if (accounts.isEmpty()) {
            createContact(null);
            return;
        }

        // We have an account switcher in "create-account" screen, so don't need to ask a user to
        // select an account here.
        createContact(accounts.get(0));
    }

    /**
     * Shows account creation screen associated with a given account.
     *
     * @param account may be null to signal a device-local contact should be created.
     */
    protected void createContact(AccountWithDataSet account) {
        final AccountTypeManager accountTypes = AccountTypeManager.getInstance(mContext);
        final AccountType accountType = accountTypes.getAccountTypeForAccount(account);

        if (accountType.getCreateContactActivityClassName() != null) {
            if (mListener != null) {
                mListener.onCustomCreateContactActivityRequested(account, mIntentExtras);
            }
        } else {
            setStateForNewContact(account, accountType, isEditingUserProfile());
        }
    }

    //
    // Data binding
    //

    private void setState(Contact contact) {
        // If we have already loaded data, we do not want to change it here to not confuse the user
        if (!mState.isEmpty()) {
            Log.v(TAG, "Ignoring background change. This will have to be rebased later");
            return;
        }

        // See if this edit operation needs to be redirected to a custom editor
        mRawContacts = contact.getRawContacts();
        sRawContactsforReplaceDialog = ImmutableList.copyOf(mRawContacts);
        if (mRawContacts.size() == 1) {
            RawContact rawContact = mRawContacts.get(0);
            String type = rawContact.getAccountTypeString();
            String dataSet = rawContact.getDataSet();
            AccountType accountType = rawContact.getAccountType(mContext);
            /**
             * SPRD: Bug 592762 androidn porting The DUT occurs crash while editing sim contact.
             * @{
             */
            if (mAccountWithDataSet == null) {
                mAccountWithDataSet = new AccountWithDataSet(rawContact.getAccountName(), type, dataSet);
            }
            /*
             * @}
             */
            if (accountType.getEditContactActivityClassName() != null &&
                    !accountType.areContactsWritable()) {
                if (mListener != null) {
                    String name = rawContact.getAccountName();
                    long rawContactId = rawContact.getId();
                    mListener.onCustomEditContactActivityRequested(
                            new AccountWithDataSet(name, type, dataSet),
                            ContentUris.withAppendedId(RawContacts.CONTENT_URI, rawContactId),
                            mIntentExtras, true);
                }
                return;
            }
        }

        String readOnlyDisplayName = null;
        // Check for writable raw contacts.  If there are none, then we need to create one so user
        // can edit.  For the user profile case, there is already an editable contact.
        if (!contact.isUserProfile() && !contact.isWritableContact(mContext)) {
            mHasNewContact = true;

            // This is potentially an asynchronous call and will add deltas to list.
            selectAccountAndCreateContact();

            readOnlyDisplayName = contact.getDisplayName();
        } else {
            mHasNewContact = false;
        }

        // This also adds deltas to list.  If readOnlyDisplayName is null at this point it is
        // simply ignored later on by the editor.
        /**
         * SPRD:Bug426810 Add a number replace dialog when add a number to an existing contact in sms screen;Bug449339
         * Original Android code:
        setStateForExistingContact(readOnlyDisplayName, contact.isUserProfile(), mRawContacts);
         * @{
         */
        int replaceDialogId = showReplaceDialog(contact);
        // For user profile, change the contacts query URI
        mIsUserProfile = contact.isUserProfile();
        if (replaceDialogId == ID_NO_CONFIRM_REPLACE_DIALOG) {
            if (mReplaceDialogShowing) {
                Message message = mHandler.obtainMessage();
                message.what = replaceDialogId;
                mHandler.sendMessage(message);
            }
            // This also adds deltas to list
            // If displayName is null at this point it is simply ignored later on by the editor.
            setStateForExistingContact(mReadOnlyDisplayName, mIsUserProfile, mRawContacts);
        } else if ((replaceDialogId == ID_CONFIRM_REPLACE_DIALOG || replaceDialogId == ID_CONFIRM_REPLACE_DIALOG_LIST)) {
            if (!mReplaceDialogShowing || mDataListChange) {
                Message message = mHandler.obtainMessage();
                message.what = replaceDialogId;
                mHandler.sendMessage(message);
            }
        }
         /**
          * @}
          */
    }

    /**
     * Prepare {@link #mState} for a newly created phone-local contact.
     */
    private void setStateForNewContact(AccountWithDataSet account, AccountType accountType,
            boolean isUserProfile) {
        setStateForNewContact(account, accountType, /* oldState =*/ null,
                /* oldAccountType =*/ null, isUserProfile);
    }

    /**
     * Prepare {@link #mState} for a newly created phone-local contact, migrating the state
     * specified by oldState and oldAccountType.
     */
    protected void setStateForNewContact(AccountWithDataSet account, AccountType accountType,
            RawContactDelta oldState, AccountType oldAccountType, boolean isUserProfile) {
        mStatus = Status.EDITING;
        mState.add(createNewRawContactDelta(account, accountType, oldState, oldAccountType));
        mIsUserProfile = isUserProfile;
        mNewContactDataReady = true;
        /*
         * SPRD: Bug 601540 The DUT fails to create user profile after clearing user data.
         * @{
         */
        mAccountWithDataSet = account;
        /*
         * @}
         */
        bindEditors();
    }

    /**
     * Returns a {@link RawContactDelta} for a new contact suitable for addition into
     * {@link #mState}.
     *
     * If oldState and oldAccountType are specified, the state specified by those parameters
     * is migrated to the result {@link RawContactDelta}.
     */
    private RawContactDelta createNewRawContactDelta(AccountWithDataSet account,
            AccountType accountType, RawContactDelta oldState, AccountType oldAccountType) {
        final RawContact rawContact = new RawContact();
        if (account != null) {
            rawContact.setAccount(account);
        } else {
            rawContact.setAccountToLocal();
        }

        final RawContactDelta result = new RawContactDelta(
                ValuesDelta.fromAfter(rawContact.getValues()));
        if (oldState == null) {
            // Parse any values from incoming intent
            RawContactModifier.parseExtras(mContext, accountType, result, mIntentExtras);
        } else {
            RawContactModifier.migrateStateForNewContact(
                    mContext, oldState, result, oldAccountType, accountType);
        }

        // Ensure we have some default fields (if the account type does not support a field,
        // ensureKind will not add it, so it is safe to add e.g. Event)
        RawContactModifier.ensureKindExists(result, accountType, Phone.CONTENT_ITEM_TYPE);
        RawContactModifier.ensureKindExists(result, accountType, Email.CONTENT_ITEM_TYPE);
        RawContactModifier.ensureKindExists(result, accountType, Organization.CONTENT_ITEM_TYPE);
        RawContactModifier.ensureKindExists(result, accountType, Event.CONTENT_ITEM_TYPE);
        RawContactModifier.ensureKindExists(result, accountType,
                StructuredPostal.CONTENT_ITEM_TYPE);

        // Set the correct URI for saving the contact as a profile
        if (mNewLocalProfile) {
            result.setProfileQueryUri();
        }

        return result;
    }

    /**
     * Prepare {@link #mState} for an existing contact.
     */
    protected void setStateForExistingContact(String readOnlyDisplayName, boolean isUserProfile,
            ImmutableList<RawContact> rawContacts) {
        setEnabled(true);
        /**
         * SPRD: bug260683
         *
         * @{
         */
        mReplaceDialogShowing = false;
        /**
         * @}
         */
        mReadOnlyDisplayName = readOnlyDisplayName;

        mState.addAll(rawContacts.iterator());
        setIntentExtras(mIntentExtras);
        mIntentExtras = null;

        // For user profile, change the contacts query URI
        mIsUserProfile = isUserProfile;
        boolean localProfileExists = false;

        if (mIsUserProfile) {
            for (RawContactDelta rawContactDelta : mState) {
                // For profile contacts, we need a different query URI
                rawContactDelta.setProfileQueryUri();
                // Try to find a local profile contact
                /**
                 * SPRD:Bug426794 Edit my profile info, remove my phone info item.
                 * Orig:
                 *
                if (rawContactDelta.getValues().getAsString(RawContacts.ACCOUNT_TYPE) == null) {
                 * @{
                 */
                if (PhoneAccountType.ACCOUNT_TYPE.equals(rawContactDelta.getValues().getAsString(RawContacts.ACCOUNT_TYPE))) {
                /**
                 * @}
                 */
                    localProfileExists = true;
                }
            }
            // Editor should always present a local profile for editing
            if (!localProfileExists) {
                mState.add(createLocalRawContactDelta());
            }
        }
        mExistingContactDataReady = true;
        bindEditors();
    }

    /**
     * Returns a {@link RawContactDelta} for a local contact suitable for addition into
     * {@link #mState}.
     */
    /**
     * SPRD:Bug426794 Edit my profile info, remove my phone info item.
     * Orig:
     *
    private static RawContactDelta createLocalRawContactDelta() {
     * @{
     */
    private RawContactDelta createLocalRawContactDelta() {
    /**
     * @}
     */
        final RawContact rawContact = new RawContact();
        /**
         * SPRD:Bug426794 Edit my profile info, remove my phone info item.
         *
         * Original Android code:
        rawContact.setAccountToLocal();
         * @{
         */
        rawContact.getValues().put(RawContacts.ACCOUNT_TYPE, AccountTypeManager.getInstance(mContext).getPhoneAccount().type);
        rawContact.getValues().put(RawContacts.ACCOUNT_NAME, AccountTypeManager.getInstance(mContext).getPhoneAccount().name);
        rawContact.getValues().putNull(RawContacts.DATA_SET);
        /**
         * @}
         */

        final RawContactDelta result = new RawContactDelta(
                ValuesDelta.fromAfter(rawContact.getValues()));
        result.setProfileQueryUri();

        return result;
    }

    /**
     * Sets group metadata on all bound editors.
     */
    abstract protected void setGroupMetaData();

    /**
     * Bind editors using {@link #mState} and other members initialized from the loaded (or new)
     * Contact.
     */
    abstract protected void bindEditors();

    /**
     * Set the enabled state of editors.
     */
    private void setEnabled(boolean enabled) {
        if (mEnabled != enabled) {
            mEnabled = enabled;

            // Enable/disable editors
            if (mContent != null) {
                int count = mContent.getChildCount();
                for (int i = 0; i < count; i++) {
                    mContent.getChildAt(i).setEnabled(enabled);
                }
            }

            // Enable/disable aggregation suggestion vies
            if (mAggregationSuggestionView != null) {
                LinearLayout itemList = (LinearLayout) mAggregationSuggestionView.findViewById(
                        R.id.aggregation_suggestions);
                int count = itemList.getChildCount();
                for (int i = 0; i < count; i++) {
                    itemList.getChildAt(i).setEnabled(enabled);
                }
            }

            // Maybe invalidate the options menu
            final Activity activity = getActivity();
            if (activity != null) activity.invalidateOptionsMenu();
        }
    }

    /**
     * Removes a current editor ({@link #mState}) and rebinds new editor for a new account.
     * Some of old data are reused with new restriction enforced by the new account.
     *
     * @param oldState Old data being edited.
     * @param oldAccount Old account associated with oldState.
     * @param newAccount New account to be used.
     */
    protected void rebindEditorsForNewContact(
            RawContactDelta oldState, AccountWithDataSet oldAccount,
            AccountWithDataSet newAccount) {
        AccountTypeManager accountTypes = AccountTypeManager.getInstance(mContext);
        AccountType oldAccountType = accountTypes.getAccountTypeForAccount(oldAccount);
        AccountType newAccountType = accountTypes.getAccountTypeForAccount(newAccount);

        if (newAccountType.getCreateContactActivityClassName() != null) {
            Log.w(TAG, "external activity called in rebind situation");
            if (mListener != null) {
                mListener.onCustomCreateContactActivityRequested(newAccount, mIntentExtras);
            }
        } else {
            mExistingContactDataReady = false;
            mNewContactDataReady = false;
            mState = new RawContactDeltaList();
            setStateForNewContact(newAccount, newAccountType, oldState, oldAccountType,
                    isEditingUserProfile());
            /**
             * SPRD: bug260683
             * Original Android code:
            if (mIsEdit) {
                setStateForExistingContact(mReadOnlyDisplayName, isEditingUserProfile(),
                        mRawContacts);
            }
             * @{
             */
            if (mIsEdit && !getReplaceDialogShowing()) {
                setStateForExistingContact(mReadOnlyDisplayName, isEditingUserProfile(),
                        mRawContacts);
            }
            /**
             * @}
             */

        }
    }

    //
    // ContactEditor
    //

    @Override
    public void setListener(Listener listener) {
        mListener = listener;
    }

    @Override
    public void load(String action, Uri lookupUri, Bundle intentExtras) {
        mAction = action;
        mLookupUri = lookupUri;
        mIntentExtras = intentExtras;
        /**
         * SPRD: Bug 535558 The DUT does not show the joined contact information
         * from the expanded editor screen.
         * @{
         */
        if (mAction != null && (mAction.equals(ContactEditorBaseActivity.ACTION_EDIT))) {
            getActivity().getIntent().setAction(ContactEditorBaseActivity.ACTION_EDIT);
            getActivity().getIntent().setData(mLookupUri);
        }
        /*
         * @}
         */
        /**
         * SPRD:
         *
         * @{
         */
        if(mIntentExtras != null && mIntentExtras.containsKey(ContactsContract.Intents.Insert.PHONE)){
            String website = null;
            if(mIntentExtras.getString(ContactsContract.Intents.Insert.PHONE) != null){
            website = mIntentExtras.getString(ContactsContract.Intents.Insert.PHONE);
            }
            if(!TextUtils.isEmpty(website) && website.contains("//")){
            mIntentExtras.remove(ContactsContract.Intents.Insert.PHONE);
            mIntentExtras.putString("website", website);
            }
        }
        /**
         * @}
         */
        if (mIntentExtras != null) {
            mAutoAddToDefaultGroup =
                    mIntentExtras.containsKey(INTENT_EXTRA_ADD_TO_DEFAULT_DIRECTORY);
            mNewLocalProfile =
                    mIntentExtras.getBoolean(INTENT_EXTRA_NEW_LOCAL_PROFILE);
            mDisableDeleteMenuOption =
                    mIntentExtras.getBoolean(INTENT_EXTRA_DISABLE_DELETE_MENU_OPTION);
            /**
             * SPRD: bug168982, remove the group settings in profile editor.
             *
             * @{
             */
            if (mLookupUri != null) {
                List<String> segments = mLookupUri.getPathSegments();
                mIsUserProfile = segments != null ? segments.contains(PROFILE_SEGMENT) : false;
            }
            /**
             * @}
             */
            if (mIntentExtras.containsKey(INTENT_EXTRA_MATERIAL_PALETTE_PRIMARY_COLOR)
                    && mIntentExtras.containsKey(INTENT_EXTRA_MATERIAL_PALETTE_SECONDARY_COLOR)) {
                mMaterialPalette = new MaterialColorMapUtils.MaterialPalette(
                        mIntentExtras.getInt(INTENT_EXTRA_MATERIAL_PALETTE_PRIMARY_COLOR),
                        mIntentExtras.getInt(INTENT_EXTRA_MATERIAL_PALETTE_SECONDARY_COLOR));
            }
            // If the user selected a different photo, don't restore the one from the Intent
            if (mPhotoId < 0) {
                mPhotoId = mIntentExtras.getLong(INTENT_EXTRA_PHOTO_ID);
            }
            mRawContactIdToDisplayAlone = mIntentExtras.getLong(
                    INTENT_EXTRA_RAW_CONTACT_ID_TO_DISPLAY_ALONE, -1);
            mRawContactDisplayAloneIsReadOnly = mIntentExtras.getBoolean(
                    INTENT_EXTRA_RAW_CONTACT_DISPLAY_ALONE_IS_READ_ONLY);
        }
    }

    @Override
    public void setIntentExtras(Bundle extras) {
        if (extras == null || extras.size() == 0) {
            return;
        }

        final AccountTypeManager accountTypes = AccountTypeManager.getInstance(mContext);
        for (RawContactDelta state : mState) {
            final AccountType type = state.getAccountType(accountTypes);
            /**
             * SPRD:fix bug126251 Add phone number to sim contact,can not replace old number.
             * Orig:
            if (type.areContactsWritable()) {
                // Apply extras to the first writable raw contact only
                RawContactModifier.parseExtras(mContext, type, state, extras);
                break;
             * @{
             */
            final String accountType = state.getAccountType();
            final String accountName = state.getAccountName();
            if (accountName == null && type.areContactsWritable()) {
                RawContactModifier.parseExtras(mContext, type, state, extras);
            } else {
                AccountWithDataSet account = new AccountWithDataSet(accountName, accountType, null);
                String mimeType = null;
                /*SPRD: Bug 548880 The DUT misses some information after replacing exit contact.*/
                String newData1 = null;
                String newData4 = null;
                if (type.areContactsWritable()) {
                    final ContentValues values = new ContentValues();
                    RawContactDelta insert = new RawContactDelta(ValuesDelta.fromAfter(values));
                    RawContactModifier.parseExtras(mContext, type, insert, extras);
                    ArrayList<ContentValues> contentValues = insert.getContentValues();
                    for (ContentValues insertValues : contentValues) {
                        String tmpMimeType = insertValues.getAsString(Data.MIMETYPE);
                        if (!CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE.equals(tmpMimeType)) {
                            mimeType = tmpMimeType;
                            /*SPRD: Bug 548880 The DUT misses some information after replacing exit contact.*/
                            newData1 = insertValues.getAsString(Data.DATA1);
                            newData4 = insertValues.getAsString(Data.DATA4);
                        } else {
                            mimeType = null;
                            /*SPRD: Bug 548880 The DUT misses some information after replacing exit contact.*/
                            newData1 = null;
                            newData4 = null;
                        }
                        if (mimeType != null) {
                            final ArrayList<ValuesDelta> entries = state.getMimeEntries(mimeType);
                            if (entries != null
                                    && !GroupMembership.CONTENT_ITEM_TYPE.equals(mimeType)) {
                                ArrayList<String> oldDataList = new ArrayList<String>();
                                for (ValuesDelta entry : entries) {
                                    String oldData = entry.getAsString(Data.DATA1);
                                    oldDataList.add(oldData);
                                }
                                int typeOverallMax = -1;
                                if (!SimAccountType.ACCOUNT_TYPE.equals(type.accountType)
                                        && !USimAccountType.ACCOUNT_TYPE.equals(type.accountType)) {
                                    typeOverallMax = AccountTypeManager
                                            .getTypeOverallMaxForAccount(type, mimeType);
                                } else {
                                    int max = AccountRestrictionUtils.get(mContext)
                                            .getTypeOverallMax(account, mimeType);
                                    if (max != 0) {
                                        typeOverallMax = max;
                                    }
                                }
                                if (oldDataList.size() == typeOverallMax) {
                                    if (typeOverallMax == 1) {
                                        for (ValuesDelta entry : entries) {
                                            /*SPRD: Bug 548880 The DUT misses some information after replacing exit contact.*/
                                            if (newData1!=null||newData4!=null){
                                                entry.put(Data.DATA1, newData1);
                                                entry.put(Data.DATA4, newData4);
                                            }
                                        }
                                    } else {
                                        Long replaceDataId = extras.getLong("replaceDataId", -1);
                                        if (replaceDataId != -1) {
                                            for (ValuesDelta entry : entries) {
                                                if (replaceDataId.equals(entry.getAsLong(Data._ID))) {
                                                    /*SPRD: Bug 548880 The DUT misses some information after replacing exit contact.*/
                                                    entry.put(Data.DATA1, newData1);
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    RawContactModifier.parseExtras(mContext, type, state, extras);
                                }
                            } else {
                                RawContactModifier.parseExtras(mContext, type, state, extras);
                            }
                        }
                    }
                    /**
                     * @}
                     */
                    break;
                }
            }
        }
    }

    @Override
    public void onJoinCompleted(Uri uri) {
        onSaveCompleted(false, SaveMode.RELOAD, uri != null, uri, /* joinContactId */ null);
    }

    @Override
    public void onSaveCompleted(boolean hadChanges, int saveMode, boolean saveSucceeded,
            Uri contactLookupUri, Long joinContactId) {
        /**
         *  SPRD: Add contact edit error toast.
         * @{
         */
        onSaveCompleted(hadChanges, saveMode, saveSucceeded, contactLookupUri, joinContactId, 0);
    }
    public void onSaveCompleted(boolean hadChanges, int saveMode, boolean saveSucceeded,
            Uri contactLookupUri, Long joinContactId, int errorToast) {
        Log.i(TAG, "onSaveCompleted(" + saveMode + ", " + contactLookupUri);
       /**
        * @}
        */
        if (hadChanges) {
            if (saveSucceeded) {
                switch (saveMode) {
                    case SaveMode.JOIN:
                        break;
                    case SaveMode.SPLIT:
                        Toast.makeText(mContext, R.string.contactUnlinkedToast, Toast.LENGTH_SHORT)
                                .show();
                        break;
                    default:
                        Toast.makeText(mContext, R.string.contactSavedToast, Toast.LENGTH_SHORT)
                                .show();
                }
            } else {
                /**
                 * Bug426619 Contacts crashed when create a new SIM contact when SIM card has full.
                 * Orig:
                 *
                Toast.makeText(mContext, R.string.contactSavedErrorToast, Toast.LENGTH_LONG).show();
                 * @{
                 */
                if (errorToast != 0) {
                    Toast.makeText(mContext, errorToast, Toast.LENGTH_LONG).show();
                } else {
                    //SPRD: add for bug621780, change toast if account is not exist
                    final AccountTypeManager accountTypeManager = AccountTypeManager.getInstance(mContext);
                    List<AccountWithDataSet> accounts = accountTypeManager.getAccounts(true);
                    TelephonyManager telManager = (TelephonyManager)mContext.getSystemService(Context.TELEPHONY_SERVICE);
                    if (accounts != null && !accounts.contains(mAccountWithDataSet)) {
                        Toast.makeText(mContext, R.string.group_insert_error, Toast.LENGTH_LONG).show();
                    } else if(isSimType() && (telManager.getSimState(0) != TelephonyManager.SIM_STATE_READY||
                                 telManager.getSimState(1) != TelephonyManager.SIM_STATE_READY)){
                        Toast.makeText(mContext, R.string.sim_disabled, Toast.LENGTH_LONG).show();
                    }else if(isSimType()){
                        Toast.makeText(mContext, R.string.beyond_sim_card_capacity, Toast.LENGTH_LONG).show();
                    }
                }
                /**
                 * @}
                 */
            }
        }
        switch (saveMode) {
            case SaveMode.CLOSE: {
                final Intent resultIntent;
                if (saveSucceeded && contactLookupUri != null) {
                    final Uri lookupUri = maybeConvertToLegacyLookupUri(
                            mContext, contactLookupUri, mLookupUri);
                    resultIntent = ImplicitIntentsUtil.composeQuickContactIntent(lookupUri,
                            QuickContactActivity.MODE_FULLY_EXPANDED);
                    resultIntent.putExtra(QuickContactActivity.EXTRA_PREVIOUS_SCREEN_TYPE,
                            ScreenType.EDITOR);
                } else {
                    resultIntent = null;
                }
                // It is already saved, so prevent it from being saved again
                mStatus = Status.CLOSING;
                if (mListener != null) mListener.onSaveFinished(resultIntent);
                break;
            }
            case SaveMode.COMPACT: {
                // It is already saved, so prevent it from being saved again
                mStatus = Status.CLOSING;
                if (mListener != null) mListener.onSaveFinished(/* resultIntent= */ null);
                break;
            }
            case SaveMode.JOIN:
                if (saveSucceeded && contactLookupUri != null && joinContactId != null) {
                    joinAggregate(joinContactId);
                }
                break;
            case SaveMode.RELOAD:
                if (saveSucceeded && contactLookupUri != null) {
                    // If this was in INSERT, we are changing into an EDIT now.
                    // If it already was an EDIT, we are changing to the new Uri now
                    mState = new RawContactDeltaList();
                    load(Intent.ACTION_EDIT, contactLookupUri, null);
                    mStatus = Status.LOADING;
                    getLoaderManager().restartLoader(LOADER_CONTACT, null, mContactLoaderListener);
                }
                break;

            case SaveMode.SPLIT:
                mStatus = Status.CLOSING;
                if (mListener != null) {
                    mListener.onContactSplit(contactLookupUri);
                } else {
                    Log.d(TAG, "No listener registered, can not call onSplitFinished");
                }
                break;
        }
    }

    /**
     * Shows a list of aggregates that can be joined into the currently viewed aggregate.
     *
     * @param contactLookupUri the fresh URI for the currently edited contact (after saving it)
     */
    /**
     * SPRD: 598433 598432 when onstop(), there is crash on splite mode to show quickcontact
     * @{
     */
    public static boolean mJoinContacts = false;
    /**
     * @
     */
    private void showJoinAggregateActivity(Uri contactLookupUri) {
        if (contactLookupUri == null || !isAdded()) {
            return;
        }
        // SPRD: 598433 598432 when onstop(), there is crash on splite mode to show quickcontact
        mJoinContacts = true;
        mContactIdForJoin = ContentUris.parseId(contactLookupUri);
        final Intent intent = new Intent(UiIntentActions.PICK_JOIN_CONTACT_ACTION);
        intent.putExtra(UiIntentActions.TARGET_CONTACT_ID_EXTRA_KEY, mContactIdForJoin);
        startActivityForResult(intent, REQUEST_CODE_JOIN);
    }

    //
    // Aggregation PopupWindow
    //

    /**
     * Triggers an asynchronous search for aggregation suggestions.
     */
    protected void acquireAggregationSuggestions(Context context,
            long rawContactId, ValuesDelta valuesDelta) {
        if (mAggregationSuggestionsRawContactId != rawContactId
                && mAggregationSuggestionView != null) {
            mAggregationSuggestionView.setVisibility(View.GONE);
            mAggregationSuggestionView = null;
            mAggregationSuggestionEngine.reset();
        }

        mAggregationSuggestionsRawContactId = rawContactId;

        if (mAggregationSuggestionEngine == null) {
            mAggregationSuggestionEngine = new AggregationSuggestionEngine(context);
            mAggregationSuggestionEngine.setListener(this);
            mAggregationSuggestionEngine.start();
        }

        mAggregationSuggestionEngine.setContactId(getContactId());

        mAggregationSuggestionEngine.onNameChange(valuesDelta);
    }

    /**
     * Returns the contact ID for the currently edited contact or 0 if the contact is new.
     */
    private long getContactId() {
        for (RawContactDelta rawContact : mState) {
            Long contactId = rawContact.getValues().getAsLong(RawContacts.CONTACT_ID);
            if (contactId != null) {
                return contactId;
            }
        }
        return 0;
    }

    @Override
    public void onAggregationSuggestionChange() {
        final Activity activity = getActivity();
        if ((activity != null && activity.isFinishing())
                || !isVisible() ||  mState.isEmpty() || mStatus != Status.EDITING) {
            return;
        }

        UiClosables.closeQuietly(mAggregationSuggestionPopup);

        if (mAggregationSuggestionEngine.getSuggestedContactCount() == 0) {
            return;
        }

        final View anchorView = getAggregationAnchorView(mAggregationSuggestionsRawContactId);
        if (anchorView == null) {
            return; // Raw contact deleted?
        }
        mAggregationSuggestionPopup = new ListPopupWindow(mContext, null);
        mAggregationSuggestionPopup.setAnchorView(anchorView);
        mAggregationSuggestionPopup.setWidth(anchorView.getWidth());
        mAggregationSuggestionPopup.setInputMethodMode(ListPopupWindow.INPUT_METHOD_NOT_NEEDED);
        mAggregationSuggestionPopup.setAdapter(
                new AggregationSuggestionAdapter(
                        getActivity(),
                        mState.size() == 1 && mState.get(0).isContactInsert(),
                        /* listener =*/ this,
                        mAggregationSuggestionEngine.getSuggestions()));
        mAggregationSuggestionPopup.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                final AggregationSuggestionView suggestionView = (AggregationSuggestionView) view;
                suggestionView.handleItemClickEvent();
                UiClosables.closeQuietly(mAggregationSuggestionPopup);
                mAggregationSuggestionPopup = null;
            }
        });
        mAggregationSuggestionPopup.show();
    }

    /**
     * Returns the raw contact editor view for the given rawContactId that should be used as the
     * anchor for aggregation suggestions.
     */
    abstract protected View getAggregationAnchorView(long rawContactId);

    /**
     * Whether the given raw contact ID matches the one used to last load aggregation
     * suggestions.
     */
    protected boolean isAggregationSuggestionRawContactId(long rawContactId) {
        return mAggregationSuggestionsRawContactId == rawContactId;
    }

    @Override
    public void onJoinAction(long contactId, List<Long> rawContactIdList) {
        final long rawContactIds[] = new long[rawContactIdList.size()];
        for (int i = 0; i < rawContactIds.length; i++) {
            rawContactIds[i] = rawContactIdList.get(i);
        }
        try {
            JoinSuggestedContactDialogFragment.show(this, rawContactIds);
        } catch (Exception ignored) {
            // No problem - the activity is no longer available to display the dialog
        }
    }

    /**
     * Joins the suggested contact (specified by the id's of constituent raw
     * contacts), save all changes, and stay in the editor.
     */
    protected void doJoinSuggestedContact(long[] rawContactIds) {
        if (!hasValidState() || mStatus != Status.EDITING) {
            return;
        }

        mState.setJoinWithRawContacts(rawContactIds);
        save(SaveMode.RELOAD);
    }

    @Override
    public void onEditAction(Uri contactLookupUri) {
        SuggestionEditConfirmationDialogFragment.show(this, contactLookupUri);
    }

    /**
     * Abandons the currently edited contact and switches to editing the suggested
     * one, transferring all the data there
     */
    protected void doEditSuggestedContact(Uri contactUri) {
        if (mListener != null) {
            // make sure we don't save this contact when closing down
            mStatus = Status.CLOSING;
            mListener.onEditOtherContactRequested(
                    contactUri, mState.get(0).getContentValues());
        }
    }

    //
    // Join Activity
    //

    /**
     * Performs aggregation with the contact selected by the user from suggestions or A-Z list.
     */
    abstract protected void joinAggregate(long contactId);

    protected void removeNewRawContactPhotos() {
        for (String key : mUpdatedPhotos.keySet()) {
            try {
                if (Integer.parseInt(key) < 0) {
                    mUpdatedPhotos.remove(key);
                }
            } catch (NumberFormatException ignored) {
            }
        }
    }

    //
    // Utility methods
    //

    /**
     * Returns a legacy version of the given contactLookupUri if a legacy Uri was originally
     * passed to the contact editor.
     *
     * @param contactLookupUri The Uri to possibly convert to legacy format.
     * @param requestLookupUri The lookup Uri originally passed to the contact editor
     *                         (via Intent data), may be null.
     */
    protected static Uri maybeConvertToLegacyLookupUri(Context context, Uri contactLookupUri,
            Uri requestLookupUri) {
        final String legacyAuthority = "contacts";
        final String requestAuthority = requestLookupUri == null
                ? null : requestLookupUri.getAuthority();
        if (legacyAuthority.equals(requestAuthority)) {
            // Build a legacy Uri if that is what was requested by caller
            final long contactId = ContentUris.parseId(Contacts.lookupContact(
                    context.getContentResolver(), contactLookupUri));
            final Uri legacyContentUri = Uri.parse("content://contacts/people");
            return ContentUris.withAppendedId(legacyContentUri, contactId);
        }
        // Otherwise pass back a lookup-style Uri
        return contactLookupUri;
    }

    /**
     * Whether the argument Intent requested a contact insert action or not.
     */
    protected static boolean isInsert(Intent intent) {
        return intent == null ? false : isInsert(intent.getAction());
    }

    protected static boolean isInsert(String action) {
        return Intent.ACTION_INSERT.equals(action)
                || ContactEditorBaseActivity.ACTION_INSERT.equals(action);
    }

    protected static boolean isEdit(String action) {
        return Intent.ACTION_EDIT.equals(action)
                || ContactEditorBaseActivity.ACTION_EDIT.equals(action);
    }
    /**
     * SPRD:
     * @{
     */
    private static final String KEY_INTENT_EXTRAS = "intentExtras";
    private static final String PROFILE_SEGMENT = "profile";
    public boolean mIsSaveFailure = false;
    /**
     * SPRD:Bug626425  The DUT will be crashed when split the screen and calling.
     * @{
     */
    public void dismissDialog(){
        Fragment prev = getFragmentManager().findFragmentByTag("dialoglist");
        if (prev != null) {
            DialogFragment df = (DialogFragment) prev;
            df.dismissAllowingStateLoss();
        }
    }
    /**
     * SPRD:Bug494110 When switch the lanugage and go into more fields,
     * the number will be repeat preserved.
     *
     * Original Android code:
    private boolean isSimType() {
     * @{
     */
    protected boolean isSimType() {
    /**
     * @}
     */
        boolean SimAccount = false;
        if (mState != null && mState.size() > 0) {
            String accountType = mState.get(0).getValues().getAsString(RawContacts.ACCOUNT_TYPE);
            if (accountType != null
                    && (accountType.equals(SimAccountType.ACCOUNT_TYPE) || accountType
                            .equals(USimAccountType.ACCOUNT_TYPE))) {
                SimAccount = true;
            }
        }
        return SimAccount;
    }
    /**
     * @}
     */
    /**
     * SPRD:Bug453331 Add the maxNum limit for contact join when the user is a monkey.
     * @{
     */
    private boolean isJoinNumMax() {
        if (ActivityManager.isUserAMonkey()) {
            if (mState != null && mState.size() > 2) {
                return true;
            }
        }
        return false;
    }
    /**
     * @}
     */
    /**
     * SPRD:
     * Bug426810 Add a number replace dialog when add a number to an existing contact in sms screen;Bug449339,Bug454782
     * @{
     */
    private static final int ID_NO_CONFIRM_REPLACE_DIALOG = 0;
    private static final int ID_CONFIRM_REPLACE_DIALOG = 1;
    private static final int ID_CONFIRM_REPLACE_DIALOG_LIST = 2;

    private static final String KEY_DATA_LIST = "dataList";
    private static final String KEY_DATA_ID_LIST = "dataIdList";
    private static final String KEY_PHONETIC_NAME_ADDED = "phoneticNameAdded";
    private static final String KEY_DIALOG_SHOWING = "hasDialog";

    private boolean mReplaceDialogShowing = false;
    private boolean mDataListChange =false;

    private Set<String> mInsertedMimeTypes;
    private ArrayList<String> mExistedDataList;
    private ArrayList<String> mExistedDataIdList;
    private ArrayList<String> mOrigExistedDataList;
    private boolean mPhoneticNameAdded;

    private int showReplaceDialog(Contact contact) {
        if (contact == null) {
            return 0;
        }
        RawContactDeltaList entityDeltaList = contact.createRawContactDeltaList();
        mExistedDataIdList = new ArrayList<String>();
        mExistedDataList = new ArrayList<String>();

        AccountWithDataSet account = contact.getAccount();
        AccountTypeManager accountTypeManager = AccountTypeManager.getInstance(mContext);
        AccountType accountType = null;
        if (account != null) {
            accountType = accountTypeManager.getAccountTypeForAccount(account);
        }
        final ContentValues contentValues = new ContentValues();
        RawContactDelta insert = new RawContactDelta(ValuesDelta.fromAfter(contentValues));
        if (accountType != null) {
            RawContactModifier.parseExtras(mContext, accountType, insert, mIntentExtras);
        }
        mInsertedMimeTypes = insert.getMimeTypes();
        mInsertedMimeTypes.remove(CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE);

        if (entityDeltaList == null) {
            return 0;
        }
        for (RawContactDelta entityDelta : entityDeltaList) {

            final RawContactDelta entity = entityDelta;
            final ValuesDelta values = entity.getValues();
            if (values == null || !values.isVisible()) {
                continue;
            }

            String strValue = null;
            for (String mimetype : entity.getMimeTypes()) {
                int maxLen = -1;
                if (mInsertedMimeTypes.contains(mimetype) && accountType != null) {
                    if (!SimAccountType.ACCOUNT_TYPE.equals(accountType.accountType)
                            && !USimAccountType.ACCOUNT_TYPE.equals(accountType.accountType)) {
                        maxLen = AccountTypeManager.getTypeOverallMaxForAccount(accountType,
                                mimetype);
                    } else {
                        int max = AccountRestrictionUtils.get(mContext)
                                .getTypeOverallMax(account, mimetype);
                        if (max != 0) {
                            maxLen = max;
                        }
                    }
                    if (mimetype == null || maxLen == -2) {
                        continue;
                    }
                    for (ValuesDelta child : entity.getMimeEntries(mimetype)) {
                        if (child.containsKey("_id") && child.containsKey("data1")) {
                            mExistedDataList.add(child.getAsString("data1"));
                            mExistedDataIdList.add(child.getAsString("_id"));
                        } else {
                            continue;
                        }
                    }
                    mDataListChange = !(mExistedDataList.equals(mOrigExistedDataList));
                    if (mExistedDataList.size() > 0 && mExistedDataList.size() == maxLen) {
                        if (maxLen == 1) {
                            return ID_CONFIRM_REPLACE_DIALOG;
                        } else if (maxLen > 1) {
                            return ID_CONFIRM_REPLACE_DIALOG_LIST;
                        }
                    } else {
                        return 0;
                    }
                }
            }
        }
        return 0;
    }

    public static class ConfirmReplaceDialogFragment extends DialogFragment {

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            AlertDialog dialog = new AlertDialog.Builder(getActivity())
                    .setTitle(R.string.non_phone_add_to_contacts)
                    .setMessage(R.string.confirmCoverPhoneNumber)
                    .setPositiveButton(android.R.string.ok,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int whichButton) {
                                    /**
                                     * SPRD: Bug 536659 The DUT crashes after switching language
                                     * from contact edit screen and clicking the alert dialog.
                                     * Android original code:
                                    ContactEditorFragment targetFragment =
                                            (ContactEditorFragment) getTargetFragment();
                                     * @{
                                     */
                                    ContactEditorBaseFragment targetFragment =
                                            (ContactEditorBaseFragment) getTargetFragment();
                                    /*
                                     * @}
                                     */
                                    /**
                                     * SPRD: Bug 606497 The DUT crashes after select replace original contact
                                     * @{
                                     */
                                    if (mRawContacts == null) {
                                        targetFragment.setStateForExistingContact(null, false,
                                                        sRawContactsforReplaceDialog);
                                    } else {
                                        targetFragment.setStateForExistingContact(null, false,
                                                        mRawContacts);
                                    }
                                    /*
                                     * @}
                                     */
                                }
                            }
                    )
                    .setNegativeButton(android.R.string.cancel,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int whichButton) {
                                    if (getActivity() != null) {
                                        getActivity().finish();
                                    }
                                }
                            })
                    .create();
            return dialog;
        }

        @Override
        public void onCancel(DialogInterface dialog) {
            if (getActivity() != null) {
                getActivity().finish();
            }
        }
    }

    public static class ConfirmReplaceListDialogFragment extends DialogFragment {
        //Bug440418,597225 Save the selected sim number on replaceDialog  when rotating the screen.
        static Long mSelectDataId ;
        static int mSelectDataItem = 0;

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            ArrayList<String> dataList = getArguments().getStringArrayList(KEY_DATA_LIST);
            ArrayList<String> dataIdList = getArguments().getStringArrayList(KEY_DATA_ID_LIST);
            String[] dataStrings = (String[]) dataList
                    .toArray(new String[dataList.size()]);
            final String[] dataIds = (String[]) dataIdList.toArray(new String[dataIdList.size()]);
            /**
             * Bug440418,597225 Save the selected sim number on replaceDialog  when rotating the screen.
             * SPRD:Bug608343 Replace sim contact number from call log, can not save the info at the second time.
             * @{
             */

            if (dataIds.length > 0) {
                 /**
                 * SPRD:Bug610452 remember selected station and restore the
                 * station when split-screen
                 * @{
                 */
                mSelectDataId = mSelectDataId == null ? Long.valueOf(dataIds[mSelectDataItem]) : mSelectDataId;
                /**
                 * @}
                 */
            }
            /**
             * @}
             */

            AlertDialog dialog = new AlertDialog.Builder(getActivity())
                    .setTitle(R.string.replaceSelected)
                    .setSingleChoiceItems(dataStrings, mSelectDataItem,
                            new DialogInterface.OnClickListener() {
                                public void onClick(DialogInterface dialog, int which) {
                                    if (which >= 0) {
                                        mSelectDataItem = which;
                                        mSelectDataId = Long.valueOf(dataIds[which]);
                                    }
                                }
                            })
                    .setPositiveButton(android.R.string.ok,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int whichButton) {
                                    /**
                                     * SPRD:Bug613963 it occured crash when click confirm button
                                     * twice under split-screen
                                     * @{
                                     */
                                 if(mSelectDataId == null){
                                     if (getActivity() != null) {
                                           Toast.makeText(getActivity(),R.string.replace_data_error,
                                               Toast.LENGTH_LONG).show();
                                           getActivity().finish();
                                      }
                                 }else{
                                    ContactEditorBaseFragment targetFragment =
                                            (ContactEditorBaseFragment) getTargetFragment();
                                    targetFragment.mIntentExtras.putLong("replaceDataId",
                                            mSelectDataId);
                                    /**
                                     * SPRD:Bug612258 The contacts crash when replacing the contacts number
                                     * @{
                                     */
                                    if (mRawContacts == null) {
                                        targetFragment.setStateForExistingContact(null, false,
                                                        sRawContactsforReplaceDialog);
                                    } else {
                                        targetFragment.setStateForExistingContact(null, false,
                                                        mRawContacts);
                                    }
                                    /**
                                     * @}
                                     */
                                }
                                /**
                                 * @}
                                 */
                               }
                            })
                    .setNegativeButton(android.R.string.cancel,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int whichButton) {
                                    if (getActivity() != null) {
                                        getActivity().finish();
                                    }
                                }
                            }).create();
            return dialog;
        }
        /**
         * SPRD:Bug440418,597225 Save the selected sim number on replaceDialog  when rotating the screen.
         * @{
         */
        @Override
        public void onSaveInstanceState(Bundle outState) {
            // TODO Auto-generated method stub
            super.onSaveInstanceState(outState);
            outState.putLong(SELECTED_DATA_ID, mSelectDataId);
            outState.putInt(SELECTED_DATA_ITEM, mSelectDataItem);
        }

        @Override
        public void onCreate(Bundle savedInstanceState) {
            // TODO Auto-generated method stub
            super.onCreate(savedInstanceState);
            if (savedInstanceState == null) {
                 /**
                 * SPRD:Bug610452 remember selected station and restore the
                 * station when split-screen
                 * @{
                 */
                if(dataItem != -1){
                  mSelectDataItem = dataItem;
                }
                /**
                 * @}
                 */
                return;
            }
            mSelectDataId = savedInstanceState.getLong(SELECTED_DATA_ID);
            mSelectDataItem = savedInstanceState.getInt(SELECTED_DATA_ITEM);
        }

        /**
         * SPRD:Bug442855 Recovery mSelectDataId on onDestory function to avoid crash when unlock screen.
         * SPRD:Bug600523 The DUT can not switch to sim account after editing a sim contact.
         * SPRD:Bug608343 Replace sim contact number from call log, can not save the info at the second time.
         * @{
         */
        @Override
        public void onDestroy() {
            // TODO Auto-generated method stub
            super.onDestroy();
            mContactFound = false;
            mSelectDataItem = 0;
            /**
             * SPRD:Bug610452 remember selected station and restore the
             * station when split-screen
             * @{
             */
            mSelectDataId = null;
            /**
             * @}
             */
        }
        /**
         * @}
         */
        /**
         * @}
         */

        @Override
        public void onCancel(DialogInterface dialog) {
            if (getActivity() != null) {
                getActivity().finish();
            }
        }
    }

    public void setReplaceDialogShowing(boolean isShowing){
        mReplaceDialogShowing = isShowing;
   }

    public boolean getReplaceDialogShowing(){
        return mReplaceDialogShowing;
   }
    /**
     * SPRD:Bug626425 The DUT will be crashed when split the screen and calling .
     * @{
     */
    private Handler mHandler = new Handler() {

        public void handleMessage(Message msg) {
            switch (msg.what) {
                case ID_CONFIRM_REPLACE_DIALOG:
                    dismissDialog();
                    ConfirmReplaceDialogFragment dialog =
                            new ConfirmReplaceDialogFragment();
                    mReplaceDialogShowing = true;
                    dialog.setTargetFragment(ContactEditorBaseFragment.this, 0);
                    dialog.showAllowingStateLoss(getFragmentManager(), "dialoglist");
                    break;
                case ID_CONFIRM_REPLACE_DIALOG_LIST:
                    dismissDialog();
                    ConfirmReplaceListDialogFragment listDialog =
                            new ConfirmReplaceListDialogFragment();
                    mReplaceDialogShowing = true;
                    Bundle args = new Bundle();
                    args.putStringArrayList(KEY_DATA_LIST, mExistedDataList);
                    args.putStringArrayList(KEY_DATA_ID_LIST, mExistedDataIdList);
                    listDialog.setArguments(args);
                    listDialog.setTargetFragment(ContactEditorBaseFragment.this, 0);
                    listDialog.showAllowingStateLoss(getFragmentManager(), "dialoglist");
                    break;
                case ID_NO_CONFIRM_REPLACE_DIALOG:
                    dismissDialog();
                    break;
            }
        }
    };
    /**
     * @}
     */

    /**
     * SPRD:Bug494562 When clear the info of contact and then delete it, don't save it again.
     *
     * @{
     */
    protected boolean isDeleting() {
        if(ContactDeletionInteraction.doDeleteContact) {
            ContactDeletionInteraction.doDeleteContact = false;
            return true;
        } else {
            return false;
        }
    }
    /**
     * @}
     */

    /**
     * SPRD:Bug494336 Need READ_EXTERNAL_STORAGE permission to pick photo.
     * @{
     */
    protected static int READ_EXTERNAL_STORAGE_REQUESTCODE = 10;
    protected boolean checkPermission() {
        return getActivity().checkSelfPermission(permission.READ_EXTERNAL_STORAGE)
                == PackageManager.PERMISSION_GRANTED;
    }
    protected void requestReadStoragePermission() {
        Trace.beginSection("requestPermissions");
        try {
            requestPermissions(new String[]{permission.READ_EXTERNAL_STORAGE},
                               READ_EXTERNAL_STORAGE_REQUESTCODE);
        } finally {
            Trace.endSection();
        }
    }
    /**
     * @}
     */
}
