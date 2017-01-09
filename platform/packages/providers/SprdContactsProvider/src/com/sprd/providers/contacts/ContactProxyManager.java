
package com.sprd.providers.contacts;

import android.accounts.Account;
import android.content.Context;
import android.content.ContentValues;
import java.lang.IllegalStateException;
import com.sprd.providers.contacts.SimContactProxy.SimContactCache;
import com.sprd.providers.contacts.SimContactProxy.SimContactGroupCache;

public class ContactProxyManager {
    private static ContactProxyManager sInstance;
    private SimContactProxy mSimContactProxy;

    private ContactProxyManager(Context context) {
        mSimContactProxy = new SimContactProxy(context);
    }

    synchronized public static ContactProxyManager getInstance(Context context) {
        if (sInstance == null) {
            sInstance = new ContactProxyManager(context);
        }
        return sInstance;
    }

    public IContactProxy getProxyForAccount(Account account) {
        if (mSimContactProxy.isMyAccount(account)) {
            return mSimContactProxy;
        }
        return null;
    }

    public SimContactCache getProxySimContactCache() {
        return mSimContactProxy.getSimContactProxySimContactCache();
    }

    public SimContactGroupCache getProxySimContactGroupCache() {
        return mSimContactProxy.getSimContactProxySimContactGroupCache();
    }

    public boolean isProxyImporting() {
        return mSimContactProxy.isImporting();
    }

    public void onImport(Account account) {
        if (mSimContactProxy.isMyAccount(account)) {
            mSimContactProxy.onImport(account);
            return;
        }
    }

    public ContentValues insert(long rawContactId, Account account) throws IllegalStateException {
        if (mSimContactProxy.isMyAccount(account)) {
            return mSimContactProxy.insert(rawContactId, account);
        }
        return null;
    }

    public void remove(long rawContactId) {
        mSimContactProxy.remove(rawContactId);
    }

    public void update(long rawContactId) throws IllegalStateException {
        mSimContactProxy.update(rawContactId);
    }

    public ContentValues insertGroup(long groupRowId, ContentValues values, Account account) {
        if (mSimContactProxy.isMyAccount(account)) {
            return mSimContactProxy.insertGroup(groupRowId, values, account);
        }
        return null;
    }

    public void removeGroup(long groupRowId) {
        mSimContactProxy.removeGroup(groupRowId);
    }

    public void updateGroup(long groupRowId, ContentValues values) {
        mSimContactProxy.updateGroup(groupRowId, values);
    }

    //rewrite the method, refers to DataRowHandlerForXXX
    public void onDataUpdate(long rawContactId, ContentValues values, String mimeType) {
        mSimContactProxy.onDataUpdate(rawContactId, values, mimeType);
    }

    public void addToNonSimContactCache(long rawContactId) {
        if (mSimContactProxy instanceof SimContactProxy) {
            mSimContactProxy.addToNonSimContactCache(rawContactId);
            return;
        }
    }

}
