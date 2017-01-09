package com.sprd.engineermode.telephony;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.util.Log;
import android.util.AttributeSet;
import android.widget.Toast;
import android.telephony.TelephonyManager;

import android.app.AlertDialog;
import android.content.DialogInterface;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class CellBroadcastActivity extends PreferenceActivity implements
		Preference.OnPreferenceChangeListener {
	private static final String TAG = "AppSettingsPrefActivity";

	private static final int STATUS_OFF = 1;
	private static final int STATUS_ON = 0;

	private EMSwitchPreference[] mCellBroadcast = null;

	private static final int GET_CELL_BROADCAST = 0;
	private static final int SET_CELL_BROADCAST = 1;

	private AppSettingHandler mAppSettingHandler;
	private Handler mUiThread = new Handler(){

		public void handleMessage(Message msg) {
			int simindex = msg.arg1;
			switch(msg.what){
				case GET_CELL_BROADCAST:
					boolean isCheck = msg.arg2==1?true:false;
					Log.d(TAG,"setui sim" + simindex +"status" + String.valueOf(isCheck));
					mCellBroadcast[simindex].setChecked(isCheck);
					break;
			}
		}
	};
	private String mStrTmp = null;

	private class AppSettingHandler extends Handler {
		public AppSettingHandler(Looper looper) {
			super(looper);
		}

		@Override
		public void handleMessage(Message msg) {
			int simindex = msg.arg1;
			switch (msg.what) {
			case GET_CELL_BROADCAST:
				mStrTmp = IATUtils.sendATCmd(engconstents.ENG_AT_GETCB,
						"atchannel"+String.valueOf(simindex));
				if (mStrTmp.contains(IATUtils.AT_OK)) {
					String[] str1 = mStrTmp.split(":");
					Log.d(TAG, str1[1]);
					String[] str2 = str1[1].split(",");
					Log.d(TAG, str2[0]);
					if ((str2[0] != null) && (str2[0].contains("0"))) {
						Log.d(TAG,"get sim" + simindex +"status on");
						mUiThread.sendMessage(mUiThread.obtainMessage(msg.what, simindex, 1));
					} else {
						Log.d(TAG,"get sim" + simindex +"status off");
						mUiThread.sendMessage(mUiThread.obtainMessage(msg.what, simindex, 0));
					}
				}
				break;

			case SET_CELL_BROADCAST:
				mStrTmp = IATUtils.sendATCmd(engconstents.ENG_AT_SETCB
						+ msg.arg2, "atchannel"+String.valueOf(simindex));

				if (mStrTmp.contains(IATUtils.AT_OK)) {
					mUiThread.post(new AppATSuccessRunnable());
				} else {
					mUiThread.post(new AppATFailRunnable(msg));
				}
				break;
			default:
				return;
			}
		}
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
		PreferenceGroup preGroup = getPreferenceScreen();
		mCellBroadcast = new EMSwitchPreference[TelephonyManager.from(this)
				.getPhoneCount()];

		for (int i = 0; i < TelephonyManager.from(this).getPhoneCount(); i++) {
			mCellBroadcast[i] = new EMSwitchPreference(this,null);

			mCellBroadcast[i].setTitle("Sim" + i);
			mCellBroadcast[i].setOnPreferenceChangeListener(this);

			TelephonyManager telMgr = (TelephonyManager) TelephonyManager.from(this);

			if (telMgr == null
					|| telMgr.getSimState(i) != TelephonyManager.SIM_STATE_READY) {
				mCellBroadcast[i].setEnabled(false);
			}
			preGroup.addPreference(mCellBroadcast[i]);
		}

		HandlerThread ht = new HandlerThread(TAG);
		ht.start();
		mAppSettingHandler = new AppSettingHandler(ht.getLooper());

		for (int i = 0; i < TelephonyManager.from(this).getPhoneCount(); i++) {
			if(mCellBroadcast[i] != null
					&&mCellBroadcast[i].isEnabled()){

				Message cellBroadcast = mAppSettingHandler
						.obtainMessage(GET_CELL_BROADCAST,i,0);
				mAppSettingHandler.sendMessage(cellBroadcast);
			}
		}
	}

	public boolean onPreferenceChange(Preference preference, Object newValue) {

		for(int i = 0;i<mCellBroadcast.length;i++){
			if(mCellBroadcast[i] == preference){
				int statusWant = STATUS_ON;
				if (mCellBroadcast[i].isChecked()) {
					statusWant = STATUS_OFF;
				}
				Message m = mAppSettingHandler.obtainMessage(SET_CELL_BROADCAST,
						i, statusWant, mCellBroadcast[i]);
				mAppSettingHandler.sendMessage(m);
			}
		}

		return true;
	}

	class AppATFailRunnable implements Runnable {
		private int arg1 = 0;
		private Object obj = null;

		public AppATFailRunnable(Message msg) {
			arg1 = msg.arg1;
			obj = msg.obj;
		}

		@Override
		public void run() {
			Toast.makeText(CellBroadcastActivity.this, "Fail",
					Toast.LENGTH_SHORT).show();
			for(EMSwitchPreference cellBroadcast:mCellBroadcast){
				if(cellBroadcast == obj){
					cellBroadcast.setChecked(arg1 == STATUS_OFF ? true : false);
				}
			}
		}

	}

	class AppATSuccessRunnable implements Runnable {

		@Override
		public void run() {
			Toast.makeText(CellBroadcastActivity.this, "Success",
					Toast.LENGTH_SHORT).show();
		}

	}

	@Override
	protected void onDestroy() {
		if (mAppSettingHandler != null) {
			Log.d(TAG, "HandlerThread has quit");
			mAppSettingHandler.getLooper().quit();
		}
		super.onDestroy();
	}

	@Override
	public void onBackPressed() {
		finish();
		super.onBackPressed();
	}
}
