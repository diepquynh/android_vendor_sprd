/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.contacts.activities;

import android.app.ActionBar;
import android.content.ContentUris;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.ContactsContract.Groups;
import android.text.TextUtils;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;

import com.android.contacts.ContactsActivity;
import com.android.contacts.R;
import com.android.contacts.common.util.ImplicitIntentsUtil;
import com.android.contacts.group.GroupDetailDisplayUtils;
import com.android.contacts.group.GroupDetailFragment;
import com.android.contacts.common.activity.RequestPermissionsActivity;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.account.AccountType;
//SPRD
import com.sprd.contacts.activities.GroupBrowseListActivity;
import com.sprd.contacts.group.GroupDetailFragmentSprd;

public class GroupDetailActivity extends ContactsActivity {

    private static final String TAG = "GroupDetailActivity";

    private boolean mShowGroupSourceInActionBar;

    private String mAccountTypeString;
    private String mDataSet;

    /**
     * SPRD:Bug422623
     * Orig:
    private GroupDetailFragment mFragment;
     *
     *@{
     */
    private GroupDetailFragmentSprd mFragment;
    /**
     * @}
     */

    @Override
    public void onCreate(Bundle savedState) {
        super.onCreate(savedState);

        /**
         * SPRD: Bug 556808 The DUT occurs Contacts app crash while closing
         * permissions of Contacts in group detail screen.
         * @{
         */
        if (RequestPermissionsActivity.startPermissionActivity(this)) {
            return;
        }
        /*
         * @}
         */
        // TODO: Create Intent Resolver to handle the different ways users can get to this list.
        // TODO: Handle search or key down

        setContentView(R.layout.group_detail_activity);

        mShowGroupSourceInActionBar = getResources().getBoolean(
                R.bool.config_show_group_action_in_action_bar);

        /**
         * SPRD:Bug422623
         * Orig:
        mFragment = (GroupDetailFragment) getFragmentManager().findFragmentById(
                R.id.group_detail_fragment);
         *
         * @{
         */
        mFragment = (GroupDetailFragmentSprd) getFragmentManager().findFragmentById(
                R.id.group_detail_fragment);
        /* @} */
        mFragment.setListener(mFragmentListener);
        mFragment.setShowGroupSourceInActionBar(mShowGroupSourceInActionBar);
        mFragment.loadGroup(getIntent().getData());
        mFragment.closeActivityAfterDelete(true);

        // We want the UP affordance but no app icon.
        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            actionBar.setDisplayOptions(ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_TITLE,
                    ActionBar.DISPLAY_HOME_AS_UP | ActionBar.DISPLAY_SHOW_TITLE
                    | ActionBar.DISPLAY_SHOW_HOME);
        }
    }

    /**
     * SPRD:Bug422623
     * Orig:
    private final GroupDetailFragment.Listener mFragmentListener =
            new GroupDetailFragment.Listener() {
     *
     * @{
     */
    private final GroupDetailFragmentSprd.Listener mFragmentListener =
            new GroupDetailFragmentSprd.Listener() {
     /**
      * @}
      */

        @Override
        public void onGroupSizeUpdated(String size) {
            getActionBar().setSubtitle(size);
        }

        @Override
        public void onGroupTitleUpdated(String title) {
            getActionBar().setTitle(title);
        }

        @Override
        public void onAccountTypeUpdated(String accountTypeString, String dataSet) {
            mAccountTypeString = accountTypeString;
            mDataSet = dataSet;
            invalidateOptionsMenu();
        }

        @Override
        public void onEditRequested(Uri groupUri) {
            final Intent intent = new Intent(GroupDetailActivity.this, GroupEditorActivity.class);
            intent.setData(groupUri);
            intent.setAction(Intent.ACTION_EDIT);
            startActivity(intent);
        }

        @Override
        public void onContactSelected(Uri contactUri) {
            Intent intent = new Intent(Intent.ACTION_VIEW, contactUri);
            ImplicitIntentsUtil.startActivityInApp(GroupDetailActivity.this, intent);
        }

    };

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        if (mShowGroupSourceInActionBar) {
            MenuInflater inflater = getMenuInflater();
            inflater.inflate(R.menu.group_source, menu);
        }
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        if (!mShowGroupSourceInActionBar) {
            return false;
        }
        MenuItem groupSourceMenuItem = menu.findItem(R.id.menu_group_source);
        if (groupSourceMenuItem == null) {
            return false;
        }
        final AccountTypeManager manager = AccountTypeManager.getInstance(this);
        final AccountType accountType =
                manager.getAccountType(mAccountTypeString, mDataSet);
        if (TextUtils.isEmpty(mAccountTypeString)
                || TextUtils.isEmpty(accountType.getViewGroupActivity())) {
            groupSourceMenuItem.setVisible(false);
            return false;
        }
        View groupSourceView = GroupDetailDisplayUtils.getNewGroupSourceView(this);
        GroupDetailDisplayUtils.bindGroupSourceView(this, groupSourceView,
                mAccountTypeString, mDataSet);
        groupSourceView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                final Uri uri = ContentUris.withAppendedId(Groups.CONTENT_URI,
                        mFragment.getGroupId());
                final Intent intent = new Intent(Intent.ACTION_VIEW, uri);
                intent.setClassName(accountType.syncAdapterPackageName,
                        accountType.getViewGroupActivity());
                ImplicitIntentsUtil.startActivityInApp(GroupDetailActivity.this, intent);
            }
        });
        groupSourceMenuItem.setActionView(groupSourceView);
        groupSourceMenuItem.setVisible(true);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                /**
                 * SPRD:Bug425152 On group detail back to GroupBrowseListActivity.
                 * Orig:
                Intent intent = new Intent(this, PeopleActivity.class);
                 * @{
                 */
                Intent intent = new Intent(this, GroupBrowseListActivity.class);
                /**
                 * @}
                 */
                intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                startActivity(intent);
                finish();
                return true;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }
}
