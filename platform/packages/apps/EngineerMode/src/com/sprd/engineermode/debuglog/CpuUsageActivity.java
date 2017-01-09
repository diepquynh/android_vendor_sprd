package com.sprd.engineermode.debuglog;


import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.Button;
import android.view.View;

import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.ShellUtils;

import android.os.Handler;
import android.os.Message; 
import android.app.ProgressDialog; 
import android.view.View.OnClickListener;

public class CpuUsageActivity extends Activity{

    private static final String[] SORTCPU = { "/system/bin/top", "-n", "1","-t","-s","cpu"};
    private String mResult = null;
    private TextView mText;
    private Button bnOnOff = null;
    private static final int UPDATE = 1;
    private boolean paused = false;
    private ProgressDialog progressDialog;  
    Message message = null;  


    private Handler handler = new Handler(){  
        @Override  
        public void handleMessage(Message msg) {  
            switch(msg.what){  
                case UPDATE:  
                    if(!paused){
                        progressDialog.dismiss();
                        String result = (String)msg.obj;
                        if (result != null){
                            mText.setText(result);
                        } else {
                            mText.setText("");
                        }
                    }
                    break;  
            }  
        }  
    };  

    private Runnable runnable = new Runnable() {
        @Override
        public void run() {
            while(!paused){
                mResult = ShellUtils.run(SORTCPU);
                message = handler.obtainMessage(UPDATE, mResult);  
                handler.sendMessageDelayed(message,1000);
            }
        }
    };

    class BnOnClick implements OnClickListener{
        @Override
        public void onClick(View arg0) {
            paused = false;
            new Thread(runnable).start();
            bnOnOff.setText(R.string.usagetopoff);
            bnOnOff.setBackground(getResources().getDrawable(R.drawable.btn_pause));
            bnOnOff.setOnClickListener(new BnOffClick());
        }

    }

    class BnOffClick implements OnClickListener{
        @Override
        public void onClick(View arg0) {
            paused = true;
            bnOnOff.setText(R.string.usagetopon);
            bnOnOff.setBackground(getResources().getDrawable(R.drawable.btn_refresh));
            bnOnOff.setOnClickListener(new BnOnClick());
        }

    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.usagetop);
        mText = (TextView) findViewById(R.id.usagetop);
        progressDialog = ProgressDialog.show(this, "Loading...", "Please wait...", true, false); 
        bnOnOff = (Button)findViewById(R.id.btn_OnOff);
        bnOnOff.setOnClickListener(new BnOffClick());
    } 

    @Override
    protected void onResume() {
        super.onResume();
        paused = false;
        new Thread(runnable).start();
    }

    @Override
    protected void onPause() {
        paused = true;
        super.onPause();
    }
}
