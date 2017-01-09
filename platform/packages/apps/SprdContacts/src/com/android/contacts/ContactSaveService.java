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

package com.android.contacts;

import static android.Manifest.permission.WRITE_CONTACTS;
import android.app.Activity;
import android.app.IntentService;
import android.content.ContentProviderOperation;
import android.content.ContentProviderOperation.Builder;
import android.content.ContentProviderResult;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.OperationApplicationException;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Parcelable;
import android.os.RemoteException;
import android.provider.ContactsContract;
import android.provider.ContactsContract.AggregationExceptions;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.CommonDataKinds.Photo;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.Groups;
import android.provider.ContactsContract.PinnedPositions;
import android.provider.ContactsContract.Profile;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.RawContactsEntity;
import android.util.Log;
import android.widget.Toast;

import com.android.contacts.activities.ContactEditorBaseActivity;
import com.android.contacts.activities.PeopleActivity;
import com.android.contacts.common.compat.CompatUtils;
import com.android.contacts.common.database.ContactUpdateUtils;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.CPOWrapper;
import com.android.contacts.common.model.RawContactDelta;
import com.android.contacts.common.model.RawContactDeltaList;
import com.android.contacts.common.model.RawContactModifier;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.util.PermissionsUtil;
import com.android.contacts.compat.PinnedPositionsCompat;
import com.android.contacts.activities.ContactEditorBaseActivity.ContactEditor.SaveMode;
import com.android.contacts.util.ContactPhotoUtils;

import com.google.common.collect.Lists;
import com.google.common.collect.Sets;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import com.android.contacts.editor.GroupMembershipView;
/**
 * SPRD:
 * @{
 */
import com.android.contacts.common.util.Constants;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.NotificationManager;
import android.support.v4.app.NotificationCompat;
import com.android.internal.telephony.IccPBForMimetypeException;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.Email;
import com.android.internal.telephony.IccPBForRecordException;
import android.database.sqlite.SQLiteFullException;
import android.database.sqlite.SQLiteDiskIOException;
import android.database.sqlite.SQLiteException;
import android.os.UserManager;
import android.os.UserHandle;
import android.os.SystemProperties;
/**
 * @}
 */
/**
 * SPRD:Bug497538 Bug503335 while do some batch operation after long click,
 * long click the batch operation contacts and share it, Contacts app FC.
 *
 * @{
 */
import com.android.contacts.interactions.ContactMultiDeletionInteraction;
import com.android.contacts.interactions.JoinContactsDialogFragment;
/**
 * @}
 */
/**
 * A service responsible for saving changes to the content provider.
 */
public class ContactSaveService extends IntentService {
    private static final String TAG = "ContactSaveService";

    /** Set to true in order to view logs on content provider operations */
	// private static final boolean DEBUG = false;
    /** sprd Bug519468 */
    private static final boolean DEBUG = !SystemProperties.get("ro.build.type").equals("user");

    public static final String ACTION_NEW_RAW_CONTACT = "newRawContact";

    public static final String EXTRA_ACCOUNT_NAME = "accountName";
    public static final String EXTRA_ACCOUNT_TYPE = "accountType";
    public static final String EXTRA_DATA_SET = "dataSet";
    public static final String EXTRA_CONTENT_VALUES = "contentValues";
    public static final String EXTRA_CALLBACK_INTENT = "callbackIntent";

    public static final String ACTION_SAVE_CONTACT = "saveContact";
    public static final String EXTRA_CONTACT_STATE = "state";
    public static final String EXTRA_SAVE_MODE = "saveMode";
    public static final String EXTRA_SAVE_IS_PROFILE = "saveIsProfile";
    public static final String EXTRA_SAVE_SUCCEEDED = "saveSucceeded";
    public static final String EXTRA_UPDATED_PHOTOS = "updatedPhotos";
    public static final String EXTRA_SHOULD_DELETE = "shouldDelete";
    public static final String ACTION_CREATE_GROUP = "createGroup";
    public static final String ACTION_RENAME_GROUP = "renameGroup";
    public static final String ACTION_DELETE_GROUP = "deleteGroup";
    public static final String ACTION_UPDATE_GROUP = "updateGroup";
    public static final String EXTRA_GROUP_ID = "groupId";
    public static final String EXTRA_GROUP_LABEL = "groupLabel";
    public static final String EXTRA_RAW_CONTACTS_TO_ADD = "rawContactsToAdd";
    public static final String EXTRA_RAW_CONTACTS_TO_REMOVE = "rawContactsToRemove";

    public static final String ACTION_SET_STARRED = "setStarred";
    public static final String ACTION_DELETE_CONTACT = "delete";
    public static final String ACTION_DELETE_MULTIPLE_CONTACTS = "deleteMultipleContacts";
    public static final String EXTRA_CONTACT_URI = "contactUri";
    public static final String EXTRA_CONTACT_IDS = "contactIds";
    public static final String EXTRA_STARRED_FLAG = "starred";

    public static final String ACTION_SET_SUPER_PRIMARY = "setSuperPrimary";
    public static final String ACTION_CLEAR_PRIMARY = "clearPrimary";
    public static final String EXTRA_DATA_ID = "dataId";

    public static final String ACTION_JOIN_CONTACTS = "joinContacts";
    public static final String ACTION_JOIN_SEVERAL_CONTACTS = "joinSeveralContacts";
    public static final String EXTRA_CONTACT_ID1 = "contactId1";
    public static final String EXTRA_CONTACT_ID2 = "contactId2";

    public static final String ACTION_SET_SEND_TO_VOICEMAIL = "sendToVoicemail";
    public static final String EXTRA_SEND_TO_VOICEMAIL_FLAG = "sendToVoicemailFlag";

    public static final String ACTION_SET_RINGTONE = "setRingtone";
    public static final String EXTRA_CUSTOM_RINGTONE = "customRingtone";
    /**
     * SPRD:Bug422864 Add move group feature
     *
     * @{
     */
    public static final String ACTION_MOVE_GROUP = "moveGroup";
    public static final String EXTRA_SRC_GROUP_ID = "srcgroupId";
    public static final String EXTRA_RAW_CONTACTS_TO_MOVE = "rawContactsToMove";
    /**
    * @}
    */
    private static final HashSet<String> ALLOWED_DATA_COLUMNS = Sets.newHashSet(
        Data.MIMETYPE,
        Data.IS_PRIMARY,
        Data.DATA1,
        Data.DATA2,
        Data.DATA3,
        Data.DATA4,
        Data.DATA5,
        Data.DATA6,
        Data.DATA7,
        Data.DATA8,
        Data.DATA9,
        Data.DATA10,
        Data.DATA11,
        Data.DATA12,
        Data.DATA13,
        Data.DATA14,
        Data.DATA15
    );

    private static final int PERSIST_TRIES = 3;

    private static final int MAX_CONTACTS_PROVIDER_BATCH_SIZE = 499;

    public interface Listener {
        public void onServiceCompleted(Intent callbackIntent);
    }

        /**
         * SPRD:513110 Contact list disapper while rotate during join contact
         *
         * @{
         */
        private static JoinContactListener mJoinContactListener;

        public interface JoinContactListener {
               public void onJoinStart();

               public void onJoinFinished();
        }
        //SPRD:517837 join contacts and rotate the phone,contact FC
        public static void setJoinContactListener(JoinContactListener listener) {
                mJoinContactListener = listener;
        }

        /**
         * @}
         */
    private static final CopyOnWriteArrayList<Listener> sListeners =
            new CopyOnWriteArrayList<Listener>();

    private Handler mMainHandler;
    /**
    * SPRD:Bug422864 Add move group feature
    *
    * @{
    */
    /** Identifies the number of Favorite contacts */
    private static UpdateGroupNotification mUpdateGroupNotification;
    public static boolean mIsGroupSaving = false;

    public static final String EXTRA_ERROR_TOAST = "errorToast";
    /**
    * @}
    */

    /**
     * SPRD:Bug503080 Sync modify the User profile info.
     * @{
     */
    private UserManager mUserManager;
    /**
     * @}
     */
    public ContactSaveService() {
        super(TAG);
        setIntentRedelivery(true);
        mMainHandler = new Handler(Looper.getMainLooper());
    }

    public static void registerListener(Listener listener) {
        if (!(listener instanceof Activity)) {
            throw new ClassCastException("Only activities can be registered to"
                    + " receive callback from " + ContactSaveService.class.getName());
        }
        sListeners.add(0, listener);
    }

    public static void unregisterListener(Listener listener) {
        sListeners.remove(listener);
    }

    /**
     * Returns true if the ContactSaveService was started successfully and false if an exception
     * was thrown and a Toast error message was displayed.
     */
    public static boolean startService(Context context, Intent intent, int saveMode) {
        try {
            context.startService(intent);
        } catch (Exception exception) {
            final int resId;
            switch (saveMode) {
                case ContactEditorBaseActivity.ContactEditor.SaveMode.SPLIT:
                    resId = R.string.contactUnlinkErrorToast;
                    break;
                case ContactEditorBaseActivity.ContactEditor.SaveMode.RELOAD:
                    resId = R.string.contactJoinErrorToast;
                    break;
                case ContactEditorBaseActivity.ContactEditor.SaveMode.CLOSE:
                    resId = R.string.contactSavedErrorToast;
                    break;
                default:
                    resId = R.string.contactGenericErrorToast;
            }
            Toast.makeText(context, resId, Toast.LENGTH_SHORT).show();
            return false;
        }
        return true;
    }

    /**
     * Utility method that starts service and handles exception.
     */
    public static void startService(Context context, Intent intent) {
        try {
            context.startService(intent);
        } catch (Exception exception) {
            Toast.makeText(context, R.string.contactGenericErrorToast, Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    public Object getSystemService(String name) {
        Object service = super.getSystemService(name);
        if (service != null) {
            return service;
        }

        return getApplicationContext().getSystemService(name);
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        if (intent == null) {
            Log.d(TAG, "onHandleIntent: could not handle null intent");
            return;
        }
        if (!PermissionsUtil.hasPermission(this, WRITE_CONTACTS)) {
            Log.w(TAG, "No WRITE_CONTACTS permission, unable to write to CP2");
            // TODO: add more specific error string such as "Turn on Contacts
            // permission to update your contacts"
            showToast(R.string.contactSavedErrorToast);
            return;
        }

        // Call an appropriate method. If we're sure it affects how incoming phone calls are
        // handled, then notify the fact to in-call screen.
        String action = intent.getAction();
        if (ACTION_NEW_RAW_CONTACT.equals(action)) {
            createRawContact(intent);
        } else if (ACTION_SAVE_CONTACT.equals(action)) {
            saveContact(intent);
        } else if (ACTION_CREATE_GROUP.equals(action)) {
            createGroup(intent);
        } else if (ACTION_RENAME_GROUP.equals(action)) {
            renameGroup(intent);
        } else if (ACTION_DELETE_GROUP.equals(action)) {
            deleteGroup(intent);
        } else if (ACTION_UPDATE_GROUP.equals(action)) {
            updateGroup(intent);
        /**
         * SPRD:bug422864 Add move group feature.
         *
         * @{
         */
        } else if (ACTION_MOVE_GROUP.equals(action)){
            moveGroup(intent);
        /**
         * @}
         */
        } else if (ACTION_SET_STARRED.equals(action)) {
            setStarred(intent);
        } else if (ACTION_SET_SUPER_PRIMARY.equals(action)) {
            setSuperPrimary(intent);
        } else if (ACTION_CLEAR_PRIMARY.equals(action)) {
            clearPrimary(intent);
        } else if (ACTION_DELETE_MULTIPLE_CONTACTS.equals(action)) {
            deleteMultipleContacts(intent);
        } else if (ACTION_DELETE_CONTACT.equals(action)) {
            deleteContact(intent);
        } else if (ACTION_JOIN_CONTACTS.equals(action)) {
            joinContacts(intent);
        } else if (ACTION_JOIN_SEVERAL_CONTACTS.equals(action)) {
            joinSeveralContacts(intent);
           /**
            * SPRD:Bug497538 Bug503335 while do some batch operation after long click,
            * long click the batch operation contacts and share it, Contacts app FC.
            *
            * @{
            */
            JoinContactsDialogFragment.isContactsJoining = false;
            /**
             * @}
             */
        } else if (ACTION_SET_SEND_TO_VOICEMAIL.equals(action)) {
            setSendToVoicemail(intent);
        } else if (ACTION_SET_RINGTONE.equals(action)) {
            setRingtone(intent);
        }
    }

    /**
     * Creates an intent that can be sent to this service to create a new raw contact
     * using data presented as a set of ContentValues.
     */
    public static Intent createNewRawContactIntent(Context context,
            ArrayList<ContentValues> values, AccountWithDataSet account,
            Class<? extends Activity> callbackActivity, String callbackAction) {
        Intent serviceIntent = new Intent(
                context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_NEW_RAW_CONTACT);
        if (account != null) {
            serviceIntent.putExtra(ContactSaveService.EXTRA_ACCOUNT_NAME, account.name);
            serviceIntent.putExtra(ContactSaveService.EXTRA_ACCOUNT_TYPE, account.type);
            serviceIntent.putExtra(ContactSaveService.EXTRA_DATA_SET, account.dataSet);
        }
        serviceIntent.putParcelableArrayListExtra(
                ContactSaveService.EXTRA_CONTENT_VALUES, values);

        // Callback intent will be invoked by the service once the new contact is
        // created.  The service will put the URI of the new contact as "data" on
        // the callback intent.
        Intent callbackIntent = new Intent(context, callbackActivity);
        callbackIntent.setAction(callbackAction);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CALLBACK_INTENT, callbackIntent);
        return serviceIntent;
    }

    private void createRawContact(Intent intent) {
        String accountName = intent.getStringExtra(EXTRA_ACCOUNT_NAME);
        String accountType = intent.getStringExtra(EXTRA_ACCOUNT_TYPE);
        String dataSet = intent.getStringExtra(EXTRA_DATA_SET);
        List<ContentValues> valueList = intent.getParcelableArrayListExtra(EXTRA_CONTENT_VALUES);
        Intent callbackIntent = intent.getParcelableExtra(EXTRA_CALLBACK_INTENT);

        ArrayList<ContentProviderOperation> operations = new ArrayList<ContentProviderOperation>();
        operations.add(ContentProviderOperation.newInsert(RawContacts.CONTENT_URI)
                .withValue(RawContacts.ACCOUNT_NAME, accountName)
                .withValue(RawContacts.ACCOUNT_TYPE, accountType)
                .withValue(RawContacts.DATA_SET, dataSet)
                .build());

        int size = valueList.size();
        for (int i = 0; i < size; i++) {
            ContentValues values = valueList.get(i);
            values.keySet().retainAll(ALLOWED_DATA_COLUMNS);
            operations.add(ContentProviderOperation.newInsert(Data.CONTENT_URI)
                    .withValueBackReference(Data.RAW_CONTACT_ID, 0)
                    .withValues(values)
                    .build());
        }

        ContentResolver resolver = getContentResolver();
        ContentProviderResult[] results;
        try {
            results = resolver.applyBatch(ContactsContract.AUTHORITY, operations);
        } catch (Exception e) {
            throw new RuntimeException("Failed to store new contact", e);
        }

        Uri rawContactUri = results[0].uri;
        callbackIntent.setData(RawContacts.getContactLookupUri(resolver, rawContactUri));

        deliverCallback(callbackIntent);
    }

    /**
     * Creates an intent that can be sent to this service to create a new raw contact
     * using data presented as a set of ContentValues.
     * This variant is more convenient to use when there is only one photo that can
     * possibly be updated, as in the Contact Details screen.
     * @param rawContactId identifies a writable raw-contact whose photo is to be updated.
     * @param updatedPhotoPath denotes a temporary file containing the contact's new photo.
     */
    public static Intent createSaveContactIntent(Context context, RawContactDeltaList state,
            String saveModeExtraKey, int saveMode, boolean isProfile,
            Class<? extends Activity> callbackActivity, String callbackAction, long rawContactId,
            Uri updatedPhotoPath) {
        Bundle bundle = new Bundle();
        bundle.putParcelable(String.valueOf(rawContactId), updatedPhotoPath);
        return createSaveContactIntent(context, state, saveModeExtraKey, saveMode, isProfile,
                callbackActivity, callbackAction, bundle,
                /* joinContactIdExtraKey */ null, /* joinContactId */ null);
    }

    /**
     * Creates an intent that can be sent to this service to create a new raw contact
     * using data presented as a set of ContentValues.
     * This variant is used when multiple contacts' photos may be updated, as in the
     * Contact Editor.
     * @param updatedPhotos maps each raw-contact's ID to the file-path of the new photo.
     * @param joinContactIdExtraKey the key used to pass the joinContactId in the callback intent.
     * @param joinContactId the raw contact ID to join to the contact after doing the save.
     */
    public static Intent createSaveContactIntent(Context context, RawContactDeltaList state,
            String saveModeExtraKey, int saveMode, boolean isProfile,
            Class<? extends Activity> callbackActivity, String callbackAction,
            Bundle updatedPhotos, String joinContactIdExtraKey, Long joinContactId) {
        Intent serviceIntent = new Intent(
                context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_SAVE_CONTACT);
        serviceIntent.putExtra(EXTRA_CONTACT_STATE, (Parcelable) state);
        serviceIntent.putExtra(EXTRA_SAVE_IS_PROFILE, isProfile);
        serviceIntent.putExtra(EXTRA_SAVE_MODE, saveMode);

        if (updatedPhotos != null) {
            serviceIntent.putExtra(EXTRA_UPDATED_PHOTOS, (Parcelable) updatedPhotos);
        }

        if (callbackActivity != null) {
            // Callback intent will be invoked by the service once the contact is
            // saved.  The service will put the URI of the new contact as "data" on
            // the callback intent.
            Intent callbackIntent = new Intent(context, callbackActivity);
            callbackIntent.putExtra(saveModeExtraKey, saveMode);
            if (joinContactIdExtraKey != null && joinContactId != null) {
                callbackIntent.putExtra(joinContactIdExtraKey, joinContactId);
            }
            callbackIntent.setAction(callbackAction);
            serviceIntent.putExtra(ContactSaveService.EXTRA_CALLBACK_INTENT, callbackIntent);
        }
        return serviceIntent;
    }

    private void saveContact(Intent intent) {
        RawContactDeltaList state = intent.getParcelableExtra(EXTRA_CONTACT_STATE);
        boolean isProfile = intent.getBooleanExtra(EXTRA_SAVE_IS_PROFILE, false);
        Bundle updatedPhotos = intent.getParcelableExtra(EXTRA_UPDATED_PHOTOS);

        if (state == null) {
            Log.e(TAG, "Invalid arguments for saveContact request");
            return;
        }

        int saveMode = intent.getIntExtra(EXTRA_SAVE_MODE, -1);
        // Trim any empty fields, and RawContacts, before persisting
        final AccountTypeManager accountTypes = AccountTypeManager.getInstance(this);
        RawContactModifier.trimEmpty(state, accountTypes);

        Uri lookupUri = null;

        final ContentResolver resolver = getContentResolver();
        boolean succeeded = false;

        // Keep track of the id of a newly raw-contact (if any... there can be at most one).
        long insertedRawContactId = -1;

        // Attempt to persist changes
        int tries = 0;
        /**
         * SPRD:
         * @{
         */
        int toastId = 0;
        /**
         * @}
         */
        boolean shouldDeleteStatus = false;
        while (tries++ < PERSIST_TRIES) {
            try {
                // Build operations and try applying
                final ArrayList<CPOWrapper> diffWrapper = state.buildDiffWrapper();

                final ArrayList<ContentProviderOperation> diff = Lists.newArrayList();

                for (CPOWrapper cpoWrapper : diffWrapper) {
                    diff.add(cpoWrapper.getOperation());
                }

                if (DEBUG) {
                    Log.v(TAG, "Content Provider Operations:");
                    for (ContentProviderOperation operation : diff) {
                        Log.v(TAG, operation.toString());
                    }
                }
                changeOrderForSim(diff);
                int numberProcessed = 0;
                boolean batchFailed = false;
                final ContentProviderResult[] results = new ContentProviderResult[diff.size()];
                while (numberProcessed < diff.size()) {
                    final int subsetCount = applyDiffSubset(diff, numberProcessed, results, resolver);
                    if (subsetCount == -1) {
                        Log.w(TAG, "Resolver.applyBatch failed in saveContacts");
                        batchFailed = true;
                        break;
                    } else {
                        numberProcessed += subsetCount;
                    }
                }

                if (batchFailed) {
                    // Retry save
                    continue;
                }

                final long rawContactId = getRawContactId(state, diffWrapper, results);
                if (rawContactId == -1) {
                    /**
                     * sprd Bug517292
                     * Orig:
                    throw new IllegalStateException("Could not determine RawContact ID after save");
                     * @{
                     */
                    Log.e(TAG, "IllegalStateException(Could not determine RawContact ID after save)");
                    return;
                    /**
                     * @}
                     */
                }
                // SPRD:Bug 536152 Check if the rawContact has data value or not,
                // if no data found, delete it in table raw_contacts.
                if (deleteNoDataRawContact(resolver, rawContactId)){
                    shouldDeleteStatus = true;
                }
                // We don't have to check to see if the value is still -1.  If we reach here,
                // the previous loop iteration didn't succeed, so any ID that we obtained is bogus.
                insertedRawContactId = getInsertedRawContactId(diffWrapper, results);
                if (isProfile) {
                    // Since the profile supports local raw contacts, which may have been completely
                    // removed if all information was removed, we need to do a special query to
                    // get the lookup URI for the profile contact (if it still exists).
                    Cursor c = resolver.query(Profile.CONTENT_URI,
                            new String[] {Contacts._ID, Contacts.LOOKUP_KEY, Contacts.DISPLAY_NAME},
                            null, null, null);
                    if (c == null) {
                        continue;
                    }
                    /**
                     * SPRD:Bug503080 Sync modify the User profile info.
                     * @{
                     */
                    String displayName = "";
                    /**
                     * @}
                     */
                    try {
                        if (c.moveToFirst()) {
                            final long contactId = c.getLong(0);
                            final String lookupKey = c.getString(1);
                            /**
                             * SPRD:Bug503080 Sync modify the User profile info.
                             * @{
                             */
                            displayName = c.getString(2);
                            /**
                             * @}
                             */
                            lookupUri = Contacts.getLookupUri(contactId, lookupKey);
                        }
                    } finally {
                        c.close();
                    }
                    /**
                     * SPRD:Bug503080 Sync modify the User profile info.
                     * @{
                     */
                    if(mUserManager == null) {
                        mUserManager = (UserManager) getSystemService(Context.USER_SERVICE);
                    }
                    mUserManager.setUserName(UserHandle.myUserId(), displayName);
                    /**
                     * @}
                     */
                } else {
                    final Uri rawContactUri = ContentUris.withAppendedId(RawContacts.CONTENT_URI,
                                    rawContactId);
                    lookupUri = RawContacts.getContactLookupUri(resolver, rawContactUri);
                }
                if (lookupUri != null) {
                    Log.v(TAG, "Saved contact. New URI: " + lookupUri);
                }

                // We can change this back to false later, if we fail to save the contact photo.
                succeeded = true;
                break;

            } catch (RemoteException e) {
                // Something went wrong, bail without success
                Log.e(TAG, "Problem persisting user edits", e);
                break;

            } catch (IllegalArgumentException e) {
                // This is thrown by applyBatch on malformed requests
                Log.e(TAG, "Problem persisting user edits", e);
                showToast(R.string.contactSavedErrorToast);
                break;

            } catch (OperationApplicationException e) {
                // Version consistency failed, re-parent change and try again
                Log.w(TAG, "Version consistency failed, re-parenting: " + e.toString());
                final StringBuilder sb = new StringBuilder(RawContacts._ID + " IN(");
                boolean first = true;
                final int count = state.size();
                Log.d(TAG, "state.size() : " + count);
                for (int i = 0; i < count; i++) {
                    Long rawContactId = state.getRawContactId(i);
                    if (rawContactId != null && rawContactId != -1) {
                        if (!first) {
                            sb.append(',');
                        }
                        sb.append(rawContactId);
                        first = false;
                    }
                }
                sb.append(")");
                if (DEBUG) {
                    Log.d(TAG, "sb : " + sb.toString());
                }
                if (first) {
                    //throw new IllegalStateException(
                            //"Version consistency failed for a new contact", e);
                    Log.e(TAG, "Version consistency failed for a new contact " + e.getMessage());
                }

                final RawContactDeltaList newState = RawContactDeltaList.fromQuery(
                        isProfile
                                ? RawContactsEntity.PROFILE_CONTENT_URI
                                : RawContactsEntity.CONTENT_URI,
                        resolver, sb.toString(), null, null);
                state = RawContactDeltaList.mergeAfter(newState, state);

                // Update the new state to use profile URIs if appropriate.
                if (isProfile) {
                    for (RawContactDelta delta : state) {
                        delta.setProfileQueryUri();
                    }
                }
            }
        }

        // Now save any updated photos.  We do this at the end to ensure that
        // the ContactProvider already knows about newly-created contacts.
        if (updatedPhotos != null) {
            for (String key : updatedPhotos.keySet()) {
                Uri photoUri = updatedPhotos.getParcelable(key);
                long rawContactId = Long.parseLong(key);

                // If the raw-contact ID is negative, we are saving a new raw-contact;
                // replace the bogus ID with the new one that we actually saved the contact at.
                if (rawContactId < 0) {
                    rawContactId = insertedRawContactId;
                }

                // If the save failed, insertedRawContactId will be -1
                if (rawContactId < 0 || !saveUpdatedPhoto(rawContactId, photoUri, saveMode)) {
                    succeeded = false;
                }
            }
        }

        Intent callbackIntent = intent.getParcelableExtra(EXTRA_CALLBACK_INTENT);
        if (callbackIntent != null) {
            if (succeeded) {
                // Mark the intent to indicate that the save was successful (even if the lookup URI
                // is now null).  For local contacts or the local profile, it's possible that the
                // save triggered removal of the contact, so no lookup URI would exist..
                callbackIntent.putExtra(EXTRA_SAVE_SUCCEEDED, true);
                //add for Bug545540 if there is no data in contacts, save the the value in callbackIntent
                callbackIntent.putExtra(EXTRA_SHOULD_DELETE, shouldDeleteStatus);
            }
            callbackIntent.setData(lookupUri);
            /**
             * SPRD:
             * @{
             */
            callbackIntent.putExtra(EXTRA_ERROR_TOAST, toastId);
            /**
             * @}
             */
            deliverCallback(callbackIntent);
        }
    }

    /**
     * Splits "diff" into subsets based on "MAX_CONTACTS_PROVIDER_BATCH_SIZE", applies each of the
     * subsets, adds the returned array to "results".
     *
     * @return the size of the array, if not null; -1 when the array is null.
     */
    private int applyDiffSubset(ArrayList<ContentProviderOperation> diff, int offset,
            ContentProviderResult[] results, ContentResolver resolver)
            throws RemoteException, OperationApplicationException {
        final int subsetCount = Math.min(diff.size() - offset, MAX_CONTACTS_PROVIDER_BATCH_SIZE);
        final ArrayList<ContentProviderOperation> subset = new ArrayList<>();
        subset.addAll(diff.subList(offset, offset + subsetCount));
	     /**
	     * SPRD:
	     * Orig:
        final ContentProviderResult[] subsetResult = resolver.applyBatch(ContactsContract
                .AUTHORITY, subset);
	     * @{
	     */
		int toastId = 0;
		ContentProviderResult[] subsetResult = null;
	    try {
	        Log.d(TAG, "ContactSaveService resolver.applyBatch");
         subsetResult = resolver.applyBatch(ContactsContract
                .AUTHORITY, subset);
	        if (DEBUG) {
	            Log.d(TAG,"saveContact_applyBatch_ContactsContract.AUTHORITY" + ContactsContract.AUTHORITY);
	        }
            if (subsetResult == null || (offset + subsetResult.length) > results.length) {
                return -1;
            }
            for (ContentProviderResult c : subsetResult) {
                results[offset++] = c;
            }
            return subsetResult.length;
	    } catch (IllegalStateException e) {
	        Log.e(TAG, "ContactSaveService resolver.applyBatch IllegalStateException: " + e.getMessage());
	        e.printStackTrace();
	    } catch (IllegalArgumentException e) {
	        Log.e(TAG, "ContactSaveService resolver.applyBatch IllegalArgumentException: " + e.getMessage());
	        e.printStackTrace();
	    } catch (SQLiteFullException e) {
	        Log.e(TAG, "ContactSaveService: SQLiteFullException");
	        toastId = R.string.sqlite_full;
	    } catch (SQLiteDiskIOException e) {
	        Log.e(TAG, "ContactSaveService: SQLiteDiskIOException");
	    } catch (SQLiteException e) {
	        Log.e(TAG, "ContactSaveService: SQLiteException");
	    } catch (IccPBForRecordException e) {
	        if (Constants.DEBUG)
	            Log.d(TAG, "IccPBForRecordException:" + e.mErrorCode);
	        if (IccPBForRecordException.ADN_RECORD_CAPACITY_FULL == e.mErrorCode) {
	            toastId = R.string.sim_is_full;
	        } else if (IccPBForRecordException.ANR_RECORD_CAPACITY_FULL == e.mErrorCode) {
	            toastId = R.string.anr_is_full;
	        }
	    } catch (IccPBForMimetypeException e) {
	        if (Constants.DEBUG)
	            Log.d(TAG, "IccPBForMimetypeException:" + e.mErrorCode + e.mMimeType);
	        if (e.mErrorCode == IccPBForMimetypeException.CAPACITY_FULL) {
	            if (Email.CONTENT_ITEM_TYPE.equals(e.mMimeType)) {
	                toastId = R.string.contactSavedErrorToastUsimEmailFull;
	            }
	        } else if (e.mErrorCode == IccPBForMimetypeException.OVER_LENGTH_LIMIT) {
	            if (Phone.CONTENT_ITEM_TYPE.equals(e.mMimeType)) {
	                toastId = R.string.over_phone_length_limit;
	            }
	        }
	    }
	    /**
	     * @}
	     */
       if (subsetResult == null || (offset + subsetResult.length) > results.length) {
           return -1;
        }
        for (ContentProviderResult c : subsetResult) {
           results[offset++] = c;
        }
        return subsetResult.length;
    }

    /**
     * Check if the rawContact has data value or not, if no data found, delete it in table
     * raw_contacts.
     *
     * @param resolver
     * @param rawContactId
     */
    private boolean deleteNoDataRawContact(ContentResolver resolver, long rawContactId) {
        Log.d(TAG, " rawcontactId = " + rawContactId);
        Cursor checkExistCursor = null;
        boolean needDelete = false;
        try {
            String selectCondition = Data.RAW_CONTACT_ID + " = " + rawContactId;
            checkExistCursor = resolver.query(Data.CONTENT_URI, new String[] {
                    Data.MIMETYPE, Data.DATA15
            }, selectCondition, null, null);
            if (checkExistCursor == null || checkExistCursor.getCount() == 0) {
                // has no this raw contact,so delete it in raw_contacts
                Log.d(TAG, " has no contact ");
                needDelete = true;
                // if there is only one data item: photo message, then check the photo is deleted or
                // not ,if deleted, then need delete raw_contacts.
            } else if (checkExistCursor.getCount() == 1 && checkExistCursor.moveToFirst()) {
                String mimeType = checkExistCursor.getString(0);
                byte[] photo = checkExistCursor.getBlob(1);
                Log.d(TAG, " has no contact photo = " + photo);
                if (Photo.CONTENT_ITEM_TYPE.equals(mimeType) && (photo == null || "".equals(photo))) {
                    needDelete = true;
                }
            }
            if (needDelete) {
                resolver.delete(RawContacts.CONTENT_URI, RawContacts._ID + " = " + rawContactId,
                        null);
            }
        } catch (Exception e) {
            Log.e(TAG, "delete no exist contact error : " + e.getMessage());
        } finally {
            if (checkExistCursor != null) {
                checkExistCursor.close();
            }
        }
        // add for Bug545540 if there is no data in contacts, delete the contacts by
        // ContactDeletionInteraction
        return needDelete;
    }

    /**
     * Bug 513347,527649,530287 change anr order
     * 
     * @{
     */
    private void changeOrderForSim(final ArrayList<ContentProviderOperation> diff) {
        ContentProviderOperation anrDiff = null;
        ContentProviderOperation deleteDiff = null;
        boolean isAnrDiff = false;
        for (ContentProviderOperation operation : diff) {
            isAnrDiff = (operation.toString() != null) ? (operation.toString()
                    .contains("data2=222") || (operation.toString().contains("data2=7"))) : false;
            if (isAnrDiff == true) {
                anrDiff = operation;
                diff.remove(operation);
                break;
            }
        }
        if (anrDiff != null) {
            diff.add(anrDiff);
        }
        for (ContentProviderOperation operation : diff) {
            if (operation.getType() == ContentProviderOperation.TYPE_DELETE) {
                deleteDiff = operation;
                diff.remove(operation);
                break;
            }
        }
        if (deleteDiff != null) {
            diff.add(deleteDiff);
        }
    }

    /**
     * @}
     */
    /**
     * Save updated photo for the specified raw-contact.
     * @return true for success, false for failure
     */
    private boolean saveUpdatedPhoto(long rawContactId, Uri photoUri, int saveMode) {
        final Uri outputUri = Uri.withAppendedPath(
                ContentUris.withAppendedId(RawContacts.CONTENT_URI, rawContactId),
                RawContacts.DisplayPhoto.CONTENT_DIRECTORY);

        return ContactPhotoUtils.savePhotoFromUriToUri(this, photoUri, outputUri, (saveMode == 0));
    }

    /**
     * Find the ID of an existing or newly-inserted raw-contact.  If none exists, return -1.
     */
    private long getRawContactId(RawContactDeltaList state,
            final ArrayList<CPOWrapper> diffWrapper,
            final ContentProviderResult[] results) {
        long existingRawContactId = state.findRawContactId();
        if (existingRawContactId != -1) {
            return existingRawContactId;
        }

        return getInsertedRawContactId(diffWrapper, results);
    }

    /**
     * Find the ID of a newly-inserted raw-contact.  If none exists, return -1.
     */
    private long getInsertedRawContactId(
            final ArrayList<CPOWrapper> diffWrapper, final ContentProviderResult[] results) {
        if (results == null) {
            return -1;
        }
        Log.d(TAG, "getInsertedRawContactId results.length: " + results.length);
        Log.d(TAG, "getInsertedRawContactId diffWrapper.size: " + diffWrapper.size());
        final int diffSize = diffWrapper.size();
        final int numResults = results.length;
        for (int i = 0; i < diffSize && i < numResults; i++) {
            final CPOWrapper cpoWrapper = diffWrapper.get(i);
            final boolean isInsert = CompatUtils.isInsertCompat(cpoWrapper);
            if (isInsert && cpoWrapper.getOperation().getUri().getEncodedPath().contains(
                    RawContacts.CONTENT_URI.getEncodedPath())) {
                return ContentUris.parseId(results[i].uri);
            }
        }
        return -1;
    }

    /**
     * Creates an intent that can be sent to this service to create a new group as
     * well as add new members at the same time.
     *
     * @param context of the application
     * @param account in which the group should be created
     * @param label is the name of the group (cannot be null)
     * @param rawContactsToAdd is an array of raw contact IDs for contacts that
     *            should be added to the group
     * @param callbackActivity is the activity to send the callback intent to
     * @param callbackAction is the intent action for the callback intent
     */
    public static Intent createNewGroupIntent(Context context, AccountWithDataSet account,
            String label, long[] rawContactsToAdd, Class<? extends Activity> callbackActivity,
            String callbackAction) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_CREATE_GROUP);
        serviceIntent.putExtra(ContactSaveService.EXTRA_ACCOUNT_TYPE, account.type);
        serviceIntent.putExtra(ContactSaveService.EXTRA_ACCOUNT_NAME, account.name);
        serviceIntent.putExtra(ContactSaveService.EXTRA_DATA_SET, account.dataSet);
        serviceIntent.putExtra(ContactSaveService.EXTRA_GROUP_LABEL, label);
        serviceIntent.putExtra(ContactSaveService.EXTRA_RAW_CONTACTS_TO_ADD, rawContactsToAdd);

        // Callback intent will be invoked by the service once the new group is
        // created.
        Intent callbackIntent = new Intent(context, callbackActivity);
        callbackIntent.setAction(callbackAction);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CALLBACK_INTENT, callbackIntent);

        return serviceIntent;
    }

    private void createGroup(Intent intent) {
        String accountType = intent.getStringExtra(EXTRA_ACCOUNT_TYPE);
        String accountName = intent.getStringExtra(EXTRA_ACCOUNT_NAME);
        String dataSet = intent.getStringExtra(EXTRA_DATA_SET);
        String label = intent.getStringExtra(EXTRA_GROUP_LABEL);
        final long[] rawContactsToAdd = intent.getLongArrayExtra(EXTRA_RAW_CONTACTS_TO_ADD);

        /**
         * SPRD:Bug426174 Cannot create namesake groups of the same accounts.
         *
         * SPRD:Bug639810 There is no marked words when create new contact with repeat group name
         * @{
         */
        boolean isRepeat = false;
        final ContentResolver resolver = getContentResolver();
        Cursor cursor = resolver
                .query(Groups.CONTENT_URI,
                        null,
                        "TITLE=? AND deleted=0 AND account_name =? AND account_type =? ",
                        new String[] { label, accountName, accountType }, null);
        if (null != cursor /* && cursor.moveToNext() */) {
            if (cursor.getCount() > 0) {
                isRepeat = true;
            }
            cursor.close();
        }
        if (isRepeat) {
            Intent repeatintent = new Intent("groupname repeat");
            sendBroadcast(repeatintent);
            stopSelf();
            showToast(R.string.group_name_repeat);
        } else {
        /**
         * @}
         */
        ContentValues values = new ContentValues();
        values.put(Groups.ACCOUNT_TYPE, accountType);
        values.put(Groups.ACCOUNT_NAME, accountName);
        values.put(Groups.DATA_SET, dataSet);
        values.put(Groups.TITLE, label);

        /* SPRD:Bug426174 Cannot create namesake groups of the same accounts.
        final ContentResolver resolver = getContentResolver();
        */

        // Create the new group
       /**
        * SPRD: Bug425303 Contacts crashed when add groups that exceeding SIM capacity.Bug454872.
        *
        * Original Android code:
        final Uri groupUri = resolver.insert(Groups.CONTENT_URI, values);
        * @{
        */

        int toastId = 0;
        Uri groupUri = null;
        try {
            groupUri = resolver.insert(Groups.CONTENT_URI, values);
        } catch (IllegalStateException e) {
            toastId = R.string.groupCreatedErrorToast;
            e.printStackTrace();
        } catch (IccPBForMimetypeException e) {
            // TODO: handle exception
            if (e.mErrorCode == IccPBForMimetypeException.OVER_LENGTH_LIMIT) {
                if (GroupMembership.CONTENT_ITEM_TYPE.equals(e.mMimeType)) {
                    toastId = R.string.overGroupNameLength;
                }
            } else if (e.mErrorCode == IccPBForMimetypeException.CAPACITY_FULL) {
                if (GroupMembership.CONTENT_ITEM_TYPE.equals(e.mMimeType)) {
                    toastId = R.string.overGroupCountLimit;
                }
            }
        } catch (Exception e) {
            // TODO: handle exception
            e.printStackTrace();
        }
       /**
        * @}
        */
        // If there's no URI, then the insertion failed. Abort early because group members can't be
        // added if the group doesn't exist
        /**
         * SPRD: Bug425303 Contacts crashed when add groups that exceeding SIM capacity.
         *
         * Original Android code:
        if (groupUri == null) {
            Log.e(TAG, "Couldn't create group with label " + label);
            return;
        }

        // Add new group members
        addMembersToGroup(resolver, rawContactsToAdd, ContentUris.parseId(groupUri));
        /*
        * @}
        */

        // TODO: Move this into the contact editor where it belongs. This needs to be integrated
        // with the way other intent extras that are passed to the {@link ContactEditorActivity}.
        values.clear();
        values.put(Data.MIMETYPE, GroupMembership.CONTENT_ITEM_TYPE);
        /**
        * SPRD:
        *   Bug425303 Contacts crashed when add groups that exceeding SIM capacity.
        *
        * @orig
        values.put(GroupMembership.GROUP_ROW_ID, ContentUris.parseId(groupUri));
        *
        * @{
        */
        if (groupUri != null) {
            values.put(GroupMembership.GROUP_ROW_ID, ContentUris.parseId(groupUri));
        }
        /**
        * @}
        */

        Intent callbackIntent = intent.getParcelableExtra(EXTRA_CALLBACK_INTENT);
        callbackIntent.setData(groupUri);
        // TODO: This can be taken out when the above TODO is addressed
        callbackIntent.putExtra(ContactsContract.Intents.Insert.DATA, Lists.newArrayList(values));
        /**
         * SPRD:Bug425303 Contacts crashed when add groups that exceeding SIM capacity.
         * @{
         */
        callbackIntent.putExtra(EXTRA_ERROR_TOAST, toastId);
        /**
         * @}
         */
        deliverCallback(callbackIntent);
        /**
         * SPRD:Bug425303 Contacts crashed when add groups that exceeding SIM capacity.
         *
         * @{
         */
        // If there's no URI, then the insertion failed. Abort early because
        // group members can't be
        // added if the group doesn't exist
        if (groupUri != null && rawContactsToAdd != null) {
            // Add new group members
            mIsGroupSaving = true;
            if (Constants.DEBUG)
                Log.d(TAG, "mIsGroupSaving = " + mIsGroupSaving);
            mUpdateGroupNotification = new UpdateGroupNotification(rawContactsToAdd.length);
            mUpdateGroupNotification.start();
            try {
                addMembersToGroup(resolver, rawContactsToAdd, ContentUris.parseId(groupUri));
                mUpdateGroupNotification.stop(true);
            } catch (Exception e) {
                e.printStackTrace();
                mUpdateGroupNotification.stop(false);
            }
            mUpdateGroupNotification = null;
        }
        }
        /**
         * @}
         */
    }

    /**
     * Creates an intent that can be sent to this service to rename a group.
     */
    public static Intent createGroupRenameIntent(Context context, long groupId, String newLabel,
            Class<? extends Activity> callbackActivity, String callbackAction) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_RENAME_GROUP);
        serviceIntent.putExtra(ContactSaveService.EXTRA_GROUP_ID, groupId);
        serviceIntent.putExtra(ContactSaveService.EXTRA_GROUP_LABEL, newLabel);

        // Callback intent will be invoked by the service once the group is renamed.
        Intent callbackIntent = new Intent(context, callbackActivity);
        callbackIntent.setAction(callbackAction);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CALLBACK_INTENT, callbackIntent);

        return serviceIntent;
    }

    private void renameGroup(Intent intent) {
        long groupId = intent.getLongExtra(EXTRA_GROUP_ID, -1);
        String label = intent.getStringExtra(EXTRA_GROUP_LABEL);

        /**
         * SPRD:Bug425303 Contacts crashed when add groups that exceeding SIM capacity.
         * @{
         */
        int toastId = 0;
        /**
         * @}
         */
        if (groupId == -1) {
            Log.e(TAG, "Invalid arguments for renameGroup request");
            return;
        }

        ContentValues values = new ContentValues();
        values.put(Groups.TITLE, label);
        /**
         * SPRD:Bug425303 Contacts crashed when add groups that exceeding SIM capacity.
         *
         * Original Android code:
        final Uri groupUri = ContentUris.withAppendedId(Groups.CONTENT_URI, groupId);
        getContentResolver().update(groupUri, values, null, null);
         * @{
         */
        Uri groupUri = ContentUris.withAppendedId(Groups.CONTENT_URI, groupId);
        try {
            getContentResolver().update(groupUri, values, null, null);
        } catch (IllegalStateException e) {
            e.printStackTrace();
            groupUri = null;
        } catch (IccPBForMimetypeException e) {
            // TODO: handle exception
            if (e.mErrorCode == IccPBForMimetypeException.OVER_LENGTH_LIMIT) {
                if (GroupMembership.CONTENT_ITEM_TYPE.equals(e.mMimeType)) {
                    toastId = R.string.overGroupNameLength;
                }
            } else if (e.mErrorCode == IccPBForMimetypeException.CAPACITY_FULL) {
                if (GroupMembership.CONTENT_ITEM_TYPE.equals(e.mMimeType)) {
                    toastId = R.string.overGroupCountLimit;
                }
            }
            groupUri = null;
        } catch (Exception e) {
            // TODO: handle exception
            e.printStackTrace();
            groupUri = null;
        }
        /**
         * @}
         */

        Intent callbackIntent = intent.getParcelableExtra(EXTRA_CALLBACK_INTENT);
        callbackIntent.setData(groupUri);
        /**
         * SPRD:Bug425303 Contacts crashed when add groups that exceeding SIM capacity.
         * @{
         */
        callbackIntent.putExtra(EXTRA_ERROR_TOAST, toastId);
        /**
         * @}
         */
        deliverCallback(callbackIntent);
    }

    /**
     * Creates an intent that can be sent to this service to delete a group.
     */
    public static Intent createGroupDeletionIntent(Context context, long groupId) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_DELETE_GROUP);
        serviceIntent.putExtra(ContactSaveService.EXTRA_GROUP_ID, groupId);
        return serviceIntent;
    }

    private void deleteGroup(Intent intent) {
        long groupId = intent.getLongExtra(EXTRA_GROUP_ID, -1);
        if (groupId == -1) {
            Log.e(TAG, "Invalid arguments for deleteGroup request");
            return;
        }

        /**
         * SPRD:Bug425719 Add for batch operation toast.
         * Orig:
        getContentResolver().delete(
                ContentUris.withAppendedId(Groups.CONTENT_URI, groupId), null, null);
         * @{
         */
        try {
            mIsGroupSaving = true;
            getContentResolver().delete(
                    ContentUris.withAppendedId(Groups.CONTENT_URI, groupId), null, null);
        } finally {
            mIsGroupSaving = false;
        }
        /**
         * @}
         */
    }

    /**
     * Creates an intent that can be sent to this service to rename a group as
     * well as add and remove members from the group.
     *
     * @param context of the application
     * @param groupId of the group that should be modified
     * @param newLabel is the updated name of the group (can be null if the name
     *            should not be updated)
     * @param rawContactsToAdd is an array of raw contact IDs for contacts that
     *            should be added to the group
     * @param rawContactsToRemove is an array of raw contact IDs for contacts
     *            that should be removed from the group
     * @param callbackActivity is the activity to send the callback intent to
     * @param callbackAction is the intent action for the callback intent
     */
    public static Intent createGroupUpdateIntent(Context context, long groupId, String newLabel,
            long[] rawContactsToAdd, long[] rawContactsToRemove,
            Class<? extends Activity> callbackActivity, String callbackAction) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_UPDATE_GROUP);
        serviceIntent.putExtra(ContactSaveService.EXTRA_GROUP_ID, groupId);
        serviceIntent.putExtra(ContactSaveService.EXTRA_GROUP_LABEL, newLabel);
        serviceIntent.putExtra(ContactSaveService.EXTRA_RAW_CONTACTS_TO_ADD, rawContactsToAdd);
        serviceIntent.putExtra(ContactSaveService.EXTRA_RAW_CONTACTS_TO_REMOVE,
                rawContactsToRemove);

        // Callback intent will be invoked by the service once the group is updated
        Intent callbackIntent = new Intent(context, callbackActivity);
        callbackIntent.setAction(callbackAction);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CALLBACK_INTENT, callbackIntent);

        return serviceIntent;
    }
    /**
     * SPRD:Bug422864 For move group member
     *
     * @{
     */
    public static Intent createGroupMoveIntent(Context context, long targetGroupId,long srcGroupId, String newLabel,
            long[] rawContactsToMove,
            Class<? extends Activity> callbackActivity, String callbackAction) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_MOVE_GROUP);
        serviceIntent.putExtra(ContactSaveService.EXTRA_GROUP_ID, targetGroupId);
        serviceIntent.putExtra(ContactSaveService.EXTRA_SRC_GROUP_ID,srcGroupId);
        serviceIntent.putExtra(ContactSaveService.EXTRA_GROUP_LABEL, newLabel);
        serviceIntent.putExtra(ContactSaveService.EXTRA_RAW_CONTACTS_TO_MOVE, rawContactsToMove);

        // Callback intent will be invoked by the service once the group is updated
        Intent callbackIntent = new Intent(context, callbackActivity);
        callbackIntent.setAction(callbackAction);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CALLBACK_INTENT, callbackIntent);

        return serviceIntent;
    }

    private void moveGroup(Intent intent){
        long targetGroupId = intent.getLongExtra(EXTRA_GROUP_ID, -1);
        long srcGroupId = intent.getLongExtra(EXTRA_SRC_GROUP_ID, -1);
        long[] rawContactsToMove = intent.getLongArrayExtra(EXTRA_RAW_CONTACTS_TO_MOVE);

        if (targetGroupId == -1) {
            Log.e(TAG, "Invalid arguments for moveGroup request");
            return;
        }

        final ContentResolver resolver = getContentResolver();
        int toastId = 0;
        Uri groupUri = ContentUris.withAppendedId(Groups.CONTENT_URI, targetGroupId);

        if (groupUri != null) {
            mIsGroupSaving = true;
            if (Constants.DEBUG)
                Log.d(TAG, "move:mIsGroupSaving = " + mIsGroupSaving);
            if ((rawContactsToMove != null)) {
                mUpdateGroupNotification = new UpdateGroupNotification(
                        rawContactsToMove.length);
            } else {
                mUpdateGroupNotification = new UpdateGroupNotification(0);
            }


            mUpdateGroupNotification.start();
            try {
                moveMembersToGroup(resolver, rawContactsToMove,targetGroupId, srcGroupId);

                mUpdateGroupNotification.stop(true);
            } catch (Exception e) {
                e.printStackTrace();
                mUpdateGroupNotification.stop(false);
            }
            mUpdateGroupNotification = null;
        }

        Intent callbackIntent = intent.getParcelableExtra(EXTRA_CALLBACK_INTENT);
        callbackIntent.setData(groupUri);

        callbackIntent.putExtra(EXTRA_ERROR_TOAST, toastId);

        deliverCallback(callbackIntent);
    }

    private static void moveMembersToGroup(ContentResolver resolver, long[] rawContactsToMove,
            long targetGroupId,long srcGroupId) {
        if (rawContactsToMove == null) {
            return;
        }
        ArrayList<ContentProviderOperation> rawContactOperations =
                new ArrayList<ContentProviderOperation>();
        int count = 0;
        for (long rawContactId : rawContactsToMove) {

            final ContentProviderOperation.Builder updateBuilder = ContentProviderOperation
                    .newUpdate(Data.CONTENT_URI);

            updateBuilder.withSelection(
                    Data.RAW_CONTACT_ID + " = ? AND " + Data.MIMETYPE + "=? AND "
                            + GroupMembership.GROUP_ROW_ID + "=?",
                    new String[] {
                            String.valueOf(rawContactId),
                            GroupMembership.CONTENT_ITEM_TYPE, String.valueOf(srcGroupId)
                    }
            );
            updateBuilder.withValue(GroupMembership.GROUP_ROW_ID,targetGroupId);
            updateBuilder.withYieldAllowed(true);
            rawContactOperations.add(updateBuilder.build());

            if (DEBUG) {
                for (ContentProviderOperation operation : rawContactOperations) {
                    Log.v(TAG, operation.toString());
                }
            }

            if (count++ > 20) {
                try {
                    resolver.applyBatch(ContactsContract.AUTHORITY, rawContactOperations);
                } catch (Exception e) {
                    throw new IllegalStateException("moveMembersToGroup failed");
                }
                if (mUpdateGroupNotification != null) {
                    mUpdateGroupNotification.updateProgress(20);
                }
                count = 0;
                rawContactOperations.clear();
            }

        }

        if (true/*Log.isIloggable()*/) {
            /*Log.stopPerfTracking(Constants.PERFORMANCE_TAG
                    + String.format(": Successfully finish moving %d contacts to group",
                            rawContactsToMove.length));*/ //sprd_4.4
        }

        try {
            resolver.applyBatch(ContactsContract.AUTHORITY, rawContactOperations);
        } catch (Exception e) {
            throw new IllegalStateException("moveMembersToGroup failed");
        }

    }
    /**
     * @}
     */
    private void updateGroup(Intent intent) {
        long groupId = intent.getLongExtra(EXTRA_GROUP_ID, -1);
        String label = intent.getStringExtra(EXTRA_GROUP_LABEL);
        long[] rawContactsToAdd = intent.getLongArrayExtra(EXTRA_RAW_CONTACTS_TO_ADD);
        long[] rawContactsToRemove = intent.getLongArrayExtra(EXTRA_RAW_CONTACTS_TO_REMOVE);

        if (groupId == -1) {
            Log.e(TAG, "Invalid arguments for updateGroup request");
            return;
        }

        final ContentResolver resolver = getContentResolver();
        /**
         * SPRD:Bug426174 Cannot create namesake groups of the same accounts.
         * @{
         */
        boolean isRepeat = false;
        if (label != null) {
            Cursor cursor = resolver
                    .query(Groups.CONTENT_URI,
                            null,
                            "TITLE=? AND deleted=0 AND (account_id = (SELECT account_id from groups WHERE _id = ?))",
                            new String[] { label, String.valueOf(groupId) },
                            null);
            if (null != cursor /* && cursor.moveToNext() */) {
                if (cursor.getCount() > 0) {
                    isRepeat = true;
                }
                cursor.close();
            }
        }
        if (isRepeat) {
            Log.v(TAG, "exist");
            Intent repeatintent = new Intent("groupname repeat");
            sendBroadcast(repeatintent);
            stopSelf();
        } else {
        /**
         * @}
         */
        /**
         * SPRD:Bug425303 Contacts crashed when add groups that exceeding SIM capacity.
         *
         * Original Android code:
        final Uri groupUri = ContentUris.withAppendedId(Groups.CONTENT_URI, groupId);
         * @{
         */

        int toastId = 0;
        Uri groupUri = ContentUris.withAppendedId(Groups.CONTENT_URI, groupId);
        /**
         * @}
         */
        // Update group name if necessary
        if (label != null) {
            ContentValues values = new ContentValues();
            values.put(Groups.TITLE, label);
            /**
             * SPRD:Bug425303 Contacts crashed when add groups that exceeding SIM capacity.
             *
             * Original Android code:
            resolver.update(groupUri, values, null, null);
             * @{
             */
            try {
                resolver.update(groupUri, values, null, null);
            } catch (IllegalStateException e) {
                e.printStackTrace();
                groupUri = null;
            } catch (IccPBForMimetypeException e) {
                // TODO: handle exception
                if (e.mErrorCode == IccPBForMimetypeException.OVER_LENGTH_LIMIT) {
                    if (GroupMembership.CONTENT_ITEM_TYPE.equals(e.mMimeType)) {
                        toastId = R.string.overGroupNameLength;
                    }
                } else if (e.mErrorCode == IccPBForMimetypeException.CAPACITY_FULL) {
                    if (GroupMembership.CONTENT_ITEM_TYPE.equals(e.mMimeType)) {
                        toastId = R.string.overGroupCountLimit;
                    }
                }
                groupUri = null;
            } catch (Exception e) {
                // TODO: handle exception
                e.printStackTrace();
                groupUri = null;
            }
            /**
             * @}
             */
        }

        /**
         * SPRD:
         * Original Android code:
        // Add and remove members if necessary
        addMembersToGroup(resolver, rawContactsToAdd, groupId);
        removeMembersFromGroup(resolver, rawContactsToRemove, groupId);
        /**
         * @}
         */

        Intent callbackIntent = intent.getParcelableExtra(EXTRA_CALLBACK_INTENT);
        callbackIntent.setData(groupUri);
        /**
         * SPRD:Bug425303 Contacts crashed when add groups that exceeding SIM capacity.
         * @{
         */
        callbackIntent.putExtra(EXTRA_ERROR_TOAST, toastId);
        /**
         * @}
         */
        deliverCallback(callbackIntent);
        /**
         * SPRD:SPRD:Bug425303 Contacts crashed when add groups that exceeding SIM capacity.
         *
         * @{
         */
        // Add and remove members if necessary
        if (groupUri != null) {
            mIsGroupSaving = true;
            if (Constants.DEBUG)
                Log.d(TAG, "update:mIsGroupSaving = " + mIsGroupSaving);

            if ((rawContactsToAdd == null) && (rawContactsToRemove != null)) {
                mUpdateGroupNotification = new UpdateGroupNotification(
                        rawContactsToRemove.length);
            }
            if ((rawContactsToAdd == null) && (rawContactsToRemove == null)) {
                mUpdateGroupNotification = new UpdateGroupNotification(0);
            }
            if ((rawContactsToAdd != null) && (rawContactsToRemove == null)) {
                mUpdateGroupNotification = new UpdateGroupNotification(
                        rawContactsToAdd.length);
            }
            if ((rawContactsToAdd != null) && (rawContactsToRemove != null)) {
                mUpdateGroupNotification = new UpdateGroupNotification(
                        rawContactsToAdd.length
                                + rawContactsToRemove.length);
            }

            mUpdateGroupNotification.start();
            try {
                addMembersToGroup(resolver, rawContactsToAdd, groupId);
                removeMembersFromGroup(resolver, rawContactsToRemove, groupId);
                mUpdateGroupNotification.stop(true);
            } catch (Exception e) {
                e.printStackTrace();
                mUpdateGroupNotification.stop(false);
            }
            mUpdateGroupNotification = null;
        }
        /**
         * @}
         */
        }
    }

    private static Context mContext = null;
    private static void addMembersToGroup(ContentResolver resolver, long[] rawContactsToAdd,
            long groupId) {
        if (rawContactsToAdd == null) {
            return;
        }
        /**
         * SPRD:Bug425719 Add batch operations.
         * Orig:
        for (long rawContactId : rawContactsToAdd) {
            try {
                final ArrayList<ContentProviderOperation> rawContactOperations =
                        new ArrayList<ContentProviderOperation>();
         * @{
         */
        ArrayList<ContentProviderOperation> rawContactOperations =
                new ArrayList<ContentProviderOperation>();
        int count = 0;

        for (long rawContactId : rawContactsToAdd) {
        /**
         * @}
         */

            /**
             * SPRD:Bug396231 Remove the assertBuilder check for member repeate.
             * Orig:
                // Build an assert operation to ensure the contact is not already in the group
                final ContentProviderOperation.Builder assertBuilder = ContentProviderOperation
                        .newAssertQuery(Data.CONTENT_URI);
                assertBuilder.withSelection(Data.RAW_CONTACT_ID + "=? AND " +
                        Data.MIMETYPE + "=? AND " + GroupMembership.GROUP_ROW_ID + "=?",
                        new String[] { String.valueOf(rawContactId),
                        GroupMembership.CONTENT_ITEM_TYPE, String.valueOf(groupId)});
                assertBuilder.withExpectedCount(0);
                rawContactOperations.add(assertBuilder.build());

              * @{
              */
               // Build an assert operation to ensure the contact is not already in the group
               /*final ContentProviderOperation.Builder assertBuilder = ContentProviderOperation
                         .newAssertQuery(Data.CONTENT_URI);
               assertBuilder.withSelection(Data.RAW_CONTACT_ID + "=? AND " +
                       Data.MIMETYPE + "=? AND " + GroupMembership.GROUP_ROW_ID + "=?",
                       new String[] { String.valueOf(rawContactId),
                       GroupMembership.CONTENT_ITEM_TYPE, String.valueOf(groupId)});
               assertBuilder.withExpectedCount(0);
               rawContactOperations.add(assertBuilder.build());*/
             /**
              * @}
              */

                // Build an insert operation to add the contact to the group
                final ContentProviderOperation.Builder insertBuilder = ContentProviderOperation
                        .newInsert(Data.CONTENT_URI);
                insertBuilder.withValue(Data.RAW_CONTACT_ID, rawContactId);
                insertBuilder.withValue(Data.MIMETYPE, GroupMembership.CONTENT_ITEM_TYPE);
                insertBuilder.withValue(GroupMembership.GROUP_ROW_ID, groupId);
                /**
                 * SPRD:
                 * @{
                 */
                insertBuilder.withYieldAllowed(true);
                /**
                 * @}
                 */
                rawContactOperations.add(insertBuilder.build());

                if (DEBUG) {
                    for (ContentProviderOperation operation : rawContactOperations) {
                        Log.v(TAG, operation.toString());
                    }
                }

                /**
                 * SPRD:Add for apply batch.
                 * Orig:
                // Apply batch
                if (!rawContactOperations.isEmpty()) {
                    resolver.applyBatch(ContactsContract.AUTHORITY, rawContactOperations);
                }
            } catch (RemoteException e) {
                // Something went wrong, bail without success
                Log.e(TAG, "Problem persisting user edits for raw contact ID " +
                        String.valueOf(rawContactId), e);
            } catch (OperationApplicationException e) {
                // The assert could have failed because the contact is already in the group,
                // just continue to the next contact
                Log.w(TAG, "Assert failed in adding raw contact ID " +
                        String.valueOf(rawContactId) + ". Already exists in group " +
                        String.valueOf(groupId), e);
            }
                * @{
                */
                if (count++ > 20) {
                    try {
                        resolver.applyBatch(ContactsContract.AUTHORITY, rawContactOperations);
                        /**
                         * SPRD: bug540015 Some time dut fail to add contacts in sim group, after
                         * reboot working fine.
                         * @{
                         */
                    } catch (IccPBForRecordException e) {
                        if (e.mErrorCode == IccPBForRecordException.OVER_GRP_MAX_LENGTH) {
                            showToastByRunnable(R.string.overGroupMaxLength);
                        }
                        /*
                         * @}
                         */
                    } catch (Exception e) {
                        throw new IllegalStateException("addMembersToGroup failed");
                    }
                    if (mUpdateGroupNotification != null) {
                        mUpdateGroupNotification.updateProgress(20);
                    }
                    count = 0;
                    rawContactOperations.clear();
                }
        }
        try {
            resolver.applyBatch(ContactsContract.AUTHORITY, rawContactOperations);
            /**
             * SPRD: bug540015 Some time dut fail to add contacts in sim group,
             * after reboot working fine.
             * @{
             */
        } catch (IccPBForRecordException e) {
            if (e.mErrorCode == IccPBForRecordException.OVER_GRP_MAX_LENGTH) {
                showToastByRunnable(R.string.overGroupMaxLength);
            }
            /*
             * @}
             */
        } catch (Exception e) {
            throw new IllegalStateException("addMembersToGroup failed");
        }
        /**
         * @}
         */
    }

    /**
     * SPRD: bug540015 Some time dut fail to add contacts in sim group, after
     * reboot working fine.
     * @param text  string resource id
     * @{
     */
    private static void showToastByRunnable(final int text) {
        Handler handler = new Handler(Looper.getMainLooper());
        handler.post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(mContext, text, Toast.LENGTH_LONG).show();
            }
        });
    }
    /*
     * @}
     */
    private static void removeMembersFromGroup(ContentResolver resolver, long[] rawContactsToRemove,
            long groupId) {
        if (rawContactsToRemove == null) {
            return;
        }
        /**
         * SPRD:Add for batch operations.
         * Orig:
        for (long rawContactId : rawContactsToRemove) {
            // Apply the delete operation on the data row for the given raw contact's
            // membership in the given group. If no contact matches the provided selection, then
            // nothing will be done. Just continue to the next contact.
            resolver.delete(Data.CONTENT_URI, Data.RAW_CONTACT_ID + "=? AND " +
                    Data.MIMETYPE + "=? AND " + GroupMembership.GROUP_ROW_ID + "=?",
                    new String[] { String.valueOf(rawContactId),
                    GroupMembership.CONTENT_ITEM_TYPE, String.valueOf(groupId)});
        }
         * @{
         */
        ArrayList<ContentProviderOperation> rawContactOperations =
                new ArrayList<ContentProviderOperation>();
        int count = 0;

        for (long rawContactId : rawContactsToRemove) {
            // Apply the delete operation on the data row for the given raw contact's
            // membership in the given group. If no contact matches the provided selection, then
            // nothing will be done. Just continue to the next contact.
            ContentProviderOperation operation = ContentProviderOperation
                    .newDelete(Data.CONTENT_URI)
                    .withSelection(
                            Data.RAW_CONTACT_ID + " = ? AND " + Data.MIMETYPE + "=? AND "
                                    + GroupMembership.GROUP_ROW_ID + "=?",
                            new String[] {
                                    String.valueOf(rawContactId),
                                    GroupMembership.CONTENT_ITEM_TYPE, String.valueOf(groupId)
                            }
                    )
                    .withYieldAllowed(true)
                    .build();
            rawContactOperations.add(operation);
            if (count++ > 20) {
                try {
                    resolver.applyBatch(ContactsContract.AUTHORITY, rawContactOperations);
                } catch (Exception e) {
                    throw new IllegalStateException("removeMembersFromGroup failed");
                }
                if (mUpdateGroupNotification != null) {
                    mUpdateGroupNotification.updateProgress(20);
                }
                count = 0;
                rawContactOperations.clear();
            }
        }
        try {
            resolver.applyBatch(ContactsContract.AUTHORITY, rawContactOperations);
        } catch (Exception e) {
            throw new IllegalStateException("removeMembersFromGroup failed");
        }
        /**
         * @}
         */
    }

    /**
     * Creates an intent that can be sent to this service to star or un-star a contact.
     */
    public static Intent createSetStarredIntent(Context context, Uri contactUri, boolean value) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_SET_STARRED);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CONTACT_URI, contactUri);
        serviceIntent.putExtra(ContactSaveService.EXTRA_STARRED_FLAG, value);

        return serviceIntent;
    }

    private void setStarred(Intent intent) {
        Uri contactUri = intent.getParcelableExtra(EXTRA_CONTACT_URI);
        boolean value = intent.getBooleanExtra(EXTRA_STARRED_FLAG, false);
        if (contactUri == null) {
            Log.e(TAG, "Invalid arguments for setStarred request");
            return;
        }

        final ContentValues values = new ContentValues(1);
        values.put(Contacts.STARRED, value);
        Cursor c = null;
        try {
            getContentResolver().update(contactUri, values, null, null);

            // Undemote the contact if necessary
            c = getContentResolver().query(contactUri,
                    new String[] { Contacts._ID }, null, null, null);
            if (c == null) {
                return;
            }
            if (c.moveToFirst()) {
                final long id = c.getLong(0);

                // Don't bother undemoting if this contact is the user's
                // profile.
                if (id < Profile.MIN_ID) {
                    PinnedPositionsCompat.undemote(getContentResolver(), id);
                }
            }
        } catch (SQLiteFullException e) {
            Log.e(TAG, e.getMessage());
        } finally {
            c.close();
        }
    }

    /**
     * Creates an intent that can be sent to this service to set the redirect to voicemail.
     */
    public static Intent createSetSendToVoicemail(Context context, Uri contactUri,
            boolean value) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_SET_SEND_TO_VOICEMAIL);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CONTACT_URI, contactUri);
        serviceIntent.putExtra(ContactSaveService.EXTRA_SEND_TO_VOICEMAIL_FLAG, value);

        return serviceIntent;
    }

    private void setSendToVoicemail(Intent intent) {
        Uri contactUri = intent.getParcelableExtra(EXTRA_CONTACT_URI);
        boolean value = intent.getBooleanExtra(EXTRA_SEND_TO_VOICEMAIL_FLAG, false);
        if (contactUri == null) {
            Log.e(TAG, "Invalid arguments for setRedirectToVoicemail");
            return;
        }

        final ContentValues values = new ContentValues(1);
        values.put(Contacts.SEND_TO_VOICEMAIL, value);
        getContentResolver().update(contactUri, values, null, null);
    }

    /**
     * Creates an intent that can be sent to this service to save the contact's ringtone.
     */
    public static Intent createSetRingtone(Context context, Uri contactUri,
            String value) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_SET_RINGTONE);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CONTACT_URI, contactUri);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CUSTOM_RINGTONE, value);

        return serviceIntent;
    }

    private void setRingtone(Intent intent) {
        Uri contactUri = intent.getParcelableExtra(EXTRA_CONTACT_URI);
        String value = intent.getStringExtra(EXTRA_CUSTOM_RINGTONE);
        if (contactUri == null) {
            Log.e(TAG, "Invalid arguments for setRingtone");
            return;
        }
        ContentValues values = new ContentValues(1);
        values.put(Contacts.CUSTOM_RINGTONE, value);
        getContentResolver().update(contactUri, values, null, null);
    }

    /**
     * Creates an intent that sets the selected data item as super primary (default)
     */
    public static Intent createSetSuperPrimaryIntent(Context context, long dataId) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_SET_SUPER_PRIMARY);
        serviceIntent.putExtra(ContactSaveService.EXTRA_DATA_ID, dataId);
        return serviceIntent;
    }

    private void setSuperPrimary(Intent intent) {
        long dataId = intent.getLongExtra(EXTRA_DATA_ID, -1);
        if (dataId == -1) {
            Log.e(TAG, "Invalid arguments for setSuperPrimary request");
            return;
        }

        ContactUpdateUtils.setSuperPrimary(this, dataId);
    }

    /**
     * Creates an intent that clears the primary flag of all data items that belong to the same
     * raw_contact as the given data item. Will only clear, if the data item was primary before
     * this call
     */
    public static Intent createClearPrimaryIntent(Context context, long dataId) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_CLEAR_PRIMARY);
        serviceIntent.putExtra(ContactSaveService.EXTRA_DATA_ID, dataId);
        return serviceIntent;
    }

    private void clearPrimary(Intent intent) {
        long dataId = intent.getLongExtra(EXTRA_DATA_ID, -1);
        if (dataId == -1) {
            Log.e(TAG, "Invalid arguments for clearPrimary request");
            return;
        }

        // Update the primary values in the data record.
        ContentValues values = new ContentValues(1);
        values.put(Data.IS_SUPER_PRIMARY, 0);
        values.put(Data.IS_PRIMARY, 0);

        getContentResolver().update(ContentUris.withAppendedId(Data.CONTENT_URI, dataId),
                values, null, null);
    }

    /**
     * Creates an intent that can be sent to this service to delete a contact.
     */
    public static Intent createDeleteContactIntent(Context context, Uri contactUri) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_DELETE_CONTACT);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CONTACT_URI, contactUri);
        return serviceIntent;
    }

    /**
     * Creates an intent that can be sent to this service to delete multiple contacts.
     */
    public static Intent createDeleteMultipleContactsIntent(Context context,
            long[] contactIds) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_DELETE_MULTIPLE_CONTACTS);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CONTACT_IDS, contactIds);
        return serviceIntent;
    }

    private void deleteContact(Intent intent) {
        Uri contactUri = intent.getParcelableExtra(EXTRA_CONTACT_URI);
        if (contactUri == null) {
            Log.e(TAG, "Invalid arguments for deleteContact request");
            return;
        }
        /**
         * SPRD:Bug605491 Crash happen when storage is full
         * @{
         */
        try {
            getContentResolver().delete(contactUri, null, null);
        } catch (SQLiteException e) {
            Log.e(TAG, "insert error : " + e.getMessage());
        }
        /**
         * @}
         */
    }

    private void deleteMultipleContacts(Intent intent) {
        final long[] contactIds = intent.getLongArrayExtra(EXTRA_CONTACT_IDS);
        if (contactIds == null) {
            Log.e(TAG, "Invalid arguments for deleteMultipleContacts request");
            return;
        }
        for (long contactId : contactIds) {
            final Uri contactUri = ContentUris.withAppendedId(Contacts.CONTENT_URI, contactId);
            getContentResolver().delete(contactUri, null, null);
        }
        /**
         * SPRD:Bug497538 while delete several selected contacts after long click,
         * long click the deleting contact and share it, Contacts app FC.
         *
         * @{
         */
        ContactMultiDeletionInteraction.isMultiContactsDeleting = false;
        /**
         * @}
         */
		final String deleteToastMessage = getResources().getQuantityString(R.plurals
                .contacts_deleted_toast, contactIds.length);
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(ContactSaveService.this, deleteToastMessage, Toast.LENGTH_LONG)
                        .show();
            }
        });
    }

    /**
     * Creates an intent that can be sent to this service to join two contacts.
     * The resulting contact uses the name from {@param contactId1} if possible.
     */
    public static Intent createJoinContactsIntent(Context context, long contactId1,
            long contactId2, Class<? extends Activity> callbackActivity, String callbackAction) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_JOIN_CONTACTS);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CONTACT_ID1, contactId1);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CONTACT_ID2, contactId2);

        // Callback intent will be invoked by the service once the contacts are joined.
        Intent callbackIntent = new Intent(context, callbackActivity);
        callbackIntent.setAction(callbackAction);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CALLBACK_INTENT, callbackIntent);

        return serviceIntent;
    }

    /**
     * Creates an intent to join all raw contacts inside {@param contactIds}'s contacts.
     * No special attention is paid to where the resulting contact's name is taken from.
     */
    public static Intent createJoinSeveralContactsIntent(Context context, long[] contactIds) {
        Intent serviceIntent = new Intent(context, ContactSaveService.class);
        serviceIntent.setAction(ContactSaveService.ACTION_JOIN_SEVERAL_CONTACTS);
        serviceIntent.putExtra(ContactSaveService.EXTRA_CONTACT_IDS, contactIds);
        return serviceIntent;
    }


    private interface JoinContactQuery {
        String[] PROJECTION = {
                RawContacts._ID,
                RawContacts.CONTACT_ID,
                RawContacts.DISPLAY_NAME_SOURCE,
        };

        int _ID = 0;
        int CONTACT_ID = 1;
        int DISPLAY_NAME_SOURCE = 2;
    }

    private interface ContactEntityQuery {
        String[] PROJECTION = {
                Contacts.Entity.DATA_ID,
                Contacts.Entity.CONTACT_ID,
                Contacts.Entity.IS_SUPER_PRIMARY,
        };
        String SELECTION = Data.MIMETYPE + " = '" + StructuredName.CONTENT_ITEM_TYPE + "'" +
                " AND " + StructuredName.DISPLAY_NAME + "=" + Contacts.DISPLAY_NAME +
                " AND " + StructuredName.DISPLAY_NAME + " IS NOT NULL " +
                " AND " + StructuredName.DISPLAY_NAME + " != '' ";

        int DATA_ID = 0;
        int CONTACT_ID = 1;
        int IS_SUPER_PRIMARY = 2;
    }

    private void joinSeveralContacts(Intent intent) {
        final long[] contactIds = intent.getLongArrayExtra(EXTRA_CONTACT_IDS);
        /*
         * @{
         * SPRD:513110 Contact list disapper while rotate during join contact
         */
        if (mJoinContactListener != null) {
                       mJoinContactListener.onJoinStart();
        }
        /*
         * @}
         */
        // Load raw contact IDs for all contacts involved.
        long rawContactIds[] = getRawContactIdsForAggregation(contactIds);
        if (rawContactIds == null) {
            Log.e(TAG, "Invalid arguments for joinSeveralContacts request");
            return;
        }

        // For each pair of raw contacts, insert an aggregation exception
        final ContentResolver resolver = getContentResolver();
        // The maximum number of operations per batch (aka yield point) is 500. See b/22480225
        final int batchSize = MAX_CONTACTS_PROVIDER_BATCH_SIZE;
        final ArrayList<ContentProviderOperation> operations = new ArrayList<>(batchSize);
        for (int i = 0; i < rawContactIds.length; i++) {
            for (int j = 0; j < rawContactIds.length; j++) {
                if (i != j) {
                    buildJoinContactDiff(operations, rawContactIds[i], rawContactIds[j]);
                }
                // Before we get to 500 we need to flush the operations list
                if (operations.size() > 0 && operations.size() % batchSize == 0) {
                    if (!applyJoinOperations(resolver, operations)) {
                        return;
                    }
                    operations.clear();
                }
            }
        }
        if (operations.size() > 0 && !applyJoinOperations(resolver, operations)) {
            return;
        }
        if (mJoinContactListener != null) {
                        mJoinContactListener.onJoinFinished();
        }
        // SPRD: 604761 The DUT fails to dismiss the dialog even contact joining finished.
        if (PeopleActivity.getJoinContactDialog() != null) {
            PeopleActivity.getJoinContactDialog().dismiss();
        }
        showToast(R.string.contactsJoinedMessage);
    }

    /** Returns true if the batch was successfully applied and false otherwise. */
    private boolean applyJoinOperations(ContentResolver resolver,
            ArrayList<ContentProviderOperation> operations) {
        try {
            resolver.applyBatch(ContactsContract.AUTHORITY, operations);
            return true;
        } catch (RemoteException | OperationApplicationException e) {
            Log.e(TAG, "Failed to apply aggregation exception batch", e);
            if (mJoinContactListener != null) {
                mJoinContactListener.onJoinFinished();
            }
            // SPRD: 604761 The DUT fails to dismiss the dialog even contact joining finished.
            if(PeopleActivity.getJoinContactDialog() != null) {
                PeopleActivity.getJoinContactDialog().dismiss();
            }
            showToast(R.string.contactSavedErrorToast);
            return false;
        }
    }


    private void joinContacts(Intent intent) {
        long contactId1 = intent.getLongExtra(EXTRA_CONTACT_ID1, -1);
        long contactId2 = intent.getLongExtra(EXTRA_CONTACT_ID2, -1);

        // Load raw contact IDs for all raw contacts involved - currently edited and selected
        // in the join UIs.
        long rawContactIds[] = getRawContactIdsForAggregation(contactId1, contactId2);
        if (rawContactIds == null) {
            Log.e(TAG, "Invalid arguments for joinContacts request");
            return;
        }

        ArrayList<ContentProviderOperation> operations = new ArrayList<ContentProviderOperation>();

        // For each pair of raw contacts, insert an aggregation exception
        for (int i = 0; i < rawContactIds.length; i++) {
            for (int j = 0; j < rawContactIds.length; j++) {
                if (i != j) {
                    buildJoinContactDiff(operations, rawContactIds[i], rawContactIds[j]);
                }
            }
        }

        final ContentResolver resolver = getContentResolver();

        // Use the name for contactId1 as the name for the newly aggregated contact.
        final Uri contactId1Uri = ContentUris.withAppendedId(
                Contacts.CONTENT_URI, contactId1);
        final Uri entityUri = Uri.withAppendedPath(
                contactId1Uri, Contacts.Entity.CONTENT_DIRECTORY);
        Cursor c = resolver.query(entityUri,
                ContactEntityQuery.PROJECTION, ContactEntityQuery.SELECTION, null, null);
        if (c == null) {
            Log.e(TAG, "Unable to open Contacts DB cursor");
            showToast(R.string.contactSavedErrorToast);
            return;
        }
        long dataIdToAddSuperPrimary = -1;
        try {
            if (c.moveToFirst()) {
                dataIdToAddSuperPrimary = c.getLong(ContactEntityQuery.DATA_ID);
            }
        } finally {
            c.close();
        }

        // Mark the name from contactId1 IS_SUPER_PRIMARY to make sure that the contact
        // display name does not change as a result of the join.
        if (dataIdToAddSuperPrimary != -1) {
            Builder builder = ContentProviderOperation.newUpdate(
                    ContentUris.withAppendedId(Data.CONTENT_URI, dataIdToAddSuperPrimary));
            builder.withValue(Data.IS_SUPER_PRIMARY, 1);
            builder.withValue(Data.IS_PRIMARY, 1);
            operations.add(builder.build());
        }

        boolean success = false;
        // Apply all aggregation exceptions as one batch
        try {
            resolver.applyBatch(ContactsContract.AUTHORITY, operations);
            showToast(R.string.contactsJoinedMessage);
            success = true;
        } catch (RemoteException | OperationApplicationException e) {
            Log.e(TAG, "Failed to apply aggregation exception batch", e);
            showToast(R.string.contactSavedErrorToast);
        }

        Intent callbackIntent = intent.getParcelableExtra(EXTRA_CALLBACK_INTENT);
        if (success) {
            Uri uri = RawContacts.getContactLookupUri(resolver,
                    ContentUris.withAppendedId(RawContacts.CONTENT_URI, rawContactIds[0]));
            callbackIntent.setData(uri);
        }
        deliverCallback(callbackIntent);
    }

    private long[] getRawContactIdsForAggregation(long[] contactIds) {
        if (contactIds == null) {
            return null;
        }

        final ContentResolver resolver = getContentResolver();
        long rawContactIds[];

        final StringBuilder queryBuilder = new StringBuilder();
        final String stringContactIds[] = new String[contactIds.length];
        for (int i = 0; i < contactIds.length; i++) {
            queryBuilder.append(RawContacts.CONTACT_ID + "=?");
            stringContactIds[i] = String.valueOf(contactIds[i]);
            if (contactIds[i] == -1) {
                return null;
            }
            if (i == contactIds.length -1) {
                break;
            }
            queryBuilder.append(" OR ");
        }

        final Cursor c = resolver.query(RawContacts.CONTENT_URI,
                JoinContactQuery.PROJECTION,
                queryBuilder.toString(),
                stringContactIds, null);
        if (c == null) {
            Log.e(TAG, "Unable to open Contacts DB cursor");
            showToast(R.string.contactSavedErrorToast);
            return null;
        }
        try {
            if (c.getCount() < 2) {
                Log.e(TAG, "Not enough raw contacts to aggregate together.");
                return null;
            }
            rawContactIds = new long[c.getCount()];
            for (int i = 0; i < rawContactIds.length; i++) {
                c.moveToPosition(i);
                long rawContactId = c.getLong(JoinContactQuery._ID);
                rawContactIds[i] = rawContactId;
            }
        } finally {
            c.close();
        }
        return rawContactIds;
    }

    private long[] getRawContactIdsForAggregation(long contactId1, long contactId2) {
        return getRawContactIdsForAggregation(new long[] {contactId1, contactId2});
    }

    /**
     * Construct a {@link AggregationExceptions#TYPE_KEEP_TOGETHER} ContentProviderOperation.
     */
    private void buildJoinContactDiff(ArrayList<ContentProviderOperation> operations,
            long rawContactId1, long rawContactId2) {
        Builder builder =
                ContentProviderOperation.newUpdate(AggregationExceptions.CONTENT_URI);
        builder.withValue(AggregationExceptions.TYPE, AggregationExceptions.TYPE_KEEP_TOGETHER);
        builder.withValue(AggregationExceptions.RAW_CONTACT_ID1, rawContactId1);
        builder.withValue(AggregationExceptions.RAW_CONTACT_ID2, rawContactId2);
        operations.add(builder.build());
    }

    /**
     * Shows a toast on the UI thread.
     */
    private void showToast(final int message) {
        mMainHandler.post(new Runnable() {

            @Override
            public void run() {
                Toast.makeText(ContactSaveService.this, message, Toast.LENGTH_LONG).show();
            }
        });
    }

    private void deliverCallback(final Intent callbackIntent) {
        mMainHandler.post(new Runnable() {

            @Override
            public void run() {
                deliverCallbackOnUiThread(callbackIntent);
            }
        });
    }

    void deliverCallbackOnUiThread(final Intent callbackIntent) {
        // TODO: this assumes that if there are multiple instances of the same
        // activity registered, the last one registered is the one waiting for
        // the callback. Validity of this assumption needs to be verified.
        for (Listener listener : sListeners) {
            if (callbackIntent.getComponent().equals(
                    ((Activity) listener).getIntent().getComponent())) {
                listener.onServiceCompleted(callbackIntent);
                return;
            }
        }
        /**
         * Bug507507 when edit email and then open more fields button,
         * CompactContactEditorActivity can't listen the listener. (workaround)
         * @{
         */
        if(callbackIntent.getComponent().toShortString().endsWith("CompactContactEditorActivity}")) {
            boolean saveResult = callbackIntent.getBooleanExtra(ContactSaveService.EXTRA_SAVE_SUCCEEDED, false);
            if (saveResult) {
                showToast(R.string.contactSavedToast);
            }
        }
        /**
         * @}
         */
    }
    /**
    * SPRD:Bug422864
    *
    * @{
    */
    class UpdateGroupNotification {
        private NotificationManager mNm = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        private NotificationCompat.Builder mBuilder;
        private int mMax;
        private int mCurrentProcess;
        private String mEditingTitle;
        private String mDoneTitle;
        private String mFailedTitle;
        private Context mContext;

        public UpdateGroupNotification(int max) {
            mContext = ContactSaveService.this;
            mMax = max;
            mBuilder = new NotificationCompat.Builder(mContext);
            mEditingTitle = getResources().getString(R.string.working_editing_group);
            mDoneTitle = getResources().getString(R.string.editing_group_done);
            mFailedTitle = getResources().getString(R.string.editing_group_failed);
        }

        public void start() {
            mNm.notify("editing_group", 0, mBuilder
                    .setAutoCancel(false)
                    .setContentTitle(mEditingTitle)
                    .setTicker(mEditingTitle)
                    .setSmallIcon(android.R.drawable.stat_sys_warning)
                    .setProgress(mMax, 0, false)
                    .setContentIntent(PendingIntent.getActivity(mContext, 0, new Intent(), 0))
                    .setOngoing(true)
                    .build());
        }

        public void stop(boolean succ) {
            String title = null;
            if (succ) {
                title = mDoneTitle;
            } else {
                title = mFailedTitle;
            }

            mNm.notify("editing_group", 0, mBuilder
                    .setAutoCancel(true)
                    .setContentTitle(title)
                    .setTicker(title)
                    .setSmallIcon(android.R.drawable.stat_sys_warning)
                    .setProgress(0, 0, false)
                    .setContentIntent(PendingIntent.getActivity(mContext, 0, new Intent(), 0))
                    .setOngoing(false)
                    .build());
            mIsGroupSaving = false;
        }

        public void updateProgress(int progress) {
            mCurrentProcess += progress;

            mNm.notify("editing_group", 0, mBuilder
                    .setAutoCancel(false)
                    .setContentTitle(mEditingTitle)
                    .setTicker(mEditingTitle)
                    .setSmallIcon(android.R.drawable.stat_sys_warning)
                    .setProgress(mMax, mCurrentProcess, false)
                    .setContentIntent(PendingIntent.getActivity(mContext, 0, new Intent(), 0))
                    .setOngoing(true)
                    .build());
        }
    }

    /**
     * @}
     */
}
