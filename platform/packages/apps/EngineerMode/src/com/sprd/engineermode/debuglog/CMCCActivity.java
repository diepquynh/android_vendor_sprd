package com.sprd.engineermode.debuglog;

import com.sprd.engineermode.R;

import android.os.Bundle;
import android.preference.PreferenceActivity;

public class CMCCActivity extends PreferenceActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_cmcc);
    }
}