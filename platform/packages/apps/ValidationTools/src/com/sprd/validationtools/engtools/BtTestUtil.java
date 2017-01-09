
package com.sprd.validationtools.engtools;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

public class BtTestUtil {
    private static String TAG = "BtTestUtil";
    private BluetoothAdapter mBluetoothAdapter = null;
    private BluetoothDiscoveryReceiver btDiscoveryReceiver = null;
    private BlueToothStateReceiver btStateReceiver = null;
    private Context mContext = null;

    public BtTestUtil() {
        
	Log.w(TAG, "=btt `");
	mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    private void registerAllReceiver() {
        // register receiver for bt search
        IntentFilter intent = new IntentFilter();
        intent.addAction(BluetoothDevice.ACTION_FOUND);
        intent.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
        btDiscoveryReceiver = new BluetoothDiscoveryReceiver();
        mContext.registerReceiver(btDiscoveryReceiver, intent);
        // register reveiver for bt state change
        btStateReceiver = new BlueToothStateReceiver();
        IntentFilter filter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
        mContext.registerReceiver(btStateReceiver, filter);
    }

    private void unregisterAllReceiver() {
        if (btDiscoveryReceiver != null) {
            mContext.unregisterReceiver(btDiscoveryReceiver);
            btDiscoveryReceiver = null;
        }
        if (btStateReceiver != null) {
            mContext.unregisterReceiver(btStateReceiver);
            btStateReceiver = null;
        }
        mContext = null;
    }

    public BluetoothAdapter getBluetoothAdapter() {
        return mBluetoothAdapter;
    }

    public void startTest(Context context) {
        mContext = context;
        registerAllReceiver();
        if (mBluetoothAdapter.isEnabled()) {
            btStateChange(BluetoothAdapter.STATE_ON);
            btPageScan();
            btStartDiscovery();
        } else {
            btStateChange(BluetoothAdapter.STATE_OFF);
            mBluetoothAdapter.enable();
        }
    }

    public void stopTest() {
        mBluetoothAdapter.cancelDiscovery();
        mBluetoothAdapter.disable();
        unregisterAllReceiver();
    }

    private void btStartDiscovery() {
        if (mBluetoothAdapter != null) {
            mBluetoothAdapter.startDiscovery();
            Log.w(TAG, "============startDiscovery===============");
        }
    }

    public void btStateChange(int newState) {
        // for override
    }

    public void btPageScan() {
    /* bt should send scan enable command to accommodate CP2 */
        if (mBluetoothAdapter != null) {
            mBluetoothAdapter.setScanMode(BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE);
            Log.w(TAG, "============btPageScan SCAN_MODE_CONNECTABLE_DISCOVERABLE===============");
        }
    }


    public void btDeviceListAdd(BluetoothDevice newDevice) {
        // for override
    }

    public void btDiscoveryFinished() {
        // for override
    }

    private class BlueToothStateReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.w(TAG, "BlueToothStateReceiver");
            int newState = mBluetoothAdapter.getState();
            switch (newState) {
                case BluetoothAdapter.STATE_ON:
                    btPageScan();
                    btStartDiscovery();
                    break;
                case BluetoothAdapter.STATE_TURNING_OFF:
                    break;
                case BluetoothAdapter.STATE_OFF:
                    mBluetoothAdapter.enable();
                    break;
                case BluetoothAdapter.STATE_TURNING_ON:
                    break;
                default:
                    // do nothing
            }

            btStateChange(newState);
        }
    }

    private class BluetoothDiscoveryReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (BluetoothDevice.ACTION_FOUND.equals(action)) {
                Log.v(TAG, "found bluetooth device");
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);

                if (device != null) {
                    if (device.getBondState() != BluetoothDevice.BOND_BONDED) {
                        btDeviceListAdd(device);
                    }
                } else {
                    Log.w(TAG, "not find any device");
                }
            } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(action)) {
                Log.v(TAG, "=============discovery finished !");
                btDiscoveryFinished();
            }
        }
    }
}
