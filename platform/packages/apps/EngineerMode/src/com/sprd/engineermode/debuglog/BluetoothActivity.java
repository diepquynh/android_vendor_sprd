package com.sprd.engineermode.debuglog;


import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.TwoStatePreference;
import android.util.Log;
import android.widget.Toast;

import com.sprd.engineermode.R;

public class BluetoothActivity extends PreferenceActivity implements
Preference.OnPreferenceChangeListener{

    private static final String TAG = "BluetoothActivity";
    
    private static final String KEY_SPEED_LIMIT = "speed_limit";
    
    private TwoStatePreference mSpeed_limit;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_cmcc_bluetooth);
       
        mSpeed_limit = (TwoStatePreference) findPreference(KEY_SPEED_LIMIT);
        if (SystemProperties.get("debug.bt.lowspeed").isEmpty()) {
            mSpeed_limit.setChecked(false);
        } else {
            mSpeed_limit.setChecked(SystemProperties.get("debug.bt.lowspeed").equals("true"));
        }
        mSpeed_limit.setOnPreferenceChangeListener(this);
        
    }

    @Override
    protected void onStart(){
        super.onStart();
    }

    @Override
    public boolean onPreferenceChange(Preference pref,Object objValue){
        if (pref == mSpeed_limit) {
            Log.d(TAG,"mSpeed_limit change ");
            if(mSpeed_limit.isChecked()){
                SystemProperties.set("debug.bt.lowspeed", "false");
                Log.d(TAG,"SET_SPEED_LIMIT: set prop is " + SystemProperties.get("debug.bt.lowspeed"));
            }else{
                SystemProperties.set("debug.bt.lowspeed", "true");
                Log.d(TAG,"SET_SPEED_LIMIT: set prop is " + SystemProperties.get("debug.bt.lowspeed"));
            }
            return true;
        }
        return false;
    }
}