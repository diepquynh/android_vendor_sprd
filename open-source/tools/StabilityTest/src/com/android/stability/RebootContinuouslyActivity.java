package com.android.stability;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.MotionEvent;
import android.widget.ImageView;
import android.widget.Toast;
import android.content.SharedPreferences;

public class RebootContinuouslyActivity extends Activity implements OnTouchListener{
    private static final String TAG = "RebootContinuouslyActivity";
	private boolean isVolumeDown = false;
	private ImageView myImageView;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.reboot_continuously);
		myImageView = (ImageView) findViewById(R.id.myImageView);
        //myImageView.setImageResource(R.drawable.ic_launcher);
        myImageView.setOnTouchListener(this); 
		Thread rebootThread = new Thread(runnable);
    	rebootThread.start();
    }
	
	Runnable runnable = new Runnable(){
		public void run() {
			// TODO Auto-generated method stub
			while(true){
				try {
					Thread.sleep(5000);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				if(isVolumeDown == false){
				rebootBroadcast();
				}
			}
		}};	

	private void rebootBroadcast(){
		Intent intent = new Intent(Intent.ACTION_REBOOT);
		intent.putExtra("nowait", 1);
		intent.putExtra("interval", 1);
		intent.putExtra("window", 0);
		sendBroadcast(intent);
	 }	
	
    public boolean onTouch(View v, MotionEvent event) {
    	// TODO Auto-generated method stub
    	if(event.getAction() == MotionEvent.ACTION_DOWN){
			SharedPreferences mPrefs = getSharedPreferences("reboot_pref", MODE_WORLD_READABLE|MODE_WORLD_WRITEABLE);
			SharedPreferences.Editor ed = mPrefs.edit();
			ed.putInt("reboot_enable", 0);
			ed.commit();
    		isVolumeDown = true;
			Log.v(TAG,"isVolumeDown = "+String.valueOf(isVolumeDown));
			Toast.makeText(this, R.string.usr_cancel_reboot, Toast.LENGTH_SHORT).show();
			finish();
			return true;
    	}
    	return false;
    }
		
}

