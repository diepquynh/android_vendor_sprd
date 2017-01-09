
package com.sprd.engineermode.connectivity.wifi;

import com.sprd.engineermode.R;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.PreferenceManager;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Spinner;
import android.widget.EditText;
import android.widget.Button;
import android.widget.TextView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Toast;
import android.widget.AdapterView.OnItemSelectedListener;
import android.graphics.Color;

public class WifiREGWRActivity extends Activity implements OnClickListener, OnItemSelectedListener {

    private static final String TAG = "WifiREGWRActivity";
    private static final String INSMODE_RES = "insmode_result";

    // cmd message
    private static final int INIT_TEST_STATUS = 0;
    private static final int DEINIT_TEST_STATUS = 1;
    private static final int WIFI_REG_READ = 2;
    private static final int WIFI_REG_WRITE = 3;

    // ui message
    private static final int DISPLAY_RESULT = 0;
    private static final int INIT_VIEW = 1;
    private static final int DEINIT_VIEW = 2;

    // setting param key
    private static final String TYPE = "reg_type";
    private static final String ADDR = "reg_addr";
    private static final String LENGTH = "reg_length";
    private static final String VALUE = "reg_value";

    private WifiREGHandler mWifiREGHandler;

    // when WifiEUT Activity insmode success(mInsmodeSuccessByEUT is
    // true),WifiREGActivity does not insmode and rmmode
    // but when when WifiEUT Activity insmode fail,WifiREGActivity will insmode
    // and rmmode
    private boolean mInsmodeSuccessByEUT = false;
    private boolean mInsmodeSuccessByREG = false;

    private Spinner mType;
    private EditText mAddr;
    private EditText mLength;
    private EditText mValue;

    private Button mRead;
    private Button mWrite;

    private AlertDialog mAlertDialog;
    private AlertDialog mCmdAlertDialog;
    private SharedPreferences mPref;

    private WifiEUTHelper.WifiREG mWifiREG;

    private int mTypePosition;
    private String mAlertDialogMsg;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.wifi_reg_wr);
        getWindow().setSoftInputMode(
                WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        mInsmodeSuccessByEUT = this.getIntent().getBooleanExtra(INSMODE_RES, false);

        initUI();
        // disabledView();

        mPref = PreferenceManager.getDefaultSharedPreferences(this);

        mAlertDialog = new AlertDialog.Builder(this)
                .setTitle(getString(R.string.alert_wifi))
                .setMessage(null)
                .setPositiveButton(
                        getString(R.string.alertdialog_ok),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                finish();
                            }
                        }).create();

        mCmdAlertDialog = new AlertDialog.Builder(this)
                .setTitle(getString(R.string.alert_wifi))
                .setMessage(null)
                .setPositiveButton(
                        getString(R.string.alertdialog_ok),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                            }
                        }).create();

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mWifiREGHandler = new WifiREGHandler(ht.getLooper());
        mWifiREG = new WifiEUTHelper.WifiREG();
    }

    @Override
    protected void onStart() {
        refreshUI();
        super.onStart();
    }

    @Override
    public void onBackPressed() {
        super.onBackPressed();
    }

    @Override
    protected void onDestroy() {
        if (mWifiREG != null) {
            Editor editor = mPref.edit();
            editor.putInt(TYPE, mTypePosition);
            editor.putString(ADDR, mWifiREG.addr);
            editor.putString(LENGTH, mWifiREG.length);
            editor.putString(VALUE, mWifiREG.value);
            editor.commit();
        }

        if (mWifiREGHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mWifiREGHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position,
            long id) {
        if (mWifiREG != null) {
            if (parent == mType) {
                mWifiREG.type = this.getResources().getStringArray(R.array.reg_type_str_arr)[position];
                mTypePosition = position;
                if (mWifiREG.type.equals("mac")) {
                    mAddr.setHint("please input 0~800");
                } else if (mWifiREG.type.equals("phy0") || mWifiREG.type.equals("phy1")) {
                    mAddr.setHint("please input 0~ff");
                } else if (mWifiREG.type.equals("rf")) {
                    mAddr.setHint("please input 0~6ff");
                }
                mAddr.setTextColor(Color.GRAY);
            }
        }
    }

/*
 * check if the String s is numeric
 */
    public final static boolean isHexNumber(String str) {
        if(str == null) {
            Log.d(TAG, "String is null");
            return false;
        }
        for(int i=0;i<str.length();i++) {
            char cc = str.charAt(i);
            if(!((cc >=48 && cc <= 57) || (cc >=65 && cc <= 70) || (cc >=97 && cc <= 102)))
                return false;
        }
        return true;
    }

    public final static boolean isInteger(String str) {
        if(str == null) {
            Log.d(TAG, "String is null");
            return false;
        }
        return str.matches("[0-9]+");
    }

    public final boolean isValueBetween(String str) {
        int min = 0;
        int max = 0;
        if (str.length() == 0) {
            Log.d(TAG, "string value is empty");
            Toast.makeText(this, "addr can not be empty!" , Toast.LENGTH_LONG).show();
            return false;
        }
        if(str.length() > 5) {
            Log.d(TAG, "string value is too long" + str.length());
            return false;
         }
        if (mWifiREG.type.equals("mac")) {
            min =0; max = 800;
            if(isInteger(str)) {
                Log.d(TAG, "mac is integer");
                int i = Integer.parseInt(str);
                Log.d(TAG, "mac i=" + i);
                if(i >= min && i <= max)
                {
                    return true;
                }else{
                    Toast.makeText(this, "please input 0~800" , Toast.LENGTH_LONG).show();
                    return false;
                }
            }
            Log.d(TAG, "mac is NOT integer");
            return false;
        } else if (mWifiREG.type.equals("phy0") || mWifiREG.type.equals("phy1")) {
            min =0; max = 0xff;
            if(isHexNumber(str)) {
                Log.d(TAG, "phy is integer");
                int i = Integer.parseInt(str, 16);
                Log.d(TAG, "phy i=" + i);
                if(i >= min && i <= max)
                {
                    return true;
                }else{
                    Toast.makeText(this, "please input 0~ff" , Toast.LENGTH_LONG).show();
                    return false;
                }
            }
            Log.d(TAG, "phy is NOT integer");
            return false;
        } else if (mWifiREG.type.equals("rf")) {
            min =0; max = 0x6ff;
            if(isHexNumber(str)) {
                Log.d(TAG, "rfis integer");
                int i = Integer.parseInt(str, 16);
                Log.d(TAG, "rf i=" + i);
                if(i >= min && i <= max)
                {
                    return true;
                }else{
                    Toast.makeText(this, "please input 0~6ff" , Toast.LENGTH_LONG).show();
                    return false;
                }
            }
            Log.d(TAG, "rf is NOT integer");
            return false;
        }
        return false;
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    @Override
    public void onClick(View v) {
        Message doMessage = null;
        // get the setting param
        getSettingParam();
        // check addr is correct
        if(!isValueBetween(mWifiREG.addr))
            return;
        if (v == mRead) {
            doMessage = mWifiREGHandler.obtainMessage(WIFI_REG_READ);
        } else if (v == mWrite) {
            doMessage = mWifiREGHandler.obtainMessage(WIFI_REG_WRITE);
        }
        mWifiREGHandler.sendMessage(doMessage);
    }

    class WifiREGHandler extends Handler {
        public WifiREGHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            Message uiMessage = null;
            switch (msg.what) {
                case INIT_TEST_STATUS:
                    if (!mInsmodeSuccessByEUT) {
                        initWifi();
                    } else {
                        mInsmodeSuccessByREG = false;
                        mWifiREG = new WifiEUTHelper.WifiREG();
                        uiMessage = mHandler.obtainMessage(INIT_VIEW, 1, 0);
                        mHandler.sendMessage(uiMessage);
                    }
                    break;
                case DEINIT_TEST_STATUS:
                    if (!WifiEUTHelper.getHelper().remodeWifi()) {
                        uiMessage = mHandler.obtainMessage(DEINIT_VIEW);
                        mHandler.sendMessage(uiMessage);
                    } else {
                        finish();
                    }
                    break;
                case WIFI_REG_READ:
                    analysisResult(WifiEUTHelper.getHelper().wifiREGR(mWifiREG));
                    break;
                case WIFI_REG_WRITE:
                    analysisResult(WifiEUTHelper.getHelper().wifiREGW(mWifiREG));
                    break;
                default:
                    break;
            }
        }
    }

    /**
     * refresh UI Handler
     */
    public Handler mHandler = new Handler() {

        public void handleMessage(android.os.Message msg) {
            if (msg == null) {
                Log.d(TAG, "UI Message is null");
                return;
            }

            switch (msg.what) {
                case INIT_VIEW:
                    if (msg.arg1 == 1) {
                        enabledView();
                        refreshUI();
                    } else {
                        mAlertDialog.setMessage("Wifi Init Fail");
                        if (!isFinishing()) {
                            mAlertDialog.show();
                        }
                    }
                    break;
                case DEINIT_VIEW:
                    mAlertDialog.setMessage("Wifi Deinit Fail");
                    if (!isFinishing()) {
                        mAlertDialog.show();
                    }
                    break;
                case DISPLAY_RESULT:
                    mCmdAlertDialog.setMessage(mAlertDialogMsg);
                    if (!isFinishing()) {
                        mCmdAlertDialog.show();
                    }
                    break;
                default:
                    break;
            }
        }
    };

    private void initUI() {
        mType = (Spinner) findViewById(R.id.wifi_eut_type);
        ArrayAdapter<String> typeAdapter = new ArrayAdapter<String>(this,
                android.R.layout.simple_spinner_item, getResources()
                        .getStringArray(R.array.reg_type_str_arr));
        typeAdapter
                .setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mType.setAdapter(typeAdapter);
        mType.setOnItemSelectedListener(this);

        mAddr = (EditText) findViewById(R.id.wifi_eut_addr);
        mAddr.addTextChangedListener(new TextWatcher() {
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
                if (mWifiREG != null) {
                    mWifiREG.addr = mAddr.getText().toString();
                }
            }
        });

        mLength = (EditText) findViewById(R.id.wifi_eut_length);
        mLength.setText("1");
        mLength.addTextChangedListener(new TextWatcher() {
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
                if (mWifiREG != null) {
                    mWifiREG.length = mLength.getText().toString();
                }
            }
        });

        mValue = (EditText) findViewById(R.id.wifi_eut_value);
        mValue.addTextChangedListener(new TextWatcher() {
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
                if (mWifiREG != null) {
                    mWifiREG.value = mValue.getText().toString();
                }
            }
        });

        mRead = (Button) findViewById(R.id.wifi_reg_read);
        mRead.setOnClickListener(this);
        mWrite = (Button) findViewById(R.id.wifi_reg_write);
        mWrite.setOnClickListener(this);
    }

    private void disabledView() {
        mType.setEnabled(false);
        mAddr.setEnabled(false);
        mLength.setEnabled(false);
        mValue.setEnabled(false);
        mRead.setEnabled(false);
        mWrite.setEnabled(false);
    }

    private void enabledView() {
        mType.setEnabled(true);
        mAddr.setEnabled(true);
        // length fixed 1
        mLength.setEnabled(false);
        mValue.setEnabled(true);
        mRead.setEnabled(true);
        mWrite.setEnabled(true);
    }

    private void getSettingParam() {
        mWifiREG.type = this.getResources().getStringArray(R.array.reg_type_str_arr)[mTypePosition];
        mWifiREG.addr = mAddr.getText().toString();
        mWifiREG.length = mLength.getText().toString();
        mWifiREG.value = mValue.getText().toString();
        Log.d(TAG, "Now testing REG in WifiEUT, the setting param is: \n type " + mWifiREG.type
                + "\n addr is " + mWifiREG.addr + "\n length is " + mWifiREG.length
                + "\n value is " + mWifiREG.value);
    }

    private void refreshUI() {
        mTypePosition = mPref.getInt(TYPE, 0);
        mType.setSelection(mTypePosition);

        mAddr.setText(mPref.getString(ADDR, "0"));
        mLength.setText(mPref.getString(LENGTH, "1"));
        mValue.setText(mPref.getString(VALUE, "0"));
    }

    private void analysisResult(String result) {
        Message uiMessage = null;
        // display result
        mAlertDialogMsg = result;
        uiMessage = mHandler.obtainMessage(DISPLAY_RESULT);
        mHandler.sendMessage(uiMessage);
    }

    private void initWifi() {
        Message uiMessage = null;
        if (WifiEUTHelper.getHelper().insmodeWifi()) {
            mWifiREG = new WifiEUTHelper.WifiREG();
            mInsmodeSuccessByREG = true;
            uiMessage = mHandler.obtainMessage(INIT_VIEW, 1, 0);
        } else {
            mInsmodeSuccessByREG = false;
            uiMessage = mHandler.obtainMessage(INIT_VIEW, 0, 0);
        }
        mHandler.sendMessage(uiMessage);
    }

    /*
     * private boolean checkValid(String setNum) { if (setNum.equals("")) {
     * return false; } String curType =
     * this.getResources().getStringArray(R.array
     * .reg_type_str_arr)[mTypePosition];
     * Log.d(TAG,"curType is "+curType+", addr num is "+setNum); if
     * (curType.equals("mac")) { if (Integer.parseInt(setNum)%4 == 0) { return
     * true; } } else if (curType.equals("phy0") || curType.equals("phy1")) { if
     * (setNum.length() < 3) return true; } else if (curType.equals("rf")) { if
     * () } return false; }
     */

}
