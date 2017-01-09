package com.android.stability;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.content.SharedPreferences;

public class singleItemsActivity extends Activity implements OnClickListener {

	private Button btn_vibrate = null;
	private Button btn_lcd = null;
	private Button btn_receiver = null;
	private Button btn_camera = null;
	private Button btn_back = null;
	private Button btn_torchlight_test = null;
	private Button btn_reboot = null;
	private Button btn_gsensor = null;
	private Button btn_fm = null;
	private Button btn_flash = null;
	private Button btn_tf = null;
	private Button btn_sim = null;
	
			
	@Override
	public void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.singleitem);
		
		btn_vibrate = (Button)findViewById(R.id.vibrate_test);
		btn_lcd = (Button)findViewById(R.id.Lcd_test);
		btn_receiver = (Button)findViewById(R.id.Speaker_test);
		btn_camera = (Button)findViewById(R.id.Camera_test);
		btn_reboot = (Button)findViewById(R.id.reboot_button);
		btn_back = (Button)findViewById(R.id.back_button);
		btn_torchlight_test = (Button)findViewById(R.id.torchlight_test);
		btn_gsensor  = (Button)findViewById(R.id.gsensor_button);
		btn_fm = (Button)findViewById(R.id.fm_button);
		btn_flash = (Button)findViewById(R.id.flash_button);
		btn_tf = (Button)findViewById(R.id.tf_button);
		btn_sim = (Button)findViewById(R.id.simstatus_test);
		
		btn_vibrate.setOnClickListener(this);
		btn_lcd.setOnClickListener(this);
		btn_receiver.setOnClickListener(this);
		btn_camera.setOnClickListener(this);
		btn_reboot.setOnClickListener(this);
		btn_back.setOnClickListener(this);
		btn_torchlight_test.setOnClickListener(this);
		btn_gsensor.setOnClickListener(this);
		btn_fm.setOnClickListener(this);
		btn_flash.setOnClickListener(this);
		btn_tf.setOnClickListener(this);
		btn_sim.setOnClickListener(this);
	}
	
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		if(v.equals(btn_vibrate)){
			Intent ivibrate = new Intent();
			ivibrate.setClass(this, VibratorActivity.class);
			startActivity(ivibrate);
		}
		else if(v.equals(btn_lcd)){
			Intent ilcd = new Intent();
			ilcd.setClass(this, LcdActivity.class);
			startActivity(ilcd);
		}
		else if(v.equals(btn_receiver)){
			Intent ireceiver = new Intent();
			ireceiver.setClass(this, ReceiverActivity.class);
			startActivity(ireceiver);
		}
		else if(v.equals(btn_camera)){
			Intent i = new Intent();
			i.setClass(this, cameraStabilityActivity.class);
			startActivity(i);
		}
		else if(v.equals(btn_torchlight_test)){
			Intent i = new Intent();
			i.setClass(this, torchlightStabilityActivity.class);
			startActivity(i);
		}
		else if(v.equals(btn_reboot)){
			SharedPreferences mPrefs = getSharedPreferences("reboot_pref", MODE_WORLD_READABLE|MODE_WORLD_WRITEABLE);
			SharedPreferences.Editor ed = mPrefs.edit();
			ed.putInt("reboot_enable", 1);
			ed.commit();
			Intent iReboot = new Intent(this,RebootContinuouslyActivity.class);
			iReboot.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
	        startActivity(iReboot); 
		}
		else if(v.equals(btn_fm)){
			Intent i = new Intent();
			i.setClass(this, FMTestActivity.class);
			startActivity(i);
		}
		else if(v.equals(btn_gsensor)){
			Intent i = new Intent();
			i.setClass(this, GesnsorTest.class);
			startActivity(i);
		}
		else if(v.equals(btn_flash)){
			Intent i = new Intent();
			i.setClass(this, FlashTest.class);
			startActivity(i);
		}
		else if(v.equals(btn_tf)){
			Intent i = new Intent();
			i.setClass(this, hwtest.class);
			startActivity(i);
		}
		
        else if (v.equals(btn_sim)) {
            Intent i = new Intent();
            i.setClass(this, SimTaskTestActivity.class);
            startActivity(i);
        }
		
		else if(v.equals(btn_back)){
			finish();
		}
	}

}
