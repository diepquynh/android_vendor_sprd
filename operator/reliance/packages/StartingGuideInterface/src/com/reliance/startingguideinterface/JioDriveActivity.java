package com.reliance.startingguideinterface;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.content.Intent;

import android.widget.Button;
import android.widget.Toast;
import android.view.View.OnClickListener;
import android.view.KeyEvent;
import android.view.Window;
import android.telephony.TelephonyManager;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import android.util.Log;


public class JioDriveActivity extends Activity {
    private static final String TAG = "JioDriveActivity";
    private static final boolean DEG = true;
    private final String JIO_4G = "Jio 4G";
    private final String PACKAGE_JIO_DRIVE = "com.newbay.syncdrive.android.reliance";
    private final String PACKAGE_JIO_SETTINGS = "com.jio.mhood.services";
    private final String RES_EXTRA = "extra_not_hook_action";

    public static final int DATA_CONNECTING = 1;

    TelephonyManager tm = TelephonyManager.getDefault();
    private Button mButtonYes = null;
    private Button mButtonSkip = null;

    private RespondingButton mRespondingButton = null;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if(DEG) Log.v(TAG,"onCreate");
        //for single window,if other activiy needed,set code here
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_main);
        initButtonAndResponding();
        setResponding();
    }
    private void initButtonAndResponding(){
        mButtonYes = (Button)findViewById(R.id.button_yes);
        mButtonSkip = (Button)findViewById(R.id.button_skip);
        mRespondingButton = new RespondingButton();
    }
    private void setResponding(){
        mButtonYes.setOnClickListener(mRespondingButton);
        mButtonSkip.setOnClickListener(mRespondingButton);
    }
    private class RespondingButton implements OnClickListener{
        @Override
        public void onClick(View v){
            switch(v.getId()){
            case R.id.button_yes:
                respondButtonYes();
                break;
            case R.id.button_skip:
                goHome();
                break;
            }
        }
    }
    private void respondButtonYes(){
        if(!((JIO_4G).equals(tm.getSimOperatorNameForPhone(0)))&&!((JIO_4G).equals(tm.getSimOperatorNameForPhone(1)))){
            ToastShow(getString(R.string.jio_card_not_exist));
            if(DEG) Log.v(TAG,"JIO_CARD_NOT_EXIST");
            goHome();
        }else{
            if(getPackageManager().getLaunchIntentForPackage(PACKAGE_JIO_DRIVE) == null){
                ToastShow(getString(R.string.jio_drive_not_exist));
                if(DEG) Log.v(TAG,"JIO_DRIVE_NOT_EXIST");
                goHome();
            }else if(getPackageManager().getLaunchIntentForPackage(PACKAGE_JIO_SETTINGS) == null){
                ToastShow(getString(R.string.jio_settings_not_exist));
                if(DEG) Log.v(TAG,"JIO_SETTINGS_NOT_EXIST");
                goHome();
            }
            else{
                showBackup();
            }
        }
    }
    protected void showBackup() {
        AlertDialog.Builder builder = new Builder(JioDriveActivity.this);
        builder.setMessage(getString(R.string.jio_dirve_backup));
        builder.setTitle(getString(R.string.notice));
        builder.setPositiveButton(getString(R.string.yes), new android.content.DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                if (tm.getDataState() == DATA_CONNECTING) {
                    if(DEG) Log.v(TAG,"DATA_CONNECTING");
                    startActivity(getPackageManager().getLaunchIntentForPackage(PACKAGE_JIO_DRIVE));
                }else{
                    showAccess();
                    if(DEG) Log.v(TAG,"DATA_NOT_CONNECTING");
                }
            }
        });
        builder.setNegativeButton(getString(R.string.no), new android.content.DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                if(DEG) Log.v(TAG,"setNegativeButton");
                goHome();
            }
        });
        builder.create().show();
    }
    protected void showAccess() {
        AlertDialog.Builder builder = new Builder(JioDriveActivity.this);
        builder.setMessage(getString(R.string.access_app));
        builder.setTitle(getString(R.string.notice));
        builder.setPositiveButton(getString(R.string.ok), new android.content.DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                if(DEG) Log.v(TAG,"showAccess");
                goHome();
            }
        });
        builder.create().show();
    }

    public void goHome(){
        Intent intent = new Intent("com.android.setupwizard.EXIT");
        if(DEG) Log.v(TAG,"reach homepage");
        intent.putExtra(RES_EXTRA, true);
        startActivity(intent);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)  {
        if (keyCode == KeyEvent.KEYCODE_BACK )  {
            if(DEG) Log.v(TAG,"onKeyDown");
            finish();
        }
        return true;
    }
    void ToastShow(String s){
        Toast.makeText(JioDriveActivity.this,s,Toast.LENGTH_SHORT).show();
    }
}
