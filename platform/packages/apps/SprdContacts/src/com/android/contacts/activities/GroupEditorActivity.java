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
import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;

import com.android.contacts.ContactsActivity;
import com.android.contacts.R;
import com.android.contacts.group.GroupEditorFragment;
import com.android.contacts.util.DialogManager;
import com.android.contacts.util.PhoneCapabilityTester;
/**
 * SPRD
 * @{
 */
import android.content.BroadcastReceiver;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.content.IntentFilter;
import android.widget.Button;
import android.widget.Toast;
import com.android.contacts.ContactSaveService;
import com.android.contacts.activities.ContactEditorBaseActivity.ContactEditor.SaveMode;
import com.android.contacts.common.activity.ContactEditorRequestPermissionActivity;
import android.os.Handler;
import android.os.Looper;
import android.content.pm.PackageManager;
/**
 * @}
 */

public class GroupEditorActivity extends ContactsActivity
        implements DialogManager.DialogShowingViewActivity {

    private static final String TAG = "GroupEditorActivity";

    public static final String ACTION_SAVE_COMPLETED = "saveCompleted";
    public static final String ACTION_ADD_MEMBER_COMPLETED = "addMemberCompleted";
    public static final String ACTION_REMOVE_MEMBER_COMPLETED = "removeMemberCompleted";

    private GroupEditorFragment mFragment;

    private DialogManager mDialogManager = new DialogManager(this);
    //SPRD
    private boolean mBackPressed;
    /**
     * sprd Bug522250 during group editor interface, close Contacts permissions, back, no
     * permission dialog or toast
     * @{
     */
    private Handler mHandler = new Handler();
    private static boolean mIsNeedPromptPermissionMessage = false;
    private static boolean mIsAllPermissionSatisfied = true;
    private Handler mMainHandler;
    /**
     * @}
     */
    @Override
    public void onCreate(Bundle savedState) {
        super.onCreate(savedState);

        /**
         * sprd Bug522250 during group editor interface, close Contacts permissions, back, no
         * permission dialog or toast
         * @{
         */
        int mSatisfiedPermissionLength =
                ContactEditorRequestPermissionActivity.requiredPermission().length;
        for (String permission : ContactEditorRequestPermissionActivity.requiredPermission()) {
            if (checkSelfPermission(permission)
                    != PackageManager.PERMISSION_GRANTED) {
                mSatisfiedPermissionLength--;
            }
        }
        if (mSatisfiedPermissionLength != ContactEditorRequestPermissionActivity
                .requiredPermission().length) {
            mIsAllPermissionSatisfied = false;
        } else {
            mIsAllPermissionSatisfied = true;
        }

        mMainHandler = new Handler(Looper.getMainLooper());
        if (!mIsAllPermissionSatisfied) {
            needPromptMessage(!mIsAllPermissionSatisfied);
        }

        if (mIsNeedPromptPermissionMessage && mIsAllPermissionSatisfied) {
            showToast(R.string.re_edit_group);
            mIsNeedPromptPermissionMessage = false;
            mIsAllPermissionSatisfied = true;
        }

        if (ContactEditorRequestPermissionActivity.startPermissionActivity(this)) {
            return;
        }

        mIsNeedPromptPermissionMessage = false;
        mIsAllPermissionSatisfied = true;
        /**
         * @}
         */

        String action = getIntent().getAction();
        if (ACTION_SAVE_COMPLETED.equals(action)) {
            finish();
            return;
        }

        setContentView(R.layout.group_editor_activity);

        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            // Inflate a custom action bar that contains the "done" button for saving changes
            // to the group
            LayoutInflater inflater = (LayoutInflater) getSystemService
                    (Context.LAYOUT_INFLATER_SERVICE);
            View customActionBarView = inflater.inflate(R.layout.editor_custom_action_bar,
                    null);
            View saveMenuItem = customActionBarView.findViewById(R.id.save_menu_item);
            saveMenuItem.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    mFragment.onDoneClicked();
                }
            });
            // Show the custom action bar but hide the home icon and title
            actionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_CUSTOM,
                    ActionBar.DISPLAY_SHOW_CUSTOM | ActionBar.DISPLAY_SHOW_HOME |
                    ActionBar.DISPLAY_SHOW_TITLE);
            actionBar.setCustomView(customActionBarView);
        }

        mFragment = (GroupEditorFragment) getFragmentManager().findFragmentById(
                R.id.group_editor_fragment);
        mFragment.setListener(mFragmentListener);
        mFragment.setContentResolver(getContentResolver());

        // NOTE The fragment will restore its state by itself after orientation changes, so
        // we need to do this only for a new instance.
        if (savedState == null) {
            Uri uri = Intent.ACTION_EDIT.equals(action) ? getIntent().getData() : null;
            mFragment.load(action, uri, getIntent().getExtras());
        }else{
            finish();
        }

    }

    /**
     * sprd Bug522250 during group editor interface, close Contacts permissions, back, no
     * permission dialog or toast
     * @{
     */
    private void needPromptMessage(boolean isNeedPrompt) {
       if (isNeedPrompt) {
           mIsNeedPromptPermissionMessage = true;
       }
    }

    /**
     * Shows a toast on the UI thread.
     */
    private void showToast(final int message) {
        mMainHandler.post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(GroupEditorActivity.this, message, Toast.LENGTH_LONG).show();
            }
        });
    }
    /**
     * SPRD:Bug422623 Add for group editor.
     *
     * @{
     */
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
    }
    /**
     * @}
     */

    @Override
    protected Dialog onCreateDialog(int id, Bundle args) {
        if (DialogManager.isManagedId(id)) {
            return mDialogManager.onCreateDialog(id, args);
        } else {
            // Nobody knows about the Dialog
            Log.w(TAG, "Unknown dialog requested, id: " + id + ", args: " + args);
            return null;
        }
    }

    @Override
    public void onBackPressed() {
        // If the change could not be saved, then revert to the default "back" button behavior.
        /**
         * SPRD:Bug422623
         * Orig:
        if (!mFragment.save()) {
            super.onBackPressed();
        }
        *
        * @{
        */
        if (!mFragment.save(SaveMode.CLOSE, true)) {
        /**
         * @}
         */
            super.onBackPressed();
        } else {
            mBackPressed = true;
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);

        if (mFragment == null) {
            return;
        }

        String action = intent.getAction();
        /**
         * SPRD:Bug422623  Defer the action to make the window properly repaint.
         * Orig:
        if (ACTION_SAVE_COMPLETED.equals(action)) {
            mFragment.onSaveCompleted(true, intent.getData());
        }
         *
         * @{
         */
        int errorToast = intent.getIntExtra(ContactSaveService.EXTRA_ERROR_TOAST, 0);
        if (ACTION_SAVE_COMPLETED.equals(action)) {
            mFragment.onSaveCompleted(true, intent.getData(), errorToast);
        }
        /**
         * @}
         */
    }

    private final GroupEditorFragment.Listener mFragmentListener =
            new GroupEditorFragment.Listener() {
        @Override
        public void onGroupNotFound() {
            finish();
        }

        @Override
        public void onReverted() {
            finish();
        }

        @Override
        public void onAccountsNotFound() {
            finish();
        }

        @Override
        public void onSaveFinished(int resultCode, Intent resultIntent) {
            if (resultIntent != null) {
                Intent intent = new Intent(GroupEditorActivity.this, GroupDetailActivity.class);
                intent.setData(resultIntent.getData());
                intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
                startActivity(intent);
                /*SPRD:Bug 425303 Contacts crashed when add groups that exceeding SIM capacity.
                finish();
                 */
            }
            /**
             * SPRD:Bug 425303 Contacts crashed when add groups that exceeding SIM capacity.
             **/
            finish();
            /**
             * @}
             */
        }
    };

    @Override
    public DialogManager getDialogManager() {
        return mDialogManager;
    }

    /**
     * SPRD: Bug426174 Cannot create namesake groups of the same accounts.
     *
     * @{
     */
    private BroadcastReceiver groupBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Toast toast = Toast.makeText(GroupEditorActivity.this,
                    R.string.contacts_group_name_exist, Toast.LENGTH_LONG);
            toast.show();
            if (mBackPressed) {
                finish();
            }
        }
    };

    @Override
    public void onResume() {
        super.onResume();
        IntentFilter groupIntentFilter = new IntentFilter("groupname repeat");
        registerReceiver(groupBroadcastReceiver, groupIntentFilter);
    }

    @Override
    public void onPause() {
        super.onPause();
        unregisterReceiver(groupBroadcastReceiver);
    }

    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home: {

            }
        }
        return false;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.edit_group, menu);
        return true;
    }
    /**
     * @}
     */
}
