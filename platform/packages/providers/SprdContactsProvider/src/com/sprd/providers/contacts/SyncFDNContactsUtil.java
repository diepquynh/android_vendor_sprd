package com.sprd.providers.contacts;


import java.util.ArrayList;

import com.android.providers.contacts.ContactsDatabaseHelper;
import com.android.providers.contacts.ContactsDatabaseHelper.Tables;

import android.accounts.Account;
import android.content.ContentProviderOperation;
import android.content.ContentProviderResult;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;
import android.os.AsyncTask;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.text.TextUtils;
import android.util.Log;

public class SyncFDNContactsUtil {
    private static String TAG = SyncFDNContactsUtil.class.getSimpleName();

    private static SyncFDNContactsUtil mInstance = new SyncFDNContactsUtil();
    private SyncFDNContactsUtil(){

    }

    /**
     * Util class uses Single mode.
     * @return
     */
    public static SyncFDNContactsUtil getInstance(){
        if (mInstance == null) {
            return new SyncFDNContactsUtil();
        }
        return mInstance;
    }

    private static String FDN_SEARCH_URI = "content://icc/fdn/subId/";

    private static String FDN_SEARCH_NAME = "name";
    private static String FDN_SEARCH_NUMBER = "number";
    private static String[] FDN_SEARCH_PROJECTION = {
        FDN_SEARCH_NAME,
        FDN_SEARCH_NUMBER
    };
    private ImportFDNTask mImportTask;
    private int mCancelPhoneId = -1;

    public boolean importFDNContacts(Context context, int phoneId, int subId,
            Account account) {
        try {
            mImportTask = new ImportFDNTask(context, phoneId,
                    subId, account);
            mImportTask.execute("");
        } catch (Exception e) {
            Log.d(TAG, "error when start mImportTask : " + e.getMessage());
            e.printStackTrace();
            return false;
        }
        return true;
    }

    /**
     * Import task to start a query and insert task to insert FDN contacts to contacts2.db
     *
     * Insert phase: insert to table in db, [raw_contacts] [data], we only need name and number
     * of a contact such as SDN contacts.
     * @author
     *
     */
    private class ImportFDNTask extends AsyncTask<Object, Object, String> {
        private Context mContext;
        // sim-card id
        private int mPhoneId;
        private int mSubId;
        private Account mAccount;

        public ImportFDNTask(Context context, int phoneId, int subId,
                Account account) {
            this.mContext = context;
            this.mPhoneId = phoneId;
            this.mSubId = subId;
            this.mAccount = account;
        }

        /**
         * FDN contacts has 10 p at most, so it's not needed to use batch operation,
         * just insert one by one is OK, it will not spend more time.
         */
        @Override
        protected String doInBackground(Object... params) {
            String uri = FDN_SEARCH_URI + mSubId;

            Cursor cursor = null;
            String fdnNumber = "";
            String fdnName = "";
            try {
                cursor = mContext.getContentResolver().query(Uri.parse(uri),
                        FDN_SEARCH_PROJECTION, null, null, null);
                Log.d(TAG, "ImportFDNTask-SearchUri : " + uri + ", cursor.getCount() = " + cursor.getCount());
                if (cursor != null && cursor.getCount() > 0) {
                    if (cursor.moveToFirst()) {
                        do {
                            fdnNumber = cursor.getString(cursor
                                    .getColumnIndexOrThrow(FDN_SEARCH_NUMBER));
                            fdnName = cursor.getString(cursor
                                    .getColumnIndexOrThrow(FDN_SEARCH_NAME));
                            Log.d(TAG, "ImportFDNTask-SearchUri : fdnNumber = " + fdnNumber + " | fdnName = " + fdnName);
                            if (isCancelled() && mCancelPhoneId == mPhoneId) {
                                Log.d(TAG, "AsyncTask is Cancelled, mCancelPhoneId = " + mCancelPhoneId);
                                mCancelPhoneId = -1;
                                break;
                            } else {
                                importFDNContactsToContactsDB(fdnNumber, fdnName,
                                        mContext.getContentResolver());
                            }
                        } while (cursor.moveToNext());
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "query FDN list error : " + e.getMessage());
                e.printStackTrace();
            } finally {
                if (null != cursor) {
                    cursor.close();
                    cursor = null;
                }
            }

            return null;
        }

        /**
         * Insert FDN contacts need insert two main table in DB.
         * Wile insert raw_contacts DB will insert contacts by trigger, and while insert
         * data DB will insert search_index/name_lookup/phone_lookup/... tables
         * So we only need insert raw_contact and data, the search function will be supported
         * auto. (Search need search_index table).
         *
         * @param fdnNumber FDN contacts number
         * @param fdnName FDN contacts name
         * @param contentResolver DB proxy
         * @return
         */
        public boolean importFDNContactsToContactsDB(String fdnNumber, String fdnName,
                ContentResolver contentResolver) {
            ArrayList<ContentProviderOperation> ops = new ArrayList<ContentProviderOperation>();
            insertRawContacts(ops);
            insertData(ops, fdnNumber, fdnName, ops.size() - 1);

            try {
                ContentProviderResult[] results = contentResolver.applyBatch(
                        ContactsContract.AUTHORITY,
                        ops);
            } catch (Exception e) {
                Log.e(TAG, "insert FDN list error : " + e.getMessage());
                e.printStackTrace();
            } finally {
                ops.clear();
            }

            return true;
        }

        public boolean deleteFDNContactsInContactsDB(String fdnNumber, String fdnName, String phoneId,
                ContentResolver contentResolver) {
            String where = RawContacts.DISPLAY_NAME_PRIMARY + " = '" + fdnName
                    + "' AND " + RawContacts.SYNC2 + " = 'fdn" + phoneId + "'";
            try {
                contentResolver.delete(RawContacts.CONTENT_URI, where, null);
            } catch (Exception e) {
                Log.e(TAG, "delete FDN contact error : " + e.getMessage());
                e.printStackTrace();
                return false;
            } finally {
            }
            return true;
        }

        public boolean updateFDNContactsToContactsDB(String fdnNumber, String originalFdnName, String fdnName, String phoneId,
                ContentResolver contentResolver) {
            deleteFDNContactsInContactsDB(fdnNumber, originalFdnName, phoneId, contentResolver);
            importFDNContactsToContactsDB(fdnNumber, fdnName, contentResolver);
            return true;
        }

        private void insertRawContacts(ArrayList<ContentProviderOperation> ops) {
            ContentValues values = new ContentValues();
            values.put(RawContacts.ACCOUNT_NAME, mAccount.name);
            values.put(RawContacts.ACCOUNT_TYPE, mAccount.type);
            values.put(RawContacts.AGGREGATION_MODE, RawContacts.AGGREGATION_MODE_DISABLED);
            values.put(RawContacts.RAW_CONTACT_IS_READ_ONLY, 1);
            /**
             * It's very important for this column: RawContacts.SYNC2
             * In all show page, we will use this column to limit menu/option/...
             */
            values.put(RawContacts.SYNC2, "fdn" + mPhoneId);
            //values.put(RawContacts.SYNC3, FDN_SEARCH_URI + mPhoneId);

            ContentProviderOperation rawContactOperation = ContentProviderOperation
                    .newInsert(
                            RawContacts.CONTENT_URI
                                    .buildUpon()
                                    .appendQueryParameter(
                                            ContactsContract.CALLER_IS_SYNCADAPTER, "true")
                                    .build())
                    .withValues(values).build();
            ops.add(rawContactOperation);
        }

        private void insertData(ArrayList<ContentProviderOperation> ops,
                String fdnNumber, String fdnName, int idOffset) {
            if (!TextUtils.isEmpty(fdnName)) {
                ContentValues values = new ContentValues();
                values.put(Data.MIMETYPE, StructuredName.CONTENT_ITEM_TYPE);
                values.put(StructuredName.DISPLAY_NAME, fdnName);
                ContentProviderOperation operation = ContentProviderOperation
                        .newInsert(
                                Data.CONTENT_URI
                                        .buildUpon()
                                        .appendQueryParameter(
                                                ContactsContract.CALLER_IS_SYNCADAPTER,
                                                "true").build())
                        .withValueBackReference(Data.RAW_CONTACT_ID, idOffset)
                        .withValues(values).withYieldAllowed(true).build();
                ops.add(operation);
            }
            if (!TextUtils.isEmpty(fdnNumber)) {
                ContentValues values = new ContentValues();
                values.put(Data.MIMETYPE, Phone.CONTENT_ITEM_TYPE);
                values.put(Phone.NUMBER, fdnNumber);
                values.put(Phone.TYPE, Phone.TYPE_MOBILE);
                ContentProviderOperation operation = ContentProviderOperation
                        .newInsert(
                                Data.CONTENT_URI
                                        .buildUpon()
                                        .appendQueryParameter(
                                                ContactsContract.CALLER_IS_SYNCADAPTER,
                                                "true").build())
                        .withValueBackReference(Data.RAW_CONTACT_ID, idOffset)
                        .withValues(values).withYieldAllowed(true).build();
                ops.add(operation);
            }
        }
    }

    //SPRD: add for bug621877, fdn feature bugfix
    public boolean purgeFDNContacts(Context mContext, String phoneId){
        Log.d(TAG, " ---> purgeFdnContact : " + phoneId);
        try {
            if (mImportTask != null) {
                mImportTask.cancel(true);
                mCancelPhoneId = Integer.parseInt(phoneId);
            }
            SQLiteDatabase db = ContactsDatabaseHelper.getInstance(mContext)
                    .getWritableDatabase();
            db.execSQL("DELETE FROM " + Tables.RAW_CONTACTS
                    + " WHERE sync2 = 'fdn" + phoneId + "'");
        } catch (Exception e) {
            Log.e(TAG, " ---> purgeFdnContact error : " + e.getMessage());
            e.printStackTrace();
            return false;
        }
        return true;
    }
}
