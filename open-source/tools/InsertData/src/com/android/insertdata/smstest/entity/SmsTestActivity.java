package com.android.insertdata.smstest.entity;


import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.view.KeyEvent;
import android.view.View;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import com.android.insertdata.R;
import com.android.insertdata.addmms.SmsAbility;
import com.android.insertdata.smstest.thread.InsertDbThread;

public class SmsTestActivity extends SmsAbility {
	public EditText phoneNoEt;
	public CheckBox autoUpCb;
	public EditText smsContentEt;
	public EditText countEt;
	
	public EditText threadCountEt;
	
	public RadioButton isReadRb;
	public RadioButton notReadRb;
	public RadioGroup isReadGroup;
	
	/*public RadioButton simRb1;
	public RadioButton simRb2;
	public RadioGroup simSelGroup;
	
	public RadioButton isThreadRb;
	public RadioButton nothreadRb;
	public RadioGroup isthreadGroup;
	public EditText threadIdEt;*/
	
	public RadioButton inboxRb;
	public RadioButton sentRb;
	public RadioButton draftRb;
	public RadioButton failRb;
	public RadioButton outboxRb;
	public RadioButton queueRb;
	public RadioGroup boxGroup;
	
	
	ProgressDialog insertProcessDialog;
	
	private ToneGenerator mToneGenerator;
	private static final int TONE_LENGTH_MS = 200;
	    
	    /** The DTMF tone volume relative to other sounds in the stream */
    private static final int TONE_RELATIVE_VOLUME = 80;

	    /** Stream type used to play the DTMF tones off call, and mapped to the volume control keys */
	 private static final int DIAL_TONE_STREAM_TYPE = AudioManager.STREAM_MUSIC;
		 
	 private static final int SHOWDIALOG = 2; 
		
     private ProgressDialog showMessage;
     static final String LOG_TAG = "AddCall";
     private PowerManager.WakeLock mWakeock;
     public MyHandler myHandle ;
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.sms);
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mWakeock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK
                , LOG_TAG);
        //mWakeock.acquire();
        if (mToneGenerator == null) {
            try {
                mToneGenerator = new ToneGenerator(DIAL_TONE_STREAM_TYPE, TONE_RELATIVE_VOLUME);
                setVolumeControlStream(DIAL_TONE_STREAM_TYPE);
            } catch (RuntimeException e) {
                mToneGenerator = null;
            }
            if(showMessage==null){
            	showMessage = new ProgressDialog(SmsTestActivity.this);
            }
        }
        
        myHandle = new MyHandler();
    }
    
    public void applyHandler(View v)
    {
        if ( !checkDefaultSmsApp() ) {
            return;
        }
    	getView();
    	SmsConfig smsConfig = new SmsConfig();
    	if(phoneNoEt.getText().toString()==null||"".equals(phoneNoEt.getText().toString())){
    		smsConfig.phoneNo ="13800138000".trim();
//    		Log.e("xxxxxxx","phoneNoEt.getText().toString()==null============="+smsConfig.phoneNo);
    		
    	}else{
    		smsConfig.phoneNo=phoneNoEt.getText().toString();
//    		Log.e("xxxxxxx","phoneNoEt.getText().toString()============="+smsConfig.phoneNo);
    	}
    	if(smsContentEt.getText().toString()==null||"".equals(smsContentEt.getText().toString())){
    		smsConfig.smsContent=SmsTestActivity.this.getResources().getString(R.string.sms_content_defualt);
//    		Log.e("xxxxxxx","smsContentEt.getText().toString()==null============="+smsConfig.smsContent);
    	}else{
    		smsConfig.smsContent=smsContentEt.getText().toString();
//    		Log.e("xxxxxxx","smsContentEt.getText().toString()============="+smsConfig.smsContent);
    	}
    	if(countEt.getText().toString()==null||"".equals(countEt.getText().toString())){
    		smsConfig.count=1000;
//    		Log.e("xxxxxxx","countEt.getText().toString()==100==null============="+smsConfig.count);
    	}else{
    		smsConfig.count = Integer.parseInt(countEt.getText().toString());
//    		Log.e("xxxxxxx","countEt.getText().toString()============="+smsConfig.count);
    	}
    	if(threadCountEt.getText().toString()==null||"".equals(threadCountEt.getText().toString())){
    		smsConfig.threadCount=1;
//    		Log.e("xxxxxxx","threadCountEt.getText().toString()==1000==null============="+smsConfig.threadCount);
    	}else{
    		smsConfig.threadCount = Integer.parseInt(threadCountEt.getText().toString());
//    		Log.e("xxxxxxx","threadCountEt.getText().toString()============="+smsConfig.threadCount);
    	}
    	if(isReadRb.isChecked())
    	{
    		smsConfig.isRead = true;
    	}
    	if(autoUpCb.isChecked())
    	{
    		smsConfig.isAutoUp = true;
    	}
    	switch(boxGroup.getCheckedRadioButtonId())
    	{
    		case R.id.box1:
    			SmsConfig.boxId = "1";
    			break;
    		case R.id.box2:
    			SmsConfig.boxId = "2";
    			break;
    		case R.id.box3:
    			SmsConfig.boxId = "3";
    			break;
    		default:
    			SmsConfig.boxId = "0";
    			break;
    	}
    	
    	SingleParam.threadSwitch = true;
    	try{
    		insertProcessDialog = new ProgressDialog(this);
        	insertProcessDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        	insertProcessDialog.setTitle(SmsTestActivity.this.getResources().getString(R.string.title));
        	insertProcessDialog.setIndeterminate(false);
        	insertProcessDialog.setCancelable(true);
        	insertProcessDialog.setOnCancelListener(new CanelClickListener());
        	insertProcessDialog.setButton(SmsTestActivity.this.getResources().getString(R.string.exit), new CanelSwitchListener());
        	insertProcessDialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
				
				public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
					return true;
				}
			});
        	
            mWakeock.acquire();
        	Thread t = new Thread(new InsertDbThread(smsConfig,this,this.getContentResolver(),insertProcessDialog, myHandle));
        	t.start();
        	insertProcessDialog.show();
        	
//			Log.e("xxxxxxx","--------------SmsTestActivity   t.start() -----------");
        	
        	insertProcessDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
				
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
				       synchronized(this) {
				           if (mToneGenerator == null) {
				               return;
				           }
				           mToneGenerator.startTone(ToneGenerator.TONE_DTMF_3, TONE_LENGTH_MS);
				       }								
				}
			});
        	
    	}catch(Exception e)
    	{
    		e.getStackTrace();
    		System.out.println(e.getMessage());
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
    
	private void showProgressDialog(){
		StringBuffer msg = new StringBuffer();
    	msg.append(SmsTestActivity.this.getResources().getString(R.string.count));
//    	android.util.Log.e("xxxxxxxxx","-----------------------SingleParam.status = "+SingleParam.status);
    	msg.append(SingleParam.status).append(SmsTestActivity.this.getResources().getString(R.string.bar));
//		android.util.Log.e("xxxxxxx","----------------smsconfig.count=========xxxxxxxxxxxxxxxAAAAAAAA====  "+SingleParam.status);
    	msg.append("\n");
    	msg.append(SmsTestActivity.this.getResources().getString(R.string.times));
    	msg.append(SingleParam.time);
//    	android.util.Log.e("xxxxxxxxx","----------------InsertDbThread.time=========xxxxxxxxxxxxxxxAAAAAAAA====  "+SingleParam.time);
    	msg.append(SmsTestActivity.this.getResources().getString(R.string.seconds));
		Dialog dialog = new AlertDialog.Builder(SmsTestActivity.this).setTitle(R.string.smscountshow)
		.setMessage(msg.toString())
        .setPositiveButton(R.string.ok,new DialogInterface.OnClickListener(){
			public void onClick(DialogInterface dialog, int which) {
				mToneGenerator.stopTone();
				SingleParam.status=0;
				SingleParam.time=0;
				mWakeock.release();
				//mWakeock=null;
				//finish();
			}
		}).create();
       dialog.show();
	}
    public void getView()
    {
    	phoneNoEt = (EditText) this.findViewById(R.id.phone_no);
    	// phoneNoEt.setHint(R.string.sms_phone_defualt);
    	autoUpCb = (CheckBox) this.findViewById(R.id.phone_auto_up);
    	smsContentEt = (EditText) this.findViewById(R.id.sms_content);
    	countEt = (EditText) this.findViewById(R.id.sms_count);
    	threadCountEt = (EditText) this.findViewById(R.id.thread_count);
    	
    	isReadRb = (RadioButton) this.findViewById(R.id.isread);
    	notReadRb = (RadioButton) this.findViewById(R.id.notread);
    	
    	isReadGroup = (RadioGroup) this.findViewById(R.id.isread_group);
    	inboxRb = (RadioButton) this.findViewById(R.id.box1);
    	sentRb = (RadioButton)this.findViewById(R.id.box2);
    	draftRb = (RadioButton)this.findViewById(R.id.box3);
    	boxGroup = (RadioGroup)this.findViewById(R.id.box_group);
    }
    
    class CanelSwitchListener implements DialogInterface.OnClickListener{

		public void onClick(DialogInterface dialog, int which) {
			SingleParam.threadSwitch = false;
			--SingleParam.status;
		 	android.util.Log.e("sss","----------------------- onClick(DialogInterface dialog, int which)  = "+SingleParam.status);
		}
	}
    class CanelClickListener implements OnCancelListener{

		public void onCancel(DialogInterface dialog) {
			SingleParam.threadSwitch = false;
		}
    }

	@Override
	protected void onPause() {
		super.onPause();
		
	}
    
}
