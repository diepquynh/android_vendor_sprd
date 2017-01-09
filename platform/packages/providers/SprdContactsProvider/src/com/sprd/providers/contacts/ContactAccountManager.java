
package com.sprd.providers.contacts;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.content.Context;
import android.os.Debug;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.util.Log;
import android.os.SystemProperties;

import com.google.android.collect.Lists;

import java.util.List;
import java.util.concurrent.CountDownLatch;

/**
 * SPRD: Singleton holder for all parsed Accounts available on the system, typically filled through
 * {@link AccountManager} queries.
 * 
 * @{
 */

public class ContactAccountManager {

    private static final String TAG = ContactAccountManager.class.getSimpleName();
    //SPRD:Bug 519455
    private static final boolean DEBUG = !SystemProperties.get("ro.build.type").equals("user");
    final static String SIM_ACCOUNT_TYPE = "sprd.com.android.account.sim";
    final static String USIM_ACCOUNT_TYPE = "sprd.com.android.account.usim";
    private static ContactAccountManager sMe;

    private static final int MESSAGE_LOAD_DATA = 0;
    private static final int MESSAGE_UPDATE_ACCOUNT = 1;

    private Context mContext;
    private HandlerThread mHandlerThread;
    private Handler mHandler;

    private AccountManager mAccountManager;

    private List<Account> mAccounts = Lists.newArrayList();

    /*
     * A latch that ensures that asynchronous initialization completes before data is used
     */
    private volatile CountDownLatch mInitializationLatch = new CountDownLatch(1);

    public static synchronized ContactAccountManager getInstance(Context context) {
        if (sMe == null) {
            sMe = new ContactAccountManager(context);
        }

        return sMe;
    }

    private ContactAccountManager(Context context) {
        mContext = context;
        mAccountManager = AccountManager.get(mContext);

        mHandlerThread = new HandlerThread(TAG);
        mHandlerThread.start();
        mHandler = new Handler(mHandlerThread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MESSAGE_LOAD_DATA:
                        if (DEBUG)
                            Log.v(TAG, "MESSAGE_LOAD_DATA");
                        loadAccountsInBackground();
                        break;
                    case MESSAGE_UPDATE_ACCOUNT:
                        if (DEBUG)
                            Log.v(TAG, "MESSAGE_UPDATE_ACCOUNT");
                        sendEmptyMessage(MESSAGE_LOAD_DATA);
                        break;
                }
            }
        };

    }

    /**
     * Loads account list and always called on a background thread.
     */
    private void loadAccountsInBackground() {
        if (DEBUG)
            Log.v(TAG, "loadAccountsInBackground +");
        synchronized (mAccounts) {
            mAccounts.clear();
            final Account accounts[] = mAccountManager.getAccounts();
            for (Account account : accounts) {
                if (!mAccounts.contains(account)) {
                    if (DEBUG)
                        Log.v(TAG, "loadAccountsInBackground add one entry: " + account);
                    mAccounts.add(account);
                }
            }
            if (DEBUG)
                Log.v(TAG, "loadAccountsInBackground Account list: \n" + mAccounts);
        }

        // A initialization flag to indicate the accounts initialized.
        if (mInitializationLatch != null) {
            mInitializationLatch.countDown();
            mInitializationLatch = null;
        }
        if (DEBUG)
            Log.v(TAG, "loadAccountsInBackground -");
    }

    /**
     * Returns instantly if accounts and account types have already been loaded. Otherwise waits for
     * the background thread to complete the loading.
     */
    void ensureAccountsLoaded() {
        CountDownLatch latch = mInitializationLatch;
        if (latch == null) {
            return;
        }
        while (true) {
            try {
                latch.await();
                return;
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
    }

    /**
     * Reload all of account on a background thread.
     */
    public synchronized void onAccountUpdate() {
        if (DEBUG)
            Log.v(TAG, "onAccountUpdate");
        mHandler.sendEmptyMessage(MESSAGE_LOAD_DATA);
    }

    public Account getSimAccount(int phoneId) {
        if (DEBUG)
            Log.v(TAG, "getSimAccount");
        ensureAccountsLoaded();
        if (DEBUG)
            Log.v(TAG, "AccountsLoaded");
        Account simAccount = null;
        synchronized (mAccounts) {
            for (Account account : mAccounts) {
                String slot = null;
                if (USIM_ACCOUNT_TYPE.equals(account.type) || SIM_ACCOUNT_TYPE.equals(account.type)) {
                    slot = mAccountManager.getUserData(account, "slot");
                }
                if (slot != null) {
                    int i = Integer.parseInt(slot);
                    if (i == phoneId) {
                        // To clone a new object to protect it from damage.
                        // e.g free it at outside
                        simAccount = new Account(account.name, account.type);
                    }
                }
            }
        }
        if (DEBUG)
            Log.v(TAG, "getSimAccount:" + phoneId + ":" + simAccount);
        return simAccount;
    }

    /* SPRD: Add for bug 379273 @{ */
    public void removeFromAccounts(Account account) {
        /* SPRD: Add Synchronization for bug 399260 @{ */
        synchronized (mAccounts) {
            if (mAccounts.contains(account)) {
                mAccounts.remove(account);
            }
        }
        /* @} */
    }
    /* @} */
}
/**
 * @}
 */
