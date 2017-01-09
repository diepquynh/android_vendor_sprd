package com.android.stability;

import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.media.AudioSystem;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;

public class ReceiverActivity extends Activity implements OnClickListener {

	private Button receiver_stop_button = null;
	private Button receiver_back_button = null;	
	private MediaPlayer player = null;
    private TextView my_textview = null;
    private AudioManager localAudioManager = null;
    private int audio_mode = AudioSystem.MODE_NORMAL;
    
    public static final String TAG = "receiverActivity";
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
    	// TODO Auto-generated method stub
    	super.onCreate(savedInstanceState);
    	setContentView(R.layout.receiverlayout);
    	
    	getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    	       
        receiver_stop_button = (Button)findViewById(R.id.receiver_stop);
        receiver_back_button = (Button)findViewById(R.id.receiver_back_button);         
        
        receiver_stop_button.setOnClickListener(this);
        receiver_back_button.setOnClickListener(this);
        
        localAudioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
        if(localAudioManager.isSpeakerphoneOn())
        {
        	localAudioManager.setSpeakerphoneOn(false);
        }
        audio_mode = localAudioManager.getMode();
        localAudioManager.setMode(AudioSystem.MODE_IN_CALL);
        player = MediaPlayer.create(this, R.raw.bootaudio);
        player.setLooping(true);
        player.start();
    }
    
	@Override
	public void onClick(View v) {
		// TODO Auto-generated method stub
		if(v.equals(receiver_stop_button)){
			if(player.isPlaying())
            {
            	player.pause();
            }
			my_textview = (TextView)findViewById(R.id.receiver_note);
			String str = getString(R.string.receiver_stop).toString();
			my_textview.setText(str);
		}
		else if(v.equals(receiver_back_button)){
			if(player.isPlaying())
            {
            	player.stop();
            }
            receiver_exit();
		}
	}
	
	private void receiver_exit()
	{
		localAudioManager.setMode(audio_mode);
		finish();
	}
	
	public void onPause()
	{
		super.onPause();
        if(player.isPlaying())
        {
        	player.stop();
        }
		localAudioManager.setMode(audio_mode);        
	}

	public void onDestroy()
	{
		super.onDestroy();
		if(player != null)
		{
			player.release();
			player = null;
		}
	}
}
