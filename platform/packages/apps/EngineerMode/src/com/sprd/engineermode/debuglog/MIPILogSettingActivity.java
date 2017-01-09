
package com.sprd.engineermode.debuglog;

/** BUG547014 zhijie.yang 2016/05/09 SPRD:add mipi log function **/
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.preference.PreferenceActivity;
import android.preference.Preference.OnPreferenceChangeListener;
import android.util.Log;
import android.widget.Toast;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.content.Context;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.SystemProperties;

import com.sprd.engineermode.utils.ShellUtils;
import java.io.File;
import com.sprd.engineermode.R;

public class MIPILogSettingActivity extends PreferenceActivity implements
        OnPreferenceChangeListener {

    private static final String TAG = "MIPILogSettingActivity";
    private static final String MIPI_LOG_FILE_PATH = "/sys/devices/soc/soc:mm/63a00000.sprd-mipi-log/channel";
    private static final String PROPERTIES_MIPI_CHANNEL = "persist.sys.mipi.channel";
    private static final String KEY_MIPI = "mipi_log";

    private static final String CHANNEL_CLOSE = "0";
    private static final String CHANNEL_TRANNING = "1";
    private static final String CHANNEL_WTL = "2";

    private ListPreference mListMIPILog;
    private SharedPreferences mSharePref;
    private Handler mUiThread = new Handler();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_mipi_log);
        mListMIPILog = (ListPreference) findPreference(KEY_MIPI);
        mSharePref = PreferenceManager.getDefaultSharedPreferences(this);
        mListMIPILog.setOnPreferenceChangeListener(this);
    }

    @Override
    public void onStart() {
        Log.d(TAG, "onStart...");
        super.onStart();
        if (mListMIPILog != null) {
            if (isSupport()) {
                String value = SystemProperties.get(PROPERTIES_MIPI_CHANNEL, CHANNEL_CLOSE);
                mListMIPILog.setValueIndex(Integer.parseInt(value.trim()));
                mListMIPILog.setSummary(mListMIPILog.getEntry());
            } else {
                mListMIPILog.setEnabled(false);
                mListMIPILog.setSummary(R.string.feature_not_support);
            }
        }
    }

    private boolean isSupport() {
        boolean isSupport = false;
        boolean isUser = SystemProperties.get("ro.build.type").equalsIgnoreCase("user");
        boolean isExist = isFileExist();
        if (!isUser && isExist) {
            isSupport = true;
        }
        Log.d(TAG, "isUser: " + isUser + "isExist: " + isExist + "isSupport: " + isSupport);
        return isSupport;
    }

    private boolean isFileExist() {
        boolean isExist = false;
        File file = new File(MIPI_LOG_FILE_PATH);
        if (file.exists()) {
            isExist = true;
        }
        return isExist;
    }

    private boolean writeToFile(String str) {
        Log.d(TAG, "writeToFile: " + str);
        if (ShellUtils.writeToFile(MIPI_LOG_FILE_PATH, str)) {
            Log.d(TAG, "write success");
            return true;
        } else {
            Log.d(TAG, "write fail");
            return false;
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        final String setValue = newValue.toString();
        Log.d(TAG, "setValueIndex: " + setValue);
        new Thread(new Runnable() {
            public void run() {
                if (writeToFile(setValue)) {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            SystemProperties.set(PROPERTIES_MIPI_CHANNEL, setValue);
                            mListMIPILog.setValueIndex(Integer.parseInt(setValue.trim()));
                            mListMIPILog.setSummary(mListMIPILog.getEntry());
                        }
                    });
                } else {
                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            mListMIPILog.setValueIndex(Integer.parseInt(SystemProperties.get(
                                    PROPERTIES_MIPI_CHANNEL, CHANNEL_CLOSE)));
                            mListMIPILog.setSummary(mListMIPILog.getEntry());
                            Toast.makeText(MIPILogSettingActivity.this, "set fail!",
                                    Toast.LENGTH_SHORT).show();
                        }
                    });
                }
            }
        }).start();

        return true;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }
}
