/** Created by Spreadtrum */
package com.sprd.launcher3.widget.contacts;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import com.android.sprdlauncher1.R;
import com.android.sprdlauncher1.Launcher;

import android.R.integer;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.AsyncTask.Status;
import android.provider.CallLog;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.Telephony.Sms;
import android.provider.ContactsContract;
import android.provider.CallLog.Calls;
import android.provider.ContactsContract.CommonDataKinds;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Data;
import android.sim.Sim;
import android.sim.SimManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;

public class ContactsData {
    private static final String TAG = "Launcher.ContactsData";

    private static final boolean ISDEBUG = false;

    /**
     * Action of the intent that start Contact select activity.
     */
    private static final String CONTACT_SELECT_APP_ACTION = "android.intent.action.PICK";

    /**
     * The key of the hashmap passed over from phonebook list.
     */
    public static final String INTENT_EXTRA_KEY = "widget_select_contacts";

    /**
     * The intent action of updating widget.
     */
    public static final String CONTACT_WIDGET_UPDATE = "com.sprd.launcher3.contact.update_contact_widget";
    /**
     * The intent action of updating widget missed call and message
     */
    public static final String CONTACT_WIDGET_UPDATE_MISS = "com.sprd.launcher3.contact.update_contact_widget_miss";

    private static final Uri CONTACT_URI = ContactsContract.Contacts.CONTENT_URI;

    private static final String[] PROJECTIONS = new String[] {
        ContactsContract.Contacts._ID,
        ContactsContract.Contacts.DISPLAY_NAME,
        ContactsContract.Contacts.PHOTO_ID,
        ContactsContract.Contacts.HAS_PHONE_NUMBER,
        ContactsContract.Contacts.LOOKUP_KEY
    };

    private static final int DISPLAY_NAME_COLUMN_INDEX = 1;
    private static final int PHOTO_ID_COLUMN_INDEX = 2;
    private static final int LOOKUP_KEY_COLUMN_INDEX = 4;
    private static final Uri PHONE_URI = ContactsContract.CommonDataKinds.Phone.CONTENT_URI;
    private static final Uri SMS_URI = Sms.CONTENT_URI;//.Inbox.CONTENT_URI;   //Uri.parse("content://sms/inbox");
    private static final Uri OBSERVER_PHONE_URI = CallLog.Calls.CONTENT_URI;

    private static final String[] PHONE_PROJECTIONS = new String[] {
        ContactsContract.CommonDataKinds.Phone.CONTACT_ID,
        ContactsContract.CommonDataKinds.Phone.NUMBER,
        ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME
    };

    /* SPRD: Fix bug 262325 @{ */
    private static final String NUM_PROJECTION[] = {
        CommonDataKinds.Phone.NUMBER,
        Data.DISPLAY_NAME,
        Data.PHOTO_URI,
        Data.CONTACT_ID,
        Contacts.HAS_PHONE_NUMBER,
        Contacts.PHOTO_ID,
        Contacts.SORT_KEY_PRIMARY,
        Contacts.LOOKUP_KEY
    };
    /* @} */

    private static final int PHONE_NUMBER_COLUMN_INDEX = 1;
    private static final int PHONE_CONTACT_ID_COLUMN_INDEX = 0;
    private static final int PHONE_NAME_COLUMN_INDEX = 2;


    private static String sDefaultName;

    /**
     * Marked the changes in contacts database.
     */
    private boolean mContentChanged = false;

    /**
     * Observe the changes in contacts database.
     */
    private ContentObserver mObserver;
    private ContentObserver mMissedObserver;

    /**
     * The list of contacts that has been selected.
     */
    private ArrayList<ContactItemInfo> mSelectedItemsList;
    private Map<String,String> fastNumbers=new HashMap<String,String>();

    private boolean mIsUpdating = false;

    private boolean mIsUpdatingMissData = false;

    private boolean mHasUpdated = false;

    private Intent mUpdateIntent;

    private Intent mMissedCallAndSmsUpdateIntent;

    /**
     * The task to load data.
     */
    private GetSelectedDataTask mUpdateDataTask;

    private GetMissedDataTask mUpdateMissedDataTask;

    private final int mNowCount = 9;

    private ContentResolver mResolver;

    private static ContactsData sInstance;

    /**
     * Special instance variable.
     */
    private static byte[] lock = new byte[0];

    private Context mContext;

    private int mCurrRequest = -1;

    private TelephonyManager teleMgr;

    private BroadcastReceiver mFdAndVmReceiver; //fast dial & Voice mail

    private static final String FAST_DIAL_CHANGE_ACTION = "intent.action.phone.fdnumber_changed";

    private static final String VOICE_MAIL_CHANGE_ACTION = "intent.action.phone.vmnumber_changed";

    static final String AUTHORITY = "com.android.phone";
    static final Uri FAST_DIAL_QUERY_URI = Uri.parse("content://" + AUTHORITY
            + "/fdnumber/phoneId/0");// sim 0
    static final String FAST_DIAL_INSERT_URI_PREX = "content://" + AUTHORITY
            + "/fdnumber/";

    private final Map<Integer, Integer> missedCallCount = new HashMap<Integer, Integer>();
    private final Map<Integer, Integer> missedSmsCount = new HashMap<Integer, Integer>();

    public static ContactsData getInstance(Context context) {
        if (sInstance == null) {
            synchronized (lock) {
                if (sInstance == null) {
                    sInstance = new ContactsData(context);
                }
            }
        }
        return sInstance;
    }

    private ContactsData(Context context) {
        init(context);
    }

    private void init(Context context) {
        mContext = context;
        Resources res = context.getResources();
        sDefaultName = res.getString(R.string.contact_default_name);
        mSelectedItemsList = new ArrayList<ContactItemInfo>();
        mResolver = context.getContentResolver();
        mUpdateIntent = new Intent(CONTACT_WIDGET_UPDATE);
        mMissedCallAndSmsUpdateIntent = new Intent(CONTACT_WIDGET_UPDATE_MISS);
        mUpdateDataTask = new GetSelectedDataTask();
        mUpdateMissedDataTask = new GetMissedDataTask();
        teleMgr = (TelephonyManager) mContext
                .getSystemService(Context.TELEPHONY_SERVICE);
    }

    public void registerContentObserver() {
        mObserver = new ContentObserver(new Handler()) {
            @Override
            public void onChange(boolean selfChange) {
                super.onChange(selfChange);
                mContentChanged = true;
                mHasUpdated = false;
            }
        };
        try {
            mResolver.registerContentObserver(ContactsData.CONTACT_URI, true, mObserver);
        } catch (Exception e) {
            e.printStackTrace();
        }
        registerBroadcastReceiver();
        registerMissedObserver();
    }

    private void registerMissedObserver() {
        mMissedObserver = new ContentObserver(new Handler()) {
            @Override
            public void onChange(boolean selfChange) {
                super.onChange(selfChange);
                Handler handler = null;
                if (mContext != null) {
                    handler = new Handler(mContext.getMainLooper());
                }
                if (handler != null) {
                    handler.postDelayed(new Runnable() {

                        @Override
                        public void run() {
                            // TODO Auto-generated method stub
                            updateMissedData();
                        }

                    }, 250);
                } else {
                    updateMissedData();
                }
            }
        };
        try {
            mResolver.registerContentObserver(OBSERVER_PHONE_URI, true,
                    mMissedObserver);
            mResolver.registerContentObserver(ContactsData.SMS_URI, true,
                    mMissedObserver);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void registerBroadcastReceiver() {
        mFdAndVmReceiver = new FdAndVmBroadcastReceiver();
        IntentFilter itf = new IntentFilter();
        itf.addAction(FAST_DIAL_CHANGE_ACTION);
        itf.addAction(VOICE_MAIL_CHANGE_ACTION);
        mContext.registerReceiver(mFdAndVmReceiver, itf);

    }

    private void unRegisterBroadcastReceiver() {
        mContext.unregisterReceiver(mFdAndVmReceiver);
    }

    class FdAndVmBroadcastReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            mContentChanged = true;
            mHasUpdated = false;
        }
    }

    private void restoreMissedCallAndSmsCount() {
        // i=0 is voice mail
        for (int i = 1; i < mSelectedItemsList.size(); i++) {
            if (mSelectedItemsList.get(i).mContactId != -1) {
                // i+1 equals the fast number
                missedCallCount.put(i,
                        findNewCallCount(mSelectedItemsList.get(i).mContactId));
                missedSmsCount.put(i,
                        findNewSmsCount(mSelectedItemsList.get(i).mContactId));
            } else {
                missedCallCount.put(i, 0);
                missedSmsCount.put(i, 0);
            }
        }
    }

    public Map<Integer, Integer> getMissedCallCount() {
        /* SPRD: fix bug223493 @{ */
        if(mIsUpdatingMissData || missedCallCount.size() != mNowCount - 1){
            return null;
        }
        /* @} */
        return missedCallCount;
    }

    public Map<Integer, Integer> getMissedSmsCount() {
        /* SPRD: fix bug223493 @{ */
        if(mIsUpdatingMissData || missedSmsCount.size() != mNowCount - 1){
            return null;
        }
        /* @} */
        return missedSmsCount;
    }
    /**
     * Get a list has been cloned from mSelectedItemsList.
     */
    @SuppressWarnings("unchecked")
    public ArrayList<ContactItemInfo> getSelectedItemsList() {
        return (ArrayList<ContactItemInfo>) mSelectedItemsList.clone();
    }

    /**
     * Add selected items to SharedPreferences.
     */
    public void addDataToList(Intent intent) {
        if (intent == null) {
            return;
        }
        Uri uri = intent.getData();
        if (uri == null) {
            return;
        }
        /* SPRD: Fix bug 262325 @{ */
        Cursor c = mResolver.query(uri, NUM_PROJECTION, null, null, null);
        if (c != null && c.moveToFirst()) {
            ContactItemInfo item = null;
            item = new ContactItemInfo();
            item.mContactId = Long.valueOf(c.getString(3));
            String result = c.getString(4);
            if (result == null || !result.equals("1")) {
                Toast.makeText(mContext, R.string.contact_no_number,
                        Toast.LENGTH_LONG).show();
                /* SPRD: Fix bug 291464 @{ */
                if(c != null)
                    c.close();
                /* @} */
                return;
            }
            item.mFastNumber = mCurrRequest;
            item.mCallNumber = c.getString(0);/*getContactNumber(item.mContactId).get(0);*/
            String name = c.getString(1);
            item.mNameStr = getItemDisplayName(item.mContactId, name, result,
                    null);
            String photoIdStr = c.getString(5);
            if (photoIdStr != null) {
                item.mPhotoId = Long.valueOf(photoIdStr);
            } else {
                item.mPhotoId = 0;
            }
            item.mSortkey = c.getString(6);
            item.mLookUpKey = c.getString(7);
            /* @} */
            replaceCurrRequest(item);
            addDataToSPF();
            mCurrRequest = -1;
            mContext.getApplicationContext().sendBroadcast(mUpdateIntent);
        }

        if(c != null)
            c.close();
        //after add new contact , refresh missed call and missed message
        updateMissedData();
    }

    public void setCurrRequest(int number) {
        mCurrRequest = number;
    }

    private void replaceCurrRequest(ContactItemInfo item) {
        if (mCurrRequest != -1) {
            for (int i = 0; i < mSelectedItemsList.size(); i++) {
                if (mSelectedItemsList.get(i).mFastNumber == mCurrRequest) {
                    mSelectedItemsList.remove(i);
                    mSelectedItemsList.add(i, item);
                }
            }
        }
    }

    /**
     * Add selected items to SharedPreferences.
     */
    private void addDataToSPF() {
        if (mSelectedItemsList == null || mSelectedItemsList.size() == 0) {
            return;
        }
        for (int i = 1; i < mSelectedItemsList.size(); i++) {
            ContactItemInfo item = mSelectedItemsList.get(i);
            addFastContactNumberToSPF(item);
            addFastContactNameToSPF(item);
        }
    }

    private void addFastContactNumberToSPF(ContactItemInfo item){
        if (item.mFastNumber == mCurrRequest && item.mCallNumber != null) {
            ContentValues values = new ContentValues();
            values.put("phoneId", 0);
            values.put("key", "fast_dial_" + mCurrRequest);
            values.put("value", item.mCallNumber);
            values.put("contactId", item.mContactId);
            mContext.getContentResolver().insert(
                    Uri.parse(FAST_DIAL_INSERT_URI_PREX + mCurrRequest),
                    values);
        }
    }

    private void addFastContactNameToSPF(ContactItemInfo item){
        if (item.mFastNumber == mCurrRequest && item.mCallNumber != null) {
            ContentValues values = new ContentValues();
            values.put("phoneId", 0);
            values.put("key", "fast_dial_name_" + mCurrRequest);
            values.put("value", item.mNameStr);
            mContext.getContentResolver().insert(
                    Uri.parse(FAST_DIAL_INSERT_URI_PREX + mCurrRequest),
                    values);
        }
    }



    /**
     * Load data from SharedPreferences.
     */
    public synchronized void loadDataFromSPF() {
        Cursor fastCursor = mResolver.query(FAST_DIAL_QUERY_URI, null, null, null, null);
        fastNumbers.clear();
        if (fastCursor != null) {
            while (fastCursor.moveToNext()) {
                String key = fastCursor.getString(fastCursor.getColumnIndex("key"));
                String value = fastCursor.getString(fastCursor.getColumnIndex("value"));
                if(value != null){
                    value = value.replace(" ", "");
                    fastNumbers.put(key, value);
                }
            }
            fastCursor.close();
        }
        Cursor cursor = null;
        mSelectedItemsList.clear();
        ContactItemInfo itemInfo1 = new ContactItemInfo();
        // add voice box
        itemInfo1.mFastNumber = 1;
        /* SPRD: Fix bug 260485 @{ */
        ArrayList<String> voiceMailList = getVoiceMailList();
        if (voiceMailList.size() > 0) {
            itemInfo1.mCallNumber = voiceMailList.get(0);
            itemInfo1.mNameStr = itemInfo1.mCallNumber;
        } else {
            itemInfo1.mIsMissed = true;
            itemInfo1.mIntent = new Intent();
            /* SPRD: Fix bug 262325 @{
             * itemInfo1.mIntent.setAction(CONTACT_SELECT_APP_ACTION);
             * itemInfo1.mIntent.setType(ContactsContract.Contacts.CONTENT_TYPE);
             */
            /* SPRD: Fix bug 313986 @{
            itemInfo1.mIntent.setAction(Intent.ACTION_GET_CONTENT);
            itemInfo1.mIntent.setType(Phone.CONTENT_ITEM_TYPE);
             */
            itemInfo1.mIntent.setAction(Intent.ACTION_PICK);
            itemInfo1.mIntent.setType(Phone.CONTENT_TYPE);
            /* @} */
            /* @} */
        }
        /* @} */
        mSelectedItemsList.add(itemInfo1);
        for (int i = 2; i < mNowCount + 1; i++) {
            String number = fastNumbers.get("fast_dial_" + i);
            // SPRD : fix bug206430 name is not necessary for defying contactId
            String name = null;//fastNumbers.get("fast_dial_name_" + i );
            ContactItemInfo itemInfo = new ContactItemInfo();
            itemInfo.mFastNumber = i;
            if (number == null || number.equals("")) {
                // this number has no fast dial number
                itemInfo.mIsMissed = true;
                itemInfo.mIntent = new Intent();
                /* SPRD: Fix bug 262325 @{
                 * itemInfo.mIntent.setAction(CONTACT_SELECT_APP_ACTION);
                 * itemInfo.mIntent.setType(ContactsContract.Contacts.CONTENT_TYPE);
                 */
                /* SPRD: Fix bug 313986 @{
                itemInfo.mIntent.setAction(Intent.ACTION_GET_CONTENT);
                itemInfo.mIntent.setType(Phone.CONTENT_ITEM_TYPE);
                 */
                itemInfo.mIntent.setAction(Intent.ACTION_PICK);
                itemInfo.mIntent.setType(Phone.CONTENT_TYPE);
                /* @} */
                /* @} */
                mSelectedItemsList.add(itemInfo);
            } else {
                long mContactId = getContactId(mResolver, number, name);
                if (mContactId == -1) {
                    itemInfo.mNameStr = name == null ? number : name;
                    itemInfo.mCallNumber = number;
                    itemInfo.mContactId = mContactId;
                    mSelectedItemsList.add(itemInfo);
                }
                else{
                    try {
                        cursor = mResolver.query(CONTACT_URI, PROJECTIONS,
                                ContactsContract.Contacts._ID + " = " + mContactId, null, null);
                        if (cursor != null && cursor.moveToFirst()) {
                            itemInfo.mCallNumber = number;
                            itemInfo.mContactId = mContactId;
                            name = cursor
                                    .getString(DISPLAY_NAME_COLUMN_INDEX);
                            itemInfo.mNameStr = name == null ? number : name;
                            itemInfo.mPhotoId = cursor
                                    .getLong(PHOTO_ID_COLUMN_INDEX);
                            itemInfo.mLookUpKey = cursor.getString(LOOKUP_KEY_COLUMN_INDEX);
                            mSelectedItemsList.add(itemInfo);
                        }
                    } finally {
                        if (cursor != null) {
                            cursor.close();
                        }
                    }
                }
            }
        }
        mHasUpdated = true;
    }

    /* SPRD: Fix bug 260485 @{ */
    public String getVoiceMailNumber(int phoneId) {
        if (teleMgr == null) {
            teleMgr = (TelephonyManager) mContext
                    .getSystemService(Context.TELEPHONY_SERVICE);
        }
        TelephonyManager mTelephonyManager = teleMgr.getDefault(phoneId);
        if (mTelephonyManager == null) {
            return null;
        }
        return mTelephonyManager.getVoiceMailNumber();
    }
    /* @} */

    /**
     * Obtain ContactIds that will be loaded.
     */
    private String getItemDisplayName(long id, String name, String hasNumber,
            String phoneNumber) {
        String displayName = null;
        if (name != null) {
            displayName = name;
        } else {
            if (phoneNumber != null) {
                displayName = phoneNumber;
            } else if (hasNumber != null && hasNumber.equals("1")) {
                ArrayList<String> numberList = getContactNumber(mResolver, id);
                displayName = numberList.get(0);
            } else {
                displayName = sDefaultName;
            }
        }
        return displayName;
    }

    public Long getContactId(ContentResolver resolver, String contactNumber, String contactName) {
        Cursor mContactIdCursor = null;
        Long contactId = -1L;
        try {
            mContactIdCursor = resolver.query(PHONE_URI, PHONE_PROJECTIONS,
                    null, null, null);
            if (mContactIdCursor != null) {
                while (mContactIdCursor.moveToNext()) {
                    String temp = mContactIdCursor.getString(
                            PHONE_NUMBER_COLUMN_INDEX).replace(" ", "");
                    String name = mContactIdCursor.getString(
                            PHONE_NAME_COLUMN_INDEX);
                    // SPRD : fix bug206430 name is not necessary for defying contactId
                    if (temp.equals(contactNumber)) {
                        contactId = mContactIdCursor
                                .getLong(PHONE_CONTACT_ID_COLUMN_INDEX);
                        // SPRD : Fix bug 291464
                        break;
                    }
                }
            }
        } finally {
            if (mContactIdCursor != null) {
                mContactIdCursor.close();
            }
        }
        return contactId;
    }

    public ArrayList<String> getContactNumber(ContentResolver resolver,
            long contactId) {
        Cursor numberCursor = null;
        ArrayList<String> contactNumberList = new ArrayList<String>();
        try {
            numberCursor = resolver.query(PHONE_URI, PHONE_PROJECTIONS,
                    PHONE_PROJECTIONS[0] + "=" + contactId, null, null);
            if (numberCursor != null) {
                while (numberCursor.moveToNext()) {
                    String number = numberCursor
                            .getString(PHONE_NUMBER_COLUMN_INDEX);
                    contactNumberList.add(number);
                }
            }
        } finally {
            if (numberCursor != null) {
                numberCursor.close();
            }
        }
        return contactNumberList;
    }

    public ArrayList<String> getContactNumber(long contactId) {

        return getContactNumber(mResolver, contactId);
    }

    /**
     * Compare contact name and sort them.
     */
    static class ContactComparator implements Comparator<ContactItemInfo> {
        private static ContactComparator sContactComparator = null;

        public static ContactComparator getInstance() {
            if (sContactComparator == null) {
                sContactComparator = new ContactComparator();
            }
            return sContactComparator;
        }

        public int compare(ContactItemInfo item1, ContactItemInfo item2) {
            String s1 = item1.mNameStr;
            String s2 = item2.mNameStr;
            if (s1 != null && s2 != null) {
                return s1.compareTo(s2);
            }
            return 0;
        }
    }

    /**
     * Update the contacts data if content has changed.
     */
    public void updateOnResume(ContactAddLayout contactView) {
        if (mContentChanged) {
            updateContactsData(contactView, true);
            mContentChanged = false;
        }
    }

    /**
     * Update the contacts data.
     */
    public void updateContactsData(ContactAddLayout contactView,
            boolean needUpdate) {
        contactView.setProgressBarVisibility(true);
        if (mSelectedItemsList == null) {
            mSelectedItemsList = new ArrayList<ContactItemInfo>();
        } else if (mSelectedItemsList.size() > 0) {
            if (!needUpdate || (needUpdate && mHasUpdated)) {
                if (ISDEBUG) {
                    Log.d(TAG, "updateContactsData use loaded data");
                }
                contactView.notifyChange();
                return;
            }
        }
        if (!mIsUpdating) {
            if (ISDEBUG) {
                Log.d(TAG, "Contact widget data is updating");
            }
            executeTask();
        }
    }

    /**
     * Execute the Async task of loading data.
     */
    private void executeTask() {
        mIsUpdating = true;
        Status nowTaskStatus = mUpdateDataTask.getStatus();

        if (nowTaskStatus.equals(Status.RUNNING)) {
            mUpdateDataTask.cancel(true);
        }
        if (!nowTaskStatus.equals(Status.PENDING)) {
            mUpdateDataTask = new GetSelectedDataTask();
        }

        mUpdateDataTask.execute();
    }

    public void updateMissedData() {
        if (!mIsUpdatingMissData) {
            executeMissedTask();
        }
    }
    /**
     * Execute the Async task of missed data.
     */
    private void executeMissedTask() {
        mIsUpdatingMissData = true;
        Status nowTaskStatus = mUpdateMissedDataTask.getStatus();

        if (nowTaskStatus.equals(Status.RUNNING)) {
            mUpdateMissedDataTask.cancel(true);
        }
        if (!nowTaskStatus.equals(Status.PENDING)) {
            mUpdateMissedDataTask = new GetMissedDataTask();
        }

        mUpdateMissedDataTask.execute();
    }
    /**
     * The Async task to load data.
     */
    class GetSelectedDataTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
        }

        @Override
        protected Void doInBackground(Void... arg0) {
            // load
            loadDataFromSPF();
            restoreMissedCallAndSmsCount();
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            // update
            mContext.getApplicationContext().sendBroadcast(mUpdateIntent);
            mIsUpdating = false;
        }
    }

    /**
     * The Async task to load missed call & sms data.
     */
    class GetMissedDataTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
        }

        @Override
        protected Void doInBackground(Void... arg0) {
            // load
            restoreMissedCallAndSmsCount();
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            // update
            mContext.getApplicationContext().sendBroadcast(mMissedCallAndSmsUpdateIntent);
            mIsUpdatingMissData = false;
        }
    }

    public void unregisterObserver() {
        /* SPRD: bug310651 2014-06-05 OOM problem optimize. @{ */
        try {
            mResolver.unregisterContentObserver(mObserver);
            mResolver.unregisterContentObserver(mMissedObserver);
        } catch (Exception e) {
            e.printStackTrace();
        }
        unRegisterBroadcastReceiver();
        releaseStaticInstance();
        /* SPRD: bug310651 2014-06-05 OOM problem optimize. @} */
    }

    public int findNewSmsCount(long contactId) {
        List<String> numbers = getContactNumber(mResolver, contactId);
        if (numbers == null || numbers.size() == 0) {
            return 0;
        }
        String number = numbers.get(0);
        number=number.replace(" ", "");
        int newSmsCount = 0;
        Cursor csr = null;
        try {
            csr = mContext
                    .getApplicationContext()
                    .getContentResolver()
                    .query(Uri.parse("content://sms"),
                            null,
                            "read = 0 and address like '%"
                                    + number + "'", null, null);
            //type = 1 and
            if (csr != null) {
                newSmsCount = csr.getCount();
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (csr != null) {
                csr.close();
            }
        }
        return newSmsCount;
    }

    public boolean hasChanged() {
        return mContentChanged;
    }

    public int findNewCallCount(long contactId) {
        List<String> numbers = getContactNumber(mResolver, contactId);
        if (numbers == null || numbers.size() == 0) {
            return 0;
        }
        String number = numbers.get(0);
        number=number.replace(" ", "");
        Cursor csr = null;
        int missedCallCount = 0;
        try {
            csr = mContext.getContentResolver().query(Calls.CONTENT_URI,
                    new String[] { Calls.NUMBER, Calls.TYPE, Calls.NEW },
                    "type=" + Calls.MISSED_TYPE + " AND new=1",
                    null, Calls.DEFAULT_SORT_ORDER);
            if (null != csr) {
                while (csr.moveToNext()) {
                    int type = csr.getInt(csr.getColumnIndex(Calls.TYPE));
                    switch (type) {
                        case Calls.MISSED_TYPE:
                            if (csr.getInt(csr.getColumnIndex(Calls.NEW)) == 1) {
                                if (number.equals(csr.getString(csr
                                        .getColumnIndex(Calls.NUMBER)))) {
                                    missedCallCount++;
                                }
                            }
                            break;
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (csr != null) {
                csr.close();
            }
        }
        return missedCallCount;
    }

    /* SPRD: Fix bug 260485 @{ */
    public ArrayList<String> getVoiceMailList() {
        ArrayList<String> voiceMailList = new ArrayList<String>();
        Sim[] sims = null;

        /* SPRD: bug310651 2014-06-05 OOM problem optimize. @{ */
        SimManager mSimManager = SimManager.get(mContext.getApplicationContext());
        /* SPRD: bug310651 2014-06-05 OOM problem optimize. @} */

        sims = mSimManager.getActiveSims();
        if (sims != null) {
            int length = sims.length;
            for (int i = 0; i < length; i++) {
                String number = getVoiceMailNumber(sims[i].getPhoneId());
                if (!TextUtils.isEmpty(number)) {
                    voiceMailList.add(number);
                }
            }
        }
        return voiceMailList;
    }
    /* @} */

    /* SPRD: bug310651 2014-06-05 OOM problem optimize. @{ */
    public static void releaseStaticInstance() {
        sInstance = null;
    }
    /* SPRD: bug310651 2014-06-05 OOM problem optimize. @} */
}
