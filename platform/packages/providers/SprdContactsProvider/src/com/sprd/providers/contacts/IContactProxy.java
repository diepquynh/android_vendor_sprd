
package com.sprd.providers.contacts;

import android.accounts.Account;
import android.content.ContentValues;
import java.lang.IllegalStateException;

public interface IContactProxy {
    public void onImport(Account account);

    ContentValues insert(long rawContactId, Account account) throws IllegalStateException;

    void remove(long rawContactId);

    void update(long rawContactId) throws IllegalStateException;

    ContentValues insertGroup(long groupRowId, ContentValues values, Account account);

    void removeGroup(long groupRowId);

    void updateGroup(long groupRowId, ContentValues values);

    //rewrite the method, refers to DataRowHandlerForXXX
    void onDataUpdate(long rawContactId, ContentValues values, String mimeType);

    // bug 254366 optimize for restoring contacts to phone account from sd card
    void addToNonSimContactCache(long rawContactId);

    boolean isMyAccount(Account account);

}
