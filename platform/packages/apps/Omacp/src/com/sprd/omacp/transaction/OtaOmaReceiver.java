/*
 * Copyright (C) 2007-2008 Esmertec AG.
 * Copyright (C) 2007-2008 The Android Open Source Project
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

package com.sprd.omacp.transaction;

import static android.provider.Telephony.Sms.Intents.WAP_PUSH_RECEIVED_ACTION;

import com.sprd.xml.parser.prv.OmacpUtils;

//import com.android.mms.transaction.MessagingNotification;
//import com.sprd.mms.ota.ui.ConfirmOTAActivity;

import android.R.integer;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.PowerManager;
import android.os.SystemProperties;
import android.util.Config;
import android.util.Log;

/**
 * Receives Intent.WAP_PUSH_RECEIVED_ACTION intents
 */
public class OtaOmaReceiver extends BroadcastReceiver {
    private static final String TAG = "OtaOmaReceiver";

    private static final boolean DEBUG = true;
    private final int ANDROID_L_VERSION = 22;
    private static final boolean LOCAL_LOGV = DEBUG ? Config.LOGD : Config.LOGV;

    @Override
    public void onReceive(Context context, Intent intent) {
        // SPRD: fix for bug 488045
        if (intent.getAction() != null) {
            if (intent.getAction().equals(WAP_PUSH_RECEIVED_ACTION)) {
                Log.i(TAG, "Received PUSH Intent: " + intent);
                Log.e(TAG, "receive ota message in OmacpManager.apk ");
                if (isUseNewOmacp(context)) {
                    // Hold a wake lock for 5 seconds, enough to give any
                    // services we start time to take their own wake locks.
                    PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
                    PowerManager.WakeLock wl = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                            "MMS PushReceiver");
                    wl.acquire(5000);
                    intent.setClass(context, OtaOmaService.class);
                    context.startService(intent);
                    OmacpUtils.registerMsgNf(context);
                }
            }
        }
    }

    private boolean isUseNewOmacp(Context context) {
        int currentVersion = android.os.Build.VERSION.SDK_INT;
        Log.d(TAG, "current version is : " + currentVersion);
        if (currentVersion > ANDROID_L_VERSION) {
            return true;
        } else
            return SystemProperties.getBoolean("ro.sys.omacp_enable", false);
    }

}
