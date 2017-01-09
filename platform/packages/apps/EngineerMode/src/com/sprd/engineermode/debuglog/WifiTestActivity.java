
package com.sprd.engineermode.debuglog;

import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.content.Context;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.TwoStatePreference;
import android.util.Log;
import android.widget.Toast;
import android.os.SystemProperties;
import android.app.Activity;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import java.nio.charset.StandardCharsets;
import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.SocketUtils;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.SupplicantState;
import android.net.NetworkInfo;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

public class WifiTestActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {

    private static final String TAG = "WifiTestActivity";
    private static final String KEY_MAX_POWER = "max_power";
    private static final String KEY_LNA_BYPASS = "lna_bypass";

    // SET_MAX_POWER1 is used for 11g/11n,SET_MAX_POWER2 is used for
    private static final String SET_MAX_POWER1 = "eng iwnpi wlan0 set_tx_power 1127";
    private static final String SET_MAX_POWER2 = "eng iwnpi wlan0 set_tx_power 127";
    private static final String UNSET_MAX_POWER1 = "eng iwnpi wlan0 set_tx_power 1088";
    private static final String UNSET_MAX_POWER2 = "eng iwnpi wlan0 set_tx_power 72";
    private static final String SET_LNA_BYPASS_ON = "eng iwnpi wlan0 lna_on";
    private static final String SET_LNA_BYPASS_OFF = "eng iwnpi wlan0 lna_off";
    private static final String GET_LNA_BYPASS_STATUS = "eng iwnpi wlan0 lna_status";

    private static final String SET_EUT_START = "eng iwnpi wlan0 start";
    private static final String SET_EUT_STOP = "eng iwnpi wlan0 stop";

    private static final String WIFI_INSMOD = "insmod /lib/modules/sprdwl.ko";
    private static final String WIFI_RMMOD = "rmmod /lib/modules/sprdwl.ko";

    private static final String SOCKET_NAME = "wcnd";

    private static final int INIT_TEST_STATUS = 1;
    private static final int DEINIT_TEST_STATUS = 2;
    private static final int SET_MAX_POWER = 3;
    private static final int UNSET_MAX_POWER = 4;
    private static final int SET_LNA_ON = 5;
    private static final int SET_LNA_OFF = 6;
    private static final int GET_LNA_STATUS = 7;

    // add 542894 by alisa.li
    private static final int DISABLED_NO_SLEEP = 0;
    private static final int ENABLED_NO_SLEEP = 8;
    boolean isMarlin = SystemProperties.get("ro.modem.wcn.enable").equals("1");

    private TwoStatePreference mMaxPower;
    private TwoStatePreference mLNA_bypass;

    private Handler mUiThread = new Handler();
    private WTHandler mWTHandler;

    private String mResult1;
    private String mResult2;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_cmccwifitest);
        mMaxPower = (TwoStatePreference) findPreference(KEY_MAX_POWER);
        mMaxPower.setOnPreferenceChangeListener(this);
        mLNA_bypass = (TwoStatePreference) findPreference(KEY_LNA_BYPASS);
        mLNA_bypass.setOnPreferenceChangeListener(this);
        /*bug653284
        if (isMarlin) {
            if (isWifiOn()) {
                Log.d(TAG, "sensen wifi on");
                AlertDialog alertDialog = new AlertDialog.Builder(WifiTestActivity.this)
                        .setTitle(getString(R.string.alert_wifi_test))
                        .setMessage(getString(R.string.alert_close_wifi))
                        .setPositiveButton(
                                getString(R.string.alertdialog_ok),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        finish();
                                    }
                                }).create();
                alertDialog.show();
                Log.d(TAG, "sensen alertDialog.show");
            }
        }*/

        // Modify 349567 by sprd
        if (SystemProperties.get("ro.modem.wcn.enable", "0").equals("0")) {
            mMaxPower.setEnabled(false);
            mMaxPower.setSummary(R.string.feature_not_support);
            mLNA_bypass.setEnabled(true);
            // mLNA_bypass.setSummary(R.string.feature_not_support);
        } else {
            mMaxPower.setEnabled(true);
            mLNA_bypass.setEnabled(true);
        }

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mWTHandler = new WTHandler(ht.getLooper());
        Message initStatus = mWTHandler.obtainMessage(INIT_TEST_STATUS);
        mWTHandler.sendMessage(initStatus);

        if(!isMarlin){
            final IntentFilter filter = new IntentFilter();
            filter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
            registerReceiver(mReceiver, filter);
        }
    }

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
                Log.d(TAG, "wifi changed: " + intent.getAction());
                Message switchMsg = null;
                WifiManager mWifiManager = (WifiManager) getSystemService(WIFI_SERVICE);
                WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
                SupplicantState state = wifiInfo.getSupplicantState();
                if (wifiInfo != null && state == SupplicantState.COMPLETED) {
                    // wifi connected
                    //Log.d(TAG, "wifi connected");
                    //Log.d(TAG, "open no sleep");
                    switchMsg = mWTHandler.obtainMessage(ENABLED_NO_SLEEP);
                    mWTHandler.sendMessage(switchMsg);
                } else {
                    // wifi disconnected
                    //Log.d(TAG, "wifi disconnected");
                    //Log.d(TAG, "close no sleep");
                    switchMsg = mWTHandler.obtainMessage(DISABLED_NO_SLEEP);
                    mWTHandler.sendMessage(switchMsg);
                }
            }
        }
    };

    @Override
    protected void onStart() {
        if (isMarlin) {
            if (mLNA_bypass != null && mLNA_bypass.isEnabled()) {
                Message getLNAStatus = mWTHandler.obtainMessage(GET_LNA_STATUS);
                mWTHandler.sendMessage(getLNAStatus);
            }
        } else {
            if (SystemProperties.get("persist.sys.nosleep.enabled", "0").equals("0")) {
                mLNA_bypass.setChecked(false);
            } else {
                mLNA_bypass.setChecked(true);
            }
        }
        super.onStart();
    }

    @Override
    protected void onDestroy() {
        if(!isMarlin){
            unregisterReceiver(mReceiver);
        }
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        Message exitStatus = mWTHandler.obtainMessage(DEINIT_TEST_STATUS);
        mWTHandler.sendMessage(exitStatus);
    }

    private boolean isWifiOn() {
        WifiManager wifiManager = (WifiManager) WifiTestActivity.this.getSystemService(
                Context.WIFI_SERVICE);
        if (wifiManager.isWifiEnabled()) {
            return true;
        }
        return false;
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object objValue) {
        Message switchMsg = null;
        if (pref == mMaxPower) {
            if (!mMaxPower.isChecked()) {
                Message setPower = mWTHandler.obtainMessage(SET_MAX_POWER);
                mWTHandler.sendMessage(setPower);
            } else {
                Message unSetPower = mWTHandler.obtainMessage(UNSET_MAX_POWER);
                mWTHandler.sendMessage(unSetPower);
            }
            return false;
        }
        if (pref == mLNA_bypass) {
            if (isMarlin) {
                if (mLNA_bypass.isChecked()) {
                    Message setOff = mWTHandler.obtainMessage(SET_LNA_OFF);
                    mWTHandler.sendMessage(setOff);
                } else {
                    Message setOn = mWTHandler.obtainMessage(SET_LNA_ON);
                    mWTHandler.sendMessage(setOn);
                }
                return false;
            } else {
                if (!checkWifiStatus()) {
                    if (mLNA_bypass.isChecked()) {
                        switchMsg = mWTHandler.obtainMessage(DISABLED_NO_SLEEP);
                    } else {
                        switchMsg = mWTHandler.obtainMessage(ENABLED_NO_SLEEP);
                    }
                    mWTHandler.sendMessage(switchMsg);
                } else {
                    Toast.makeText(WifiTestActivity.this, "Please open wifi", Toast.LENGTH_SHORT)
                            .show();
                }
            }
            return false;
        }
        return false;
    }

    class WTHandler extends Handler {
        private LocalSocket mSocketClient;
        private LocalSocketAddress mSocketAddress;
        private OutputStream mOutputStream;
        private InputStream mInputStream;

        private LocalSocket mInsmodeSocketClient;
        private LocalSocketAddress mInsmodeSocketAddress;
        private OutputStream mInsmodeOutputStream;
        private InputStream mInsmodeInputStream;

        public boolean mWifiDriverInstalled = false;
        public boolean mWifiStarted = false;

        public boolean wifiStart() {
            if (isMarlin) {
                mResult1 = SocketUtils.sendCmdAndRecResult(SOCKET_NAME,
                        LocalSocketAddress.Namespace.ABSTRACT, SET_EUT_START);
                Log.d(TAG, "SET_EUT_START Result is " + mResult1);
                if (mResult1 != null && mResult1.contains(SocketUtils.OK)) {
                    mWifiStarted = true;
                    return true;
                } else {
                    Log.e(TAG, "wifiStart fail");
                    return false;
                }
            } else {
                return true;
            }
        }

        /**
         * send iwnpi wlan0 stop
         * 
         * @return true if success
         */
        public boolean wifiStop() {
            if (isMarlin) {
                mResult1 = SocketUtils.sendCmdAndRecResult(SOCKET_NAME,
                        LocalSocketAddress.Namespace.ABSTRACT, SET_EUT_STOP);
                Log.d(TAG, "SET_EUT_STOP Result is " + mResult1);
                if (mResult1 != null && mResult1.contains(SocketUtils.OK)) {
                    mWifiStarted = false;
                    return true;
                } else {
                    Log.e(TAG, "wifiStop fail");
                    return false;
                }
            } else {
                return true;
            }
        }

        public boolean insmodWifi() {
            if (isMarlin) {
                // connect socket and send cmd (insmod /lib/modules/sprdwl.ko)
                SystemProperties.set("persist.sys.cmdservice.enable", "enable");
                String status = SystemProperties.get("persist.sys.cmdservice.enable", "");
                Log.d(TAG, "cmd_service pro is " + status);
                // sleep 100ms to make sure cmd_service start
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                try {
                    mInsmodeSocketClient = new LocalSocket();
                    mInsmodeSocketAddress = new LocalSocketAddress("cmd_skt",
                            LocalSocketAddress.Namespace.ABSTRACT);
                    mInsmodeSocketClient.connect(mInsmodeSocketAddress);
                    Log.d(TAG,
                            "mInsmodeSocketClient connect is " + mInsmodeSocketClient.isConnected());
                    // insmod driver
                    try {
                        byte[] buffer = new byte[1024];
                        mInsmodeOutputStream = mInsmodeSocketClient.getOutputStream();
                        if (mInsmodeOutputStream != null) {
                            final StringBuilder cmdBuilder = new StringBuilder(
                                    "insmod /lib/modules/sprdwl.ko").append('\0');
                            final String cmmand = cmdBuilder.toString();
                            mInsmodeOutputStream.write(cmmand.getBytes(StandardCharsets.UTF_8));
                            mInsmodeOutputStream.flush();
                        }
                        mInsmodeInputStream = mInsmodeSocketClient.getInputStream();
                        int count = mInsmodeInputStream.read(buffer, 0, 1024);
                        String insmodResult = new String(buffer, "utf-8");
                        Log.d(TAG, "insmodeResult is " + insmodResult);
                        String[] str = insmodResult.split("\n");
                        if ("Result".equals(str[0].trim()) || insmodResult.contains("File exists")) {
                            Log.d(TAG, "insmod /lib/modules/sprdwl.ko is success");
                            return true;
                        }
                    } catch (Exception e) {
                        Log.d(TAG, "Failed get outputStream: " + e);
                        e.printStackTrace();
                        return false;
                    }
                } catch (Exception e) {
                    Log.d(TAG, "mInsmodeSocketClient connect is false");
                    e.printStackTrace();
                    return false;
                }
                return false;
            } else {
                return true;
            }
        }

        /**
         * we shoule reload the wifi driver when finish WifiTXActivity/WifiRXActivity
         * 
         * @return true if reload success, false if fail
         */
        public boolean rmmodWifi() {
            if (isMarlin) {
                if (!mInsmodeSocketClient.isConnected()) {
                    try {
                        mInsmodeSocketClient.connect(mInsmodeSocketAddress);
                    } catch (Exception e) {
                        Log.d(TAG, "remodeWifi mInsmodeSocketClient is not connected");
                    }
                }
                try {
                    byte[] buffer = new byte[1024];
                    mInsmodeOutputStream = mInsmodeSocketClient.getOutputStream();
                    if (mInsmodeOutputStream != null) {
                        final StringBuilder cmdBuilder = new StringBuilder(
                                "rmmod /lib/modules/sprdwl.ko").append('\0');
                        final String cmmand = cmdBuilder.toString();
                        mInsmodeOutputStream.write(cmmand.getBytes(StandardCharsets.UTF_8));
                        mInsmodeOutputStream.flush();
                    }
                    mInsmodeInputStream = mInsmodeSocketClient.getInputStream();
                    int count = mInsmodeInputStream.read(buffer, 0, 1024);
                    String rmmodResult = new String(buffer, "utf-8");
                    Log.d(TAG, "count is " + count + ",rmmodResult is " + rmmodResult);
                    String[] str = rmmodResult.split("\n");
                    if (!"Result".equals(str[0].trim())) {
                        Log.d(TAG, "rmmod /lib/modules/sprdwl.ko is fail");
                        return false;
                    }
                } catch (Exception e) {
                    Log.d(TAG, "Failed get outputStream: " + e);
                    e.printStackTrace();
                    return false;
                } finally {
                    // close socket
                    try {
                        if (mInsmodeOutputStream != null) {
                            mInsmodeOutputStream.close();
                        }
                        if (mInsmodeInputStream != null) {
                            mInsmodeInputStream.close();
                        }
                        if (mInsmodeSocketClient != null) {
                            mInsmodeSocketClient.close();
                        }
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }

                SystemProperties.set("persist.sys.cmdservice.enable", "disable");
                String status = SystemProperties.get("persist.sys.cmdservice.enable", "");
                Log.d(TAG, "status:" + status);
                return true;
            } else {
                return true;
            }
        }

        public WTHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case INIT_TEST_STATUS: {
                    if(isWifiOn())
                    {
                        Log.d(TAG, "wifi is on, so ignore insmodWifi and wifiStart");
                        //do nothing.
                        //this is to fix  Bug #546594
                    }
                    else
                    {
                        if (insmodWifi() && wifiStart()) {
                            Log.d(TAG, "insmodWifi and wifiStart success");
                        } else {
                            Log.d(TAG, "insmodWifi and wifiStart failed");
                            Toast.makeText(WifiTestActivity.this,
                                    "Insmod /lib/modules/sprdwl.ko or Start Wifi Fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    }
                    break;
                }

                case DEINIT_TEST_STATUS: {
                    if(isWifiOn())
                    {
                        Log.d(TAG, "wifi is on, so ignore wifiStop and rmmodWifi");
                        //do nothing.
                        // this is to fix Bug #546594
                    }
                    else
                    {
                        if (wifiStop() && rmmodWifi()) {
                            Log.d(TAG, "wifiStop and rmmodWifi success");
                        } else {
                            Log.d(TAG, "wifiStop and rmmodWifi failed");
                            Toast.makeText(WifiTestActivity.this,
                                    "Rmmod /lib/modules/sprdwl.ko or Stop Wifi Fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    }
                    finish();
                    break;
                }

                case SET_MAX_POWER: {
                    mResult1 = SocketUtils.sendCmdAndRecResult(SOCKET_NAME,
                            LocalSocketAddress.Namespace.ABSTRACT, SET_MAX_POWER1);
                    Log.d(TAG, "SET_MAX_POWER1 Result is " + mResult1);
                    mResult2 = SocketUtils.sendCmdAndRecResult(SOCKET_NAME,
                            LocalSocketAddress.Namespace.ABSTRACT, SET_MAX_POWER2);
                    Log.d(TAG, "SET_MAX_POWER2 Result is " + mResult2);
                    if (mResult1 != null && mResult1.contains(SocketUtils.OK) || mResult2 != null
                            && mResult2.contains(SocketUtils.OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mMaxPower.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mMaxPower.setChecked(false);
                            }
                        });
                    }
                    break;
                }
                case UNSET_MAX_POWER: {
                    mResult1 = SocketUtils.sendCmdAndRecResult(SOCKET_NAME,
                            LocalSocketAddress.Namespace.ABSTRACT, UNSET_MAX_POWER1);
                    Log.d(TAG, "UNSET_MAX_POWER1 Result is " + mResult1);
                    mResult1 = SocketUtils.sendCmdAndRecResult(SOCKET_NAME,
                            LocalSocketAddress.Namespace.ABSTRACT, UNSET_MAX_POWER2);
                    Log.d(TAG, "UNSET_MAX_POWER2 Result is " + mResult2);
                    if (mResult1 != null && mResult1.contains(SocketUtils.OK) || mResult2 != null
                            && mResult2.contains(SocketUtils.OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mMaxPower.setChecked(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mMaxPower.setChecked(true);
                            }
                        });
                    }
                    break;
                }
                case SET_LNA_ON: {
                    mResult1 = SocketUtils.sendCmdAndRecResult(SOCKET_NAME,
                            LocalSocketAddress.Namespace.ABSTRACT, SET_LNA_BYPASS_ON);
                    Log.d(TAG, "SET_LNA_ON Result is " + mResult1);
                    if (mResult1 != null && mResult1.contains(SocketUtils.OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLNA_bypass.setChecked(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLNA_bypass.setChecked(false);
                            }
                        });
                    }
                    Log.d(TAG, "SET_LNA_ON, mLNA_bypass.setChecked");
                    break;
                }
                case SET_LNA_OFF: {
                    mResult1 = SocketUtils.sendCmdAndRecResult(SOCKET_NAME,
                            LocalSocketAddress.Namespace.ABSTRACT, SET_LNA_BYPASS_OFF);
                    Log.d(TAG, "SET_LNA_OFF Result is " + mResult1);
                    if (mResult1 != null && mResult1.contains(SocketUtils.OK)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLNA_bypass.setChecked(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLNA_bypass.setChecked(true);
                            }
                        });
                    }
                    Log.d(TAG, "SET_LNA_OFF, mLNA_bypass.setChecked");
                    break;
                }
                case GET_LNA_STATUS: {
                    mResult1 = SocketUtils.sendCmdAndRecResult(SOCKET_NAME,
                            LocalSocketAddress.Namespace.ABSTRACT, GET_LNA_BYPASS_STATUS);
                    if (mResult1 != null && mResult1.contains(SocketUtils.OK)) {
                        String[] str = mResult1.split("\\:");
                        String status = str[1].trim();
                        Log.d(TAG, "LNA Status is " + status);
                        if (status.equals("1")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mLNA_bypass.setChecked(true);
                                }
                            });
                        } else if (status.equals("0")) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mLNA_bypass.setChecked(false);
                                }
                            });
                        }
                    } else {
                        Log.d(TAG, "GET_LNA_STATUS Fail");
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mLNA_bypass.setEnabled(false);
                                // mLNA_bypass.setSummary(R.string.feature_abnormal);
                            }
                        });
                    }
                    break;
                }
                case DISABLED_NO_SLEEP: {
                    SystemProperties.set("ctl.start", "wl_PM_2");
                    SystemProperties.set("ctl.start", "wl_mpc_1");
                    SystemProperties.set("ctl.start", "wl_press_0");
                    SystemProperties.set("persist.sys.nosleep.enabled", "0");
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            // TODO Auto-generated method stub
                            mLNA_bypass.setChecked(false);
                        }
                    });
                    break;
                }
                case ENABLED_NO_SLEEP: {
                    SystemProperties.set("ctl.start", "wl_PM_0");
                    SystemProperties.set("ctl.start", "wl_mpc_0");
                    SystemProperties.set("ctl.start", "wl_press_1");
                    SystemProperties.set("persist.sys.nosleep.enabled", "1");
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            // TODO Auto-generated method stub
                            mLNA_bypass.setChecked(true);
                        }
                    });
                    break;
                }
                default:
                    break;
            }
        }
    }

    private boolean checkWifiStatus() {
        WifiManager wifiManager = (WifiManager) this.getSystemService(this.WIFI_SERVICE);
        if (wifiManager.isWifiEnabled()) {
            return false;
        }
        return true;
    }
}
