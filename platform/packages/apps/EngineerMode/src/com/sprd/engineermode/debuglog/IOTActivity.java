
package com.sprd.engineermode.debuglog;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.preference.TwoStatePreference;
import android.util.Log;
import android.widget.Toast;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.R.xml;
import com.sprd.engineermode.utils.IATUtils;

public class IOTActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener, OnSharedPreferenceChangeListener {

    private static final String TAG = "IOTActivity";

    private static final String KEY_AOCSETTING_IOT = "aocsetting_iot";
    private static final String KEY_FBSETTING_IOT = "fbsetting_iot";
    private static final String KEY_SUPPLSERVICEQUERY = "supplementary_service_query";
    private static final String KEY_PROTOCALSETTING = "cta_iot_protocal";
    private static final String KEY_FBANDSETTING = "cta_iot_fband";
    private static final String KEY_UPLMNCON = "uplmn_control";
    private static final String KEY_BIN_ENABLED = "bih_enabled_key";

    private static final String CFU_CONTROL = "persist.sys.callforwarding";

    private static final int FAT = 0;
    private static final int INTEPROTECTION = 1;
    private static final int NET_STATUS = 2;
    private static final int AOC = 3;
    private static final int GSM_SUPPORT = 4;
    private static final int PROTOCALNUM = 5;
    private static final int PS_DATA = 6;
    private static final int UE_TYPE = 7;
    private static final int COMMUNITY_SUPPORT = 8;
    private static final int DSP_CTA = 9;
    private static final int DSP_LOG = 10;
    private static final int FBAND = 11;

    private static final int GET_IOT_STATUS = 1;
    private static final int SET_AOC_OPEN = 2;
    private static final int SET_AOC_CLOSE = 3;
    private static final int SET_CFU = 4;
    private static final int SET_PROTOCAL = 5;
    private static final int SET_FBAND = 6;

    private String mATCmd;
    private String mProtocalNum;
    private String mFbandNum;

    private Handler mUiThread = new Handler();
    private IOTHandler mIOTHandler;

    private TwoStatePreference mAocsettingIot;
    private TwoStatePreference mUplmnCon;
    private ListPreference mSupplementaryServiceQuery;
    private ListPreference mProtocalSetting;
    private ListPreference mFbandSetting;
    private Preference mFbsettingIot;
    private Preference mBIHEnabled;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_dft_cta_iot);
        mAocsettingIot = (TwoStatePreference) findPreference(KEY_AOCSETTING_IOT);
        mAocsettingIot.setOnPreferenceChangeListener(this);
        mFbsettingIot = findPreference(KEY_FBSETTING_IOT);
        mSupplementaryServiceQuery = (ListPreference) findPreference(KEY_SUPPLSERVICEQUERY);
        mProtocalSetting = (ListPreference) findPreference(KEY_PROTOCALSETTING);
        mFbandSetting = (ListPreference) findPreference(KEY_FBANDSETTING);
        mBIHEnabled = findPreference(KEY_BIN_ENABLED);
        mBIHEnabled.setEnabled(false);
        mBIHEnabled.setSummary(R.string.feature_not_support);
        if (TelephonyManagerSprd.getModemType() != TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
            mFbandSetting.setEnabled(false);
            mFbandSetting.setSummary(R.string.feature_not_support);
            mFbandSetting = null;
        }
        mUplmnCon = (TwoStatePreference) findPreference(KEY_UPLMNCON);
        mUplmnCon.setOnPreferenceChangeListener(this);
        SharedPreferences sharePref = PreferenceManager.getDefaultSharedPreferences(this);
        sharePref.registerOnSharedPreferenceChangeListener(this);

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mIOTHandler = new IOTHandler(ht.getLooper());
    }

    @Override
    protected void onStart() {
        mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery.getEntry());
        Message mParamStatus = mIOTHandler.obtainMessage(GET_IOT_STATUS);
        mIOTHandler.sendMessage(mParamStatus);
        if (mUplmnCon != null && mUplmnCon.isEnabled()) {
            mUplmnCon.setChecked(SystemProperties.getBoolean("persist.sys.uplmn", false));
        }
        super.onStart();
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        if (mIOTHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mIOTHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    class IOTHandler extends Handler {

        public IOTHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String responValue;
            String aocStatus;
            switch (msg.what) {
                case GET_IOT_STATUS: {
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_SET_SPTEST1, "atchannel0");
                    Log.d(TAG, "GET_IOT_STATUS is " + responValue);
                    if(responValue == null || responValue.contains("fail") || responValue.contains("ERROR")) {
                        break;
                    }
                    aocStatus = analysisResponse(responValue, GET_IOT_STATUS, AOC);
                    mProtocalNum = analysisResponse(responValue, GET_IOT_STATUS, PROTOCALNUM);
                    if (mFbandSetting != null) {
                        mFbandNum = analysisResponse(responValue, GET_IOT_STATUS, FBAND);
                    }
                    Log.d(TAG, "AOC Status is " + aocStatus + ", PROTOCALNUM is " + mProtocalNum
                            + ", FBANDNUM is " + mFbandNum);
                    if (aocStatus.equals("1")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAocsettingIot.setChecked(true);
                                mProtocalSetting.setValue(mProtocalNum);
                                mProtocalSetting.setSummary(mProtocalSetting.getEntry());
                                if (mFbandSetting != null) {
                                    mFbandSetting.setValue(mFbandNum);
                                    mFbandSetting.setSummary(mFbandSetting.getEntry());
                                }
                            }
                        });
                    } else if (aocStatus.equals("0")) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAocsettingIot.setChecked(false);
                                mProtocalSetting.setValue(mProtocalNum);
                                mProtocalSetting.setSummary(mProtocalSetting.getEntry());
                                if (mFbandSetting != null) {
                                    mFbandSetting.setValue(mFbandNum);
                                    mFbandSetting.setSummary(mFbandSetting.getEntry());
                                }
                            }
                        });
                    } else {
                        mProtocalSetting.setSummary(mProtocalSetting.getEntry());
                        if (mFbandSetting != null) {
                            mFbandSetting.setSummary(mFbandSetting.getEntry());
                        }
                    }
                    break;
                }
                case SET_AOC_OPEN: {
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_SET_SPTEST + "3,1",
                            "atchannel0");
                    Log.d(TAG, "SET_AOC_OPEN is " + responValue);
                    if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAocsettingIot.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAocsettingIot.setChecked(false);
                            }
                        });
                    }
                    break;
                }
                case SET_AOC_CLOSE: {
                    responValue = IATUtils.sendATCmd(engconstents.ENG_AT_SET_SPTEST + "3,0",
                            "atchannel0");
                    Log.d(TAG, "SET_AOC_CLOSE is " + responValue);
                    if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAocsettingIot.setChecked(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mAocsettingIot.setChecked(true);
                            }
                        });
                    }
                    break;
                }
                case SET_CFU: {
                    String valueStr = (String) msg.obj;
                    SystemProperties.set(CFU_CONTROL, valueStr);
                    break;
                }
                case SET_PROTOCAL: {
                    String valueStr = (String) msg.obj;
                    mATCmd = engconstents.ENG_AT_SET_SPTEST + "5," + valueStr;
                    responValue = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    Log.d(TAG, "SET_PROTOCAL is " + responValue + ", cmd is " + mATCmd);
                    if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mProtocalSetting.setSummary(mProtocalSetting.getEntry());
                                mProtocalNum = mProtocalSetting.getValue();
                            }
                        });
                    } else {
                        //bug 611583
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mProtocalSetting.setValue(mProtocalNum);
                                mProtocalSetting.setSummary(mProtocalSetting.getEntry());
                            }
                        });
                    }
                    break;
                }
                case SET_FBAND: {
                    String valueStr = (String) msg.obj;
                    mATCmd = engconstents.ENG_AT_SET_SPTEST + "11," + valueStr;
                    responValue = IATUtils.sendATCmd(mATCmd, "atchannel0");
                    Log.d(TAG, "SET_FBAND is " + responValue + ", cmd is " + mATCmd);
                    if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                if (mFbandSetting != null) {
                                    mFbandSetting.setSummary(mFbandSetting.getEntry());
                                    mFbandNum = mFbandSetting.getValue();
                                }
                            }
                        });
                    } else {
                        mFbandSetting.setValue(mFbandNum);
                        mFbandSetting.setSummary(mFbandSetting.getEntry());
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    private String analysisResponse(String response, int messType, int getType) {
        if (response != null && response.contains(IATUtils.AT_OK)) {
            if (messType == GET_IOT_STATUS) {
                String[] str = response.split("\\+");
                if (getType == AOC) {
                    String[] str1 = str[3].split("\\:");
                    String[] str2 = str1[1].split("\\,");
                    return str2[1].trim();
                } else if (getType == PROTOCALNUM) {
                    String[] str1 = str[5].split("\\:");
                    String[] str2 = str1[1].split("\\,");
                    return str2[1].trim();
                } else if (getType == FBAND) {
                    String[] str1 = str[11].split("\\:");
                    String[] str2 = str1[1].split("\n");
                    String[] str3 = str2[0].split("\\,");
                    return str3[1].trim();
                }
            } else {
                return IATUtils.AT_OK;
            }
        }
        return IATUtils.AT_FAIL;
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
            String key) {
        int messageType = 0;
        if (key.equals(KEY_SUPPLSERVICEQUERY)) {
            mSupplementaryServiceQuery.setSummary(mSupplementaryServiceQuery.getEntry());
            messageType = SET_CFU;
            String re = sharedPreferences.getString(key, "");
            Message mParamSet = mIOTHandler.obtainMessage(messageType, re);
            mIOTHandler.sendMessage(mParamSet);
        }
        if (key.equals(KEY_PROTOCALSETTING)) {
            messageType = SET_PROTOCAL;
            String re = sharedPreferences.getString(key, "");
            Message mParamSet = mIOTHandler.obtainMessage(messageType, re);
            mIOTHandler.sendMessage(mParamSet);
        }
        if (key.equals(KEY_FBANDSETTING)) {
            messageType = SET_FBAND;
            String re = sharedPreferences.getString(key, "");
            Message mParamSet = mIOTHandler.obtainMessage(messageType, re);
            mIOTHandler.sendMessage(mParamSet);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object newValue) {
        if (pref == mAocsettingIot) {
            int messageType;
            if (!mAocsettingIot.isChecked()) {
                messageType = SET_AOC_OPEN;
            } else {
                messageType = SET_AOC_CLOSE;
            }
            Message mAOTSetting = mIOTHandler.obtainMessage(messageType);
            mIOTHandler.sendMessage(mAOTSetting);
        } else if (pref == mUplmnCon) {
            if (!mUplmnCon.isChecked()) {
                SystemProperties.set("persist.sys.uplmn", "1");
            } else {
                SystemProperties.set("persist.sys.uplmn", "0");
            }
        }
        return true;
    } 
}
