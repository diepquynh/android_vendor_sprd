package com.sprd.engineermode.telephony;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.view.View;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;
import java.lang.String;
import java.util.Timer;
import java.util.TimerTask;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import android.widget.LinearLayout;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.R;
import android.widget.ArrayAdapter;

public class CategaryActivity extends Activity {

    private static final int GET_CATEGARY_VALUE = 0;
    private static final String TAG = "CategaryActivity";

    private String cat3Value = "Cat3/4: ";
    private CategaryHandler mCategaryHandler;
    private Handler mUiThread = new Handler();
    private TextView txtViewlabel01;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.categary_result);
        txtViewlabel01 = (TextView) findViewById(R.id.categray_result);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mCategaryHandler = new CategaryHandler(ht.getLooper());
        Message categaryValue = mCategaryHandler
                .obtainMessage(GET_CATEGARY_VALUE);
        mCategaryHandler.sendMessage(categaryValue);
    }

    @Override
    protected void onDestroy() {
        if (mCategaryHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mCategaryHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    class CategaryHandler extends Handler {
        public CategaryHandler(Looper looper) {
            super(looper);
        }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
            case GET_CATEGARY_VALUE:
                String mATCmd;
                String result = null;
                mATCmd = engconstents.ENG_AT_CATEGARY;
                result = IATUtils.sendATCmd(mATCmd, "atchannel0");
                Log.d(TAG, engconstents.ENG_AT_CATEGARY + ": " + result);
                if (result != null && result.contains(IATUtils.AT_OK)) {
                    String[] str1 = result.split("\n");
                    String[] str2 = str1[0].split(":");
                    cat3Value = cat3Value + str2[1];
                } else {
                    cat3Value = cat3Value + "NA";
                    Log.d(TAG, "AT abnormal return value");
                }
                mUiThread.post(new Runnable() {
                    @Override
                    public void run() {
                        txtViewlabel01.setText(cat3Value);
                    }
                });
                break;
            default:
                break;
            }
        }
    }
}
