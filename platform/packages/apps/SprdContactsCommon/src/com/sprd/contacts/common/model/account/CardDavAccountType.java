package com.sprd.contacts.common.model.account;

import com.android.contacts.common.model.account.BaseAccountType;

public class CardDavAccountType extends BaseAccountType{

    public static final String ACCOUNT_TYPE = "com.sprd.carddav.account";

    @Override
    public boolean areContactsWritable() {
        return true;
    }
}
