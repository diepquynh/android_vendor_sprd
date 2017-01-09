package com.android.insertdata.addmms;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.Intent;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.provider.Telephony;
import android.provider.Telephony.Sms;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Toast;

import com.android.insertdata.R;


public class AddMMSActivity extends SmsAbility {
	static final String TAG = "AddMMSActivity";
	private PowerManager.WakeLock mWakeock;

	public EditText phoneNoEdit;
	public CheckBox autoUpCb;
	public EditText addCountEdit;
	public EditText mmsSizeEdit;

	public RadioButton isReadRb;
	public RadioButton notReadRb;
	public RadioGroup isReadGroup;

	public RadioButton inboxRb;
	public RadioButton sentRb;
	public RadioButton draftRb;
	public RadioGroup boxGroup;

	ProgressDialog insertProcessDialog;

	private ToneGenerator mToneGenerator;
	private static final int TONE_LENGTH_MS = 200;
	private static final int TONE_RELATIVE_VOLUME = 80;
	private static final int DIAL_TONE_STREAM_TYPE = AudioManager.STREAM_MUSIC;

	private static final int SHOWDIALOG = 2;

	private ProgressDialog showMessage;
	public MyHandler myHandle;
	private static final int MAX_MMS_SIZE = 298;
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.add_mms);
		PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
		mWakeock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, TAG);
		mWakeock.acquire();
		SingleMmsParam.threadSwitch = false;
		SingleMmsParam.finishCount = 0;
		setReadGrayState();
		if (mToneGenerator == null) {
			try {
				mToneGenerator = new ToneGenerator(DIAL_TONE_STREAM_TYPE,TONE_RELATIVE_VOLUME);
				setVolumeControlStream(DIAL_TONE_STREAM_TYPE);
			} catch (RuntimeException e) {
				mToneGenerator = null;
			}
			if (showMessage == null) {
				showMessage = new ProgressDialog(AddMMSActivity.this);
			}
		}

		myHandle = new MyHandler();
	}

	// get current data
	public MmsConfig GetEditData() {
		MmsConfig mmsConfig = new MmsConfig();
		/*
		if (0 == phoneNoEdit.getText().length()) {
			mmsConfig.phoneNo = "13800138000".trim();
		} else {
			mmsConfig.phoneNo = phoneNoEdit.getText().toString();
		}
		mmsConfig.isAutoUp = autoUpCb.isChecked();*/
		mmsConfig.phoneNo = "13800138000".trim();
		mmsConfig.isAutoUp = true;
		if (0 == addCountEdit.getText().length()) {
			mmsConfig.mmsCount = 20;
		} else {
			mmsConfig.mmsCount = Integer.parseInt(addCountEdit.getText()
					.toString());
		}
		if (0 == mmsSizeEdit.getText().length()) {
			mmsConfig.mmsSize = 50;
		} else {
			mmsConfig.mmsSize = Integer.parseInt(mmsSizeEdit.getText()
					.toString());
			if (mmsConfig.mmsSize >= MAX_MMS_SIZE && mmsConfig.mmsSize <= 300){
				mmsConfig.mmsSize = 298;
			}
		} 
		mmsConfig.isRead = isReadRb.isChecked();
		if (inboxRb.isChecked()) {
			mmsConfig.boxId = MmsConstant.INBOX;
		} else if (sentRb.isChecked()) {
			mmsConfig.boxId = MmsConstant.SENTBOX;
		} else {
			mmsConfig.boxId = MmsConstant.DRAFTBOX;
		}
		Log.d(TAG, "GetEditData1 phoneNo:" + mmsConfig.phoneNo + " isAutoUp=" + mmsConfig.isAutoUp);
		Log.d(TAG, "GetEditData2 mmsCount=" + mmsConfig.mmsCount + " mmsSize=" + mmsConfig.mmsSize + " isRead=" + mmsConfig.isRead + " box=" + mmsConfig.boxId);
		return mmsConfig;
	}

	@SuppressWarnings("deprecation")
	public void applyMmsHandler(View v) {
	    if ( !checkDefaultSmsApp() ) {
	        return;
	    }
		getView();
		MmsConfig mmsConfig = GetEditData();
		if (0 == mmsConfig.mmsCount){
			DisplayToast("abc");
			return;
		}
		if (0 == mmsConfig.mmsSize || 300 < mmsConfig.mmsSize){
			DisplayToast("300KB");
			return;
		}
		SingleMmsParam.threadSwitch = true;
		try {
			insertProcessDialog = new ProgressDialog(this);
			insertProcessDialog
					.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
			insertProcessDialog.setTitle(AddMMSActivity.this.getResources()
					.getString(R.string.title));
			insertProcessDialog.setIndeterminate(false);
			insertProcessDialog.setCancelable(true);
			insertProcessDialog.setOnCancelListener(new CanelClickListener());
			insertProcessDialog.setButton(AddMMSActivity.this.getResources()
					.getString(R.string.exit), new CanelSwitchListener());
			insertProcessDialog
					.setOnKeyListener(new DialogInterface.OnKeyListener() {

						public boolean onKey(DialogInterface dialog,
								int keyCode, KeyEvent event) {
							return true;
						}
					});

			mWakeock.acquire();
			Thread t = new Thread(new InsertMmsDbThread(mmsConfig, this,
					this.getContentResolver(), insertProcessDialog, myHandle));
			t.start();
			insertProcessDialog.show();

			insertProcessDialog
					.setOnDismissListener(new DialogInterface.OnDismissListener() {

						public void onDismiss(DialogInterface dialog) {
							playTone();
						}

						private void playTone() {
							AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
							int ringerMode = audioManager.getRingerMode();
							if ((ringerMode == AudioManager.RINGER_MODE_SILENT)
									|| (ringerMode == AudioManager.RINGER_MODE_VIBRATE)) {

								return;
							}
							synchronized (this) {
								if (mToneGenerator == null) {
									return;
								}
								mToneGenerator.startTone(
										ToneGenerator.TONE_DTMF_3,
										TONE_LENGTH_MS);
							}
						}
					});

		} catch (Exception e) {
			e.getStackTrace();
			Log.e(TAG,"Exception "+e.getMessage());
			return;
		}
	}

	private class MyHandler extends Handler {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {

			case SHOWDIALOG:
				showProgressDialog();
				break;
			}
			super.handleMessage(msg);

		};
	}

	private void showProgressDialog() {
		StringBuffer msg = new StringBuffer();
		msg.append(AddMMSActivity.this.getResources().getString(R.string.count));
		msg.append(SingleMmsParam.finishCount).append(
				AddMMSActivity.this.getResources().getString(R.string.bar));
		msg.append("\n");
		msg.append(AddMMSActivity.this.getResources().getString(R.string.times));
		msg.append(SingleMmsParam.time);;
		msg.append(AddMMSActivity.this.getResources().getString(
				R.string.seconds));
		Dialog dialog = new AlertDialog.Builder(AddMMSActivity.this)
				.setTitle(R.string.mms_count_show)
				.setMessage(msg.toString())
				.setPositiveButton(R.string.ok,
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog,
									int which) {
								mToneGenerator.stopTone();
								SingleMmsParam.finishCount = 0;
								SingleMmsParam.time = 0;
								mWakeock.release();
								//mWakeock = null;
								//finish();
							}
						}).create();
		dialog.show();
	}

	public void getView() {
		//phoneNoEdit = (EditText) this.findViewById(R.id.phone_no);
		//autoUpCb = (CheckBox) this.findViewById(R.id.phone_auto_up);
		addCountEdit = (EditText) this.findViewById(R.id.mms_count);
		mmsSizeEdit = (EditText) this.findViewById(R.id.mms_size);

		isReadRb = (RadioButton) this.findViewById(R.id.isread);
		notReadRb = (RadioButton) this.findViewById(R.id.notread);
		isReadGroup = (RadioGroup) this.findViewById(R.id.isread_group);

		inboxRb = (RadioButton) this.findViewById(R.id.box1);
		sentRb = (RadioButton) this.findViewById(R.id.box2);
		draftRb = (RadioButton) this.findViewById(R.id.box3);
		boxGroup = (RadioGroup) this.findViewById(R.id.box_group);
	}

	class CanelSwitchListener implements DialogInterface.OnClickListener {

		public void onClick(DialogInterface dialog, int which) {
			SingleMmsParam.threadSwitch = false;
			Log.d(TAG,"CanelSwitchListener SingleMmsParam.status="+SingleMmsParam.finishCount);
		}
	}

	class CanelClickListener implements OnCancelListener {

		public void onCancel(DialogInterface dialog) {
			SingleMmsParam.threadSwitch = false;
			Log.d(TAG,"CanelClickListener");
		}
	}
	 private void setReadGrayState(){
	   
	        RadioButton radioButtonBox1 = (RadioButton) findViewById(R.id.box1);
	        radioButtonBox1.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
	            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
	                // TODO Auto-generated method stub
	                if(isChecked){
	                	setReadEditState(true);
	                }
	            }
	        });

	        RadioButton radioButtonBox2 = (RadioButton) findViewById(R.id.box2);
	        radioButtonBox2.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
	            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
	                // TODO Auto-generated method stub
	                if(isChecked){
	                	setReadEditState(false);
	                }
	            }
	        });

	        RadioButton radioButtonBox3 = (RadioButton) findViewById(R.id.box3);
	        radioButtonBox3.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
	            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
	                // TODO Auto-generated method stub
	                if(isChecked){
	                	setReadEditState(false);
	                }
	            }
	        });
	    	
	    }
	    
	    private void setReadEditState(boolean isEnable){
			RadioButton radioButton1 = (RadioButton) findViewById(R.id.isread);
			radioButton1.setEnabled(isEnable);
			RadioButton radioButton2 = (RadioButton) findViewById(R.id.notread);
			radioButton2.setEnabled(isEnable);
	    }
	@Override
	protected void onPause() {
		super.onPause();
		Log.d(TAG,"onPause");
	}
	
	public void DisplayToast(String str)
	{
		Toast.makeText(this, str, Toast.LENGTH_SHORT).show();
	}
}
