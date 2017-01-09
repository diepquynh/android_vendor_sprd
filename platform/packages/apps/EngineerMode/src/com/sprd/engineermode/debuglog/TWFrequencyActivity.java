package com.sprd.engineermode.debuglog;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.TwoStatePreference;
import android.preference.SwitchPreference;
import android.util.Log;
import android.widget.Toast;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.EMSwitchPreference;

public class TWFrequencyActivity extends PreferenceActivity implements
Preference.OnPreferenceChangeListener{

	private static final String TAG = "TWFrequencyActivity";
	private static final String KEY_TW_FREQUENCY = "tw_frequency";
	private static final String KEY_TW_QUERY_SIM = "tw_frequency_sim";

	private static final int UNLOCK_FREQUENCY = 1;
	private static final int LOCK_FREQUENCY = 2;

	private String mTR;
	private String mFrequency;
	private int mFrequencyCount = 0;
	private String mATCmd;
	private int mFrequencyIndex;
	private int mSIM = 0; 
	private String mServerName = "atchannel0";

	private TwoStatePreference[] mFrequencySwitch;

	PreferenceGroup mPreGroup = null;

	private Handler mUiThread = new Handler();
	private TWFHandler mTWFHandler;

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		HandlerThread ht = new HandlerThread(TAG);
		ht.start();
		mTWFHandler = new TWFHandler(ht.getLooper());

		setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
		mPreGroup = getPreferenceScreen();
		Bundle extras = this.getIntent().getExtras();
		if(extras == null){
			return;
		}
		mTR = extras.getString(KEY_TW_FREQUENCY);
		mSIM = extras.getInt(KEY_TW_QUERY_SIM);
		mServerName = "atchannel"+mSIM;
		if(mTR.contains(IATUtils.AT_OK)){
			String[] str = mTR.split("\\+");
			mFrequencySwitch = new EMSwitchPreference[str.length];
			for(int i=1;i<str.length;i++){
				String[] str1 = str[i].split("\\:");
				String[] str2 = str1[1].split("\\,");
				mFrequency = str2[0].trim();
				if(!mFrequency.equals("0") && mFrequency != null){
					mFrequencySwitch[mFrequencyCount] = new EMSwitchPreference(this,null);
					mFrequencySwitch[mFrequencyCount].setOnPreferenceChangeListener(this); 
					mFrequencySwitch[mFrequencyCount].setKey(mSIM+KEY_TW_FREQUENCY+mFrequencyCount);
					mFrequencySwitch[mFrequencyCount].setTitle(mFrequency);
					mFrequencySwitch[mFrequencyCount].setDefaultValue(false);
					mFrequencyCount++;
				}
			}
		}
		Log.d(TAG,"There are "+mFrequencyCount+"Frequency");
		if(mFrequencyCount != 0){
			for(int i=0;i<mFrequencyCount;i++){
				mPreGroup.addPreference( mFrequencySwitch[i]);
			}  
		}else{
			AlertDialog alertDialog = new AlertDialog.Builder(this)
			.setTitle(getString(R.string.frequency_set))
			.setMessage(getString(R.string.empty_frequency))
			.setNegativeButton(R.string.alertdialog_cancel, 
					new DialogInterface.OnClickListener() {
				@Override
				public void onClick(DialogInterface dialog, int which) {
					finish();
				}
			}).create();
			alertDialog.show();
		}
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		if(mTWFHandler != null){
			Log.d(TAG,"HandlerThread has quit");
			mTWFHandler.getLooper().quit();
		} 
		super.onDestroy();
	}

	@Override
	public void onBackPressed() {
		// TODO Auto-generated method stub
		finish();
		super.onBackPressed();
	}

	@Override
	public boolean onPreferenceChange(Preference pref, Object newValue){
		for (int i=0;i<mFrequencyCount;i++){
			if(pref == mFrequencySwitch[i]){
				String title = (String)mFrequencySwitch[i].getTitle();
				if(mFrequencySwitch[i].isChecked()){
					Message unLockF = mTWFHandler.obtainMessage(UNLOCK_FREQUENCY, i, mSIM, title);                    
					mTWFHandler.sendMessage(unLockF);  
				}else{
					Message lockF = mTWFHandler.obtainMessage(LOCK_FREQUENCY, i, mSIM, title);         
					mTWFHandler.sendMessage(lockF); 
				}
			}
		}
		return true;
	}

	class TWFHandler extends Handler {

		public TWFHandler(Looper looper) {
			super(looper);
		}

		@Override
		public void handleMessage(Message msg){
			mFrequencyIndex = msg.arg1;
			String frequency = (String)msg.obj;
			String responValue;
			switch(msg.what){
			case UNLOCK_FREQUENCY:{
				mATCmd = engconstents.ENG_AT_SPFRQ+"1,"+mFrequencyIndex+","+frequency;
				responValue = IATUtils.sendATCmd(mATCmd,mServerName);           
				Log.d(TAG,"<"+mSIM+"> Channel is "+mServerName+",UNLOCK_FREQUENCY is "+mATCmd+", responValue is "+responValue);
				if(responValue.contains(IATUtils.AT_OK)){
					mUiThread.post(new Runnable() {
						@Override
						public void run() {
							mFrequencySwitch[mFrequencyIndex].setChecked(false);
							Toast.makeText(TWFrequencyActivity.this, "Success", Toast.LENGTH_SHORT).show();
						}
					});
				}else{
					mUiThread.post(new Runnable() {
						@Override
						public void run() {
							mFrequencySwitch[mFrequencyIndex].setChecked(true);
							Toast.makeText(TWFrequencyActivity.this, "Fail", Toast.LENGTH_SHORT).show();
						}
					}); 
				}
				break;
			}
			case LOCK_FREQUENCY:{
				mATCmd = engconstents.ENG_AT_SPFRQ+"0,"+mFrequencyIndex+","+frequency;
				responValue = IATUtils.sendATCmd(mATCmd,mServerName);                  
				Log.d(TAG,"<"+mSIM+">Channel is "+mServerName+",LOCK_FREQUENCY is "+mATCmd+", responValue is "+responValue);
				if(responValue.contains(IATUtils.AT_OK)){
					mUiThread.post(new Runnable() {
						@Override
						public void run() {
							mFrequencySwitch[mFrequencyIndex].setChecked(true);
							Toast.makeText(TWFrequencyActivity.this, "Success", Toast.LENGTH_SHORT).show();
						}
					});
				}else{
					mUiThread.post(new Runnable() {
						@Override
						public void run() {
							mFrequencySwitch[mFrequencyIndex].setChecked(false);
							Toast.makeText(TWFrequencyActivity.this, "Fail", Toast.LENGTH_SHORT).show();
						}
					}); 
				}
				break;
			}
			}
		}
	}
}