package com.sprd.engineermode.telephony;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.widget.Toast;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import com.sprd.engineermode.SimSelectHelper;
import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class PSRelatedPrefActivity extends PreferenceActivity implements
        OnSharedPreferenceChangeListener, Preference.OnPreferenceChangeListener {

    private static final String TAG = "PSRelatedPrefActivity";
    private static final boolean debug = true;

    private static final int STATUS_ON = 1;
    private static final int STATUS_OFF = 0;

    private static final int GET_AUTO_ATTACH = 1;
    private static final int SET_AUTO_ATTACH = 2;
    private static final int GET_PDPSTATE = 3;
    private static final int SET_PDPSTATE = 4;
    private static final int GET_SMSERVER = 5;
    private static final int SET_SMSERVER = 6;
    private static final int GET_TIMECONF = 7;
    private static final int SET_TIMECONF = 8;
    private static final int GET_SIM_INFO = 9;
    private static final int SET_SIM_INFO = 10;
    private static final int SET_UNLOCKCELL = 15;

    private static final String AUTOPOWERON = "autoattach_value";
    private static final String PDPACT = "pdpact";
    private static final String SMSSERVER = "smsserver";
    private static final String TIMECONFLICT = "timeslotconflict";
    private static final String SIMINFO = "siminfo";
    private static final String HSPA_SETTING = "hspa_setting";
    private static final String UNLOCKCELL = "unlockcell";
    private static final String AOCSETTING = "aocsetting";
    private static final String GSM_FREQUENCY = "gsm_frequency";
    private static final String OTHERS_FREQUENCY = "others_frequency";

    private EMSwitchPreference mAutoPowrPref;
    private ListPreference mSmsServerPref;
    private Preference mSimInfoPref;
    private EMSwitchPreference mTimeConflictPref;
    private Preference mHspa;
    private int mState;

    private Handler mUiThread = new Handler();
    private mPsRelativeHandler mPsRelativeHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_psrelative);
        mAutoPowrPref = (EMSwitchPreference) findPreference(AUTOPOWERON);
        mAutoPowrPref.setOnPreferenceChangeListener(this);
        mSmsServerPref = (ListPreference) findPreference(SMSSERVER);
        mTimeConflictPref = (EMSwitchPreference) findPreference(TIMECONFLICT);
        mTimeConflictPref.setOnPreferenceChangeListener(this);
        mSimInfoPref = findPreference(SIMINFO);
        mHspa = findPreference(HSPA_SETTING);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mPsRelativeHandler = new mPsRelativeHandler(ht.getLooper());

        Message getAutoPower = mPsRelativeHandler
                .obtainMessage(GET_AUTO_ATTACH);
        mPsRelativeHandler.sendMessage(getAutoPower);
        if (TelephonyManagerSprd.MODEM_TYPE_TDSCDMA != TelephonyManagerSprd.getModemType()) {
            mTimeConflictPref.setEnabled(false);
            mTimeConflictPref.setSummary(R.string.feature_not_support);
        } else {
            Message getTimeConflict = mPsRelativeHandler
                    .obtainMessage(GET_TIMECONF);
            mPsRelativeHandler.sendMessage(getTimeConflict);
        }
        if ((TelephonyManagerSprd.getModemType() == TelephonyManagerSprd.MODEM_TYPE_WCDMA)
                || TelephonyManagerSprd.getRadioCapbility() == TelephonyManagerSprd.RadioCapbility.FDD_CSFB) {
            mHspa.setEnabled(true);
        } else {
            mHspa.setEnabled(false);
            mHspa.setSummary(R.string.feature_not_support);
        }

        Message getSmsServerInfo = mPsRelativeHandler
                .obtainMessage(GET_SMSERVER);
        mPsRelativeHandler.sendMessage(getSmsServerInfo);
        mSmsServerPref.setSummary(mSmsServerPref.getEntry());

        SharedPreferences defaultPrefs = PreferenceManager
                .getDefaultSharedPreferences(this);
        defaultPrefs.registerOnSharedPreferenceChangeListener(this);

        if (mSimInfoPref != null && !isCardExist()) {
            mSimInfoPref.setEnabled(false);
        }
    }

    private boolean isCardExist() {
        int phoneCount = TelephonyManager.from(this).getPhoneCount();
        int cardExistNum = 0;
        TelephonyManager telephonyManager[] = new TelephonyManager[phoneCount];
        for (int i = 0; i < phoneCount; i++) {
            telephonyManager[i] = (TelephonyManager) TelephonyManager
                    .from(PSRelatedPrefActivity.this);
            if (telephonyManager[i] != null
                    && telephonyManager[i].getSimState(i) == TelephonyManager.SIM_STATE_READY) {
                cardExistNum++;
            }
        }
        if (cardExistNum >= 1) {
            return true;
        } else {
            return false;
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        Log.d(TAG, "[change]onPreferenceChange  key=" + key);
        int logType = 0;

        if (key == null) {
            return true;
        }
        if (preference instanceof EMSwitchPreference) {
            int statusWant = STATUS_ON;
            if (((EMSwitchPreference) preference).isChecked()) {
                statusWant = STATUS_OFF;
            }

            if (preference == mAutoPowrPref) {
                logType = SET_AUTO_ATTACH;
            } else if (preference == mTimeConflictPref) {
                logType = SET_TIMECONF;
            } else {
                return true;
            }
            Message m = mPsRelativeHandler.obtainMessage(logType, statusWant,
                    0, preference);
            mPsRelativeHandler.sendMessage(m);
            preference.setEnabled(false);
        }
        return true;
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) {
        String key = preference.getKey();
        Log.d(TAG, "[TreeCllik]onPreferenceTreeClick  key=" + key);
        int logType = 0;

        if (key == null) {
            return true;
        }

        if (key.equals(SIMINFO)) {
            logType = GET_SIM_INFO;
        } else if (key.equals(SMSSERVER)) {
            logType = GET_SMSERVER;
        } else if (key.equals(UNLOCKCELL)) {
            logType = SET_UNLOCKCELL;
        } else if (key.equals(AOCSETTING)) {
            Intent intent = new Intent(this, AOCSettingsPrefActivity.class);
            SimSelectHelper.startSimSelect(intent, true, null, this);
        } else if (key.equals(GSM_FREQUENCY)) {
            Intent intent = new Intent("android.intent.action.GSMFREQUENCY");
            startActivity(intent);
        } else if (key.equals(OTHERS_FREQUENCY)) {
            Intent intent = new Intent("android.intent.action.LOCKFREQUENCY");
            startActivity(intent);
        } else {
            return false;
        }
        Message m = mPsRelativeHandler.obtainMessage(logType);
        mPsRelativeHandler.sendMessage(m);
        return true;
    }

    private String analysisResponse(String response, int type) {
        if (response.contains(IATUtils.AT_OK)) {
            switch (type) {
            case GET_AUTO_ATTACH:
            case GET_SMSERVER: {
                String[] str = response.split("\n");
                String[] str1 = str[0].split(":");
                if (debug)
                    Log.d(TAG, type + "  " + str1[1]);
                return str1[1].trim();
            }
            case GET_TIMECONF: {
                String[] str = response.split("\n");
                String[] str1 = str[0].split(",");

                for (int i = 0; i < str1.length; i++) {
                    Log.d(TAG, type + "  " + str1[i]);
                }
                if (debug)
                    Log.d(TAG, type + "  " + str1[2]);
                return str1[2].trim();
            }
            case GET_PDPSTATE: {
                String[] str = response.split("\n");
                String[] str1 = str[0].split(",");
                if (debug)
                    Log.d(TAG, type + "  " + str1[1]);
                return str1[1].trim();
            }
            case SET_UNLOCKCELL:
            case SET_AUTO_ATTACH:
            case SET_TIMECONF:
            case SET_SMSERVER:
                return IATUtils.AT_OK;
            }
        }
        return IATUtils.AT_FAIL;
    }

    class mPsRelativeHandler extends Handler {

        public mPsRelativeHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String response;
            String analysisResponse;
            switch (msg.what) {
            case PSRelatedPrefActivity.GET_AUTO_ATTACH: {
                response = IATUtils.sendATCmd(engconstents.ENG_AT_GETAUTOATT,
                        "atchannel0");
                analysisResponse = analysisResponse(response, GET_AUTO_ATTACH);
                if (analysisResponse.equals("1")) {
                    mUiThread.post(new Runnable() {

                        @Override
                        public void run() {
                            mAutoPowrPref.setChecked(true);
                        }

                    });
                } else {
                    mUiThread.post(new Runnable() {

                        @Override
                        public void run() {
                            mAutoPowrPref.setChecked(false);
                        }

                    });
                }

            }
                break;
            case PSRelatedPrefActivity.SET_AUTO_ATTACH: {
                if (msg.arg1 == STATUS_ON) {
                    response = IATUtils.sendATCmd(
                            engconstents.ENG_AT_SETAUTOATT + "1", "atchannel0");

                } else {
                    response = IATUtils.sendATCmd(
                            engconstents.ENG_AT_SETAUTOATT + "0", "atchannel0");
                }
                analysisResponse = analysisResponse(response, SET_AUTO_ATTACH);
                if (analysisResponse.equals(IATUtils.AT_OK)) {
                    mUiThread.post(new PSRelativeATSuccessRunnable(msg));
                } else {
                    mUiThread.post(new PSRelativeATFailRunnable(msg));
                }
            }
                break;
            case PSRelatedPrefActivity.GET_SMSERVER: {
                response = IATUtils.sendATCmd(engconstents.ENG_AT_GETSMSSERVER,
                        "atchannel0");
                analysisResponse = analysisResponse(response, GET_SMSERVER);
                if (!analysisResponse.contains(IATUtils.AT_FAIL)) {
                    mState = Integer.valueOf(analysisResponse);
                    if (!analysisResponse.equals("-1")) {
                        mUiThread.post(new Runnable() {

                            @Override
                            public void run() {
                                mSmsServerPref.setValueIndex(Integer
                                        .valueOf(mState));
                            }

                        });
                    }
                }
            }
                break;
            case PSRelatedPrefActivity.SET_SMSERVER: {
                String valueStr = (String) msg.obj;
                response = IATUtils.sendATCmd(engconstents.ENG_AT_SETSMSSERVER
                        + (Integer.valueOf(valueStr) - 1), "atchannel0");
                Log.d(TAG,
                        engconstents.ENG_AT_SETSMSSERVER
                                + (Integer.valueOf(valueStr) - 1));
                analysisResponse = analysisResponse(response, SET_SMSERVER);
                if (analysisResponse.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new PSRelativeATSuccessRunnable(msg));
                } else {
                    mUiThread.post(new PSRelativeATFailRunnable(msg));
                }
            }
                break;
            case PSRelatedPrefActivity.GET_TIMECONF: {
                response = IATUtils.sendATCmd(
                        engconstents.ENG_AT_GETTIMECONFLICT, "atchannel0");
                Log.d(TAG, response);
                analysisResponse = analysisResponse(response, GET_TIMECONF);
                if ("1".equals(analysisResponse)) {
                    mUiThread.post(new Runnable() {

                        @Override
                        public void run() {
                            mTimeConflictPref.setChecked(true);
                        }

                    });
                } else if ("0".equals(analysisResponse)) {
                    mUiThread.post(new Runnable() {

                        @Override
                        public void run() {
                            mTimeConflictPref.setChecked(false);
                        }

                    });
                } else {
                    mUiThread.post(new Runnable() {

                        @Override
                        public void run() {
                            mTimeConflictPref.setChecked(false);
                            mTimeConflictPref.setSummary("Get State Fail!!");
                        }

                    });
                }
            }
                break;

            case PSRelatedPrefActivity.SET_TIMECONF: {
                if (msg.arg1 == STATUS_ON) {
                    response = IATUtils.sendATCmd(
                            engconstents.ENG_AT_SETTIMECONFLICT + "1",
                            "atchannel0");
                } else {
                    response = IATUtils.sendATCmd(
                            engconstents.ENG_AT_SETTIMECONFLICT + "0",
                            "atchannel0");
                }
                Log.d(TAG, response);
                analysisResponse = analysisResponse(response, SET_TIMECONF);
                Log.v(TAG, "SET_TIMECONF~ mATResponse = " + response
                        + " , and analysisResponse = " + analysisResponse);
                if (analysisResponse.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new PSRelativeATSuccessRunnable(msg));
                } else {
                    mUiThread.post(new PSRelativeATFailRunnable(msg));
                }
            }
                break;
            case PSRelatedPrefActivity.GET_SIM_INFO: {
                Intent intent = new Intent(PSRelatedPrefActivity.this,
                        TextInfoActivity.class);
                startActivity(intent.putExtra("text_info", 5));
            }
                break;
            case PSRelatedPrefActivity.SET_SIM_INFO: {

            }
                break;
            case PSRelatedPrefActivity.SET_UNLOCKCELL: {
                response = IATUtils.sendATCmd(engconstents.ENG_AT_SETSPFRQ1
                        + "1," + "0", "atchannel0");
                analysisResponse = analysisResponse(response, SET_UNLOCKCELL);
                if (analysisResponse.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new PSRelativeATSuccessRunnable(msg));
                } else {
                    mUiThread.post(new PSRelativeATFailRunnable(msg));
                }
            }
                break;
            }
        }

    }

    class PSRelativeATFailRunnable implements Runnable {
        private int arg1 = 0;
        private Object obj = null;

        public PSRelativeATFailRunnable(Message msgFailed) {
            arg1 = msgFailed.arg1;
            obj = msgFailed.obj;
        }

        @Override
        public void run() {
            Toast.makeText(PSRelatedPrefActivity.this, "Fail",
                    Toast.LENGTH_SHORT).show();
            if (obj instanceof EMSwitchPreference) {
                boolean resetValue = (arg1 == STATUS_OFF) ? true : false;

                ((EMSwitchPreference) obj).setChecked(resetValue);
                ((EMSwitchPreference) obj).setEnabled(true);
            }
        }
    }

    class PSRelativeATSuccessRunnable implements Runnable {
        private int arg1 = 0;
        private Object obj = null;

        public PSRelativeATSuccessRunnable(Message msgSuccessed) {
            arg1 = msgSuccessed.arg1;
            obj = msgSuccessed.obj;
        }

        @Override
        public void run() {
            Toast.makeText(PSRelatedPrefActivity.this, "Success",
                    Toast.LENGTH_SHORT).show();
            if (obj instanceof EMSwitchPreference) {
                ((EMSwitchPreference) obj).setEnabled(true);
            }
        }

    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences,
            String key) {
        if (key.equals(SMSSERVER)) {
            String re = sharedPreferences.getString(key, "");
            mSmsServerPref.setSummary(mSmsServerPref.getEntry());
            Log.d(TAG, "onSharedPreferenceChanged key=" + key + " value=" + re);
            Message mSetDSPLog = mPsRelativeHandler.obtainMessage(SET_SMSERVER,
                    re);
            mPsRelativeHandler.sendMessage(mSetDSPLog);
        }

    }

    @Override
    protected void onDestroy() {
        if (mPsRelativeHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mPsRelativeHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        finish();
        super.onBackPressed();
    }
}
