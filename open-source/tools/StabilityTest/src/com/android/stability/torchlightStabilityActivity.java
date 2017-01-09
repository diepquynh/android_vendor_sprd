package com.android.stability;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.content.Intent;


public class torchlightStabilityActivity extends Activity implements OnClickListener {

        private Button torchlight_onoff_button = null;
	private Button torchlight_long_button = null;
	private Button torchlight_back_button = null;

    public static final String TAG = "Vibrator_Activity";
	@Override
	public void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.torchlight_layout);
		
		torchlight_onoff_button = (Button)findViewById(R.id.torchlight_onoff);
                torchlight_long_button = (Button)findViewById(R.id.torchlight_long);  
                torchlight_back_button  = (Button)findViewById(R.id.torchlight_back_button0);     
               
        
        torchlight_onoff_button.setOnClickListener(this);
        torchlight_long_button.setOnClickListener(this);
        torchlight_back_button.setOnClickListener(this);
        
        
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, 
        		WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
	}
	
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		if(v.equals(torchlight_onoff_button)){
			Intent i = new Intent();
			i.setClass(this, torchlightonoffActivity.class);
			startActivity(i);
		}
		else if(v.equals(torchlight_long_button)){
			Intent i = new Intent();
			i.setClass(this, torchlightlongActivity.class);
			startActivity(i);
		}
		else if(v.equals(torchlight_back_button)){
			finish();
		}
		}
}
        
