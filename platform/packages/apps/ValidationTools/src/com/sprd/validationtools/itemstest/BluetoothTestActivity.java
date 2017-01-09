
package com.sprd.validationtools.itemstest;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;
import com.sprd.validationtools.engtools.BtTestUtil;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;
import android.os.Debug;

public class BluetoothTestActivity extends BaseActivity {
    private static final String TAG = "BluetoothTestActivity";

    private List<BluetoothDevice> mBluetoothDeviceList = new ArrayList<BluetoothDevice>();

    private TextView tvBtAddr = null;
    private TextView tvBtState = null;
    private TextView tvBtDeviceList = null;

    private BtTestUtil btTestUtil = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.w(TAG, "+++++++enter bt+++++++++");
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        setContentView(R.layout.bluetooth_result);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);

        btTestUtil = new BtTestUtil() {

            public void btStateChange(int newState) {
                switch (newState) {
                    case BluetoothAdapter.STATE_ON:
                        tvBtState.setText("Bluetooth ON,Discovering...");
                        // SPRD: update bluetooth address when bt power on
                        tvBtAddr.setText(btTestUtil.getBluetoothAdapter().getAddress() + "\n");
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

            public void btDeviceListAdd(BluetoothDevice device) {

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

            public void btDiscoveryFinished() {
                if (mBluetoothDeviceList != null
                        && mBluetoothDeviceList.size() > 0) {
                    Toast.makeText(BluetoothTestActivity.this, R.string.text_pass,
                            Toast.LENGTH_SHORT).show();
                    storeRusult(true);

                } else {
                    Toast.makeText(BluetoothTestActivity.this, R.string.text_fail,
                            Toast.LENGTH_SHORT).show();
                    storeRusult(false);

                }
                btTestUtil.stopTest();
                finish();
            }
        };

        tvBtAddr = (TextView) findViewById(R.id.bt_addr_content);
        tvBtState = (TextView) findViewById(R.id.bt_state_content);
        tvBtDeviceList = (TextView) findViewById(R.id.tv_bt_device_list);

        tvBtAddr.setText(btTestUtil.getBluetoothAdapter().getAddress() + "\n");
    }

    public void onClick(View v){
        btTestUtil.stopTest();
        super.onClick(v);
    }

    @Override
    protected void onResume() {
        super.onResume();
        btTestUtil.startTest(this);
    }

    /*SPRD: fix bug408662 stop bluetooth rest on pause @{ */
    @Override
    protected void onPause() {
        super.onPause();
        btTestUtil.stopTest();
    }
    /* @}*/

    public void onBackPressed(){
        btTestUtil.stopTest();
        super.onBackPressed();
    }
}


