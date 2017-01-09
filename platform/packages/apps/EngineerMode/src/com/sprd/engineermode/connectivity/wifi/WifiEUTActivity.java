
package com.sprd.engineermode.connectivity.wifi;

import com.sprd.engineermode.R;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.net.LocalSocketAddress;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import com.sprd.engineermode.EMSwitchPreference;
import android.util.Log;
import android.widget.Toast;
import com.sprd.engineermode.utils.SocketUtils;
import android.os.SystemProperties;

public class WifiEUTActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener, Preference.OnPreferenceClickListener {

    private static final String TAG = "WifiTestActivity";
    private static final String KEY_POWER_SAVE_MODE = "wifi_power_save_mode";
    private static final String KEY_WIFI_TX = "wifi_tx";
    private static final String KEY_WIFI_RX = "wifi_rx";
    private static final String KEY_WIFI_REG = "wifi_reg_wr";

    // socket communication message
    private static final int DISABLED_POWER_SAVE = 0;
    private static final int ENABLED_POWER_SAVE = 1;
    private static final int INIT_TEST_STATUS = 2;
    private static final int DEINIT_TEST_STATUS = 3;

    // ui message
    private static final int UI_MESSAGE = 0;

    private static final String INSMODE_RES = "insmode_result";

    private EMSwitchPreference mDisPowerSaveMode;
    private Preference mWifi_TX;
    private Preference mWifi_RX;
    private Preference mWifi_REG;

    private ProgressDialog mProgress;

    private Handler mUiThread = new Handler();
    private WTHandler mWTHandler;
    private String mResult;

    private boolean mInsmodeSuccess = false;
    boolean isMarlin = SystemProperties.get("ro.modem.wcn.enable").equals("1");

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_wifi_test);
        mDisPowerSaveMode = (EMSwitchPreference) findPreference(KEY_POWER_SAVE_MODE);
        mDisPowerSaveMode.setOnPreferenceChangeListener(this);
        mDisPowerSaveMode.setEnabled(false);

        mWifi_TX = (Preference) findPreference(KEY_WIFI_TX);
        mWifi_TX.setOnPreferenceClickListener(this);
        mWifi_TX.setEnabled(false);

        mWifi_RX = (Preference) findPreference(KEY_WIFI_RX);
        mWifi_RX.setOnPreferenceClickListener(this);
        mWifi_RX.setEnabled(false);

        mWifi_REG = (Preference) findPreference(KEY_WIFI_REG);
        mWifi_REG.setOnPreferenceClickListener(this);
        mWifi_REG.setEnabled(false);

        mProgress = ProgressDialog.show(this, "Wifi Initing...", "Please wait...", true, true);

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mWTHandler = new WTHandler(ht.getLooper());
        Message initStatus = mWTHandler.obtainMessage(INIT_TEST_STATUS);
        mWTHandler.sendMessage(initStatus);
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    public void onBackPressed() {
        Log.d(TAG,"onBackPressed() mProgress="+mProgress);
        if (mProgress != null) {
            mProgress.dismiss();
            finish();
        } else {
            Message deInitStatus = mWTHandler.obtainMessage(DEINIT_TEST_STATUS);
            mWTHandler.sendMessage(deInitStatus);
        }
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object objValue) {
        Message switchMsg = null;
        if (pref.getKey().equals(KEY_POWER_SAVE_MODE)) {
            if (mDisPowerSaveMode.isChecked()) {
                switchMsg = mWTHandler.obtainMessage(ENABLED_POWER_SAVE);
            } else {
                switchMsg = mWTHandler.obtainMessage(DISABLED_POWER_SAVE);
            }
            mWTHandler.sendMessage(switchMsg);
        }
        return false;
    }

    @Override
    public boolean onPreferenceClick(Preference pref) {
        Intent intent = new Intent();
        intent.putExtra(INSMODE_RES, mInsmodeSuccess);
        Log.d(TAG, "WifiEUTActivity insmode result is " + mInsmodeSuccess);
        if (pref.getKey().equals(KEY_WIFI_TX)) {
            intent.setClassName("com.sprd.engineermode",
                    "com.sprd.engineermode.connectivity.wifi.WifiTXActivity");
        } else if (pref.getKey().equals(KEY_WIFI_RX)) {
            intent.setClassName("com.sprd.engineermode",
                    "com.sprd.engineermode.connectivity.wifi.WifiRXActivity");
        } else if (pref.getKey().equals(KEY_WIFI_REG)) {
            intent.setClassName("com.sprd.engineermode",
                    "com.sprd.engineermode.connectivity.wifi.WifiREGWRActivity");
        }
        startActivity(intent);
        return false;
    }

    class WTHandler extends Handler {

        public WTHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case INIT_TEST_STATUS:
                    if (WifiEUTHelper.getHelper().insmodeWifi()
                            && WifiEUTHelper.getHelper().wifiStart()) {
                        mInsmodeSuccess = true;
                        // enable other view
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mProgress.dismiss();
                                mProgress = null;
                                mWifi_TX.setEnabled(true);
                                mWifi_RX.setEnabled(true);
                                mWifi_REG.setEnabled(true);
                                if (isMarlin) {
                                    mDisPowerSaveMode.setEnabled(true);
                                } else {
                                    // mDisPowerSaveMode.setSummary(R.string.feature_not_support);
                                    mDisPowerSaveMode.setEnabled(false);
                                }
                            }
                        });
                        if (isMarlin) {
                            mResult = WifiEUTHelper.getHelper().wifiGetStatus();
                            if (mResult != null && mResult.contains(SocketUtils.OK)) {
                                String[] str = mResult.split("\\:");
                                String status = str[1].trim();
                                Log.d(TAG, "Power Save mode Status is " + status);
                                if (status.equals("1")) {
                                    mUiThread.post(new Runnable() {
                                        @Override
                                        public void run() {
                                            mDisPowerSaveMode.setChecked(false);
                                        }
                                    });
                                } else if (status.equals("0")) {
                                    mUiThread.post(new Runnable() {
                                        @Override
                                        public void run() {
                                            mDisPowerSaveMode.setChecked(true);
                                        }
                                    });
                                }
                            } else {
                                mUiThread.post(new Runnable() {
                                    @Override
                                    public void run() {
                                        mDisPowerSaveMode.setEnabled(false);
                                        Toast.makeText(WifiEUTActivity.this,
                                                "GET_Power_Save_Mode Fail", Toast.LENGTH_SHORT)
                                                .show();
                                    }
                                });
                            }
                        }
                    } else {
                        Toast.makeText(WifiEUTActivity.this,
                                "Insmod /system/lib/modules/sprdwl.ko or Start Wifi Fail",
                                Toast.LENGTH_SHORT).show();
                        finish();
                    }
                    break;
                case DISABLED_POWER_SAVE:
                    mResult = WifiEUTHelper.getHelper().wifiSetStatusOff();
                    Log.d(TAG, "DISABLED_POWER_SAVE Result is " + mResult);
                    if (mResult != null && mResult.contains(SocketUtils.OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDisPowerSaveMode.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDisPowerSaveMode.setChecked(false);
                                Toast.makeText(WifiEUTActivity.this, "DISABLED_POWER_SAVE Fail",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    }
                    break;
                case ENABLED_POWER_SAVE:
                    mResult = WifiEUTHelper.getHelper().wifiSetStatusOn();
                    Log.d(TAG, "ENABLED_POWER_SAVE Result is " + mResult);
                    if (mResult != null && mResult.contains(SocketUtils.OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDisPowerSaveMode.setChecked(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mDisPowerSaveMode.setChecked(true);
                                Toast.makeText(WifiEUTActivity.this, "ENABLED_POWER_SAVE Fail",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    }
                    break;
                case DEINIT_TEST_STATUS:
                    if (!WifiEUTHelper.getHelper().wifiStop()
                            || !WifiEUTHelper.getHelper().remodeWifi()) {
                        Toast.makeText(WifiEUTActivity.this,
                                "Rmmod /system/lib/modules/sprdwl.ko or Stop Wifi Fail",
                                Toast.LENGTH_SHORT).show();
                    } else {
                        mInsmodeSuccess = false;
                    }
                    finish();
                    break;
                default:
                    break;
            }
        }
    }
}
