/*
 * Copyright (C) 2011 The Android Open Source Project
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

package plugin.sprd.buildnumbersettings;

import android.app.AddonManager;
import android.content.Context;
import android.os.Build;
import android.os.SystemProperties;
import android.util.Log;
import com.android.settings.R;

import com.android.settings.DeviceInfoSettings;
import com.android.settings.BuildNumberSettingsUtil;
public class BuildNumberSettings extends BuildNumberSettingsUtil implements
        AddonManager.InitialCallback {

    public static String TAG = "BuildNumberSettings";
    public static boolean DEBUG = true;

    public BuildNumberSettings() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        return clazz;
    }

    @Override
    public boolean operatorVersion() {
        if (DEBUG) {
            Log.i(TAG, "operatorVersion");
        }
        return SystemProperties.get("ro.operator.version").equals("enabled");
    }

    @Override
    public String getSystemProperties() {
        if (DEBUG) {
            Log.i(TAG, "getSystemProperties");
        }
        return SystemProperties.get("ro.operator.display.version", "unknown");
    }
}
