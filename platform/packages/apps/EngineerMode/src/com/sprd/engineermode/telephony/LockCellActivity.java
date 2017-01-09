
package com.sprd.engineermode.telephony;

import android.app.Activity;
import android.util.Log;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import android.os.Bundle;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.charset.Charset;

import android.util.Log;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.widget.Button;
import android.view.View;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class LockCellActivity extends Activity {
    private static final String TAG = "LockCellActivity";
    private GetLockCellHandler unLockCellHandler;
    private Button mButton;
    private static EditText[] mET = new EditText[16];
    private static String[] ss = new String[16];

    private static final int GETLOCKCELL = 1;

    private Handler mUiThread = new Handler();
    private String mATResponse;
    private String mAnalysisResponse;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.lockcell);

        mET[0] = (EditText) findViewById(R.id.editText1);
        mET[1] = (EditText) findViewById(R.id.editText11);
        mET[2] = (EditText) findViewById(R.id.editText12);
        mET[3] = (EditText) findViewById(R.id.editText13);

        mET[4] = (EditText) findViewById(R.id.editText2);
        mET[5] = (EditText) findViewById(R.id.editText21);
        mET[6] = (EditText) findViewById(R.id.editText22);
        mET[7] = (EditText) findViewById(R.id.editText23);

        mET[8] = (EditText) findViewById(R.id.editText3);
        mET[9] = (EditText) findViewById(R.id.editText31);
        mET[10] = (EditText) findViewById(R.id.editText32);
        mET[11] = (EditText) findViewById(R.id.editText33);

        mET[12] = (EditText) findViewById(R.id.editText4);
        mET[13] = (EditText) findViewById(R.id.editText41);
        mET[14] = (EditText) findViewById(R.id.editText42);
        mET[15] = (EditText) findViewById(R.id.editText43);

        mButton = (Button) findViewById(R.id.Button_get);
        mButton.setText("Get data");
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        unLockCellHandler = new GetLockCellHandler(ht.getLooper());
        mButton.setOnClickListener(new Button.OnClickListener() {

            public void onClick(View v) {
                Message m = unLockCellHandler.obtainMessage(GETLOCKCELL);
                unLockCellHandler.sendMessage(m);
            }

        });
    }

    private String analysisResponse(String response, int type) {
        if (response.contains(IATUtils.AT_OK)) {
            String[] str = response.split("\n");
            Log.e(TAG, str[0]);
            String strtemp2 = str[0].replace("+SPFRQ:", ",").substring(1);
            Log.e(TAG, strtemp2);
            ss = strtemp2.split(",");
            return IATUtils.AT_OK;
        }
        return IATUtils.AT_FAIL;

    }

    private class GetLockCellHandler extends Handler {
        public GetLockCellHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {

            switch (msg.what) {
                case GETLOCKCELL:
                    mATResponse = IATUtils.sendATCmd(engconstents.ENG_AT_GETSPFRQ1, "atchannel0");
                    mAnalysisResponse = analysisResponse(mATResponse, GETLOCKCELL);
                    if (mAnalysisResponse.equals(IATUtils.AT_OK)) {
                        mUiThread.post(new Runnable() {

                            @Override
                            public void run() {
                                for (int i = 0; i < 16; i++) {
                                    mET[i].setText(ss[i]);
                                }
                            }

                        });
                        mUiThread.post(new LockCellATSuccessRunnable());
                    } else {
                        mUiThread.post(new LockCellATFailRunnable());
                    }

                    break;
            }
        }
    }

    class LockCellATFailRunnable implements Runnable {

        @Override
        public void run() {
            Toast.makeText(LockCellActivity.this, "Fail", Toast.LENGTH_SHORT).show();
        }

    }

    class LockCellATSuccessRunnable implements Runnable {

        @Override
        public void run() {
            Toast.makeText(LockCellActivity.this, "Success", Toast.LENGTH_SHORT).show();
        }

    }

}
