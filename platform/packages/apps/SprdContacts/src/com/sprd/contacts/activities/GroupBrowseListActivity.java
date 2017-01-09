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

package com.sprd.contacts.activities;

import android.app.Fragment;
import android.app.FragmentManager;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcelable;

import android.provider.Settings;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract;
import com.android.contacts.ContactsActivity;
import com.android.contacts.R;
import com.android.contacts.activities.GroupDetailActivity;
import com.android.contacts.activities.GroupEditorActivity;
import com.android.contacts.common.activity.RequestPermissionsActivity;
import com.android.contacts.common.list.ContactEntryListFragment;
import com.android.contacts.common.list.ContactListFilter;
import com.android.contacts.common.list.ContactListFilterController;
import com.android.contacts.common.list.ContactTileAdapter.DisplayType;
import com.android.contacts.list.OnContactBrowserActionListener;
import com.android.contacts.common.util.AccountFilterUtil;
import com.android.contacts.common.util.Constants;
import java.util.ArrayList;
import java.util.HashMap;
import android.app.ActionBar;
import com.android.contacts.activities.ContactEditorBaseActivity.ContactEditor.SaveMode;
import com.android.contacts.activities.ContactEditorBaseActivity;
import com.android.contacts.common.util.ImplicitIntentsUtil;
import com.android.contacts.group.GroupBrowseListFragment.OnGroupBrowserActionListener;
import com.android.contacts.group.GroupBrowseListFragment;
import com.android.contacts.list.ContactsRequest;
import com.android.contacts.ContactsApplication;
import com.android.contacts.ContactSaveService;
import com.sprd.contacts.activities.DeleteGroupActivity;
import com.sprd.contacts.group.GroupBrowseListFragmentSprd;
import com.sprd.contacts.group.GroupDetailFragmentSprd;

/**
 * Displays a list to browse contacts.
 */
public class GroupBrowseListActivity extends ContactsActivity {

    private static final String TAG = "GroupBrowseListActivity";

    private GroupBrowseListFragment mGroupBrowseListFragment;
    private GroupBrowseListFragmentSprd mGroupsFragment;
    private static final int SUBACTIVITY_NEW_GROUP = 4;
    private static final int SUBACTIVITY_EDIT_GROUP = 5;
    private ContactsRequest mRequest;
    private final GroupDetailFragmentListener mGroupDetailFragmentListener =
            new GroupDetailFragmentListener();

    /**
     * Initialize fragments that are (or may not be) in the layout. For the fragments that are in
     * the layout, we initialize them in {@link #createViewsAndFragments(Bundle)} after inflating
     * the layout. However, the {@link ContactsUnavailableFragment} is a special fragment which may
     * not be in the layout, so we have to do the initialization here. The
     * ContactsUnavailableFragment is always created at runtime.
     */
    @Override
    protected void onCreate(Bundle savedState) {
        if (Log.isLoggable(Constants.PERFORMANCE_TAG, Log.DEBUG)) {
            Log.d(Constants.PERFORMANCE_TAG, "GroupBrowseListActivity.onCreate start");
        }

        super.onCreate(savedState);
        /**
         *  SPRD: Bug515700 Can not delete contacts group after close all the
         *  permissions of Contacts app.
         * @{
         */
        if (RequestPermissionsActivity.startPermissionActivity(this)) {
            return;
        }
        /*
         * @}
         */
        setContentView(R.layout.group_browse_overlay);

        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setTitle(getResources().getString(
                    R.string.groups_tab_label));
        }
        final FragmentManager fragmentManager = getFragmentManager();
        mGroupsFragment = (GroupBrowseListFragmentSprd)
                fragmentManager.findFragmentById(R.id.group_browse_list_fragment);
        createViewsAndFragments(savedState);

    }

    private void createViewsAndFragments(Bundle savedState) {
        mGroupsFragment.setListener(new GroupBrowserActionListener());
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {

        super.onCreateOptionsMenu(menu);

        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.group_browse_options, menu);

        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        final MenuItem deletaGroupMenu = menu.findItem(R.id.menu_delete_group);
        if (mGroupsFragment != null &&
                mGroupsFragment.getAdapter() != null &&
                mGroupsFragment.getAdapter().getCount() > 0) {
            deletaGroupMenu.setVisible(true);
        } else {
            deletaGroupMenu.setVisible(false);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home: {
                finish();
                return true;
            }
            case R.id.menu_accounts: {
                final Intent intent = new Intent(Settings.ACTION_SYNC_SETTINGS);
                intent.putExtra(Settings.EXTRA_AUTHORITIES, new String[] {
                        ContactsContract.AUTHORITY
                });
                intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                /**
                 * Sprd492768 Can't find the account setting activity.
                 *
                 * Original Android code:
                 ImplicitIntentsUtil.startActivityInApp(this,intent);
                 * @{
                 */
                ImplicitIntentsUtil.startActivityInAppIfPossible(this,intent);
                /**
                 * @}
                 */
                return true;
            }
            case R.id.menu_delete_group: {
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving) {
                    Toast.makeText(GroupBrowseListActivity.this,
                            R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                HashMap<Long, ArrayList<Uri>> hashMap = mGroupsFragment.getGroupPhotoUri();
                ArrayList<Long> groupId = mGroupsFragment.getGroupIdList();
                Bundle bundle = new Bundle();
                for (Long id : groupId) {
                    bundle.putParcelableArrayList(id.toString(), hashMap.get(id));
                }
                final Intent intent = new Intent(this, DeleteGroupActivity.class);
                intent.putExtra(DeleteGroupActivity.GROUP_PHOTO_URI, bundle);
                startActivity(intent);
                }
                return true;
            }
            case R.id.menu_add_group: {
                if (ContactsApplication.sApplication.isBatchOperation()
                        || ContactSaveService.mIsGroupSaving) {
                    Toast.makeText(GroupBrowseListActivity.this,
                            R.string.toast_batchoperation_is_running,
                            Toast.LENGTH_LONG).show();
                } else {
                createNewGroup();
                }
                return true;
            }
        }
        return false;
    }

    private void createNewGroup() {
        final Intent intent = new Intent(this, GroupEditorActivity.class);
        intent.setAction(Intent.ACTION_INSERT);
        startActivityForResult(intent, SUBACTIVITY_NEW_GROUP);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == SUBACTIVITY_NEW_GROUP) {
            if (resultCode == RESULT_OK /* && PhoneCapabilityTester.isUsingTwoPanes(this) */) {
                mGroupsFragment.setSelectedUri(data.getData());
            }
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    private final class GroupBrowserActionListener implements OnGroupBrowserActionListener {

        GroupBrowserActionListener() {
        }

        public void onBackPressed() {
            finish();
        }

        @Override
        public void onViewGroupAction(Uri groupUri) {
            /**
             * SPRD: Bug422623
             *
             * @{
             */
            if (ContactsApplication.sApplication.isBatchOperation()
                    || ContactSaveService.mIsGroupSaving) {
                Toast.makeText(GroupBrowseListActivity.this,
                        R.string.toast_batchoperation_is_running,
                        Toast.LENGTH_LONG).show();
                return;
            }
            /**
             * @}
             */
            Intent intent = new Intent(GroupBrowseListActivity.this, GroupDetailActivity.class);
            intent.setData(groupUri);
            startActivity(intent);
        }
    }

    private class GroupDetailFragmentListener implements GroupDetailFragmentSprd.Listener {

        GroupDetailFragmentListener() {
        }

        @Override
        public void onGroupSizeUpdated(String size) {
            // Nothing needs to be done here because the size will be displayed in the detail
            // fragment
        }

        @Override
        public void onGroupTitleUpdated(String title) {
            // Nothing needs to be done here because the title will be displayed in the detail
            // fragment
        }

        @Override
        public void onAccountTypeUpdated(String accountTypeString, String dataSet) {
            // Nothing needs to be done here because the group source will be displayed in the
            // detail fragment
        }

        @Override
        public void onEditRequested(Uri groupUri) {
            final Intent intent = new Intent(GroupBrowseListActivity.this,
                    GroupEditorActivity.class);
            intent.setData(groupUri);
            intent.setAction(Intent.ACTION_EDIT);
            startActivityForResult(intent, SUBACTIVITY_EDIT_GROUP);
        }

        @Override
        public void onContactSelected(Uri contactUri) {
            // Nothing needs to be done here because either quickcontact will be displayed
            // or activity will take care of selection
        }
    }
}

