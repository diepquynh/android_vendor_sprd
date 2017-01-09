/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.server.lights;

import android.util.Slog;

public class SprdLightsUtils implements ILightsUtils{

    private final static String TAG = "SprdLightsUtils";

    private boolean isBatteryLightOpen = false;

    public SprdLightsUtils(){

    }

    @Override
    public boolean isBatteryOpenWhenNotificationCome(int id, int color) {
        boolean batteryOpenWhenNotificationCome = (id == LightsManager.LIGHT_ID_NOTIFICATIONS)&&(isBatteryLightOpen);

        if (batteryOpenWhenNotificationCome){
            Slog.d(TAG, "id = " + id + "; isBatteryLightOpen = " + isBatteryLightOpen);
        }

        if(id == LightsManager.LIGHT_ID_BATTERY){
            if(color == 0 ){
                isBatteryLightOpen = false;
            } else
                isBatteryLightOpen = true;
        }

        return batteryOpenWhenNotificationCome;
    }
}
