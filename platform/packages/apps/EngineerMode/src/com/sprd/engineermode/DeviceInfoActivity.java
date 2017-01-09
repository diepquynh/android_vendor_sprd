package com.sprd.engineermode;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.TextView;

public class DeviceInfoActivity extends Activity implements View.OnClickListener{

    private static final String ADC_CALIBRATION = "adb_calibration";
    private static final String RF_CALIBRATION = "rf_calibration";
    public static final int OK_BUTTON = R.id.button_ok;
    private static final int DIALOG_DEFAULT_TIMEOUT = (30 * 1000);
    private static final int MSG_ID_TIMEOUT = 1;

    String mDialogMessage;
    private boolean isValidAdcCalibrate = true; 
    private boolean isValidRfCalibrate = true;

    Handler mTimeoutHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
                case MSG_ID_TIMEOUT:
                    finish();
                    break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);


        //requestWindowFeature(Window.FEATURE_LEFT_ICON);
        Window window = getWindow();

        setContentView(R.layout.device_info);
        TextView mMessageView = (TextView) window
                .findViewById(R.id.dialog_message);

        Button okButton = (Button) findViewById(R.id.button_ok);
        okButton.setOnClickListener(this);
        if(getIntent() != null){
            isValidAdcCalibrate = getIntent().getBooleanExtra(ADC_CALIBRATION, true);
            isValidRfCalibrate = getIntent().getBooleanExtra(RF_CALIBRATION, true);
            if(!isValidAdcCalibrate){
                if(!isValidRfCalibrate){
                    mDialogMessage = "ADC & RF Calibration Failed";
                }else{
                    mDialogMessage = "ADC Calibration Failed";
                }
            }else{
                if(!isValidRfCalibrate){
                    mDialogMessage = "RF Calibration Failed";
                }
            }
        }
        setTitle("Device Calibration Info");
        mMessageView.setText(mDialogMessage);
    }

    public void onClick(View v) {
        switch (v.getId()) {
            case OK_BUTTON:
                finish();
                break;
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                finish();
                break;
        }
        return false;
    }

    private void cancelTimeOut() {
        mTimeoutHandler.removeMessages(MSG_ID_TIMEOUT);
    }

    private void startTimeOut() {
        // Reset timeout.
        cancelTimeOut();
        mTimeoutHandler.sendMessageDelayed(mTimeoutHandler.obtainMessage(MSG_ID_TIMEOUT),
                DIALOG_DEFAULT_TIMEOUT);
    }

    @Override
    public void onResume() {
        super.onResume();
        startTimeOut();
    }

    @Override
    public void onPause() {
        super.onPause();
        cancelTimeOut();
       //finish();
    }
}