/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.quicksearchbox;

import android.app.Application;
import android.content.res.Configuration;

public class QsbApplicationWrapper extends Application {

    private QsbApplication mApp;
    private Configuration mConfiguration;

    /*
     * SPRD: Add 20140410 Spreadst of bug296731, icons of apps didn't update
     * when theme change @{
     */
    @Override
    public void onCreate() {
        super.onCreate();
        mConfiguration = new Configuration(getResources().getConfiguration());
        if (mConfiguration == null) {
            mConfiguration = new Configuration();
            mConfiguration.setToDefaults();
        }
    }
    /* @} */

    @Override
    public void onTerminate() {
        synchronized (this) {
            if (mApp != null) {
                mApp.close();
            }
        }
        super.onTerminate();
    }

    public synchronized QsbApplication getApp() {
        if (mApp == null) {
            mApp = createQsbApplication();
        }
        return mApp;
    }

    protected QsbApplication createQsbApplication() {
        return new QsbApplication(this);
    }

    /*
     * SPRD: Add 20140410 Spreadst of bug296731, icons of apps didn't update
     * when theme change @{
     */
    /*
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (newConfig != null) {
            // Only user setting theme could take effect
            if (newConfig.UserSetTheme != null
                    && (mConfiguration.UserSetTheme == null
                    || !mConfiguration.UserSetTheme
                            .equals(newConfig.UserSetTheme))) {
                mConfiguration.UserSetTheme = newConfig.UserSetTheme;
                getApp().getCorpora().update();
            }
        }
    }
    */
    /* @} */
}
