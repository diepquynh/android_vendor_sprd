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
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.FileWriter;
import java.io.FileReader;
import android.os.FileUtils;
import java.io.File;

public class APCTSettings extends Activity{
    Context mContext;
    SharedPreferences mSp;
    static long    mApctConfig;
    private static final long APCT_FUNC_SUPPORT               = 0x8000;
    private static final long ACTIVITY_LAUNCH_TIME_SHOW       = 0x8001;
    private static final long FPS_SHOW                        = 0x8002;
    private static final long MEMINFO_SHOW                    = 0x8004;
    private static final long BOOT_TIME_SHOW                  = 0x8008;
    private static final long CAMERA_INIT_TIME_SHOW           = 0x8010;
    private static final long NET_SEARCH_TIME_SHOW            = 0x8020;
    private static final long ACTIVITY_LAUNCH_DETAIL_SHOW     = 0x8040;    
    private static final long BOOT_PROCESS_DETAIL_SHOW        = 0x8080; 
    private static final long POWER_OFF_TIME_SHOW             = 0x8100;
    private static final long POWER_OFF_TO_CHARGE_TIME_SHOW   = 0x8200;
    private static final long HOME_TO_IDLE_TIME_SHOW          = 0x8400;
    private static final long TOP_SHOW                        = 0x8800;
    private static final long PROCRANK_SHOW                   = 0x9000;
    private static final long APCT_MASK                       = 0x9FFF;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = APCTContextUtil.getInstance();
        mSp = mContext.getSharedPreferences("checkbox", Context.MODE_PRIVATE);
        mApctConfig = 0;
        getFragmentManager().beginTransaction().replace(android.R.id.content, new APCTSettingsFragment()).commit();
    }

    public void WriteApctStatus()
    {
        final String FILE_NAME = "/data/data/com.sprd.APCT/apct/apct_support";
        String str = null;
        char[] buffer = String.valueOf(mApctConfig).toCharArray();
        FileWriter fr = null;

        try
        {
            fr = new FileWriter(FILE_NAME, false);
            if (fr != null)
            {
                fr.write(buffer);
            }
        }catch (IOException e){
            e.printStackTrace();
        }finally {
            try{
                if (fr != null){
                    fr.close();
                }
            }catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static void getApctStatus()
    {
        final String FILE_NAME = "/data/data/com.sprd.APCT/apct/apct_support";
        FileReader fr = null;
        BufferedReader reader = null;
        String str = null;

        File f = new File("/data/data/com.sprd.APCT/apct");

        if(!f.isDirectory())
        {
            f.mkdir();
            FileUtils.setPermissions("/data/data/com.sprd.APCT/apct", 0777, -1, -1);
        }

        try
        {
            reader = new BufferedReader(new FileReader(FILE_NAME));
            str = reader.readLine();
            FileUtils.setPermissions(FILE_NAME, 0777, -1, -1);
        }catch (IOException e) {
            e.printStackTrace();
        }finally{
            try {
                if (reader != null)
                {
                    reader.close();
                }
            }catch (IOException e) {
                e.printStackTrace();
            }
        }

        if (str != null)
        {
             String strNb = toNumString(str);
             mApctConfig = Long.parseLong(strNb);
        }
    }

    public static String toNumString(String src)
    {
        char[] cs = src.toCharArray();
	    char[] dest = new char[cs.length];
	    int index = 0;

    	for(char c : cs)
	    {
            if(c <= '9' && c >= '0')
            {
                dest[index++] = c;
	        }
        }
	    return new String(dest, 0, index);
    }

    public static boolean isApctSupport()
    {
        return  (mApctConfig & APCT_MASK) == APCT_MASK ? true : false;
    }

    public static boolean isApctAppTimeSupport()
    {
        return (mApctConfig & ACTIVITY_LAUNCH_TIME_SHOW) == ACTIVITY_LAUNCH_TIME_SHOW ? true : false;
    }

    public static boolean isApctFpsSupport()
    {
        return (mApctConfig & FPS_SHOW) == FPS_SHOW ? true : false;
    }

    public static boolean isApctMemInfoSupport()
    {
        return (mApctConfig & MEMINFO_SHOW) == MEMINFO_SHOW ? true : false;
    }

    public static boolean isApctBootTimeSupport()
    {
        return (mApctConfig & BOOT_TIME_SHOW) == BOOT_TIME_SHOW ? true : false;
    }

    public static boolean isApctBootDataSupport()
    {
        return (mApctConfig & BOOT_PROCESS_DETAIL_SHOW) == BOOT_PROCESS_DETAIL_SHOW ? true : false;
    }

    public static boolean isApctAppDataSupport()
    {
        return (mApctConfig & ACTIVITY_LAUNCH_DETAIL_SHOW) == ACTIVITY_LAUNCH_DETAIL_SHOW ? true : false;
    }

    public static boolean isApctCameraInitSupport()
    {
        return (mApctConfig & CAMERA_INIT_TIME_SHOW) == CAMERA_INIT_TIME_SHOW ? true : false;
    }

    public static boolean isApctPowerOffSupport()
    {
        return (mApctConfig & POWER_OFF_TIME_SHOW) == POWER_OFF_TIME_SHOW ? true : false;
    }

    public static boolean isApctChargeSupport()
    {
        return (mApctConfig & POWER_OFF_TO_CHARGE_TIME_SHOW) == POWER_OFF_TO_CHARGE_TIME_SHOW ? true : false;
    }

    public static boolean isApctHome2IdleSupport()
    {
        return (mApctConfig & HOME_TO_IDLE_TIME_SHOW) == HOME_TO_IDLE_TIME_SHOW ? true : false;
    }

    public static boolean isApctNetSearchSupport()
    {
        return (mApctConfig & NET_SEARCH_TIME_SHOW) == NET_SEARCH_TIME_SHOW ? true : false;
    }

    public static boolean isApctTopSupport()
    {
        return (mApctConfig & TOP_SHOW) == TOP_SHOW ? true : false;
    }

    public static boolean isApctProcrankSupport()
    {
        return (mApctConfig & PROCRANK_SHOW) == PROCRANK_SHOW ? true : false;
    }

    public class APCTSettingsFragment extends PreferenceFragment{
    private IWindowManager mWindowManager;
    private IBackupManager mBackupManager;
    private DevicePolicyManager mDpm;
    private ConnectivityManager mConnectivityManager;

    private CheckBoxPreference mOpenApct;

    private CheckBoxPreference mOpenAppLaunchTime;
    private CheckBoxPreference mOpenFpsDisp;
    private CheckBoxPreference mOpenMemInfoDisp;
    private CheckBoxPreference mOpenPwrOnTime;

    private CheckBoxPreference mOpenCamInitTime;
    private CheckBoxPreference mOpenNetSearchTime;
    private CheckBoxPreference mOpenAppData;
    private CheckBoxPreference mOpenBootData;

    private CheckBoxPreference mOpenPwrOffTime;
    private CheckBoxPreference mOpenChargeTime;
    private CheckBoxPreference mOpenHome2IdleTime;
    private CheckBoxPreference mOpenTop;
    private CheckBoxPreference mOpenProcrank;

    private final ArrayList<Preference> mAllPrefs = new ArrayList<Preference>();
    private final ArrayList<CheckBoxPreference> mResetCbPrefs
            = new ArrayList<CheckBoxPreference>();

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        mWindowManager = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));
        mBackupManager = IBackupManager.Stub.asInterface(ServiceManager.getService(Context.BACKUP_SERVICE));
        mDpm = (DevicePolicyManager)getActivity().getSystemService(Context.DEVICE_POLICY_SERVICE);
        mConnectivityManager = (ConnectivityManager) getActivity().getSystemService(Context.CONNECTIVITY_SERVICE);

        addPreferencesFromResource(R.xml.apct_prefs_switch);
        mOpenApct = findAndInitCheckboxPref("open_all");
        mOpenAppLaunchTime = findAndInitCheckboxPref("open_app_launch_time");
        mOpenFpsDisp = findAndInitCheckboxPref("open_fps_disp");
        mOpenMemInfoDisp = findAndInitCheckboxPref("open_meminfo_disp");;
        mOpenPwrOnTime = findAndInitCheckboxPref("open_pwron_time");
        mOpenCamInitTime = findAndInitCheckboxPref("open_camera_init_time");
        mOpenNetSearchTime = findAndInitCheckboxPref("open_net_search_time");
        mOpenAppData = findAndInitCheckboxPref("open_app_data");
        mOpenBootData = findAndInitCheckboxPref("open_boot_data");
        mOpenPwrOffTime = findAndInitCheckboxPref("open_power_off_time");
        mOpenChargeTime = findAndInitCheckboxPref("open_charge_time");
        mOpenHome2IdleTime = findAndInitCheckboxPref("open_return_to_idle_time");
        mOpenTop = findAndInitCheckboxPref("open_top_recorder");
        mOpenProcrank = findAndInitCheckboxPref("open_procrank_recorder");
        getApctStatus();
        updateAllOptions();
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

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();
        WriteApctStatus();
    }

    private void updateAllOptions() {
        mOpenApct.setChecked(isApctSupport());
        mOpenAppLaunchTime.setChecked(isApctAppTimeSupport());
        mOpenFpsDisp.setChecked(isApctFpsSupport());
        mOpenMemInfoDisp.setChecked(isApctMemInfoSupport());
        mOpenPwrOnTime.setChecked(isApctBootTimeSupport());
        mOpenCamInitTime.setChecked(isApctCameraInitSupport());
        mOpenNetSearchTime.setChecked(isApctNetSearchSupport());
        mOpenAppData.setChecked(isApctAppDataSupport());
        mOpenBootData.setChecked(isApctBootDataSupport());
        mOpenPwrOffTime.setChecked(isApctPowerOffSupport());
        mOpenChargeTime.setChecked(isApctChargeSupport());
        mOpenHome2IdleTime.setChecked(isApctHome2IdleSupport());       
        mOpenTop.setChecked(isApctTopSupport());
        mOpenProcrank.setChecked(isApctProcrankSupport());
    }

    private void CheckAllItems(boolean b)
    {
        mOpenAppLaunchTime.setChecked(b);
        mOpenFpsDisp.setChecked(b);
        mOpenMemInfoDisp.setChecked(b);
        mOpenPwrOnTime.setChecked(b);
        mOpenCamInitTime.setChecked(b);
        mOpenNetSearchTime.setChecked(b);
        mOpenAppData.setChecked(b);
        mOpenBootData.setChecked(b);
        mOpenPwrOffTime.setChecked(b);
        mOpenChargeTime.setChecked(b);
        mOpenHome2IdleTime.setChecked(b);       
        mOpenTop.setChecked(b);
        mOpenProcrank.setChecked(b);        
    }

    private void CheckAllSpStatus(boolean b)
    {
        Editor edit = mSp.edit();

        for (int i = 0; i < 13; i++)
        {
            edit.putBoolean(APCTMainActivity.item_name[i], b);
        }
        edit.commit();

        Intent intent = new Intent();
        intent.setAction("com.sprd.APCT.APCTService.status_changed");
        intent.putExtra("cmd", 100);
        mContext.sendBroadcast(intent);        
    }

    private void CheckSpStatus(int index, boolean b)
    {
        Editor edit = mSp.edit();
        edit.putBoolean(APCTMainActivity.item_name[index], b);
        edit.commit();

        Intent intent = new Intent();
        intent.setAction("com.sprd.APCT.APCTService.status_changed");
        intent.putExtra("cmd", index);
        mContext.sendBroadcast(intent);
    }

    private int getCheckboxIdx(Preference pref)
    {
        int idx = -1;

        if (pref == mOpenApct)                  idx = 0;
        else if (pref == mOpenAppLaunchTime)    idx = 1;
        else if (pref == mOpenFpsDisp)          idx = 2;
        else if (pref == mOpenMemInfoDisp)      idx = 3;
        else if (pref == mOpenPwrOnTime)        idx = 4;
        else if (pref == mOpenCamInitTime)      idx = 5;
        else if (pref == mOpenNetSearchTime)    idx = 6;
        else if (pref == mOpenAppData)          idx = 7;
        else if (pref == mOpenBootData)         idx = 8;
        else if (pref == mOpenPwrOffTime)       idx = 9;
        else if (pref == mOpenChargeTime)       idx = 10;
        else if (pref == mOpenHome2IdleTime)    idx = 11;
        else if (pref == mOpenTop)              idx = 12;
        else if (pref == mOpenProcrank)         idx = 13;
        return idx;
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        int idx = getCheckboxIdx(preference);

        if (idx != -1)
        {
            boolean b = ((CheckBoxPreference)preference).isChecked();

            if (idx == 0)
            {
                mApctConfig = (b ? APCT_MASK : 0);
                CheckAllItems(b);
                
                if (!b)
                {
                    CheckAllSpStatus(false);
                }
            }
            else
            {
                if (b)
                {
                    mApctConfig |=  (APCT_FUNC_SUPPORT | (1 << (idx - 1)));
                }
                else
                {
                    mApctConfig &=  ~(1 << (idx - 1));
                    CheckSpStatus(idx - 1, false);
                    mOpenApct.setChecked(false);
                }

                if (isApctSupport())
                {
                    CheckAllItems(true);
                }
                else
                if ((mApctConfig & ~APCT_FUNC_SUPPORT) == 0)
                {
                    mApctConfig = 0;
                    CheckAllItems(false);
                    CheckAllSpStatus(false);
                }
            }
            WriteApctStatus();
        }
        return false;
    }    
  }
}

