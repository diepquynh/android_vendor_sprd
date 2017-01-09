
package com.sprd.engineermode.debuglog;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.utils.SocketUtils;

import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.TwoStatePreference;
import android.util.Log;
import android.widget.Toast;
import android.content.Context;
import android.os.PowerManager;
import com.sprd.engineermode.debuglog.slogui.SlogAction;
import com.sprd.engineermode.utils.SocketUtils;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;

public class WCDMAIQActivity extends PreferenceActivity implements
        OnSharedPreferenceChangeListener, OnPreferenceChangeListener {

    private static final String TAG = "WCDMAIQActivityActivity";

    private static final String KEY_WIQ_SWITCH = "wiq_switch";
    private static final String KEY_FUNCTION_ITEM = "function_item";
    private static final String KEY_IQ_SAMPLE_POS = "iq_sample_pos";

    private static final int GET_WIQ_SWITCH = 1;
    private static final int SET_WIQ_SWITCH_OPEN = 2;
    private static final int SET_WIQ_SWITCH_CLOSE = 3;
    private static final int GET_FUNCTION_ITEM = 4;
    private static final int SET_FUNCTION_ITEM = 5;
    private static final int GET_IQ_SAMPLE_POS = 6;
    private static final int SET_IQ_SAMPLE_POS = 7;

    private TwoStatePreference mWiqSwitchPref;
    private ListPreference mFunctionItemPref;
    private ListPreference mIqSamplePosPref;

    private String functionItemValue = null;
    private String iqSamplePosValue = null;
    private String mSupportMode = null;

    private mWcdmaIqHandler mWcdmaIqHandler;
    private Handler uiThread = new Handler();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        this.addPreferencesFromResource(R.xml.pref_wcdma_iq);

        mWiqSwitchPref = (TwoStatePreference) this
                .findPreference(KEY_WIQ_SWITCH);
        mFunctionItemPref = (ListPreference) this
                .findPreference(KEY_FUNCTION_ITEM);
        mIqSamplePosPref = (ListPreference) this
                .findPreference(KEY_IQ_SAMPLE_POS);

        mWiqSwitchPref.setOnPreferenceChangeListener(this);
        if (SlogAction.CP0_ENABLE) {
            mSupportMode = "WCDMA";
        }  else if (SlogAction.CP4_ENABLE) {
            mSupportMode = "FDD-LTE";
        }  else if (SlogAction.CP5_ENABLE) {
            mSupportMode = "5MODE";
        }
        if (mSupportMode == null) {
            mWiqSwitchPref.setEnabled(false);
        }
        HandlerThread ht = new HandlerThread("wcdmaIqActivity");
        ht.start();
        mWcdmaIqHandler = new mWcdmaIqHandler(ht.getLooper());
        SharedPreferences sharPref = PreferenceManager.getDefaultSharedPreferences(this);
        sharPref.registerOnSharedPreferenceChangeListener(this);
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        finish();
        super.onBackPressed();
    }

    @Override
    protected void onStart() {
        super.onStart();
        if (mWiqSwitchPref != null) {
            Message getWiqSwitch = mWcdmaIqHandler.obtainMessage(GET_WIQ_SWITCH);
            mWcdmaIqHandler.sendMessage(getWiqSwitch);
        }
        if (mFunctionItemPref != null) {
            // lastFunctionItem = mFunctionItemPref.getValue();
            // mFunctionItemPref.setSummary(mFunctionItemPref.getEntry());
            Message getFunctionItem = mWcdmaIqHandler
                    .obtainMessage(GET_FUNCTION_ITEM);
            mWcdmaIqHandler.sendMessage(getFunctionItem);
        }
        if (mIqSamplePosPref != null) {
            // lastIqSamplePos = mIqSamplePosPref.getValue();
            // mIqSamplePosPref.setSummary(mIqSamplePosPref.getEntry());
            Message getIqSamplePos = mWcdmaIqHandler
                    .obtainMessage(GET_IQ_SAMPLE_POS);
            mWcdmaIqHandler.sendMessage(getIqSamplePos);
        }
    }

    class mWcdmaIqHandler extends Handler {
        public mWcdmaIqHandler(Looper looper) {
            super.getLooper();
        }

        @Override
        public void handleMessage(Message msg) {
            String atResponse;
            String responseValue;
            switch (msg.what) {
                case GET_WIQ_SWITCH:
                    atResponse = sendAt(engconstents.ENG_AT_IQMENU + "0,0",
                            "atchannel0");
                    responseValue = analysisResponse(atResponse, GET_WIQ_SWITCH);
                    if (responseValue.equals(IATUtils.AT_FAIL)) {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Log.d(TAG, "GET_WIQ_SWITCH Fail");
                                mWiqSwitchPref.setEnabled(false);
                                mWiqSwitchPref.setSummary(R.string.feature_abnormal);
                                mFunctionItemPref.setEnabled(false);
                                mIqSamplePosPref.setEnabled(false);
                            }
                        });
                    }
                    if (responseValue.trim().equals("1")) {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                mWiqSwitchPref.setChecked(true);
                            }
                        });
                    } else {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                mWiqSwitchPref.setChecked(false);
                                mFunctionItemPref.setEnabled(false);
                                mIqSamplePosPref.setEnabled(false);
                            }
                        });
                    }
                    break;
                case SET_WIQ_SWITCH_OPEN:
                    atResponse = sendAt(engconstents.ENG_AT_IQMENU + "0,1,1",
                            "atchannel0");
                    responseValue = analysisResponse(atResponse,
                            SET_WIQ_SWITCH_OPEN);
                    String atIqOpen = SendSlogModemAt("ENABLE_IQ " + mSupportMode + " WCDMA");
                if (responseValue != null
                        && responseValue.contains(IATUtils.AT_OK)
                        && atIqOpen != null && atIqOpen.contains(IATUtils.AT_OK)) {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                mWiqSwitchPref.setChecked(true);
                                mFunctionItemPref.setEnabled(true);
                                mIqSamplePosPref.setEnabled(true);
                                PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
                                pm.reboot("iqmode");
                            }
                        });
                    } else {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                mWiqSwitchPref.setChecked(false);
                            }
                        });
                    }
                    break;
                case SET_WIQ_SWITCH_CLOSE:
                    atResponse = sendAt(engconstents.ENG_AT_IQMENU + "0,1,0",
                            "atchannel0");
                    responseValue = analysisResponse(atResponse,
                            SET_WIQ_SWITCH_CLOSE);
                    String atIqClose = SendSlogModemAt("DISABLE_IQ " + mSupportMode + " WCDMA");
                    if (responseValue != null && responseValue.contains(IATUtils.AT_OK)
                            && atIqClose != null && atIqClose.contains(IATUtils.AT_OK)) {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                mWiqSwitchPref.setChecked(false);
                                mFunctionItemPref.setEnabled(false);
                                mIqSamplePosPref.setEnabled(false);
                                PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
                                pm.reboot(null);
                            }

                        });
                    } else {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                mWiqSwitchPref.setChecked(true);
                            }
                        });
                    }
                    break;
                case GET_FUNCTION_ITEM:
                    atResponse = sendAt(engconstents.ENG_AT_IQMENU + "1,0",
                            "atchannel0");
                    functionItemValue = analysisResponse(atResponse,
                            GET_FUNCTION_ITEM);
                    if (!functionItemValue.trim().equals("-1")
                            && atResponse.contains(IATUtils.AT_OK)) {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                // TODO Auto-generated method stub
                                mFunctionItemPref.setValueIndex(Integer
                                        .valueOf(functionItemValue.trim()));
                                mFunctionItemPref.setSummary(mFunctionItemPref
                                        .getEntry());
                            }
                        });
                    } else if (functionItemValue.contains(IATUtils.AT_FAIL)) {
                        Toast.makeText(WCDMAIQActivity.this,
                                "GET_FUNCTION_ITEM Fail", Toast.LENGTH_SHORT)
                                .show();
                    }
                    break;
                case SET_FUNCTION_ITEM:
                    boolean flag = true;
                    if ((Integer.valueOf(mFunctionItemPref.getValue())).intValue() == 15) {
                        try {
                            String[] iqCmd = {
                                    "system/bin/iqdata_daemon", "start", "/sdcard"
                            };
                            Runtime.getRuntime().exec(iqCmd);
                        } catch (Exception e) {
                            flag = false;
                            e.printStackTrace();
                        }
                        if (flag) {
                            uiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mFunctionItemPref.setSummary(mFunctionItemPref
                                            .getEntry());
                                }
                            });
                        } else {
                            uiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Message getFunctionItem = mWcdmaIqHandler
                                            .obtainMessage(GET_FUNCTION_ITEM);
                                    mWcdmaIqHandler.sendMessage(getFunctionItem);
                                }

                            });
                        }
                    } else {
                        String ftValue = (String) msg.obj;
                        atResponse = sendAt(engconstents.ENG_AT_IQMENU + "1,1,"
                                + ftValue, "atchannel0");
                        responseValue = analysisResponse(atResponse, SET_FUNCTION_ITEM);
                        if (responseValue.contains(IATUtils.AT_OK)) {
                            uiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mFunctionItemPref.setSummary(mFunctionItemPref
                                            .getEntry());
                                }
                            });
                        } else {
                            uiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    Message getFunctionItem = mWcdmaIqHandler
                                            .obtainMessage(GET_FUNCTION_ITEM);
                                    mWcdmaIqHandler.sendMessage(getFunctionItem);
                                }

                            });
                        }
                    }
                    break;
                case GET_IQ_SAMPLE_POS:
                    atResponse = sendAt(engconstents.ENG_AT_IQMENU + "2,0",
                            "atchannel0");
                    iqSamplePosValue = analysisResponse(atResponse,
                            GET_IQ_SAMPLE_POS);
                    if (!iqSamplePosValue.trim().equals("-1")
                            && atResponse.contains(IATUtils.AT_OK)) {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIqSamplePosPref.setSummary(mIqSamplePosPref
                                        .getEntry());
                                mIqSamplePosPref.setValueIndex(Integer
                                        .valueOf(iqSamplePosValue));
                            }
                        });
                    } else if (iqSamplePosValue.contains(IATUtils.AT_FAIL)) {
                        Toast.makeText(WCDMAIQActivity.this,
                                "GET_IQ_SAMPLE_POS Fail", Toast.LENGTH_SHORT)
                                .show();
                    }
                    break;
                case SET_IQ_SAMPLE_POS:
                    String iqpValue = (String) msg.obj;
                    atResponse = sendAt(engconstents.ENG_AT_IQMENU + "2,1,"
                            + iqpValue, "atchannel0");
                    responseValue = analysisResponse(atResponse, SET_IQ_SAMPLE_POS);
                    if (responseValue.contains(IATUtils.AT_OK)) {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mIqSamplePosPref.setSummary(mIqSamplePosPref
                                        .getEntry());
                            }
                        });
                    } else {
                        uiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Message getIqSamplePos = mWcdmaIqHandler
                                        .obtainMessage(GET_IQ_SAMPLE_POS);
                                mWcdmaIqHandler.sendMessage(getIqSamplePos);
                            }
                        });
                    }
                    break;
            }
        }
    }

    private String sendAt(String cmd, String serverName) {
        String strTemp = IATUtils.sendATCmd(cmd, serverName);
        return strTemp;
    }

    private String SendSlogModemAt(String cmd) {
        Log.d(TAG, "SendSlogModemAt " + cmd);
        String strTmp = SocketUtils.sendCmdAndRecResult("slogmodem",
                LocalSocketAddress.Namespace.ABSTRACT, cmd);
        return strTmp;
    }

    private String analysisResponse(String response, int type) {
        if (response != null && response.contains(IATUtils.AT_OK)) {
            Log.d(TAG, response + "");
            if (type == GET_WIQ_SWITCH || type == GET_FUNCTION_ITEM
                    || type == GET_IQ_SAMPLE_POS) {
                String[] str = response.split("\n");
                String[] strs = str[0].split(",");
                Log.d(TAG, type + " " + strs[2]);
                return strs[2].trim();
            } else if (type == SET_FUNCTION_ITEM || type == SET_IQ_SAMPLE_POS
                    || type == SET_WIQ_SWITCH_OPEN
                    || type == SET_WIQ_SWITCH_CLOSE) {
                return IATUtils.AT_OK;
            }
        }
        return IATUtils.AT_FAIL;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object object) {
        // TODO Auto-generated method stub
        int logType = 0;
        if (preference == mWiqSwitchPref) {
            if (mWiqSwitchPref.isChecked()) {
                logType = SET_WIQ_SWITCH_CLOSE;
            } else {
                logType = SET_WIQ_SWITCH_OPEN;
            }
        }
        Message message = mWcdmaIqHandler.obtainMessage(logType);
        mWcdmaIqHandler.sendMessage(message);
        return true;
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
            String key) {
        if (key.equals(KEY_FUNCTION_ITEM)) {
            String str = sharedPreferences.getString(key, "");
            Log.d(TAG, "onSharedPreferenceChange,key=" + key + ",value=" + str);
            Message setFunctionItem = mWcdmaIqHandler.obtainMessage(
                    SET_FUNCTION_ITEM, str);
            mWcdmaIqHandler.sendMessage(setFunctionItem);
        }
        if (key.equals(KEY_IQ_SAMPLE_POS)) {
            String str = sharedPreferences.getString(key, "");
            Log.d(TAG, "onSharedPreferenceChange,key=" + key + ",value=" + str);
            Message setIqSamplePos = mWcdmaIqHandler.obtainMessage(
                    SET_IQ_SAMPLE_POS, str);
            mWcdmaIqHandler.sendMessage(setIqSamplePos);
        }
    }
}
