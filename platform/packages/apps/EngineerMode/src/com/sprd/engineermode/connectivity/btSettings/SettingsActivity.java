package com.sprd.engineermode.connectivity.btSettings;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.InputStream;
import java.net.Socket;
import java.net.UnknownHostException;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketAddress;
import java.net.SocketTimeoutException;
import java.net.InetSocketAddress;

import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.preference.PreferenceScreen;
import android.preference.TwoStatePreference;
import android.util.Log;
import android.widget.Toast;
import com.sprd.engineermode.debuglog.slogui.SlogAction;

import android.content.Intent;
import android.content.Context;
import com.sprd.engineermode.R;

import android.bluetooth.BluetoothAdapter;

import android.net.LocalSocketAddress;

public class SettingsActivity extends PreferenceActivity implements
         Preference.OnPreferenceChangeListener, Preference.OnPreferenceClickListener{

    private static final String TAG = "BTSettingsActivity";
    private static final String KEY_CONTROLLER_BQB = "controller_bqb_mode";

    private static final String LOCAL_ADDRESS = "127.0.0.1";
    private static final String PROPERTY__HCITOOLS_SOCKET = "bluetooth.hcitools.socket";
    private static final String PROPERTY__HCITOOLS_SERVER = "bluetooth.hcitools.server";

    private static final String CONTROLLER_BQB_SOCKET = "/data/misc/.bqb_ctrl";
    private static final String COMM_CONTROLLER_ENABLE = "\r\nAT+SPBQBTEST=1\r\n";
    private static final String COMM_CONTROLLER_DISABLE = "\r\nAT+SPBQBTEST=0\r\n";
    private static final String COMM_CONTROLLER_TRIGGER = "\r\nAT+SPBQBTEST=?\r\n";

    private static final String NOTIFY_BQB_ENABLE = "\r\n+SPBQBTEST OK: ENABLED\r\n";
    private static final String NOTIFY_BQB_DISABLE = "\r\n+SPBQBTEST OK: DISABLE\r\n";
    private static final String TRIGGER_BQB_ENABLE = "\r\n+SPBQBTEST OK: BQB\r\n";
    private static final String TRIGGER_BQB_DISABLE = "\r\n+SPBQBTEST OK: AT\r\n";
    private static final String CONTROLLER_BQB_ENABLED = "controller bqb enabled";
    private static final String CONTROLLER_BQB_DISABLED = "controller bqb disabled";

    private static final int MESSAGE_CONTROLLER_BQB_ONOFF = 3;
    private static final int MESSAGE_CONTROLLER_BQB_TRIGGER = 4;

    private static int socket_port = 0;
    private LocalSocket mConBqbSocket = null;
    private InputStream mConBqbInputStream = null;
    private OutputStream mConBqbOutputStream = null;

    private Handler mUiThread = new Handler();
    private BTSettingsHandler mBTSettingsHandler;

    private SharedPreferences mSharePref;
    private Preference mControllerBqb;

    private Context mContext;

    private BluetoothAdapter mAdapter;

    private Socket client = null;
    private InputStream in = null;
    private OutputStream out = null;

    private Object sync = new Object();

    private boolean mControllerBqbState = false;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_bt_settings);

        mControllerBqb = (Preference) this.findPreference(KEY_CONTROLLER_BQB);
        mControllerBqb.setOnPreferenceClickListener(this);

        mContext = this;

        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mBTSettingsHandler = new BTSettingsHandler(ht.getLooper());

        mAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    @Override
    public void onStart() {
        super.onStart();

        if (mAdapter.isEnabled()) {
            mControllerBqb.setEnabled(false);
            Log.d(TAG, "read DSP LOG state");
            if (SystemProperties.get(PROPERTY__HCITOOLS_SERVER, "stopped").equals("stopped")) {
                Log.e(TAG, "HCI SERVER did not start");
            } else {
                socket_port =  Integer.parseInt(SystemProperties.get(PROPERTY__HCITOOLS_SOCKET, "0"));
                Log.d(TAG, PROPERTY__HCITOOLS_SOCKET + ": " + socket_port);
                if (socket_port == 0) {
                    Log.e(TAG, "unknow socket");
                }
            }
        } else {
            mControllerBqb.setEnabled(true);
            Message msg = mBTSettingsHandler.obtainMessage(MESSAGE_CONTROLLER_BQB_TRIGGER);
            mBTSettingsHandler.sendMessage(msg);

           //uiLog("please enable bt!");
           //finish();
        }
    }

    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
        super.onDestroy();
        try {
            synchronized(sync) {
                if (in != null) {
                    in.close();
                    in = null;
                }
                if (out != null) {
                    out.close();
                    out = null;
                }
                if (client != null) {
                    client.close();
                    client = null;
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "error: " + e);
        }
        closeConBqb();
        finish();
    }

    @Override
    public void onBackPressed() {
        // TODO Auto-generated method stub
        super.onBackPressed();
        try {
            synchronized(sync) {
                if (in != null) {
                    in.close();
                    in = null;
                }
                if (out != null) {
                    out.close();
                    out = null;
                }
                if (client != null) {
                    client.close();
                    client = null;
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "error: " + e);
        }
        closeConBqb();
        finish();
    }

    @Override
    public boolean onPreferenceChange(Preference pref, Object objValue) {
        return true;
    }

    class BTSettingsHandler extends Handler {

        public BTSettingsHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_CONTROLLER_BQB_ONOFF:
                controllerBqbEnable(!mControllerBqbState);
                break;
            case MESSAGE_CONTROLLER_BQB_TRIGGER:
                mControllerBqbState = getControllerBqbState();
            default:
                break;
            }
        }
    }

    @Override
    public boolean onPreferenceClick(Preference pref) {
        if (pref.getKey().equals(KEY_CONTROLLER_BQB)) {
            Log.d(TAG, "Controller BQB State Changed " + mControllerBqbState + " -> " + !mControllerBqbState);
            Message msg = mBTSettingsHandler.obtainMessage(MESSAGE_CONTROLLER_BQB_ONOFF);
            mBTSettingsHandler.sendMessage(msg);
        }
        return true;
    }

    private void uiLog(String log) {
        final String stringLog = log;
        mUiThread.post(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(mContext, stringLog,
                        Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void controllerBqbEnable(boolean enable){
        Log.d(TAG, "controllerBqbEnable: " + enable);
        try {
        Log.d(TAG, "init bqb local socket");
            LocalSocketAddress address = new LocalSocketAddress(CONTROLLER_BQB_SOCKET,
                LocalSocketAddress.Namespace.ABSTRACT);
            mConBqbSocket = new LocalSocket();
            mConBqbSocket.connect(address);
            mConBqbOutputStream = mConBqbSocket.getOutputStream();
            mConBqbInputStream = mConBqbSocket.getInputStream();

            String commandString = enable ? COMM_CONTROLLER_ENABLE : COMM_CONTROLLER_DISABLE;
            Log.d(TAG, "write: " + commandString);
            mConBqbOutputStream.write(commandString.getBytes());
            byte[] buffer = new byte[128];
            Log.d(TAG, "wait bqb response");
            int len = mConBqbInputStream.read(buffer);
            String res = new String(buffer, 0, len);
            Log.d(TAG, "Response: " + res);
            if (-1 != res.indexOf(NOTIFY_BQB_ENABLE)) {
                mControllerBqbState = true;
                mUiThread.post(new Runnable() {
                    @Override
                    public void run() {
                        Log.d(TAG, "CONTROLLER BQB ENABLE");
                        mControllerBqb.setSummary(CONTROLLER_BQB_ENABLED);
                    }
                });
            } else if (-1 != res.indexOf(NOTIFY_BQB_DISABLE)) {
                mControllerBqbState = false;
                mUiThread.post(new Runnable() {
                    @Override
                    public void run() {
                        Log.d(TAG, "CONTROLLER BQB DISABLE");
                        mControllerBqb.setSummary(CONTROLLER_BQB_DISABLED);
                    }
                });
            }

        } catch (IOException ex) {
            Log.e(TAG, "Communication error :", ex);
        } finally {
            closeConBqb();
        }
    }

    private void closeConBqb(){
        synchronized(sync) {
            try {
                if (mConBqbOutputStream != null) {
                    mConBqbOutputStream.close();
                    mConBqbOutputStream = null;
                }
                if (mConBqbInputStream != null) {
                    mConBqbInputStream.close();
                    mConBqbInputStream = null;
                }
                if (mConBqbSocket != null) {
                    mConBqbSocket.close();
                    mConBqbSocket = null;
                }
            } catch (IOException ex) {
                Log.e(TAG, "close error :", ex);
            }
        }
    }

    private boolean getControllerBqbState(){
        InputStream mInputStream = null;
        OutputStream mOutputStream = null;
        try {
        Log.d(TAG, "init bqb local socket");
            LocalSocketAddress address = new LocalSocketAddress(CONTROLLER_BQB_SOCKET,
                LocalSocketAddress.Namespace.ABSTRACT);
            mConBqbSocket = new LocalSocket();
            mConBqbSocket.connect(address);
            mOutputStream = mConBqbSocket.getOutputStream();
            mInputStream = mConBqbSocket.getInputStream();
            Log.d(TAG, "write bqb command: " + COMM_CONTROLLER_TRIGGER);
            mOutputStream.write(COMM_CONTROLLER_TRIGGER.getBytes());
            byte[] buffer = new byte[128];
            Log.d(TAG, "waite bqb response");
            int len = mInputStream.read(buffer);
            String res = new String(buffer, 0, len);
            Log.d(TAG, "Response: " + res);
            if (-1 != res.indexOf(TRIGGER_BQB_ENABLE)) {
                mUiThread.post(new Runnable() {
                    @Override
                    public void run() {
                        Log.d(TAG, "CONTROLLER BQB ENABLE");
                        mControllerBqb.setSummary(CONTROLLER_BQB_ENABLED);
                    }
                });
                return true;
            } else if (-1 != res.indexOf(TRIGGER_BQB_DISABLE)) {
                mUiThread.post(new Runnable() {
                    @Override
                    public void run() {
                        Log.d(TAG, "CONTROLLER BQB DISABLE");
                        mControllerBqb.setSummary(CONTROLLER_BQB_DISABLED);
                    }
                });
                return false;
            }
        } catch (IOException ex) {
            Log.e(TAG, "Communication error :", ex);
        } finally {
            try {
                if (mOutputStream != null) {
                    mOutputStream.close();
                    mOutputStream = null;
                }
                if (mInputStream != null) {
                    mInputStream.close();
                    mInputStream = null;
                }
                if (mConBqbSocket != null) {
                    mConBqbSocket.close();
                    mConBqbSocket = null;
                }
            } catch (IOException ex) {
                Log.e(TAG, "close error :", ex);
            }
        }
        return false;
    }

}
