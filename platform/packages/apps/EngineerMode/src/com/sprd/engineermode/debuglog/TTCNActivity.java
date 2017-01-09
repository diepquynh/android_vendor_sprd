
package com.sprd.engineermode.debuglog;

import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.preference.TwoStatePreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceManager;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.util.Log;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashSet;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import com.android.internal.telephony.SMSDispatcher;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;

public class TTCNActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener, OnSharedPreferenceChangeListener {
    private static final String TAG = "TTCNActivity";
    private static final String KEY_INTEGRITY_PROTECTION = "integrity_protection";
    private static final String KEY_MESSAGE_RETRANSMISSION = "message_retransmission";
    private static final String KEY_SUPPLSERVICEQUERY = "supplementary_service_query";

    private static final String RETRY_SMS_CONTROL = "persist.sys.msms_retry_control";
    private static final String CFU_CONTROL = "persist.sys.callforwarding";

    private static final int GET_INTEGRITYPROTECTION = 1;
    private static final int SET_INTEGRITYPROTECTION = 2;
    private static final int SET_CFU = 3;

    private Context mContext;

    private Handler mUiThread = new Handler();

    private TwoStatePreference mIntegrityProtectionSwitchPreference;
    private TwoStatePreference mSmsRetryControl;
    private ListPreference mSupplementaryServiceQuery;
    private TTCNHandler mTTCNHandler;

    private String mStrTmp;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mTTCNHandler = new TTCNHandler(ht.getLooper());

        addPreferencesFromResource(R.xml.pref_ttcn);
        mIntegrityProtectionSwitchPreference = (TwoStatePreference) findPreference(KEY_INTEGRITY_PROTECTION);
        mIntegrityProtectionSwitchPreference.setOnPreferenceChangeListener(this);
        mSmsRetryControl = (TwoStatePreference) findPreference(KEY_MESSAGE_RETRANSMISSION);
        mSmsRetryControl.setOnPreferenceChangeListener(this);
        mSupplementaryServiceQuery = (ListPreference) findPreference(KEY_SUPPLSERVICEQUERY);

        mContext = this;
        SharedPreferences sharePref = PreferenceManager.getDefaultSharedPreferences(this);
        sharePref.registerOnSharedPreferenceChangeListener(this);
    }

    @Override
    protected void onStart() {
        mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery.getEntry());

        String smsControl = SystemProperties.get(RETRY_SMS_CONTROL);
        if (smsControl.equals("0")) {
            mSmsRetryControl.setChecked(false);
        } else if (smsControl.equals("3")) {
            mSmsRetryControl.setChecked(true);
        }
        if (mIntegrityProtectionSwitchPreference != null) {
            Message mIntegrityProtection = mTTCNHandler.obtainMessage(GET_INTEGRITYPROTECTION);
            mTTCNHandler.sendMessage(mIntegrityProtection);
        }
        super.onStart();
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        if (mTTCNHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mTTCNHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    class TTCNHandler extends Handler {

        public TTCNHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String atResponse;
            String responValue;
            switch (msg.what) {
                case GET_INTEGRITYPROTECTION: {
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_AT_SET_SPTEST1, "atchannel0");
                    responValue = analysisResponse(mStrTmp,
                            TTCNActivity.GET_INTEGRITYPROTECTION);
                    if (responValue.equals("1")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIntegrityProtectionSwitchPreference.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIntegrityProtectionSwitchPreference.setChecked(false);
                            }
                        });
                    }
                    break;
                }
                case SET_INTEGRITYPROTECTION: {
                    String valueStr = (String) msg.obj;
                    if (valueStr.equals("1,1")) {
                        mStrTmp = IATUtils.sendATCmd(engconstents.ENG_AT_SET_SPTEST + valueStr,
                                "atchannel0");
                        responValue = analysisResponse(mStrTmp,
                                TTCNActivity.SET_INTEGRITYPROTECTION);
                        if (responValue.equals(IATUtils.AT_OK)) {
                            mUiThread.post(new TTCNRunnable(mIntegrityProtectionSwitchPreference,
                                    true, true));
                        } else {
                            mUiThread.post(new TTCNRunnable(mIntegrityProtectionSwitchPreference,
                                    false, false));
                        }
                    } else {
                        mStrTmp = IATUtils.sendATCmd(engconstents.ENG_AT_SET_SPTEST + valueStr,
                                "atchannel0");
                        responValue = analysisResponse(mStrTmp,
                                TTCNActivity.SET_INTEGRITYPROTECTION);
                        if (responValue.equals(IATUtils.AT_OK)) {
                            mUiThread.post(new TTCNRunnable(mIntegrityProtectionSwitchPreference,
                                    false, true));
                        } else {
                            mUiThread.post(new TTCNRunnable(mIntegrityProtectionSwitchPreference,
                                    true, false));
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

    private String analysisResponse(String response, int type) {
        if (response != null && response.contains(IATUtils.AT_OK)) {
            if (type == TTCNActivity.GET_INTEGRITYPROTECTION) {
                String[] str = response.split("\\+");
                String[] str1 = str[1].split("\\:");
                String[] str2 = str1[1].split("\\,");
                Log.d(TAG, "IntegrityProtection is " + str2[1]);
                return str2[1];
            } else if (type == TTCNActivity.SET_INTEGRITYPROTECTION) {
                return IATUtils.AT_OK;
            }
        }
        return IATUtils.AT_FAIL;

    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object newValue) {
        if (pref == mSmsRetryControl) {
            if (!mSmsRetryControl.isChecked()) {
                SystemProperties.set(RETRY_SMS_CONTROL, "3");
                mSmsRetryControl.setChecked(true);
                Log.d(TAG, "MmsRetry enable");
            } else {
                SystemProperties.set(RETRY_SMS_CONTROL, "0");
                mSmsRetryControl.setChecked(false);
                Log.d(TAG, "MmsRetry unable");
            }
        } else if (pref == mIntegrityProtectionSwitchPreference) {
            String set;
            if (!mIntegrityProtectionSwitchPreference.isChecked()) {
                set = "1,1";
            } else {
                set = "1,0";
            }
            Message mIntegrityProtection = mTTCNHandler.obtainMessage(SET_INTEGRITYPROTECTION, set);
            mTTCNHandler.sendMessage(mIntegrityProtection);
        }
        return true;
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
            String key) {
        if (key.equals(KEY_SUPPLSERVICEQUERY)) {
            mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery.getEntry());
            String re = sharedPreferences.getString(key, "");
            Message mSupplService = mTTCNHandler.obtainMessage(SET_CFU, re);
            mTTCNHandler.sendMessage(mSupplService);
        }
    }

    class TTCNRunnable implements Runnable {
        private TwoStatePreference pref;
        private boolean value;
        private boolean result;

        public TTCNRunnable(TwoStatePreference preference, boolean value, boolean result) {
            this.pref = preference;
            this.value = value;
            this.result = result;
        }

        @Override
        public void run() {
            pref.setChecked(value);
            if (!result) {
                pref.setEnabled(false);
                pref.setSummary(R.string.feature_abnormal);
            }
        }
    }
}
