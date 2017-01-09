/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.server.policy;

import android.content.ContentResolver;
import android.content.Context;
import android.database.ContentObserver;
import android.os.Handler;
import android.view.IWindowManager;
import android.view.KeyEvent;


public abstract class AbsPhoneWindowManager {
    volatile boolean mAppSwitchKeyHandled;

    static final int MSG_APP_SWITCH_LONG_PRESS = 20;

    void cancelPendingAppSwitchKeyAction() {
    }

    void appswitchLongPress() {
    }

    public void handleQuickCamera(KeyEvent event, boolean down) {
    }

    /* SPRD: add for dynamic navigationbar @{ */
    boolean mDynamicNavigationBar = false;
    public static final String ACTION_HIDE_NAVIGATIONBAR = "com.action.hide_navigationbar";
    public static final String NAVIGATIONBAR_CONFIG = "navigationbar_config";
    public static boolean mNaviBooted = false;
    void initNavStatus() {
    }

    void showNavigationBar(boolean show) {
    }

    public boolean isNavigationBarShowing() {
        return true;
    }

    void registerNavIfNeeded() {
    }
    /* @} */

    /* SPRD: add pocket mode acquirement @ { */
    protected void hideDisableTouchModePanel() {
    }

    protected void showDisableTouchModePanel() {
    }

    protected void registerSprdSensors() {
    }

    protected void updateSprdSettings(ContentResolver resolver) {
    }

    protected boolean isPocketModeEnabled() {
        return false;
    }

    protected void sprdObserve(ContentResolver resolver, ContentObserver observer) {
    }

    protected void interceptDisableTouchModeChord() {
    }

    protected void interceptUpKeyEx(KeyEvent event) {
    }

    protected boolean isDisableTouchModeVolumeUpKeyConsumed() {
        return false;
    }

    protected void setDisableTouchModeVolumeUpKeyConsumed(boolean consumed) {
    }
    /* @} */
}

