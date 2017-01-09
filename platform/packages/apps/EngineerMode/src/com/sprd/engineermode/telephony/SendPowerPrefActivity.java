
package com.sprd.engineermode.telephony;

import java.util.HashMap;
import android.os.PowerManager.WakeLock;
import android.os.PowerManager;
import java.util.Iterator;
import java.util.Set;
import java.util.Map.Entry;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.content.Context;
import android.util.AttributeSet;
import android.preference.Preference;
import android.preference.EditTextPreference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.widget.Toast;
import android.widget.EditText;
import android.widget.Button;
import android.view.View;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import java.util.Arrays;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import android.os.SystemProperties;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;

public class SendPowerPrefActivity extends Activity {
    private static final String TAG = "SendPowerPrefActivity";

    private static final int SET_GSM850_MAX_POWER = 0;
    private static final int CLEAR_GSM850_MAX_POWER = 1;
    private static final int SET_EGSM900_MAX_POWER = 2;
    private static final int CLEAR_EGSM900_MAX_POWER = 3;
    private static final int SET_DCS1800_MAX_POWER = 4;
    private static final int CLEAR_DCS1800_MAX_POWER = 5;
    private static final int SET_PCS1900_MAX_POWER = 6;
    private static final int CLEAR_PCS1900_MAX_POWER = 7;
    private static final int SET_TD19_MAX_POWER = 8;
    private static final int CLEAR_TD19_MAX_POWER = 9;
    private static final int SET_TD21_MAX_POWER = 10;
    private static final int CLEAR_TD21_MAX_POWER = 11;
    private static final int SET_WBand1_MAX_POWER = 12;
    private static final int CLEAR_WBand1_MAX_POWER = 13;
    private static final int SET_WBand2_MAX_POWER = 14;
    private static final int CLEAR_WBand2_MAX_POWER = 15;
    private static final int SET_WBand5_MAX_POWER = 16;
    private static final int CLEAR_WBand5_MAX_POWER = 17;
    private static final int SET_WBand8_MAX_POWER = 18;
    private static final int CLEAR_WBand8_MAX_POWER = 19;

    private EditText mGsm850 = null;
    private EditText mEgsm900 = null;
    private EditText mDcs1800 = null;
    private EditText mPcs1900 = null;
    private EditText mTD19 = null;
    private EditText mTD21 = null;
    private EditText mWBand1 = null;
    private EditText mWBand2 = null;
    private EditText mWBand5 = null;
    private EditText mWBand8 = null;
    private Handler mSetMaxPowerHandler = null;
    private Handler mUiHandler = new Handler();

    private boolean isSupportTD = false;
    private boolean isSupportWCD = false;

    private final static String ORI_WCDMA_BAND = "ORI_WCDMA_BAND";
    private static final int[] mWBandMap = {
            1, 2, 5, 8
    };
    private static final int[] mWBandET = {
            R.id.et_wband1, R.id.et_wband2,
            R.id.et_wband5, R.id.et_wband8
    };
    private static final int[] mWBandOK = {
            R.id.ok_wband1, R.id.ok_wband2,
            R.id.ok_wband5, R.id.ok_wband8
    };
    private static final int[] mWBandClear = {
            R.id.clear_wband1, R.id.clear_wband2,
            R.id.clear_wband5, R.id.clear_wband8
    };
    private static final int[] mWBandText = {
            R.id.text_wband1, R.id.text_wband2,
            R.id.text_wband5, R.id.text_wband8
    };

    private int mWCDMASupportBand = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.send_power_set);

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mSetMaxPowerHandler = new SetMaxPowerHandler(ht.getLooper());

        mGsm850 = (EditText) findViewById(R.id.et_gsm850);
        mEgsm900 = (EditText) findViewById(R.id.et_egsm900);
        mDcs1800 = (EditText) findViewById(R.id.et_dcs1800);
        mPcs1900 = (EditText) findViewById(R.id.et_pcs1900);
        mTD19 = (EditText) findViewById(R.id.et_td19);
        mTD21 = (EditText) findViewById(R.id.et_td21);
        mWBand1 = (EditText) findViewById(R.id.et_wband1);
        mWBand2 = (EditText) findViewById(R.id.et_wband2);
        mWBand5 = (EditText) findViewById(R.id.et_wband5);
        mWBand8 = (EditText) findViewById(R.id.et_wband8);
        TelephonyManagerSprd tm = new TelephonyManagerSprd(SendPowerPrefActivity.this);
        isSupportTD = TelephonyManagerSprd.RadioCapbility.TDD_CSFB
                .equals(TelephonyManagerSprd.getRadioCapbility())
                || (TelephonyManagerSprd.RadioCapbility.CSFB.equals(TelephonyManagerSprd
                        .getRadioCapbility())
                && tm.getPreferredNetworkType() == TelephonyManagerSprd.NT_TD_LTE_TDSCDMA_GSM);
        isSupportWCD = TelephonyManagerSprd.RadioCapbility.FDD_CSFB
                .equals(TelephonyManagerSprd
                        .getRadioCapbility())
                || TelephonyManagerSprd.RadioCapbility.CSFB.equals(TelephonyManagerSprd
                        .getRadioCapbility());
        Button buttonOk = null;
        buttonOk = (Button) findViewById(R.id.ok_gsm850);
        buttonOk.setText("Set");
        buttonOk.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mGsm850.getText().toString().equals("")) {
                    Toast.makeText(SendPowerPrefActivity.this, "please input data",
                            Toast.LENGTH_SHORT)
                            .show();
                } else {
                    Message msg = mSetMaxPowerHandler.obtainMessage(SET_GSM850_MAX_POWER,
                            Integer.valueOf(mGsm850.getText().toString()), 0);
                    mSetMaxPowerHandler.sendMessage(msg);
                }
            }
        });
        buttonOk = (Button) findViewById(R.id.ok_egsm900);
        buttonOk.setText("Set");
        buttonOk.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mEgsm900.getText().toString().equals("")) {
                    Toast.makeText(SendPowerPrefActivity.this, "please input data",
                            Toast.LENGTH_SHORT)
                            .show();
                } else {
                    Message msg = mSetMaxPowerHandler.obtainMessage(SET_EGSM900_MAX_POWER,
                            Integer.valueOf(mEgsm900.getText().toString()), 0);
                    mSetMaxPowerHandler.sendMessage(msg);
                }
            }
        });
        buttonOk = (Button) findViewById(R.id.ok_dcs1800);
        buttonOk.setText("Set");
        buttonOk.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mDcs1800.getText().toString().equals("")) {
                    Toast.makeText(SendPowerPrefActivity.this, "please input data",
                            Toast.LENGTH_SHORT)
                            .show();
                } else {
                    Message msg = mSetMaxPowerHandler.obtainMessage(SET_DCS1800_MAX_POWER,
                            Integer.valueOf(mDcs1800.getText().toString()), 0);
                    mSetMaxPowerHandler.sendMessage(msg);
                }
            }
        });
        buttonOk = (Button) findViewById(R.id.ok_pcs1900);
        buttonOk.setText("Set");
        buttonOk.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mPcs1900.getText().toString().equals("")) {
                    Toast.makeText(SendPowerPrefActivity.this, "please input data",
                            Toast.LENGTH_SHORT)
                            .show();
                } else {
                    Message msg = mSetMaxPowerHandler.obtainMessage(SET_PCS1900_MAX_POWER,
                            Integer.valueOf(mPcs1900.getText().toString()), 0);
                    mSetMaxPowerHandler.sendMessage(msg);
                }
            }
        });
        buttonOk = (Button) findViewById(R.id.ok_td19);
        buttonOk.setText("Set");
        buttonOk.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mTD19.getText().toString().equals("")) {
                    Toast.makeText(SendPowerPrefActivity.this, "please input data",
                            Toast.LENGTH_SHORT)
                            .show();
                } else {
                    Message msg = mSetMaxPowerHandler.obtainMessage(SET_TD19_MAX_POWER,
                            Integer.valueOf(mTD19.getText().toString()), 0);
                    mSetMaxPowerHandler.sendMessage(msg);
                }
            }
        });
        buttonOk = (Button) findViewById(R.id.ok_td21);
        buttonOk.setText("Set");
        buttonOk.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mTD21.getText().toString().equals("")) {
                    Toast.makeText(SendPowerPrefActivity.this, "please input data",
                            Toast.LENGTH_SHORT)
                            .show();
                } else {
                    Message msg = mSetMaxPowerHandler.obtainMessage(SET_TD21_MAX_POWER,
                            Integer.valueOf(mTD21.getText().toString()), 0);
                    mSetMaxPowerHandler.sendMessage(msg);
                }
            }
        });
        buttonOk = (Button) findViewById(R.id.ok_wband1);
        buttonOk.setText("Set");
        buttonOk.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mWBand1.getText().toString().equals("")) {
                    Toast.makeText(SendPowerPrefActivity.this,
                            "please input data", Toast.LENGTH_SHORT).show();
                } else {
                    Message msg = mSetMaxPowerHandler.obtainMessage(
                            SET_WBand1_MAX_POWER,
                            Integer.valueOf(mWBand1.getText().toString()), 0);
                    mSetMaxPowerHandler.sendMessage(msg);
                }
            }
        });
        buttonOk = (Button) findViewById(R.id.ok_wband2);
        buttonOk.setText("Set");
        buttonOk.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mWBand2.getText().toString().equals("")) {
                    Toast.makeText(SendPowerPrefActivity.this,
                            "please input data", Toast.LENGTH_SHORT).show();
                } else {
                    Message msg = mSetMaxPowerHandler.obtainMessage(
                            SET_WBand2_MAX_POWER,
                            Integer.valueOf(mWBand2.getText().toString()), 0);
                    mSetMaxPowerHandler.sendMessage(msg);
                }
            }
        });
        buttonOk = (Button) findViewById(R.id.ok_wband5);
        buttonOk.setText("Set");
        buttonOk.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mWBand5.getText().toString().equals("")) {
                    Toast.makeText(SendPowerPrefActivity.this,
                            "please input data", Toast.LENGTH_SHORT).show();
                } else {
                    Message msg = mSetMaxPowerHandler.obtainMessage(
                            SET_WBand5_MAX_POWER,
                            Integer.valueOf(mWBand5.getText().toString()), 0);
                    mSetMaxPowerHandler.sendMessage(msg);
                }
            }
        });
        buttonOk = (Button) findViewById(R.id.ok_wband8);
        buttonOk.setText("Set");
        buttonOk.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mWBand8.getText().toString().equals("")) {
                    Toast.makeText(SendPowerPrefActivity.this,
                            "please input data", Toast.LENGTH_SHORT).show();
                } else {
                    Message msg = mSetMaxPowerHandler.obtainMessage(
                            SET_WBand8_MAX_POWER,
                            Integer.valueOf(mWBand8.getText().toString()), 0);
                    mSetMaxPowerHandler.sendMessage(msg);
                }
            }
        });

        Button buttonClear = null;
        buttonClear = (Button) findViewById(R.id.clear_gsm850);
        buttonClear.setText("Clear");
        buttonClear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mSetMaxPowerHandler.sendEmptyMessage(CLEAR_GSM850_MAX_POWER);
            }
        });
        buttonClear = (Button) findViewById(R.id.clear_egsm900);
        buttonClear.setText("Clear");
        buttonClear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mSetMaxPowerHandler.sendEmptyMessage(CLEAR_EGSM900_MAX_POWER);
            }
        });
        buttonClear = (Button) findViewById(R.id.clear_dcs1800);
        buttonClear.setText("Clear");
        buttonClear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mSetMaxPowerHandler.sendEmptyMessage(CLEAR_DCS1800_MAX_POWER);
            }
        });
        buttonClear = (Button) findViewById(R.id.clear_pcs1900);
        buttonClear.setText("Clear");
        buttonClear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mSetMaxPowerHandler.sendEmptyMessage(CLEAR_PCS1900_MAX_POWER);
            }
        });
        buttonClear = (Button) findViewById(R.id.clear_td19);
        buttonClear.setText("Clear");
        buttonClear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mSetMaxPowerHandler.sendEmptyMessage(CLEAR_TD19_MAX_POWER);
            }
        });
        buttonClear = (Button) findViewById(R.id.clear_td21);
        buttonClear.setText("Clear");
        buttonClear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mSetMaxPowerHandler.sendEmptyMessage(CLEAR_TD21_MAX_POWER);
            }
        });
        buttonClear = (Button) findViewById(R.id.clear_wband1);
        buttonClear.setText("Clear");
        buttonClear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mSetMaxPowerHandler.sendEmptyMessage(CLEAR_WBand1_MAX_POWER);
            }
        });
        buttonClear = (Button) findViewById(R.id.clear_wband2);
        buttonClear.setText("Clear");
        buttonClear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mSetMaxPowerHandler.sendEmptyMessage(CLEAR_WBand2_MAX_POWER);
            }
        });
        buttonClear = (Button) findViewById(R.id.clear_wband5);
        buttonClear.setText("Clear");
        buttonClear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mSetMaxPowerHandler.sendEmptyMessage(CLEAR_WBand5_MAX_POWER);
            }
        });
        buttonClear = (Button) findViewById(R.id.clear_wband8);
        buttonClear.setText("Clear");
        buttonClear.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                mSetMaxPowerHandler.sendEmptyMessage(CLEAR_WBand8_MAX_POWER);
            }
        });
        if (!isSupportTD) {
            findViewById(R.id.et_td19).setVisibility(View.GONE);
            findViewById(R.id.et_td21).setVisibility(View.GONE);
            findViewById(R.id.ok_td19).setVisibility(View.GONE);
            findViewById(R.id.ok_td21).setVisibility(View.GONE);
            findViewById(R.id.clear_td19).setVisibility(View.GONE);
            findViewById(R.id.clear_td21).setVisibility(View.GONE);
            findViewById(R.id.text_td19).setVisibility(View.GONE);
            findViewById(R.id.text_td21).setVisibility(View.GONE);
        }
        if (!isSupportWCD) {
            findViewById(R.id.et_wband1).setVisibility(View.GONE);
            findViewById(R.id.et_wband2).setVisibility(View.GONE);
            findViewById(R.id.et_wband5).setVisibility(View.GONE);
            findViewById(R.id.et_wband8).setVisibility(View.GONE);
            findViewById(R.id.ok_wband1).setVisibility(View.GONE);
            findViewById(R.id.ok_wband2).setVisibility(View.GONE);
            findViewById(R.id.ok_wband5).setVisibility(View.GONE);
            findViewById(R.id.ok_wband8).setVisibility(View.GONE);
            findViewById(R.id.clear_wband1).setVisibility(View.GONE);
            findViewById(R.id.clear_wband2).setVisibility(View.GONE);
            findViewById(R.id.clear_wband5).setVisibility(View.GONE);
            findViewById(R.id.clear_wband8).setVisibility(View.GONE);
            findViewById(R.id.text_wband1).setVisibility(View.GONE);
            findViewById(R.id.text_wband2).setVisibility(View.GONE);
            findViewById(R.id.text_wband5).setVisibility(View.GONE);
            findViewById(R.id.text_wband8).setVisibility(View.GONE);
        }
        if (isSupportWCD) {
            SharedPreferences sp = PreferenceManager
                    .getDefaultSharedPreferences(SendPowerPrefActivity.this);
            mWCDMASupportBand = sp.getInt(ORI_WCDMA_BAND, -1);
            if (mWCDMASupportBand == -1) {
                for (int i = 0; i < mWBandMap.length; i++) {
                    String resp = IATUtils.sendATCmd(engconstents.ENG_AT_W_LOCKED_BAND
                            + mWBandMap[i], "atchannel0");
                    Log.d(TAG, "getSelectedBand() resp:" + resp);
                    if (resp != null && resp.contains(IATUtils.AT_OK)) {
                        if (parseWCDMAResp(resp) == 0) {
                            findViewById(mWBandET[i]).setVisibility(View.GONE);
                            findViewById(mWBandOK[i]).setVisibility(View.GONE);
                            findViewById(mWBandClear[i]).setVisibility(View.GONE);
                            findViewById(mWBandText[i]).setVisibility(View.GONE);
                        }
                    }
                }
            } else {
                for (int i = 0; i < mWBandMap.length; i++) {
                    if (((mWCDMASupportBand >> i) & (0x01)) == 0) {
                        findViewById(mWBandET[i]).setVisibility(View.GONE);
                        findViewById(mWBandOK[i]).setVisibility(View.GONE);
                        findViewById(mWBandClear[i]).setVisibility(View.GONE);
                        findViewById(mWBandText[i]).setVisibility(View.GONE);
                    }
                }
            }
        }
    }

    private int parseWCDMAResp(String resp) {
        String str[] = resp.split(":|\n|,");
        Log.d(TAG, "parseWCDMAResp() str:" + Arrays.toString(str));
        if (str.length >= 3 && str[3].trim().equals("1")) {
            return 1;
        }
        return 0;
    }

    class SetMaxPowerHandler extends Handler {
        public SetMaxPowerHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String mStrTmp = IATUtils.AT_FAIL;
            int max_value = msg.arg1;
            switch (msg.what) {
                case SET_GSM850_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "1,1,"
                            + max_value + ",0,0,0",
                            "atchannel0");
                    break;
                case CLEAR_GSM850_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "1,0," + "0,0,0,0",
                            "atchannel0");
                    break;
                case SET_EGSM900_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "2,1,"
                            + max_value + ",0,0,0",
                            "atchannel0");
                    break;
                case CLEAR_EGSM900_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "2,0," + "0,0,0,0",
                            "atchannel0");
                    break;
                case SET_DCS1800_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "3,1,"
                            + max_value + ",0,0,0",
                            "atchannel0");
                    break;
                case CLEAR_DCS1800_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "3,0," + "0,0,0,0",
                            "atchannel0");
                    break;
                case SET_PCS1900_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "4,1,"
                            + max_value + ",0,0,0",
                            "atchannel0");
                    break;
                case CLEAR_PCS1900_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "4,0," + "0,0,0,0",
                            "atchannel0");
                    break;
                case SET_TD19_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "5,1,"
                            + max_value + ",10054,0,0",
                            "atchannel0");
                    break;
                case CLEAR_TD19_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "5,0," + "0,0,0,0",
                            "atchannel0");
                    break;
                case SET_TD21_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "6,1,"
                            + max_value + ",9404,0,0",
                            "atchannel0");
                    break;
                case CLEAR_TD21_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "6,0," + "0,0,0,0",
                            "atchannel0");
                    break;
                case SET_WBand1_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "7,1,"
                            + max_value + ",10693,0,0",
                            "atchannel0");
                    break;
                case CLEAR_WBand1_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "7,0," + "0,0,0,0",
                            "atchannel0");
                    break;
                case SET_WBand2_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "8,1,"
                            + max_value + ",9875,0,0",
                            "atchannel0");
                    break;
                case CLEAR_WBand2_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "8,0," + "0,0,0,0",
                            "atchannel0");
                    break;
                case SET_WBand5_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "9,1,"
                            + max_value + ",4450,0,0",
                            "atchannel0");
                    break;
                case CLEAR_WBand5_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "9,0," + "0,0,0,0",
                            "atchannel0");
                    break;
                case SET_WBand8_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "10,1,"
                            + max_value + ",3012,0,0",
                            "atchannel0");
                    break;
                case CLEAR_WBand8_MAX_POWER:
                    mStrTmp = IATUtils.sendATCmd(engconstents.ENG_SEND_POWER + "10,0," + "0,0,0,0",
                            "atchannel0");
                    break;
                default:
                    return;
            }

            if (mStrTmp.contains(IATUtils.AT_OK)) {
                Toast.makeText(SendPowerPrefActivity.this, "Success", Toast.LENGTH_SHORT).show();
                switch (msg.what) {
                    case CLEAR_GSM850_MAX_POWER:
                        mUiHandler.post(new Runnable() {

                            @Override
                            public void run() {
                                mGsm850.setText("");
                            }

                        });
                        break;

                    case CLEAR_EGSM900_MAX_POWER:
                        mUiHandler.post(new Runnable() {

                            @Override
                            public void run() {
                                mEgsm900.setText("");
                            }

                        });
                        break;

                    case CLEAR_DCS1800_MAX_POWER:
                        mUiHandler.post(new Runnable() {

                            @Override
                            public void run() {
                                mDcs1800.setText("");
                            }

                        });
                        break;

                    case CLEAR_PCS1900_MAX_POWER:
                        mUiHandler.post(new Runnable() {

                            @Override
                            public void run() {
                                mPcs1900.setText("");
                            }

                        });
                        break;

                    case CLEAR_TD19_MAX_POWER:
                        mUiHandler.post(new Runnable() {

                            @Override
                            public void run() {
                                mTD19.setText("");
                            }

                        });
                        break;

                    case CLEAR_TD21_MAX_POWER:
                        mUiHandler.post(new Runnable() {

                            @Override
                            public void run() {
                                mTD21.setText("");
                            }

                        });
                        break;
                    case CLEAR_WBand1_MAX_POWER:
                        mUiHandler.post(new Runnable() {

                            @Override
                            public void run() {
                                mWBand1.setText("");
                            }

                        });
                        break;
                    case CLEAR_WBand2_MAX_POWER:
                        mUiHandler.post(new Runnable() {

                            @Override
                            public void run() {
                                mWBand2.setText("");
                            }

                        });
                        break;
                    case CLEAR_WBand5_MAX_POWER:
                        mUiHandler.post(new Runnable() {

                            @Override
                            public void run() {
                                mWBand5.setText("");
                            }

                        });
                        break;
                    case CLEAR_WBand8_MAX_POWER:
                        mUiHandler.post(new Runnable() {

                            @Override
                            public void run() {
                                mWBand8.setText("");
                            }

                        });
                        break;
                }
            } else {
                Toast.makeText(SendPowerPrefActivity.this, "Fail", Toast.LENGTH_SHORT).show();
            }
        }
    }

}
