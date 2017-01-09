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

import com.android.internal.telephony.cat.CatLog;
import com.sprd.stk.StkTelcelOperatorPluginsHelper;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TelephonyProperties;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.app.PackageManagerEx;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.os.SystemProperties;

/**
 * Application installer for SIM Toolkit.
 *
 */
abstract class StkAppInstaller {
    private static final String STK_MAIN_ACTIVITY = "com.android.stk.StkMain";
    private static final String LOG_TAG = "StkAppInstaller";

    public static final int NOT_INSTALLED = 0;
    public static final int INSTALLED = 1;
    private static int mInstalled = -1;
    private StkAppInstaller() {
        CatLog.d(LOG_TAG, "init");
    }

    public static void install(Context context) {
        setAppState(context, true);
    }

    public static void unInstall(Context context) {
        setAppState(context, false);
    }

    private static void setAppState(Context context, boolean install) {
        if (install) {
            CatLog.d(LOG_TAG, "[setAppState] === install");
        } else {
            CatLog.d(LOG_TAG, "[setAppState] === uninstall");
        }
        if (context == null) {
            CatLog.d(LOG_TAG, "[setAppState]- no context, just return.");
            return;
        }
        PackageManagerEx pmx = (PackageManagerEx)context.getPackageManager();
        PackageManager pm = context.getPackageManager();
        if (pm == null) {
            CatLog.d(LOG_TAG, "[setAppState]- no package manager, just return.");
            return;
        }
        ComponentName cName = new ComponentName("com.android.stk", STK_MAIN_ACTIVITY);
        int state = install ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED
                : PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
        /* SPRD: add for cucc two icon feature . @{ */
        CatLog.d(LOG_TAG, "cName = " + cName);
        CatLog.d(LOG_TAG, "state = " + state);
        CatLog.d(LOG_TAG, "context = " + context);
        CatLog.d(LOG_TAG, "SettingState = " + pm.getComponentEnabledSetting(cName));
        /* @} */
        if (StkTelcelOperatorPluginsHelper.getInstance(context).isTelcelOperator()) {
            if ((mInstalled == NOT_INSTALLED && state == PackageManager.COMPONENT_ENABLED_STATE_DISABLED)
                    || (mInstalled == INSTALLED && state == PackageManager.COMPONENT_ENABLED_STATE_ENABLED)) {
                CatLog.d("<0>StkAppInstaller", "Do not need to change STK app state");
            } else {
                mInstalled = install ? INSTALLED : NOT_INSTALLED;
                ComponentName mName = new ComponentName("com.android.stk", "com.android.stk.StkMenuActivity");
                try {
                    pm.setComponentEnabledSetting(cName, PackageManager.COMPONENT_ENABLED_STATE_ENABLED,
                            PackageManager.DONT_KILL_APP);
                    StkAppService stkService = StkAppService.getInstance();
                    String labelName = null;
                    if (stkService != null && stkService.isShowSetupMenuTitle()) {
                        if (stkService.getMenu(0) != null) {
                            labelName = stkService.getMenu(0).title;
                        } else if (stkService.getMenu(1) != null) {
                            labelName = stkService.getMenu(1).title;
                        } else {
                            labelName = "SIM Telcel";
                        }
                    } else {
                        labelName = "SIM Telcel";
                    }
                    Intent intent = new Intent();
                    intent.putExtra("setup.menu.labelName", labelName);
                    pmx.setComponentEnabledSettingForSetupMenu(cName, PackageManager.DONT_KILL_APP, intent);
                    pmx.setComponentEnabledSettingForSetupMenu(mName, PackageManager.DONT_KILL_APP, intent);
                } catch (Exception e) {
                    CatLog.d("<0>StkAppInstaller", "Could not change STK app state");
                }
            }
            return;
        }

        if (((PackageManager.COMPONENT_ENABLED_STATE_ENABLED == state) &&
                (PackageManager.COMPONENT_ENABLED_STATE_ENABLED ==
                pm.getComponentEnabledSetting(cName))) ||
                ((PackageManager.COMPONENT_ENABLED_STATE_DISABLED == state) &&
                (PackageManager.COMPONENT_ENABLED_STATE_DISABLED ==
                pm.getComponentEnabledSetting(cName)))) {
            CatLog.d(LOG_TAG, "Need not change app state!!");
        } else {
            CatLog.d(LOG_TAG, "Change app state[" + install + "]");
            try {
                /* SPRD: Set STK launcher name with insert one SIM card. @{ */
                StkAppService appService = StkAppService.getInstance();
                String showName = null;
                if (appService != null && appService.isShowSetupMenuTitle()) {
                    if (appService.getMenu(0) != null) {
                        showName = appService.getMenu(0).title;
                    } else if (appService.getMenu(1) != null) {
                        showName = appService.getMenu(1).title;
                    }
                    CatLog.d(LOG_TAG, "Set launcher name to showName:" + showName);
                    if (!TextUtils.isEmpty(showName)) {
                        Intent intent = new Intent();
                        intent.putExtra("setup.menu.labelName", showName);
                        pmx.setComponentEnabledSettingForSetupMenu(cName,
                                PackageManager.DONT_KILL_APP, intent);
                    }
                }
                /* @} */

                CatLog.d(LOG_TAG, "setComponentEnabledSetting:" + state);
                pm.setComponentEnabledSetting(cName, state, PackageManager.DONT_KILL_APP);
            } catch (Exception e) {
                CatLog.d(LOG_TAG, "Could not change STK app state e:" + e);
            }
        }
        CatLog.d(LOG_TAG, "[setAppState]-");
    }


    /* SPRD: add for cucc two icon feature . @{ */
    public static void install(Context context, int slotid) {
        setAppState(context, true, slotid);
    }

    public static void unInstall(Context context, int slotid) {
        setAppState(context, false, slotid);
    }

    private static void setAppState(Context context, boolean install, int slotid) {
        CatLog.d(LOG_TAG, "setAppState() and install: " + install + " slotid: " + slotid);
        if ( null == context || slotid < 0) {
            return;
        } else {
            CatLog.d(LOG_TAG, "context= " + context);
            PackageManager pm = context.getPackageManager();
            if (pm == null) {
                CatLog.d(LOG_TAG, "setAppState()- no package manager, just return.");
                return;
            }
            String[] launcherActivity = {
                    "com.android.stk.StkMain1",
                    "com.android.stk.StkMain2"
            };
            ComponentName cName = new ComponentName("com.android.stk", launcherActivity[slotid]);
            int state = install ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED
                    : PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
            CatLog.d(LOG_TAG, "cName = " + cName);
            CatLog.d(LOG_TAG, "SettingState = " + pm.getComponentEnabledSetting(cName));
            if (((PackageManager.COMPONENT_ENABLED_STATE_ENABLED == state) &&
                    (PackageManager.COMPONENT_ENABLED_STATE_ENABLED ==
                    pm.getComponentEnabledSetting(cName))) ||
                    ((PackageManager.COMPONENT_ENABLED_STATE_DISABLED == state) &&
                    (PackageManager.COMPONENT_ENABLED_STATE_DISABLED ==
                    pm.getComponentEnabledSetting(cName)))) {
                CatLog.d(LOG_TAG, "Need not change app state!!");
            } else {
                try {
                    CatLog.d(LOG_TAG, "setComponentEnabledSetting[" + state + "]");
                    pm.setComponentEnabledSetting(cName, state, PackageManager.DONT_KILL_APP);
                } catch (Exception e) {
                    CatLog.d(LOG_TAG, "Could not change STK app state");
                }
            }
        }
    }
    /* @} */
}
