package com.sprd.engineermode.debuglog.slogui;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.view.View;
import android.content.Intent;
import android.widget.EditText;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Toast;
import android.view.View.OnClickListener;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.util.Log;
import android.text.method.NumberKeyListener;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;
import android.telephony.TelephonyManager;
import android.os.SystemProperties;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class CustomSettingActivity extends Activity implements OnClickListener {

    private static final String TAG = "CustomSettingActivity";
    private static final String LOG_SCENARIOS_STATUS = "scenarios_status";
    private static final String LOG_SPECIAL_STATUS = "special_status";
    private static final String CMD_INPUT = "cmd_input";
    private static final String ARM_CMD_INPUT = "arm_cmd";

    private static final int GET_LOG_SCENARIOS_STATUS = 0;
    private static final int SET_CUSTOM_SCENARIOS = 1;
    private static final int SET_CUSTOM_DEFAULT = 2;
    private EditText mLogLevel;
    private EditText mTraceModule;
    private EditText mMsgSap;
    private Button mCustomSet;
    private Button mCustomDefault;
    private String mServerName = "atchannel0";
    private int mCmdInputStyle = -1;
    private boolean isUser = false;
    private boolean isSupportW = false;
    private int mScenariosStatus = 0;
    private String mLastCmd = null;
    private CustomSettingHandler mCustomSettingHandler;
    private Handler mUiThread = new Handler();

    private static SharedPreferences pref;

    private static final String LOG_USER_CMD = "1,0,"
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\""
            + ","
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\"";
    private static final String LOG_DEBUG_CMD = "1,3,"
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\""
            + ","
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\"";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.custom_setting);
        Intent intent = this.getIntent();
        Log.d(TAG,"cmd_input: " + mCmdInputStyle);
        isUser = SystemProperties.get("ro.build.type").equalsIgnoreCase("user");
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mCustomSettingHandler = new CustomSettingHandler(ht.getLooper());
        mLogLevel = (EditText) findViewById(R.id.edit_log_level);
        mLogLevel.setSingleLine(false);
        mTraceModule = (EditText) findViewById(R.id.edit_trace_module);
        mTraceModule.setSingleLine(false);
        mCustomSet = (Button) findViewById(R.id.button_custom_set);
        mCustomSet.setOnClickListener(this);
        mCustomDefault = (Button) findViewById(R.id.button_custom_default_set);
        mCustomDefault.setOnClickListener(this);
        pref = PreferenceManager.getDefaultSharedPreferences(this);
        sync();
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mCustomSettingHandler != null) {
            mCustomSettingHandler.getLooper().quit();
        }
    }

    public void sync() {
        if (mLogLevel != null && mTraceModule != null) {
            mLastCmd = pref.getString(ARM_CMD_INPUT, "");
            if (mLastCmd.equals("")) {
                mLogLevel.setText(getString(R.string.custom_last_cmd_no));
            } else {
                mLogLevel.setText(mLastCmd.trim());
            }
            mTraceModule.setText(null);
        }
    }

    @Override
    public void onClick(View v) {
        if (v.equals(mCustomSet)) {
            Message setScenariosStatus = mCustomSettingHandler
                    .obtainMessage(SET_CUSTOM_SCENARIOS);
            mCustomSettingHandler.sendMessage(setScenariosStatus);
        } else if (v.equals(mCustomDefault)) {
            Message setDefault = mCustomSettingHandler
                    .obtainMessage(SET_CUSTOM_DEFAULT);
            mCustomSettingHandler.sendMessage(setDefault);
       }
    }

    private boolean checkEditTextInput() {
        int mLogLevelInput = Integer.parseInt(mLogLevel.getText().toString());
        if (mLogLevelInput < 0 || mLogLevelInput > 5) {
            return false;
        }
        if (mTraceModule.getText().toString().length() < 32) {
            return false;
        }
        if (mMsgSap.getText().toString().length() < 64) {
            return false;
        }
        return true;
    }

    class CustomSettingHandler extends Handler {
        public CustomSettingHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String atResponse = null;
            String atCmd = null;
            switch (msg.what) {
            case SET_CUSTOM_SCENARIOS:
                String inputCmd = mTraceModule.getText().toString().trim();
                if ("".equals(inputCmd)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Toast.makeText(CustomSettingActivity.this,
                                    "Input is empty, pleast check it", Toast.LENGTH_SHORT)
                                    .show();
                        }
                    });
                    break;
                    }
                atResponse = IATUtils.sendATCmd(inputCmd, mServerName);
                Log.d(TAG, inputCmd + ":" + atResponse);
                if (atResponse != null
                        && atResponse.contains(IATUtils.AT_OK)) {
                    pref.edit()
                    .putInt(LOG_SCENARIOS_STATUS, 3)
                    .putInt(LOG_SPECIAL_STATUS, 0)
                    .putString(ARM_CMD_INPUT, inputCmd)
                    .commit();
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG, "set ok");
                            sync();
                            Toast.makeText(CustomSettingActivity.this,
                                    "Set Success", Toast.LENGTH_SHORT)
                                    .show();
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG, "set fail");
                            Toast.makeText(CustomSettingActivity.this,
                                    "Input is wrong, pleast check it",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                break;
            case SET_CUSTOM_DEFAULT:
                 int scenartionStatus = 0;
                 if (isUser) {
                     atCmd = engconstents.ENG_SET_LOG_LEVEL + LOG_DEBUG_CMD;
                     scenartionStatus = 0;
                 } else {
                     atCmd = engconstents.ENG_SET_LOG_LEVEL + LOG_DEBUG_CMD;
                     scenartionStatus = 0;
                 }
                 atResponse = IATUtils.sendATCmd(atCmd, mServerName);
                 Log.d(TAG, atCmd + ":" + atResponse);
                 if (atResponse != null
                         && atResponse.contains(IATUtils.AT_OK)) {
                     pref.edit()
                     .putInt(LOG_SCENARIOS_STATUS, scenartionStatus)
                     .putInt(LOG_SPECIAL_STATUS, 0)
                     .commit();
                     mUiThread.post(new Runnable() {
                         @Override
                         public void run() {
                             Log.d(TAG, "set fail");
                             Toast.makeText(CustomSettingActivity.this,
                                     "Restore Default Success",
                                     Toast.LENGTH_SHORT).show();
                         }
                     });
                 } else {
                     mUiThread.post(new Runnable() {
                         @Override
                         public void run() {
                             Log.d(TAG, "set fail");
                             Toast.makeText(CustomSettingActivity.this,
                                     "Restore Default Fail",
                                     Toast.LENGTH_SHORT).show();
                         }
                     });
                 }
                 break;
            default:
                break;
            }
        }
    }
}
