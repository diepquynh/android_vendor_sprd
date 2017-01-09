package com.sprd.engineermode.connectivity.BT;

import android.preference.PreferenceActivity;
import android.os.Bundle;
import com.sprd.engineermode.R;

public class BTNOSIGNALActivity extends PreferenceActivity {
	@Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        this.addPreferencesFromResource(R.xml.pref_bt_nosignal_test);
    }
}