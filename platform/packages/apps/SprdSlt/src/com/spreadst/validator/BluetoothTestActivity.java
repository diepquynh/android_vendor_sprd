package com.spreadst.validator;

import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.widget.Toast;

public class BluetoothTestActivity extends Activity{
    private static final String TAG = "BluetoothTestActivity";

    private TextView tvBtAddr = null;

    private TextView tvBtState = null;

    private TextView tvBtDeviceList = null;

    private BluetoothAdapter mBluetoothAdapter = null;

    private BluetoothDiscoveryReceiver btDiscoveryReceiver = null;

    private BlueToothStateReceiver btStateReceiver = null;

    private List<BluetoothDevice> mBluetoothDeviceList = new ArrayList<BluetoothDevice>();

    private class BlueToothStateReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "BlueToothStateReceiver");
            int newState = mBluetoothAdapter.getState();
            switch (newState) {
                case BluetoothAdapter.STATE_ON:
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
                Log.d(TAG, "found bluetooth device");
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);

                if (device != null) {
                    if (device.getBondState() != BluetoothDevice.BOND_BONDED) {
                        btDeviceListAdd(device);
                    }
                } else {
                    Log.w(TAG, "not find any device");
                }
            } else if (BluetoothAdapter.ACTION_DISCOVERY_FINISHED.equals(action)) {
                Log.d(TAG, "discovery finished !");
                btDiscoveryFinished();
            }
        }
    }

    private void registerAllReceiver() {
        Log.d(TAG, "registerAllReceiver");
        // register receiver for bt search
        IntentFilter intent = new IntentFilter();
        intent.addAction(BluetoothDevice.ACTION_FOUND);
        intent.addAction(BluetoothAdapter.ACTION_DISCOVERY_FINISHED);
        btDiscoveryReceiver = new BluetoothDiscoveryReceiver();
        registerReceiver(btDiscoveryReceiver, intent);
        // register reveiver for bt state change
        btStateReceiver = new BlueToothStateReceiver();
        IntentFilter filter = new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED);
        registerReceiver(btStateReceiver, filter);
    }

    private void unregisterAllReceiver() {
        Log.d(TAG, "unregisterAllReceiver");
        if (btDiscoveryReceiver != null) {
            unregisterReceiver(btDiscoveryReceiver);
        }
        if (btStateReceiver != null) {
            unregisterReceiver(btStateReceiver);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");

        setContentView(R.layout.bluetooth_scan_view);
        tvBtAddr = (TextView) findViewById(R.id.bt_addr_content);
        tvBtState = (TextView) findViewById(R.id.bt_state_content);
        tvBtDeviceList = (TextView) findViewById(R.id.tv_bt_device_list);

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();

        tvBtAddr.setText(mBluetoothAdapter.getAddress() + "\n");

        registerAllReceiver();
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
        startBluetoothDiscovery();
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy");
        mBluetoothAdapter.cancelDiscovery();
        mBluetoothAdapter.disable();
        unregisterAllReceiver();
        super.onDestroy();
    }

    private void startBluetoothDiscovery() {
        Log.d(TAG, "startBluetoothDiscovery");
        if (mBluetoothAdapter.isEnabled()) {
            btStateChange(BluetoothAdapter.STATE_ON);
            btStartDiscovery();
        } else {
            btStateChange(BluetoothAdapter.STATE_OFF);
            boolean open = mBluetoothAdapter.enable();
            if (!open) {
                Log.d(TAG, "open Bluetooth fail");
                Toast.makeText(BluetoothTestActivity.this, R.string.open_bt_fail,
                        Toast.LENGTH_SHORT).show();
                finish();
            }
        }
    }

    private void btStateChange(int newState) {
        Log.d(TAG, "btStateChange");
        switch (newState) {
            case BluetoothAdapter.STATE_ON:
                tvBtState.setText("Bluetooth ON,Discovering...");
                break;
            case BluetoothAdapter.STATE_TURNING_OFF:
                tvBtState.setText("Bluetooth Closing");
                break;
            case BluetoothAdapter.STATE_OFF:
                tvBtState.setText("Bluetooth OFF");
                break;
            case BluetoothAdapter.STATE_TURNING_ON:
                tvBtState.setText("Bluetooth Opening...");
                break;
            default:
                tvBtState.setText("Bluetooth state Unknown");
                break;
        }
    }

    private void btDeviceListAdd(BluetoothDevice device) {
        Log.d(TAG, "btDeviceListAdd");
        if (mBluetoothDeviceList.contains(device)) {
            return;
        }

        if (device != null) {
            mBluetoothDeviceList.add(device);
            if (device.getBondState() != BluetoothDevice.BOND_BONDED) {
                String name = device.getName();
                if (name == null || name.isEmpty()) {
                    return;
                }
                StringBuffer deviceInfo = new StringBuffer();
                deviceInfo.append("device name: ");
                deviceInfo.append(name);
                deviceInfo.append("\n");
                Log.w(TAG, "======find bluetooth device => name : " + name
                        + "\n address :" + device.getAddress());
                tvBtDeviceList.append(deviceInfo.toString());
            }
        }
    }

    private void btDiscoveryFinished() {
        Log.d(TAG, "btDiscoveryFinished");
        if (mBluetoothDeviceList != null
                && mBluetoothDeviceList.size() > 0) {
            Toast.makeText(BluetoothTestActivity.this, R.string.text_pass,
                    Toast.LENGTH_SHORT).show();
            finish();
        } else {
            Toast.makeText(BluetoothTestActivity.this, R.string.text_fail,
                    Toast.LENGTH_SHORT).show();
            finish();
        }
    }

    private void btStartDiscovery() {
        if (mBluetoothAdapter != null) {
            mBluetoothAdapter.startDiscovery();
            Log.d(TAG, "============btStartDiscovery===============");
        }
    }
}
