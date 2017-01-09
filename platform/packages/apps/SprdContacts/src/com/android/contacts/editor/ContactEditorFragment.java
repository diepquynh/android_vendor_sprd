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
 * limitations under the License
 */

package com.android.contacts.editor;

import android.accounts.Account;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.Photo;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.LinearLayout;
import android.widget.ListPopupWindow;

import com.android.contacts.ContactSaveService;
import com.android.contacts.R;
import com.android.contacts.activities.ContactEditorActivity;
import com.android.contacts.activities.ContactEditorBaseActivity.ContactEditor;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.RawContactDelta;
import com.android.contacts.common.model.RawContactDeltaList;
import com.android.contacts.common.model.ValuesDelta;
import com.android.contacts.common.model.account.AccountType;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.util.AccountsListAdapter;
import com.android.contacts.common.util.AccountsListAdapter.AccountListFilter;
import com.android.contacts.detail.PhotoSelectionHandler;
import com.android.contacts.editor.Editor.EditorListener;
import com.android.contacts.util.ContactPhotoUtils;
import com.android.contacts.util.UiClosables;

import java.io.FileNotFoundException;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
/**
 * SPRD:
 *
 * @{
 */
import android.widget.PopupMenu;
import java.util.Set;
import java.util.Map;
import java.util.Locale;
import android.app.ActivityManager;
import android.os.Handler;
import android.os.Message;
import android.provider.ContactsContract.CommonDataKinds;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract;
import android.view.WindowManager;
import com.android.contacts.common.util.Constants;
import com.android.contacts.interactions.ContactDeletionInteraction;
import com.sprd.contacts.util.AccountRestrictionUtils;
import com.sprd.contacts.common.model.account.SimAccountType;
import com.sprd.contacts.common.model.account.USimAccountType;
import com.sprd.contacts.common.model.account.PhoneAccountType;
import com.sprd.contacts.util.AccountsForMimeTypeUtils;
import android.content.pm.PackageManager;
import android.widget.Toast;
/**
 * @}
 */
/**
 * Contact editor with all fields displayed.
 */
public class ContactEditorFragment extends ContactEditorBaseFragment implements
        RawContactReadOnlyEditorView.Listener {

    private static final String KEY_EXPANDED_EDITORS = "expandedEditors";

    private static final String KEY_RAW_CONTACT_ID_REQUESTING_PHOTO = "photorequester";
    private static final String KEY_CURRENT_PHOTO_URI = "currentphotouri";
    private static final String KEY_UPDATED_PHOTOS = "updatedPhotos";

    // Used to store which raw contact editors have been expanded. Keyed on raw contact ids.
    private HashMap<Long, Boolean> mExpandedEditors = new HashMap<Long, Boolean>();

    /**
     * The raw contact for which we started "take photo" or "choose photo from gallery" most
     * recently.  Used to restore {@link #mCurrentPhotoHandler} after orientation change.
     */
    private long mRawContactIdRequestingPhoto;

    /**
     * The {@link PhotoHandler} for the photo editor for the {@link #mRawContactIdRequestingPhoto}
     * raw contact.
     *
     * A {@link PhotoHandler} is created for each photo editor in {@link #bindPhotoHandler}, but
     * the only "active" one should get the activity result.  This member represents the active
     * one.
     */
    private PhotoHandler mCurrentPhotoHandler;
    private Uri mCurrentPhotoUri;
    private Bundle mUpdatedPhotos = new Bundle();

    public ContactEditorFragment() {
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedState) {
        final View view = inflater.inflate(R.layout.contact_editor_fragment, container, false);

        mContent = (LinearLayout) view.findViewById(R.id.editors);

        setHasOptionsMenu(true);

        return view;
    }

    @Override
    public void onCreate(Bundle savedState) {
        super.onCreate(savedState);

        if (savedState != null) {
            mExpandedEditors = (HashMap<Long, Boolean>)
                    savedState.getSerializable(KEY_EXPANDED_EDITORS);
            mRawContactIdRequestingPhoto = savedState.getLong(
                    KEY_RAW_CONTACT_ID_REQUESTING_PHOTO);
            mCurrentPhotoUri = savedState.getParcelable(KEY_CURRENT_PHOTO_URI);
            mUpdatedPhotos = savedState.getParcelable(KEY_UPDATED_PHOTOS);
            mRawContactIdToDisplayAlone = savedState.getLong(
                    ContactEditorBaseFragment.INTENT_EXTRA_RAW_CONTACT_ID_TO_DISPLAY_ALONE, -1);
        }
    }

    @Override
    public void load(String action, Uri lookupUri, Bundle intentExtras) {
        super.load(action, lookupUri, intentExtras);
        if (intentExtras != null) {
            mRawContactIdToDisplayAlone = intentExtras.getLong(
                    ContactEditorBaseFragment.INTENT_EXTRA_RAW_CONTACT_ID_TO_DISPLAY_ALONE, -1);
        }
    }

    @Override
    public void onStart() {
        getLoaderManager().initLoader(LOADER_GROUPS, null, mGroupsLoaderListener);
        super.onStart();
    }

    @Override
    public void onStop() {
        super.onStop();

        /**
         * SPRD:Bug 566047 The DUT fails to save contact after locking and unlocking screen.
         * @{
         */
        if (PhotoActionPopup.mlistPopupWindow != null) {
            PhotoActionPopup.mlistPopupWindow.dismiss();
        }
        /*
         * @}
         */

        /* SPRD: Bug604592 add and join contacts fail if entering from APP-Messaging or APP-Phone. @{ */
        if (mJoinContacts) {
            return;
        }
        /* @{ */

        // If anything was left unsaved, save it now and return to the compact editor.
        /**
         * SPRD:Bug494562 When clear the info of contact and then delete it, don't save it again.
         *
         * Original Android code:
        if (!getActivity().isChangingConfigurations() && mStatus == Status.EDITING) {
         * @{
         */
        if (!isDeleting() && !getActivity().isChangingConfigurations() && mStatus == Status.EDITING) {
        /**
         * @}
         */
            /**
             * SPRD:Bug 509303 The SIM contact edit screen is closed after locking and unlocking
             * screen of devices.
             * SPRD:Bug536289 save contacts and press recent twice, the DUT
             * return to the original interface instead of the edit interface
             * SPRD: 598433 when onstop(), there is crash on splite mode to show quickcontact
             * SPRD:Bug612672 The editor is closed when rotate the screen
             * @{
             **/
           if(!sameDataChanged){
            if (isSimType()) {
                //save(SaveMode.CLOSE, /* backPressed = */false);
                isSave = true;
                save(SaveMode.COMPACT);
            } else {
                //save(SaveMode.RELOAD, /* backPressed = */false);
                save(SaveMode.COMPACT);
            }
           }
           sameDataChanged = false;
            /*
             * @}
             */
        }
    }

    @Override
    public void onExternalEditorRequest(AccountWithDataSet account, Uri uri) {
        if (mListener != null) {
            mListener.onCustomEditContactActivityRequested(account, uri, null, false);
        }
    }

    @Override
    public void onEditorExpansionChanged() {
        updatedExpandedEditorsMap();
    }

    @Override
    protected void setGroupMetaData() {
        if (mGroupMetaData == null) {
            return;
        }
        int editorCount = mContent.getChildCount();
        for (int i = 0; i < editorCount; i++) {
            BaseRawContactEditorView editor = (BaseRawContactEditorView) mContent.getChildAt(i);
            editor.setGroupMetaData(mGroupMetaData);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            return revert();
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void bindEditors() {
        // bindEditors() can only bind views if there is data in mState, so immediately return
        // if mState is null
        if (mState.isEmpty()) {
            return;
        }

        // Check if delta list is ready.  Delta list is populated from existing data and when
        // editing an read-only contact, it's also populated with newly created data for the
        // blank form.  When the data is not ready, skip. This method will be called multiple times.
        if ((mIsEdit && !mExistingContactDataReady) || (mHasNewContact && !mNewContactDataReady)) {
            return;
        }

        // Sort the editors
        Collections.sort(mState, mComparator);

        // Remove any existing editors and rebuild any visible
        mContent.removeAllViews();

        final LayoutInflater inflater = (LayoutInflater) mContext.getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);
        final AccountTypeManager accountTypes = AccountTypeManager.getInstance(mContext);
        int numRawContacts = mState.size();

        for (int i = 0; i < numRawContacts; i++) {
            // TODO ensure proper ordering of entities in the list
            final RawContactDelta rawContactDelta = mState.get(i);
            if (!rawContactDelta.isVisible()) continue;

            final AccountType type = rawContactDelta.getAccountType(accountTypes);
            final long rawContactId = rawContactDelta.getRawContactId();

            if (mRawContactIdToDisplayAlone != -1 && mRawContactIdToDisplayAlone != rawContactId) {
                continue;
            }

            final BaseRawContactEditorView editor;
            if (!type.areContactsWritable()) {
                editor = (BaseRawContactEditorView) inflater.inflate(
                        R.layout.raw_contact_readonly_editor_view, mContent, false);
            } else {
                editor = (RawContactEditorView) inflater.inflate(R.layout.raw_contact_editor_view,
                        mContent, false);
            }
            /**
             * SPRD:Bug498039 Dismiss the custom label dialog, when lock the screen.
             * @{
             */
            mContactEditor = editor;
            /**
             * @}
             */
            editor.setListener(this);
            /**
             * SPRD:
             *
             * @{
             */
            final String accountType = rawContactDelta.getAccountType();
            final String accountName = rawContactDelta.getAccountName();
            final Account account = (accountType != null && accountName != null)? new Account(accountName, accountType): null;
            // verify bindEditors EntityDelta
            String strValue = null;
            int maxLen = -1, maxLength = -1;
            for (String mimetype : rawContactDelta.getMimeTypes()) {
                maxLen = accountTypes.getAccountTypeFieldsMaxLength(mContext,
                        account, mimetype);
                if (mimetype == null || maxLen <= 0) {
                    continue;
                }
                for (ValuesDelta child : rawContactDelta.getMimeEntries(mimetype)) {
                    if (child == null) {
                        continue;
                    }
                    for (String cKey : child.keySet()) {
                        strValue = child.getAsString(cKey);
                        maxLength = accountTypes.getTextFieldsEditorMaxLength(mContext, account,
                                strValue, maxLen);
                        if (cKey == null || strValue == null || maxLength <= 0) {
                            continue;
                        }
                        if (!cKey.equals("_id") && !strValue.equals(mimetype)
                                && (strValue.length() > maxLength)) {
                            if (Phone.CONTENT_ITEM_TYPE.equals(mimetype) &&
                                    (SimAccountType.ACCOUNT_TYPE.equals(accountType) ||
                                    USimAccountType.ACCOUNT_TYPE.equals(accountType))) {
                                if (strValue.length() > 0 && strValue.charAt(0) == '+') {
                                    maxLength = maxLength + 1;
                                }
                            }
                            child.put(cKey, strValue.substring(0, maxLength));
                        }
                    }
                }
            }
            /**
             * @}
             */
            final List<AccountWithDataSet> accounts = AccountTypeManager.getInstance(mContext)
                    .getAccounts(true);
            if (mHasNewContact && !mNewLocalProfile && accounts.size() > 1) {
                addAccountSwitcher(mState.get(0), editor);
            }

            editor.setEnabled(isEnabled());

            if (mRawContactIdToDisplayAlone != -1) {
                editor.setCollapsed(false);
            } else if (mExpandedEditors.containsKey(rawContactId)) {
                editor.setCollapsed(mExpandedEditors.get(rawContactId));
            } else {
                // By default, only the first editor will be expanded.
                editor.setCollapsed(i != 0);
            }

            mContent.addView(editor);

            editor.setState(rawContactDelta, type, mViewIdGenerator, isEditingUserProfile());
            if (mRawContactIdToDisplayAlone != -1) {
                editor.setCollapsible(false);
            } else {
                editor.setCollapsible(numRawContacts > 1);
            }

            // Set up the photo handler.
            bindPhotoHandler(editor, type, mState);

            // If a new photo was chosen but not yet saved, we need to update the UI to
            // reflect this.
            final Uri photoUri = updatedPhotoUriForRawContact(rawContactId);
            if (photoUri != null) editor.setFullSizedPhoto(photoUri);

            if (editor instanceof RawContactEditorView) {
                final Activity activity = getActivity();
                final RawContactEditorView rawContactEditor = (RawContactEditorView) editor;
                final ValuesDelta nameValuesDelta = rawContactEditor.getNameEditor().getValues();
                final EditorListener structuredNameListener = new EditorListener() {

                    @Override
                    public void onRequest(int request) {
                        // Make sure the activity is running
                        if (activity.isFinishing()) {
                            return;
                        }
                        if (!isEditingUserProfile()) {
                            if (request == EditorListener.FIELD_CHANGED) {
                                if (!nameValuesDelta.isSuperPrimary()) {
                                    unsetSuperPrimaryForAllNameEditors();
                                    nameValuesDelta.setSuperPrimary(true);
                                }
                                acquireAggregationSuggestions(activity,
                                        rawContactEditor.getNameEditor().getRawContactId(),
                                        rawContactEditor.getNameEditor().getValues());
                            } else if (request == EditorListener.FIELD_TURNED_EMPTY) {
                                if (nameValuesDelta.isSuperPrimary()) {
                                    nameValuesDelta.setSuperPrimary(false);
                                }
                            }
                        }
                    }

                    @Override
                    public void onDeleteRequested(Editor removedEditor) {
                    }
                };

                final StructuredNameEditorView nameEditor = rawContactEditor.getNameEditor();
                /**
                 * SPRD: bug431383,452269
                 *
                 * Original Android code:
                nameEditor.setEditorListener(listener);
                 * @{
                 */
                boolean isSimContact = (accountName != null) ? accountName.toLowerCase(Locale.US).contains("sim") : false;
                if(!isSimContact) {
                    nameEditor.setEditorListener(structuredNameListener);
                }
                /**
                 * @}
                 */

                rawContactEditor.setAutoAddToDefaultGroup(mAutoAddToDefaultGroup);
                /**
                 * SPRD: bug431383
                 *
                 * Original Android code:
                if (!isEditingUserProfile() && isAggregationSuggestionRawContactId(rawContactId)) {
                 * @{
                 */
                if (!isEditingUserProfile() && isAggregationSuggestionRawContactId(rawContactId) &&  !isSimContact) {
                /**
                 * @}
                 */
                    acquireAggregationSuggestions(activity,
                            rawContactEditor.getNameEditor().getRawContactId(),
                            rawContactEditor.getNameEditor().getValues());
                }
            }
        }

        setGroupMetaData();

        // Show editor now that we've loaded state
        mContent.setVisibility(View.VISIBLE);

        // Refresh Action Bar as the visibility of the join command
        // Activity can be null if we have been detached from the Activity
        invalidateOptionsMenu();

        updatedExpandedEditorsMap();
    }

    private void unsetSuperPrimaryForAllNameEditors() {
        for (int i = 0; i < mContent.getChildCount(); i++) {
            final View view = mContent.getChildAt(i);
            if (view instanceof RawContactEditorView) {
                final RawContactEditorView rawContactEditorView = (RawContactEditorView) view;
                final StructuredNameEditorView nameEditorView =
                        rawContactEditorView.getNameEditor();
                if (nameEditorView != null) {
                    final ValuesDelta valuesDelta = nameEditorView.getValues();
                    if (valuesDelta != null) {
                        valuesDelta.setSuperPrimary(false);
                    }
                }
            }
        }
    }

    /**
     * Update the values in {@link #mExpandedEditors}.
     */
    private void updatedExpandedEditorsMap() {
        for (int i = 0; i < mContent.getChildCount(); i++) {
            final View childView = mContent.getChildAt(i);
            if (childView instanceof BaseRawContactEditorView) {
                BaseRawContactEditorView childEditor = (BaseRawContactEditorView) childView;
                mExpandedEditors.put(childEditor.getRawContactId(), childEditor.isCollapsed());
            }
        }
    }

    /**
     * If we've stashed a temporary file containing a contact's new photo, return its URI.
     * @param rawContactId identifies the raw-contact whose Bitmap we'll try to return.
     * @return Uru of photo for specified raw-contact, or null
     */
    private Uri updatedPhotoUriForRawContact(long rawContactId) {
        return (Uri) mUpdatedPhotos.get(String.valueOf(rawContactId));
    }

    private void bindPhotoHandler(BaseRawContactEditorView editor, AccountType type,
            RawContactDeltaList state) {
        final int mode;
        boolean showIsPrimaryOption;
        if (type.areContactsWritable()) {
            if (editor.hasSetPhoto()) {
                mode = PhotoActionPopup.Modes.WRITE_ABLE_PHOTO;
                showIsPrimaryOption = hasMoreThanOnePhoto();
            } else {
                mode = PhotoActionPopup.Modes.NO_PHOTO;
                showIsPrimaryOption = false;
            }
        } else if (editor.hasSetPhoto() && hasMoreThanOnePhoto()) {
            mode = PhotoActionPopup.Modes.READ_ONLY_PHOTO;
            showIsPrimaryOption = true;
        } else {
            // Read-only and either no photo or the only photo ==> no options
            editor.getPhotoEditor().setEditorListener(null);
            editor.getPhotoEditor().setShowPrimary(false);
            return;
        }
        if (mRawContactIdToDisplayAlone != -1) {
            showIsPrimaryOption = false;
        }
        final PhotoHandler photoHandler = new PhotoHandler(mContext, editor, mode, state);
        editor.getPhotoEditor().setEditorListener(
                (PhotoHandler.PhotoEditorListener) photoHandler.getListener());
        editor.getPhotoEditor().setShowPrimary(showIsPrimaryOption);

        // Note a newly created raw contact gets some random negative ID, so any value is valid
        // here. (i.e. don't check against -1 or anything.)
        if (mRawContactIdRequestingPhoto == editor.getRawContactId()) {
            mCurrentPhotoHandler = photoHandler;
        }
    }

    private void addAccountSwitcher(
            final RawContactDelta currentState, BaseRawContactEditorView editor) {
        final AccountWithDataSet currentAccount = new AccountWithDataSet(
                currentState.getAccountName(),
                currentState.getAccountType(),
                currentState.getDataSet());
        final View accountView = editor.findViewById(R.id.account);
        final View anchorView = editor.findViewById(R.id.account_selector_container);
        if (accountView == null) {
            return;
        }
        anchorView.setVisibility(View.VISIBLE);
        accountView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                final ListPopupWindow popup = new ListPopupWindow(mContext, null);
                final AccountsListAdapter adapter =
                        new AccountsListAdapter(mContext,
                        AccountListFilter.ACCOUNTS_CONTACT_WRITABLE, currentAccount);
                popup.setWidth(anchorView.getWidth());
                popup.setAnchorView(anchorView);
                popup.setAdapter(adapter);
                popup.setModal(true);
                popup.setInputMethodMode(ListPopupWindow.INPUT_METHOD_NOT_NEEDED);
                popup.setOnItemClickListener(new AdapterView.OnItemClickListener() {
                    @Override
                    public void onItemClick(AdapterView<?> parent, View view, int position,
                            long id) {
                        UiClosables.closeQuietly(popup);
                        AccountWithDataSet newAccount = adapter.getItem(position);
                        if (!newAccount.equals(currentAccount)) {
                            rebindEditorsForNewContact(currentState, currentAccount, newAccount);
                        }
                    }
                });
                popup.show();
            }
        });
    }

    @Override
    protected boolean doSaveAction(int saveMode, Long joinContactId) {
        final Intent intent = ContactSaveService.createSaveContactIntent(mContext, mState,
                SAVE_MODE_EXTRA_KEY, saveMode, isEditingUserProfile(),
                ((Activity) mContext).getClass(), ContactEditorActivity.ACTION_SAVE_COMPLETED,
                mUpdatedPhotos, JOIN_CONTACT_ID_EXTRA_KEY, joinContactId);
        return startSaveService(mContext, intent, saveMode);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        outState.putSerializable(KEY_EXPANDED_EDITORS, mExpandedEditors);
        outState.putLong(KEY_RAW_CONTACT_ID_REQUESTING_PHOTO, mRawContactIdRequestingPhoto);
        outState.putParcelable(KEY_CURRENT_PHOTO_URI, mCurrentPhotoUri);
        outState.putParcelable(KEY_UPDATED_PHOTOS, mUpdatedPhotos);
        outState.putLong(ContactEditorBaseFragment.INTENT_EXTRA_RAW_CONTACT_ID_TO_DISPLAY_ALONE,
                mRawContactIdToDisplayAlone);
        super.onSaveInstanceState(outState);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (mStatus == Status.SUB_ACTIVITY) {
            mStatus = Status.EDITING;
        }

        // See if the photo selection handler handles this result.
        if (mCurrentPhotoHandler != null && mCurrentPhotoHandler.handlePhotoActivityResult(
                requestCode, resultCode, data)) {
            return;
        }

        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    protected void joinAggregate(final long contactId) {
        final Intent intent = ContactSaveService.createJoinContactsIntent(
                mContext, mContactIdForJoin, contactId, ContactEditorActivity.class,
                ContactEditorActivity.ACTION_JOIN_COMPLETED);
        mContext.startService(intent);
    }

    /**
     * Sets the photo stored in mPhoto and writes it to the RawContact with the given id
     */
    private void setPhoto(long rawContact, Bitmap photo, Uri photoUri) {
        BaseRawContactEditorView requestingEditor = getRawContactEditorView(rawContact);

        if (photo == null || photo.getHeight() <= 0 || photo.getWidth() <= 0) {
            // This is unexpected.
            Log.w(TAG, "Invalid bitmap passed to setPhoto()");
        }

        if (requestingEditor != null) {
            requestingEditor.setPhotoEntry(photo);
            // Immediately set all other photos as non-primary. Otherwise the UI can display
            // multiple photos as "Primary photo".
            for (int i = 0; i < mContent.getChildCount(); i++) {
                final View childView = mContent.getChildAt(i);
                if (childView instanceof BaseRawContactEditorView
                        && childView != requestingEditor) {
                    final BaseRawContactEditorView rawContactEditor
                            = (BaseRawContactEditorView) childView;
                    rawContactEditor.getPhotoEditor().setSuperPrimary(false);
                }
            }
        } else {
            Log.w(TAG, "The contact that requested the photo is no longer present.");
        }

        mUpdatedPhotos.putParcelable(String.valueOf(rawContact), photoUri);
    }

    /**
     * Finds raw contact editor view for the given rawContactId.
     */
    @Override
    protected View getAggregationAnchorView(long rawContactId) {
        BaseRawContactEditorView editorView = getRawContactEditorView(rawContactId);
        return editorView == null ? null : editorView.findViewById(R.id.anchor_view);
    }

    public BaseRawContactEditorView getRawContactEditorView(long rawContactId) {
        for (int i = 0; i < mContent.getChildCount(); i++) {
            final View childView = mContent.getChildAt(i);
            if (childView instanceof BaseRawContactEditorView) {
                final BaseRawContactEditorView editor = (BaseRawContactEditorView) childView;
                if (editor.getRawContactId() == rawContactId) {
                    return editor;
                }
            }
        }
        return null;
    }

    /**
     * Returns true if there is currently more than one photo on screen.
     */
    private boolean hasMoreThanOnePhoto() {
        int countWithPicture = 0;
        final int numEntities = mState.size();
        for (int i = 0; i < numEntities; i++) {
            final RawContactDelta entity = mState.get(i);
            if (entity.isVisible()) {
                final ValuesDelta primary = entity.getPrimaryEntry(Photo.CONTENT_ITEM_TYPE);
                if (primary != null && primary.getPhoto() != null) {
                    countWithPicture++;
                } else {
                    final long rawContactId = entity.getRawContactId();
                    final Uri uri = mUpdatedPhotos.getParcelable(String.valueOf(rawContactId));
                    if (uri != null) {
                        try {
                            mContext.getContentResolver().openInputStream(uri);
                            countWithPicture++;
                        } catch (FileNotFoundException e) {
                        }
                    }
                }

                if (countWithPicture > 1) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * SPRD:Bug494336 Need READ_EXTERNAL_STORAGE permission to pick photo.
     * @{
     */
    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[],
            int[] grantResults) {
        if (requestCode == READ_EXTERNAL_STORAGE_REQUESTCODE && permissions != null
                && permissions.length == 1 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            if(mCurrentPhotoHandler != null) {
                // mCurrentPhotoHandler.getListener().doOnPickFromGalleryChosen();
            }
        } else {
            Toast.makeText(getContext(), R.string.missing_required_permission, Toast.LENGTH_SHORT).show();
        }
    }
    /**
     * @}
     */

    /**
     * Custom photo handler for the editor.  The inner listener that this creates also has a
     * reference to the editor and acts as an {@link EditorListener}, and uses that editor to hold
     * state information in several of the listener methods.
     */
    private final class PhotoHandler extends PhotoSelectionHandler {

        final long mRawContactId;
        private final BaseRawContactEditorView mEditor;
        private final PhotoActionListener mPhotoEditorListener;

        public PhotoHandler(Context context, BaseRawContactEditorView editor, int photoMode,
                RawContactDeltaList state) {
            super(context, editor.getPhotoEditor().getChangeAnchorView(), photoMode, false, state);
            mEditor = editor;
            mRawContactId = editor.getRawContactId();
            mPhotoEditorListener = new PhotoEditorListener();
        }

        @Override
        public PhotoActionListener getListener() {
            return mPhotoEditorListener;
        }

        @Override
        public void startPhotoActivity(Intent intent, int requestCode, Uri photoUri) {
            mRawContactIdRequestingPhoto = mEditor.getRawContactId();
            mCurrentPhotoHandler = this;
            mStatus = Status.SUB_ACTIVITY;
            mCurrentPhotoUri = photoUri;
//          ContactEditorFragment.this.startActivityForResult(intent, requestCode); //orign code

            //SPRD: Bug613046 Contacts crash due to fragment-destroyed
            if (!checkFragmentDetached()){
                ContactEditorFragment.this.startActivityForResult(intent, requestCode);
            }
        }

        private final class PhotoEditorListener extends PhotoSelectionHandler.PhotoActionListener
                implements EditorListener {

            @Override
            public void onRequest(int request) {
                if (!hasValidState()) return;

                if (request == EditorListener.REQUEST_PICK_PHOTO) {
                    onClick(mEditor.getPhotoEditor());
                }
                if (request == EditorListener.REQUEST_PICK_PRIMARY_PHOTO) {
                    useAsPrimaryChosen();
                }
            }

            @Override
            public void onDeleteRequested(Editor removedEditor) {
                // The picture cannot be deleted, it can only be removed, which is handled by
                // onRemovePictureChosen()
            }

            /**
             * User has chosen to set the selected photo as the (super) primary photo
             */
            public void useAsPrimaryChosen() {
                // Set the IsSuperPrimary for each editor
                int count = mContent.getChildCount();
                for (int i = 0; i < count; i++) {
                    final View childView = mContent.getChildAt(i);
                    if (childView instanceof BaseRawContactEditorView) {
                        final BaseRawContactEditorView editor =
                                (BaseRawContactEditorView) childView;
                        final PhotoEditorView photoEditor = editor.getPhotoEditor();
                        photoEditor.setSuperPrimary(editor == mEditor);
                    }
                }
                bindEditors();
            }

            /**
             * User has chosen to remove a picture
             */
            @Override
            public void onRemovePictureChosen() {
//                mEditor.setPhotoEntry(null);    //orign code
//
//                // sprd Bug521129 For inserts where the raw contact ID is a negative number, we must clear
//                // any previously saved full resolution photos under negative raw contact IDs so that the
//                // compact editor will use the newly selected photo, instead of an old one.
//                if (isInsert(getActivity().getIntent()) && mRawContactId < 0) {
//                    removeNewRawContactPhotos();
//                }
//                // Prevent bitmap from being restored if rotate the device.
//                // (only if we first chose a new photo before removing it)
//                mUpdatedPhotos.remove(String.valueOf(mRawContactId));
//                bindEditors();

                //SPRD: Bug613046 Contacts crash due to fragment-destroyed
                if (!checkFragmentDetached()){
                    mEditor.setPhotoEntry(null);

                    // sprd Bug521129 For inserts where the raw contact ID is a negative number, we must clear
                    // any previously saved full resolution photos under negative raw contact IDs so that the
                    // compact editor will use the newly selected photo, instead of an old one.
                    if (isInsert(getActivity().getIntent()) && mRawContactId < 0) {
                        removeNewRawContactPhotos();
                    }
                    // Prevent bitmap from being restored if rotate the device.
                    // (only if we first chose a new photo before removing it)
                    mUpdatedPhotos.remove(String.valueOf(mRawContactId));
                    bindEditors();
                }
            }

            @Override
            public void onPhotoSelected(Uri uri) throws FileNotFoundException {
                final Bitmap bitmap = ContactPhotoUtils.getBitmapFromUri(mContext, uri);
                setPhoto(mRawContactId, bitmap, uri);
                mCurrentPhotoHandler = null;
                bindEditors();
            }

            @Override
            public Uri getCurrentPhotoUri() {
                return mCurrentPhotoUri;
            }

            @Override
            public void onPhotoSelectionDismissed() {
                // Nothing to do.
            }

            /**
             * SPRD:Bug494336 Need READ_EXTERNAL_STORAGE permission to pick photo.
             * @{
             */
            @Override
            public void onPickFromGalleryChosen() {
//                if(checkPermission()) {                    //orign code
//                    super.onPickFromGalleryChosen();
//                } else {
//                    requestReadStoragePermission();
//                }

                //SPRD: Bug613046 Contacts crash due to fragment-destroyed
                if (!checkFragmentDetached()) {
                    if(checkPermission()){
                        super.onPickFromGalleryChosen();
                    } else {
                        requestReadStoragePermission();
                    }
                }
            }

//            @Override
//            public void doOnPickFromGalleryChosen() {
//                // super.onPickFromGalleryChosen();
//            }
            /**
             * @}
             */
        }
    }
    /**
     * SPRD: Bug613046 Contacts crash when click the button in fragment which is destroying
     * by long-click RECENT_APP_KEY to cancle multi-window
     * @{
     */
    public boolean checkFragmentDetached(){
        if(ContactEditorFragment.this.isDetached()
           || ContactEditorFragment.this.getHost() == null){
            return true;
        }
        return false;
    }
    /**
     * @}
     */
}
