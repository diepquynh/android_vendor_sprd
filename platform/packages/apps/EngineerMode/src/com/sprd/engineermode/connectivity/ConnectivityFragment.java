
package com.sprd.engineermode.connectivity;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.TwoStatePreference;
import android.view.LayoutInflater;
import android.view.View;
import android.app.Fragment;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.Toast;
import android.media.AudioManager;
import android.net.LocalSocketAddress;
import android.net.LocalSocket;
import android.net.wifi.WifiManager;
import android.util.Log;
import android.app.AlertDialog;
import android.content.DialogInterface;

import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.SocketUtils;
import com.sprd.engineermode.utils.IATUtils;
import com.sprd.engineermode.EngineerModeActivity;

import android.bluetooth.BluetoothAdapter;
import java.text.NumberFormat;
import android.view.View.OnClickListener;

public class ConnectivityFragment extends PreferenceFragment implements
        Preference.OnPreferenceClickListener {

    private static final String TAG = "ConnectivityFragment";
    private static final String KEY_WIFI_EUT = "wifi eut";
    private static final String KEY_BT = "bt";
    private static final String KEY_FM = "fm";
    private Context mContext = null;
    private ProgressDialog mProgress;
    private String mServiceState = null;
    private Preference mWifiTest;
    private Preference mBtTest;
    private Preference mFmTest;
    private Button mStartService;
    private Button mStopService;
    private LinearLayout mlayout = null;
    boolean isUser = SystemProperties.get("ro.build.type").equalsIgnoreCase("user");
    boolean isMarlin = SystemProperties.get("ro.modem.wcn.enable").equals("1");

    private void checkServiceStatus() {
        final Handler UIHandler = new Handler();
        UIHandler.postDelayed(new Runnable() {
            public void run() {
                mServiceState = SystemProperties.get("init.svc.wcnd_eng");
                Log.d(TAG, "wcnd_eng service state is " + mServiceState);
                if (!mServiceState.equals("running")) {
                    UIHandler.postDelayed(this, 100);
                } else {
                    if (mProgress != null) {
                        mProgress.dismiss();
                    }
                    // when wcnd_eng service has started, we can test wifi/bt/fm
                    mWifiTest.setEnabled(true);
                    mBtTest.setEnabled(true);
                    if (!isUser) {
                        mFmTest.setEnabled(true);
                    }
                    //mSoftwifiChannel.setEnabled(true);
                    mStopService.setEnabled(true);
                    mStartService.setEnabled(false);
                }
            }
        }, 100);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreateView(inflater, container, savedInstanceState);
        View view = inflater.inflate(R.layout.connectivity, container, false);
        // getListView().setItemsCanFocus(true);
        mStartService = (Button) view.findViewById(R.id.start_service);
        mStartService.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                // start service
                SystemProperties.set("ctl.start", "wcnd_eng");
                SystemProperties.set("ctl.start", "enghardwaretest");
                Log.d(TAG, "start wcnd_eng service");
                mProgress = ProgressDialog.show(mContext, "Start wcnd_eng service",
                        "Please wait...", true, true);
                checkServiceStatus();
            }
        });
        mStopService = (Button) view.findViewById(R.id.stop_service);
        mStopService.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                // stop service
                SystemProperties.set("ctl.stop", "wcnd_eng");
                SystemProperties.set("ctl.stop", "enghardwaretest");
                Log.d(TAG, "stop wcnd_eng service");
                // when wcnd_eng service has stopped, we can not test wifi/bt/fm
                mWifiTest.setEnabled(false);
                mBtTest.setEnabled(false);
                mFmTest.setEnabled(false);
                //mSoftwifiChannel.setEnabled(false);
                mStopService.setEnabled(false);
                mStartService.setEnabled(true);
            }
        });
        return view;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_connectivitytab);
        mContext = this.getActivity();
        mWifiTest = (Preference) findPreference(KEY_WIFI_EUT);
        mWifiTest.setOnPreferenceClickListener(this);
        mBtTest = (Preference) findPreference(KEY_BT);
        mBtTest.setOnPreferenceClickListener(this);
        mFmTest = (Preference) findPreference(KEY_FM);
        mFmTest.setOnPreferenceClickListener(this);
        if (isUser) {
            mFmTest.setEnabled(false);
            mFmTest.setSummary(R.string.feature_not_support_by_user_version);
        }
    }

    @Override
    public void onStart() {
        if (!SystemProperties.get("init.svc.wcnd_eng").equals("running")) {
            mStopService.setEnabled(false);
            mWifiTest.setEnabled(false);
            mBtTest.setEnabled(false);
            mFmTest.setEnabled(false);
            //mSoftwifiChannel.setEnabled(false);
        } else {
            mStartService.setEnabled(false);
        }
        super.onStart();
    }

    @Override
    public boolean onPreferenceClick(Preference pref) {
        if (pref.getKey().equals(KEY_WIFI_EUT)) {
            // Tester should close wifi when wifi eut testing in order to
            // getting correct result
            if (!isWifiOn()) {
                AlertDialog alertDialog = new AlertDialog.Builder(this.getActivity())
                        .setTitle(getString(R.string.alert_wifi_test))
                        .setMessage(getString(R.string.alert_wifi))
                        .setCancelable(true)
                        .setPositiveButton(
                                getString(R.string.alertdialog_ok),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        Intent intent = new Intent();
                                        intent.setFlags(Intent.FLAG_ACTIVITY_NO_USER_ACTION);
                                        intent.setClass(
                                                mContext,
                                                com.sprd.engineermode.connectivity.wifi.WifiEUTActivity.class);
                                        startActivity(intent);
                                    }
                                }).create();
                alertDialog.show();
            } else {
                AlertDialog alertDialog = new AlertDialog.Builder(this.getActivity())
                        .setTitle(getString(R.string.alert_wifi_test))
                        .setMessage(getString(R.string.alert_close_wifi))
                        .setPositiveButton(
                                getString(R.string.alertdialog_ok),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                    }
                                }).create();
                alertDialog.show();
            }
        } else if (pref.getKey().equals(KEY_BT)) {
            if (!isBTOn()) {
                AlertDialog alertDialog = new AlertDialog.Builder(this.getActivity())
                        .setTitle(getString(R.string.alert_bt_test))
                        .setMessage(getString(R.string.alert_bt))
                        .setCancelable(true)
                        .setPositiveButton(
                                getString(R.string.alertdialog_ok),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        Intent intent = new Intent();
                                        intent.setFlags(Intent.FLAG_ACTIVITY_NO_USER_ACTION);
                                        intent.setClass(
                                                mContext,
                                                com.sprd.engineermode.connectivity.BT.BTActivity.class);
                                        startActivity(intent);
                                    }
                                }).create();
                alertDialog.show();
            } else {
                AlertDialog alertDialog = new AlertDialog.Builder(this.getActivity())
                        .setTitle(getString(R.string.alert_bt_test))
                        .setMessage(getString(R.string.alert_close_bt))
                        .setPositiveButton(
                                getString(R.string.alertdialog_ok),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                    }
                                }).create();
                alertDialog.show();
            }
        } else if (pref.getKey().equals(KEY_FM)) {
            if (!isFMOn()) {
                AlertDialog alertDialog = new AlertDialog.Builder(this.getActivity())
                        .setTitle(getString(R.string.alert_fm_test))
                        .setMessage(getString(R.string.alert_fm))
                        .setCancelable(true)
                        .setPositiveButton(
                                getString(R.string.alertdialog_ok),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        Intent intent = new Intent();
                                        intent.setFlags(Intent.FLAG_ACTIVITY_NO_USER_ACTION);
                                        intent.setClass(
                                                mContext,
                                                com.sprd.engineermode.connectivity.fm.FMActivity.class);
                                        startActivity(intent);
                                    }
                                }).create();
                alertDialog.show();
            } else {
                AlertDialog alertDialog = new AlertDialog.Builder(this.getActivity())
                        .setTitle(getString(R.string.alert_fm_test))
                        .setMessage(getString(R.string.alert_close_fm))
                        .setPositiveButton(
                                getString(R.string.alertdialog_ok),
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                    }
                                }).create();
                alertDialog.show();
            }
        }
        return true;
    }

    private boolean isWifiOn() {
        WifiManager wifiManager = (WifiManager) this.getActivity().getSystemService(
                Context.WIFI_SERVICE);
        if (wifiManager.isWifiEnabled()) {
            return true;
        }
        return false;
    }

    private boolean isBTOn() {
        boolean isBtOn = false;
        int btState = BluetoothAdapter.getDefaultAdapter().getState();
        Log.d(TAG, "Connectivity BT Status is " + btState);
        if (btState == BluetoothAdapter.STATE_ON) {
            isBtOn = true;
        }
        return isBtOn;
    }

    private boolean isFMOn() {
        // AudioManager audioManager = (AudioManager) this.getActivity().getSystemService(
        // Context.AUDIO_SERVICE);
        // if (audioManager.isFmActive()) {
        // return true;
        // }
        return false;
    }
}
