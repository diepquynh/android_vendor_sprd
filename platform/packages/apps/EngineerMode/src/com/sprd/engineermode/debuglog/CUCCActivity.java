
package com.sprd.engineermode.debuglog;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.R.xml;
import com.sprd.engineermode.utils.IATUtils;

import android.content.ContentValues;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.net.Uri;
import android.os.SystemProperties;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.TwoStatePreference;
import android.provider.Settings;
import android.provider.Telephony;
import android.util.Log;
import android.widget.Toast;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import android.content.Intent;
import android.content.ComponentName;
import android.database.Cursor;
import android.content.ContentUris;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;

import com.android.internal.telephony.TelephonyProperties;

public class CUCCActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener, OnSharedPreferenceChangeListener,
        Preference.OnPreferenceClickListener {

    private static final String TAG = "CUCCActivity";
    private static final String KEY_REAL_NETWORK = "real_network";
    private static final String KEY_SUPPLSERVICEQUERY = "supplementary_service_query";
    private static final String KEY_SUPPLSERVICEQUERY1 = "supplementary_service_query1";
    private static final String KEY_APN_SETTING = "apn_setting";
    private static final String KEY_SELECT_SIM = "select_sim";
    private static final String KEY_INSERT_APN = "cucc_insert_apn";

    private static final String DEFAULT_APN = "test.rohde-schwarz";
    private static final int APN_INDEX = 2;

    private static final int SET_REAL_NET_OPEN = 1;
    private static final int SET_REAL_NET_CLOSE = 2;
    private static final int SET_CFU = 3;
    private static final int GET_NET_STATUS = 4;

    private static final String CFU_CONTROL = "persist.sys.callforwarding";

    private TwoStatePreference mRealNet;
    private ListPreference mSupplementaryServiceQuery;
    private ListPreference mSupplementaryServiceQuery1;
    private Preference mAPNSetting;
    private SharedPreferences mSharePref;
    private SubscriptionManager mSubscriptionManager;

    private Handler mUiThread = new Handler();
    private CUCCHandler mCUCCHandler;

    private String mATCmd;
    private String mATResponse;
    private Context mContext;
    private int mPhoneSimCount;
    private boolean mIsCardExist = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_cucc);
        mContext = this;
        mSubscriptionManager = (SubscriptionManager) SubscriptionManager
                .from(mContext);
        mRealNet = (TwoStatePreference) findPreference(KEY_REAL_NETWORK);
        mRealNet.setOnPreferenceChangeListener(this);
        // SPRD:fix 264196
        mRealNet.setSummary(R.string.not_support_well);
        mPhoneSimCount = TelephonyManager.from(this).getPhoneCount();
//        mAPNSetting = (Preference) findPreference(KEY_APN_SETTING);
//        mAPNSetting.setOnPreferenceClickListener(this);
        mSupplementaryServiceQuery = (ListPreference) findPreference(KEY_SUPPLSERVICEQUERY);
        mSupplementaryServiceQuery1 = (ListPreference) findPreference(KEY_SUPPLSERVICEQUERY1);
        mSharePref = PreferenceManager.getDefaultSharedPreferences(this);
        mSharePref.registerOnSharedPreferenceChangeListener(this);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mCUCCHandler = new CUCCHandler(ht.getLooper());
    }

    @Override
    protected void onStart() {
        mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery.getEntry());
        mSupplementaryServiceQuery1.setSummary(mSupplementaryServiceQuery.getEntry());
        if (mRealNet != null) {
            Message mGetRealNet = mCUCCHandler.obtainMessage(GET_NET_STATUS);
            mCUCCHandler.sendMessage(mGetRealNet);
        }
        super.onStart();
    }

    @Override
    protected void onResume() {
        if (isCardExist() && isAirplaneModeOff()) {
//            mAPNSetting.setEnabled(true);
//            mAPNSetting.setEnabled(true);
            for (int i = 0; i < mSubscriptionManager.getActiveSubscriptionIdList().length; i++) {
                if (!isDefaultAPNExist(mSubscriptionManager.getActiveSubscriptionIdList()[i], DEFAULT_APN)) {
                    fillDB(mSubscriptionManager.getActiveSubscriptionIdList()[i]);
                    //isDefaultAPNExist(TelephonyManager.getActiveSubIdList()[i], DEFAULT_APN);
                }
            }
            Log.d(TAG, "mPhoneSimCount is " + mPhoneSimCount + " mIsCardExist is " + mIsCardExist);
        } else {
//            mAPNSetting.setEnabled(false);
//            mAPNSetting.setSummary(R.string.set_ready);
            Log.d(TAG, "mPhoneSimCount is " + mPhoneSimCount + " mIsCardExist is " + mIsCardExist);
        }
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        if (mCUCCHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mCUCCHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        // TODO Auto-generated method stub
        finish();
        super.onBackPressed();
    }

    class CUCCHandler extends Handler {

        public CUCCHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String responValue;
            switch (msg.what) {
                case SET_REAL_NET_OPEN: {
                    mATCmd = engconstents.ENG_AT_SET_SPTEST + "2,1";
                    responValue = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    Log.d(TAG, "SET_REAL_NET_OPEN is " + responValue + ", mATCmd is " + mATCmd);
                    if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mRealNet.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mRealNet.setChecked(false);
                            }
                        });
                    }
                    break;
                }
                case SET_REAL_NET_CLOSE: {
                    mATCmd = engconstents.ENG_AT_SET_SPTEST + "2,0";
                    responValue = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    Log.d(TAG, "SET_REAL_NET_CLOSE is " + responValue + ", mATCmd is " + mATCmd);
                    if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mRealNet.setChecked(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mRealNet.setChecked(true);
                            }
                        });
                    }
                    break;
                }
                case GET_NET_STATUS: {
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_SET_SPTEST1, "atchannel0");
                    Log.d(TAG, "GET_NET_STATUS is " + responValue);
                    if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                        String[] str = responValue.split("\\+");
                        String[] str1 = str[2].split("\\:");
                        String[] str2 = str1[1].split("\\,");
                        mATResponse = str2[1].trim();
                        if (mATResponse.equals("1")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mRealNet.setChecked(true);
                                }
                            });
                        } else {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mRealNet.setChecked(false);
                                }
                            });
                        }
                    }
                    break;
                }
                case SET_CFU: {
                    String valueStr = (String) msg.obj;
                    SystemProperties.set(CFU_CONTROL, valueStr);
                    break;
                }
                default:
                    break;
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object newValue) {
        if (pref == mRealNet) {
            int messageType;
            if (!mRealNet.isChecked()) {
                messageType = SET_REAL_NET_OPEN;
            } else {
                messageType = SET_REAL_NET_CLOSE;
            }
            Message mSetRealNet = mCUCCHandler.obtainMessage(messageType);
            mCUCCHandler.sendMessage(mSetRealNet);
        }
        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference pref) {
        if (pref == mAPNSetting) {
            if (mPhoneSimCount == 1) {
                Intent intent = new Intent("android.intent.action.APNSETTING");
                intent.putExtra(KEY_SELECT_SIM, 0);
                startActivity(intent);
            } else {
                Intent intent = new Intent("android.intent.action.SIMSELECT_APN");
                startActivity(intent);
            }
        }
        return true;
    }

    private boolean isCardExist() {
        TelephonyManager telephonyManager[] = new TelephonyManager[mPhoneSimCount];
        for (int i = 0; i < mPhoneSimCount; i++) {
            telephonyManager[i] = TelephonyManager.from(CUCCActivity.this);
            if (telephonyManager[i] != null
                    && telephonyManager[i].getSimState(i) == TelephonyManager.SIM_STATE_READY) {
                mIsCardExist = true;
                break;
            }
        }
        return mIsCardExist;
    }

    private boolean isAirplaneModeOff() {
        return Settings.System.getInt(mContext.getContentResolver(),
                Settings.System.AIRPLANE_MODE_ON, 0) == 0;
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
            String key) {
        if (key.equals(KEY_SUPPLSERVICEQUERY)) {
            mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery.getEntry());
            mSupplementaryServiceQuery1.setSummary(mSupplementaryServiceQuery.getEntry());
            mSupplementaryServiceQuery1.setValue(mSupplementaryServiceQuery.getValue());
            Log.d(TAG, mSupplementaryServiceQuery.getValue());
            String re = sharedPreferences.getString(key, "");
            Message mSupplService = mCUCCHandler.obtainMessage(SET_CFU, re);
            mCUCCHandler.sendMessage(mSupplService);
        }
        if (key.equals(KEY_SUPPLSERVICEQUERY1)) {
            mSupplementaryServiceQuery1.setSummary(mSupplementaryServiceQuery1.getEntry());
            mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery1.getEntry());
            mSupplementaryServiceQuery.setValue(mSupplementaryServiceQuery1.getValue());
            Log.d(TAG, mSupplementaryServiceQuery1.getValue());
            String re = sharedPreferences.getString(key, "");
            Message mSupplService = mCUCCHandler.obtainMessage(SET_CFU, re);
            mCUCCHandler.sendMessage(mSupplService);
        }
    }

    private boolean isDefaultAPNExist(int subId, String defaultApn) {
        boolean isDefaultAPNExist = false;
        String where;
        Uri contentUri = Telephony.Carriers.CONTENT_URI;
        if (TelephonyManager.from(CUCCActivity.this).isMultiSimEnabled()) {
            String operator = getOperatorNumeric(subId);
            where = "numeric=\""+ operator + "\"";
            where += " and sub_id = \'"+subId +"\'";
            Log.d(TAG, "if multi sim, where = " + where);
        } else {
            where = "numeric=\""
                    + android.os.SystemProperties.get(
                            TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC, "")
                    + "\"";
        }
        where += " and name!='CMCC DM'";

        Cursor cursor = getContentResolver().query(contentUri, new String[] {
                "_id", "name", "apn", "type"
        }, where, null,
                Telephony.Carriers.DEFAULT_SORT_ORDER);
        if (cursor != null) {
            Log.d(TAG, "cursor count = " + cursor.getCount());
            cursor.moveToFirst();
            while (!cursor.isAfterLast()) {
                String apn = cursor.getString(APN_INDEX);
                if (apn.equals(DEFAULT_APN)) {
                    isDefaultAPNExist = true;
                    break;
                }
                cursor.moveToNext();
            }
            cursor.close();
        }
        return isDefaultAPNExist;
    }

    private void fillDB(int subId) {
        String numeric;
        int mDDS = 0;
        String mcc = "";
        String mnc = "";
        Uri uri;
        uri = Telephony.Carriers.CONTENT_URI;
       // uri = getContentResolver().insert(Telephony.Carriers.CONTENT_URI, new ContentValues());
        ContentValues values = new ContentValues();
        if (TelephonyManager.from(CUCCActivity.this).isMultiSimEnabled()) {
            // APNs are automatically added with MCC+MNC.
            // Get the value from appropriate Telephony Property based on the
            // mPhoneId.
            numeric = getOperatorNumeric(subId);
        } else {
            numeric =
                    SystemProperties.get(TelephonyProperties.PROPERTY_ICC_OPERATOR_NUMERIC);
        }
        Log.d(TAG, "numeric = " + numeric);
        // MCC is first 3 chars and then in 2 - 3 chars of MNC
        if (numeric != null && numeric.length() > 4) {
            // Country code
            mcc = numeric.substring(0, 3);
            // Network code
            mnc = numeric.substring(3);
        }

        values.put(Telephony.Carriers.NAME, this.getString(R.string.default_set).toString());
        values.put(Telephony.Carriers.APN, DEFAULT_APN);
        values.put(Telephony.Carriers.PROXY, "");
        values.put(Telephony.Carriers.PORT, "");
        values.put(Telephony.Carriers.MMSPROXY, "");
        values.put(Telephony.Carriers.MMSPORT, "");
        values.put(Telephony.Carriers.USER, "");
        values.put(Telephony.Carriers.SERVER, "");
        values.put(Telephony.Carriers.PASSWORD, "");
        values.put(Telephony.Carriers.MMSC, "");
        values.put(Telephony.Carriers.PROTOCOL, "");
        values.put(Telephony.Carriers.ROAMING_PROTOCOL, "");
        values.put(Telephony.Carriers.TYPE, "*");
        values.put(Telephony.Carriers.MCC, mcc);
        values.put(Telephony.Carriers.MNC, mnc);
        values.put(Telephony.Carriers.NUMERIC, mcc + mnc);
        values.put(Telephony.Carriers.CURRENT, 1);
        //values.put(Telephony.Carriers.SUB_ID, subId);
        Log.d(TAG, "subId = " + subId);

        try {
            mDDS = Settings.System
                    .getInt(getContentResolver(), Settings.Global.MULTI_SIM_DATA_CALL_SUBSCRIPTION);
        } catch (Exception e) {
            Log.e(TAG, "Exception Reading Dual Sim Data Subscription Value.", e);
        }
        //getContentResolver().update(uri, values, null, null);
        getContentResolver().insert(uri, values);
    }
    /* SPRD:get mcc+mnc from subinfo */
    private String getOperatorNumeric(int subId) {
        String mcc = null;
        String mnc = null;
        SubscriptionInfo subInfoRecord = mSubscriptionManager.getActiveSubscriptionInfo(subId);
        if (subInfoRecord != null) {
            mcc = String.valueOf(subInfoRecord.getMcc());
            if (subInfoRecord.getMnc() < 10) {
                mnc = "0" + String.valueOf(subInfoRecord.getMnc());
            } else {
                mnc = String.valueOf(subInfoRecord.getMnc());
            }
        }
        return mcc + mnc;
    }
}
