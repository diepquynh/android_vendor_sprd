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

package com.android.sprdlauncher2;

import android.app.Application;
import android.content.res.Configuration;

public class LauncherApplication extends Application {
    /* SPRD: fix bug251158 @{ */
    static final float LAUNCHER_FONT_SIZE = 1.0f;
    private final Configuration mCurConfig = new Configuration();
    /* @} */

    @Override
    public void onCreate() {
        super.onCreate();
        /* SPRD: fix bug251158 @{ */
        checkForConfigChanges();
        /* @} */
        LauncherAppState.setApplicationContext(this);
        LauncherAppState.getInstance();
    }

    @Override
    public void onTerminate() {
        super.onTerminate();
        LauncherAppState.getInstance().onTerminate();
    }

/* SPRD: fix bug251158  @{ */
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        checkForConfigChanges();
    }

    private void checkForConfigChanges() {
        if (mCurConfig != null) {
            mCurConfig.updateFrom(getResources().getConfiguration());

            if (mCurConfig.fontScale != LAUNCHER_FONT_SIZE) {
                mCurConfig.fontScale = LAUNCHER_FONT_SIZE;
            }

            if (mCurConfig.orientation != Configuration.ORIENTATION_PORTRAIT) {
                mCurConfig.orientation = Configuration.ORIENTATION_PORTRAIT;
            }

            if (!getResources().getConfiguration().equals(mCurConfig)) {
                getResources().updateConfiguration(mCurConfig, null);
            }
        }

    }
    /* @} */
}
