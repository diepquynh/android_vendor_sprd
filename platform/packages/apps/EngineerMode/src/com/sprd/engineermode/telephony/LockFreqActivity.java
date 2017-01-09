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

public class LockFreqActivity extends Activity {
    private static final String TAG = "LockFreqActivity";

    private boolean isSupportLTE = false;

    private static final int LTE_LOCK_MAX = 2;
    private static final int VALUES_COUNT = 4;
    private static final int DEFAULT_LOCK_MAX = 4;
    private Button mButtonLock;
    private Button mButtonUnlock;
    private TextView freq;
    private TextView cell1;
    private TextView cell2;
    private TextView cell3;
    private EditText[] mFreqEdit = new EditText[DEFAULT_LOCK_MAX];
    private EditText[] mCell1Edit = new EditText[DEFAULT_LOCK_MAX];
    private EditText[] mCell2Edit = new EditText[DEFAULT_LOCK_MAX];
    private EditText[] mCell3Edit = new EditText[DEFAULT_LOCK_MAX];

    private static final int GETLOCKFRQ = 0;
    private static final int SETLOCKFRQ = 1;
    private static final int SETUNLOCKFRQ = 2;

    private Handler mUiThread = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case GETLOCKFRQ: {
                String atRsult = (String) msg.obj;
                Log.d(TAG, "atRsult = " + atRsult);
                if (atRsult.contains(IATUtils.AT_OK)) {
                    String freqs[] = atRsult.split("\r")[0].split("\\+SPFRQ\\:");
                    Log.d(TAG, "freqs line is :" + freqs.length);
                    int line = 0;
                    if (LTE_LOCK_MAX == (freqs.length - 1)) {
                        isSupportLTE = true;
                        freq.setVisibility(View.VISIBLE);
                        mFreqEdit[0].setVisibility(View.VISIBLE);
                        mFreqEdit[1].setVisibility(View.VISIBLE);
                        for (String freq : freqs) {
                            Log.d(TAG, "line = " + line + " freq = " + freq);
                            if (line == LTE_LOCK_MAX) {
                                break;
                            }
                            String value[] = freq.split(",");
                            if (value.length == VALUES_COUNT) {
                                mFreqEdit[line].setText(value[0].trim());
                                line++;
                            }
                        }
                    } else {
                        isSupportLTE = false;
                        freq.setVisibility(View.VISIBLE);
                        cell1.setVisibility(View.VISIBLE);
                        cell2.setVisibility(View.VISIBLE);
                        cell3.setVisibility(View.VISIBLE);
                        for (int i = 0; i < DEFAULT_LOCK_MAX; i++) {
                            mFreqEdit[i].setVisibility(View.VISIBLE);
                            mCell1Edit[i].setVisibility(View.VISIBLE);
                            mCell2Edit[i].setVisibility(View.VISIBLE);
                            mCell3Edit[i].setVisibility(View.VISIBLE);
                            for (String freq : freqs) {
                                Log.d(TAG, "line = " + line + " freq = " + freq);
                                if (line == DEFAULT_LOCK_MAX) {
                                    break;
                                }
                                String value[] = freq.split(",");
                                if (value.length == VALUES_COUNT) {
                                    mFreqEdit[line].setText(value[0].trim());
                                    mCell1Edit[line].setText(value[1].trim());
                                    mCell2Edit[line].setText(value[2].trim());
                                    mCell3Edit[line].setText(value[3].trim());
                                    line++;
                                }
                            }
                        }
                    }
                    mButtonLock.setEnabled(true);
                    mButtonUnlock.setEnabled(true);
                } else {
                    mButtonLock.setEnabled(false);
                    mButtonUnlock.setEnabled(false);
                }
            }
                break;
            case SETLOCKFRQ: {
                String atRsult = (String) msg.obj;
                if (atRsult.contains(IATUtils.AT_OK)) {
                    Toast.makeText(LockFreqActivity.this, "Lock Success",
                            Toast.LENGTH_SHORT).show();
                    mButtonLock.setEnabled(true);
                    mButtonUnlock.setEnabled(true);
                }
            }
                break;

            case SETUNLOCKFRQ: {
                String atRsult = (String) msg.obj;
                if (atRsult.contains(IATUtils.AT_OK)) {
                    Toast.makeText(LockFreqActivity.this, "Unlock Success",
                            Toast.LENGTH_SHORT).show();
                    Message m = lockFreqHandler.obtainMessage(GETLOCKFRQ);
                    lockFreqHandler.sendMessage(m);
                }
            }
                break;
            }
        }
    };
    private LockFreqHandler lockFreqHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.lockfreq);

        mButtonLock = (Button) findViewById(R.id.lock_button);
        mButtonUnlock = (Button) findViewById(R.id.clear_button);
        mButtonLock.setText("Lock");
        mButtonUnlock.setText("Unlock");

        initialpara();

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        lockFreqHandler = new LockFreqHandler(ht.getLooper());

        Message m = lockFreqHandler.obtainMessage(GETLOCKFRQ);
        lockFreqHandler.sendMessage(m);
    }

    private void initialpara() {
        freq = (TextView) findViewById(R.id.Text1);
        cell1 = (TextView) findViewById(R.id.Text2);
        cell2 = (TextView) findViewById(R.id.Text3);
        cell3 = (TextView) findViewById(R.id.Text4);

        freq.setText("Freq:");
        cell1.setText("Cell1:");
        cell2.setText("Cell2:");
        cell3.setText("Cell3:");

        mFreqEdit[0] = (EditText) findViewById(R.id.editText11);
        mCell1Edit[0] = (EditText) findViewById(R.id.editText12);
        mCell2Edit[0] = (EditText) findViewById(R.id.editText13);
        mCell3Edit[0] = (EditText) findViewById(R.id.editText14);

        mFreqEdit[1] = (EditText) findViewById(R.id.editText21);
        mCell1Edit[1] = (EditText) findViewById(R.id.editText22);
        mCell2Edit[1] = (EditText) findViewById(R.id.editText23);
        mCell3Edit[1] = (EditText) findViewById(R.id.editText24);

        mFreqEdit[2] = (EditText) findViewById(R.id.editText31);
        mCell1Edit[2] = (EditText) findViewById(R.id.editText32);
        mCell2Edit[2] = (EditText) findViewById(R.id.editText33);
        mCell3Edit[2] = (EditText) findViewById(R.id.editText34);

        mFreqEdit[3] = (EditText) findViewById(R.id.editText41);
        mCell1Edit[3] = (EditText) findViewById(R.id.editText42);
        mCell2Edit[3] = (EditText) findViewById(R.id.editText43);
        mCell3Edit[3] = (EditText) findViewById(R.id.editText44);

        freq.setVisibility(View.GONE);
        cell1.setVisibility(View.GONE);
        cell2.setVisibility(View.GONE);
        cell3.setVisibility(View.GONE);
        for (int line = 0; line < DEFAULT_LOCK_MAX; line++) {
            mFreqEdit[line].setVisibility(View.GONE);
            mCell1Edit[line].setVisibility(View.GONE);
            mCell2Edit[line].setVisibility(View.GONE);
            mCell3Edit[line].setVisibility(View.GONE);
        }
        mButtonLock.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                if (findInvalidText() != null) {
                    return;
                }
                mButtonLock.setEnabled(false);
                mButtonUnlock.setEnabled(false);
                Message m = lockFreqHandler.obtainMessage(SETLOCKFRQ);
                lockFreqHandler.sendMessage(m);
            }
        });
        mButtonUnlock.setOnClickListener(new Button.OnClickListener() {

            public void onClick(View v) {
                mButtonLock.setEnabled(false);
                mButtonUnlock.setEnabled(false);
                Message m = lockFreqHandler.obtainMessage(SETUNLOCKFRQ);
                lockFreqHandler.sendMessage(m);
            }
        });
    }

    private String analysisResponse(String response, int type) {
        if (response.contains(IATUtils.AT_OK)) {
            return IATUtils.AT_OK;
        }
        return IATUtils.AT_FAIL;

    }

    class LockFreqHandler extends Handler {
        public LockFreqHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case GETLOCKFRQ: {
                String atResponse = IATUtils.sendATCmd(
                        engconstents.ENG_AT_GETSPFRQ1, "atchannel0");
                mUiThread.sendMessage(mUiThread.obtainMessage(GETLOCKFRQ,
                        atResponse));
            }
                break;
            case SETLOCKFRQ: {
                String atResponse = IATUtils.AT_OK;
                if (!isSupportLTE) {
                    for (int line = 0; line < DEFAULT_LOCK_MAX; line++) {
                        atResponse = IATUtils.sendATCmd(
                                engconstents.ENG_AT_SETSPFRQ1
                                        + "0,"
                                        + line
                                        + ","
                                        + mFreqEdit[line].getText().toString()
                                                .trim()
                                        + ","
                                        + mCell1Edit[line].getText().toString()
                                                .trim()
                                        + ","
                                        + mCell2Edit[line].getText().toString()
                                                .trim()
                                        + ","
                                        + mCell3Edit[line].getText().toString()
                                                .trim(), "atchannel0");
                        if (!atResponse.contains(IATUtils.AT_OK)) {
                            mUiThread.sendMessage(mUiThread.obtainMessage(
                                    SETLOCKFRQ, atResponse));
                            return;
                        }
                    }
                } else {
                    for (int line = 0; line < LTE_LOCK_MAX; line++) {
                        atResponse = IATUtils.sendATCmd(
                                engconstents.ENG_AT_SETSPFRQ1
                                        + "0,"
                                        + (4 + line)
                                        + ","
                                        + mFreqEdit[line].getText().toString()
                                                .trim(), "atchannel0");
                        if (!atResponse.contains(IATUtils.AT_OK)) {
                            mUiThread.sendMessage(mUiThread.obtainMessage(
                                    SETLOCKFRQ, atResponse));
                            return;
                        }
                    }
                }
                mUiThread.sendMessage(mUiThread.obtainMessage(SETLOCKFRQ,
                        atResponse));
            }
                break;

            case SETUNLOCKFRQ: {
                String atResponse = IATUtils.AT_OK;
                for (int i = 0; i < DEFAULT_LOCK_MAX; i++) {
                    atResponse = IATUtils.sendATCmd(
                            engconstents.ENG_AT_SETSPFRQ1 + "1", "atchannel0");
                    if (!atResponse.contains(IATUtils.AT_OK)) {
                        mUiThread.sendMessage(mUiThread.obtainMessage(
                                SETUNLOCKFRQ, atResponse));
                        return;
                    }
                }

                mUiThread.sendMessage(mUiThread.obtainMessage(SETUNLOCKFRQ,
                        atResponse));
            }
                break;
            }
        }
    }

    private EditText findInvalidText() {
        if (isSupportLTE) {
            if (mFreqEdit[0].getText().toString().trim() != null
                    && mFreqEdit[0].getText().toString().trim().length() != 0
                    && mFreqEdit[1].getText().toString().trim() != null
                    && mFreqEdit[1].getText().toString().trim().length() != 0) {
                int freq0 = Integer.parseInt(mFreqEdit[0].getText().toString()
                        .trim());
                int freq1 = Integer.parseInt(mFreqEdit[1].getText().toString()
                        .trim());
                if (freq0 < 0 || freq0 > 65535) {
                    Toast.makeText(LockFreqActivity.this, "Para is wrong",
                            Toast.LENGTH_SHORT).show();
                    return mFreqEdit[0];
                }
                if (freq1 < 0 || freq1 > 65535) {
                    Toast.makeText(LockFreqActivity.this, "Para is wrong",
                            Toast.LENGTH_SHORT).show();
                    return mFreqEdit[1];
                }
                if (freq0 > freq1) {
                    Toast.makeText(
                            LockFreqActivity.this,
                            "The first number must not be greater than the second number",
                            Toast.LENGTH_SHORT).show();
                    return mFreqEdit[0];
                }
            } else {
                Toast.makeText(
                        LockFreqActivity.this,
                        "Input is wrong, it can not be empty",
                        Toast.LENGTH_SHORT).show();
                return mFreqEdit[0];
            }
        } else {
            for (int line = 0; line < DEFAULT_LOCK_MAX; line++) {
                if (mFreqEdit[line].getText().toString().trim() != null
                        && mFreqEdit[line].getText().toString().trim().length() != 0
                        && mCell1Edit[line].getText().toString().trim() != null
                        && mCell1Edit[line].getText().toString().trim().length() != 0
                        && mCell2Edit[line].getText().toString().trim() != null
                        && mCell2Edit[line].getText().toString().trim().length() != 0
                        && mCell3Edit[line].getText().toString().trim() != null
                        && mCell3Edit[line].getText().toString().trim().length() != 0) {
                    int freq = Integer.parseInt(mFreqEdit[line].getText()
                            .toString().trim());
                    int cell1 = Integer.parseInt(mCell1Edit[line].getText()
                            .toString().trim());
                    int cell2 = Integer.parseInt(mCell2Edit[line].getText()
                            .toString().trim());
                    int cell3 = Integer.parseInt(mCell3Edit[line].getText()
                            .toString().trim());

                    if (freq < 0 || freq > 65535) {
                        Toast.makeText(LockFreqActivity.this, "Para is wrong",
                                Toast.LENGTH_SHORT).show();
                        return mFreqEdit[line];
                    }

                    if (cell1 < 0 || cell1 > 255) {
                        Toast.makeText(LockFreqActivity.this, "Para is wrong",
                                Toast.LENGTH_SHORT).show();
                        return mCell1Edit[line];
                    }

                    if (cell2 < 0 || cell2 > 255) {
                        Toast.makeText(LockFreqActivity.this, "Para is wrong",
                                Toast.LENGTH_SHORT).show();
                        return mCell2Edit[line];
                    }

                    if (cell3 < 0 || cell3 > 255) {
                        Toast.makeText(LockFreqActivity.this, "Para is wrong",
                                Toast.LENGTH_SHORT).show();
                        return mCell3Edit[line];
                    }
                } else {
                    Toast.makeText(
                            LockFreqActivity.this,
                            "Input is wrong, it can not be empty",
                            Toast.LENGTH_SHORT).show();
                    return mFreqEdit[line];
                }
           
            }
        }
        return null;
    }

    class LockFreqATFailRunnable implements Runnable {

        @Override
        public void run() {
            Toast.makeText(LockFreqActivity.this, "Fail", Toast.LENGTH_SHORT)
                    .show();
        }

    }

    class LockFreqATSuccessRunnable implements Runnable {

        @Override
        public void run() {
            Toast.makeText(LockFreqActivity.this, "Success", Toast.LENGTH_SHORT)
                    .show();
        }

    }

    @Override
    protected void onDestroy() {
        if (lockFreqHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            lockFreqHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        finish();
        super.onBackPressed();
    }

}
