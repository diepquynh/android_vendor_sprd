
package com.sprd.engineermode.telephony;

import android.app.Activity;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.os.Bundle;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

import android.view.View;
import android.widget.Toast;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.AlertDialog.Builder;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.content.DialogInterface;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class UnlockFreqActivity extends Activity {
    private static final String TAG = "UnlockFreqActivity";
    public EditText mET;
    private Button mButton;
    private Button mButton01;
    private static final int SETLOCKFRQ = 1;

    private Handler mUiThread = new Handler();
    private UnLockHandler lockFreqHandler;
    private String mATResponse;
    private String mAnalysisResponse;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.unlockfreq);

        mET = (EditText) findViewById(R.id.editText1);
        mButton = (Button) findViewById(R.id.lock_button1);
        mButton01 = (Button) findViewById(R.id.clear_button1);
        clearEditText();
        mButton.setText("Unlock");
        mButton01.setText("Clear");
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        lockFreqHandler = new UnLockHandler(ht.getLooper());

        mButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                Message m = lockFreqHandler.obtainMessage(SETLOCKFRQ);
                lockFreqHandler.sendMessage(m);
            }
        });

        mButton01.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                clearEditText();
            }

        });
    }

    private void clearEditText() {
        mET.setText("0");
    }

    private String analysisResponse(String response, int type) {
        if (response.contains(IATUtils.AT_OK)) {
            return IATUtils.AT_OK;
        }
        return IATUtils.AT_FAIL;

    }

    private class UnLockHandler extends Handler {
        public UnLockHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case SETLOCKFRQ:
                    mATResponse = IATUtils.sendATCmd(engconstents.ENG_AT_SETSPFRQ1 + "1,"
                            + mET.getEditableText().toString().trim(), "atchannel0");
                    mAnalysisResponse = analysisResponse(mATResponse, SETLOCKFRQ);
                    if (mAnalysisResponse.equals(IATUtils.AT_OK)) {
                        mUiThread.post(new UnLockFreqATSuccessRunnable());
                    } else {
                        mUiThread.post(new UnLockFreqATFailRunnable());
                    }
            }
        }
    }

    class UnLockFreqATFailRunnable implements Runnable {

        @Override
        public void run() {
            Toast.makeText(UnlockFreqActivity.this, "Fail", Toast.LENGTH_SHORT).show();
        }

    }

    class UnLockFreqATSuccessRunnable implements Runnable {

        @Override
        public void run() {
            Toast.makeText(UnlockFreqActivity.this, "Success", Toast.LENGTH_SHORT).show();
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
