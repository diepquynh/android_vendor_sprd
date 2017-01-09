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

import android.util.Log;

import java.lang.reflect.Constructor;

public  class LightsUtilsFactory {

    private static final String TAG = "LightsUtilsFactory";

    private static ILightsUtils sInstance;

    private LightsUtilsFactory(){

    }

    public synchronized static ILightsUtils getInstance() {
        if (sInstance != null) {
            return sInstance;
        }
        Class clazz = null;

        try {
            clazz = Class.forName("com.android.server.lights.SprdLightsUtils");
        } catch (Throwable t) {
            Log.d(TAG, "Can't find specific SprdLightsUtils");
        }

        if (clazz != null) {
            try {
                Constructor ctor = clazz.getConstructor();
                if (ctor != null) {
                    sInstance = (ILightsUtils) ctor.newInstance();
                    Log.d(TAG, "Create SprdLightsUtils");
                }
            } catch (Throwable t) {
                Log.e(TAG, "Can't create specific ObjectFactory");
            }
        }

        if (sInstance == null) {
            sInstance = new ILightsUtils() {
                @Override
                public boolean isBatteryOpenWhenNotificationCome(int id, int color) {
                    return false;
                }
            };
        }

        return sInstance;
    }

}
