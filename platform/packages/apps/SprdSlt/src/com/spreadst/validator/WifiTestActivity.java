package com.spreadst.validator;

import java.util.Calendar;
import java.util.List;
import java.util.ArrayList;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.view.View;
import android.os.Bundle;
import java.io.Serializable;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import android.util.Log;

import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.net.ConnectivityManager;
import android.net.wifi.WifiConfiguration.KeyMgmt;
import android.net.wifi.WifiManager.ActionListener;
import android.os.SystemClock;

public class WifiTestActivity extends Activity {
    private static final String TAG = "WifiTestActivity";

    private static final int SECURITY_NONE = 100;

    private static final int SECURITY_WEP = 101;

    private static final int SECURITY_PSK = 102;

    private static final int SECURITY_EAP = 103;

    private static final int SECURITY_WAPI_PSK = 104;

    private static final int SECURITY_WAPI_CERT = 105;

    private WifiManager mWifiManager = null;

    private ConnectivityManager mConnectivityManager = null;

    private WifiReceiver mWifiReceiver = null;

    private boolean mHasConnectSpecify = false;

    private boolean mHasPressedConnect = false;

    private boolean mHasConnectNoPassWord = false;

    private boolean mHasConnectManual = false;

    private boolean mActivityHasFinish = false;

    private boolean mConnectSuccess = false;

    private WifiScanThread mWifiScanThread = null;

    private AlertDialog mInputWifiDialog = null;

    private AlertDialog.Builder mInputBuilder = null;

    private TextView tvWifiAddr = null;

    private TextView tvWifiState = null;

    private TextView tvWifiDeviceList = null;

    private View mInputView = null;

    private EditText mSsidText = null;

    private EditText mPasswordText = null;

    private Button mCancelButton = null;

    private Button mConnectButton = null;

    private static final int DELAY_TIME = 1000;

    private Long mBeginTime = 0l;

    private Long mEndTime = 0l;

    private static final int TIME_OUT = 3000;

    private ActionListener mListener = new ActionListener() {
        @Override
        public void onSuccess() {
            Log.d(TAG, "connect success");
        }

        @Override
        public void onFailure(int reason) {
            Log.e(TAG, " connect failed ");
            mConnectSuccess = false;
        }
    };

    private void handleEvent(Context context, Intent intent) {
        String action = intent.getAction();
        Log.d(TAG, "action ="+action);
        if (WifiManager.WIFI_STATE_CHANGED_ACTION.equals(action)) {
            int newState = mWifiManager.getWifiState();
            Log.d(TAG, "newState: " + newState);
            switch (newState) {
                case WifiManager.WIFI_STATE_ENABLED:
                    startWifiScan();
                    if (!hasAvaliableWifiNetwork()) {
                        startConnectWifiConfig();
                    }
                    break;
                case WifiManager.WIFI_STATE_DISABLED:
                case WifiManager.WIFI_STATE_DISABLING:
                case WifiManager.WIFI_STATE_UNKNOWN:
                case WifiManager.WIFI_STATE_ENABLING:
                default:
                    // do nothing
                    break;
            }
            wifiStateChange(newState);
        } else if (WifiManager.SCAN_RESULTS_AVAILABLE_ACTION.equals(action)) {
            List<ScanResult> wifiScanResultList = mWifiManager.getScanResults();
            Log.d(TAG, "wifiScanResultList ="+wifiScanResultList);

            if (wifiScanResultList == null) {
                return;
            }
            if (hasAvaliableWifiNetwork() && !mActivityHasFinish) {
                Toast.makeText(WifiTestActivity.this, R.string.wifi_connected, Toast.LENGTH_SHORT).show();
                mActivityHasFinish = true;
                finish();
            }
            if (timeOut()) {
                mConnectSuccess = false;
            }
            if (timeOut() && mHasPressedConnect && mHasConnectManual) {
                mHasPressedConnect = false;
                mHasConnectManual = false;
            }
            if (wifiScanResultList.size() != 0) {
                wifiDeviceListChange(wifiScanResultList);

                if (!mConnectSuccess) {
                    startConnectWifiNoPassWord(wifiScanResultList);
                }
            }
        } else if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
            NetworkInfo info = (NetworkInfo) intent.getParcelableExtra(
                    WifiManager.EXTRA_NETWORK_INFO);
            if (hasAvaliableWifiNetwork() && !mActivityHasFinish) {
//            if (info.isConnected() && !mActivityHasFinish) {
                Toast.makeText(WifiTestActivity.this, R.string.wifi_connected, Toast.LENGTH_SHORT).show();
                mActivityHasFinish = true;
                finish();
            }
            if (timeOut()) {
                mConnectSuccess = false;
            } else {
                return;
            }
            if (!mConnectSuccess && mHasConnectNoPassWord && !mHasConnectSpecify) {//only connect no password and specify one time
                startConnectWifiSpecify();
            } else if (!mConnectSuccess && mHasConnectSpecify && !mHasConnectManual) {
                startConnectWifiManual();
            } else if (!mConnectSuccess && mHasPressedConnect) {//can connect many times
                mHasPressedConnect = false;
                mHasConnectManual = false;
                startConnectWifiManual();
            }
        } else if(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION.equals(action)) {
            if (hasAvaliableWifiNetwork() && !mActivityHasFinish) {
                Toast.makeText(WifiTestActivity.this, R.string.wifi_connected, Toast.LENGTH_SHORT).show();
                mActivityHasFinish = true;
                finish();
            }
            if (timeOut()) {
                mConnectSuccess = false;
            } else {
                return;
            }
            if (!mConnectSuccess && mHasConnectNoPassWord && !mHasConnectSpecify) {//only connect no password and specify one time
                startConnectWifiSpecify();
            } else if (!mConnectSuccess && mHasConnectSpecify && !mHasConnectManual) {
                startConnectWifiManual();
            } else if (!mConnectSuccess && mHasPressedConnect) {//can connect many times
                mHasPressedConnect = false;
                mHasConnectManual = false;
                startConnectWifiManual();
            }
        }
    }

    private boolean timeOut() {
        mEndTime = Calendar.getInstance().getTime().getTime();
        return (mEndTime - mBeginTime) >= TIME_OUT ? true : false;
    }

    private class WifiReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG,"WifiStateReceiver onReceive");
            handleEvent(context, intent);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");

        setContentView(R.layout.wifi_scan_view);
        tvWifiAddr = (TextView) findViewById(R.id.wifi_addr_content);
        tvWifiState = (TextView) findViewById(R.id.wifi_state_content);
        tvWifiDeviceList = (TextView) findViewById(R.id.tv_wifi_device_list);

        mInputView = getLayoutInflater().inflate(R.layout.wifi_input_view, null);
        mSsidText = (EditText) mInputView.findViewById(R.id.ssid);
        mPasswordText = (EditText) mInputView.findViewById(R.id.password);

        mCancelButton = (Button) mInputView.findViewById(R.id.cancel);
        mCancelButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                Log.d(TAG, "mCancelButton onClick");
                mInputWifiDialog.dismiss();
                finish();
            }
        });

        mConnectButton = (Button) mInputView.findViewById(R.id.connect);
        mConnectButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View arg0) {
                Log.d(TAG, "mConnectButton onClick");
                mHasPressedConnect = true;
                WifiConfiguration wifiConfiguration = new WifiConfiguration();
                wifiConfiguration.allowedKeyManagement.set(KeyMgmt.WPA_PSK);

                if (mSsidText.length() != 0) {
                    wifiConfiguration.SSID = convertToQuotedString(mSsidText.getText().toString());
                } else {
                    Toast.makeText(WifiTestActivity.this, R.string.ssid_length, Toast.LENGTH_SHORT).show();
                    return;
                }

                if (mPasswordText.length() != 0) {
                    String password = mPasswordText.getText().toString();
                    if (password.matches("[0-9A-Fa-f]{64}")) {
                        wifiConfiguration.preSharedKey = password;
                    } else {
                        wifiConfiguration.preSharedKey = '"' + password + '"';
                    }
                } else {
                    Toast.makeText(WifiTestActivity.this, R.string.password_length, Toast.LENGTH_SHORT).show();
                    return;
                }
                Log.d(TAG, "ssid = "+wifiConfiguration.SSID + " password="+ wifiConfiguration.preSharedKey);
                mWifiManager.connect(wifiConfiguration, mListener);
                mConnectSuccess = true;
                mBeginTime = Calendar.getInstance().getTime().getTime();
                mSsidText.setText("");
                mPasswordText.setText("");
                mInputWifiDialog.dismiss();
            }
        });
        mInputWifiDialog = new AlertDialog.Builder(WifiTestActivity.this)
        .setIcon(R.drawable.alert_dialog_icon)
        .setTitle(R.string.please_input_ssid_password)
        .setView(mInputView)
        .create();
        mWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        mConnectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);

        tvWifiAddr.setText(mWifiManager.getConnectionInfo().getMacAddress());
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
        registerWifiReceiver();
        startWifiAutoConnectTest();
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause");
        unregisterWifiReceiver();
        mHasConnectSpecify = false;
        mHasPressedConnect = false;
        mHasConnectNoPassWord = false;
        mHasConnectManual = false;
        mConnectSuccess = false;
    }
    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy");
        super.onDestroy();
        mInputWifiDialog = null;
    }

    private void registerWifiReceiver() {
        Log.d(TAG, "registerWifiReceiver");
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
        intentFilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
        intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        intentFilter.addAction(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION);
        mWifiReceiver = new WifiReceiver();
        registerReceiver(mWifiReceiver, intentFilter);
    }

    private void unregisterWifiReceiver() {
        Log.d(TAG, "unregisterWifiReceiver");
        if (mWifiReceiver != null) {
            unregisterReceiver(mWifiReceiver);
            mWifiReceiver = null;
        }
    }

    private boolean hasAvaliableWifiNetwork() {
        return mConnectivityManager.getActiveNetworkInfo() != null &&
                mConnectivityManager.getActiveNetworkInfo().getType() == ConnectivityManager.TYPE_WIFI;
    }

    private void startWifiAutoConnectTest() {
        Log.d(TAG, "startWifiAutoConnectTest");
        if (mWifiManager.isWifiEnabled()) {
            wifiStateChange(WifiManager.WIFI_STATE_ENABLED);
        } else {
            wifiStateChange(WifiManager.WIFI_STATE_DISABLED);
            boolean isOpen = mWifiManager.setWifiEnabled(true);
            if (!isOpen) {
                Log.e(TAG, "startWifiAutoConnectTest() failed -> WifiManager.setWifiEnabled(true) return false!");
                Toast.makeText(this, R.string.open_wifi_fail, Toast.LENGTH_SHORT).show();
                finish();
            }
        }
    }

    private void startConnectWifiConfig() {
        Log.d(TAG, "startConnectWifiConfig");
        List<WifiConfiguration> configs = mWifiManager.getConfiguredNetworks();
        if (configs.size() != 0) {
            WifiConfiguration wifiConfiguration = new WifiConfiguration();
            int prioritymax = -1;
            for (WifiConfiguration config : configs) {
                if(prioritymax < config.priority) {
                    prioritymax = config.priority;
                    wifiConfiguration = config;
                }
            }
            Log.d(TAG, "start connect with SSID : " + wifiConfiguration.SSID);
            mWifiManager.connect(wifiConfiguration, mListener);
            mConnectSuccess = true;
            mBeginTime = Calendar.getInstance().getTime().getTime();
        }
    }

    private void startConnectWifiNoPassWord(List<ScanResult> wifiScanResultList) {
        Log.d(TAG, "startConnectWifiNoPassWord, mHasConnectNoPassWord="+mHasConnectNoPassWord);
        if (!mHasConnectNoPassWord) {
            mHasConnectNoPassWord = true;
            boolean found = false;
            for (ScanResult result : wifiScanResultList) {
                if (result.SSID == null || result.SSID.length() == 0 ||
                        result.capabilities.contains("[IBSS]")) {
                    continue;
                }
                if (SECURITY_NONE == getSecurity(result)){
                    WifiConfiguration wifiConfiguration = new WifiConfiguration();
                    wifiConfiguration.SSID = convertToQuotedString(result.SSID);
                    wifiConfiguration.allowedKeyManagement.set(KeyMgmt.NONE);
                    found =true;
                    Log.d(TAG, "start connect with SSID : " + wifiConfiguration.SSID);
                    mWifiManager.connect(wifiConfiguration, mListener);
                    mConnectSuccess = true;
                    mBeginTime = Calendar.getInstance().getTime().getTime();
                    break;
                }
            }

            if (!found) {
                startConnectWifiSpecify();
            }
        } else {
            startConnectWifiSpecify();
        }
    }

    private String convertToQuotedString(String string) {
        return "\"" + string + "\"";
    }

    private void startConnectWifiSpecify(){
        Log.d(TAG, "startConnectWifiSpecify, mHasConnectSpecify="+mHasConnectSpecify);
        if (!mHasConnectSpecify) {
            mHasConnectSpecify = true;
            String ssid = getResources().getString(R.string.specify_wifi_ssid);
            String password = getResources().getString(R.string.specify_wifi_password);
            WifiConfiguration wifiConfiguration = new WifiConfiguration();
            wifiConfiguration.allowedKeyManagement.set(KeyMgmt.WPA_PSK);
            if (ssid != null && ssid.length() != 0){
                wifiConfiguration.SSID = convertToQuotedString(ssid);
            }
            if (password.matches("[0-9A-Fa-f]{64}")) {
                wifiConfiguration.preSharedKey = password;
            } else {
                wifiConfiguration.preSharedKey = '"' + password + '"';
            }
            Log.d(TAG, "ssid = "+wifiConfiguration.SSID + " password="+ wifiConfiguration.preSharedKey);
            mWifiManager.connect(wifiConfiguration, mListener);
            mConnectSuccess = true;
            mBeginTime = Calendar.getInstance().getTime().getTime();
        } else {
            startConnectWifiManual();
        }
    }

    private void startConnectWifiManual() {
        Log.d(TAG, "startConnectWifiManual, mHasConnectManual="+mHasConnectManual);
        if (!mHasConnectManual && !mActivityHasFinish) {
            mHasConnectManual = true;
            mSsidText.setFocusable(true);
            mSsidText.requestFocus();
            mInputWifiDialog.setCanceledOnTouchOutside(false);
            mInputWifiDialog.show();
        }
    }

    private int getSecurity(ScanResult result) {
        // Broadcom, WAPI
        if (result.capabilities.contains("WAPI-PSK")) {
            return SECURITY_WAPI_PSK;
        } else if (result.capabilities.contains("WAPI-CERT")) {
            return SECURITY_WAPI_CERT;
        } else
        // Broadcom, WAPI
        if (result.capabilities.contains("WEP")) {
            return SECURITY_WEP;
        } else if (result.capabilities.contains("PSK")) {
            return SECURITY_PSK;
        } else if (result.capabilities.contains("EAP")) {
            return SECURITY_EAP;
        }
        return SECURITY_NONE;
    }

    private void startWifiScan() {
        Log.d(TAG, "startWifiScan");
        mWifiScanThread = new WifiScanThread();
        mWifiScanThread.start();
    }

    class WifiScanThread extends Thread {
        @Override
        public void run() {
                // wait until other actions finish.
//          SystemClock.sleep(DELAY_TIME);
            mWifiManager.startScan();
        }
    }

    private void wifiStateChange(int newState) {
        Log.d(TAG, "wifiStateChange");
        switch (newState) {
            case WifiManager.WIFI_STATE_ENABLED:
                tvWifiState.setText("Wifi ON,Discovering...");
                break;
            case WifiManager.WIFI_STATE_DISABLED:
                tvWifiState.setText("Wifi OFF");
                break;
            case WifiManager.WIFI_STATE_DISABLING:
                tvWifiState.setText("Wifi Closing");
                break;
            case WifiManager.WIFI_STATE_ENABLING:
                tvWifiState.setText("Wifi Opening");
                break;
            case WifiManager.WIFI_STATE_UNKNOWN:
            default:
                tvWifiState.setText("Wifi state Unknown");
                break;
        }
    }

    private void wifiDeviceListChange(List<ScanResult> wifiDeviceList) {
        Log.d(TAG, "wifiDeviceListChange");
        if (wifiDeviceList == null) {
            return;
        }

        tvWifiDeviceList.setText("");
        for (ScanResult result : wifiDeviceList) {
            tvWifiDeviceList.append("device name: ");
            tvWifiDeviceList.append(result.SSID);
            tvWifiDeviceList.append("\nsignal level: ");
            tvWifiDeviceList.append(String.valueOf(result.level));
            tvWifiDeviceList.append("\n\n");
        }
    }
}
