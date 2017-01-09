package com.android.stability;

import java.io.FileNotFoundException;
import java.io.FileReader;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import android.hardware.fm.FmManager;

public class FMTestActivity extends Activity {

    private EditText mFreqEdit;
	private Button mSelectFreqBtn;
	private Button mSpeakerSwitchBtn;
	private TextView mPlayingDeviceText;
	private FmManager fmManager;
	private static final String RADIO_DEVICE = "/dev/radio0";
	private float freq = 88.7f;
	private static final int TRANS_MULT = 10;
	private static final String HEADSET_STATE_PATH = "/sys/class/switch/h2w/state";
    private Dialog mDialog = null;
    private static final int HEADSET_DIALOG = 1;
    private static boolean mIsUseHeadset = false;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		setContentView(R.layout.fm_layout);
	    getWindow().addFlags(WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED);
		mSelectFreqBtn = (Button)findViewById(R.id.btnSetFreq);
		mSpeakerSwitchBtn = (Button)findViewById(R.id.btnHeadset);
		mFreqEdit = (EditText)findViewById(R.id.editFreq);
		mPlayingDeviceText = (TextView)findViewById(R.id.headsetplaying);
		mSelectFreqBtn.setOnClickListener(new OnClickListener() {
            
            @Override
            public void onClick(View v) {
                changeFreq();
            }
        });
		
		mSpeakerSwitchBtn.setOnClickListener(new OnClickListener() {
            
            @Override
            public void onClick(View v) {
                switchSpeker();
            }
        });
		if(!isHeadsetExists()){
			showDialog(HEADSET_DIALOG);
		}
		
	}



    protected Dialog onCreateDialog(int id) {
        switch (id) {
            case HEADSET_DIALOG:
            {
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                mDialog = builder.setTitle(R.string.fm_dialog_tittle).setMessage(R.string.fm_dialog_message).create();
                mDialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
                    public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                           if (keyCode == KeyEvent.KEYCODE_BACK) {
                                finish();
                                if(mDialog != null)
                                	mDialog.cancel();
                                return true;
                            } else if (keyCode == KeyEvent.KEYCODE_SEARCH) {
                                return true;
                            }
                        return false;
                    }
                });
                return mDialog;
            }
        }
        return null;
    }

    private BroadcastReceiver earphonePluginReceiver = new BroadcastReceiver() {

        public void onReceive(Context context, Intent earphoneIntent) {

            if (earphoneIntent != null && earphoneIntent.getAction() != null) {
                if (earphoneIntent.getAction().equalsIgnoreCase(
                        Intent.ACTION_HEADSET_PLUG)) {

                    int st = 0;

                    st = earphoneIntent.getIntExtra("state", 0);

                    if (st > 0) {
                    	if(mDialog != null) {
                    		mDialog.cancel();
                    	}
                    }
                    else if (st == 0) {
            			showDialog(HEADSET_DIALOG);
                    }

                }
            }

        }
    };

	@Override
	protected void onResume() {
		super.onResume();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_HEADSET_PLUG);
        registerReceiver(earphonePluginReceiver, filter);

		Intent intent = new Intent(Intent.ACTION_FM);
        intent.putExtra("state", 1);
        intent.putExtra("speaker", 0);
        sendBroadcast(intent);
		try {
		fmManager = new FmManager(this);
		}catch (Exception e) {
            Log.e("Validationtools", "exception when creating FmReceiver instance");
            e.printStackTrace();
        }
		fmManager.powerUp();
		fmManager.setFreq((int) (freq * TRANS_MULT));

	}

	@Override
	protected void onStop() {
		super.onStop();
		Intent intent = new Intent(Intent.ACTION_FM);
        intent.putExtra("state",0);
        intent.putExtra("speaker", 0);
        sendBroadcast(intent);
        fmManager.powerDown();
        unregisterReceiver(earphonePluginReceiver);

	}

	private void changeFreq(){
		try {
			String freqStr = mFreqEdit.getText().toString();
			freq = Float.valueOf(freqStr);
			fmManager.setFreq((int) (freq * TRANS_MULT));
		}catch(Exception e) {
		    mFreqEdit.setText("");
			e.printStackTrace();
		}
	}

    private boolean isHeadsetExists() {
        char[] buffer = new char[1024];

        int newState = 0;

        /* start:  close fileStream in finllay{] . updated by spreadst 20120731 */
        FileReader file = null;
        try {
            /*FileReader*/ file = new FileReader(HEADSET_STATE_PATH);
            int len = file.read(buffer, 0, 1024);
            newState = Integer.valueOf((new String(buffer, 0, len)).trim());
        }
        catch (FileNotFoundException e) {
            Log.e("FMTest", "This kernel does not have wired headset support");
        }
        catch (Exception e) {
            Log.e("FMTest", "", e);
        }finally{
           if(file!=null){
               try{
                  file.close();
               }catch(Exception e){
                    e.printStackTrace();
               }
           }
        }
        /* end:  close fileStream in finllay{] . updated by spreadst 20120731 */
        return newState != 0;
    }
    
    private void switchSpeker() {
        if (mIsUseHeadset) {
            Intent intentaudio = new Intent(Intent.ACTION_FM);
            intentaudio.putExtra("FM", 0);
            intentaudio.putExtra("state", 1);
            intentaudio.putExtra("speaker", 0);
            sendBroadcast(intentaudio);
            mIsUseHeadset = false;
            mPlayingDeviceText.setText(R.string.headsetplaying);
        }
        else {
            Intent intentaudio = new Intent(Intent.ACTION_FM);
            intentaudio.putExtra("FM", 0);
            intentaudio.putExtra("state", 1);
            intentaudio.putExtra("speaker", 1);
            sendBroadcast(intentaudio);
            mIsUseHeadset = true;
            mPlayingDeviceText.setText(R.string.SpeakerPlaying);
        }
    }

}
