/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.systemui.statusbar.policy;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.telephony.SubscriptionInfo;
import com.android.internal.annotations.VisibleForTesting;
import com.android.systemui.statusbar.policy.NetworkController.EmergencyListener;
import com.android.systemui.statusbar.policy.NetworkController.IconState;
import com.android.systemui.statusbar.policy.NetworkController.SignalCallback;

import com.android.systemui.statusbar.policy.VendorSignalCallback;

import java.util.ArrayList;
import java.util.List;


/**
 * Implements network listeners and forwards the calls along onto other listeners but on
 * the current or specified Looper.
 */
public class CallbackHandlerEx extends CallbackHandler {

    private final ArrayList<SignalCallback> mSignalCallbacks = getSignalCallbacks();

    public CallbackHandlerEx() {
        super();
    }

    @VisibleForTesting
    CallbackHandlerEx(Looper looper) {
        super(looper);
    }


    @Override
    public void setMobileVolteIndicators(final boolean show, final int subId,
            final int resId) {
        post(new Runnable() {
            @Override
            public void run() {
                for (SignalCallback signalCluster : mSignalCallbacks) {
                    if (signalCluster instanceof VendorSignalCallback) {
                        ((VendorSignalCallback) signalCluster)
                                .setMobileVolteIndicators(show, subId, resId);
                    }
                }
            }
        });
    }

    @Override
    public void setMobileRoamingIndicators(final boolean show, final int subId,
            final int resId) {
        post(new Runnable() {
            @Override
            public void run() {
                for (SignalCallback signalCluster : mSignalCallbacks) {
                    if (signalCluster instanceof VendorSignalCallback) {
                        ((VendorSignalCallback) signalCluster)
                                .setMobileRoamingIndicators(show, subId, resId);
                    }
                }
            }
        });
    }

    @Override
    public void setMobileHdVoiceIndicators(final boolean show, final int subId,
            final int resId) {
        post(new Runnable() {
            @Override
            public void run() {
                for (SignalCallback signalCluster : mSignalCallbacks) {
                    if (signalCluster instanceof VendorSignalCallback) {
                        ((VendorSignalCallback) signalCluster)
                                .setMobileHdVoiceIndicators(show, subId, resId);
                    }
                }
            }
        });
    }

    @Override
    public void setMobileDataConnectedIndicators(final boolean show,
            final int subId) {
        post(new Runnable() {
            @Override
            public void run() {
                for (SignalCallback signalCluster : mSignalCallbacks) {
                    if (signalCluster instanceof VendorSignalCallback) {
                        ((VendorSignalCallback) signalCluster)
                                .setMobileDataConnectedIndicators(show, subId);
                    }
                }
            }
        });
    }

}
