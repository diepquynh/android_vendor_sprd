
package com.sprd.engineermode.telephony;

import com.sprd.engineermode.R;
import com.sprd.engineermode.SimSelectHelper;

import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;
import android.telephony.TelephonyManager;
import android.content.Intent;
import android.util.Log;
import android.preference.PreferenceActivity;

public class SimSelectHelperActivity extends PreferenceActivity {

    private static final String TAG = "SimSelectHelperActivity";
    private static final String SIM = "SIM";

    public static final String INTENT_SIM_INDEX = "simindex";

    private int mPhoneCount;
    private Preference[] mSIMPref;
    private TelephonyManager[] mTelephonyManager;
    PreferenceGroup mPreGroup = null;

    private Intent mIntentForStart = null;
    private boolean mAutoDisable = false;
    private Runnable mOnChange = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        mPreGroup = getPreferenceScreen();
        mPhoneCount = TelephonyManager.from(this).getPhoneCount();
        mTelephonyManager = new TelephonyManager[mPhoneCount];
        mSIMPref = new Preference[mPhoneCount];

        mIntentForStart = SimSelectHelper.getIntentForStart();
        mAutoDisable = SimSelectHelper.isAutoDisable();
        mOnChange = SimSelectHelper.getOnChangeCB();

        for (int i = 0; i < mPhoneCount; i++) {
            mSIMPref[i] = new Preference(this);
            mSIMPref[i].setTitle(SIM + i);
            mSIMPref[i].setKey(SIM + i);
            mPreGroup.addPreference(mSIMPref[i]);
            if (mAutoDisable) {
                mTelephonyManager[i] = (TelephonyManager) TelephonyManager
                        .from(SimSelectHelperActivity.this);
                if (mTelephonyManager[i] != null
                        && mTelephonyManager[i].getSimState(i) == TelephonyManager.SIM_STATE_READY) {
                    mSIMPref[i].setEnabled(true);
                } else {
                    mSIMPref[i].setEnabled(false);
                    mSIMPref[i].setSummary(R.string.input_card_to_test);
                }
            }
        }
    }

    @Override
    protected void onResume() {
        if (mOnChange != null) {
            mOnChange.run();
        }
        super.onResume();
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        String key = preference.getKey();

        String[] temp = key.split("M");

        int simIndex = Integer.valueOf(temp[1]);
        mIntentForStart.putExtra(INTENT_SIM_INDEX, simIndex);
        startActivity(mIntentForStart);
        return false;
    }
}
