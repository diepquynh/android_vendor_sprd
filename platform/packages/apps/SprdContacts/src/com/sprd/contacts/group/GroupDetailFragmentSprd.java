
package com.sprd.contacts.group;

import java.util.ArrayList;
import java.util.HashMap;

import android.app.Activity;
import android.app.LoaderManager;
import android.app.LoaderManager.LoaderCallbacks;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.CursorLoader;
import android.content.Intent;
import android.content.Loader;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.MergeCursor;
import android.os.Bundle;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.Contacts;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import com.android.contacts.ContactSaveService;
import com.android.contacts.ContactsApplication;
import com.android.contacts.R;
import com.android.contacts.GroupListLoader;
import com.android.contacts.GroupMemberLoader;
import com.android.contacts.list.GroupMemberTileAdapter;
import com.android.contacts.list.UiIntentActions;
import com.android.contacts.util.PhoneCapabilityTester;
import com.android.contacts.common.list.ContactTileAdapter;
import com.android.contacts.group.GroupDetailFragment;
import com.android.contacts.common.list.ContactEntry;
import android.content.ActivityNotFoundException;
import android.net.Uri;

public class GroupDetailFragmentSprd extends GroupDetailFragment {

    private static final int BULK_MMS = 3;
    private static final int BULK_EMAIL = 4;
    private static final int LIMIT_FOR_PROGRESS_DIALOG = 250;
    private static final int LOADER_GROUP = 6;

    public static final String SRC_GROUP_ID = "src_group_id";
    public static final String TARGET_GROUP_IDS = "target_group_ids";
    public static final String TARGET_GROUP_TITLES = "target_group_titles";

    private final String KEY_BULKMMSOREMAIL = "bulkMmsOrEmail_state";
    private final String KEY_BULKMODE = "bulk_mode";
    private final String KEY_HAS_OTHER_GROUP = "hasOtherGroup";

    private boolean mGroupSaving = false;
    private boolean mHasGroupMembers = false;

    private Context mContext;

    private HashMap<String, String> mMmsReceivers = new HashMap<String, String>();
    private ProgressDialog mProgressDialog;
    private ArrayList<String> mEmailReceivers = new ArrayList<String>();

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        mContext = activity;
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mContext = null;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedState) {

        if (savedState != null) {
            mIsBulkMmsOrEmail = savedState.getBoolean(KEY_BULKMMSOREMAIL);
            mBulkMode = savedState.getInt(KEY_BULKMODE);
            mHasOtherGroup = savedState.getBoolean(KEY_HAS_OTHER_GROUP);
        }
        return super.onCreateView(inflater, container, savedState);
    }

    /**
     * The listener for the group members list loader
     */


    @Override
    public void onPrepareOptionsMenu(Menu menu) {
        super.onPrepareOptionsMenu(menu);

        boolean optionsMenuGroupDeletable = isGroupDeletable() && isVisible();
        boolean optionsMenuGroupEditable = isGroupEditableAndPresent() && isVisible();

        final MenuItem mmsMenu = menu.findItem(R.id.menu_mms_group);
        final MenuItem emailMenu = menu.findItem(R.id.menu_email_group);
        final MenuItem moveMemberMenu = menu.findItem(R.id.menu_move_group_member);
        final MenuItem deleteMemberMenu = menu.findItem(R.id.menu_delete_group_member);

        if (getTileAdapter() != null) {
            mHasGroupMembers = getTileAdapter().getCount() > 0;
        }
        /**
         * SPRD:Bug 403071 597494 Send group mms cause ActivityNotFoundException on gms version
         * @{
         */
        if (mmsMenu != null) {
//            mmsMenu.setEnabled(optionsMenuGroupEditable && mHasGroupMembers && !mGroupSaving);
            mmsMenu.setVisible(optionsMenuGroupEditable && mHasGroupMembers && !mGroupSaving);
        }
        /**
         * @}
         */
        /**
         * SPRD: add for bug 379282 597494
         * orig:
        emailMenu.setEnabled(optionsMenuGroupEditable && mHasGroupMembers && !mGroupSaving);
         * @{
         */
        if(PhoneCapabilityTester.isIntentRegistered(mContext, new Intent("GROUP.EMAIL.SENDTO"))) {
//            emailMenu.setVisible(true);
//            emailMenu.setEnabled(optionsMenuGroupEditable && mHasGroupMembers && !mGroupSaving);
            emailMenu.setVisible(optionsMenuGroupEditable && mHasGroupMembers && !mGroupSaving);
        } else {
            emailMenu.setVisible(false);
        }
        /**
         * @}
         */

        /**
         * SPRD: BUG 594335
         * orig:
        moveMemberMenu.setEnabled(optionsMenuGroupEditable && !mGroupSaving && mHasGroupMembers
                && mHasOtherGroup);
        deleteMemberMenu.setEnabled(optionsMenuGroupEditable && mHasGroupMembers && !mGroupSaving);
         * @{
         */
        moveMemberMenu.setVisible(optionsMenuGroupEditable && !mGroupSaving && mHasGroupMembers
                && mHasOtherGroup);
        deleteMemberMenu.setVisible(optionsMenuGroupEditable && mHasGroupMembers && !mGroupSaving);
        /**
         * @}
         */
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        super.onOptionsItemSelected(item);

        switch (item.getItemId()) {
            case R.id.menu_mms_group: {
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving) {
                    Toast.makeText(getActivity(), R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    if (getTileAdapter().getCount() == 0) {
                        Toast.makeText(getActivity(), R.string.noContactsInGroup, Toast.LENGTH_SHORT)
                                .show();
                    } else {
                        bulkMmsOrEmail(BULK_MMS);
                    }
                }

                return true;
            }
            case R.id.menu_email_group: {
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving) {
                    Toast.makeText(getActivity(), R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    if (getTileAdapter().getCount() == 0) {
                        Toast.makeText(getActivity(), R.string.noContactsInGroup, Toast.LENGTH_SHORT)
                                .show();
                    } else {
                        bulkMmsOrEmail(BULK_EMAIL);
                    }
                }
                return true;
            }
            case R.id.menu_move_group_member: {
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving) {
                    Toast.makeText(getActivity(), R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    final Intent intent = new Intent(UiIntentActions.MULTI_PICK_ACTION);
                    intent.setData(Contacts.CONTENT_URI);
                    intent.putExtra("move_group_member", getGroupId());
                    intent.putExtra(SRC_GROUP_ID, getGroupId());
                    intent.putExtra(TARGET_GROUP_IDS, otherGroupIds);
                    intent.putExtra(TARGET_GROUP_TITLES, otherGroupTitles);
                    startActivity(intent);

                }
                break;
            }
            case R.id.menu_delete_group_member: {
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving) {
                    Toast.makeText(getActivity(), R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                    final Intent intent = new Intent(UiIntentActions.MULTI_PICK_ACTION);
                    intent.setData(Contacts.CONTENT_URI);
                    intent.putExtra("delete_group_member", getGroupId());
                    startActivity(intent);
                }
                break;
            }
        }
        return false;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putBoolean(KEY_BULKMMSOREMAIL, mIsBulkMmsOrEmail);
        outState.putInt(KEY_BULKMODE, mBulkMode);
        outState.putBoolean(KEY_HAS_OTHER_GROUP, mHasOtherGroup);
    }

    private ArrayList<String> getPhoneNumber(final String name) {
        final ArrayList<String> phoneNum = new ArrayList<String>();
        Cursor cursor = null;
        try {
            cursor = mContext.getContentResolver().query(Phone.CONTENT_URI, new String[] {
                    Phone.NUMBER
            }, Phone.DISPLAY_NAME + "=?",
                    new String[] {
                        name
                    }, null);
            if (cursor.moveToFirst()) {
                phoneNum.add(cursor.getString(0));
            }
            while (cursor.moveToNext()) {
                phoneNum.add(cursor.getString(0));
            }
        } finally {
            if (cursor != null)
                cursor.close();
        }
        return phoneNum;
    }

    public void bulkMmsOrEmail(int mode) {
        String title;
        mBulkMode = mode;
        String message = getString(R.string.please_wait);
        if (mode == BULK_MMS) {
            title = getString(R.string.menu_mmsGroup);
        } else {
            title = getString(R.string.menu_emailGroup);
        }
        mIsBulkMmsOrEmail = true;
        if (getTileAdapter().getCount() > LIMIT_FOR_PROGRESS_DIALOG) {
            mProgressDialog = ProgressDialog.show(getActivity(), title, message, true, false);
        }
        startGroupBulkLoader();
    }

    private void startGroupBulkLoader() {
        getLoaderManager().restartLoader(LOADER_GROUP, null, mGroupBulkLoaderListener);
    }

    private final LoaderManager.LoaderCallbacks<Cursor> mGroupBulkLoaderListener =
            new LoaderCallbacks<Cursor>() {

                @Override
                public CursorLoader onCreateLoader(int id, Bundle args) {
                    ArrayList<Integer> groupMembers = new ArrayList<Integer>();
                    int count = getTileAdapter().getCount();
                    int itemSize;
                    ContactEntry contactEntry;
                    for (int i = 0; i < count; i++) {
                        itemSize = getTileAdapter().getItem(i).size();
                        for (int j = 0; j < itemSize; j++) {
                            contactEntry = getTileAdapter().getItem(i).get(j);
                            if (contactEntry != null) {
                                groupMembers.add(contactEntry.contactId);
                            }
                        }
                    }
                    StringBuilder arg = new StringBuilder();
                    for (int i = 0; i < groupMembers.size(); i++) {
                        if (i > 0) {
                            arg.append(",");
                        }
                        arg.append(groupMembers.get(i));
                    }
                    if (mBulkMode == BULK_MMS) {
                        return new MmsBulkLoader(mContext, arg.toString());
                    } else {
                        return new EmailBulkLoader(mContext, arg.toString());
                    }
                }

                @Override
                public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
                    if (mBulkMode == BULK_MMS) {
                        if (data != null) {
                            if (data instanceof MergeCursor) {
                                MergeCursor cursor = (MergeCursor) data;
                                if (cursor.moveToFirst()) {
                                    do {
                                        mMmsReceivers.put(cursor.getString(0), cursor.getString(1));
                                    } while (cursor.moveToNext());
                                }
                            } else {
                                if (data.moveToFirst()) {
                                    do {
                                        mMmsReceivers.put(data.getString(0), null);
                                    } while (data.moveToNext());
                                }
                            }
                        }
                        if (mMmsReceivers.size() == 0) {
                            Toast.makeText(getActivity(), R.string.noContactsWithPhoneNumbers,
                                    Toast.LENGTH_SHORT).show();
                            mIsBulkMmsOrEmail = false;
                            if (mProgressDialog != null) {
                                mProgressDialog.dismiss();
                            }
                            getLoaderManager().destroyLoader(LOADER_GROUP);
                            return;
                        }
                        try {
                            final Intent intent = new Intent(Intent.ACTION_SENDTO);
                            String recipients = mMmsReceivers.keySet().toString()
                                    .replace("[", "").replace("]", "").replace(",", ";");
                            intent.setData(Uri.parse("smsto:" + recipients));
                            startActivity(intent);
                        } catch (ActivityNotFoundException e) {
                            Toast.makeText(getActivity(), R.string.noActivityHandle, Toast.LENGTH_SHORT).show();
                        }
                        mMmsReceivers.clear();
                        mIsBulkMmsOrEmail = false;
                        if (mProgressDialog != null) {
                            mProgressDialog.dismiss();
                        }
                    } else {
                        if (data != null && data.moveToFirst()) {
                            do {
                                String emailTo = "mailto:" + data.getString(0);
                                if (!mEmailReceivers.contains(emailTo)) {
                                    mEmailReceivers.add(emailTo);
                                }
                            } while (data.moveToNext());
                        }
                        if (mEmailReceivers.size() == 0) {
                            Toast.makeText(getActivity(), R.string.noContactswithEmail,
                                    Toast.LENGTH_SHORT).show();
                            mIsBulkMmsOrEmail = false;
                            if (mProgressDialog != null) {
                                mProgressDialog.dismiss();
                            }
                            getLoaderManager().destroyLoader(LOADER_GROUP);
                            return;
                        }
                        final Intent intent = new Intent();
                        intent.setAction("GROUP.EMAIL.SENDTO");
                        intent.putStringArrayListExtra("receivers", mEmailReceivers);
                        startActivity(intent);
                        mEmailReceivers.clear();
                        mIsBulkMmsOrEmail = false;
                        if (mProgressDialog != null) {
                            mProgressDialog.dismiss();
                        }
                    }
                    getLoaderManager().destroyLoader(LOADER_GROUP);
                }

                @Override
                public void onLoaderReset(Loader<Cursor> loader) {
                }
            };

    static String mArg;

    static class MmsBulkLoader extends CursorLoader {

        public MmsBulkLoader(Context context, String arg) {
            super(context);
            mArg = arg;
        }

        @Override
        public Cursor loadInBackground() {
            String hasPhoneNum = null;
            Cursor phoneCursor = getContext().getContentResolver().query(
                    Data.CONTENT_URI, new String[] {
                            Data.DATA1,
                    },
                    Data.CONTACT_ID + " IN (" + mArg + ")" + " AND " + Data.MIMETYPE + " ='" + Phone.CONTENT_ITEM_TYPE + "'",
                    null, null);
                return phoneCursor;
        }
    }

    static class EmailBulkLoader extends CursorLoader {
        public EmailBulkLoader(Context context, String arg) {
            super(context, Email.CONTENT_URI, new String[] {
                    Email.ADDRESS
            }, "contact_id in (" + arg + ")", null, null);
        }
    }
}
