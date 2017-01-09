package com.android.stability;

import android.app.Activity;
import android.graphics.Color;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;

public class LcdActivity extends Activity implements OnClickListener{

	private int mcurColor = 0;
	private int actionDownNum = 0;
	private View mlayout = null;
	private CountDownTimer cTimer = null;
	private long mtimeOut = 0;
	private Button mbtn_back = null;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.lcd_layout);
		
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		
		mlayout = findViewById(R.id.layout_id);
		//mlayout.setOnTouchListener(mListener);
		
		mbtn_back = (Button)findViewById(R.id.btn_back);
		mbtn_back.setOnClickListener(this);


		mtimeOut = java.lang.Long.MAX_VALUE - 10;
		Log.d("lcdtest", "timeoutis_max=" + mtimeOut);
		
		cTimer = new CountDownTimer(mtimeOut,2000) {
			
			@Override
			public void onTick(long millisUntilFinished) {
				// TODO Auto-generated method stub
				
				if(mcurColor >= 6){
					mcurColor = 0;
				}
				changeColor(mcurColor);
				mcurColor++;
			}
			
			@Override
			public void onFinish() {
				// TODO Auto-generated method stub
				mtimeOut = java.lang.Long.MAX_VALUE - 10;
				cTimer.start();
			}
		}.start();
	}
	
	View.OnTouchListener mListener = new View.OnTouchListener() {
		
		@Override
		public boolean onTouch(View v, MotionEvent event) {
			// TODO Auto-generated method stub
			
			if(event.getAction() == MotionEvent.ACTION_DOWN){
				
				cTimer.start();
				
				if(actionDownNum > 1){
					cTimer.cancel();
				}
				actionDownNum++;
				
				return true;
			}
			
			return false;
		}
	}; 
	
	private void changeColor(int color){
		Log.d("lcdtest", "getchangeColor=" + color);
		switch(color){
		case 0:
			Log.d("lcdtest", "changeColor.RED");
			mlayout.setBackgroundColor(Color.RED);
			break;
		case 1:
			Log.d("lcdtest", "changeColor.BLACK");
			mlayout.setBackgroundColor(Color.BLACK);
			break;
		case 2:
			Log.d("lcdtest", "changeColor.WHITE");
			mlayout.setBackgroundColor(Color.WHITE);
			break;
		case 3:
			Log.d("lcdtest", "changeColor.YELLOW");
			mlayout.setBackgroundColor(Color.YELLOW);
			break;
		case 4:
			Log.d("lcdtest", "changeColor.BLUE");
			mlayout.setBackgroundColor(Color.BLUE);
			break;
		case 5:
			Log.d("lcdtest", "changeColor.GREEN");
			mlayout.setBackgroundColor(Color.GREEN);
			break;
		default:
			Log.d("lcdtest", "changeColor_REDRED");
			mlayout.setBackgroundColor(Color.RED);
			break;
		}
	}
	
	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		if(cTimer != null){
			cTimer.cancel();
		}
		super.onDestroy();
	}

	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		if(v.equals(mbtn_back)){
			finish();
		}
	}
}
