package com.sprd.engineermode.telephony;

import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.widget.Toast;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceCategory;
import android.preference.PreferenceGroup;
import android.preference.PreferenceManager;
import android.preference.Preference.OnPreferenceClickListener;
import com.sprd.engineermode.R;
import com.sprd.engineermode.R.xml;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import android.preference.TwoStatePreference;
import android.preference.SwitchPreference;
import android.os.SystemProperties;
import android.widget.Toast;

public class Csfb2GsmDelayActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {

    private static final String TAG = "Csfb2GsmDelayActivity";
    private static final String KEY_GRRC_RESIDENT_STATE = "GrrcResidentState";
    private static final String KEY_GRRC_RANDOM_STATE = "GrrcRandomAccessState";
    private static final int KEY_GET_CSFB2GSM_DELAY = 0;
    private static final int KEY_GRRC_RESIDENT_OPEN = 1;
    private static final int KEY_GRRC_RESIDENT_CLOSE = 2;
    private static final int KEY_GRRC_RANDOM_OPEN = 3;
    private static final int KEY_GRRC_RANDOM_CLOSE = 4;

    PreferenceGroup mPreGroup = null;
    private int mPhoneCount;
    private PreferenceCategory mPreferenceCategory;
    private TwoStatePreference[] mResidentState;
    private TwoStatePreference[] mRandomState;
    private int mSIM = 0;
    private String mResult = null;
    private String mServerName = "atchannel";

    private Csfb2GsmDelayHandler mCsfb2GsmDelayHandler;
    private Handler mUiThread = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            String responValue = null;
            int sim = 0;
            switch (msg.what) {
            case KEY_GET_CSFB2GSM_DELAY:
                responValue = (String) msg.obj;
                sim = (int) msg.arg1;
                if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                    String[] str = responValue.split("\n");
                    String[] str1 = str[0].split("\\:");
                    String[] str2 = str1[1].split(",");
                    if (str2[0].trim().contains("1")) {
                        mResidentState[sim].setChecked(true);
                        mResidentState[sim].setEnabled(true);
                        mRandomState[sim].setEnabled(false);
                        mRandomState[sim].setChecked(false);

                    } else if (str2[1].trim().contains("1")) {
                        mResidentState[sim].setChecked(false);
                        mResidentState[sim].setEnabled(false);
                        mRandomState[sim].setEnabled(true);
                        mRandomState[sim].setChecked(true);
                    } else {
                        mResidentState[sim].setChecked(false);
                        mResidentState[sim].setEnabled(true);
                        mRandomState[sim].setChecked(false);
                        mRandomState[sim].setEnabled(true);
                    }
                } else {
                    mResidentState[sim].setChecked(false);
                    mResidentState[sim].setEnabled(false);
                    mRandomState[sim].setEnabled(false);
                    mRandomState[sim].setChecked(false);
                    mResidentState[sim].setSummary(R.string.feature_abnormal);
                    mRandomState[sim].setSummary(R.string.feature_abnormal);
                }
                break;
            case KEY_GRRC_RESIDENT_OPEN:
                responValue = (String) msg.obj;
                sim = (int) msg.arg1;
                if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                    mResidentState[sim].setChecked(true);
                    mRandomState[sim].setEnabled(false);
                    Toast.makeText(Csfb2GsmDelayActivity.this, "Success",
                            Toast.LENGTH_SHORT).show();
                } else {
                    mResidentState[sim].setChecked(false);
                    Toast.makeText(Csfb2GsmDelayActivity.this, "Fail",
                            Toast.LENGTH_SHORT).show();
                }
                break;
            case KEY_GRRC_RESIDENT_CLOSE:
                responValue = (String) msg.obj;
                sim = (int) msg.arg1;
                if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                    mResidentState[sim].setChecked(false);
                    mRandomState[sim].setEnabled(true);
                    Toast.makeText(Csfb2GsmDelayActivity.this, "Success",
                            Toast.LENGTH_SHORT).show();

                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mResidentState[mSIM].setChecked(true);
                            Toast.makeText(Csfb2GsmDelayActivity.this, "Fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                break;
            case KEY_GRRC_RANDOM_OPEN:
                responValue = (String) msg.obj;
                sim = (int) msg.arg1;
                if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                    mRandomState[sim].setChecked(true);
                    mResidentState[sim].setEnabled(false);
                    Toast.makeText(Csfb2GsmDelayActivity.this, "Success",
                            Toast.LENGTH_SHORT).show();
                } else {
                    mRandomState[sim].setChecked(false);
                    Toast.makeText(Csfb2GsmDelayActivity.this, "Fail",
                            Toast.LENGTH_SHORT).show();
                }
                break;
            case KEY_GRRC_RANDOM_CLOSE:
                responValue = (String) msg.obj;
                sim = (int) msg.arg1;
                if (responValue != null && responValue.contains(IATUtils.AT_OK)) {
                    mRandomState[sim].setChecked(false);
                    mResidentState[sim].setEnabled(true);
                    Toast.makeText(Csfb2GsmDelayActivity.this, "Success",
                            Toast.LENGTH_SHORT).show();
                } else {
                    mRandomState[sim].setChecked(true);
                    Toast.makeText(Csfb2GsmDelayActivity.this, "Fail",
                            Toast.LENGTH_SHORT).show();
                }
                break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mPhoneCount = TelephonyManager.from(this).getPhoneCount();
        mResidentState = new TwoStatePreference[mPhoneCount];
        mRandomState = new TwoStatePreference[mPhoneCount];
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        mPreGroup = getPreferenceScreen();
        for (int i = 0; i < mPhoneCount; i++) {
            mPreferenceCategory = new PreferenceCategory(this);
            mPreferenceCategory.setTitle("SIM" + i);
            mPreGroup.addPreference(mPreferenceCategory);
            mResidentState[i] = new SwitchPreference(this);
            mResidentState[i].setKey(KEY_GRRC_RESIDENT_STATE + i);
            mResidentState[i].setTitle(R.string.grrc_resident_state);
            mResidentState[i].setChecked(false);
            mResidentState[i].setOnPreferenceChangeListener(this);
            mRandomState[i] = new SwitchPreference(this);
            mRandomState[i].setKey(KEY_GRRC_RANDOM_STATE + i);
            mRandomState[i].setTitle(R.string.grrc_random_access_state);
            mRandomState[i].setChecked(false);
            mRandomState[i].setOnPreferenceChangeListener(this);
            mPreGroup.addPreference(mResidentState[i]);
            mPreGroup.addPreference(mRandomState[i]);
        }
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mCsfb2GsmDelayHandler = new Csfb2GsmDelayHandler(ht.getLooper());

    }

    @Override
    public void onStart() {
        super.onStart();
        Message getCsfb2gsm_delay = mCsfb2GsmDelayHandler
                .obtainMessage(KEY_GET_CSFB2GSM_DELAY);
        mCsfb2GsmDelayHandler.sendMessage(getCsfb2gsm_delay);
    }

    @Override
    protected void onDestroy() {
        if (mCsfb2GsmDelayHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mCsfb2GsmDelayHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object newValue) {
        for (int i = 0; i < mPhoneCount; i++) {
            if (pref == mResidentState[i]) {
                if (!mResidentState[i].isChecked()) {
                    Message GrrcResidentOpen = mCsfb2GsmDelayHandler
                            .obtainMessage(KEY_GRRC_RESIDENT_OPEN, i);
                    mCsfb2GsmDelayHandler.sendMessage(GrrcResidentOpen);
                } else {
                    Message GrrcResidentClose = mCsfb2GsmDelayHandler
                            .obtainMessage(KEY_GRRC_RESIDENT_CLOSE, i);
                    mCsfb2GsmDelayHandler.sendMessage(GrrcResidentClose);
                }

            } else if (pref == mRandomState[i]) {
                if (!mRandomState[i].isChecked()) {
                    Message GrrcRandomOpen = mCsfb2GsmDelayHandler
                            .obtainMessage(KEY_GRRC_RANDOM_OPEN, i);
                    mCsfb2GsmDelayHandler.sendMessage(GrrcRandomOpen);
                } else {
                    Message GrrcRandomClose = mCsfb2GsmDelayHandler
                            .obtainMessage(KEY_GRRC_RANDOM_CLOSE, i);
                    mCsfb2GsmDelayHandler.sendMessage(GrrcRandomClose);
                }
            }
        }
        return true;
    }

    class Csfb2GsmDelayHandler extends Handler {

        public Csfb2GsmDelayHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String responValue = null;
            int sim = 0;
            String atCmd = null;
            switch (msg.what) {
            case KEY_GET_CSFB2GSM_DELAY:
                for (int i = 0; i < mPhoneCount; i++) {
                    mServerName = mServerName + i;
                    mSIM = i;
                    responValue = IATUtils.sendATCmd(
                            engconstents.ENG_AT_GET_CSFB2GSM, mServerName);
                    Log.d(TAG, "<" + mSIM + ">get Csfb2Gsm_delay Result is "
                            + responValue);
                    Message mes0 = mUiThread
                            .obtainMessage(KEY_GET_CSFB2GSM_DELAY);
                    mes0.arg1 = i;
                    mes0.obj = responValue;
                    mUiThread.sendMessage(mes0);
                }
                break;
            case KEY_GRRC_RESIDENT_OPEN:
                sim = (Integer) msg.obj;
                mServerName = "atchannel" + sim;
                atCmd = engconstents.ENG_AT_SET_CSFB2GSM + "1,1";
                responValue = IATUtils.sendATCmd(atCmd, mServerName);
                Log.d(TAG, "<" + sim + ">Channel is " + mServerName
                        + ",KEY_GRRC_RESIDENT_OPEN " + responValue);
                Message mes1 = mUiThread.obtainMessage(KEY_GRRC_RESIDENT_OPEN);
                mes1.arg1 = sim;
                mes1.obj = responValue;
                mUiThread.sendMessage(mes1);
                break;
            case KEY_GRRC_RESIDENT_CLOSE:
                sim = (Integer) msg.obj;
                mServerName = "atchannel" + sim;
                atCmd = engconstents.ENG_AT_SET_CSFB2GSM + "0,1";
                responValue = IATUtils.sendATCmd(atCmd, mServerName);
                Log.d(TAG, "<" + sim + ">Channel is " + mServerName
                        + ",KEY_GRRC_RESIDENT_CLOSE " + responValue);
                Message mes2 = mUiThread.obtainMessage(KEY_GRRC_RESIDENT_CLOSE);
                mes2.arg1 = sim;
                mes2.obj = responValue;
                mUiThread.sendMessage(mes2);
                break;
            case KEY_GRRC_RANDOM_OPEN:
                sim = (Integer) msg.obj;
                mServerName = "atchannel" + sim;
                atCmd = engconstents.ENG_AT_SET_CSFB2GSM + "1,2";
                responValue = IATUtils.sendATCmd(atCmd, mServerName);
                Log.d(TAG, "<" + sim + ">Channel is " + mServerName
                        + ",KEY_GRRC_RANDOM_OPEN " + responValue);
                Message mes3 = mUiThread.obtainMessage(KEY_GRRC_RANDOM_OPEN);
                mes3.arg1 = sim;
                mes3.obj = responValue;
                mUiThread.sendMessage(mes3);
                break;
            case KEY_GRRC_RANDOM_CLOSE:
                sim = (Integer) msg.obj;
                mServerName = "atchannel" + sim;
                atCmd = engconstents.ENG_AT_SET_CSFB2GSM + "0,2";
                responValue = IATUtils.sendATCmd(atCmd, mServerName);
                Log.d(TAG, "<" + sim + ">Channel is " + mServerName
                        + ",KEY_GRRC_RANDOM_CLOSE" + responValue);
                Message mes4 = mUiThread.obtainMessage(KEY_GRRC_RANDOM_CLOSE);
                mes4.arg1 = sim;
                mes4.obj = responValue;
                mUiThread.sendMessage(mes4);
                break;
            default:
                break;
            }
        }
    }

}
