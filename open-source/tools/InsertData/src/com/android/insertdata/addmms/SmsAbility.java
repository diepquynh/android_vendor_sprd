package com.android.insertdata.addmms;

import com.android.insertdata.R;

import android.app.Activity;
import android.content.Intent;
import android.os.Handler;
import android.provider.Telephony;
import android.util.Log;
import android.view.KeyEvent;
import android.widget.Toast;

public class SmsAbility extends Activity {
    private static final String TAG = "DefaultSmsAbility";
    private String mDefaultSmsApp;
    protected Handler mHandler = new Handler();

    @Override
    protected void onStart() {
        mDefaultSmsApp = Telephony.Sms.getDefaultSmsPackage(this);
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                setDefaultSmsApp();
            }
        });
        super.onStart();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        resetDefaultSmsApp();
                    }
                });
                break;
        }

        return super.onKeyDown(keyCode, event);
    }

    protected void setDefaultSmsApp() {
        Log.d(TAG,"setDefaultSmsApp()");
        if ( Telephony.Sms.getDefaultSmsPackage(this).compareTo(getPackageName()) != 0 ) {
            Intent intent = new Intent(Telephony.Sms.Intents.ACTION_CHANGE_DEFAULT);
            intent.putExtra(Telephony.Sms.Intents.EXTRA_PACKAGE_NAME, getPackageName());
            startActivity(intent);
        }
    }

    protected void resetDefaultSmsApp() {
        Log.d(TAG,"resetDefaultSmsApp()");
        if ( Telephony.Sms.getDefaultSmsPackage(this).compareTo(getPackageName()) == 0
                && getPackageName().compareTo(mDefaultSmsApp) != 0 ) {
            Intent intent = new Intent(Telephony.Sms.Intents.ACTION_CHANGE_DEFAULT);
            intent.putExtra(Telephony.Sms.Intents.EXTRA_PACKAGE_NAME, mDefaultSmsApp);
            startActivity(intent);
        }
    }
    
    protected boolean checkDefaultSmsApp() {
        if (Telephony.Sms.getDefaultSmsPackage(this).compareTo(getPackageName()) != 0) {
            Toast.makeText(this, R.string.must_default_sms_app, Toast.LENGTH_LONG).show();
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    setDefaultSmsApp();
                }
            }, 1000);
            return false;
        }
        return true;
    }
}
