package com.sprd.engineermode.telephony;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
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
import com.sprd.engineermode.EMSwitchPreference;
import android.preference.TwoStatePreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

public class HspaSettingActivity extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {

    private static final String TAG = "HspaSettingActivity";
    private static final String KEY_HSDPA = "hsdpa_setting";
    private static final String KEY_HSUPA = "hsupa_setting";

    private static final int GET_HSPA_STATUS = 0;
    private static final int OPEN_HSDPA = 1;
    private static final int CLOSE_HSDPA = 2;
    private static final int OPEN_HSUPA = 3;
    private static final int CLOSE_HSUPA = 4;

    private TwoStatePreference mHsdpa;
    private TwoStatePreference mHsupa;

    private Handler mUiThread = new Handler();
    private mHspaSettingHandler mHspaSettingHandler;
    private Context mContext;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_hspa_setting);
        mContext = this;
        mHsdpa = (EMSwitchPreference) findPreference(KEY_HSDPA);
        mHsdpa.setChecked(false);
        mHsdpa.setOnPreferenceChangeListener(this);
        mHsupa = (EMSwitchPreference) findPreference(KEY_HSUPA);
        mHsupa.setChecked(false);
        mHsupa.setOnPreferenceChangeListener(this);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mHspaSettingHandler = new mHspaSettingHandler(ht.getLooper());

    }

    @Override
    public void onResume() {
        if (mHsdpa != null && mHsdpa.isEnabled() && mHsupa != null
                && mHsupa.isEnabled()) {
            Message getHspaStatus = mHspaSettingHandler
                    .obtainMessage(GET_HSPA_STATUS);
            mHspaSettingHandler.sendMessage(getHspaStatus);
        }
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        if (mHspaSettingHandler != null) {
            Log.d(TAG, "HandlerThread has quit");
            mHspaSettingHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String key = preference.getKey();
        Log.d(TAG, "[change]onPreferenceChange  key=" + key);
        int logType = 0;
        if (key == null) {
            return true;
        }
        if (preference == mHsdpa) {
            if (!mHsdpa.isChecked()) {
                logType = OPEN_HSDPA;
            } else {
                logType = CLOSE_HSDPA;
            }
        } else if (preference == mHsupa) {
            if (!mHsupa.isChecked()) {
                logType = OPEN_HSUPA;
            } else {
                logType = CLOSE_HSUPA;
            }
        } else {
            return true;
        }
        Message mes = mHspaSettingHandler.obtainMessage(logType);
        mHspaSettingHandler.sendMessage(mes);
        return true;
    }

    class mHspaSettingHandler extends Handler {

        public mHspaSettingHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String mATCmd;
            String mStrTmp;
            int mHsdpaEnable;
            int mHsupaEnable;
            switch (msg.what) {
            case GET_HSPA_STATUS: {
                mATCmd = engconstents.ENG_GET_AT_HSPA;
                mStrTmp = IATUtils.sendATCmd(mATCmd, "atchannel0");
                Log.d(TAG, "HSPA Status: " + mStrTmp);
                if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)) {
                    String[] str0 = mStrTmp.split("\n");
                    String[] str1 = str0[0].split(":");
                    String[] str2 = str1[1].split(",");
                    mHsdpaEnable = Integer.valueOf(str2[1].trim().substring(0,
                            1));
                    mHsupaEnable = Integer.valueOf(str2[0].trim().substring(0,
                            1));
                    if (mHsdpaEnable == 1) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mHsdpa.setChecked(true);
                            }
                        });
                    }
                    if (mHsupaEnable == 1) {
                        mUiThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mHsupa.setChecked(true);
                            }
                        });
                    }
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mHsdpa.setEnabled(false);
                            mHsdpa.setSummary(R.string.feature_abnormal);
                            mHsupa.setEnabled(false);
                            mHsupa.setSummary(R.string.feature_abnormal);
                        }
                    });
                }
            }
                break;
            case OPEN_HSDPA: {
                mHsdpaEnable = 1;
                if (mHsupa.isChecked()) {
                    mHsupaEnable = 1;
                } else {
                    mHsupaEnable = 0;
                }
                mATCmd = engconstents.ENG_SET_AT_HSPA + mHsupaEnable + ","
                        + mHsdpaEnable;
                mStrTmp = IATUtils.sendATCmd(mATCmd, "atchannel0");
                Log.d(TAG, "OPEN_HSDPA" + mStrTmp);
                if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mHsdpa.setChecked(true);
                            Toast.makeText(mContext, "Open HSDPA Success",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mHsdpa.setChecked(false);
                            Toast.makeText(mContext, "Open HSDPA Fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
            }
                break;
            case CLOSE_HSDPA: {
                mHsdpaEnable = 0;
                if (mHsupa.isChecked()) {
                    mHsupaEnable = 1;
                } else {
                    mHsupaEnable = 0;
                }
                mATCmd = engconstents.ENG_SET_AT_HSPA + mHsupaEnable + ","
                        + mHsdpaEnable;
                mStrTmp = IATUtils.sendATCmd(mATCmd, "atchannel0");
                Log.d(TAG, "CLOSE_HSDPA" + mStrTmp);
                if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mHsdpa.setChecked(false);
                            Toast.makeText(mContext, "Close HSDPA Success",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mHsdpa.setChecked(true);
                            Toast.makeText(mContext, "Close HSDPA Fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
            }
                break;
            case OPEN_HSUPA: {
                mHsupaEnable = 1;
                if (mHsdpa.isChecked()) {
                    mHsdpaEnable = 1;
                } else {
                    mHsdpaEnable = 0;
                }
                mATCmd = engconstents.ENG_SET_AT_HSPA + mHsupaEnable + ","
                        + mHsdpaEnable;
                mStrTmp = IATUtils.sendATCmd(mATCmd, "atchannel0");
                Log.d(TAG, "OPEN_HSUPA" + mStrTmp);
                if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mHsupa.setChecked(true);
                            Toast.makeText(mContext, "Open HSUPA Success",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mHsupa.setChecked(false);
                            Toast.makeText(mContext, "Open HSUPA Fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
            }
                break;
            case CLOSE_HSUPA: {
                mHsupaEnable = 0;
                if (mHsdpa.isChecked()) {
                    mHsdpaEnable = 1;
                } else {
                    mHsdpaEnable = 0;
                }
                mATCmd = engconstents.ENG_SET_AT_HSPA + mHsupaEnable + ","
                        + mHsdpaEnable;
                mStrTmp = IATUtils.sendATCmd(mATCmd, "atchannel0");
                Log.d(TAG, "CLOSE_HSUPA" + mStrTmp);
                if (mStrTmp != null && mStrTmp.contains(IATUtils.AT_OK)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mHsupa.setChecked(false);
                            Toast.makeText(mContext, "Close HSUPA Success",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mHsupa.setChecked(true);
                            Toast.makeText(mContext, "Close HSUPA Fail",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
            }
                break;
            default:
                break;
            }
        }
    }
}
