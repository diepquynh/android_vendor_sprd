package com.android.stability;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;


public class torchlightlongActivity extends Activity implements OnClickListener {

        private Button torchlight_back_button = null;
	private engfetch  test;

    public static final String TAG = "torchlightlong_Activity";
	@Override
	public void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.torchlightlong_layout);
                Log.v(TAG, "onCreate");
                test=new engfetch();
                test.flashon(1);
		 
                torchlight_back_button  = (Button)findViewById(R.id.torchlight_back_button1);     
                torchlight_back_button.setOnClickListener(this);
                
        
        
	}
	
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		
	 if(v.equals(torchlight_back_button)){
	                test.flashoff(0);
			finish();
		}
	}
		

}
        
