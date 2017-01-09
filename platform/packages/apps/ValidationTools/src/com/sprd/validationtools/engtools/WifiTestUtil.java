
package com.sprd.validationtools.engtools;

import java.util.List;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.SystemClock;
import android.util.Log;

public class WifiTestUtil {
    private static String TAG = "WifiTestUtil";

    private static final int DELAY_TIME = 5000;
    private WifiManager mWifiManager = null;
    private Context mContext = null;
    private int mLastCount = 0;
    private WifiStateChangeReceiver mWifiStateChangeReceiver = null;
    private WifiScanReceiver mWifiScanReceiver = null;
    private StartScanThread mStartScanThread = null;

    public WifiTestUtil(WifiManager wifiManager) {
        mWifiManager = wifiManager;
    }

    private void registerAllReceiver() {
        mWifiStateChangeReceiver = new WifiStateChangeReceiver();
        String filterFlag1 = WifiManager.WIFI_STATE_CHANGED_ACTION;
        IntentFilter filter1 = new IntentFilter(filterFlag1);
        mContext.registerReceiver(mWifiStateChangeReceiver, filter1);

        mWifiScanReceiver = new WifiScanReceiver();
        String filterFlag2 = WifiManager.SCAN_RESULTS_AVAILABLE_ACTION;
        IntentFilter filter2 = new IntentFilter(filterFlag2);
        mContext.registerReceiver(mWifiScanReceiver, filter2);
    }

    private void unregisterAllReceiver() {
        // release wifi enabled receiver
        if (mWifiStateChangeReceiver != null) {
            mContext.unregisterReceiver(mWifiStateChangeReceiver);
            mWifiStateChangeReceiver = null;
        }

        // release wifi scan receiver
        if (mWifiScanReceiver != null) {
            mContext.unregisterReceiver(mWifiScanReceiver);
            mWifiScanReceiver = null;
        }

        mContext = null;
    }

    public WifiManager getWifiManager() {
        return mWifiManager;
    }

    public void startTest(Context context) {
        mContext = context;
        registerAllReceiver();
        if (mWifiManager.isWifiEnabled()) {
            wifiStateChange(WifiManager.WIFI_STATE_ENABLED);
            wifiStartDiscovery();
        } else {
            wifiStateChange(WifiManager.WIFI_STATE_DISABLED);
            mWifiManager.setWifiEnabled(true);
        }
    }

    public void stopTest() {
        // mWifiManager.cancelDiscovery();
        unregisterAllReceiver();
        mWifiManager.setWifiEnabled(false);
    }

    private void wifiStartDiscovery() {
        if (mWifiManager != null) {
            mStartScanThread = new StartScanThread();
            mStartScanThread.start();
            Log.w(TAG, "============startDiscovery===============");
        }
    }

    public void wifiStateChange(int newState) {
        // for override
    }

    public void wifiDeviceListChange(List<ScanResult> wifiDeviceList) {
        // for override
    }

    public void wifiDiscoveryFinished() {
        // for override
    }

    private class WifiStateChangeReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            int newState = mWifiManager.getWifiState();
            Log.d(TAG, "" + newState);
            switch (newState) {
                case WifiManager.WIFI_STATE_ENABLED:
                    wifiStartDiscovery();
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
        }
    }

    private class WifiScanReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (action.equals(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION)) {
                List<ScanResult> wifiScanResultList = mWifiManager.getScanResults();

                if (wifiScanResultList != null
                        && wifiScanResultList.size() != mLastCount) {
                    wifiDeviceListChange(wifiScanResultList);

                    mLastCount = wifiScanResultList.size();
                }
            }
        }
    }

    class StartScanThread extends Thread {
        @Override
        public void run() {
            try {
                // wait until other actions finish.
                SystemClock.sleep(DELAY_TIME);
                mWifiManager.startScan();
            } catch (Exception e) {
                // do nothing
            }
        }
    }
}
