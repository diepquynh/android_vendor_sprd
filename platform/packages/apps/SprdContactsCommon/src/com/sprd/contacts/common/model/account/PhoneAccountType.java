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
import com.android.contacts.common.model.account.FallbackAccountType;
import com.android.contacts.common.model.dataitem.DataKind;

import android.content.ContentValues;
import android.content.Context;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.Event;
import android.provider.ContactsContract.CommonDataKinds.Im;
import android.provider.ContactsContract.CommonDataKinds.Nickname;
import android.provider.ContactsContract.CommonDataKinds.Note;
import android.provider.ContactsContract.CommonDataKinds.Organization;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.Photo;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.CommonDataKinds.StructuredPostal;
import android.provider.ContactsContract.CommonDataKinds.Website;
import android.util.Log;

import java.util.Locale;

public class PhoneAccountType extends FallbackAccountType {
    public static final String ACCOUNT_TYPE = "sprd.com.android.account.phone";
    private static final String TAG = "PhoneAccountType";

    public PhoneAccountType(Context context, String resPackageName) {
        super(context, resPackageName);
        try {
            addDataKindGroupMembership(context);
        } catch (DefinitionException e) {
            Log.e(TAG, "Problem building account type", e);
        }
    }

    @Override
    public boolean isGroupMembershipEditable() {
        return true;
    }

}
