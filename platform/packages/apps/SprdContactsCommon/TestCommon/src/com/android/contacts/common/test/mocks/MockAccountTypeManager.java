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
package com.android.contacts.common.test.mocks;

import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.account.AccountType;
import com.android.contacts.common.model.account.AccountTypeWithDataSet;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.model.account.BaseAccountType;
import com.google.common.base.Objects;
import com.google.common.collect.Lists;
import com.google.common.collect.Maps;

import java.util.Arrays;
import java.util.List;
import java.util.Map;

//sprd add
import java.util.ArrayList;
import android.graphics.drawable.Drawable;

/* SPRD:443449 orange_ef anr/aas/sne @{ */
import com.android.contacts.common.model.AccountTypeManager.SimAas;
import android.content.Context;
import android.net.Uri;
/* @} */

/**
 * A mock {@link AccountTypeManager} class.
 */
public class MockAccountTypeManager extends AccountTypeManager {

    public AccountType[] mTypes;
    public AccountWithDataSet[] mAccounts;

    public MockAccountTypeManager(AccountType[] types, AccountWithDataSet[] accounts) {
        this.mTypes = types;
        this.mAccounts = accounts;
    }

    @Override
    public AccountType getAccountType(AccountTypeWithDataSet accountTypeWithDataSet) {
        // Add fallback accountType to mimic the behavior of AccountTypeManagerImpl
        AccountType mFallbackAccountType = new BaseAccountType() {
            @Override
            public boolean areContactsWritable() {
                return false;
            }
        };
        mFallbackAccountType.accountType = "fallback";
        for (AccountType type : mTypes) {
            if (Objects.equal(accountTypeWithDataSet.accountType, type.accountType)
                    && Objects.equal(accountTypeWithDataSet.dataSet, type.dataSet)) {
                return type;
            }
        }
        return mFallbackAccountType;
    }

    @Override
    public List<AccountWithDataSet> getAccounts(boolean writableOnly) {
        return Arrays.asList(mAccounts);
    }

    @Override
    public List<AccountWithDataSet> getGroupWritableAccounts() {
        return Arrays.asList(mAccounts);
    }

    @Override
    public Map<AccountTypeWithDataSet, AccountType> getUsableInvitableAccountTypes() {
        return Maps.newHashMap(); // Always returns empty
    }

    @Override
    public List<AccountType> getAccountTypes(boolean writableOnly) {
        final List<AccountType> ret = Lists.newArrayList();
        synchronized (this) {
            for (AccountType type : mTypes) {
                if (!writableOnly || type.areContactsWritable()) {
                    ret.add(type);
                }
            }
        }
        return ret;
    }

    // SPRD:add for Account
    @Override
    public String getSimSlotName(int phoneId) {
        return null;
    }

    @Override
    public List<AccountWithDataSet> getSimAccounts() {
        return null;
    }

    @Override
    public AccountWithDataSet getPhoneAccount() {
        return null;
    }

    @Override
    public boolean isPhoneAccount(AccountWithDataSet account) {
        return false;
    }

    @Override
    public boolean isSimAccount(AccountWithDataSet account) {
        return false;
    }

    @Override
    public Drawable getAccountIcon(AccountWithDataSet account, boolean isSdn) {
        return null;
    }

    public ArrayList<AccountWithDataSet> getAccountsWithNoSim(boolean contactWritableOnly) {
        return null;
    }

    /*
     * SPRD: AndroidN porting add for new sim icon feature.
     * @see com.android.contacts.common.model.AccountTypeManager#getListSimIcon(String, String, boolean)
     * @{
     */
    @Override
    public Drawable getListSimIcon(String accountType, String accountName, boolean isSdn) {
        return null;
    }
    /*
     * @}
     */

    //SPRD: add for bug617830, add fdn feature
    @Override
    public Drawable getListFdnIcon(int phoneId) {
        return null;
    }

    /* SPRD:443449 orange_ef anr/aas/sne @{ */
    @Override
    public ArrayList<SimAas> getAasList() {
        return null;
    }

    @Override
    public int getSneSize() {
        return 0;
    }

    @Override
    public Uri insertAas(Context context, String aas) {
        return null;
    }

    @Override
    public boolean updateAas(Context context, String aasIndex, String aas) {
        return false;
    }

    @Override
    public boolean deleteAas(Context context, String aasIndex, String aas) {
        return false;
    }

    @Override
    public boolean findAasInContacts(Context context, String aasIndex, String aas) {
        return false;
    }
    /* @} */
}
