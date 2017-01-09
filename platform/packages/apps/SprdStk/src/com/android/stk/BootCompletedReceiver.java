/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.stk;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;

import com.android.internal.telephony.cat.CatLog;

/* SPRD: airplane mode change install/uninstall  support @{*/
import android.telephony.TelephonyManager;
import android.provider.Settings;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.IccCardConstants;
/* @}*/

/**
 * Boot completed receiver. used to reset the app install state every time the
 * device boots.
 *
 */
public class BootCompletedReceiver extends BroadcastReceiver {

    /* SPRD: airplane mode change install/uninstall support @{ */
    private boolean isCardReady(Context context) {
        int simCount = TelephonyManager.from(context).getSimCount();
        TelephonyManager tm = TelephonyManager.from(context);
        StkAppService appService = StkAppService.getInstance();
        CatLog.d(this, "simCount: " + simCount);
        for (int i = 0; i < simCount; i++) {
            // Check if the card is inserted.
            if (tm.hasIccCard(i)) {
                CatLog.d(this, "SIM " + i + " is inserted.");
                CatLog.d(this, "mTm.hasIccCard(i) =" + tm.hasIccCard(i));
                CatLog.d(this, "appservice =" + appService);
                if (tm.getSimState(i) == TelephonyManager.SIM_STATE_READY && appService != null
                        && appService.getStkContext(i) != null
                        && appService.getStkContext(i).mMainCmd != null) {
                    CatLog.d(this, "SIM " + i + " is ready.");
                    return true;
                }
            } else {
                CatLog.d(this, "SIM " + i + " is not inserted.");
            }
        }
        return false;
    }

    private boolean isCardReady(Context context, int slot) {
        TelephonyManager tm = TelephonyManager.from(context);
        StkAppService appService = StkAppService.getInstance();
        // Check if the card is inserted.
        if (slot < 0) {
            return false;
        }
        CatLog.d(this, "tm.getSimState(slot) =" + tm.getSimState(slot));
        if (tm.getSimState(slot) == TelephonyManager.SIM_STATE_READY
                && appService != null
                && appService.getStkContext(slot) != null
                && appService.getStkContext(slot).mMainCmd != null) {
            CatLog.d(this, "SIM " + slot + " is ready.");
            return true;

        } else {
            CatLog.d(this, "SIM " + slot + " is not inserted.");
        }
        return false;
    }
    /* @} */

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        CatLog.d(this, "action:" + action);
        // make sure the app icon is removed every time the device boots.
        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            Bundle args = new Bundle();
            args.putInt(StkAppService.OPCODE, StkAppService.OP_BOOT_COMPLETED);
            context.startService(new Intent(context, StkAppService.class).putExtras(args));
        } else if (Intent.ACTION_USER_INITIALIZE.equals(action)) {
            // TODO: http://b/25155491
            if (!android.os.Process.myUserHandle().isSystem()) {
                // Disable package for all secondary users. Package is only
                // required for device owner.
                context.getPackageManager().setApplicationEnabledSetting(context.getPackageName(),
                        PackageManager.COMPONENT_ENABLED_STATE_DISABLED, 0);
                return;
            }
            /* SPRD: airplane mode change install/uninstall support @{ */
        } else if (Intent.ACTION_AIRPLANE_MODE_CHANGED.equals(action)
                || TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(action)) {
            StkAppService appService = StkAppService.getInstance();
            if (null == appService) {
                CatLog.d(this, "appService is null...");
                return;
            }
            boolean isCUCC = appService.isCUCCOperator();
            boolean isAirPlaneModeOn = Settings.Global.getInt(context.getContentResolver(),
                    Settings.Global.AIRPLANE_MODE_ON, 0) != 0;
            CatLog.d(this, "isCUCC: " + isCUCC + " and isAirPlaneModeOn: " + isAirPlaneModeOn);
            if (TelephonyIntents.ACTION_SIM_STATE_CHANGED.equals(action)) {
                int id = intent.getIntExtra(PhoneConstants.SLOT_KEY, -1);
                String state = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                CatLog.d(this, "slotID: " + id + " and state: " + state);
                if (id < 0 || isAirPlaneModeOn) {
                    return;
                }
                if (isCUCC) {
                    if (isCardReady(context, id)) {
                        StkAppInstaller.install(context, id);
                    } else {
                        if (IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(state)) {
                            StkAppInstaller.unInstall(context, id);
                        }
                    }
                } else {
                    if (isCardReady(context)) {
                        StkAppInstaller.install(context);
                    } else {
                        if (IccCardConstants.INTENT_VALUE_ICC_ABSENT.equals(state)) {
                            StkAppInstaller.unInstall(context);
                        }
                    }
                }
            } else if (Intent.ACTION_AIRPLANE_MODE_CHANGED.equals(action)) {
                int simCount = TelephonyManager.from(context).getSimCount();
                if (isAirPlaneModeOn) {
                    if (isCUCC) {
                        for (int i = 0; i < simCount; i++) {
                            StkAppInstaller.unInstall(context, i);
                        }
                    } else {
                        StkAppInstaller.unInstall(context);
                    }
                } else {
                    if (isCUCC) {
                        for (int i = 0; i < simCount; i++) {
                            if (isCardReady(context, i)) {
                                StkAppInstaller.install(context, i);
                            }
                        }
                    } else {
                        if (isCardReady(context)) {
                            StkAppInstaller.install(context);
                        }
                    }
                }
            }
        } else if (TelephonyIntents.ACTION_SERVICE_STATE_CHANGED.equals(action)) {
            CatLog.d(this, "Receive ACTION_SERVICE_STATE_CHANGED do nothing......");
            return;
        }
        /* @} */
    }
}
