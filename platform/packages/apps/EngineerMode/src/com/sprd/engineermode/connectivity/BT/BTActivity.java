
package com.sprd.engineermode.connectivity.BT;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.LocalSocketAddress;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.TwoStatePreference;
import android.util.Log;
import android.os.SystemProperties;
import android.widget.Toast;

import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.SocketUtils;

public class BTActivity extends PreferenceActivity implements OnPreferenceChangeListener,
        Preference.OnPreferenceClickListener {

    private static final String TAG = "BTActivity";

    private static final String KEY_BT_EUT = "bt_eut";
    private static final String KEY_BT_NOSIGNAL_TEST = "bt_nosignal_test";
    private static final String KEY_BQB_MODE = "bqb_mode";
    private static final String KEY_NO_SSP = "no_ssp";

    // private static final String CMD_BT_ON = "eng bt bt_on";
    // private static final String CMD_BT_OFF = "eng bt bt_off";
    private static final String CMD_BT_EUT_OPEN = "eng bt dut_mode_configure 1";
    private static final String CMD_BT_EUT_CLOSE = "eng bt dut_mode_configure 0";
    private static final String CMD_BT_EUT_STATUS = "eng bt eut_status";

    private static final String PERSIST_BT_NO_SSP = "persist.sys.bt.non.ssp";
    private static final String SOCKET_NAME = "wcnd_eng";

    private static final int MSG_BT_OFF = 0;
    private static final int MSG_BT_ON = 1;
    private static final int MSG_GET_EUT_STATUS = 2;
    private static final int MSG_SET_EUT_ON = 3;
    private static final int MSG_SET_EUT_OFF = 4;
    private static final int MSG_SET_BQB_ON = 5;
    private static final int MSG_SET_BQB_OFF = 6;

    private static final boolean isSprdBoard = SystemProperties.getBoolean("ro.modem.wcn.enable",false);

    private Preference mBTNosignalTest;
    private TwoStatePreference mBtEUT;
    private TwoStatePreference mSsp;
    private TwoStatePreference mBqbMode;

    private Handler mUIThread = new Handler();
    private BTHandler mBtHandler;

    private String mCmdRes;

    /**
     * this adapted to bcm and sprd ro.modem.wcn.enable:1 sprd
     * ro.modem.wcn.enable:0 bcm and other not sprd CR:383499
     */
    // private boolean mIsSprd = SystemProperties.get("ro.modem.wcn.enable",
    // "0").equals("1");

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        this.addPreferencesFromResource(R.xml.pref_bt);

        mBtEUT = (TwoStatePreference) this.findPreference(KEY_BT_EUT);
        mBtEUT.setOnPreferenceChangeListener(this);
        if (SystemProperties.get("ro.build.display.id", "unknown").contains("9630")) {
            mBtEUT.setEnabled(false);
            mBtEUT.setSummary(R.string.feature_not_support);
        }

        mBqbMode = (TwoStatePreference) this.findPreference(KEY_BQB_MODE);
        mBqbMode.setOnPreferenceChangeListener(this);
        // BQB Mode base api does not merge,app disabled
        mBqbMode.setChecked(false);
        mBqbMode.setEnabled(false);
        mSsp = (TwoStatePreference) this.findPreference(KEY_NO_SSP);
        mSsp.setOnPreferenceChangeListener(this);
        mBTNosignalTest = (Preference) this.findPreference(KEY_BT_NOSIGNAL_TEST);
        mBTNosignalTest.setOnPreferenceClickListener(this);

        if (!isSprdBoard) {
            mBqbMode.setEnabled(false);
            mSsp.setEnabled(false);
            mBTNosignalTest.setEnabled(false);
        }

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mBtHandler = new BTHandler(ht.getLooper());
    }

    @Override
    protected void onStart() {
        if (mSsp != null && mSsp.isEnabled()) {
            String sspStatus = SystemProperties.get(PERSIST_BT_NO_SSP, "");
            Log.d(TAG, "sspStatus is " + sspStatus);
            if ("open".equals(sspStatus)) {
                mSsp.setChecked(true);
            }
            if ("close".equals(sspStatus)) {
                mSsp.setChecked(false);
            }
        }

        if (mBtEUT != null && mBtEUT.isEnabled()) {
            Message get_bt_eut_status = mBtHandler.obtainMessage(MSG_GET_EUT_STATUS);
            mBtHandler.sendMessage(get_bt_eut_status);
        }
        super.onStart();
    }

    @Override
    public void onBackPressed() {
        SocketUtils.closeSocket();
        super.onBackPressed();
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object value) {
        if (pref.getKey().equals(KEY_NO_SSP)) {
            if (mSsp.isChecked()) {
                SystemProperties.set(PERSIST_BT_NO_SSP, "close");
                mSsp.setChecked(false);
            } else {
                SystemProperties.set(PERSIST_BT_NO_SSP, "open");
                mSsp.setChecked(true);
            }
        } else if (pref.getKey().equals(KEY_BQB_MODE)) {
            if (mBqbMode.isChecked()) {
                Message close_bqb = mBtHandler.obtainMessage(MSG_SET_BQB_OFF);
                mBtHandler.sendMessage(close_bqb);
            } else {
                Message open_bqb = mBtHandler.obtainMessage(MSG_SET_BQB_ON);
                mBtHandler.sendMessage(open_bqb);
            }
        } else if (pref.getKey().equals(KEY_BT_EUT)) {
            mBtEUT.setEnabled(false);
            if (mBtEUT.isChecked()) {
                Message close_eut = mBtHandler.obtainMessage(MSG_SET_EUT_OFF);
                mBtHandler.sendMessage(close_eut);
            } else {
                Message close_eut = mBtHandler.obtainMessage(MSG_SET_EUT_ON);
                mBtHandler.sendMessage(close_eut);
            }
        }
        return false;
    }

    @Override
    public boolean onPreferenceClick(Preference pref) {
        if (pref.getKey().equals(KEY_BT_NOSIGNAL_TEST)) {
            if (mBtEUT.isChecked()) {
                AlertDialog alertDialog = new AlertDialog.Builder(this)
                        .setTitle(getString(R.string.alert_bt_test))
                        .setMessage(getString(R.string.alert_close_bt_eut))
                        .setPositiveButton(getString(R.string.alertdialog_ok),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                    }
                                }).create();
                alertDialog.show();
            } else {
                Intent intent = new Intent(this,
                        com.sprd.engineermode.connectivity.BT.BTNOSIGNALActivity.class);
                startActivity(intent);
            }
        }
        return false;
    }

    class BTHandler extends Handler {

        public BTHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_GET_EUT_STATUS:
                    if (sendCmd(SOCKET_NAME, CMD_BT_EUT_STATUS, MSG_GET_EUT_STATUS)) {
                        String[] str = mCmdRes.split("\\:");
                        String status = str[1].trim();
                        if (status.equals("1")) {
                            mUIThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mBtEUT.setChecked(true);
                                }
                            });
                        } else {
                            mUIThread.post(new Runnable() {
                                @Override
                                public void run() {
                                    mBtEUT.setChecked(false);
                                }
                            });
                        }
                    } else {
                        Log.d(TAG, "GET_BT_EUT_STATUS Fail");
                        Toast.makeText(BTActivity.this, "GET_BT_EUT_STATUS Fail",
                                Toast.LENGTH_SHORT).show();
                    }
                    break;
                case MSG_SET_EUT_ON:
                    if (sendCmd(SOCKET_NAME, CMD_BT_EUT_OPEN, MSG_SET_EUT_ON)) {
                        mUIThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mBtEUT.setEnabled(true);
                                mBtEUT.setChecked(true);
                            }
                        });
                    } else {
                        mUIThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mBtEUT.setEnabled(true);
                                mBtEUT.setChecked(false);
                            }
                        });
                    }
                    break;
                case MSG_SET_EUT_OFF:
                    if (sendCmd(SOCKET_NAME, CMD_BT_EUT_CLOSE, MSG_SET_EUT_OFF)) {
                        mUIThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mBtEUT.setEnabled(true);
                                mBtEUT.setChecked(false);
                            }
                        });
                    } else {
                        mUIThread.post(new Runnable() {
                            @Override
                            public void run() {
                                mBtEUT.setEnabled(true);
                                mBtEUT.setChecked(true);
                            }
                        });
                    }
                    break;
                case MSG_SET_BQB_ON:
                    break;
                case MSG_SET_BQB_OFF:
                    break;
                default:
                    break;
            }
        }
    }

    private boolean sendCmd(String socketName, String cmd, int msg) {
        String res = null;
        /**
         * bcm need to connect "BDTD" server
         */
        /*
         * if (!mIsSprd) { // sprd cmd need to change //CMD_BT_EUT_OPEN =
         * "eng bt dut_mode_configure 1"; //CMD_BT_EUT_CLOSE =
         * "eng bt dut_mode_configure 0"; //CMD_BT_EUT_STATUS =
         * "eng bt eut_status"; cmd = "eng bt "+cmd; }
         */
        Log.d(TAG, "connect socket is " + socketName + ", cmd is " + cmd);
        res = SocketUtils.sendCmdNoCloseSocket(socketName, LocalSocketAddress.Namespace.ABSTRACT,
                cmd);
        mCmdRes = res;
        return analyRes(msg, res);
    }

    private boolean analyRes(int msg, String result) {
        boolean isSuccess = false;
        Log.d(TAG, "analyRes msg is " + msg + ", result is " + result);
        switch (msg) {
            case MSG_BT_ON:
                if (result != null && result.contains("bt_status=1")) {
                    isSuccess = true;
                }
                break;
            case MSG_BT_OFF:
                if (result != null && result.contains("bt_status=0")) {
                    isSuccess = true;
                }
                break;
            case MSG_GET_EUT_STATUS:
            case MSG_SET_EUT_ON:
            case MSG_SET_EUT_OFF:
                if (result != null && result.contains(SocketUtils.OK)) {
                    isSuccess = true;
                }
                break;
            default:
                break;
        }
        return isSuccess;
    }
}
