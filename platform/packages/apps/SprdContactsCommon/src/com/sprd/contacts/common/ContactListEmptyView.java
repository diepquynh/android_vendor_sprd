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

package com.sprd.contacts.common;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.ContentResolver;
import android.content.Context;
import android.os.RemoteException;
import android.provider.ContactsContract;
import android.telephony.TelephonyManager;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.ScrollView;
import android.widget.TextView;
import com.android.contacts.common.R;

/**
 * Displays a message when there is nothing to display in a contact list.
 */
public class ContactListEmptyView extends ScrollView {

    private static final String TAG = "ContactListEmptyView";

    public ContactListEmptyView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void hide() {
        TextView empty = (TextView) findViewById(R.id.emptyText);
        empty.setVisibility(GONE);
    }

    public void show(boolean searchMode, boolean displayOnlyPhones,
            boolean isFavoritesMode, boolean isQueryMode, boolean isShortcutAction,
            boolean isMultipleSelectionEnabled, boolean showSelectedOnly) {
        if (searchMode) {
            return;
        }

        TextView empty = (TextView) findViewById(R.id.emptyText);
        Context context = getContext();
        if (displayOnlyPhones) {
            empty.setText(context.getText(R.string.listFoundAllContactsZero));
        } else if (isFavoritesMode) {
            empty.setText(context.getText(R.string.listTotalAllContactsZeroStarred));
        } else if (isQueryMode) {
            empty.setText(context.getText(R.string.no_match_contact));
        } if (isMultipleSelectionEnabled) {
            if (showSelectedOnly) {
                empty.setText(context.getText(R.string.listFoundAllContactsZero));
            } else {
                empty.setText(context.getText(R.string.listFoundAllContactsZero));
            }
        } else {
            TelephonyManager telephonyManager =
                    (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
            boolean hasSim = telephonyManager.hasIccCard();
            empty.setText(context.getText(R.string.listFoundAllContactsZero));
        }
        empty.setVisibility(VISIBLE);
    }

}
