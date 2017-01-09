
package com.sprd.engineermode.debuglog;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.List;

import com.sprd.engineermode.EMSwitchPreference;
import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemProperties;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.Preference.OnPreferenceChangeListener;
import android.util.Log;
import android.widget.Toast;
import android.preference.TwoStatePreference;

import android.os.SystemProperties;

public class SystemSettingActivity extends PreferenceActivity implements OnPreferenceChangeListener {

    private static final String TAG = "SystemSettingActivity";

    private static final String KEY_SYSTEM_SETTINGS = "system_settings";
    private static final String KEY_CORE_FILE = "core_file";

    private static final String PERSIST_CORE_FILE = "persist.sys.corefile.enable";

    private TwoStatePreference mCoreFile;

    private Handler uiThread = new Handler();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG,"onCreate...");
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_system_settings);
        mCoreFile = (TwoStatePreference) findPreference(KEY_CORE_FILE);
        mCoreFile.setOnPreferenceChangeListener(this);
    }
    
    @Override
    public void onResume() {
        super.onResume();
        if (mCoreFile != null) {
            checkCoreFile();
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object keyValue) {
        if (preference == mCoreFile) {
            if (!mCoreFile.isChecked()) {
                setCoreFile(true);
                checkCoreFile();
            } else {
                setCoreFile(false);
                checkCoreFile();
            }
            return true;
        }
        return false;
    }

    /**
     * open need to write /data/corefile/core-%e-%p to /proc/sys/kernel/core_pattern
     * colse need to write /dev/null to /proc/sys/kernel/core_pattern
     **/
    public void setCoreFile(boolean enable) {
        Log.d(TAG, "setCoreFile: " + enable);
        if (enable) {
            SystemProperties.set(PERSIST_CORE_FILE, "1");
        } else {
            SystemProperties.set(PERSIST_CORE_FILE, "0");
        }
    }

    public void checkCoreFile() {
        Log.d(TAG, "checkCoreFile...");
        final boolean isEnable = SystemProperties.getBoolean(PERSIST_CORE_FILE, false);
        uiThread.post(new Runnable() {
            @Override
            public void run() {
                mCoreFile.setChecked(isEnable);
            }
        });
    }
}
