package com.sprd.engineermode.telephony.netinfostatistics;

import com.sprd.engineermode.R;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.telephony.TelephonyManager;
import android.content.Intent;
import android.preference.PreferenceActivity;

public class SIMSelectForNetStatiActivity extends PreferenceActivity {

    private static final String TAG = "SIMSelectForNetStatiActivity";
    private static final String SIM = "SIM";
    private static final String KEY_SIM_INDEX = "simindex";

    private int mPhoneCount;
    private Preference[] mSIMPref;
    private TelephonyManager[] mTelephonyManager;
    PreferenceGroup mPreGroup = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        mPreGroup = getPreferenceScreen();
        mPhoneCount = TelephonyManager.from(this).getPhoneCount();
        mTelephonyManager = new TelephonyManager[mPhoneCount];
        mSIMPref = new Preference[mPhoneCount];

        for (int i = 0; i < mPhoneCount; i++) {
            mSIMPref[i] = new Preference(this);
            mSIMPref[i].setTitle(SIM + i);
            mSIMPref[i].setKey(SIM + i);
            mPreGroup.addPreference(mSIMPref[i]);
            mTelephonyManager[i] = (TelephonyManager) TelephonyManager
                    .from(this);
            if (mTelephonyManager[i] == null
                    || mTelephonyManager[i].getSimState(i) != TelephonyManager.SIM_STATE_READY) {
                mSIMPref[i].setEnabled(false);
                mSIMPref[i].setSummary(R.string.input_card_to_test);
            } else {
                mSIMPref[i].setEnabled(true);
            }
        }
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        String key = preference.getKey();
        if (key.equals("SIM0") || key.equals("SIM1") || key.equals("SIM2")) {
            int simIndex = Integer.valueOf(key.substring(3, 4));
            Intent intent = new Intent("android.intent.action.NETINFOSTATI");
            intent.putExtra(KEY_SIM_INDEX, simIndex);
            startActivity(intent);
        }
        return false;
    }
}