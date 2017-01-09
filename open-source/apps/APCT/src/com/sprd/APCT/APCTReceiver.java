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

import com.sprd.APCT.APCTService;
import com.sprd.APCT.APCTSettings;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import java.io.FileWriter;
import java.io.FileReader;
import java.io.IOException;
import java.lang.Float;
import android.os.SystemClock;
import android.telephony.ServiceState;
import com.android.internal.telephony.TelephonyIntents;
import android.provider.Settings;
import android.content.ContentResolver;
import android.app.Activity;
import android.content.SharedPreferences;
import com.android.internal.telephony.Phone;
import android.telephony.TelephonyManager;

public class APCTReceiver extends BroadcastReceiver
{
    static final String BOOT_ACTION  = "android.intent.action.BOOT_COMPLETED";
    static final String SIM1_ACTION    = "android.intent.action.SERVICE_STATE";
    static final String SIM2_ACTION    = "android.intent.action.SERVICE_STATE1";
    static final String SIM3_ACTION    = "android.intent.action.SERVICE_STATE2";
    static final String SIM4_ACTION    = "android.intent.action.SERVICE_STATE3";
    static final String TAG = "APCTReceiver";
    static boolean first_flg[] = {true, true, true, true};
    int sim_count = TelephonyManager.getPhoneCount();
    long[] sim_time = new long[sim_count << 1];
    static String net_str = null;

    public boolean IsFloatWinShow(Context context)
    {
        boolean ret  = false;
        boolean ret1 = false;
        boolean ret2 = false;

        SharedPreferences sp = context.getSharedPreferences("checkbox", context.MODE_PRIVATE);

        if (  sp.getBoolean("APP_LAUNCH_TIME", false) || sp.getBoolean("FPS_DISPLAY",false) || sp.getBoolean("MEMINFO_DISPLAY",false)
           || sp.getBoolean("BOOT_TIME",false) || sp.getBoolean("CAM_INIT_TIME",false) || sp.getBoolean("NET_TIME",false)
           || sp.getBoolean("APP_DATA",false) || sp.getBoolean("BOOT_DATA",false) || sp.getBoolean("PWR_OFF_TIME",false)
           || sp.getBoolean("PWR_CHARGE_TIME",false) || sp.getBoolean("HOME_IDLE_TIME",false) || sp.getBoolean("TOP_RECORDER",false)
           || sp.getBoolean("PROCRANK_RECORDER",false))
        {
            ret1 = true;
        }

        if (  APCTSettings.isApctAppTimeSupport() || APCTSettings.isApctFpsSupport() || APCTSettings.isApctBootTimeSupport() 
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

    public void processNetSearch(int phoneId, Intent intent)
    {
        ServiceState serviceState = ServiceState.newFromBundle(intent.getExtras());
        if (first_flg[phoneId] && serviceState.getState() == ServiceState.STATE_POWER_OFF)
        {
            sim_time[phoneId<<1] = SystemClock.elapsedRealtime();
        }
        else
        if (first_flg[phoneId] && serviceState.getState() == ServiceState.STATE_IN_SERVICE)
        {
            sim_time[(phoneId<<1) + 1] = SystemClock.elapsedRealtime();
            first_flg[phoneId] = false;
            AddNetSimTime(phoneId);
        }
    }

    public void onReceive(Context context, Intent intent)
    {
        if (BOOT_ACTION.equals(intent.getAction()) && IsFloatWinShow(context))
	 {
            Intent service = new Intent();
            service.setClass(context, APCTService.class);
            context.startService(service);
        }
        else
        if (SIM1_ACTION.equals(intent.getAction()))
        {
            processNetSearch(0, intent);
        }
        else
        if (SIM2_ACTION.equals(intent.getAction()))
        {
            processNetSearch(1, intent);
        }
        else
        if (SIM3_ACTION.equals(intent.getAction()))
        {
            processNetSearch(2, intent);
        }
        else
        if (SIM4_ACTION.equals(intent.getAction()))
        {
            processNetSearch(3, intent);
        }
    }

    public void AddNetSimTime(int id)
    {
        if (id >= sim_count)
        {
            return;
        }

        long net_time = sim_time[(id<<1) + 1] - sim_time[id<<1];
        Long net_time2 = new Long(net_time);

        if (net_str == null)
        {
            net_str = "SIM";
        }
        else
        {
            net_str += "\nSIM";
        }

        net_str += Long.toString(id+1) + " Search Time: " + Long.toString(net_time2) + " ms";

        writeNetTimeToProc();
        net_time2 = null;
    }

    public final void writeNetTimeToProc()
    {
        char[] buffer = net_str.toCharArray();
        final String NET_TIME_PROC = "/proc/benchMark/net_time";

        FileWriter fr = null;
        try
        {
            fr = new FileWriter(NET_TIME_PROC, false);

            if (fr != null)
            {
                fr.write(buffer);
            }
        }catch (IOException e){
            Log.d(TAG, "+++APCT Cannot write /proc/benchMark/net_time");
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
}
