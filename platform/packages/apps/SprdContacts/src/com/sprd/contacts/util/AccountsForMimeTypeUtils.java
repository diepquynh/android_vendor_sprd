
package com.sprd.contacts.util;

import android.content.ContentValues;
import android.content.Context;
import android.os.Bundle;
import android.provider.ContactsContract.CommonDataKinds;
import android.util.Log;
import com.android.contacts.common.model.account.AccountWithDataSet;
import com.android.contacts.common.model.AccountTypeManager;
import com.android.contacts.common.model.RawContactModifier;
import com.android.contacts.common.model.RawContactDelta;
import com.android.contacts.common.model.ValuesDelta;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

public class AccountsForMimeTypeUtils {
    private static final String TAG = AccountsForMimeTypeUtils.class.getSimpleName();

    public static ArrayList<AccountWithDataSet> getAccountsForMimeType(Context context,
            Bundle extras) {
        AccountTypeManager accountManager = AccountTypeManager.getInstance(context);
        ArrayList<AccountWithDataSet> ret = new ArrayList<AccountWithDataSet>();
        ArrayList<AccountWithDataSet> accounts = (ArrayList)accountManager.getAccounts(true);
        Log.i(TAG, "getAccountsForMimeType:" + (extras == null ? " null" : extras.toString())
                + " accounts = " + accounts);
        if (extras == null) {
            return accounts;
        }
        for (AccountWithDataSet account : accounts) {
            final ContentValues values = new ContentValues();
            RawContactDelta insert = new RawContactDelta(ValuesDelta.fromAfter(values));
            RawContactModifier.parseExtras(context,
                    accountManager.getAccountTypeForAccount(account), insert, extras);
            Set<String> supportedMimeTypes = insert.getMimeTypes();
            Log.i(TAG, "supportedMimeTypes:" + supportedMimeTypes);
            supportedMimeTypes.remove(CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE);
            if (!supportedMimeTypes.isEmpty()) {
                ret.add(account);
            }
        }
        Log.i(TAG, "getAccountsForMimeType: the result accounts obtained after treatment is "
                + ret);
        return ret;
    }

    public static boolean isAllAccountsForMimeType(Context context, Bundle extras) {
        AccountTypeManager accountManager = AccountTypeManager.getInstance(context);
        List<AccountWithDataSet> allAccounts = accountManager.getAccounts(true);
        ArrayList<AccountWithDataSet> accountsForMimeType = getAccountsForMimeType(context, extras);
        if (allAccounts != null && accountsForMimeType != null) {
            return allAccounts.size() == accountsForMimeType.size();
        }
        return false;
    }
}
