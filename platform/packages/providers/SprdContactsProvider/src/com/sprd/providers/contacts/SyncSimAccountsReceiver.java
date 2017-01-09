
package com.sprd.providers.contacts;

import android.accounts.Account;
import android.accounts.AccountAuthenticatorActivity;
import android.accounts.AccountManager;
import android.accounts.AccountManagerFuture;
import android.accounts.AccountManagerCallback;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.app.NotificationManager;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.Manifest.permission;
import android.os.Bundle;
import android.os.Debug;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.UserHandle;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Email;
import android.provider.ContactsContract.CommonDataKinds.GroupMembership;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.Groups;
import android.provider.ContactsContract.ProviderStatus;
import android.provider.ContactsContract.RawContacts;
import android.provider.Telephony;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerEx;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.IIccPhoneBook;
import com.android.internal.telephony.IIccPhoneBookEx;

import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.TelephonyIntentsEx;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import com.android.internal.telephony.uicc.IccCardApplicationStatus;
import com.android.providers.contacts.AccountWithDataSet;
import com.android.providers.contacts.ContactsDatabaseHelper.RawContactsColumns;
import com.android.providers.contacts.ContactsDatabaseHelper.Tables;
import com.android.providers.contacts.database.ContactsTableUtil;
import com.android.providers.contacts.ContactsDatabaseHelper;
import com.android.providers.contacts.R;
import com.sprd.providers.contacts.ContactProxyManager;
import com.android.internal.telephony.PhoneConstants;

import java.io.IOException;
import java.sql.SQLException;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.CountDownLatch;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
/* sprd bug490245 read sne and aas for orange @{ */
import android.provider.ContactsContract.CommonDataKinds.Nickname;
/* @} */

/* SPRD: Bug 509736 */
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
/**
 * SPRD:
 * 
 * @{
 */
class FSA {
    interface Event {
        int ICC_LOADED = 1;
        int FDN_ENABLED = 2;
    }

    interface Action {
        int BOOT_COMPLETED = 8;
        int BOOTSTRAP = 9;

        int REMOVE_ACCOUNT = 10;
        int ADD_ACCOUNT = 11;
        int PURGE_CONTACT = 12;
        int IMPORT_CONTACT = 13;
    }

    interface State {
        int INIT = 0;
        int BOOT_COMPLETED = 1;
        int ACCOUNT_REMOVED = 3;
        int CONTACT_PURGED = 5;
        int ACCOUNT_ADDED = 7;
        int CONTACT_IMPORTED = 9;
    }

    private static final String TAG = FSA.class.getSimpleName();
    private static final boolean DEBUG = true;
    final static String PHONE_ACCOUNT_TYPE = "sprd.com.android.account.phone";
    final static String SIM_ACCOUNT_TYPE = "sprd.com.android.account.sim";
    final static String USIM_ACCOUNT_TYPE = "sprd.com.android.account.usim";

    private int mEvent;

    private int mPhoneId;
    private int mSubId;
    private Account mAccount = null;

    private int mState = State.INIT;
    public boolean mUserSwitchLoading = false;
    private Handler mHandler;
    private Context mContext;
    private TelephonyManager mTelephonyManager;
    private TelephonyManagerEx mTelephonyManagerEx;

    private static Map<Integer, FSA> sInstance = new HashMap<Integer, FSA>();

    public synchronized static FSA getInstance(Context context, int phoneId, int subId) {
        FSA ret = sInstance.get(phoneId);
        if (ret == null) {
            ret = new FSA(context, phoneId, subId);
            sInstance.put(phoneId, ret);
        } else if (subId != 0) {
            ret.mSubId = subId;
        }
        return ret;
    }

    /* SPRD: Add for bug 379273 @{ */
    public int getSubId() {
        return mSubId;
    }

    /* @} */

    private boolean isEventOn(int event) {
        return (mEvent & (1 << event)) == 0 ? false : true;
    }

    // state:
    // init->boot_completed->purge_contact->remove_account->add_account->import_contact
    private FSA(Context context, int phoneId, int subId) {
        mPhoneId = phoneId;
        mSubId = subId;
        mContext = context.getApplicationContext();
        mTelephonyManager = (TelephonyManager) TelephonyManager.from(context);
        mTelephonyManagerEx = (TelephonyManagerEx) TelephonyManagerEx.from(context);

        // FDN_ENABLED is initialized to true, we will rely on ICC_LOADED event
        // to reset it to the right state later.
        mEvent |= (1 << Event.FDN_ENABLED);

        HandlerThread handlerThread = new HandlerThread("FSA_" + phoneId,
                Process.THREAD_PRIORITY_LOWEST);
        handlerThread.start();
        mHandler = new Handler(handlerThread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                int event = msg.what;

                switch (event) {
                    case Event.ICC_LOADED:
                        if (isEventOn(Event.ICC_LOADED)) {
                            onEvent(Event.FDN_ENABLED,
                                   mTelephonyManagerEx.getIccFdnEnabled(mSubId));
                        }
                        // NOTE: fall-through
                    case Event.FDN_ENABLED: {
                        if (mState == State.ACCOUNT_REMOVED) {
                            if (isEventOn(Event.ICC_LOADED)) {
                                if (addAccount() == -1)
                                    break;
                                mState = State.ACCOUNT_ADDED;
                                onAction(Action.IMPORT_CONTACT);
                            }
                        }

                        if (mState == State.CONTACT_IMPORTED) {
                            markSyncableIfNecessary();
                        }
                    }
                        break;

                    case Action.BOOT_COMPLETED:
                        if (mState != State.INIT) {
                            return;
                        }
                        mState = State.BOOT_COMPLETED;
                        onAction(Action.PURGE_CONTACT);
                        break;

                    case Action.BOOTSTRAP: {
                        mState = State.INIT;
                        onAction(Action.BOOT_COMPLETED);
                    }
                        break;

                    case Action.PURGE_CONTACT:
                        if (mState != State.BOOT_COMPLETED) {
                            return;
                        }
                        mAccount = ContactAccountManager.getInstance(mContext).getSimAccount(
                                mPhoneId);
                        log("PURGE_CONTACT mAccount = " + mAccount);
                        if (mAccount != null) {
                            ContentResolver cr = mContext.getContentResolver();
                            cr.setIsSyncable(mAccount, ContactsContract.AUTHORITY, 0);
                        }
                        purgeContact();
                        mState = State.CONTACT_PURGED;

                        onAction(Action.REMOVE_ACCOUNT);
                        break;

                    case Action.REMOVE_ACCOUNT:
                        if (mState != State.CONTACT_PURGED) {
                            return;
                        }

                        addPhoneAccount();
                        removeAccount();
                        break;

                    case Action.ADD_ACCOUNT:
                        if (mState != State.ACCOUNT_REMOVED) {
                            return;
                        }
                        if (isEventOn(Event.ICC_LOADED)) {
                            if (addAccount() == -1)
                                break;
                            mState = State.ACCOUNT_ADDED;
                            onAction(Action.IMPORT_CONTACT);
                        }
                        break;

                    case Action.IMPORT_CONTACT: {
                        if (mState != State.ACCOUNT_ADDED) {
                            return;
                        }
                        if (DEBUG)
                            log("startImport for " + mAccount);
                        updateProviderStatus(mContext, true);
                        ContactProxyManager.getInstance(mContext)
                                .onImport(mAccount);
                        updateProviderStatus(mContext, false);
                        mState = State.CONTACT_IMPORTED;

                        markSyncableIfNecessary();
                        mUserSwitchLoading = false;
                    }
                        break;
                    default:
                        break;
                }
            }
        };
        /* SPRD: Modify for bug 379273 @{ */
        SubscriptionManager subScriptionManager = new SubscriptionManager(context);
        if (mTelephonyManager.getSimState(mPhoneId) == TelephonyManager.SIM_STATE_READY) {
            List<SubscriptionInfo> subInfos = subScriptionManager.getActiveSubscriptionInfoList();
            if (subInfos == null) {
                subInfos = new ArrayList<SubscriptionInfo>();
            }
            for (SubscriptionInfo subInfo : subInfos) {
                if (subInfo.getSimSlotIndex() == mPhoneId) {
                    mSubId = subInfo.getSubscriptionId();
                }
            }
            /* SPRD: Bug 592770 sometimes fails to load contact with wrong subid. @{ */
            if (mSubId > 0) {
                onEvent(Event.ICC_LOADED, true);
            } else {
                onEvent(Event.ICC_LOADED, false);
            }
            /* @} */
        } else {
            onEvent(Event.ICC_LOADED, false);
        }
        /* @} */
    }

    private static AtomicInteger sImportingCounter = new AtomicInteger(0);

    private static void updateProviderStatus(Context context, boolean importing) {
        int counter = 0;
        if (importing) {
            sImportingCounter.incrementAndGet();
        } else {
            sImportingCounter.decrementAndGet();
        }
        ContentResolver cr = context.getContentResolver();
        do {
            counter = sImportingCounter.get();
            if (DEBUG)
                Log.d(TAG, "sImportingCounter=" + counter);
            ContentValues status = new ContentValues();
            if (counter > 0) {
                status.put(ProviderStatus.STATUS, ProviderStatus.STATUS_IMPORTING);
            } else {
                status.put(ProviderStatus.STATUS, ProviderStatus.STATUS_NORMAL);
            }
            cr.update(ProviderStatus.CONTENT_URI, status, null, null);
        } while (sImportingCounter.get() != counter);
    }

    private void markSyncableIfNecessary() {
        ContentResolver cr = mContext.getContentResolver();
        if (mAccount != null) {
            int syncable = 1;
            /**
             * SPRD Bug553479 USIM card,startup FDN,Contact of SIM card display as usual.
             *     if (mAccount.type.equals(SIM_ACCOUNT_TYPE)
             * @{
             */
            if ((mAccount.type.equals(SIM_ACCOUNT_TYPE) || mAccount.type.equals(USIM_ACCOUNT_TYPE))
            /**
             * @}
             */
            && mTelephonyManagerEx.getIccFdnEnabled(mSubId)) {
                syncable = 0;
                removeAccount();
            }
            if (DEBUG)
                log("setIsSyncable: " + mAccount + " " + syncable);
            cr.setIsSyncable(mAccount, ContactsContract.AUTHORITY, syncable);
        }
    }

    /* *
     * Remove this function and replace it w/ {@link ContactAccountManager.getPhoneAccount}. private
     * Account phoneToAccount(int phoneId) { AccountManager am = AccountManager.get(mContext);
     * Account[] simAccounts = am.getAccounts(); ContentResolver cr = mContext.getContentResolver();
     * Account ret = null; for (Account account : simAccounts) { String slot =
     * am.getUserDataPrivileged(account, "slot"); if (slot != null) { int i =
     * Integer.parseInt(slot); if (i == phoneId) { ret = account; } } } Log.e(TAG, "phoneToAccount:"
     * + phoneId + ":" + ret); return ret; }
     */

    private void purgeContact() {
        if (DEBUG)
            log(">>>purgeContact:" + mPhoneId + ":" + mAccount);

        //SPRD: add for bug621877, fdn feature bugfix
        SyncFDNContactsUtil.getInstance().purgeFDNContacts(mContext,
                String.valueOf(mPhoneId));

        if (mAccount == null) {
            if (DEBUG)
                log("mAccount is null");
            return;
        }

        Long accountId = ContactsDatabaseHelper.getInstance(mContext).getAccountIdOrNull(
                new AccountWithDataSet(mAccount.name, mAccount.type, null));
        if (accountId == null) {
            return;
        }
        SQLiteDatabase db = ContactsDatabaseHelper.getInstance(mContext).getWritableDatabase();
        db.beginTransaction();
        Cursor cursor = null;
        try {
            db.execSQL(
                    "DELETE FROM " + Tables.GROUPS + " WHERE account_id = " + accountId);

            db.execSQL(
                    "DELETE FROM " + Tables.RAW_CONTACTS + " WHERE account_id = " + accountId);

            if (DEBUG)
                Log.d(TAG, "Delete contacts from table " + Tables.RAW_CONTACTS
                        + " where " + RawContactsColumns.ACCOUNT_ID + " = "
                        + accountId);

            db.setTransactionSuccessful();
        } catch (RuntimeException e) {
            Log.d(TAG, "purgeContact error occur " + e.toString());
        } finally {
            if (cursor != null)
                cursor.close();
            db.endTransaction();
        }

        if (DEBUG)
            log("<<<purgeContact:" + mPhoneId);
    }

    private int addAccount() {
        if (DEBUG)
            log("addAccount for " + mPhoneId + " and subId  " + mSubId);
        ContentResolver cr = mContext.getContentResolver();
        IIccPhoneBookEx ipb = IIccPhoneBookEx.Stub.asInterface(ServiceManager
                .getService("simphonebookEx"));

        boolean isUsim = false;
        boolean isSim = false;
        try {
            if (ipb != null) {
                isUsim = ipb.isApplicationOnIcc(
                        IccCardApplicationStatus.AppType.APPTYPE_USIM.ordinal(), mSubId);
                isSim = ipb.isApplicationOnIcc(
                        IccCardApplicationStatus.AppType.APPTYPE_SIM.ordinal(), mSubId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }

        //SPRD: add for bug621877, fdn feature bugfix
        SyncFDNContactsUtil.getInstance().purgeFDNContacts(mContext,
                String.valueOf(mPhoneId));

        String accountType = null;
        String accountNameTmpl = null;
        if (!isEventOn(Event.FDN_ENABLED)) {
            if (isUsim) {
                if (DEBUG)
                    log("addAccount: USIM");
                accountType = USIM_ACCOUNT_TYPE;
                accountNameTmpl = "SIM";
            } else if (isSim) {
                if (DEBUG)
                    log("addAccount: SIM");
                accountType = SIM_ACCOUNT_TYPE;
                accountNameTmpl = "SIM";
            } else {
                if (DEBUG)
                    log("addAccount: can't detetct sim type,assume it to be SIM");
                accountType = SIM_ACCOUNT_TYPE;
                accountNameTmpl = "SIM";
            }
        }
        /**
         * SPRD Bug553479 USIM card,startup FDN,Contact of SIM card display as usual.
         *     else if (isUsim) {
         *         if (DEBUG)
         *         log("addAccount: USIM");
         *         accountType = USIM_ACCOUNT_TYPE;
         *         accountNameTmpl = "SIM";
         *      }
         */
         else {
            //SPRD: add for bug617830, add fdn feature
            log("addAccount: not usim type and enabled FDN, add FDN contacts");
            SyncFDNContactsUtil.getInstance().importFDNContacts(mContext,
                    mPhoneId, mSubId,
                    new Account("Phone", FSA.PHONE_ACCOUNT_TYPE));
            return -1;
        }

        AccountManager am = AccountManager.get(mContext);
        Account account;

        boolean isSingleSim = ((TelephonyManager) TelephonyManager.from(mContext)).getPhoneCount() == 1 ? true
                : false;
        if (isSingleSim) {
            account = new Account(accountNameTmpl, accountType);
        } else {
            account = new Account(accountNameTmpl + (mPhoneId + 1), accountType);
        }

        am.addAccountExplicitly(account, null, makeUserData(mPhoneId, isSingleSim));
        mAccount = account;
        cr.setIsSyncable(account, ContactsContract.AUTHORITY, 0);
        return 0;
    }

    private Bundle makeUserData(int phoneId, boolean isSingleSim) {
        Bundle ret = new Bundle();
        if (!isSingleSim) {
            // `identifier` , as a text, will be drawn directly on the top-right of the contact avatar
            ret.putString("identifier", Integer.toString(phoneId + 1));
        }
        ret.putString("slot", Integer.toString(phoneId));
        // set icc uri
        String iccUri = "";
        String iccGroupUri = "";
        String iccSdnUri = "";
        /* sprd bug490245 read sne and aas for orange @{ */
        String iccAasUri = "";
        String iccSneUri = "";
        /* @} */
        if (isSingleSim) {
            iccUri = "content://icc/adn";
            iccGroupUri = "content://icc/gas";
            iccSdnUri = "content://icc/sdn";
            /* sprd bug490245 read sne and aas for orange @{ */
            iccAasUri = "content://icc/aas";
            iccSneUri = "content://icc/sne";
            /* @} */
        } else {
            iccUri = "content://icc/adn/subId/" + mSubId;
            iccGroupUri = "content://icc/gas/subId/" + mSubId;
            iccSdnUri = "content://icc/sdn/subId/" + mSubId;
            /* sprd bug490245 read sne and aas for orange @{ */
            iccAasUri = "content://icc/aas/subId/" + mSubId;
            iccSneUri = "content://icc/sne/subId/" + mSubId;
            /* @} */
        }

        ret.putString("icc_uri", iccUri);
        ret.putString("icc_gas_uri", iccGroupUri);
        ret.putString("icc_sdn_uri", iccSdnUri);
        /* sprd bug490245 read sne and aas for orange @{ */
        ret.putString("icc_aas_uri", iccAasUri);
        ret.putString("icc_sne_uri", iccSneUri);
        /* @} */

        // set account restriction
        int nameLength = SimUtils.getSimContactNameLength(mSubId);
        if (nameLength != -1) {
            ret.putString(StructuredName.CONTENT_ITEM_TYPE + "_length",
                    Integer.toString(nameLength));
        }

        /* sprd bug490245 read sne and aas for orange @{ */
        int sneLength = SimUtils.getSimSneLength(mSubId);
        if(sneLength != -1) {
            ret.putString(Nickname.CONTENT_ITEM_TYPE + "_length",
                    Integer.toString(sneLength));
        }
        /* @} */

        int phoneLength = SimUtils.getSimContactPhoneLength(mSubId);
        if (phoneLength != -1) {
            ret.putString(Phone.CONTENT_ITEM_TYPE + "_length", Integer.toString(phoneLength));
        }

        int emailLength = SimUtils.getSimContactEmailLength(mSubId);
        if (emailLength != -1) {
            ret.putString(Email.CONTENT_ITEM_TYPE + "_length", Integer.toString(emailLength));
        }

        int simCardLength = SimUtils.getSimCardLength(mSubId);
        if (simCardLength != -1) {
            ret.putString("capacity", Integer.toString(simCardLength));
        }

        int emailCapacity = SimUtils.getSimContactEmailCapacity(mSubId);
        if (emailCapacity != -1) {
            ret.putString(Email.CONTENT_ITEM_TYPE + "_capacity", Integer.toString(emailCapacity));
        }

        int phoneTypeOverallMax = SimUtils.getSimContactPhoneTypeOverallMax(mSubId);
        if (phoneTypeOverallMax != -1) {
            ret.putString(Phone.CONTENT_ITEM_TYPE + "_typeoverallmax",
                    Integer.toString(phoneTypeOverallMax));
        }

        int emailTypeOverallMax = SimUtils.getSimContactEmailTypeOverallMax(mSubId);
        if (emailTypeOverallMax != -1) {
            ret.putString(Email.CONTENT_ITEM_TYPE + "_typeoverallmax",
                    Integer.toString(emailTypeOverallMax));
        }

        int usimGroupNameMaxLen = SimUtils.getUsimGroupNameMaxLen(mSubId);
        if (usimGroupNameMaxLen != 0) {
            ret.putString(GroupMembership.CONTENT_ITEM_TYPE + "_length",
                    Integer.toString(usimGroupNameMaxLen));
        }

        int usimGroupCapacity = SimUtils.getUsimGroupCapacity(mSubId);
        ret.putString(GroupMembership.CONTENT_ITEM_TYPE + "_capacity",
                Integer.toString(usimGroupCapacity));

        if (DEBUG)
            log("account: userdata:" + ret.toString());
        return ret;
    }

    private void removeAccount() {
        if (DEBUG)
            log(">>>remove sim account:" + mPhoneId);
        if (mAccount == null) {
            if (DEBUG)
                log("mAccount is null");
            mState = State.ACCOUNT_REMOVED;
            onAction(Action.ADD_ACCOUNT);
        } else {
            AccountManager am = AccountManager.get(mContext);
            am.removeAccount(mAccount, new AccountManagerCallback<Boolean>() {
                public void run(AccountManagerFuture<Boolean> future) {
                    mState = State.ACCOUNT_REMOVED;
                    onAction(Action.ADD_ACCOUNT);
                }
            }, new Handler(Looper.myLooper()));
            /* SPRD: Add for bug 379273 @{ */
            ContactAccountManager.getInstance(mContext).removeFromAccounts(mAccount);
            /* @} */
        }
        if (DEBUG)
            log("<<<remove sim account:" + mPhoneId);
        return;
    }

    private void addPhoneAccount() {
        if (DEBUG)
            log(">>>addPhoneAccount");
        AccountManager am = AccountManager.get(mContext);
        Account[] accounts = am.getAccountsByType(PHONE_ACCOUNT_TYPE);
        if (accounts != null && accounts.length != 0) {
            if (DEBUG)
                log("phone account already exists");
            return;
        }
        //SPRD: add for bug622348, named phone account as "Phone"
        Account account = new Account("Phone", PHONE_ACCOUNT_TYPE);
        am.addAccountExplicitly(account, null, null);
        // set is syncable
        ContentResolver cr = mContext.getContentResolver();
        cr.setIsSyncable(account, ContactsContract.AUTHORITY, 1);
        if (DEBUG)
            log("<<<addPhoneAccount");
    }

    public void onEvent(int event, boolean on) {
        if (DEBUG)
            log("onEvent:" + mPhoneId + ":" + event + ":" + on);
        if (on) {
            mEvent |= (1 << event);
        } else {
            mEvent &= ~(1 << event);
        }

        mHandler.sendMessage(mHandler.obtainMessage(event));
    }

    public void onAction(int action) {
        if (DEBUG)
            log("onAction: mPhoneId = " + mPhoneId + ", action = " + action);
        mHandler.sendMessage(mHandler.obtainMessage(action));
    }

    private void log(String msg) {
        Log.d(TAG + mPhoneId, msg);
    }
}

public class SyncSimAccountsReceiver extends BroadcastReceiver {

    private static final String TAG = SyncSimAccountsReceiver.class.getSimpleName();
    private static final boolean DEBUG = true;//Debug.isDebug();sprdPorting
    static boolean[] mIccLoaded = {false,false};
    private static boolean isPermissionSatisfied = false;
    private static String[] PERMISSIONS = new String[] {
            permission.READ_CONTACTS,
            permission.READ_PHONE_STATE,
    };
    private static boolean isFdnEnabled = false;
    private static int mFdnPhoneId = -1;
    private static int mFdnSubId = -1;
    private TelephonyManagerEx mTelephonyManagerEx;

    class GetSubscriptionInformationTask extends Thread {
        private Context mContext;

        public GetSubscriptionInformationTask(Context context){
            this.mContext = context;
        }

        @Override
        public void run(){
            SubscriptionManager subScriptionManager = new SubscriptionManager(mContext);
            List<SubscriptionInfo> subInfos = subScriptionManager.getActiveSubscriptionInfoList();
            if (subInfos == null) {
                subInfos = new ArrayList<SubscriptionInfo>();
            }

            for (SubscriptionInfo subInfo : subInfos) {
                int phoneId = subInfo.getSimSlotIndex();
                int subId = subInfo.getSubscriptionId();
                Log.d(TAG, "SUBINFO_RECORD_UPDATED: phoneId = " + phoneId + ", subId = " + subId);
                if(subId < 0) {
                    return;
                }
                if (FSA.getInstance(mContext, phoneId, 0).getSubId() <= 0
                        && TelephonyManager.from(mContext).getSimState(phoneId) == TelephonyManager.SIM_STATE_READY && mIccLoaded[phoneId]) {
                    FSA.getInstance(mContext, phoneId, subId).onEvent(FSA.Event.ICC_LOADED, true);
                }
            }
        }
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent == null) {
            return;
        }
        String action = intent.getAction();
        /**
         *  SPRD: Bug 498443 Close the phone permission in the settings,acore stops running.
         * @{
         */
        String origAction = intent.getStringExtra("origIntent");
        if (!TextUtils.isEmpty(origAction)) {
            action = origAction;
        }
        /**
        * @}
        */
        Log.w(TAG, "action = " + action + ", isFdnEnabled = " + isFdnEnabled);
        if (TextUtils.isEmpty(action)) {
            return;
        }

        /**
         *  SPRD: Bug 498443 Close the phone permission in the settings,acore stops running.
         *  SPRD: Bug 514657 The sim contacts can not be loaded after closing and opening the phone
         * permission of Contacts.
         *
         * @{
         */
        for (String permission : PERMISSIONS) {
            if (context.checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
                isPermissionSatisfied = true;
                break;
            }
        }
        if (isPermissionSatisfied) {
            isPermissionSatisfied = false;
            if (action.startsWith(TelephonyIntents.ACTION_SIM_STATE_CHANGED)) {
                String state = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                if (state.equals("LOADED")) {
                    Log.d(TAG,"Sim is loaded,but the permission for READ_PHONE_STATE is denied, can not load sim contacts.");
                }
            }
            final Intent PermissionIntent = new Intent(context, RequestProviderPermissionsActivity.class);
            PermissionIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            PermissionIntent.putExtra("origIntent",action);
            context.startActivity(PermissionIntent);
            return;
        }
        /**
        * @}
        */
        SubscriptionManager subScriptionManager = new SubscriptionManager(context);
        if (action.startsWith(TelephonyIntents.ACTION_SIM_STATE_CHANGED)) {
            String state = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
            int phoneId = intent.getIntExtra(PhoneConstants.SLOT_KEY, 0);
            int subId = intent.getIntExtra(PhoneConstants.SUBSCRIPTION_KEY, 0);
            Log.e(TAG, "SIM_STATE_CHANGED: state: " + state + " phoneId: " + phoneId + " subId: "
                    + subId);
            if (!TextUtils.isEmpty(state)) {
                if (IccCardConstants.INTENT_VALUE_ICC_LOADED.equals(state)) {
                    mIccLoaded[phoneId] = true;
                    //SPRD: add for bug632539, SIM state is ready, subId is invalid
                    if (subId <= 0) {
                        SubscriptionInfo subscriptionInfo = subScriptionManager.getActiveSubscriptionInfoForSimSlotIndex(phoneId);
                        if (subscriptionInfo != null) {
                            subId = subscriptionInfo.getSubscriptionId();
                            Log.d(TAG, "subId <= 0, update subId = " + subId);
                        }
                    }
                    if (subId > 0) {
                        FSA.getInstance(context, phoneId, subId).onEvent(FSA.Event.ICC_LOADED, true);
                    }
                } else if (IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(state)) {
                    mIccLoaded[phoneId] = false;
                    FSA.getInstance(context, phoneId, subId).onEvent(FSA.Event.ICC_LOADED, false);
                    FSA.getInstance(context, phoneId, subId).onAction(FSA.Action.BOOTSTRAP);
                    Log.d(TAG, "deleteFdnContacts");
                    SyncFDNContactsUtil.getInstance().purgeFDNContacts(context,
                            String.valueOf(phoneId));
                /**
                 *  SPRD: 499917 DUT Can not add sim Contact after modem manual assert.
                 * @{
                 */
                } else if (IccCardConstants.INTENT_VALUE_ICC_NOT_READY.equals(state)) {
                    mIccLoaded[phoneId] = false;
                    FSA.getInstance(context, phoneId, subId).onEvent(FSA.Event.ICC_LOADED, false);
                    FSA.getInstance(context, phoneId, subId).onAction(FSA.Action.BOOTSTRAP);
                /**
                 * @}
                 */
                } else {
                    mIccLoaded[phoneId] = false;
                    FSA.getInstance(context, phoneId, subId).onEvent(FSA.Event.ICC_LOADED, false);
                }
            }
        } else if (action.startsWith(TelephonyIntents.ACTION_SUBINFO_RECORD_UPDATED)) {
            Log.e(TAG, "action with android.intent.action.ACTION_SUBINFO_RECORD_UPDATED");
            GetSubscriptionInformationTask getSubTask = new GetSubscriptionInformationTask(context);
            getSubTask.start();
        } else if (action.startsWith("android.intent.action.FDN_STATE_CHANGED")) {
            isFdnEnabled = intent.getBooleanExtra("fdn_status", false);
            mFdnPhoneId = intent.getIntExtra("phone", 0);
            mFdnSubId = intent.getIntExtra(PhoneConstants.SUBSCRIPTION_KEY, 0);
            FSA.getInstance(context, mFdnPhoneId, mFdnSubId).onEvent(FSA.Event.FDN_ENABLED, isFdnEnabled);
            SubscriptionInfo subscriptionInfo = subScriptionManager.getActiveSubscriptionInfoForSimSlotIndex(mFdnPhoneId);
            if (subscriptionInfo != null) {
                mFdnSubId = subscriptionInfo.getSubscriptionId();
            }
            Log.e(TAG, "FDN_STATE_CHANGED: state: " + isFdnEnabled + " phoneId: " + mFdnPhoneId
                    + " subId: " + mFdnSubId);
            //SPRD: add for bug617830, add fdn feature
            if (!isFdnEnabled) {
                Log.d(TAG, "deleteFdnContacts");
                SyncFDNContactsUtil.getInstance().purgeFDNContacts(context,
                        String.valueOf(mFdnPhoneId));
            }
        } else if (action.equals("android.intent.action.FDN_LIST_CHANGED")) {
            mFdnSubId = intent.getIntExtra("subid", 0);
            mFdnPhoneId = subScriptionManager.getPhoneId(mFdnSubId);
            mTelephonyManagerEx = (TelephonyManagerEx) TelephonyManagerEx.from(context);
            if (mTelephonyManagerEx != null) {
                isFdnEnabled = mTelephonyManagerEx.getIccFdnEnabled(mFdnSubId);
            }
            Log.d(TAG, "mFdnPhoneId = "+ mFdnPhoneId + ", mFdnSubId = " + mFdnSubId + ", isFdnEnabled = " + isFdnEnabled);
            if (isFdnEnabled) {
                Log.d(TAG, "deleteFdnContacts");
                SyncFDNContactsUtil.getInstance().purgeFDNContacts(context,
                        String.valueOf(mFdnPhoneId));
                Log.d(TAG, "importFDNContacts");
                SyncFDNContactsUtil.getInstance().importFDNContacts(context,
                        mFdnPhoneId, mFdnSubId,
                        new Account("Phone", FSA.PHONE_ACCOUNT_TYPE));
            }
        /* SPRD: Bug 498443 502625 509736 513096 */
        } else if (action.equals("sync_sim_fake_boot_completed")) {
            int switchToId = intent.getIntExtra(Intent.EXTRA_USER_HANDLE, -1);
            DevicePolicyManager dpm = (DevicePolicyManager) context.getSystemService(Context.DEVICE_POLICY_SERVICE);
            ComponentName profileOwner = dpm.getProfileOwnerAsUser(UserHandle.myUserId());
            String packageName = "";
            if (profileOwner != null) {
                packageName = profileOwner.getPackageName();
            }
            Log.e(TAG, "sync_sim_fake_boot_completed: profileOwner is " + profileOwner);
            Log.e(TAG, "sync_sim_fake_boot_completed: switch to Id:" + switchToId + " myId:"+UserHandle.myUserId());
            if((switchToId == -1 || switchToId == UserHandle.myUserId())
                && !packageName.equals("com.android.cts.managedprofile")) {
                for (int i = 0; i < ((TelephonyManager) TelephonyManager.from(context)).getPhoneCount(); ++i) {
                    /* For too many sim status types, ignore it temporary. */
                    if(!FSA.getInstance(context, i, 0).mUserSwitchLoading) {
                        FSA.getInstance(context, i, 0).mUserSwitchLoading = true;
                        FSA.getInstance(context, i, 0).onAction(FSA.Action.BOOTSTRAP);
                    }
                }
            }
        } else if (action.equals(TelephonyIntentsEx.ACTION_STK_REFRESH_SIM_CONTACTS)) {
            int phoneId = intent.getIntExtra("phone_id", 0);
            int subId = intent.getIntExtra(PhoneConstants.SUBSCRIPTION_KEY, 0);
            FSA.getInstance(context, phoneId, subId).onAction(FSA.Action.BOOTSTRAP);
        }
    }
}
/**
 * @}
 */
