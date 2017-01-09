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

package com.android.contacts.activities;

import com.android.contacts.R;
import com.android.contacts.common.activity.ContactEditorRequestPermissionActivity;
import com.android.contacts.editor.ContactEditorFragment;
import com.android.contacts.util.DialogManager;
import com.android.contacts.common.activity.RequestPermissionsActivity;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
/**
 * SPRD:
 * @{
 */
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.widget.Toast;
import android.content.pm.PackageManager;
/**
 * @}
 */

/**
 * Contact editor with all fields displayed.
 */
public class ContactEditorActivity extends ContactEditorBaseActivity
        implements DialogManager.DialogShowingViewActivity {

    @Override
    public void onCreate(Bundle savedState) {
        super.onCreate(savedState);

        /**
         * sprd Bug518811 couldn't create new contact through dialpad interface
         * Original Android code:
        if (RequestPermissionsActivity.startPermissionActivity(this)) {
            return;
        }
         *@{
         */

        /**
         *SPRD: Bug594617 The Contacts app stops running after closing permissons
         * of Contacts from editing screen and fixing the screen
         * Qriginal Android code:
         if (RequestPermissionsActivity.startPermissionActivity(this)) {
         return;
         }
         *@{
         */
        for (String permission : RequestPermissionsActivity.requiredPermission()) {
            if (checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                finish();
                return;
            }
        }
        /**
         * @}
         */
        
        /**
         * @}
         */

        setContentView(R.layout.contact_editor_activity);

        mFragment = (ContactEditorFragment) getFragmentManager().findFragmentById(
                R.id.contact_editor_fragment);
        mFragment.setListener(mFragmentListener);

        final String action = getIntent().getAction();
        final Uri uri = ContactEditorBaseActivity.ACTION_EDIT.equals(action)
                || Intent.ACTION_EDIT.equals(action) ? getIntent().getData() : null;
        mFragment.load(action, uri, getIntent().getExtras());
    }

    /**
     * SPRD:
     *
     * @{
     */
    private boolean mCreatedNewGroup;
    public void setCreateNewGroup(boolean createNewGroup) {
        mCreatedNewGroup = createNewGroup;
    }

    public boolean getCreateNewGroup() {
        return mCreatedNewGroup;
    }
    /**
     * @}
     */

    /**
     * SPRD:Bug498681 When new a namesake group in edit page, pop a tip.
     * @{
     */
    private BroadcastReceiver groupBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Toast.makeText(ContactEditorActivity.this,
                    R.string.contacts_group_name_exist, Toast.LENGTH_LONG).show();
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
    /**
     * @}
     */
}
