package com.sprd.validationtools.itemstest;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import android.hardware.soundtrigger.SoundTrigger;
import android.hardware.soundtrigger.SoundTriggerModule;
import android.hardware.soundtrigger.SoundTrigger.RecognitionEvent;
import android.hardware.soundtrigger.SoundTrigger.SoundModelEvent;
import android.hardware.soundtrigger.SoundTrigger.SoundModel;
import android.hardware.soundtrigger.SoundTrigger.ConfidenceLevel;
import android.hardware.soundtrigger.SoundTrigger.KeyphraseRecognitionEvent;
import android.hardware.soundtrigger.SoundTrigger.KeyphraseRecognitionExtra;
import android.hardware.soundtrigger.SoundTrigger.KeyphraseSoundModel;
import android.media.MediaPlayer;
import java.util.UUID;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;

public class SoundTriggerTestActivity<mpv> extends BaseActivity 
    implements SoundTrigger.StatusListener{

	private Button mBtnStart;
	private TextView mTxNote;
    private SoundTriggerModule stm;
    private static final int MODULE_ID = 1;
    private static final String TAG = "SoundTriggerTest";
    private int soundModelHandle[] = {1};
    private byte data[] = {100,100};
    private byte data2[] = {10,11,12,14};
	private SoundTrigger.RecognitionConfig config ;
	private SoundTrigger.KeyphraseRecognitionExtra[] recognitionExtra;

	private MediaPlayer mpv = null;

    public void playMusic(){
    	mpv = new MediaPlayer();
        mpv = MediaPlayer.create(this,R.raw.soundtriggermp);
    	mpv.start();
    	/*
    	mp = new MediaPlayer();
    	String song = ".mp3";
    	try{
    		mp.setDataSource();
    		mp.prepare();
    		mp.start();
    	}catch(Exception e){
    		mTxNote.setText("the voice play error");
    	}
    	*/
    }
    
	private void initSoundTrigger(){
		Log.d(TAG, "start ");
		//String appDirFull = getFilesDir().toString() + "/SMicTD1.dat";
		//Log.d(TAG,"reloadSoundModel appDirFull="+appDirFull);
    
		Log.d(TAG, "loadSoundModel() start"); 
		InputStream is= getResources().openRawResource(R.raw.soundtriggerx);
		
        try {
            SoundTrigger.SoundModel sd = new SoundTrigger.SoundModel(
                    UUID.fromString("119341a0-8469-11df-81f9-0012a5d5c51b"),
                    UUID.fromString("119341a0-8469-11df-81f9-0002a5d7c51b"),
                    SoundTrigger.SoundModel.TYPE_KEYPHRASE, toByteArray(is));
            stm.loadSoundModel(sd, soundModelHandle);
            Log.d(TAG, "loadSoundModel() finish");
        } catch (IOException e) {
            Toast.makeText(this, "error in loadSoundModel", Toast.LENGTH_SHORT).show();
            mTxNote.setText(R.string.test_fail);
            Log.d(TAG, e.toString());
        }
    }

	@Override
	protected void onCreate(Bundle savedInstanceState){
		super.onCreate(savedInstanceState);
		setContentView(R.layout.soundtrigger_test);
		mBtnStart = (Button)findViewById(R.id.btn_start);
		mBtnStart.setOnClickListener(this);
		mBtnStart.setClickable(false);
		
		mTxNote = (TextView)findViewById(R.id.txt_note);
		mTxNote.setText(R.string.loading_soundTrigger);
		stm = SoundTrigger.attachModule(MODULE_ID,this, new Handler());
		
		initSoundTrigger();
	}
	
	/**
	 * implements StatusListener when soundtrigger get sound will call back it
	 * @param event
	 */
	@Override
    public void onRecognition(RecognitionEvent event){
        Log.d(TAG, "the sound had get");
		// pass
        mTxNote.setText(R.string.test_pass);
		stm.stopRecognition(soundModelHandle[0]);
		stm.startRecognition(soundModelHandle[0], config);
		if(mpv!=null){
			mpv.pause();
			mpv.release();
			mpv = null;
		}
	}
	
	/**
	 * implements StatusListener when soundtriggersystem begin work will call back it
	 * @param event
	 */
  	@Override
    public void onSoundModelUpdate(SoundModelEvent event){
		Log.d(TAG,"load SoundTriggle finished,and you can test");	
        mBtnStart.setClickable(true);
        mTxNote.setText(R.string.soundtrigger_click_button);
	}
  	
  	@Override
  	public void onServiceDied(){
  		
  	}
  	@Override
  	public void onServiceStateChange(int status){
  		
  	}
  	    
  	@Override
  	public void onClick(View v) {
  	    if (v == mPassButton) {
  	        if (canPass) {
  	            Log.d("onclick", "pass.." + this);
  	            storeRusult(true);
  	            finish();
  	            } else {
  	            Toast.makeText(this, R.string.can_not_pass, Toast.LENGTH_SHORT).show();
  	        }
  	    } else if (v == mFailButton) {
  	        storeRusult(false);
  	        finish();
  	    } else if (v == mBtnStart ) {
            mTxNote.setText(R.string.soundtrigger_load);
  			recognitionExtra = new SoundTrigger.KeyphraseRecognitionExtra[1];
  			recognitionExtra[0] = new SoundTrigger.KeyphraseRecognitionExtra(1,2, 0, new ConfidenceLevel[0]);
  			config = new SoundTrigger.RecognitionConfig(true , true , recognitionExtra , data);
  				
  			Log.d(TAG, "update ketphrase:startRecognition()...");
  			stm.stopRecognition(soundModelHandle[0]);
  		    stm.startRecognition(soundModelHandle[0], config);
  		    Log.d(TAG,"Recognition finished");
  		    mTxNote.setText(R.string.prepare_play_sound);
  		    try{
  		    Thread.sleep(3000);
  		    }catch(Exception e){
  		    	
  		    }
  		    //mTxNote.setText("the sound is playing");
  		    playMusic();
  		    //mTxNote.setText("the sound is over");
  		    
  	    }
  	}
  	    
  	@Override
  	protected void onDestroy() {
    	stm.stopRecognition(soundModelHandle[0]);
		stm.unloadSoundModel(soundModelHandle[0]);
		stm.detach();
		if(mpv!=null){
			mpv.pause();
			mpv.release();
			mpv = null;
		}
		
        Log.d(TAG,"Destory finish");
  		super.onDestroy();
  	}
  	
    public static byte[] toByteArray(String filename) throws IOException {  
        
        File f = new File(filename);  
        if (!f.exists()) {  
            throw new FileNotFoundException(filename);  
        }  
  
        ByteArrayOutputStream bos = new ByteArrayOutputStream((int) f.length());  
        Log.d(TAG,"wenjianchangdu:"+f.length());
        BufferedInputStream in = null;  
        try {  
            in = new BufferedInputStream(new FileInputStream(f));  
            int buf_size = 1024;  
            byte[] buffer = new byte[buf_size];  
            int len = 0;  
            while (-1 != (len = in.read(buffer, 0, buf_size))) {  
                bos.write(buffer, 0, len);  
            }  
            return bos.toByteArray();  
        } catch (IOException e) {  
            e.printStackTrace();  
            throw e;  
        } finally {  
            try {  
                in.close();  
            } catch (IOException e) {  
                e.printStackTrace();  
            }  
            bos.close();  
        }  
    } 

    public static byte[] toByteArray(InputStream is) throws IOException {  
  
        ByteArrayOutputStream bos = new ByteArrayOutputStream(20000);  
        BufferedInputStream in = null;  
        try {  
            in = new BufferedInputStream(is);  
            int buf_size = 1024;  
            byte[] buffer = new byte[buf_size];  
            int len = 0;  
            while (-1 != (len = in.read(buffer, 0, buf_size))) {  
                bos.write(buffer, 0, len);  
            }  
            return bos.toByteArray();  
        } catch (IOException e) {  
            e.printStackTrace();  
            throw e;  
        } finally {  
            try {  
                in.close();  
            } catch (IOException e) {  
                e.printStackTrace();  
            }  
            bos.close();  
        }  
    } 

}