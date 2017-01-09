/*package com.sprd.engineermode.telephony;

import android.app.Activity;
import android.text.method.NumberKeyListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;
import android.os.Bundle;
import android.view.View;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class SendGPRSDataActivity extends Activity {
    private static final String TAG = "SendGPRSDataActivity";

    private EditText mET01;
    private EditText mET02;
    private int mInt01;

    private static final int SENDDATA = 1;

    private Button mButton;
    private Button mButton01;

    private String strInput = "";
    private Handler mUiThread = new Handler();
    private String mATResponse;
    private String mAnalysisResponse;
    private SendgprsdataSettingHandler sendgprsdataSettingHandler;

    private boolean bHasContent;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.sendgprsdata);
        initialpara();

    }

    private void initialpara() {
        bHasContent = true;
        mET01 = (EditText) findViewById(R.id.editText1);
        mET02 = (EditText) findViewById(R.id.editText2);
        mET01.setKeyListener(numberKeyListener);
        mET02.setKeyListener(numberKeyListener);
        clearEditText();
        mButton = (Button) findViewById(R.id.send_button);
        mButton01 = (Button) findViewById(R.id.clear_button1);
        mButton.setText("Send Data");
        mButton01.setText("Clear Data");
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        sendgprsdataSettingHandler = new SendgprsdataSettingHandler(ht.getLooper());

        mButton.setOnClickListener(new Button.OnClickListener() {

            public void onClick(View v) {
                Message m = sendgprsdataSettingHandler.obtainMessage(SENDDATA);
                sendgprsdataSettingHandler.sendMessage(m);
            }

        });

        mButton01.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                clearEditText();
            }

        });
    }

    private void clearEditText() {
        mET01.setText("0");
        mET02.setText("0");
    }

    private boolean getEditTextValue() {
        if ((mET01.getText().toString()).equals("")) {
            mInt01 = 0;
        } else {
            try {
                mInt01 = Integer.parseInt(mET01.getText().toString());
            } catch (NumberFormatException nfe) {
                mInt01 = 0;
                mET01.selectAll();
                mUiThread.post(new Runnable() {

                    @Override
                    public void run() {
                        Toast.makeText(SendGPRSDataActivity.this,
                                "data number is not a valid number", Toast.LENGTH_SHORT).show();
                    }

                });
                return false;
            }
        }

        if ((mET02.getText().toString()).equals("")) {
            bHasContent = false;
        } else {
            strInput = mET02.getText().toString();
        }
        return true;
    }

    private NumberKeyListener numberKeyListener = new NumberKeyListener() {
        private char[] numberChars = {
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
        };

        public int getInputType() {
            return android.text.InputType.TYPE_CLASS_PHONE;
        }

        protected char[] getAcceptedChars() {
            return numberChars;
        }
    };

    private String analysisResponse(String response, int type) {
        if (response.contains(IATUtils.AT_OK)) {
            return IATUtils.AT_OK;
        }
        return IATUtils.AT_FAIL;

    }

    private class SendgprsdataSettingHandler extends Handler {
        public SendgprsdataSettingHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case SENDDATA: {
                    if (!getEditTextValue())
                        return;
                    if (bHasContent) {
                        String atCmd = engconstents.ENG_AT_SGPRSDATA1
                                + "2,2," + "\"" + strInput + "\"";
                        mATResponse = IATUtils.sendATCmd(atCmd, "atchannel0");
                        mAnalysisResponse = analysisResponse(mATResponse, SENDDATA);
                    } else {
                        mATResponse = IATUtils.sendATCmd(engconstents.ENG_AT_SGPRSDATA1 + mInt01,
                                "atchannel0");
                        mAnalysisResponse = analysisResponse(mATResponse, SENDDATA);
                    }
                    if (IATUtils.AT_OK.equals(mAnalysisResponse)) {
                        mUiThread.post(new SendgprsdataATSuccessRunnable());
                    } else {
                        mUiThread.post(new SendgprsdataATFailRunnable());
                    }
                }
            }

        }
    }

    class SendgprsdataATFailRunnable implements Runnable {

        @Override
        public void run() {
            Toast.makeText(SendGPRSDataActivity.this, "Fail", Toast.LENGTH_SHORT).show();
        }

    }

    class SendgprsdataATSuccessRunnable implements Runnable {

        @Override
        public void run() {
            Toast.makeText(SendGPRSDataActivity.this, "Success", Toast.LENGTH_SHORT).show();
        }

    }

}
*/