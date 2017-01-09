package com.sprd.engineermode.debuglog;

import android.content.Intent;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.telephony.TelephonyManager;
import android.content.ComponentName;
import android.util.Log;

public class SIMSelectForAPNActivity extends PreferenceActivity implements
Preference.OnPreferenceClickListener{

    private static final String TAG = "SIMSelectForAPNActivity";
    private static final String KEY_SIM_PREFERENCE = "sim_";
    private static final String KEY_SELECT_SIM = "select_sim";

    private int mPhoneCount;
    private Preference[] mSIMSelectPre;
    PreferenceGroup mPreGroup = null;

    protected void onCreate(Bundle savedInstanceState){
        super.onCreate(savedInstanceState);   
        mPhoneCount = TelephonyManager.from(this).getPhoneCount();
        mSIMSelectPre = new Preference[mPhoneCount];
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        mPreGroup = getPreferenceScreen();
        for(int i=0;i<mPhoneCount;i++){
            mSIMSelectPre[i] = new Preference(this);
            mSIMSelectPre[i].setTitle("SIM"+i);
            mSIMSelectPre[i].setKey(KEY_SIM_PREFERENCE+i);
            if(isCardExist(i)){
                mSIMSelectPre[i].setEnabled(true);
                mSIMSelectPre[i].setOnPreferenceClickListener(this);
            }else{
                mSIMSelectPre[i].setEnabled(false);
            }
            mPreGroup.addPreference(mSIMSelectPre[i]);
        }
    }

    @Override
    public boolean onPreferenceClick(Preference pref){
        for(int i=0;i<mPhoneCount;i++){
            if(pref == mSIMSelectPre[i]){
                Intent intent = new Intent();
                intent.putExtra(KEY_SELECT_SIM, i);
                intent.setComponent(new ComponentName("com.sprd.engineermode","com.sprd.engineermode.debuglog.APNSettingActivity"));    
                startActivity(intent);
            }
        }
        return true;
    } 

    private boolean isCardExist(int phoneId) {
        TelephonyManager telephonyManager = TelephonyManager.from(this);
        if (telephonyManager != null
                && telephonyManager.getSimState(phoneId) == TelephonyManager.SIM_STATE_READY) {
            return true;
        } else {
            return false;
        }
    }
}