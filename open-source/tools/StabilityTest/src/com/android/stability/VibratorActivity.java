package com.android.stability;

import android.app.Activity;
import android.os.Bundle;
import android.os.Vibrator;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.os.PowerManager;
import android.os.IPowerManager;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
public class VibratorActivity extends Activity implements OnClickListener {

	private static final long[] mVibratePattern = new long[] {1000, 2000};//Õ£÷π1√Î ≤•∑≈2√Î
	private Button vibrator_fail_button = null;
	private Button vibrator_succeed_button = null;
	private Button vibrator_stop_button = null;
	protected PowerManager.WakeLock mWakeLock;
	private Button vibrator_back_button = null;	
	private engfetch  test;
	private int isstart;
    /** Vibration for key press. */
    //rivate Vibrator mVibrator = null;
    private TextView my_textview;
	
    public static final String TAG = "Vibrator_Activity";
    private static final int KILLER = 1000;
    public static final int SNOOZE = 2000;
    
      public Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
        
          if(isstart==0)
          {
             return;
          
          }
            switch (msg.what) {
                case KILLER:
                    vstop();
                    
                    break;
                case SNOOZE:
                   vstart();
                   break;
            }
        }
    };
    
    
    
    
    
    
	@Override
	public void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		
	
            PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
            mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);
                    mWakeLock.acquire();
                  Log.i(TAG, "mWakeLock ");  
		super.onCreate(savedInstanceState);
		test=new engfetch();
		setContentView(R.layout.vibrator_layout);
		
		vibrator_fail_button = (Button)findViewById(R.id.vibrator_fail);
        vibrator_succeed_button = (Button)findViewById(R.id.vibrator_succeed);        
        vibrator_stop_button = (Button)findViewById(R.id.vibrator_stop);
        vibrator_back_button = (Button)findViewById(R.id.vibrator_back_button);         
        
        vibrator_fail_button.setOnClickListener(this);
        vibrator_succeed_button.setOnClickListener(this);
        vibrator_stop_button.setOnClickListener(this);
        vibrator_back_button.setOnClickListener(this);
        
       
        test.vibratortest(1);
         isstart=1;
         mHandler.sendEmptyMessageDelayed(KILLER, 1000*2);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, 
        		WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
	}
	
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		if(v.equals(vibrator_fail_button)){
			test.vibratorstop(0); 
			mHandler.removeMessages(SNOOZE);
			mHandler.removeMessages(KILLER); 
			mWakeLock.release();
			finish();
		}
		else if(v.equals(vibrator_succeed_button)){

			test.vibratorstop(0);
			mHandler.removeMessages(SNOOZE);
			mHandler.removeMessages(KILLER); 
			mWakeLock.release();
			finish();
			
		}
		else if(v.equals(vibrator_stop_button)){
			test.vibratorstop(0);
			isstart=0;
			mHandler.removeMessages(SNOOZE);
			mHandler.removeMessages(KILLER); 
			mWakeLock.release();
			my_textview = (TextView)findViewById(R.id.vibrator_note);
			String str = getString(R.string.vibrator_stop).toString();
			my_textview.setText(str);
		}
		else if(v.equals(vibrator_back_button)){
			test.vibratorstop(0);
			isstart=0;
			mHandler.removeMessages(SNOOZE);
			mHandler.removeMessages(KILLER); 
			mWakeLock.release();
			finish();
		}
	}
	
	@Override
	protected void onPause() {
		// TODO Auto-generated method stub
		super.onPause();
	}
       	
       protected void vstart() {
                 
		 test.vibratortest(1);
		 if(isstart==1)
		 mHandler.sendEmptyMessageDelayed(KILLER, 1000*2);
		 
	}
       protected void vstop() {
		test.vibratorstop(0);
		 if(isstart==1)
		mHandler.sendEmptyMessageDelayed(SNOOZE, 1000*1);
	}

}
