
package com.sprd.contacts.activities;

import android.app.ActionBar;
import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.RawContactsMerge;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import com.android.contacts.R;
import com.android.contacts.editor.AggregationSuggestionEngine.RawContact;
import com.android.contacts.common.dialog.IndeterminateProgressDialog;

import com.sprd.contacts.DeduplicationCandidate;
import com.sprd.contacts.common.model.account.PhoneAccountType;
import com.sprd.contacts.list.DeduplicateCandidatesAdapter;
import com.sprd.contacts.list.DeduplicateCandidatesAdapter.RawContactWitAccount;
import com.sprd.contacts.list.GroupCheckAdapter;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;


import android.app.ActivityManagerNative;
import com.android.contacts.common.activity.RequestPermissionsActivity;

public class ContactDeduplicationActivity extends
        GroupCheckActivity<DeduplicationCandidate> {

    private static final String TAG = "ContactDeduplicationActivity";
    private static final int REQUEST_REMOVE_IDENTICAL = REQUEST_EXTEND_BASE + 1;
    private static final int REQUEST_MERGE_SIMILAR = REQUEST_EXTEND_BASE + 2;

    private static final int RESPONSE_REMOVE_IDENTICAL = RESPONSE_EXTEND_BASE + 1;
    private static final int RESPONSE_MERGE_SIMILAR = RESPONSE_EXTEND_BASE + 2;

    private static final int MERGE_MAX_OPERATION = 400;
    private static boolean MERGE_CONTACTS_IN_DIFFERENT_ACCOUNTS = false;

    private IndeterminateProgressDialog mProcessDialog;
    ContentObserver mContentObserver;
    private PowerManager mPowerManager = null;
    private PowerManager.WakeLock mWakeLock = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mContentObserver == null) {
            mContentObserver = new ContentObserver(mHander) {
                @Override
                public void onChange(boolean selfChange) {
                    Log.d(TAG, "on dup data change");
                    // bug 608059 can't refresh the list after merge duplicate contacts start
                    if (mArray != null) {
                        mArray = null;
                    }
                    requestDataAccess(REQUEST_QUERY_ITEMS, null);
                }
            };
        }
        getContentResolver().registerContentObserver(ContactsContract.AUTHORITY_URI, true,
                mContentObserver);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                onBackPressed();
                return true;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    // bug 394723 begin; Bug461717 Checked items not updated when some dup contacts are deleted.
    @Override
    protected void onDestroy() {
        super.onDestroy();
        getContentResolver().unregisterContentObserver(mContentObserver);
    }
    // bug 394723 end

    @Override
    protected GroupCheckAdapter<DeduplicationCandidate> createListAdapter() {
        return new DeduplicateCandidatesAdapter(this);
    }

    @Override
    protected void handleRequest(int what, Object obj) {
        switch (what) {
            case REQUEST_REMOVE_IDENTICAL:
                removeIdenticalContact();
                break;
            case REQUEST_MERGE_SIMILAR:
                mergeSimilarEntry((SparseArray<ArrayList<Object>>) obj);
                break;
            default:
                break;
        }
    }

    @Override
    protected void handleResponse(int what) {
        switch (what) {
            case RESPONSE_REMOVE_IDENTICAL:
                requestDataAccess(REQUEST_QUERY_ITEMS, null);
                break;
            case RESPONSE_MERGE_SIMILAR: {
                dismissWaitingDialog();
                
                // bug 606143 end
                break;
            }
            default:
                break;
        }

    }
   //SPRD:613721  contacts stop running after closed permission when lock the screen begin
    public boolean isInLockTaskMode() {
        try {
            return ActivityManagerNative.getDefault().isInLockTaskMode();
        } catch (Exception e) {
            return false;
        }
    }
   //SPRD:613721  contacts stop running after closed permission when lock the screen end
    @Override
    public void onBackPressed() {
   //SPRD:613721  contacts stop running after closed permission when lock the screen
        if (isInLockTaskMode()) {
            RequestPermissionsActivity.startPermissionActivity(this);
        } else {
            super.onBackPressed();
        }
    }
    @Override
    protected ArrayList<ArrayList<DeduplicationCandidate>> getItems() {
        Cursor cursor = null;
        Uri uri = Data.CONTENT_URI.buildUpon()
                .appendPath("deduplication_candidates").build();
        ArrayList<ArrayList<DeduplicationCandidate>> result = new ArrayList<ArrayList<DeduplicationCandidate>>();
        final String[] projection = new String[] {
                Data.RAW_CONTACT_ID,
                Data.DATA1,
                Data.PHOTO_ID,
                Data.DISPLAY_NAME_PRIMARY,
                RawContacts.ACCOUNT_ID,
                RawContacts.ACCOUNT_TYPE,
                RawContacts.ACCOUNT_NAME
        };
        ContentResolver resolver = getContentResolver();
        String orderBy = Data.DISPLAY_NAME + "," + RawContacts.ACCOUNT_ID + " asc, "
                + Data.PHOTO_ID + " desc";
        cursor = resolver.query(uri, projection, null, null, orderBy);
        int count = 0;
        String nameTemp = null;
        long accountIdTemp = -1;
        try {
            long rawContactId;
            String number;
            long photoId;
            String name;
            long accountId;
            String accoutName;
            String labelPhone = getString(R.string.label_phone);
            if (cursor != null && cursor.moveToFirst()) {

                ArrayList<DeduplicationCandidate> candidateGroup = new ArrayList<DeduplicationCandidate>();
                do {
                    rawContactId = cursor.getLong(0);
                    number = cursor.getString(1);
                    photoId = cursor.getLong(2);
                    name = cursor.getString(3);
                    accountId = cursor.getShort(4);
                    String accountType = cursor.getString(5);
                    if(PhoneAccountType.ACCOUNT_TYPE.equals(cursor.getString(5))) {
                        accoutName = labelPhone;
                    }else {
                        accoutName = cursor.getString(6);
                    }
                    DeduplicationCandidate canditae = new DeduplicationCandidate(
                            rawContactId, name, number, photoId, accountId, accoutName);
                    if (count == 0) {
                        candidateGroup.add(canditae);
                        nameTemp = name;
                        accountIdTemp = accountId;
                        count++;
                    } else if (TextUtils.equals(nameTemp, name) && accountIdTemp == accountId) {
                        candidateGroup.add(canditae);
                    } else {
                        /* SPRD: 547646 The DUT shows single deduplication contact for merging.*/
                        if (candidateGroup.size() > 1) {
                            result.add(candidateGroup);
                        }
                        candidateGroup = new ArrayList<DeduplicationCandidate>();
                        candidateGroup.add(canditae);
                        nameTemp = name;
                        accountIdTemp = accountId;
                    }
                } while (cursor.moveToNext());
                /* SPRD: 547646 The DUT shows single deduplication contact for merging.*/
                if (candidateGroup.size() > 1) {
                    result.add(candidateGroup);
                }
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return result;
    }

    @Override
    protected void onDataAccessReady() {
        super.onDataAccessReady();
        requestDataAccess(REQUEST_REMOVE_IDENTICAL, null);
    }

    @Override
    protected int getDoneResId() {
        return R.string.merge_contact;
    }

    @Override
    protected void onDonePressed(SparseArray<ArrayList<Object>> checkedItems) {
        if (checkedItems.size() > 0) {
            showWaitingDialog();
            requestDataAccess(REQUEST_MERGE_SIMILAR, checkedItems);
        }
    }

    private void removeIdenticalContact() {
        Cursor dupCountCursor = null;
        int dupCount = 0;
        try {
            //SPRD: add for bug632067, fdn contacts can not join
            dupCountCursor = getContentResolver().query(ContactsContract.RawContacts.CONTENT_URI,
                    new String[]{ContactsContract.RawContacts._ID, ContactsContract.RawContacts.CONTACT_ID},
                    " deleted=0 and account_type not in ('sprd.com.android.account.sim', 'sprd.com.android.account.usim') " +
                    " and (sync2 not like 'fdn%' or sync2 is null) " +
                    " and _id not in (select _id from raw_contacts where deleted=0 group by account_id, sync4 )",
                    null, null);
            dupCount = dupCountCursor.getCount();
        } finally {
            if(dupCountCursor != null){
                dupCountCursor.close();
            }
        }
        if(dupCount > 0){
            mHander.post(new Runnable() {

                @Override
                public void run() {
                    /**
                      * SPRD:Bug545218 Low frequently report Contact crash when contacts merged
                      * Original Android code:
                     showWaitingDialog();
                      *
                      * @{
                      */
                     try {
                         showWaitingDialog();
                     } catch (Exception e) {
                         // TODO: handle exception
                         dismissWaitingDialog();
                     }
                     /**
                      * @}
                      */
                }
            });
        }
        Uri uri = Uri.withAppendedPath(ContactsContract.AUTHORITY_URI,
                "raw_contacts/unique_nosim");
        ContentValues valuse = new ContentValues();
        getContentResolver().update(uri, valuse, null, null);
        if(dupCount > 0){
            mHander.post(new Runnable() {
                @Override
                public void run() {
                    dismissWaitingDialog();
                }
            });
        }
        responseForDataAccess(RESPONSE_REMOVE_IDENTICAL);
    }

    private void mergeSimilarEntry(
            SparseArray<ArrayList<Object>> selectedItems) {
        DeduplicateCandidatesAdapter adapter = (DeduplicateCandidatesAdapter) getAdapter();
        int count = adapter.getCount();

        ArrayList<ContentProviderOperation> operationList = new ArrayList<ContentProviderOperation>();

        try {
            mPowerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
            mWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "ContactDeduplication");
            mWakeLock.acquire();
        } catch (SecurityException e) {
            Log.w(TAG, "No permission to acquire wake lock");
            e.printStackTrace();
            mWakeLock = null;
        }

        try {
            for (int i = 0; i < count; i++) {
                ArrayList<Object> itemGroup = selectedItems.get(i);
                StringBuilder sb = new StringBuilder();
                if (itemGroup != null) {
                    // assume the list of RawContactWitAccount is ordered by
                    // account
                    // id
                    long lastAccountId = -1;
                    long targetRawContactId = -1;
                    ArrayList<Long> rawConactIdToJoin = new ArrayList<Long>();
                    ArrayList<Long> rawContactIdToMerge = new ArrayList<Long>();
                    for (Object l : itemGroup) {
                        RawContactWitAccount rawContact = (RawContactWitAccount) l;

                        if (rawContact.accountId == lastAccountId) {
                            rawContactIdToMerge.add(rawContact.id);
                        } else if (lastAccountId == -1) {
                            lastAccountId = rawContact.accountId;
                            targetRawContactId = rawContact.id;
                            if (MERGE_CONTACTS_IN_DIFFERENT_ACCOUNTS) {
                                rawConactIdToJoin.add(rawContact.id);
                            }
                        } else {
                            if (MERGE_CONTACTS_IN_DIFFERENT_ACCOUNTS) {
                                rawConactIdToJoin.add(rawContact.id);
                            }
                            operationList.add(buildMergeOperation(targetRawContactId,
                                    rawContactIdToMerge));
                            lastAccountId = rawContact.accountId;
                            rawContactIdToMerge.clear();
                            targetRawContactId = rawContact.id;

                        }
                    }

                    if (!rawContactIdToMerge.isEmpty()) {
                        operationList.add(buildMergeOperation(targetRawContactId,
                                rawContactIdToMerge));
                    }

                    if (MERGE_CONTACTS_IN_DIFFERENT_ACCOUNTS) {
                        if (rawConactIdToJoin.size() > 1) {
                            operationList.add(buildJoinOperation(rawConactIdToJoin));
                        }

                    }

                    if (operationList.size() > MERGE_MAX_OPERATION) {
                        getContentResolver().applyBatch(ContactsContract.AUTHORITY, operationList);
                    }
                }
            }

            if (operationList.size() > 0) {
                getContentResolver().applyBatch(ContactsContract.AUTHORITY, operationList);
            }

        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            responseForDataAccess(RESPONSE_MERGE_SIMILAR);
            if (mWakeLock != null) {
                Log.i(TAG, "mergeSimilarEntry : release wake lock ");
                mWakeLock.release();
                mWakeLock = null;
            }
        }

    }

    private ContentProviderOperation buildJoinOperation(ArrayList<Long> rawContactIdArray) {
        return null;
    }

    private ContentProviderOperation buildMergeOperation(long targtId,
            ArrayList<Long> rawContactIdArray) {
        Uri uri = Uri.withAppendedPath(ContactsContract.AUTHORITY_URI,
                "raw_contacts/merge");
        int count = rawContactIdArray.size();
        ContentProviderOperation.Builder builder = ContentProviderOperation.newUpdate(uri)
                .withYieldAllowed(false).withValue(RawContactsMerge.TARGET_RAW_CONTACT_ID, targtId)
                .withValue(RawContactsMerge.SUBJECT_COUNT, count);
        for (int i = 0; i < count; i++) {
            builder.withValue(RawContactsMerge.getSubjectKey(i), rawContactIdArray.get(i));
        }

        return builder.build();
    }

    private void showWaitingDialog() {
        mProcessDialog = IndeterminateProgressDialog.show(
                getFragmentManager(), getString(R.string.clearup_contacts),
                getString(R.string.merge_inprocess), 500);
    }

    private void dismissWaitingDialog() {
        if (!isFinishing() && mProcessDialog != null) {
            mProcessDialog.dismiss();
            mProcessDialog = null;
        }
    }

}
