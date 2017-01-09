
package com.sprd.engineermode.debuglog;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.preference.SwitchPreference;
import android.preference.TwoStatePreference;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.R.xml;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;

public class CTAActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener, OnSharedPreferenceChangeListener {

    private static final String TAG = "CTAActivity";
    private static final String KEY_DRIVERSETTING_IT3 = "driversetting";
    private static final String KEY_SUPPLSERVICEQUERY = "supplementary_service_query";

    private static final String CFU_CONTROL = "persist.sys.callforwarding";

    private static final int SET_CFU = 1;
    private static final int SET_USIMDRVLS_OPEN = 2;
    private static final int SET_USIMDRVLS_CLOSE = 3;
    private static final int GET_USIMDRVLS = 4;

    private TwoStatePreference mDriverSettingIt3;
    private ListPreference mSupplementaryServiceQuery;

    private Context mContext;
    private Handler mUiThread = new Handler();
    private CTAHandler mCTAHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = this;
        addPreferencesFromResource(R.xml.pref_dft_cta_it3);
        mDriverSettingIt3 = (TwoStatePreference) findPreference(KEY_DRIVERSETTING_IT3);
        mDriverSettingIt3.setOnPreferenceChangeListener(this);
        mSupplementaryServiceQuery = (ListPreference) findPreference(KEY_SUPPLSERVICEQUERY);
        SharedPreferences sharePref = PreferenceManager.getDefaultSharedPreferences(this);
        sharePref.registerOnSharedPreferenceChangeListener(this);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mCTAHandler = new CTAHandler(ht.getLooper());
    }

    @Override
    protected void onStart() {
        mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery.getEntry());
        if (mDriverSettingIt3 != null) {
            Message mDriverMessage = mCTAHandler.obtainMessage(GET_USIMDRVLS);
            mCTAHandler.sendMessage(mDriverMessage);
        }
        super.onStart();
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        if (mCTAHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mCTAHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        // TODO Auto-generated method stub
        finish();
        super.onBackPressed();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (preference == mDriverSettingIt3) {
            if (!mDriverSettingIt3.isChecked()) {
                Message mDriverMessage = mCTAHandler.obtainMessage(SET_USIMDRVLS_OPEN);
                mCTAHandler.sendMessage(mDriverMessage);
            } else {
                Message mDriverMessage = mCTAHandler.obtainMessage(SET_USIMDRVLS_CLOSE);
                mCTAHandler.sendMessage(mDriverMessage);
            }
        }
        return true;
    }

    class CTAHandler extends Handler {

        public CTAHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case SET_CFU: {
                    String valueStr = (String) msg.obj;
                    SystemProperties.set(CFU_CONTROL, valueStr);
                    break;
                }
                case SET_USIMDRVLS_OPEN: {
                    String responValue;
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_USIMDRIVERLOG + "1",
                            "atchannel0");
                    Log.d(TAG, "SET_USIMDRVLS_OPEN is " + responValue);
                    if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDriverSettingIt3.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDriverSettingIt3.setChecked(false);
                            }
                        });
                    }
                    break;
                }
                case SET_USIMDRVLS_CLOSE: {
                    String responValue;
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_USIMDRIVERLOG + "0",
                            "atchannel0");
                    Log.d(TAG, "SET_USIMDRVLS_CLOSE is " + responValue);
                    if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDriverSettingIt3.setChecked(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDriverSettingIt3.setChecked(true);
                            }
                        });
                    }
                    break;
                }
                case GET_USIMDRVLS: {
                    String responValue;
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_USIMDRIVERLOG1,
                            "atchannel0");
                    Log.d(TAG, "GET_USIMDRVLS is " + responValue);
                    if (responValue != null && responValue.contains("1")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDriverSettingIt3.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDriverSettingIt3.setChecked(false);
                            }
                        });
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
            String key) {
        if (key.equals(KEY_SUPPLSERVICEQUERY)) {
            mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery.getEntry());
            String re = sharedPreferences.getString(key, "");
            Message mSupplService = mCTAHandler.obtainMessage(SET_CFU, re);
            mCTAHandler.sendMessage(mSupplService);
        }
    }
}
