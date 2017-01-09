
package com.sprd.validationtools.itemstest;

import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.PhaseCheckParse;
import com.sprd.validationtools.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.os.BatteryManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.os.Debug;

public class ChargerTest extends BaseActivity {

    private static final String TAG = "ChargerTest";
    private static final String INPUT_ELECTRONIC = "/sys/class/power_supply/battery/real_time_current";
    private static final String CHARGER_ELECTRONIC = "/sys/class/power_supply/sprdfgu/fgu_current";
    private static final String ENG_CHARGER_VOL = "/sys/class/power_supply/battery/charger_voltage";
    private static final String ENG_BATTERY_TVOL = "/sys/class/power_supply/battery/real_time_voltage";
    private static final String STATUS = "status";
    private static final String PLUGGED = "plugged";
    private static final String VOLTAGE = "voltage";
    private static final String EXT_CHARGE_IC = "ext charge ic";
    private static final String TEST_RESULT_SUCCESS = "success";
    private static final String TEST_RESULT_FAIL = "fail";

    private TextView statusTextView, pluggedTextView, voltageTextView, mElectronicTextView,
            mBatteryTextView, mTestResultTextView;
    private PhaseCheckParse mParse;
    private String mPluggeString = null;
    private boolean mIsPlugUSB = false;
    private float mChargerElectronic;
    private float mChargerVoltage;
    private String mInputCurrent = null;
    private int mRetryNum = 0;
    private int mWaitTime = 1000;

    private Handler mHandler;
    private Runnable mR = new Runnable() {
        public void run() {
            showResultDialog(getString(R.string.charger_info));
        }
    };

    private Runnable mElectronicUpdate = new Runnable() {
        public void run() {

            String testResult = getInputElectronic();
            Log.d(TAG, "mElectronicUpdate data,mChargerVoltage=" + mChargerVoltage
                    + ", mChargerElectronic=" + mChargerElectronic + ",testResult:" + testResult);

            if (TEST_RESULT_SUCCESS.equals(testResult)) {

                if (mIsPlugUSB) {
                    mTestResultTextView.setText(getString(R.string.charger_test_success));
                    mTestResultTextView.setTextColor(Color.GREEN);
                    storeRusult(true);
                    mHandler.postDelayed(mCompleteTest, 500);
                } else {
                    mHandler.postDelayed(mElectronicUpdate, 1000);
                }

            } else {

                if (mIsPlugUSB) {
                    mRetryNum++;
                    if (mRetryNum <= 5) {
                        mWaitTime = 500 * mRetryNum;

                        mHandler.post(mElectronicUpdate);
                        Log.d(TAG, "retry test num:" + mRetryNum + ",wait time is " + mWaitTime);
                    } else {
                        mTestResultTextView.setText(getString(R.string.charger_test_fail));
                        mTestResultTextView.setTextColor(Color.RED);
                        storeRusult(false);
                        mHandler.postDelayed(mCompleteTest, 500);
                    }
                } else {
                    mHandler.postDelayed(mElectronicUpdate, 1000);
                }
            }
        }
    };

    private Runnable mCompleteTest = new Runnable() {
        public void run() {
            finish();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setTitle(R.string.battery_title_text);
        setContentView(R.layout.battery_charged_result);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
        mParse = new PhaseCheckParse();
        mHandler = new Handler();
        statusTextView = (TextView) findViewById(R.id.statusTextView);
        pluggedTextView = (TextView) findViewById(R.id.pluggedTextView);
        voltageTextView = (TextView) findViewById(R.id.voltageTextView);
        mElectronicTextView = (TextView) findViewById(R.id.electronicTextView);
        mBatteryTextView = (TextView) findViewById(R.id.batteryelectronicTextView);
        mTestResultTextView = (TextView) findViewById(R.id.test_resultTextView);

        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_BATTERY_CHANGED);
        registerReceiver(mBroadcastReceiver, filter);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mHandler.postDelayed(mElectronicUpdate, 500);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mHandler.removeCallbacks(mElectronicUpdate);
    }

    private String getInputElectronic() {
        float batteryElectronic = (float) -1.0;
        String result = "";
        mInputCurrent = readFile(INPUT_ELECTRONIC).trim();
        if (mIsPlugUSB && "discharging".equals(mInputCurrent)) {
            startCharge();
            mInputCurrent = readFile(INPUT_ELECTRONIC).trim();
        }
        Log.d(TAG, "inputCurrent[" + mInputCurrent + "]");
        try {

            initView();
            if (EXT_CHARGE_IC.equals(mInputCurrent)) {
                int c1 = Integer.parseInt(readFile(CHARGER_ELECTRONIC).trim());
                if (c1 > -40) {
                    return TEST_RESULT_SUCCESS;
                }
                stopCharge();
                Thread.sleep(1000);
                int c2 = Integer.parseInt(readFile(CHARGER_ELECTRONIC).trim());
                startCharge();
                Thread.sleep(mWaitTime);
                int c3 = Integer.parseInt(readFile(CHARGER_ELECTRONIC).trim());
                int i = c1 - c2;
                int i1 = c3 - c2;
                if (c1 > -40 || c3 > -40 || (i > 300 && i1 > 300)) {
                    result = TEST_RESULT_SUCCESS;
                } else {
                    result = TEST_RESULT_FAIL;
                }
                mBatteryTextView.setText(c3 + " ma");
                Log.d(TAG, "getInputElectronic() c1:" + c1 + " c2:" + c2 + " c3:" + c3 + " i:" + i
                        + " i1:" + i1);
            } else {

                if (!isNum(mInputCurrent)) {
                    Log.d(TAG, "get values isn`t number.");
                    return mInputCurrent;
                }

                if (mChargerElectronic > 200) {
                    result = TEST_RESULT_SUCCESS;
                } else {
                    if (mChargerVoltage > 4150) {
                        result = TEST_RESULT_SUCCESS;
                    } else {
                        result = TEST_RESULT_FAIL;
                    }
                }

            }
        } catch (Exception e) {
            Log.w(TAG, "getInputElectronic fail", e);
        }
        return result;
    }

    private void initView() {

        try {
            Thread.sleep(mWaitTime);
        } catch (Exception e) {
            e.printStackTrace();
        }
        mChargerElectronic = getDateFromNode(CHARGER_ELECTRONIC);
        mChargerVoltage = getDateFromNode(ENG_CHARGER_VOL);

        if (mChargerElectronic > -40.0 && mIsPlugUSB) {
            mBatteryTextView.setText(mChargerElectronic + " ma");
        } else {
            mBatteryTextView.setText("n/a");
        }

        // General power of the test will have an initial value of 40mv.
        // Unfriendly so set a value greater than 100 and must plug usb or
        // ac
        if (mChargerVoltage >= 100.0 && mIsPlugUSB) {
            mElectronicTextView.setText(mChargerVoltage + " mv");
        } else {
            mElectronicTextView.setText("n/a");
        }
    }

    public boolean isNum(String str) {
        return str.matches("^[-+]?(([0-9]+)([.]([0-9]+))?|([.]([0-9]+))?)$");
    }

    private float getDateFromNode(String nodeString) {
        char[] buffer = new char[1024];
        // Set a special value -100, to distinguish mChargerElectronic greater
        // than -40.
        float batteryElectronic = -100;
        FileReader file = null;
        try {
            file = new FileReader(nodeString);
            int len = file.read(buffer, 0, 1024);
            batteryElectronic = Float.valueOf((new String(buffer, 0, len)));
            if (file != null) {
                file.close();
                file = null;
            }
        } catch (Exception e) {
            try {
                if (file != null) {
                    file.close();
                    file = null;
                }
            } catch (IOException io) {
                Log.e(TAG, "getDateFromNode fail , nodeString is:" + nodeString);
            }
        }
        return batteryElectronic;
    }

    private BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_BATTERY_CHANGED)) {
                int status = intent.getIntExtra(STATUS, 0);
                int plugged = intent.getIntExtra(PLUGGED, 0);
                int voltage = intent.getIntExtra(VOLTAGE, 0);
                String statusString = "";
                switch (status) {
                    case BatteryManager.BATTERY_STATUS_UNKNOWN:
                        statusString = getResources().getString(R.string.charger_unknown);
                        break;
                    case BatteryManager.BATTERY_STATUS_CHARGING:
                        statusString = getResources().getString(R.string.charger_charging);
                        // mHandler.postDelayed(mR, 1000);
                        break;
                    case BatteryManager.BATTERY_STATUS_DISCHARGING:
                        statusString = getResources().getString(R.string.charger_discharging);
                        break;
                    case BatteryManager.BATTERY_STATUS_NOT_CHARGING:
                        statusString = getResources().getString(R.string.charger_not_charging);
                        break;
                    case BatteryManager.BATTERY_STATUS_FULL:
                        statusString = getResources().getString(R.string.charger_full);
                        break;
                    default:
                        break;
                }
                switch (plugged) {
                    case BatteryManager.BATTERY_PLUGGED_AC:
                        mIsPlugUSB = true;
                        mPluggeString = getResources().getString(R.string.charger_ac_plugged);
                        mTestResultTextView.setVisibility(View.VISIBLE);
                        break;
                    case BatteryManager.BATTERY_PLUGGED_USB:
                        mIsPlugUSB = true;
                        mPluggeString = getResources().getString(R.string.charger_usb_plugged);
                        mTestResultTextView.setVisibility(View.VISIBLE);
                        break;
                    default:
                        mIsPlugUSB = false;
                        mPluggeString = getResources().getString(R.string.charger_no_plugged);
                        mTestResultTextView.setText(getString(R.string.charging_test));
                        mTestResultTextView.setTextColor(Color.WHITE);
                        mTestResultTextView.setVisibility(View.GONE);
                        // Prevent unplug the usb cable is still charging
                        // status.
                        if (statusString.equals(getString(R.string.charger_charging))) {
                            statusString = getResources().getString(R.string.charger_discharging);
                            Log.d(TAG, "Correct the error displays charge status.");
                        }
                        break;
                }

                statusTextView.setText(statusString);
                pluggedTextView.setText(mPluggeString);
                voltageTextView.setText(Integer.toString(voltage) + " mv");
                Log.v(STATUS, statusString);
                Log.v(PLUGGED, mPluggeString);
            }
        }
    };

    @Override
    public void onDestroy() {
        unregisterReceiver(mBroadcastReceiver);
        mHandler.removeCallbacks(mR);
        mHandler.removeCallbacks(mCompleteTest);
        // stopCharge();
        super.onDestroy();
    }

    private String readFile(String path) {
        char[] buffer = new char[1024];
        String batteryElectronic = "";
        FileReader file = null;
        try {
            file = new FileReader(path);
            int len = file.read(buffer, 0, 1024);
            batteryElectronic = new String(buffer, 0, len);
        } catch (Exception e) {
            Log.w(TAG, "read fail:" + e);
        } finally {
            try {
                if (file != null) {
                    file.close();
                    file = null;
                }
            } catch (IOException io) {
                Log.w(TAG, "read file close fail");
            }
        }
        return batteryElectronic;
    }

    private void stopCharge() {
        mParse.writeChargeSwitch(1);
    }

    private void startCharge() {
        mParse.writeChargeSwitch(0);
    }
}
