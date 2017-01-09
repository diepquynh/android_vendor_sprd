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

package com.android.contacts.common.preference;

import android.app.AlertDialog;
import android.content.Context;
import android.preference.ListPreference;
import android.util.AttributeSet;
import com.android.contacts.common.R;

import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.account.AccountType;
import com.android.contacts.common.model.account.AccountTypeWithDataSet;
import com.android.contacts.common.model.account.AccountWithDataSet;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.ArrayList;
import android.util.Log;

public class DefaultAccountPreference extends ListPreference {
    private ContactsPreferences mPreferences;
    private Map<String, AccountWithDataSet> mAccountMap;
    final static String PHONE_ACCOUNT_TYPE = "sprd.com.android.account.phone";
    /** SPRD:Bug610218  when setting default account is the phone, the name of the account is "Phone"
     * @{
     */
    final static String PHONE_ACCOUNT_NAME = "Phone";
    /**
     * @}
     */
    public DefaultAccountPreference(Context context) {
        super(context);
        prepare();
    }

    public DefaultAccountPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        prepare();
    }

    private void prepare() {
        mPreferences = new ContactsPreferences(getContext());
        mAccountMap = new HashMap<>();
        final AccountTypeManager accountTypeManager = AccountTypeManager.getInstance(getContext());
        /**
         * SPRD:Bug606105 when setting accounts,accounts show disorderly
         * @{
         */
        List<String> accountNamesList = new ArrayList<String>();
        List<AccountWithDataSet> accounts = accountTypeManager.getAccounts(true);
        /** SPRD:Bug610218  when setting default account is the phone, the name of the account is "Phone"
         * @{
         */
        for (AccountWithDataSet account : accounts) {
            boolean mIsPhoneAccount = account.type.equals(PHONE_ACCOUNT_TYPE);
            mAccountMap.put(account.name, account);
            if (mIsPhoneAccount) {
               accountNamesList.add(getContext().getResources().getString(R.string.label_phone));
            } else {
               accountNamesList.add(account.name);
            }
        }
        final String[] accountNamesArray = accountNamesList.toArray(new String[accountNamesList.size()]);
        setEntries(accountNamesArray);
        setEntryValues(accountNamesArray);
        String defaultAccount = String.valueOf(mPreferences.getDefaultAccount());
        if (PHONE_ACCOUNT_NAME.equals(defaultAccount)) {
            defaultAccount = getContext().getResources().getString(R.string.label_phone);
        }
        if (accounts.size() == 1) {
            if (accounts.get(0).type.equals(PHONE_ACCOUNT_TYPE)) {
                setValue(getContext().getResources().getString(R.string.label_phone));
            } else {
                setValue(accounts.get(0).name);
            }
        } else if (accountNamesList.contains(defaultAccount)) {
            setValue(defaultAccount);
        } else {
            setValue(null);
        }
        /**
         * @}
         */
        /**
         * @}
         */
    }

    @Override
    protected boolean shouldPersist() {
        return false;   // This preference takes care of its own storage
    }

    @Override
    public CharSequence getSummary() {
        /** SPRD:Bug610218  when setting default account is the phone, the name of the account is "Phone"
         * @{
         */
        if (PHONE_ACCOUNT_NAME.equals(mPreferences.getDefaultAccount())) {
            return getContext().getResources().getString(R.string.label_phone);
        }
        /**
         * @}
         */
        return mPreferences.getDefaultAccount();
    }

    @Override
    protected boolean persistString(String value) {
        if (value == null && mPreferences.getDefaultAccount() == null) {
            return true;
        }
        /** SPRD:Bug610218  when setting default account is the phone, the name of the account is "Phone"
         * @{
         */
        if (mAccountMap.get(value) == null && value != null && (value.equals(getContext().getResources().getString(R.string.label_phone)))) {
            mPreferences.setDefaultAccount(new AccountWithDataSet(PHONE_ACCOUNT_NAME,PHONE_ACCOUNT_TYPE, null));
            notifyChanged();
            return true;
        }
        /**
         * @}
         */
        if (value == null || mPreferences.getDefaultAccount() == null
                || !value.equals(mPreferences.getDefaultAccount())) {
            mPreferences.setDefaultAccount(mAccountMap.get(value));
            notifyChanged();
        }
        return true;
    }

    @Override
    // UX recommendation is not to show cancel button on such lists.
    protected void onPrepareDialogBuilder(AlertDialog.Builder builder) {
        super.onPrepareDialogBuilder(builder);
        builder.setNegativeButton(null, null);
    }
}
