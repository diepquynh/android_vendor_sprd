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
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;
import android.preference.PreferenceCategory;
import android.preference.CheckBoxPreference;
import android.view.View.OnClickListener;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class GFrequencyActivity extends PreferenceActivity implements
OnClickListener,Preference.OnPreferenceClickListener{

    private static final String TAG = "GFrequencyActivity";
    private static final String KEY_G_FREQUENCY = "g_frequency";
    private static final String KEY_G_QUERY_SIM = "g_frequency_sim";
    private static final int UNLOCK_FREQUENCY = 1;
    private static final int LOCK_FREQUENCY = 2;

    private String mTR;
    private String mFrequency;
    private String mATCmd;
    private int mFrequencyCount = 0; 
    private String[] mSelectFrequency;
    private int mSelectFrequencyCount=0;
    private int mFrequencyIndex;
    private int mSIM =0;
    private String mServerName = "atchannel0";

    private CheckBoxPreference[] mFrequencyCheck1 = null;
    private CheckBoxPreference[] mFrequencyCheck2 = null;
    private Button mLockBt;
    private Button mUnLockBt;

    PreferenceGroup mPreGroup = null;

    private Handler mUiThread = new Handler();
    private GFHandler mGFHandler;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        mPreGroup = getPreferenceScreen();
        Bundle extras = this.getIntent().getExtras();
        if(extras == null){
            return;
        }
        mTR = extras.getString(KEY_G_FREQUENCY);
        mSIM = extras.getInt(KEY_G_QUERY_SIM);

        if(mTR.contains(IATUtils.AT_OK)){
            String[] str = mTR.split("\\:");
            String[] str1 = str[1].split("\\,");
            if(mSIM == 1){
                mServerName = "atchannel1";                
                mFrequencyCheck2 = new CheckBoxPreference[str1.length];
                for(int i=1;i<str1.length;i++){
                    mFrequency = str1[i].trim();
                    if(!mFrequency.equals("0") && mFrequency != null){
                        mFrequencyCheck2[mFrequencyCount] = new CheckBoxPreference(this);
                        mFrequencyCheck2[mFrequencyCount].setTitle(mFrequency);
                        mFrequencyCheck2[mFrequencyCount].setDefaultValue(false);
                        mFrequencyCheck2[mFrequencyCount].setKey(mSIM+KEY_G_FREQUENCY+mFrequencyCount);
                        mFrequencyCount++;
                    }
                } 
            }else{
                mFrequencyCheck1 = new CheckBoxPreference[str1.length];
                for(int i=1;i<str1.length;i++){
                    mFrequency = str1[i].trim();
                    if(!mFrequency.equals("0") && mFrequency != null){
                        mFrequencyCheck1[mFrequencyCount] = new CheckBoxPreference(this);
                        mFrequencyCheck1[mFrequencyCount].setTitle(mFrequency);
                        mFrequencyCheck1[mFrequencyCount].setDefaultValue(false);
                        mFrequencyCheck1[mFrequencyCount].setKey(mSIM+KEY_G_FREQUENCY+mFrequencyCount);
                        mFrequencyCount++;
                    }
                }  
            }
        }
        Log.d(TAG,"There are "+mFrequencyCount+"Frequency");
        if(mFrequencyCount != 0){ 
            if(mSIM == 1){
                for(int i=0;i<mFrequencyCount;i++){
                    mPreGroup.addPreference( mFrequencyCheck2[i]);
                }  
            }else{
                for(int i=0;i<mFrequencyCount;i++){
                    mPreGroup.addPreference( mFrequencyCheck1[i]);
                }  
            }

            setContentView(R.layout.g_frequency); 
            mLockBt = (Button)findViewById(R.id.lock_frequency);
            mLockBt.setOnClickListener(this);
            mUnLockBt = (Button)findViewById(R.id.unlock_frequency);
            mUnLockBt.setOnClickListener(this);
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

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mGFHandler = new GFHandler(ht.getLooper());
    }

    @Override
    protected void onStart() {
        super.onStart();
        if(mSIM == 1){
            for(int i=0;i<mFrequencyCount;i++){
                if(mFrequencyCheck2[i].isChecked()){
                    mSelectFrequency[mSelectFrequencyCount] = (String)mFrequencyCheck2[i].getTitle();
                    mSelectFrequencyCount++;
                }
            }
        }else{
            for(int i=0;i<mFrequencyCount;i++){
                if(mFrequencyCheck1[i].isChecked()){
                    mSelectFrequency[mSelectFrequencyCount] = (String)mFrequencyCheck1[i].getTitle();
                    mSelectFrequencyCount++;
                }
            } 
        }  
        if(mLockBt != null && mUnLockBt != null){
            if(mSelectFrequencyCount == 0){
                mLockBt.setEnabled(false);
                mUnLockBt.setEnabled(false);
            }else{
                mLockBt.setEnabled(true);
                mUnLockBt.setEnabled(true);
            }
        }     
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        if(mGFHandler != null){
            Log.d(TAG,"HandlerThread has quit");
            mGFHandler.getLooper().quit();
        } 
        super.onDestroy();
    }
    
    @Override
    public void onBackPressed() {
        // TODO Auto-generated method stub
        finish();
        super.onBackPressed();
    }
    
    class GFHandler extends Handler {

        public GFHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg){
            mFrequencyIndex = msg.arg1;
            String frequency = (String)msg.obj;
            String responValue;
            switch(msg.what){
                case UNLOCK_FREQUENCY:{
                    mATCmd = engconstents.ENG_AT_SPFRQ+"0";   
                    responValue = IATUtils.sendATCmd(mATCmd,mServerName); 
                    Log.d(TAG,"<"+mSIM+">Channel is "+mServerName+",UNLOCK_FREQUENCY is "+mATCmd+", responValue is "+responValue);
                    if(responValue.contains(IATUtils.AT_OK)){

                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                if(mSIM == 1){
                                    for(int i=0;i<mFrequencyCount;i++){
                                        mFrequencyCheck2[i].setChecked(false);
                                    }
                                }else{
                                    for(int i=0;i<mFrequencyCount;i++){
                                        mFrequencyCheck2[i].setChecked(false);
                                    } 
                                }
                                Toast.makeText(GFrequencyActivity.this, "Success", Toast.LENGTH_SHORT).show();
                            }
                        });

                    }else{
                        Toast.makeText(GFrequencyActivity.this, "Fail", Toast.LENGTH_SHORT).show();
                    }
                    break;
                }
                case LOCK_FREQUENCY:{
                    mATCmd = engconstents.ENG_AT_SPGSMFRQ+"1";
                    for(int i=0;i<mSelectFrequencyCount;i++){
                        mATCmd = mATCmd+","+mSelectFrequency[i];
                    }
                    responValue = IATUtils.sendATCmd(mATCmd,mServerName);
                    Log.d(TAG,"<"+mSIM+">Channel is "+mServerName+",LOCK_FREQUENCY is "+mATCmd+", responValue is "+responValue);
                    if(responValue.contains(IATUtils.AT_OK)){
                        Toast.makeText(GFrequencyActivity.this, "Success", Toast.LENGTH_SHORT).show();
                    }else{
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                if(mSIM == 1){
                                    for(int i=0;i<mFrequencyCount;i++){
                                        mFrequencyCheck2[i].setChecked(false);
                                    } 
                                }else{
                                    for(int i=0;i<mFrequencyCount;i++){
                                        mFrequencyCheck1[i].setChecked(false);
                                    }
                                }
                                Toast.makeText(GFrequencyActivity.this, "Fail", Toast.LENGTH_SHORT).show();
                            }
                        }); 
                    }
                    break;
                }
            }
        }
    }

    @Override
    public void onClick(View v) {
        if(v.equals(mLockBt)){
            Message lockF = mGFHandler.obtainMessage(LOCK_FREQUENCY);         
            mGFHandler.sendMessage(lockF); 
        }else if(v.equals(mUnLockBt)){
            AlertDialog alertDialog = new AlertDialog.Builder(this)
            .setTitle(getString(R.string.lock_frequency))
            .setMessage(getString(R.string.alert_lock_frequency))
            .setPositiveButton(
                    getString(R.string.alertdialog_ok),
                    new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            Message unlockF = mGFHandler.obtainMessage(UNLOCK_FREQUENCY);         
                            mGFHandler.sendMessage(unlockF); 
                        }
                    })
                    .setNegativeButton(R.string.alertdialog_cancel, 
                            new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                        }
                    }).create();
            alertDialog.show();
        }
    }

    @Override
    public boolean onPreferenceClick(Preference pref){
        if(mSIM == 1){
            for(int i=0;i<mFrequencyCount;i++){
                if(pref == mFrequencyCheck2[i]){
                    if(mFrequencyCheck2[i].isChecked()){
                        mSelectFrequency[mSelectFrequencyCount] = (String)mFrequencyCheck2[i].getTitle();
                        mSelectFrequencyCount++;
                    }
                }
            }
        }else{
            for(int i=0;i<mFrequencyCount;i++){
                if(pref == mFrequencyCheck1[i]){
                    if(mFrequencyCheck1[i].isChecked()){
                        mSelectFrequency[mSelectFrequencyCount] = (String)mFrequencyCheck1[i].getTitle();
                        mSelectFrequencyCount++;
                    }
                }
            }
        }
        return true;
    }
}
