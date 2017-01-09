
package com.sprd.engineermode.telephony;

import android.os.Handler;
import android.os.Message;
import android.preference.Preference;
import android.preference.ListPreference;
import android.preference.PreferenceScreen;
import android.preference.PreferenceManager;
import android.preference.PreferenceActivity;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.widget.Toast;
import android.os.Looper;
import android.os.Bundle;
import android.os.HandlerThread;
import android.util.Log;

import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.IATUtils;

public class VamosActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener, OnSharedPreferenceChangeListener {
    private static final String TAG = "VamosActivity";
    private static final String KEY_DEVICE_CAPABILITY_STATUS = "device_capability";
    private static final String KEY_CAPABILITY_REPORT = "capability_report";
    private static final String KEY_WORKING_STATUS = "working_status";
    private static final String CAPABILITY_REPORT_STATUS = "CAPABILITY_REPORT_STATUS";
    private static final int IS_SUPPORT_VAMOS = 0;
    private static final int SET_STATUS = 1;
    private static final int SET_CAPABILITY_REPORT = 2;

    private Preference mSettingStatus;
    private ListPreference mWorkingStatus;
    private Preference mVamosStatus;
    private SharedPreferences mSharePref;

    private Handler mUiThread = new Handler();
    private VamosHandler mVamosHandler;
    private Editor mEditor;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mVamosHandler = new VamosHandler(ht.getLooper());
        addPreferencesFromResource(R.xml.pref_vamos);

        mSettingStatus = findPreference(KEY_DEVICE_CAPABILITY_STATUS);
        mWorkingStatus = (ListPreference) findPreference(KEY_CAPABILITY_REPORT);
        mVamosStatus = findPreference(KEY_WORKING_STATUS);
        SharedPreferences mPref = PreferenceManager.getDefaultSharedPreferences(this);
        mEditor = mPref.edit();
        mSharePref = PreferenceManager.getDefaultSharedPreferences(this);
        mSharePref.registerOnSharedPreferenceChangeListener(this);

    }

    @Override
    public void onStart() {
        if (mSettingStatus != null && mSettingStatus.isEnabled()) {
            Message isSupportVamosService = mVamosHandler.obtainMessage(IS_SUPPORT_VAMOS);
            mVamosHandler.sendMessage(isSupportVamosService);
        }
        super.onStart();
    }

    @Override
    protected void onDestroy() {
        if (mVamosHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mVamosHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object newValue) {
        return false;
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
            String key) {
        if (key.equals(KEY_CAPABILITY_REPORT)) {
            String re = sharedPreferences.getString(key, "");
            Message setStateService = mVamosHandler.obtainMessage(SET_CAPABILITY_REPORT, re);
            mVamosHandler.sendMessage(setStateService);
        }
    }

    class VamosHandler extends Handler {
        public VamosHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String atResponse = null;
            String atCmd = null;
            switch (msg.what) {
                case IS_SUPPORT_VAMOS: {
                    atCmd = "AT+SPENGMD=0,0,7";
                    atResponse = IATUtils.sendATCmd(atCmd, "atchannel0");
                    if (atResponse != null && atResponse.contains(IATUtils.AT_OK)) {
                        final String[] stateStr = atResponse.split("-");
                        if (stateStr != null && stateStr.length > 7 && stateStr[7].contains(",")) {
                            final String[] temp = stateStr[7].split(",");
                            if (temp != null && temp.length >= 2) {
                                mUiThread.post(new Runnable() {
                                    @Override
                                    public void run() {
                                        if (temp[0].equals("1")) {
                                            mSettingStatus.setEnabled(true);
                                            mWorkingStatus.setEnabled(true);
                                            mSettingStatus.setSummary(R.string.vamos_setting_status_summery_1);
                                            updateWorkingState(mSharePref.getString(CAPABILITY_REPORT_STATUS, temp[1]));
                                            if (temp[1].equals("1")) {
                                                mVamosStatus.setSummary(R.string.vamos_working_status_yes);
                                            } else {
                                                mVamosStatus.setSummary(R.string.vamos_working_status_no);
                                            }
                                        } else {
                                            mSettingStatus.setEnabled(false);
                                            mWorkingStatus.setEnabled(false);
                                            mVamosStatus.setEnabled(false);
                                            mSettingStatus.setSummary(R.string.vamos_setting_status_summery_2);
                                        }
                                    }
                                });
                            }
                        } else {
                            Log.d(TAG, "IS_SUPPORT_VAMOS  Parse error");
                        }
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Log.d(TAG, "IS_SUPPORT_VAMOS  ATCmd send fail");
                                mSettingStatus.setEnabled(false);
                            }
                        });
                    }
                    break;
                }
                case SET_CAPABILITY_REPORT: {
                    final String valueStr = (String) msg.obj;
                    atCmd = "AT+SPENGMD=1,0,7," + valueStr;
                    Log.d(TAG, "SET_STATUS atCmd = " + atCmd);
                    atResponse = IATUtils.sendATCmd(atCmd, "atchannel0");
                    Log.d(TAG, "SET_STATUS responValue = " + atResponse);
                    if (atResponse != null && atResponse.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mEditor.putString(CAPABILITY_REPORT_STATUS, valueStr);
                                mEditor.commit();
                                updateWorkingState(valueStr);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(VamosActivity.this, "Set Fail", Toast.LENGTH_SHORT)
                                        .show();
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

    private void updateWorkingState(String valueStr) {
        if (valueStr.equals("0")) {
            mWorkingStatus.setSummary(R.string.vamos_working_status_summery_2);
        } else if (valueStr.equals("1")) {
            mWorkingStatus.setSummary(R.string.vamos_working_status_summery_1);
        }
    }
}
