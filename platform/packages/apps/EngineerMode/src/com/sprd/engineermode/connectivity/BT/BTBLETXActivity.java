package com.sprd.engineermode.connectivity.BT;
//bug567135 add by suyan.yang 2016-05-27
import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.view.WindowManager;
import android.widget.Spinner;
import android.widget.EditText;
import android.widget.Button;
import android.widget.TextView;
import android.view.View.OnClickListener;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.AdapterView.OnItemSelectedListener;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.util.Log;
import android.widget.Toast;

import com.sprd.engineermode.R;

public class BTBLETXActivity extends Activity implements OnClickListener, OnItemSelectedListener {

    private static final String TAG = "BTBLETXActivity";

    // handle message
    private static final int MSG_BT_BLE_TX_START = 0;
    private static final int MSG_BT_BLE_TX_STOP = 1;
    private static final int MSG_BT_BLE_OFF = 2;

    private static final String MaxPacLen="37";

    private Spinner mPattern;
    private EditText mChannel;
    private Spinner mPacType;
    private EditText mPacLen;
    private TextView mPacLenSummary;
    private Spinner mPowerType;
    private EditText mPowerValue;
    private EditText mPacCnt;
    private Button mStart;
    private Button mStop;
    private String mMaxPacLen = "0";

    private BTHelper.BTTX mBTBLETx;
    private BTTXHandler mBTBLETxHandler;
    private Handler mUiThread = new Handler();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.bt_ble_tx);
        getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);

        initUI();
        mBTBLETx = new BTHelper.BTTX();
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mBTBLETxHandler = new BTTXHandler(ht.getLooper());
    }

    @Override
    public void onBackPressed() {
        Message doMessage = null;
        // if BT TX is not stop, app shoule stop when click back button
        if (mStop.isEnabled()) {
            Log.d(TAG, "Stop is Enabled when click back");
            doMessage = mBTBLETxHandler.obtainMessage(MSG_BT_BLE_TX_STOP);
            mBTBLETxHandler.sendMessage(doMessage);
        }

        // close BT when click back button
        if (BTHelper.isBTOn) {
            Log.d(TAG, "BT is On when click back");
            doMessage = mBTBLETxHandler.obtainMessage(MSG_BT_BLE_OFF);
            mBTBLETxHandler.sendMessage(doMessage);
        } else {
            super.onBackPressed();
        }
    }

    @Override
    public void onClick(View v) {
        Message doMessage = null;
        // get the setting param
        if (!getSettingParam()) {
            return;
        }
        if (v == mStart) {
            doMessage = mBTBLETxHandler.obtainMessage(MSG_BT_BLE_TX_START);
        } else if (v == mStop) {
            doMessage = mBTBLETxHandler.obtainMessage(MSG_BT_BLE_TX_STOP);
        }
        mBTBLETxHandler.sendMessage(doMessage);
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position,
            long id) {
        if (mBTBLETx != null) {
            if (parent == mPacType) {
                mBTBLETx.pactype = this.getResources().getStringArray(R.array.bt_ble_pac_type_int)[position];
                //mMaxPacLen = this.getResources().getStringArray(R.array.bt_ble_pac_max_len)[position];
                mMaxPacLen = MaxPacLen;
                mPacLenSummary.setText("MaxLen is " + mMaxPacLen);
            } else if (parent == mPattern) {
                mBTBLETx.pattern = this.getResources().getStringArray(R.array.bt_ble_tx_pattern_int)[position];
            }else if (parent == mPowerType) {
                mBTBLETx.powertype = mPowerType.getSelectedItem().toString();
                mPowerValue.setText("");
                if (mBTBLETx.powertype.equals("0")) {
                    mPowerValue.setHint("0xFC8 0xFF8 0xFDA 0xFFC");
                } else {
                    mPowerValue.setHint("0~4");
                }
            }
        }
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    private void initUI() {
        mPattern = (Spinner) findViewById(R.id.bt_ble_tx_pattern);
        ArrayAdapter<String> patternAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, getResources()
                        .getStringArray(R.array.bt_ble_tx_pattern));
        patternAdapter
                .setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mPattern.setAdapter(patternAdapter);
        mPattern.setOnItemSelectedListener(this);

        mChannel = (EditText) findViewById(R.id.bt_ble_tx_channel);
        mChannel.addTextChangedListener(new TextWatcher() {
            @Override
            public void onTextChanged(CharSequence s, int start, int before,
                    int count) {
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count,
                    int after) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (mChannel.getText().length() != 0) {
                    if (Integer.parseInt(mChannel.getText().toString()) >= 0
                            && Integer.parseInt(mChannel.getText().toString()) <= 39) {
                        return;
                    } else {
                        Toast.makeText(BTBLETXActivity.this, "number between 0 and 39",
                                Toast.LENGTH_SHORT).show();
                        return;
                    }
                }
                /*
                 * if (mWifiTX != null) { mWifiTX.pktlength =
                 * mChannel.getText().toString(); }
                 */
            }
        });

        mPacType = (Spinner) findViewById(R.id.bt_ble_tx_pac_type);
        ArrayAdapter<String> pactype = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, getResources()
                        .getStringArray(R.array.bt_ble_pac_type));
        pactype
                .setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mPacType.setAdapter(pactype);
        mPacType.setOnItemSelectedListener(this);

        mPacLen = (EditText) findViewById(R.id.bt_ble_tx_pac_len);
        mPacLenSummary = (TextView) findViewById(R.id.bt_ble_tx_pac_len_summary);
        mPowerType = (Spinner) findViewById(R.id.bt_ble_tx_power_type);
        ArrayAdapter<String> powerTypeAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, getResources()
                        .getStringArray(R.array.bt_ble_tx_power_type));
        powerTypeAdapter
                .setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mPowerType.setAdapter(powerTypeAdapter);
        mPowerType.setOnItemSelectedListener(this);

        mPowerValue = (EditText) findViewById(R.id.bt_ble_tx_power_value);
        mPowerValue.addTextChangedListener(new TextWatcher() {
            @Override
            public void onTextChanged(CharSequence s, int start, int before,
                    int count) {
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count,
                    int after) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (mPowerValue.getText().length() != 0) {
                    if(mBTBLETx.powertype.equals("0")){
                        if(!(mPowerValue.getText().toString().equals("0xFC8") || mPowerValue.getText().toString().equals("0xFF8") ||
                                mPowerValue.getText().toString().equals("0xFDA") || mPowerValue.getText().toString().equals("0xFFC"))){
                            Toast.makeText(BTBLETXActivity.this, "proper number is 0xFC8 or 0xFF8 or 0xFDA or 0xFFC",
                                    Toast.LENGTH_SHORT).show();
                            return;
                        }
                    }else{
                        if (Integer.parseInt(mPowerValue.getText().toString()) < 0
                                || Integer.parseInt(mPowerValue.getText().toString()) > 4) {
                            Toast.makeText(BTBLETXActivity.this, "number between 0 and 4",
                                    Toast.LENGTH_SHORT).show();
                            return;
                        }
                    }
                }
                /*
                 * if (mWifiTX != null) { mWifiTX.pktlength =
                 * mChannel.getText().toString(); }
                 */
            }
        });

        mPacCnt = (EditText) findViewById(R.id.bt_ble_tx_pac_cnt);
        mPacCnt.addTextChangedListener(new TextWatcher() {
            @Override
            public void onTextChanged(CharSequence s, int start, int before,
                    int count) {
            }

            @Override
            public void beforeTextChanged(CharSequence s, int start, int count,
                    int after) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (mPacCnt.getText().length() != 0) {
                    if (Integer.parseInt(mPacCnt.getText().toString()) < 0
                            || Integer.parseInt(mPacCnt.getText().toString()) > 65535) {
                        Toast.makeText(BTBLETXActivity.this, "number between 0 and 65535",
                                Toast.LENGTH_SHORT).show();
                        return;
                    }
                }

                /*
                 * if (mWifiTX != null) { mWifiTX.pktlength =
                 * mChannel.getText().toString(); }
                 */
            }
        });

        mStart = (Button) findViewById(R.id.bt_ble_tx_start);
        mStart.setOnClickListener(this);
        mStop = (Button) findViewById(R.id.bt_ble_tx_stop);
        mStop.setOnClickListener(this);
        mStop.setEnabled(false);
    }

    private boolean getSettingParam() {

        if (mChannel.getText().length() == 0) {
            Toast.makeText(BTBLETXActivity.this, "please input BLE TX Channel",
                    Toast.LENGTH_SHORT).show();
            return false;
        } else {
            mBTBLETx.channel = mChannel.getText().toString();
        }

        if (mPacLen.getText().length() == 0) {
            Toast.makeText(BTBLETXActivity.this, "please input BLE TX Pac Len",
                    Toast.LENGTH_SHORT).show();
            return false;
        } else {
            if (Integer.parseInt(mPacLen.getText().toString()) > Integer.parseInt(mMaxPacLen)) {
                mBTBLETx.paclen = mMaxPacLen;
            } else {
                mBTBLETx.paclen = mPacLen.getText().toString();
            }
        }

        //mBTBLETx.powertype = mPowerType.getSelectedItem().toString();

        if (mPowerValue.getText().length() == 0) {
            Toast.makeText(BTBLETXActivity.this, "please input BLE TX Power Value",
                    Toast.LENGTH_SHORT).show();
            return false;
        } else {
            mBTBLETx.powervalue = mPowerValue.getText().toString();
        }

        if (mPacCnt.getText().length() == 0) {
            Toast.makeText(BTBLETXActivity.this, "please input BLE TX Pac Cnt",
                    Toast.LENGTH_SHORT).show();
            return false;
        } else {
            mBTBLETx.paccnt = mPacCnt.getText().toString();
        }
        Log.d(TAG, "Now BT BLE TX Start" + "\n" + "BLE TX Pattern is " + mBTBLETx.pattern + "\n"
                + "BLE TX Channel is " + mBTBLETx.channel + "\n"
                + "BLE TX Pac Type is " + mBTBLETx.pactype + "\n" + "BLE TX Pac Len is " + mBTBLETx.paclen + "\n"
                + "BLE TX Power Type is " + mBTBLETx.powertype + "\n" + "BLE TX Power Value is "
                + mBTBLETx.powervalue + "\n"
                + "BLE TX Pac Cnt is " + mBTBLETx.paccnt);
        return true;
    }

    class BTTXHandler extends Handler {
        public BTTXHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_BT_BLE_TX_START:
                    if (BTHelper.BTBLETxStart(mBTBLETx)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(BTBLETXActivity.this, "BLE TX Start Success",
                                        Toast.LENGTH_SHORT).show();
                                mStart.setEnabled(false);
                                mStop.setEnabled(true);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(BTBLETXActivity.this, "BLE TX Start Fail",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    }
                    break;
                case MSG_BT_BLE_TX_STOP:
                    if (BTHelper.BTBLETxStop(mBTBLETx)) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(BTBLETXActivity.this, "BLE TX Stop Success",
                                        Toast.LENGTH_SHORT).show();
                                mStart.setEnabled(true);
                                mStop.setEnabled(false);
                            }
                        });
                    } else {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                Toast.makeText(BTBLETXActivity.this, "BLE TX Stop Fail",
                                        Toast.LENGTH_SHORT).show();
                            }
                        });
                    }
                    break;
                case MSG_BT_BLE_OFF:
                    if (BTHelper.BTOff()) {
                        Toast.makeText(BTBLETXActivity.this, "BT Off Success",
                                Toast.LENGTH_SHORT).show();
                    } else {
                        Toast.makeText(BTBLETXActivity.this, "BT Off Fail",
                                Toast.LENGTH_SHORT).show();
                    }
                    BTHelper.closeSocket();
                    finish();
                    break;
                default:
                    break;
            }
        }
    }

}
