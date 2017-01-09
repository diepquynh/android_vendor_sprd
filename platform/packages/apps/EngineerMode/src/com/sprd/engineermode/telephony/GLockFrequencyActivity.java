package com.sprd.engineermode.telephony;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.text.InputFilter;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import android.widget.TextView;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class GLockFrequencyActivity extends Activity {

    private static final String TAG = "GLockFrequencyActivity";
    private static final int DEFAULT_LOCK_MAX = 6;
    private static final int GET_GSM_LOCK_FREQ = 0;
    private static final int SET_GSM_LOCK_FREQ = 1;
    private static final int SET_GSM_UNLOCK_FREQ = 2;
    private TextView[] mFreq = new TextView[DEFAULT_LOCK_MAX];
    private EditText[] mEditFreq = new EditText[DEFAULT_LOCK_MAX];
    private Button mButtonLock;
    private Button mButtonUnlock;
    private String mServerName = "atchannel0";

    private GsmLockFreqHandler mGsmLockFreqHandler;
    private Handler mUiThread = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case GET_GSM_LOCK_FREQ:
                String atRsult = (String) msg.obj;
                Log.d(TAG, "atRsult = " + atRsult);
                if (atRsult != null && atRsult.contains(IATUtils.AT_OK)) {
                    mButtonLock.setEnabled(true);
                    mButtonUnlock.setEnabled(true);
                    String[] str = atRsult.split("\n");
                    String[] str1 = str[0].split("\\:");
                    if (str1[1].contains(",")) {
                        String[] str2 = str1[1].split("\\,");
                        for (int i = 0; i < DEFAULT_LOCK_MAX; i++) {
                            if (i < (str2.length - 1)) {
                                mEditFreq[i].setText(str2[i + 1].trim());
                            } else {
                                mEditFreq[i].setText("0");
                            }
                        }
                    } else {
                        for (int i = 0; i < DEFAULT_LOCK_MAX; i++) {
                            mEditFreq[i].setText("0");
                        }
                    }
                } else {
                    mButtonLock.setEnabled(false);
                    mButtonUnlock.setEnabled(false);
                }
                break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.gsm_lockfreq);
        initialpara();
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mGsmLockFreqHandler = new GsmLockFreqHandler(ht.getLooper());
        Message m = mGsmLockFreqHandler.obtainMessage(GET_GSM_LOCK_FREQ);
        mGsmLockFreqHandler.sendMessage(m);
    }

    @Override
    protected void onDestroy() {
        if (mGsmLockFreqHandler != null) {
            mGsmLockFreqHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    private void initialpara() {
        mFreq[0] = (TextView) findViewById(R.id.Text11);
        mFreq[1] = (TextView) findViewById(R.id.Text12);
        mFreq[2] = (TextView) findViewById(R.id.Text13);
        mFreq[3] = (TextView) findViewById(R.id.Text21);
        mFreq[4] = (TextView) findViewById(R.id.Text22);
        mFreq[5] = (TextView) findViewById(R.id.Text23);

        mEditFreq[0] = (EditText) findViewById(R.id.editText11);
        mEditFreq[1] = (EditText) findViewById(R.id.editText12);
        mEditFreq[2] = (EditText) findViewById(R.id.editText13);
        mEditFreq[3] = (EditText) findViewById(R.id.editText21);
        mEditFreq[4] = (EditText) findViewById(R.id.editText22);
        mEditFreq[5] = (EditText) findViewById(R.id.editText23);

        mButtonLock = (Button) findViewById(R.id.lock_button);
        mButtonUnlock = (Button) findViewById(R.id.clear_button);
        mButtonLock.setText("Lock");
        mButtonUnlock.setText("Unlock");
        for (int i = 0; i < DEFAULT_LOCK_MAX; i++) {
            mFreq[i].setText("Freq" + i);
        }
        mButtonLock.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                if (findInvalidText() != null) {
                    Toast.makeText(GLockFrequencyActivity.this,
                            "Para is wrong", Toast.LENGTH_SHORT).show();
                    return;
                }
                mButtonLock.setEnabled(false);
                mButtonUnlock.setEnabled(false);
                Message m = mGsmLockFreqHandler
                        .obtainMessage(SET_GSM_LOCK_FREQ);
                mGsmLockFreqHandler.sendMessage(m);
            }
        });
        mButtonUnlock.setOnClickListener(new Button.OnClickListener() {

            public void onClick(View v) {
                mButtonLock.setEnabled(false);
                mButtonUnlock.setEnabled(false);
                Message m = mGsmLockFreqHandler
                        .obtainMessage(SET_GSM_UNLOCK_FREQ);
                mGsmLockFreqHandler.sendMessage(m);
            }
        });
    }

    private EditText findInvalidText() {
        for (int line = 0; line < DEFAULT_LOCK_MAX; line++) {
            if (!mEditFreq[line].getText().toString().trim().equals("")) {
                int freq = Integer.parseInt(mEditFreq[line].getText()
                        .toString().trim());
                if (freq < 0 || freq > 2047) {
                    return mEditFreq[line];
                }
            } else {
                return mEditFreq[line];
            }
        }
        return null;
    }

    class GsmLockFreqHandler extends Handler {
        public GsmLockFreqHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String responValue = null;
            String mATCmd = null;
            switch (msg.what) {
            case GET_GSM_LOCK_FREQ:
                mATCmd = engconstents.ENG_AT_SPGSMFRQ1;
                responValue = IATUtils.sendATCmd(mATCmd, mServerName);
                mUiThread.sendMessage(mUiThread.obtainMessage(
                        GET_GSM_LOCK_FREQ, responValue));
                break;
            case SET_GSM_LOCK_FREQ:
                mATCmd = engconstents.ENG_AT_SPGSMFRQ + "1";
                for (int i = 0; i < DEFAULT_LOCK_MAX; i++) {
                    if (!mEditFreq[i].equals("")) {
                        mATCmd = mATCmd + ","
                                + mEditFreq[i].getText().toString().trim();
                    }
                }
                responValue = IATUtils.sendATCmd(mATCmd, mServerName);
                Log.d(TAG, "resluts of " + mATCmd + "is :" + responValue);
                if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mButtonLock.setEnabled(true);
                            mButtonUnlock.setEnabled(true);
                            Toast.makeText(GLockFrequencyActivity.this,
                                    "Success", Toast.LENGTH_SHORT).show();
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mButtonLock.setEnabled(false);
                            mButtonUnlock.setEnabled(false);
                            Toast.makeText(GLockFrequencyActivity.this, "Fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                break;
            case SET_GSM_UNLOCK_FREQ:
                mATCmd = engconstents.ENG_AT_SPGSMFRQ + "0";
                responValue = IATUtils.sendATCmd(mATCmd, mServerName);
                Log.d(TAG, "resluts of " + mATCmd + "is :" + responValue);
                if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mButtonLock.setEnabled(true);
                            mButtonUnlock.setEnabled(true);
                            Message m = mGsmLockFreqHandler
                                    .obtainMessage(GET_GSM_LOCK_FREQ);
                            mGsmLockFreqHandler.sendMessage(m);
                            Toast.makeText(GLockFrequencyActivity.this,
                                    "Success", Toast.LENGTH_SHORT).show();
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mButtonLock.setEnabled(false);
                            mButtonUnlock.setEnabled(false);
                            Toast.makeText(GLockFrequencyActivity.this, "Fail",
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
