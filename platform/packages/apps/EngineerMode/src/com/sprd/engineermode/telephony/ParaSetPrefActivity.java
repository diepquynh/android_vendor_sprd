
package com.sprd.engineermode.telephony;

import java.util.HashMap;
import android.os.PowerManager.WakeLock;
import android.os.PowerManager;
import java.util.Iterator;
import java.util.Set;
import java.util.Map.Entry;

import android.app.AlertDialog.Builder;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.widget.Toast;
import android.os.Looper;
import android.content.SharedPreferences;
import android.os.HandlerThread;
import android.os.SystemProperties;
import android.preference.PreferenceManager;
import android.content.BroadcastReceiver;
import android.content.Context;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;
import android.os.SystemProperties;

import com.sprd.engineermode.telephony.SendPowerPrefActivity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;

public class ParaSetPrefActivity extends PreferenceActivity implements
        EMSwitchPreference.OnPreferenceChangeListener {
    private static final String TAG = "ParaSetPrefActivity";
    private static final String FORBIDPLMN = "key_forbidplmn";
    private static final String PLMNSELECT = "key_plmnselect";
    private static final String MANUAL_MODE = "key_manualassert";
    private static final String SLEEP_MODE = "key_sleepmode";
    private static final String CP2_RESET = "key_cp2_reset";
    private static final String MODEM_ASSERT = "modem_assert";
    private static final String MODEM_STAT = "modem_stat";
    private static final String MODEM_ALIVE = "modem_alive";
    private static final String SEND_POWER = "key_sendpower";
    private static final String DIVERSITY = "diversity";
    private static final String KEY_VAMOS = "key_vamos";

    private static final int MSG_MODEM_ASSERT = 0;

    private Handler mUiThread = new Handler();
    private ParaSetHandler mParaSetHandler;
    private Preference mVamos;
    private static Preference mModemAssert;
    private Preference mCp2Assert;
    private boolean mATResponOK = false;
    private WakeLock mWakeLock;
    private static SharedPreferences mSharePref;
    private Preference mSendPower;
    private static ProgressDialog mProgress;
    private PreferenceScreen mDiversity;

    private boolean isSupportLTE = (TelephonyManagerSprd.getModemType() == TelephonyManagerSprd.MODEM_TYPE_LTE);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mParaSetHandler = new ParaSetHandler(ht.getLooper());
        addPreferencesFromResource(R.xml.pref_paraset);
        mModemAssert = (Preference) findPreference(MANUAL_MODE);
        mSendPower = (Preference) findPreference(SEND_POWER);
        // SPRD: Bug 527396 add Vamos switch in engineermode
        mVamos = (Preference) findPreference(KEY_VAMOS);

        if (mSendPower != null && !isSupportLTE) {
            mSendPower.setEnabled(false);
            mSendPower.setSummary(R.string.feature_not_support);
        }
        mCp2Assert = (Preference) findPreference(CP2_RESET);
        mSharePref = PreferenceManager.getDefaultSharedPreferences(this);
        mDiversity = (PreferenceScreen) findPreference(DIVERSITY);
    }

    @Override
    protected void onStart() {
        mModemAssert.setEnabled(mSharePref.getBoolean(MANUAL_MODE, true));
        if (mCp2Assert != null) {
            if (SystemProperties.get("ro.modem.wcn.enable", "0").equals("0")) {
                mCp2Assert.setSummary(R.string.feature_not_support);
                mCp2Assert.setEnabled(false);
            }
        }
        super.onStart();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        return true;
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {

        if (preference.getKey().endsWith(MANUAL_MODE)) {
            Message manualAssert = mParaSetHandler.obtainMessage(MSG_MODEM_ASSERT);
            mParaSetHandler.sendMessage(manualAssert);
            mSharePref.edit().putBoolean(MANUAL_MODE, false).commit();
            mModemAssert.setEnabled(false);
            /*
             * SPRD: modify 20140514 Spreadtrum of 312036 EngineerMode, para set-manual assert show
             * the result of processing @{
             */
            Toast.makeText(ParaSetPrefActivity.this, "success", Toast.LENGTH_SHORT).show();
            if (SystemProperties.getBoolean("persist.sys.sprd.modemreset", false)) {
                mProgress = ProgressDialog.show(ParaSetPrefActivity.this, "modem assert...",
                        "Please wait...", false, false);

                // sleep 9000ms to make sure modem and ril restart completely
                new Thread(new Runnable() {
                    public void run() {
                        try {
                            Thread.sleep(9000);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                        if (mProgress != null) {
                            mProgress.dismiss();
                            mProgress = null;
                        }
                        /* SPRD:modify bug 390645 */
                        if (mModemAssert != null) {
                            mUiThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mModemAssert.setEnabled(mSharePref
                                            .getBoolean(MANUAL_MODE, true));
                                }
                            });
                        }
                    }
                }).start();
            }
            /* 312036}@ */
        } else if (preference.getKey().endsWith(FORBIDPLMN)) {
            Intent intent = new Intent(this, TextInfoActivity.class);
            startActivity(intent.putExtra("text_info", 1));
        } else if (preference.getKey().endsWith(PLMNSELECT)) {
            Intent intent = new Intent(this, TextInfoActivity.class);
            startActivity(intent.putExtra("text_info", 2));
        } else if (preference.getKey().endsWith(SEND_POWER)) {
            AlertDialog alertDialog = new AlertDialog.Builder(
                    ParaSetPrefActivity.this)
                    .setTitle(getString(R.string.send_power))
                    .setMessage(getString(R.string.send_power_dailog))
                    .setPositiveButton(getString(R.string.alertdialog_ok),
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    Intent intent = new Intent(
                                            ParaSetPrefActivity.this,
                                            SendPowerPrefActivity.class);
                                    startActivity(intent);
                                }
                            })
                    .setNegativeButton(R.string.alertdialog_cancel,
                            new DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                }
                            }).create();
            alertDialog.show();
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    @Override
    protected void onDestroy() {
        if (mParaSetHandler != null) {
            mParaSetHandler.getLooper().quit();
            Log.d(TAG, "HandlerThread has quit");
        }
        super.onDestroy();
    }

    private void acquireWakeLock() {
        if (mWakeLock == null) {
            PowerManager pm = (PowerManager) getSystemService(POWER_SERVICE);
            mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);
            mWakeLock.acquire();
        }
    }

    private void releaseWakeLock() {
        if (mWakeLock != null && mWakeLock.isHeld()) {
            mWakeLock.release();
            mWakeLock = null;
        }
    }

    public static class ModemAssertReceiver extends BroadcastReceiver {
        /*
         * if modem reset not open, manual assert can do only one time if modem reset open, manual
         * assert enable when receive broadcast with com.sprd.modemassert.MODEM_STAT_CHANGE action
         */
        @Override
        public void onReceive(Context context, Intent intent) {
            String modemAsserStat = intent.getStringExtra(MODEM_STAT);
            Log.d(TAG, "modem state is " + modemAsserStat);
            if (MODEM_ALIVE.equals(modemAsserStat)) {
                if (SystemProperties.getBoolean("persist.sys.sprd.modemreset", false)) {
                    /* SPRD:modify bug 393941 modem assert through arm log tools */
                    if (mSharePref == null) {
                        mSharePref = PreferenceManager.getDefaultSharedPreferences(context);
                    }
                    mSharePref.edit().putBoolean(MANUAL_MODE, true).commit();
                }
            }
        }
    }

    class ParaSetHandler extends Handler {
        public ParaSetHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_MODEM_ASSERT:
                    IATUtils.sendATCmd(engconstents.ENG_AT_SET_MANUAL_ASSERT, "atchannel0");
                    break;
                default:
                    break;
            }
        }
    }
}
