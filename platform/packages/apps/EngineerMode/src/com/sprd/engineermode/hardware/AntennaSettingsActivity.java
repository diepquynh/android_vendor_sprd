package com.sprd.engineermode.hardware;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.util.Log;
import android.widget.Toast;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.content.SharedPreferences.Editor;
import android.content.Context;
import com.sprd.engineermode.R;
import android.os.PowerManager;
import android.os.ServiceManager;

import android.os.SystemProperties;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.engconstents;

public class AntennaSettingsActivity extends PreferenceActivity implements
        OnPreferenceChangeListener {

    private static final String TAG = "AntennaSettingsActivity";
    private static final String KEY_LTE_SET = "lte_set";
    private static final String KEY_WCDMA_SET = "wcdma_set";
    private static final int GET_ANTENNA_STATE = 0;
    private static final int SET_LET_ANTENNA = 1;
    private static final int SET_WCDMA_ANTENNA = 2;

    private static final int LTE_VALUE_DEFAULT = -1;
    private static final int WCDMA_VALUE_DEFAULT = -1;

    private int mCurrentLValue = LTE_VALUE_DEFAULT;
    private int mCurrentWValue = WCDMA_VALUE_DEFAULT;

    private ListPreference mLteList;
    private ListPreference mWcdmaList;

    private AnSetHandler mAnSetHandler;
    private Handler mUiThread = new Handler();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_antenna_set);
        mLteList = (ListPreference) findPreference(KEY_LTE_SET);
        mWcdmaList = (ListPreference) findPreference(KEY_WCDMA_SET);
        mLteList.setOnPreferenceChangeListener(this);
        mWcdmaList.setOnPreferenceChangeListener(this);

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mAnSetHandler = new AnSetHandler(ht.getLooper());
    }

    @Override
    public void onStart() {
        if (mLteList != null && mWcdmaList != null) {
            Message mGetAntennaState = mAnSetHandler
                    .obtainMessage(GET_ANTENNA_STATE);
            mAnSetHandler.sendMessage(mGetAntennaState);
        }
        super.onStart();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mAnSetHandler != null) {
            mAnSetHandler.getLooper().quit();
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        final String re = newValue.toString();
        Log.d(TAG, "onPreferenceChange" + re);
        if (preference instanceof ListPreference) {
            ListPreference listPreference = (ListPreference) preference;
            String key = listPreference.getKey();
            if (key.equals(KEY_LTE_SET)) {
                Log.d(TAG, "Lte antenna settings");
                createDialog(SET_LET_ANTENNA, re);
                return true;
            } else if (key.equals(KEY_WCDMA_SET)) {
                Log.d(TAG, "Wcdma antenna settings");
                createDialog(SET_WCDMA_ANTENNA, re);
                return true;
            }
        }
        return false;
    }

    private int changeValueToIndex(String PrefKey, int value) {
        int valueIndex = 0;
        Log.d(TAG, "Key value: " + PrefKey + ", Get value: " + value);
        if (KEY_LTE_SET.equals(PrefKey)) {
            valueIndex = value;
        } else if (KEY_WCDMA_SET.equals(PrefKey)) {
            if (value == 0) {
                valueIndex = 1;
            } else if (value == 1) {
                valueIndex = 0;
            } else if (value == 17) {
                valueIndex = 2;
            }
        }
        return valueIndex;
    }

    private int changeIndexToValue(String PrefKey, int setValueIndex) {
        int setValue = 0;
        Log.d(TAG, "Key value: " + PrefKey + ", set Value Index: "
                + setValueIndex);
        if (KEY_LTE_SET.equals(PrefKey)) {
            setValue = setValueIndex;
        } else if (KEY_WCDMA_SET.equals(PrefKey)) {
            if (setValueIndex == 0) {
                setValue = 1;
            } else if (setValueIndex == 1) {
                setValue = 0;
            } else if (setValueIndex == 2) {
                setValue = 17;
            }
        }
        return setValue;
    }

    void createDialog(final int message, final String re) {
        AlertDialog alertDialog = new AlertDialog.Builder(AntennaSettingsActivity.this)
                .setTitle(getString(R.string.antenna_set))
                .setMessage(getString(R.string.mode_switch_waring))
                .setCancelable(false)
                .setPositiveButton(getString(R.string.alertdialog_ok),
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                Message mSetAntenna = mAnSetHandler
                                        .obtainMessage(message, re);
                                mAnSetHandler.sendMessage(mSetAntenna);
                            }
                        })
                .setNegativeButton(R.string.alertdialog_cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog,
                                    int which) {
                                update();
                            }
                        }).create();
        alertDialog.show();
    }

    public void update() {
        if (mCurrentLValue == 0 || mCurrentLValue == 1 || mCurrentLValue == 2) {
            mUiThread.post(new Runnable() {
                public void run() {
                    mLteList.setValueIndex(changeValueToIndex(KEY_LTE_SET,
                            mCurrentLValue));
                    mLteList.setSummary(mLteList.getEntry());
                }
            });
        } else {
            mUiThread.post(new Runnable() {
                public void run() {
                    mLteList.setEnabled(false);
                    mLteList.setSummary(R.string.feature_not_support);
                }
            });
        }
        Log.d(TAG, "mCurrentLValue1:"+mCurrentLValue+"mCurrentWValue1:"+mCurrentWValue);
        if (mCurrentWValue == 0 || mCurrentWValue == 1 || mCurrentWValue == 17) {
            mUiThread.post(new Runnable() {
                public void run() {
                    mWcdmaList.setValueIndex(changeValueToIndex(KEY_WCDMA_SET,
                            mCurrentWValue));
                    mWcdmaList.setSummary(mWcdmaList.getEntry());
                }
            });
        } else {
            mUiThread.post(new Runnable() {
                public void run() {
                    mWcdmaList.setEnabled(false);
                    mWcdmaList.setSummary(R.string.feature_not_support);
                }
            });
        }
    }

    public void powerOff() {
        String atCmd = null;
        String atResult = null;
        try{
            Thread.sleep(3000);
        } catch(Exception e){
            Log.d(TAG, "" + e);
        }
        atCmd = engconstents.ENG_GET_ANTENNA_STATE;
        atResult = IATUtils.sendATCmd(atCmd, "atchannel0");
        Log.d(TAG, "poweroff atCmd :" + atCmd+"poweroff atResult:"+atResult);
        PowerManager pm = (PowerManager) AntennaSettingsActivity.this
                .getSystemService(Context.POWER_SERVICE);
        pm.reboot("AntennaSettingsActivity");
    }
    class AnSetHandler extends Handler {
        public AnSetHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String atCmd = null;
            String atResult = null;
            switch (msg.what) {
            case GET_ANTENNA_STATE:
                atCmd = engconstents.ENG_GET_ANTENNA_STATE;
                atResult = IATUtils.sendATCmd(atCmd, "atchannel0");
                Log.d(TAG, atCmd + ": " + atResult);
                if (atResult != null && atResult.contains(IATUtils.AT_OK)) {
                    String[] str0 = atResult.split("\n");
                    String[] str1 = str0[0].split(":")[1].split(",");
                    if (str1.length >= 2) {
                        mCurrentLValue = Integer.parseInt(str1[0].trim());
                        mCurrentWValue = Integer.parseInt(str1[1].trim());
                        Log.d(TAG, "mCurrentLValue:"+mCurrentLValue+"mCurrentWValue:"+mCurrentWValue);
                    } else {
                        mCurrentWValue = Integer.parseInt(str1[0].trim());
                    }
                } else {
                    Log.d(TAG, "Get antanne state fail");
                }
                update();
                break;
            case SET_LET_ANTENNA:
                String setLteValue = (String) msg.obj;
                atCmd = engconstents.ENG_SET_ANTENNA
                        + String.valueOf(changeIndexToValue(KEY_LTE_SET,
                                Integer.parseInt(setLteValue))
                                + ","
                                + String.valueOf(mCurrentWValue));
                atResult = IATUtils.sendATCmd(atCmd, "atchannel0");
                Log.d(TAG, atCmd + ": " + atResult);
                if (atResult != null && atResult.contains(IATUtils.AT_OK)) {
                    Log.d(TAG, "set sucess");
                    powerOff();
                } else {
                    Log.d(TAG, "set fail");
                    update();
                    mUiThread.post(new Runnable() {
                        public void run() {
                            Toast.makeText(AntennaSettingsActivity.this, "set fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                break;
            case SET_WCDMA_ANTENNA:
                String setWcdmaValue = (String) msg.obj;
                atCmd = engconstents.ENG_SET_ANTENNA
                        + (mLteList.isEnabled() ? String.valueOf(mCurrentLValue)+"," : "")
                        + String.valueOf(changeIndexToValue(KEY_WCDMA_SET,
                                Integer.parseInt(setWcdmaValue)));
                atResult = IATUtils.sendATCmd(atCmd, "atchannel0");
                Log.d(TAG, atCmd + ": " + atResult);
                if (atResult != null && atResult.contains(IATUtils.AT_OK)) {
                    Log.d(TAG, "set sucess");
                    powerOff();
                } else {
                    Log.d(TAG, "set fail");
                    update();
                    mUiThread.post(new Runnable() {
                        public void run() {
                            Toast.makeText(AntennaSettingsActivity.this, "set fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                break;
            }
        }
    }
}
