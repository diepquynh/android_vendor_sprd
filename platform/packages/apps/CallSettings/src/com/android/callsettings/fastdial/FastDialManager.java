/**
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 *
 *   FastDialManager.java
 *   Created at 2:38:45 PM, Sep 1, 2015
 *
 */

package com.android.callsettings.fastdial;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.PhoneConstants;
import com.android.callsettings.R;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.provider.ContactsContract.CommonDataKinds;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Data;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.GridView;
import android.widget.SimpleAdapter;

import java.util.ArrayList;
import java.util.HashMap;
import com.android.callsettings.plugins.CallSettingsRelianceHelper;

public class FastDialManager {

    static final String TAG = "FastDial";
    private static final String PHONE_PACKAGE = "com.android.phone";
    private static final String SHARED_PREFERENCES_NAME = "fast_dial_numbers";
    private static final String NUM_PROJECTION[] = {
        CommonDataKinds.Phone.NUMBER,
        Data.DISPLAY_NAME,
        Data.PHOTO_URI,
        Data.CONTACT_ID,
        "account_type", // Contacts.DISPLAY_ACCOUNT_TYPE, // FIXME
        Data._ID
    };
    private final static int MSG_FLUSH_FD_MEM = 1;

    private static FastDialManager sFastDialManager;

    private Context mContext;
    private SharedPreferences mPerSavedFDNumbers;
    private int mPhoneCount;
    private ArrayList<HashMap<String, Object>> memList;
    private HashMap<Object, HashMap<String, Object>> mSimTmpMemMap;
    private SimpleAdapter mAdapter;

    private boolean mContactsBootLoaded = false;
    private int mFinalSimStateFlag = 0;
    private int mNeedSimLoadedCount = 0;
    private boolean editUriChangeBySelf = false;

    private FastDialManager(Context context) {
        mContext = context;
        memList = new ArrayList<HashMap<String, Object>>();
        mSimTmpMemMap = new HashMap<Object, HashMap<String, Object>>();
        mAdapter = new SimpleAdapter(mContext,
                memList,
                R.layout.fast_dial_grid_layout_ex,
                new String[] {
                        "img_back", "img_num", "img_lmail", "contacts_cell_name"
                },
                new int[] {
                        R.id.img_back, R.id.img_num, R.id.img_lmail, R.id.contacts_cell_name
                });
        try {
            Context appContext = mContext.getApplicationContext();
            Context phoneContext = appContext.createPackageContext(
                    PHONE_PACKAGE, Context.CONTEXT_IGNORE_SECURITY);
            mPerSavedFDNumbers = phoneContext.getSharedPreferences(
                    SHARED_PREFERENCES_NAME,
                    Context.MODE_WORLD_READABLE | Context.MODE_MULTI_PROCESS);
            mPerSavedFDNumbers.registerOnSharedPreferenceChangeListener(
                    mSharedPreferenceChangeListener);
            CallSettingsRelianceHelper.getInstance().initEmergencyNumber(this);

        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }
        mProcessHandler.sendEmptyMessage(MSG_FLUSH_FD_MEM);
        mPhoneCount = TelephonyManager.from(mContext).getPhoneCount();
        Log.d(TAG, "FastDialManager: " + this);
    }

    public static void init(Context context) {
        if (sFastDialManager == null) {
            sFastDialManager = new FastDialManager(context);
        }
    }

    public static FastDialManager getInstance() {
        return sFastDialManager;
    }

    private ContentObserver mObserver = new ContentObserver(new Handler()) {
        public void onChange(boolean selfChange, Uri uri) {
            Log.d(TAG, "fastDialManager onChange: " + selfChange);
            flushFdMem();
        };
    };

    private Handler mProcessHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_FLUSH_FD_MEM:
                    new ContactQueryTask().execute();
                    break;
                default:
                    Log.wtf(TAG, "what happened, we should not get here!");
                    break;
        }
    }
    };

    private void registerFastContactsURI() {
        mContext.getContentResolver().registerContentObserver(Data.CONTENT_URI, true, mObserver);
    }

    private void unRegisterFastContactsURI() {
        mContext.getContentResolver().unregisterContentObserver(mObserver);
    }

    private class ContactQueryTask extends AsyncTask<Integer, Void, Integer> {
        @Override
        protected Integer doInBackground(Integer... params) {
            for (int i = 1; i < 10; i++) {
                String mName = null;
                String mPhoto = null;
                String mContactId = null;
                String mNumber = null;
                String mDataUri = null;
                String mAccountType = null;
                boolean mSimContactChange = false;
                HashMap<String, Object> map = new HashMap<String, Object>();
                map.put("img_num", getNumResourceId(i));
                if (i == 1) {
                    map.put("img_back", R.drawable.fastdial_grid_background_ex);
                    map.put("img_lmail", R.drawable.contact_qds_icon_lmail_ex);
                    map.put("contacts_cell_name", mContext.getString(R.string.voicemail));
                } else {
                    if (mPerSavedFDNumbers == null) {
                        return 0;
                    }
                    mDataUri = mPerSavedFDNumbers.getString("fast_dial_dataUri_" + i, "");
                    Cursor cursor1 = null;
                    try {
                        cursor1 = mContext.getContentResolver().query(Uri.parse(mDataUri),
                                NUM_PROJECTION,
                                null, null, null);
                        if (cursor1 != null && cursor1.moveToFirst()) {
                            mNumber = cursor1.getString(0);
                            mName = cursor1.getString(1);
                            mPhoto = cursor1.getString(2);
                            mContactId = cursor1.getString(3);
                            mAccountType = cursor1.getString(4);
                        } else if (TextUtils.isEmpty(mDataUri)) {
                            mNumber = mPerSavedFDNumbers.getString("fast_dial_" + i, "");
                        }
                    } finally {
                        /* SPRD: add for bug515865 @{ */
                        if (mNumber == null) {
                            mNumber = mPerSavedFDNumbers.getString("fast_dial_" + i, "");
                            Log.d(TAG, "mNumber = " + mNumber);
                        }
                        /* @} */
                        if (cursor1 != null) {
                            cursor1.close();
                        }
                    }

                    Log.d(TAG, "mContactId: " + mContactId + "   "
                        + mPerSavedFDNumbers.getString("fast_dial_" + i + "_contactId", "") + "   "
                        + mName + "   " + mNumber + "   "
                        + mAccountType + "     " + i);
                    if (TextUtils.isEmpty(mContactId) && !mSimTmpMemMap.isEmpty()
                            && mContactsBootLoaded) {
                        HashMap map2 = mSimTmpMemMap.get(i);
                        if (map2 != null) {
                            String name = (String) map2.get("name");
                            String number = (String) map2.get("number");
                            String accountType = (String) map2.get("accountType");
                            Cursor cursor2 = null;
                            try {
                                cursor2 = mContext.getContentResolver().query(
                                        Data.CONTENT_URI,
                                        NUM_PROJECTION,
                                        CommonDataKinds.Phone.NUMBER + "=? AND "
                                                + Data.DISPLAY_NAME + "=? AND "
                                                + "account_type"  + "=?",
                                        new String[] {
                                                number, name, accountType
                                        }, null);
                                if (cursor2 != null && cursor2.moveToFirst()) {
                                    mNumber = cursor2.getString(0);
                                    mName = cursor2.getString(1);
                                    mPhoto = cursor2.getString(2);
                                    mContactId = cursor2.getString(3);
                                    mAccountType = cursor2.getString(4);
                                    String id = cursor2.getString(5);
                                    mDataUri = Data.CONTENT_URI.toString() + "/" + id;
                                    Log.d(TAG, "reflesh sim contact, new dataUri: " + mDataUri);
                                    editUriChangeBySelf = true;
                                    removeSimTmpMem(i);
                                }
                                mSimContactChange = true;
                                if ((mFinalSimStateFlag
                                          & ((1 << mPhoneCount) - 1)) == (1 << mPhoneCount) - 1) {
                                    if (0 == mNeedSimLoadedCount) {
                                        removeSimTmpMem(i);
                                    }
                                }
                            } finally {
                                if (cursor2 != null) {
                                    cursor2.close();
                                }
                            }
                        }
                    }

                    if (!TextUtils.isEmpty(mPerSavedFDNumbers.getString("fast_dial_" + i
                            + "_contactId", "")) || mSimContactChange) {
                        if (mPerSavedFDNumbers.getString("fast_dial_accountType_" + i, "")
                                .endsWith("sim") && !mContactsBootLoaded) {
                            Log.d(TAG,
                                  "accountType is sim  and is not contactsBootLoaded to cache");
                            HashMap<String, Object> map2 = new HashMap<String, Object>();
                            map2.put("name", mPerSavedFDNumbers
                                                  .getString("fast_dial_name_" + i, ""));
                            map2.put("number", mPerSavedFDNumbers.getString("fast_dial_" + i, ""));
                            map2.put("accountType",
                                    mPerSavedFDNumbers.getString("fast_dial_accountType_" + i, ""));
                            mSimTmpMemMap.put(i, map2);
                        }

                        if (!TextUtils.isEmpty(mContactId) && TextUtils.isEmpty(mPhoto)) {
                            Cursor cursor3 = null;
                            try {
                                cursor3 = mContext.getContentResolver().query(Data.CONTENT_URI,
                                        NUM_PROJECTION, Data.CONTACT_ID + "=?",
                                        new String[] { mContactId }, null);
                                if (cursor3 != null && cursor3.moveToFirst()) {
                                    mPhoto = cursor3
                                               .getString(cursor3.getColumnIndex(Data.PHOTO_URI));
                                }
                            } finally {
                                if (cursor3 != null) {
                                    cursor3.close();
                                }
                            }
                        }
                        updateExistFDPer(i, mName, mNumber, mPhoto, mContactId, mDataUri,
                                mAccountType);
                    }

                    if (TextUtils.isEmpty(mNumber)) {
                        map.put("img_back", R.drawable.fastdial_grid_background_ex);
                        map.put("img_lmail", R.drawable.contact_qds_icon_add_ex);
                        map.put("contacts_cell_name", mContext.getString(R.string.menu_add));
                    } else {
                        Log.d(TAG, "mName= " + mName + "  mPhoto= " + mPhoto);
                        if (TextUtils.isEmpty(mPhoto)) {
                            map.put("img_back", R.drawable.call_detail_photo_ex);
                        } else {
                            map.put("img_back", Uri.parse(mPhoto));
                        }
                        map.put("img_lmail", null);
                        if (TextUtils.isEmpty(mName)) {
                            map.put("contacts_cell_name", mNumber);
                        } else {
                            map.put("contacts_cell_name", mName);
                        }
                    }
                }
                if(memList.size() == 9){
                    memList.remove(i-1);
                }
                memList.add(i-1, map);
            }
            return 0;
        }

        protected void onPostExecute(Integer result) {
            mAdapter.notifyDataSetChanged();
        }

        private void updateExistFDPer(int index, String name, String number, String photo,
                String contactId, String dataUri, String accountType) {
            if (mPerSavedFDNumbers == null) {
                return;
            }
            if (TextUtils.isEmpty(number)) {
                editUriChangeBySelf = true;
                saveFastDialNumber(index, null, null, null, null, null, null);
            } else if (TextUtils.isEmpty(name)) {
                saveFastDialNumber(index, number, number, photo, contactId, dataUri, accountType);
            } else if (!mPerSavedFDNumbers.getString("fast_dial_" + index, "").equals(number)
                    || !mPerSavedFDNumbers.getString("fast_dial_name_" + index, "").equals(name)
                    || !mPerSavedFDNumbers.getString("fast_dial_photo_" + index, "").equals(
                            photo == null ? "" : photo)
                    || !mPerSavedFDNumbers.getString("fast_dial_dataUri_" + index, "").equals(
                            dataUri)) {
                saveFastDialNumber(index, number, name, photo, contactId, dataUri, accountType);
            }
        }
    }

    public void setGridView(final View gridView) {
        if (gridView != null) {
            ((GridView) gridView).setAdapter(mAdapter);
        }
    }

    private int getNumResourceId(int num) {
        int resId = 0;
        switch (num) {
            case 1:
                resId = R.drawable.contact_nub_01_ex;
                break;
            case 2:
                resId = R.drawable.contact_nub_02_ex;
                break;
            case 3:
                resId = R.drawable.contact_nub_03_ex;
                break;
            case 4:
                resId = R.drawable.contact_nub_04_ex;
                break;
            case 5:
                resId = R.drawable.contact_nub_05_ex;
                break;
            case 6:
                resId = R.drawable.contact_nub_06_ex;
                break;
            case 7:
                resId = R.drawable.contact_nub_07_ex;
                break;
            case 8:
                resId = R.drawable.contact_nub_08_ex;
                break;
            case 9:
                resId = R.drawable.contact_nub_09_ex;
                break;
            default:
                resId = 0;
                break;
        }
        return resId;
    }

    public void saveFastDialNumber(int key, String phoneNumber, String phoneName, String photoUri,
            String contactId, String dataUri, String accountType) {
        if (mPerSavedFDNumbers == null || key - 2 < 0) {
            Log.d(TAG, "mPerSavedFDNumbers=null or key<2 " + key);
            return;
        }
        if (phoneNumber == null) {
            phoneNumber = "";
        }
        Editor editor = mPerSavedFDNumbers.edit();
        editor.putString("fast_dial_" + key, phoneNumber);
        editor.putString("fast_dial_name_" + key, phoneName);
        editor.putString("fast_dial_photo_" + key, photoUri);
        editor.putString("fast_dial_" + key + "_contactId", contactId);
        editor.putString("fast_dial_dataUri_" + key, dataUri);
        editor.putString("fast_dial_accountType_" + key, accountType);
        editor.apply();

        sendFastDialNumberChangedBroadcast(0);
        sendFastDialNumberChangedBroadcast(1);
    }

    private void sendFastDialNumberChangedBroadcast(int phoneId) {
        // send a broad to notify fastdial changed,it will work for home widget
        Intent intent = new Intent("intent.action.phone.fdnumber_changed");
        intent.putExtra("phoneId", phoneId);
        mContext.sendBroadcast(intent);
    }

    public void addFastDial(Intent myData, int fastDialIndex) {
        addFastDial(myData, fastDialIndex, null);
    }
    /* SPRD: add for bug402764 @{ */
    public void addFastDial(Intent myData, int fastDialIndex, String number) {
        addFastDial(myData, fastDialIndex, number, null);
    }
    /* @} */

    private void addFastDial(final Intent myData, final int fastDialIndex,
             final String number, final String strUri) {

        new AsyncTask<Object, Object, Object>() {

            String phoneNumber;
            String phoneName;
            String photoUri;
            String contactId;
            String dataUri;
            String accountType;

            @Override
            protected Object doInBackground(Object... params) {
                /* SPRD: add for bug402764 @{ */
                if (myData == null && strUri == null && number != null) {
                    phoneNumber = number;
                    editUriChangeBySelf = true;
                    return 0;
                }
                /* @} */
                Cursor cursor = null;
                Uri uri = null;
                try {
                    if (myData != null) {
                        uri = myData.getData();
                        editUriChangeBySelf = true;
                    } else {
                        uri = Uri.parse(strUri);
                        editUriChangeBySelf = false;
                    }
                    if (uri != null) {
                        cursor = mContext.getContentResolver().query(uri, NUM_PROJECTION,
                                null, null, null);
                    }
                    if ((cursor == null) || (!cursor.moveToFirst())) {
                        return null;
                    }
                    phoneNumber = cursor.getString(0);
                    phoneName = cursor.getString(1);
                    photoUri = cursor.getString(2);
                    contactId = cursor.getString(3);
                    accountType = cursor.getString(4);
                    dataUri = uri.toString();
                } finally {
                    if (cursor != null) {
                        cursor.close();
                    }
                }
                return 0;
            }

            @Override
            protected void onPostExecute(Object object) {
                if (object == null) {
                    Log.d(TAG, "onActivityResult: bad contact data, no results found.");
                    return;
                }

                Log.d(TAG, "select number is : " + phoneNumber);
                Log.d(TAG, "select name is : " + phoneName);
                Log.d(TAG, "select photoUri is : " + photoUri);
                Log.d(TAG, "select contactId is : " + contactId);
                Log.d(TAG, "select accountType is : " + accountType);
                // update UI
                HashMap<String, Object> map = new HashMap<String, Object>();
                if (photoUri == null) {
                    map.put("img_back", R.drawable.call_detail_photo_ex);
                } else {
                    map.put("img_back", Uri.parse(photoUri));
                }
                map.put("img_num", getNumResourceId(fastDialIndex));
                map.put("img_lmail", null);
                /* SPRD: add for bug402764 @{ */
                if (!TextUtils.isEmpty(phoneName)) {
                    map.put("contacts_cell_name", phoneName);
                } else {
                    map.put("contacts_cell_name", phoneNumber);
                }
                /* @} */
                Log.d(TAG, "onPostExecute fastDialIndex: " + fastDialIndex);
                if (fastDialIndex <= 0) {
                    return;
                } else if (memList.size() == 9) {
                    memList.remove(fastDialIndex - 1);
                    memList.add(fastDialIndex - 1, map);
                    removeSimTmpMem(fastDialIndex);
                }
                mAdapter.notifyDataSetChanged();
                // store phoneNumber
                saveFastDialNumber(fastDialIndex, phoneNumber, phoneName, photoUri, contactId,
                        dataUri, accountType);

            }
        }.execute();
    }

    public void deleteFastDial(int fastDialIndex) {
        HashMap<String, Object> map = new HashMap<String, Object>();
        map.put("img_num", getNumResourceId(fastDialIndex));
        map.put("img_back", R.drawable.fastdial_grid_background_ex);
        map.put("img_lmail", R.drawable.contact_qds_icon_add_ex);
        map.put("contacts_cell_name", mContext.getString(R.string.menu_add));
        memList.remove(fastDialIndex - 1);
        memList.add(fastDialIndex - 1, map);
        mAdapter.notifyDataSetChanged();
        // clear phoneNumber
        editUriChangeBySelf = true;
        removeSimTmpMem(fastDialIndex);
        saveFastDialNumber(fastDialIndex, null, null, null, null, null, null);
    }

    public void onAppWidgetUpdate () {
        Log.d(TAG, "onAppWidgetUpdate");
        mNeedSimLoadedCount--;
        if (!mContactsBootLoaded) {
            mContactsBootLoaded = true;
            registerFastContactsURI();
        }
        flushFdMem();
    }

    public void onSimStateChanged(Intent intent) {
        int simPinOrPukCount = 0;
        if (!mContactsBootLoaded) {
            int phoneId = intent.getIntExtra(PhoneConstants.SUBSCRIPTION_KEY, 0);
            boolean isAbsent = IccCardConstants.INTENT_VALUE_ICC_ABSENT
                           .equals(intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE));
            boolean isLoaded = IccCardConstants.INTENT_VALUE_ICC_LOADED
                    .equals(intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE));
            boolean isSimPinOrPuk = IccCardConstants.INTENT_VALUE_ICC_LOCKED
                    .equals(intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE));
            Log.d(TAG, "isAbsent:" + isAbsent + " isLoaded:" + isLoaded + " isSimPinOrPuk:"
                    + isSimPinOrPuk + " phoneId:" + phoneId);

            if (isAbsent || isLoaded || isSimPinOrPuk) {
                if ((mFinalSimStateFlag & (1 << phoneId)) != (mFinalSimStateFlag | (1 << phoneId))) {
                    if (isLoaded) {
                        mNeedSimLoadedCount++;
                    } else if (isSimPinOrPuk) {
                        mNeedSimLoadedCount++;
                        simPinOrPukCount++;
                    }
                }
                mFinalSimStateFlag |= (1 << phoneId);
            }
            Log.d(TAG, "  needSimLoadedCount:" + mNeedSimLoadedCount
                    + "  simPinOrPukCount:"
                    + simPinOrPukCount + "  mFinalSimStateFlag:" + mFinalSimStateFlag);

            if ((mFinalSimStateFlag & ((1 << mPhoneCount) - 1)) == (1 << mPhoneCount) - 1) {
                if (mNeedSimLoadedCount == 0 || mNeedSimLoadedCount == simPinOrPukCount) {
                    Log.d(TAG, "no sim card to registerFastContactsURI");
                    mContactsBootLoaded = true;
                    registerFastContactsURI();
                    flushFdMem();
                }
            }
        }
    }

    public void onLocaleChanged() {
        Log.d(TAG, "onLocaleChanged.........");
        flushFdMem();
    }

    // SPRD: modify for bug601333
    public void flushFdMem() {
        mProcessHandler.removeMessages(MSG_FLUSH_FD_MEM);
        mProcessHandler.sendEmptyMessageDelayed(MSG_FLUSH_FD_MEM, 1000);
    }

    private OnSharedPreferenceChangeListener mSharedPreferenceChangeListener
                                                = new OnSharedPreferenceChangeListener() {
        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            // Log.d(TAG, "SharedPreferenceChanged key: " + key);
            if (key.contains("dataUri")) {
                if (!editUriChangeBySelf) {
                    if (mPerSavedFDNumbers == null) {
                        return;
                    }
                    String uri = mPerSavedFDNumbers.getString(key, "");
                    int index = Integer.valueOf(key.substring(key.length() - 1));
                    Log.d(TAG, "SharedPreferenceChanged by other: " + uri + "   " + index);
                    reload4OtherAdd(index, uri);
                } else {
                    editUriChangeBySelf = false;
                }
            }
        }
    };

    private void reload4OtherAdd(int fastDialIndex, String uri) {
        addFastDial(null, fastDialIndex, null, uri);
    }

    private void removeSimTmpMem(int i) {
        mSimTmpMemMap.remove(i);
        Log.d(TAG, "remove SimTmpMemMap index:" + i + ", size:" + mSimTmpMemMap.size());
    }
}
