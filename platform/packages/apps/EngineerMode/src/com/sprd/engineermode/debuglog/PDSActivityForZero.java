
package com.sprd.engineermode.debuglog;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.PrintStream;
import java.util.Arrays;
import android.os.Build;
import android.content.Intent;

import com.sprd.engineermode.R;

import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.SystemProperties;
import android.os.Bundle;
import android.os.Handler;
import android.preference.EditTextPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.TwoStatePreference;
import android.text.InputType;
import android.text.method.NumberKeyListener;
import android.util.Log;
import android.widget.Toast;
import java.io.InputStreamReader;
import android.os.SystemProperties;

public class PDSActivityForZero extends PreferenceActivity implements
        OnPreferenceChangeListener, Preference.OnPreferenceClickListener {

    private static final String TAG = "PDSActivityForZero";

    private static final String PATH_DVFS_AVAILABLE = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors";
    private static final String PATH_DVFS_HOTPLUG_1 = "/sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable";
    private static final String PATH_DVFS_HOTPLUG_2 = "/sys/devices/system/cpu/cpuhotplug/cpu_hotplug_disable";
    private static final String PATH_DVFS_GOVERNOR = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";

    private static final String PERSIST_DVFS_MODE = "persist.sys.dvfs.mode";
    private static final String KEY_SET_FREQ = "set_freq";

    private static final String KEY_DVFS = "dvfs";
    private static final String KEY_CPU_FREQUENCE = "cpuFrequence";

    private TwoStatePreference mDVFSPre;

    private Preference mCPUPre;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_power_dissipation1);

        mDVFSPre = (TwoStatePreference) this.findPreference(KEY_DVFS);
        mCPUPre = (Preference) this.findPreference(KEY_CPU_FREQUENCE);

        mDVFSPre.setOnPreferenceChangeListener(this);
        mCPUPre.setOnPreferenceClickListener(this);
        checkDVFSSupport();
    }

    @Override
    protected void onStart() {
        if (mDVFSPre != null && mDVFSPre.isEnabled()) {
            checkDVFSStatus();
        }
        super.onStart();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (preference == mDVFSPre) {
            if (mDVFSPre.isChecked()) {
                closeDVFS();
            } else {
                openDVFS();
            }
        }
        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference pref) {
        if (pref == mCPUPre) {
            Intent intent = new Intent();
            intent.setClass(this, CpuFreqListActivity.class);
            startActivityForResult(intent, 0);
        }
        return false;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (resultCode) {
            case RESULT_OK:
                if (requestCode == 0) {
                    Log.d(TAG,"set Freq is "+ data.getExtras().getString(KEY_SET_FREQ));
                    mCPUPre.setSummary(data.getExtras().getString(KEY_SET_FREQ));
                }
                break;
            default:
                break;
        }
    }

    private void checkDVFSSupport() {
        String mReadRes = readFile(PATH_DVFS_AVAILABLE);
        if (!"readError".equals(mReadRes) && !"".equals(mReadRes)) {
            if (!mReadRes.contains("userspace")) {
                mDVFSPre.setEnabled(false);
                mDVFSPre.setSummary(R.string.feature_not_support);
                mCPUPre.setEnabled(false);
                mCPUPre.setSummary(R.string.feature_not_support);
            }
        }
    }

    private void checkDVFSStatus() {
        //the DVFS switch set only one time
       String mReadRes = readFile(PATH_DVFS_GOVERNOR);
        if (!"readError".equals(mReadRes) && !"".equals(mReadRes)) {
            if ("interactive".equals(mReadRes)) {
                mDVFSPre.setChecked(true);
                SystemProperties.set(PERSIST_DVFS_MODE,"interactive");
            } else if ("sprdemand".equals(mReadRes)) {
                mDVFSPre.setChecked(true);
                SystemProperties.set(PERSIST_DVFS_MODE,"sprdemand");
            } else if ("userspace".equals(mReadRes)) {
                mDVFSPre.setChecked(false);
                mCPUPre.setEnabled(true);
                initCpuFre();
            }
            return;
        } 
        Log.d(TAG, "*0 chip DVFS Abnormal because read file error");
        mDVFSPre.setEnabled(false);
        mDVFSPre.setSummary(R.string.feature_abnormal);
    }

    protected void initCpuFre() {
        if (mCPUPre != null && mCPUPre.isEnabled()) {
            String cpuFre = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
            if (!"".equals(cpuFre) && !"readError".equals(cpuFre)) {
                mCPUPre.setSummary(Integer.valueOf(cpuFre).intValue() / 1000 + "mHz");
            } else {
                Log.d(TAG, "CPU Abnormal because initCpuFre Fail");
                mCPUPre.setEnabled(false);
                mCPUPre.setSummary(R.string.feature_abnormal);
            }
        }
    }

    private void closeDVFS() {
        Log.d(TAG,"close Dvfs");
        String mDvfsMode = SystemProperties.get(PERSIST_DVFS_MODE,
                "interactive");
        if ("interactive".equals(mDvfsMode)) {
            execShellStr("echo 1 > /sys/devices/system/cpu/cpuhotplug/cpu_hotplug_disable");
        } else if ("sprdemand".equals(mDvfsMode)) {
            execShellStr("echo 1 > /sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable");
        }
        execShellStr("echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        mCPUPre.setEnabled(true);
        initCpuFre();
    }

    private void openDVFS() {
        Log.d(TAG,"open Dvfs");
        String mDvfsMode = SystemProperties.get(PERSIST_DVFS_MODE,
                "interactive");
        if ("interactive".equals(mDvfsMode)) {
            execShellStr("echo 0 > /sys/devices/system/cpu/cpuhotplug/cpu_hotplug_disable");
            execShellStr("echo interactive > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        } else if ("sprdemand".equals(mDvfsMode)) {
            execShellStr("echo 0 > /sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable");
            execShellStr("echo sprdemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        }
        mCPUPre.setEnabled(false);
    }

    // write file
    public boolean writeFile(String str, String path) {
        Log.d(TAG, "str->" + str);
        Log.d(TAG, "path->" + path);
        boolean flag = true;
        FileOutputStream out = null;
        PrintStream p = null;
        try {
            out = new FileOutputStream(path);
            p = new PrintStream(out);
            p.print(str);
        } catch (Exception e) {
            flag = false;
            Log.d(TAG, "Write file error!!!");
            e.printStackTrace();
        } finally {
            if (out != null) {
                try {
                    out.close();
                } catch (Exception e2) {
                    e2.printStackTrace();
                }
            }
            if (p != null) {
                try {
                    p.close();
                } catch (Exception e2) {
                    e2.printStackTrace();
                }
            }
        }
        return flag;
    }

    // read file
    public String readFile(String path) {
        File file = new File(path);
        String str = new String("");
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new FileReader(file));
            String line = null;
            while ((line = reader.readLine()) != null) {
                str = str + line;
            }
        } catch (Exception e) {
            Log.d(TAG, "Read file error!!!");
            str = "readError";
            e.printStackTrace();
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (Exception e2) {
                    e2.printStackTrace();
                }
            }
        }
        Log.d(TAG, "read " + path + " value is " + str.trim());
        return str.trim();
    }

    // exec shell cmd
    private String execShellStr(String cmd) {
        String[] cmdStrings = new String[] {
                "sh", "-c", cmd
        };
        StringBuffer retString = new StringBuffer("");

        try {
            Process process = Runtime.getRuntime().exec(cmdStrings);
            BufferedReader stdout = new BufferedReader(new InputStreamReader(
                    process.getInputStream(), "UTF-8"), 7777);
            BufferedReader stderr = new BufferedReader(new InputStreamReader(
                    process.getErrorStream(), "UTF-8"), 7777);

            String line = null;

            while ((null != (line = stdout.readLine()))
                    || (null != (line = stderr.readLine()))) {
                if ("" != line) {
                    retString = retString.append(line).append("\n");
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.d(TAG, cmd + ":" + retString.toString() + "");
        return retString.toString();
    }
}
