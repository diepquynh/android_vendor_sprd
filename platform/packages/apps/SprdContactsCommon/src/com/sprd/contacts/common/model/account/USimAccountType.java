/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.sprd.contacts.common.model.account;

import com.android.contacts.common.R;
import com.android.contacts.common.model.account.AccountType.DefinitionException;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.ContentValues;
import android.content.Context;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.CommonDataKinds.Nickname;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.CommonDataKinds.Email;

import android.util.Log;

import java.util.Locale;
import java.util.ArrayList;
import com.android.contacts.common.model.account.BaseAccountType;
import com.android.contacts.common.model.dataitem.DataKind;

/* SPRD:443449 orange_ef anr/aas/sne @{ */
import com.android.contacts.common.model.AccountTypeManager.SimAas;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.internal.telephony.TeleFrameworkFactory;
import com.google.common.collect.Lists;
import com.sprd.contacts.common.plugin.EFDisplaySupportUtils;
/* @} */

public class USimAccountType extends SimAccountType {
    private static final String TAG = "USimAccountType";

    public static final String ACCOUNT_TYPE = "sprd.com.android.account.usim";

    private Context context = null;

    public USimAccountType(Context context, String resPackageName) {
        super(context, resPackageName);
        this.context = context;
        this.accountType = ACCOUNT_TYPE;
        this.resourcePackageName = resPackageName;
        this.syncAdapterPackageName = resPackageName;

        try {
            /* SPRD:443449 orange_ef anr/aas/sne @{ */
            int sneSize = AccountTypeManager.getInstance(context).getSneSize();
            if ((sneSize > 0) && EFDisplaySupportUtils.getInstance().isEFDisplaySupport()) {
                addDataKindNickname(context);
            }
            /* @} */
            if (isGroupMembershipEditable())
                addDataKindGroupMembership(context);
            if (isEmaiFieldEditable())
                addDataKindEmail(context);
            mIsInitialized = true;
        } catch (DefinitionException e) {
            Log.e(TAG, "Problem building account type", e);
        }
    }

    @Override
    protected DataKind addDataKindEmail(Context context) throws DefinitionException {
        final DataKind kind = super.addDataKindEmail(context);

        kind.typeOverallMax = 1;
        kind.typeList = new ArrayList();

        kind.fieldList = new ArrayList();
        kind.fieldList.add(new EditField(Email.DATA, R.string.emailLabelsGroup, FLAGS_EMAIL));

        return kind;
    }

    /* SPRD:443449 orange_ef anr/aas/sne @{ */
    @Override
    protected DataKind addDataKindNickname(Context context) throws DefinitionException {
        final DataKind kind = super.addDataKindNickname(context);
        kind.typeOverallMax = 1;
        kind.fieldList = Lists.newArrayList();
        kind.fieldList.add(new EditField(Nickname.NAME, R.string.nicknameLabelsGroup, FLAGS_PERSON_NAME));
        return kind;
    }
    /* @} */

    @Override
    protected DataKind addDataKindStructuredName(Context context) throws DefinitionException {
        DataKind kind = addKind(new DataKind(StructuredName.CONTENT_ITEM_TYPE,
                R.string.nameLabelsGroup, -1, true));
        kind.actionHeader = new SimpleInflater(R.string.nameLabelsGroup);
        kind.actionBody = new SimpleInflater(Nickname.NAME);

        kind.typeOverallMax = 1;

        kind.fieldList = new ArrayList();
        kind.fieldList.add(new EditField(StructuredName.DISPLAY_NAME,
                R.string.full_name, FLAGS_PERSON_NAME).setOptional(false));
        return kind;
    }

    @Override
    protected DataKind addDataKindDisplayName(Context context) throws DefinitionException {
        DataKind kind = addKind(new DataKind(DataKind.PSEUDO_MIME_TYPE_DISPLAY_NAME,
                R.string.nameLabelsGroup, -1, true));
        kind.typeOverallMax = 1;
        kind.fieldList = new ArrayList();
        kind.fieldList.add(new EditField(StructuredName.DISPLAY_NAME,
                R.string.full_name, FLAGS_PERSON_NAME).setOptional(false));
        return kind;
    }

    @Override
    protected DataKind addDataKindPhoneticName(Context context) throws DefinitionException {
//        int phoneticName;
//            phoneticName = R.layout.phonetic_name_editor_view;
//        }
        DataKind kind = addKind(new DataKind(DataKind.PSEUDO_MIME_TYPE_PHONETIC_NAME,
                R.string.name_phonetic, -1, true));
        kind.actionHeader = new SimpleInflater(R.string.nameLabelsGroup);
        kind.actionBody = new SimpleInflater(Nickname.NAME);

        kind.typeOverallMax = 1;

        kind.fieldList = new ArrayList();
        kind.fieldList.add(new EditField(StructuredName.PHONETIC_FAMILY_NAME,
                R.string.name_phonetic_family, FLAGS_PHONETIC));
        kind.fieldList.add(new EditField(StructuredName.PHONETIC_GIVEN_NAME,
                R.string.name_phonetic_given, FLAGS_PHONETIC));

        return kind;
    }

    @Override
    protected DataKind addDataKindPhone(Context context) throws DefinitionException {
        final DataKind kind = super.addDataKindPhone(context);

        kind.typeColumn = Phone.TYPE;
        kind.typeOverallMax = 2;
        kind.typeList = new ArrayList();

        kind.typeList.add(buildPhoneType(Phone.TYPE_MOBILE).setSpecificMax(1));

         /*
         * SPRD:443449 orange_ef anr/aas/sne
         *
         * @orig: kind.typeList.add(buildPhoneType(Phone.TYPE_OTHER));
         *
         * @{
         */
        if (TeleFrameworkFactory.getInstance().isSupportOrange()) {
            ArrayList<SimAas> aasList = AccountTypeManager.getInstance(context)
                    .getAasList();
            int i = -1;
            if (aasList != null) {
                for (SimAas aas : aasList) {
                    Log.d(TAG, "aas.name = " + aas.name + ", aas.index = "
                            + aas.index);
                    kind.typeList.add(new EditType(i, i, aas.name, aas.index));
                    i--;
                }
            }
            kind.typeList.add(buildPhoneType(Phone.TYPE_CUSTOM).setSecondary(
                    true).setCustomColumn(Phone.LABEL));
        } else {
            kind.typeList.add(buildPhoneType(Phone.TYPE_FIXED_NUMBER));
        }
        /* @} */

        // for (int i=0;i<kind.typeOverallMax;++i) {
        // kind.typeList.add(buildPhoneType(Phone.TYPE_HOME+i).setSpecificMax(1));
        // }

        kind.fieldList = new ArrayList();
        kind.fieldList.add(new EditField(Phone.NUMBER, R.string.phoneLabelsGroup, FLAGS_PHONE));

        return kind;
    }

    @Override
    public boolean isGroupMembershipEditable() {
        AccountManager am = AccountManager.get(this.context);
        Account[] accountMatch = am.getAccountsByType(this.accountType);
        /* Bug458472 Add accountMatch length judgement to avoid exception throwing. @{ */
        if (accountMatch.length > 0) {
        /* @} */
            String length = am.getUserData(accountMatch[0], GroupMembership.CONTENT_ITEM_TYPE
                    + "_capacity");
            /* SPRD: modify for bug441633 @{ */
            if (null != length) {
                int groupCapacity = Integer.parseInt(length);
                if (groupCapacity > 0) {
                    return true;
                }
            }
            /* @} */
        }

        return false;
    }

    @Override
    public boolean areContactsWritable() {
        return true;
    }

    public boolean isEmaiFieldEditable() {
        AccountManager am = AccountManager.get(this.context);
        Account[] accountMatch = am.getAccountsByType(this.accountType);
        if (accountMatch.length > 0) {
            String length = am.getUserData(accountMatch[0], Email.CONTENT_ITEM_TYPE + "_capacity");
            if (null != length) {
                int emailCapacity = Integer.parseInt(length);
                if (emailCapacity > 0) {
                    return true;
                }
            }
        }
        return false;
    }
}
