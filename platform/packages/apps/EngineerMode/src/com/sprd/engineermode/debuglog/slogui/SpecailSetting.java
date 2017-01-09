package com.sprd.engineermode.debuglog.slogui;

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.PowerManager;
import android.content.Context;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
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
import android.preference.PreferenceGroup;
import android.preference.TwoStatePreference;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.widget.Toast;

import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

import android.view.ContextMenu;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RadioButton;
import android.widget.TextView;
import android.widget.Toast;
import java.util.ArrayList;
import java.util.HashMap;

import com.sprd.engineermode.EMPreference;

public class SpecailSetting extends PreferenceActivity implements
        Preference.OnPreferenceChangeListener {

    private static final String TAG = "SpecailSetting";
    private static final String LOG_SPECIAL_STATUS = "special_status";
    private static final String LOG_SCENARIOS_STATUS = "scenarios_status";

    private static final String LOG_W_L1_CMD = "1,2,"
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\""
            + ","
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\"";
    private static final String LOG_W_L2_CMD = "1,2,"
            + "\""
            + "FFFFFF701FFFFFFFFFFFFFFFFFFFFFFF"
            + "\""
            + ","
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFD"
            + "\"";
    private static final String LOG_W_SLEEP_CMD = "1,1,"
            + "\""
            + "FFFFFF701FFFFFFFFFFFFFFFFFFFFFFF"
            + "\""
            + ","
            + "\""
            + "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
            + "\"";

    private static final int SET_LOG_SPECIAL_STATUS = 0;
    public static final int DEFAULT_SPECIAL_W_L1_ID = 1;
    public static final int DEFAULT_SPECIAL_W_L2_ID = 2;
    public static final int DEFAULT_SPECIAL_W_SLEEP_ID = 3;

    private static int mSpecialStatus = 0;
    private String mServerName = "atchannel0";
    private SharedPreferences pref;
    private Context mContext;
    private static HashMap<Integer, EMPreference> mAllProfilesMap;
    PreferenceGroup mPreGroup = null;

    private SpecialSettingHandler mSpecialSettingHandler;
    private Handler mUiThread = new Handler();

    @Override
    protected void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        setPreferenceScreen(getPreferenceManager().createPreferenceScreen(this));
        pref = PreferenceManager.getDefaultSharedPreferences(this);
        mPreGroup = getPreferenceScreen();
        mContext = this;
        HandlerThread ht = new HandlerThread(TAG);
        ht.start();
        mSpecialSettingHandler = new SpecialSettingHandler(ht.getLooper());
        mAllProfilesMap = new HashMap<Integer, EMPreference>();
        EMPreference wL1 = new EMPreference(this);
        wL1.setId(DEFAULT_SPECIAL_W_L1_ID);
        wL1.setKey("w_l1");
        wL1.setTitle(R.string.special_w_l1);
        wL1.setPersistent(false);
        wL1.setOnPreferenceChangeListener(this);

        EMPreference wL2 = new EMPreference(this);
        wL2.setId(DEFAULT_SPECIAL_W_L2_ID);
        wL2.setKey("w_l2");
        wL2.setTitle(R.string.special_w_l2);
        wL2.setPersistent(false);
        wL2.setOnPreferenceChangeListener(this);

        EMPreference wSleep = new EMPreference(this);
        wSleep.setId(DEFAULT_SPECIAL_W_SLEEP_ID);
        wSleep.setKey("w_sleep");
        wSleep.setTitle(R.string.special_w_sleep);
        wSleep.setPersistent(false);
        wSleep.setOnPreferenceChangeListener(this);

        mPreGroup.addPreference(wL1);
        mPreGroup.addPreference(wL2);
        mPreGroup.addPreference(wSleep);

        mAllProfilesMap.put(wL1.getId(), wL1);
        mAllProfilesMap.put(wL2.getId(), wL2);
        mAllProfilesMap.put(wSleep.getId(), wSleep);

    }

    @Override
    protected void onResume() {
        super.onResume();
        mSpecialStatus = pref.getInt(LOG_SPECIAL_STATUS, 0);
        updateSelectedState();
    }

    @Override
    protected void onDestroy() {
        if (mAllProfilesMap != null) {
            mAllProfilesMap.clear();
            mAllProfilesMap = null;
        }
        if (mSpecialSettingHandler != null) {
            mSpecialSettingHandler.getLooper().quit();
            mSpecialSettingHandler = null;
        }
        super.onDestroy();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        // update to system
        if (newValue instanceof Integer) {
            int profileId = ((Integer) newValue).intValue();
            Log.d(TAG, "PreferenceChangedId = " + profileId);
            Message setSpecialStatus = mSpecialSettingHandler.obtainMessage(
                    SET_LOG_SPECIAL_STATUS, profileId);
            mSpecialSettingHandler.sendMessage(setSpecialStatus);
        }
        return true;
    }

    private static void updateSelectedState() {
        if (mAllProfilesMap == null) {
            return;
        }
        for (HashMap.Entry<Integer, EMPreference> entry : mAllProfilesMap
                .entrySet()) {
            if (mSpecialStatus != entry.getKey()) {
                ((EMPreference) entry.getValue()).setChecked(false);
            } else {
                ((EMPreference) entry.getValue()).setChecked(true);
            }
        }
    }

    String changeIndexToCmd(int value) {
        String cmd = null;
        switch (value) {
        case 1:
            cmd = engconstents.ENG_SET_LOG_LEVEL + LOG_W_L1_CMD;
            break;
        case 2:
            cmd = engconstents.ENG_SET_LOG_LEVEL + LOG_W_L2_CMD;
            break;
        case 3:
            cmd = engconstents.ENG_SET_LOG_LEVEL + LOG_W_SLEEP_CMD;
            break;
        default:
            break;
        }
        Log.d(TAG, "changeIndex is: " + value + "cmd is: " + cmd);
        return cmd;
    }

    class SpecialSettingHandler extends Handler {
        public SpecialSettingHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String atResponse = null;
            String atCmd = null;
            switch (msg.what) {
            case SET_LOG_SPECIAL_STATUS:
                int value = (Integer) msg.obj;
                atCmd = changeIndexToCmd(value);
                atResponse = IATUtils.sendATCmd(atCmd, mServerName);
                if (atResponse != null && atResponse.contains(IATUtils.AT_OK)) {
                    mSpecialStatus = value;
                    pref.edit().putInt(LOG_SPECIAL_STATUS, value)
                            .putInt(LOG_SCENARIOS_STATUS, 2).commit();
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG, "Set log level success");
                            updateSelectedState();
                            Toast.makeText(SpecailSetting.this,
                                    "Set Success", Toast.LENGTH_SHORT).show();
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            Log.d(TAG, "Set log level fail");
                            updateSelectedState();
                            Toast.makeText(SpecailSetting.this,
                                    "Set Fail", Toast.LENGTH_SHORT).show();
                        }
                    });
                }
                break;
            default:
                break;
            }
        }
    }
}


