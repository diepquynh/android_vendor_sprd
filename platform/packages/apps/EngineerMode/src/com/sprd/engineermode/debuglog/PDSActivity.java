
package com.sprd.engineermode.debuglog;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.PrintStream;
import java.util.Arrays;
import android.os.Build;

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

public class PDSActivity extends PreferenceActivity implements
        OnPreferenceChangeListener {

    private static final String TAG = "PDSActivity";

    private static final String PATH_DFS_1 = "/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_freq";
    private static final String PATH_DFS_2 = "/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_enable";
    private static final String PATH_DVFS_1 = "/sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable";
    private static final String PATH_DVFS_2 = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";

    private static final String PERSIST_KEY_CPU_FREQUENCE = "persist.sys.cpuFrequence";
    private static final String PERSIST_KEY_VDDARM = "persist.sys.vddarm";

    private static final String KEY_DFS = "dfs";
    private static final String KEY_DVFS = "dvfs";
    private static final String KEY_CPU_FREQUENCE = "cpuFrequence";
    private static final String KEY_VDDARM = "vddarm";

    private TwoStatePreference mDFSPre, mDVFSPre;
    private EditTextPreference mCPUPre;
    private EditTextPreference mVDDARMPre;

    private String mVersion = null;
    private String mCPUFre = null;
    private String mVDDARM = null;
    private boolean mIsFifteenChip = false;
    private boolean mIsZeroChip = false;

    // ReadFileRes
    private String mReadRes = null;
    private String mReadRes1 = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_power_dissipation);

        //mVersion = SystemProperties.get("ro.build.display.id", "unknown");
        mVersion = Build.MODEL;
        Log.d(TAG, "mVersion-->" + mVersion);

        if (mVersion.contains("7715") || mVersion.contains("8815") || mVersion.contains("6815")) {
            mIsFifteenChip = true;
            Log.d(TAG, "mIsFifteenChip is true");
        } else if (mVersion.contains("7730") || mVersion.contains("8830")
                || mVersion.contains("9620") || mVersion.contains("7731")) {
            mIsZeroChip = true;
            Log.d(TAG, "mIsZeroChip is true");
        }

        mDFSPre = (TwoStatePreference) this.findPreference(KEY_DFS);
        mDVFSPre = (TwoStatePreference) this.findPreference(KEY_DVFS);
        mCPUPre = (EditTextPreference) this.findPreference(KEY_CPU_FREQUENCE);
        mVDDARMPre = (EditTextPreference) this.findPreference(KEY_VDDARM);

        mDFSPre.setOnPreferenceChangeListener(this);
        mDVFSPre.setOnPreferenceChangeListener(this);
        mCPUPre.setOnPreferenceChangeListener(this);
        mVDDARMPre.setOnPreferenceChangeListener(this);

        if (!mIsFifteenChip && !mIsZeroChip && !mVersion.contains("9630")) {
            mDFSPre.setEnabled(false);
            mDFSPre.setSummary(R.string.feature_not_support);
            mDVFSPre.setEnabled(false);
            mDVFSPre.setSummary(R.string.feature_not_support);
        }
        if (!mIsFifteenChip) {
            mCPUPre.setEnabled(false);
            mCPUPre.setSummary(R.string.feature_not_support);
            mVDDARMPre.setEnabled(false);
            mVDDARMPre.setSummary(R.string.feature_not_support);
        }
    }

    @Override
    protected void onStart() {
        if (mDFSPre != null && mDFSPre.isEnabled()) {
            mReadRes = readFile(PATH_DFS_1);
            mReadRes1 = readFile(PATH_DFS_2);
            if (!"readError".equals(mReadRes) && !"readError".equals(mReadRes1)
                    && !"".equals(mReadRes) && !"".equals(mReadRes1)) {
                if ("0".equals(mReadRes.substring(0, 1))
                        && "1".equals(mReadRes1)) {
                    mDFSPre.setChecked(true);
                } else if ("533000".equals(mReadRes.substring(0, 6))
                        && "0".equals(mReadRes1)) {
                    mDFSPre.setChecked(false);
                } else {
                    Log.d(TAG, "DFS Abnormal because file value wrong");
                    mDFSPre.setEnabled(false);
                    mDFSPre.setSummary(R.string.feature_abnormal);
                }
            } else {
                Log.d(TAG, "DFS Abnormal because read file error");
                mDFSPre.setEnabled(false);
                mDFSPre.setSummary(R.string.feature_abnormal);
            }
        }
        if (mDVFSPre != null && mDVFSPre.isEnabled()) {
            checkDVFSStatus();
        }
        super.onStart();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (preference == mDFSPre) {
            if (mDFSPre.isChecked()) {
                execShellStr("echo 533000 > /sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_freq");
                execShellStr("echo 0 > /sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_enable");
                mReadRes = readFile("/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_freq");
                mReadRes1 = readFile("/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_enable");
                Log.d(TAG, "DFS setting close res dfs1 ->" + mReadRes.substring(0, 6) + ", dfs2 ->"
                        + mReadRes1);
                if (mReadRes != null && "533000".equals(mReadRes.substring(0, 6))
                        && mReadRes1 != null && "0".equals(mReadRes1)) {
                    SystemProperties.set("persist.sys.dfs1", "533000");
                    SystemProperties.set("persist.sys.dfs2", "0");
                    mDFSPre.setChecked(false);
                } else {
                    mDFSPre.setChecked(true);
                }
            } else {
                execShellStr("echo 0 > /sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_freq");
                execShellStr("echo 1 > /sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_enable");
                mReadRes = readFile("/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_freq");
                mReadRes1 = readFile("/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_enable");
                Log.d(TAG, "DFS setting open res dfs1 ->" + mReadRes.substring(0, 1) + ",dfs2 ->"
                        + mReadRes1);
                if (mReadRes != null && "0".equals(mReadRes.substring(0, 1))
                        && mReadRes1 != null && "1".equals(mReadRes1)) {
                    SystemProperties.set("persist.sys.dfs1", "0");
                    SystemProperties.set("persist.sys.dfs2", "1");
                    mDFSPre.setChecked(true);
                } else {
                    mDFSPre.setChecked(false);
                }
            }
        }
        if (preference == mDVFSPre) {
            if (mIsFifteenChip) {
                if (mDVFSPre.isChecked()) {
                    execShellStr("echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                    mReadRes = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                    Log.d(TAG, "15 chip DVFS setting close res dvfs ->" + mReadRes);
                    if (mReadRes != null && "performance".equals(mReadRes)) {
                        SystemProperties.set("persist.sys.dvfs1", "performance");
                        mDVFSPre.setChecked(false);
                        mCPUPre.setEnabled(true);
                        mVDDARMPre.setEnabled(true);
                        closeDvfs();
                        initCpuFre();
                        initVddarm();
                    } else {
                        mDVFSPre.setChecked(true);
                    }
                } else {
                    execShellStr("echo sprdemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                    mReadRes = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                    Log.d(TAG, "15 chip DVFS setting open res dvfs ->" + mReadRes);
                    if (mReadRes != null && "sprdemand".equals(mReadRes)) {
                        SystemProperties.set("persist.sys.dvfs1", "sprdemand");
                        mDVFSPre.setChecked(true);
                        mCPUPre.setEnabled(false);
                        mVDDARMPre.setEnabled(false);
                    } else {
                        mDVFSPre.setChecked(false);
                    }
                }
            } else if (mIsZeroChip) {
                if (mDVFSPre.isChecked()) {
                    execShellStr("echo 1 > /sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable");
                    mReadRes = readFile("/sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable");
                    execShellStr("echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                    mReadRes1 = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                    Log.d(TAG, "*0 chip DVFS setting close res dvfs1 ->" + mReadRes + ", dvfs2 ->"
                            + mReadRes1);
                    if ("1".equals(mReadRes)
                            && "performance".equals(mReadRes1)) {
                        SystemProperties.set("persist.sys.dvfs1", "performance");
                        SystemProperties.set("persist.sys.dvfs2", "1");
                        mDVFSPre.setChecked(false);
                    } else {
                        mDVFSPre.setChecked(true);
                    }
                } else {
                    execShellStr("echo 0 > /sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable");
                    mReadRes = readFile("/sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable");
                    execShellStr("echo sprdemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                    mReadRes1 = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                    Log.d(TAG, "*0 chip DVFS setting open res dvfs1 ->" + mReadRes + ", dvfs2 ->"
                            + mReadRes1);
                    if ("0".equals(mReadRes)
                            && "sprdemand".equals(mReadRes1)) {
                        SystemProperties.set("persist.sys.dvfs1", "sprdemand");
                        SystemProperties.set("persist.sys.dvfs2", "0");
                        mDVFSPre.setChecked(true);
                    } else {
                        mDVFSPre.setChecked(false);
                    }
                }
            } else if (mVersion.contains("9630")) {
                if (mDVFSPre.isChecked()) {
                    execShellStr("echo 1 > /sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable");
                    execShellStr("echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                    mReadRes = readFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                    Log.d(TAG, "9630 DVFS setting close res dvfs1 ->" + mReadRes);
                    if ("performance".equals(mReadRes)) {
                        SystemProperties.set("persist.sys.dvfs1", "performance");
                        SystemProperties.set("persist.sys.dvfs2", "1");
                        mDVFSPre.setChecked(false);
                    } else {
                        mDVFSPre.setChecked(true);
                    }
                } else {
                    execShellStr("echo sprdemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
                    execShellStr("echo sprdemand > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor");
                    execShellStr("echo sprdemand > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor");
                    execShellStr("echo sprdemand > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor");
                    execShellStr("echo 0 > /sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable");
                    mReadRes = readFile("/sys/devices/system/cpu/cpufreq/sprdemand/cpu_hotplug_disable");
                    Log.d(TAG, "9630 DVFS setting open res dvfs1 ->" + mReadRes);
                    if ("0".equals(mReadRes)) {
                        SystemProperties.set("persist.sys.dvfs1", "sprdemand");
                        SystemProperties.set("persist.sys.dvfs2", "0");
                        mDVFSPre.setChecked(true);
                    } else if ("1".equals(mReadRes)) {
                        mDVFSPre.setChecked(false);
                    }
                }
            }
        }
        if (preference == mCPUPre) {
            final String cpuVal = mCPUPre.getEditText().getText().toString();
            Log.d(TAG, "cpuVal->" + cpuVal.trim());
            if (!"".equals(cpuVal.trim())
                    && (cpuVal.trim().contains("1200") || cpuVal.trim().contains("1000")
                            || cpuVal.trim().contains("768") || cpuVal.trim().contains("600"))) {
                Log.d("Testcpu", new StringBuffer(cpuVal.trim()).append("000").toString());
                boolean flag = writeFile(new StringBuffer(cpuVal.trim()).append("000").toString(),
                        "/sys/power/cpufreq_frequency");
                if (flag) {
                    final String cpuCurr = readFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq");
                    Log.d(TAG, "cpuCurr->" + cpuCurr);
                    if (!"".equals(cpuCurr) && !"readError".equals(cpuCurr)) {
                        mCPUPre.setSummary(Integer.valueOf(cpuCurr).intValue() / 1000
                                + "mHz");
                        SystemProperties.set(PERSIST_KEY_CPU_FREQUENCE, cpuVal.trim()
                                + "000");
                    } else {
                        Toast.makeText(PDSActivity.this, "Read Fail", Toast.LENGTH_SHORT).show();
                        return false;
                    }
                } else {
                    Toast.makeText(PDSActivity.this, "Write Fail", Toast.LENGTH_SHORT).show();
                    return false;
                }
            } else {
                Toast.makeText(PDSActivity.this, "number must be 600,768,1000,1200",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }
        if (preference == mVDDARMPre) {
            final String vddarmVal = mVDDARMPre.getEditText().getText().toString();
            Log.d(TAG, "vddarmVal->" + vddarmVal.trim());
            if (!"".equals(vddarmVal.trim())
                    && (vddarmVal.trim().contains("1300") || vddarmVal.trim().contains("1200")
                            || vddarmVal.trim().contains("1150") || vddarmVal.contains("1100"))) {
                boolean flag = writeFile(vddarmVal.trim(), "/sys/power/cpufreq_voltage");
                if (flag) {
                    final String vddarmCurr = readFile("/sys/devices/platform/sc2711-regulator/regulator/regulator.13/microvolts");
                    Log.d(TAG, "vddarmCurr->" + vddarmCurr);
                    if (!"".equals(vddarmCurr) && !"readError".equals(vddarmCurr)) {
                        mVDDARMPre.setSummary(Integer.valueOf(vddarmCurr).intValue() / 1000
                                + "mV");
                        SystemProperties.set(PERSIST_KEY_VDDARM, vddarmVal.trim());
                    } else {
                        Toast.makeText(PDSActivity.this, "Read Fail", Toast.LENGTH_SHORT).show();
                        return false;
                    }
                } else {
                    Toast.makeText(PDSActivity.this, "Write fail", Toast.LENGTH_SHORT).show();
                    return false;
                }
            } else {
                Toast.makeText(PDSActivity.this, "number must be 1100,1150,1200,1300",
                        Toast.LENGTH_SHORT).show();
                return false;
            }
        }
        return true;
    }

    private void checkDVFSStatus() {
        if (mIsFifteenChip) {
            mReadRes = readFile(PATH_DVFS_2);
            if (!"readError".equals(mReadRes) && !"".equals(mReadRes)) {
                if ("sprdemand".equals(mReadRes)) {
                    mDVFSPre.setChecked(true);
                    mCPUPre.setEnabled(false);
                    mVDDARMPre.setEnabled(false);
                    return;
                } else if ("performance".equals(mReadRes)) {
                    mDVFSPre.setChecked(false);
                    // enabled the setting CPU and VDD
                    mCPUPre.setEnabled(true);
                    mVDDARMPre.setEnabled(true);
                    initCpuFre();
                    initVddarm();
                    return;
                }
            }
            Log.d(TAG, "15 chip DVFS Abnormal because read file error");
            mDVFSPre.setEnabled(false);
            mDVFSPre.setSummary(R.string.feature_abnormal);
        } else if (mIsZeroChip) {
            mReadRes = readFile(PATH_DVFS_1);
            mReadRes1 = readFile(PATH_DVFS_2);
            if (!"readError".equals(mReadRes) && !"readError".equals(mReadRes1)
                    && !"".equals(mReadRes) && !"".equals(mReadRes1)) {
                if ("0".equals(mReadRes)
                        && "sprdemand".equals(mReadRes1)) {
                    mDVFSPre.setChecked(true);
                    return;
                } else if ("1".equals(mReadRes)
                        && "performance".equals(mReadRes1)) {
                    mDVFSPre.setChecked(false);
                    return;
                }
            }
            Log.d(TAG, "*0 chip DVFS Abnormal because read file error");
            mDVFSPre.setEnabled(false);
            mDVFSPre.setSummary(R.string.feature_abnormal);
        } else if (mVersion.contains("9630")) {
            if (SystemProperties.get("persist.sys.dvfs1", "performance").equals("performance")
                    && SystemProperties.get("persist.sys.dvfs2", "1").equals("1")) {
                mDVFSPre.setChecked(false);
            } else if (SystemProperties.get("persist.sys.dvfs1", "performance").equals(
                    "sprdemand")
                    && SystemProperties.get("persist.sys.dvfs2", "1").equals("0")) {
                mDVFSPre.setChecked(true);
            } else {
                mDVFSPre.setEnabled(false);
                mDVFSPre.setSummary(R.string.feature_abnormal);
            }
        }
    }

    protected void initCpuFre() {
        if (mCPUPre != null && mCPUPre.isEnabled()) {
            String cpuFre = readFile("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq");
            if (!"".equals(cpuFre) && !"readError".equals(cpuFre)) {
                mCPUPre.setSummary(Integer.valueOf(cpuFre).intValue() / 1000 + "mHz");
            } else {
                Log.d(TAG, "CPU/VDDARM Abnormal because initCpuFre Fail");
                mCPUPre.setEnabled(false);
                mVDDARMPre.setEnabled(false);
                mCPUPre.setSummary(R.string.feature_abnormal);
                mVDDARMPre.setSummary(R.string.feature_abnormal);
            }
        }
    }

    protected void initVddarm() {
        if (mVDDARMPre != null && mVDDARMPre.isEnabled()) {
            String vddarm = readFile("/sys/devices/platform/sc2711-regulator/regulator/regulator.13/microvolts");
            if (!"".equals(vddarm) && !"readError".equals(vddarm)) {
                mVDDARMPre.setSummary(Integer.valueOf(vddarm).intValue() / 1000 + "mV");
            } else {
                Log.d(TAG, "CPU/VDDARM Abnormal because initVddarm Fail");
                mCPUPre.setEnabled(false);
                mVDDARMPre.setEnabled(false);
                mCPUPre.setSummary(R.string.feature_abnormal);
                mVDDARMPre.setSummary(R.string.feature_abnormal);
            }
        }
    }

    protected void closeDvfs() {
        mCPUFre = SystemProperties.get(PERSIST_KEY_CPU_FREQUENCE, "");
        mVDDARM = SystemProperties.get(PERSIST_KEY_VDDARM, "");
        Log.d(TAG, "SystemProperties mCPUFre->" + mCPUFre + ",SystemProperties vddarm->"
                + mVDDARM);

        if (!"".equals(mCPUFre)) {
            writeFile(mCPUFre, "/sys/power/cpufreq_frequency");
        }
        if (!"".equals(mVDDARM)) {
            writeFile(mVDDARM, "/sys/power/cpufreq_voltage");
        }
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
