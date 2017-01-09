/** Create by Spreadst */

package com.spreadst.drag;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.AsyncTask.Status;
import android.os.Handler;
import android.provider.ContactsContract;
import android.telephony.TelephonyManager;

public class ContactsData {

    private static final Uri CONTACT_URI = ContactsContract.Contacts.CONTENT_URI;
    private static final String[] PROJECTIONS = new String[] {
            ContactsContract.Contacts._ID,
            ContactsContract.Contacts.DISPLAY_NAME,
            ContactsContract.Contacts.PHOTO_ID,
            ContactsContract.Contacts.HAS_PHONE_NUMBER
    };
    private static final int DISPLAY_NAME_COLUMN_INDEX = 1;
    private static final int PHOTO_ID_COLUMN_INDEX = 2;
    private static final Uri PHONE_URI = ContactsContract.CommonDataKinds.Phone.CONTENT_URI;
    private static final String[] PHONE_PROJECTIONS = new String[] {
            ContactsContract.CommonDataKinds.Phone.CONTACT_ID,
            ContactsContract.CommonDataKinds.Phone.NUMBER
    };
    private static final int PHONE_NUMBER_COLUMN_INDEX = 1;
    private static final int PHONE_CONTACT_ID_COLUMN_INDEX = 0;
    /**
     * Observe the changes in contacts database.
     */
    private ArrayList<ContactItemInfo> mSelectedItemsList;
    private GetSelectedDataTask mUpdateDataTask;
    private static final int mNowCount = 9;
    private ContentResolver mResolver;
    private static ContactsData sInstance;
    /**
     * Special instance variable.
     */
    private static byte[] lock = new byte[0];
    private Context mContext;
    private TelephonyManager teleMgr[] = new TelephonyManager[TelephonyManager.getPhoneCount()];
    static final String AUTHORITY = "com.android.phone";
    static final Uri FAST_DIAL_QUERY_URI = Uri.parse("content://" + AUTHORITY
            + "/fdnumber/phoneId/0");// sim 0
    static final String FAST_DIAL_INSERT_URI_PREX = "content://" + AUTHORITY
            + "/fdnumber/";

    private Map<String, String> fastNumbers = new HashMap<String, String>();
    private Handler mHandler;

    public static final int DATA_ALREADY_UPLOAD = 10;

    private int mVoiceMailPhoneId;

    public static ContactsData getInstance(Context context, Handler handler) {
        if (sInstance == null) {
            synchronized (lock) {
                if (sInstance == null) {
                    sInstance = new ContactsData(context, handler);
                }
            }
        }
        return sInstance;
    }

    public ContactsData(Context context, Handler handler) {
        init(context, handler);
    }

    private void init(Context context, Handler handler) {
        this.mHandler = handler;
        mContext = context;
        mSelectedItemsList = new ArrayList<ContactItemInfo>();
        mResolver = context.getContentResolver();
        mUpdateDataTask = new GetSelectedDataTask();
        for (int i = 0; i < TelephonyManager.getPhoneCount(); i++) {
            teleMgr[i] = (TelephonyManager) mContext
                    .getSystemService(TelephonyManager
                            .getServiceName(Context.TELEPHONY_SERVICE, i));
        }
    }

    /**
     * Get a list has been cloned from mSelectedItemsList.
     */
    @SuppressWarnings("unchecked")
    public ArrayList<ContactItemInfo> getSelectedItemsList() {
        return (ArrayList<ContactItemInfo>) mSelectedItemsList.clone();
    }

    /**
     * Load data from SharedPreferences.
     */
    private synchronized void loadDataFromSPF() {
        Cursor fastCursor = mResolver.query(FAST_DIAL_QUERY_URI, null, null, null, null);
        try {
            if (fastCursor != null) {
                fastNumbers.clear();
                while (fastCursor.moveToNext()) {
                    String key = fastCursor.getString(fastCursor.getColumnIndex("key"));
                    String value = fastCursor.getString(fastCursor.getColumnIndex("value"));
                    if (value != null) {
                        value = value.replace(" ", "");
                        fastNumbers.put(key, value);
                    }
                }
            }
        } finally {
            if (fastCursor != null) {
                fastCursor.close();
            }
        }
        Cursor cursor = null;
        mSelectedItemsList.clear();
        ContactItemInfo itemInfo1 = new ContactItemInfo();
        // add voice box
        itemInfo1.mFastNumber = 1;
        String voiceMailNumber = getVoiceMailNumber();
        if (voiceMailNumber != null && !voiceMailNumber.equals("")) {
            itemInfo1.mCallNumber = voiceMailNumber;
            itemInfo1.mNameStr = itemInfo1.mCallNumber;
            itemInfo1.voiceMailPhoneId = mVoiceMailPhoneId;
        } else {
            itemInfo1.mIsMissed = true;
        }
        mSelectedItemsList.add(itemInfo1);
        for (int i = 2; i < mNowCount + 1; i++) {
            String number = fastNumbers.get("fast_dial_" + i);
            ContactItemInfo itemInfo = new ContactItemInfo();
            itemInfo.mFastNumber = i;
            if (number == null || number.equals("")) {
                // has no fast dial number
                itemInfo.mIsMissed = true;
                mSelectedItemsList.add(itemInfo);
            } else {
                long mContactId = getContactId(mResolver, number);
                if (mContactId == -1) {
                    // this fast dial number is not a contact's number
                    itemInfo.mNameStr = number;
                    itemInfo.mCallNumber = number;
                    itemInfo.mContactId = mContactId;
                    mSelectedItemsList.add(itemInfo);
                } else {
                    try {
                        cursor = mResolver.query(CONTACT_URI, PROJECTIONS,
                                ContactsContract.Contacts._ID + " = " + mContactId, null, null);
                        if (cursor != null && cursor.moveToFirst()) {
                            itemInfo.mCallNumber = number;
                            itemInfo.mContactId = mContactId;
                            String name = cursor
                                    .getString(DISPLAY_NAME_COLUMN_INDEX);
                            itemInfo.mNameStr = name == null ? number : name;
                            itemInfo.mPhotoId = cursor
                                    .getLong(PHOTO_ID_COLUMN_INDEX);
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
    }

    private String getVoiceMailNumber() {
        String voiceMailNumber = "";
        for (int i = 0; i < TelephonyManager.getPhoneCount(); i++) {
            if (teleMgr[i] != null) {
                voiceMailNumber = teleMgr[i].getVoiceMailNumber();
                if (voiceMailNumber != null && !voiceMailNumber.equals("")) {
                    mVoiceMailPhoneId = i;
                    break;
                }
            }
        }
        return voiceMailNumber;
    }

    public Long getContactId(ContentResolver resolver, String contactNumber) {
        Cursor mContactIdCursor = null;
        Long contactId = -1L;
        try {
            mContactIdCursor = resolver.query(PHONE_URI, PHONE_PROJECTIONS,
                    null, null, null);
            if (mContactIdCursor != null) {
                while (mContactIdCursor.moveToNext()) {
                    String temp = mContactIdCursor.getString(
                            PHONE_NUMBER_COLUMN_INDEX).replace(" ", "");
                    if (temp.equals(contactNumber)) {
                        contactId = mContactIdCursor
                                .getLong(PHONE_CONTACT_ID_COLUMN_INDEX);
                        return contactId;
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
     * start load data
     */
    public void startLoadData() {
        executeTask();
    }

    /**
     * Execute the Async task of loading data.
     */
    private void executeTask() {
        Status nowTaskStatus = mUpdateDataTask.getStatus();

        if (nowTaskStatus.equals(Status.RUNNING)) {
            mUpdateDataTask.cancel(true);
        }
        if (!nowTaskStatus.equals(Status.PENDING)) {
            mUpdateDataTask = new GetSelectedDataTask();
        }

        mUpdateDataTask.execute();
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
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            // update
            mHandler.sendEmptyMessage(DATA_ALREADY_UPLOAD);
        }
    }

    public void cancelTask() {
        if (mUpdateDataTask != null) {
            Status nowTaskStatus = mUpdateDataTask.getStatus();
            if (nowTaskStatus.equals(Status.RUNNING)) {
                mUpdateDataTask.cancel(true);
            }
        }
        if (mHandler != null) {
            mHandler.removeMessages(DATA_ALREADY_UPLOAD);
            mHandler = null;
        }

    }

}
