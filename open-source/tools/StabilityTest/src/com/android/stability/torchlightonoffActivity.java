package com.android.stability;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.content.Context;
import android.os.Handler;
import android.os.Message;

public class torchlightonoffActivity extends Activity implements OnClickListener {

	private Button torchlight_back_button = null;
        private engfetch  test;
        private int isstart;
        
        private static final int KILLER = 1000;
    private static final int SNOOZE = 2000;
    private int count = 10000;
    
      public Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
        
          if(isstart==0)
          {
             return;
          
          }
            switch (msg.what) {
                case KILLER:
                    tstop();
                    
                    break;
                case SNOOZE:
                   tstart();
                   break;
            }
        }
    };
        
    public static final String TAG = "torchlight_onoff_Activity";
	@Override
	public void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.torchlightonoff_layout);
		 
                torchlight_back_button  = (Button)findViewById(R.id.torchlight_back_button);    
                torchlight_back_button.setOnClickListener(this);
                test=new engfetch();
		 test.flashon(1);
		 isstart=1;
		 
		    mHandler.sendEmptyMessageDelayed(KILLER, 1000*2);
		 
		
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, 
        		WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
	}
	
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		if(v.equals(torchlight_back_button)){
		        test.flashoff(0);
		        mHandler.removeMessages(SNOOZE);
			mHandler.removeMessages(KILLER);
			finish();
		}
		}
       protected void tstart() {
                 
		 test.flashon(1);
		 if(isstart==1)
		 {
			 mHandler.sendEmptyMessageDelayed(KILLER, 1000*2);
		 }
		  
	}
       protected void tstop() {
		test.flashoff(0);
		 if(isstart==1 && count > 1)
		 {
			count--;
			mHandler.sendEmptyMessageDelayed(SNOOZE, 1000*2);
		}
	}
}
        
