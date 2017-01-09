/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.sprd.APCT;

import com.sprd.APCT.R;
import com.sprd.APCT.APCTSettings;
import java.util.ArrayList;
import java.util.HashSet;

import android.app.ActionBar;
import android.app.Activity;
import android.app.ActivityManagerNative;
import android.app.ActivityThread;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.DialogFragment;
import android.app.admin.DevicePolicyManager;
import android.app.backup.IBackupManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.os.AsyncTask;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.StrictMode;
import android.os.SystemProperties;
import android.os.Trace;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.MultiCheckPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceFragment;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.HardwareRenderer;
import android.view.IWindowManager;
import android.view.View;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.app.ActivityManager;
import android.util.Log;
import android.app.Fragment;

public class APCTMainActivity extends Activity{
    Context mContext;
    SharedPreferences sp;
    Editor editor;
    static String item_name[] = {"APP_LAUNCH_TIME",
                                 "FPS_DISPLAY",
                                 "MEMINFO_DISPLAY",
                                 "BOOT_TIME",
                                 "CAM_INIT_TIME",
                                 "NET_TIME",
                                 "APP_DATA",
                                 "BOOT_DATA",
                                 "PWR_OFF_TIME",
                                 "PWR_CHARGE_TIME",
                                 "HOME_IDLE_TIME",
                                 "TOP_RECORDER",
                                 "PROCRANK_RECORDER",
                                 "CHIP_TEMP"
                                };

	@Override
	protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mContext = APCTMainActivity.this;
        sp = getSharedPreferences("checkbox", Context.MODE_PRIVATE);
        editor = sp.edit();
        getFragmentManager().beginTransaction().replace(android.R.id.content, new APCTFragment()).commit();
    }

    public class APCTFragment extends PreferenceFragment{
    private IWindowManager mWindowManager;
    private IBackupManager mBackupManager;
    private DevicePolicyManager mDpm;
    private ConnectivityManager mConnectivityManager;

    private Switch mEnabledSwitch;
    private boolean mLastEnabledState;
    private boolean mHaveDebugSettings;
    private boolean mDontPokeProperties;

    private CheckBoxPreference mEnableAppLaunchTime;
    private CheckBoxPreference mEnableFpsDisp;
    private CheckBoxPreference mEnableMemDisp;
    private CheckBoxPreference mEnablePwrOnTime;

    private CheckBoxPreference mEnableCamInitTime;
    private CheckBoxPreference mEnableNetSearchTime;
    private CheckBoxPreference mEnableAppData;
    private CheckBoxPreference mEnableBootData;

    private CheckBoxPreference mEnablePwrOffTime;
    private CheckBoxPreference mEnableChargeTime;
    private CheckBoxPreference mEnableHome2IdleTime;

    private CheckBoxPreference mEnableTopRecorder;
    private CheckBoxPreference mEnableProcrankRecorder;

    private CheckBoxPreference mEnableChipTemp;

    private PreferenceScreen mTpRate;
    private PreferenceScreen mSensorRate;
    private PreferenceScreen mRamThroughput;
    private PreferenceScreen mRomSpeed;
    private PreferenceScreen mTop;
    private PreferenceScreen mProcrank;
    private PreferenceScreen mPsInfo;

    private final ArrayList<Preference> mAllPrefs = new ArrayList<Preference>();
    private final ArrayList<CheckBoxPreference> mResetCbPrefs
            = new ArrayList<CheckBoxPreference>();

    private final HashSet<Preference> mDisabledPrefs = new HashSet<Preference>();

    public APCTFragment(){};

    public Fragment instantiate()
    {
        return this;
    }

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        mWindowManager = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));
        mBackupManager = IBackupManager.Stub.asInterface(ServiceManager.getService(Context.BACKUP_SERVICE));
        mDpm = (DevicePolicyManager)getActivity().getSystemService(Context.DEVICE_POLICY_SERVICE);
        mConnectivityManager = (ConnectivityManager) getActivity().getSystemService(Context.CONNECTIVITY_SERVICE);

        addPreferencesFromResource(R.xml.apct_prefs);

        mEnableAppLaunchTime = findAndInitCheckboxPref("app_launch_time");
        mEnableFpsDisp = findAndInitCheckboxPref("fps_disp");
        mEnableMemDisp = findAndInitCheckboxPref("meminfo_disp");
        mEnablePwrOnTime = findAndInitCheckboxPref("pwron_time");
        mEnableCamInitTime = findAndInitCheckboxPref("camera_init_time");
        mEnableNetSearchTime = findAndInitCheckboxPref("net_search_time");
        mEnablePwrOffTime = findAndInitCheckboxPref("enable_power_off_time");
        mEnableChargeTime = findAndInitCheckboxPref("charge_time");
        mEnableHome2IdleTime = findAndInitCheckboxPref("return_to_idle_time");
        mEnableAppData = findAndInitCheckboxPref("app_data");
        mEnableBootData = findAndInitCheckboxPref("boot_data");
        mEnableTopRecorder = findAndInitCheckboxPref("top_recorder");
        mEnableProcrankRecorder = findAndInitCheckboxPref("procrank_recorder");
        mEnableChipTemp = findAndInitCheckboxPref("chip_temp");
        updateAllEnableStatus();

        mTpRate = (PreferenceScreen)findPreference("tp_rate");
        mAllPrefs.add(mTpRate);
        mSensorRate = (PreferenceScreen)findPreference("sensor_rate");
        mAllPrefs.add(mSensorRate);
        mRamThroughput = (PreferenceScreen)findPreference("ram_throughput");
        mAllPrefs.add(mRamThroughput);
        mRomSpeed = (PreferenceScreen)findPreference("rom_wr_speed");
        mAllPrefs.add(mRomSpeed);
        mTop = (PreferenceScreen)findPreference("realtime_top");
        mAllPrefs.add(mTop);
        mProcrank = (PreferenceScreen)findPreference("procrank_result");
        mAllPrefs.add(mProcrank);
        mPsInfo = (PreferenceScreen)findPreference("processinfo");
        mAllPrefs.add(mPsInfo);

        updateAllCheckStatus();

	    if (IsFloatWinShow() && !isAPCTServiceWorked())
	    {
            Intent service = new Intent();
            service.setClass(APCTMainActivity.this, APCTService.class);
            mContext.startService(service);
	    }
    }

       public boolean IsFloatWinShow()
       {
           boolean ret  = false;
           boolean ret1 = false;
           boolean ret2 = false;

           if (  mEnableAppLaunchTime.isChecked() || mEnableFpsDisp.isChecked() || mEnableMemDisp.isChecked()
              || mEnablePwrOnTime.isChecked() || mEnableCamInitTime.isChecked() || mEnableNetSearchTime.isChecked()
              || mEnablePwrOffTime.isChecked() || mEnableChargeTime.isChecked() || mEnableHome2IdleTime.isChecked()
              || mEnableAppData.isChecked() || mEnableBootData.isChecked() || mEnableTopRecorder.isChecked()
              || mEnableProcrankRecorder.isChecked() || mEnableChipTemp.isChecked())
           {
               ret1 = true;
           }

           if (   APCTSettings.isApctAppTimeSupport() || APCTSettings.isApctFpsSupport() || APCTSettings.isApctBootTimeSupport() 
               || APCTSettings.isApctCameraInitSupport() || APCTSettings.isApctNetSearchSupport() || APCTSettings.isApctAppDataSupport()
               || APCTSettings.isApctBootDataSupport() || APCTSettings.isApctPowerOffSupport() || APCTSettings.isApctChargeSupport()
               || APCTSettings.isApctHome2IdleSupport() || APCTSettings.isApctMemInfoSupport() || APCTSettings.isApctTopSupport()
           || APCTSettings.isApctProcrankSupport())
           {
               ret2 = true;
           }
           ret = ret1 && ret2;
           return ret;
       }

       public boolean isAPCTServiceWorked()
       {
           ActivityManager myManager=(ActivityManager)mContext.getSystemService(Context.ACTIVITY_SERVICE);
           ArrayList<ActivityManager.RunningServiceInfo> runningService = (ArrayList<ActivityManager.RunningServiceInfo>) myManager.getRunningServices(30);

           for(int i = 0 ; i<runningService.size();i++)
           {
               if(runningService.get(i).service.getClassName().toString().equals("com.sprd.APCT.APCTService"))
               {
                   return true;
               }
           }
           return false;
       }

    private CheckBoxPreference findAndInitCheckboxPref(String key) {
        CheckBoxPreference pref = (CheckBoxPreference) findPreference(key);
        if (pref == null) {
            throw new IllegalArgumentException("Cannot find preference with key = " + key);
        }
        mAllPrefs.add(pref);
        mResetCbPrefs.add(pref);
        return pref;
    }

    private void setPrefsEnabledState(boolean enabled) {
        for (int i = 0; i < mAllPrefs.size(); i++) {
            Preference pref = mAllPrefs.get(i);
            pref.setEnabled(enabled && !mDisabledPrefs.contains(pref));
        }
        updateAllCheckStatus();
    }

    @Override
    public void onResume() {
        super.onResume();
        APCTSettings.getApctStatus();
        updateAllEnableStatus();
        updateAllCheckStatus();
    }

    void updateCheckBox(CheckBoxPreference checkBox, boolean value) {
        checkBox.setChecked(value);
    }

    void updateAllEnableStatus()
    {
        APCTSettings.getApctStatus();
        mEnableAppLaunchTime.setEnabled(APCTSettings.isApctAppTimeSupport());
        mEnableFpsDisp.setEnabled(APCTSettings.isApctFpsSupport());
        mEnablePwrOnTime.setEnabled(APCTSettings.isApctBootTimeSupport());
        mEnableCamInitTime.setEnabled(APCTSettings.isApctCameraInitSupport());
        mEnableNetSearchTime.setEnabled(APCTSettings.isApctNetSearchSupport());
        mEnableAppData.setEnabled(APCTSettings.isApctAppDataSupport());
        mEnableBootData.setEnabled(APCTSettings.isApctBootDataSupport());
        mEnablePwrOffTime.setEnabled(APCTSettings.isApctPowerOffSupport());
        mEnableChargeTime.setEnabled(APCTSettings.isApctChargeSupport());
        mEnableHome2IdleTime.setEnabled(APCTSettings.isApctHome2IdleSupport());
        mEnableTopRecorder.setEnabled(APCTSettings.isApctTopSupport());
        mEnableProcrankRecorder.setEnabled(APCTSettings.isApctProcrankSupport());
        mEnableMemDisp.setEnabled(APCTSettings.isApctMemInfoSupport());
        mEnableChipTemp.setEnabled(true);
    }

    private void updateAllCheckStatus() {
        updateCheckBox(mEnableAppLaunchTime, sp.getBoolean(item_name[0], false));
        updateCheckBox(mEnableFpsDisp, sp.getBoolean(item_name[1], false));
        updateCheckBox(mEnableMemDisp, sp.getBoolean(item_name[2], false));
        updateCheckBox(mEnablePwrOnTime, sp.getBoolean(item_name[3], false));
        updateCheckBox(mEnableCamInitTime, sp.getBoolean(item_name[4], false));
        updateCheckBox(mEnableNetSearchTime, sp.getBoolean(item_name[5], false));
        updateCheckBox(mEnableAppData, sp.getBoolean(item_name[6], false));
        updateCheckBox(mEnableBootData, sp.getBoolean(item_name[7], false));
        updateCheckBox(mEnablePwrOffTime, sp.getBoolean(item_name[8], false));
        updateCheckBox(mEnableChargeTime, sp.getBoolean(item_name[9], false));
        updateCheckBox(mEnableHome2IdleTime, sp.getBoolean(item_name[10], false));
        updateCheckBox(mEnableTopRecorder, sp.getBoolean(item_name[11], false));
        updateCheckBox(mEnableProcrankRecorder, sp.getBoolean(item_name[12], false));
        updateCheckBox(mEnableChipTemp, sp.getBoolean(item_name[13], false));
    }

    private int getCheckboxIdx(Preference pref)
    {
        int idx = -1;

        if (pref == mEnableAppLaunchTime)         idx = 0;
        else if (pref == mEnableFpsDisp)          idx = 1;
        else if (pref == mEnableMemDisp)          idx = 2;
        else if (pref == mEnablePwrOnTime)        idx = 3;
        else if (pref == mEnableCamInitTime)      idx = 4;
        else if (pref == mEnableNetSearchTime)    idx = 5;
        else if (pref == mEnableAppData)          idx = 6;
        else if (pref == mEnableBootData)         idx = 7;
        else if (pref == mEnablePwrOffTime)       idx = 8;
        else if (pref == mEnableChargeTime)       idx = 9;
        else if (pref == mEnableHome2IdleTime)    idx = 10;
        else if (pref == mEnableTopRecorder)      idx = 11;
        else if (pref == mEnableProcrankRecorder) idx = 12;
        else if (pref == mEnableChipTemp)          idx = 13;

        return idx;
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        int idx = getCheckboxIdx(preference);

        if (idx != -1)
        {
            boolean b = ((CheckBoxPreference)preference).isChecked();
            editor.putBoolean(item_name[idx], b);
            editor.commit();
            boolean b0 = sp.getBoolean(item_name[idx], false);

            if (b && !isAPCTServiceWorked())
            {
                Intent service = new Intent();
                service.setClass(APCTMainActivity.this, APCTService.class);
                startService(service);
            }
            else
            if (!IsFloatWinShow())
            {
                Intent service = new Intent();
                service.setClass(APCTMainActivity.this, APCTService.class);
                stopService(service);
            }

            if (isAPCTServiceWorked())
            {
                Intent intent = new Intent();
                intent.setAction("com.sprd.APCT.APCTService.status_changed");
                intent.putExtra("cmd", idx);
                mContext.sendBroadcast(intent);
            }
        }
        return false;
    }
  }
}


