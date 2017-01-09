/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.keyguard;

import java.util.List;
import android.app.ActivityManagerNative;
import android.app.ActivityOptions;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.SystemClock;
import android.os.UserHandle;
import android.telecom.TelecomManager;
import android.util.AttributeSet;
import android.util.Slog;
import android.view.View;
import android.widget.Button;

import com.android.internal.logging.MetricsLogger;
import com.android.internal.logging.MetricsProto.MetricsEvent;
import com.android.internal.telephony.IccCardConstants.State;
import com.android.internal.widget.LockPatternUtils;

import com.android.settingslib.WirelessUtils;
import android.telecom.PhoneAccount;
import android.telephony.ServiceState;
import android.telephony.SubscriptionInfo;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import com.android.internal.telephony.TelephonyIntents;
import com.sprd.keyguard.KeyguardPluginsHelper;

/**
 * This class implements a smart emergency button that updates itself based
 * on telephony state.  When the phone is idle, it is an emergency call button.
 * When there's a call in progress, it presents an appropriate message and
 * allows the user to return to the call.
 */
public class KeyguardEmergencyButton extends Button {
    private static final Intent INTENT_EMERGENCY_DIAL = new Intent()
            .setAction("com.android.phone.EmergencyDialer.DIAL")
            .setPackage("com.android.phone")
            .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                    | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS
                    | Intent.FLAG_ACTIVITY_CLEAR_TOP);

    private static final String LOG_TAG = "EmergencyButton";
    private static final String TAG = "EmergencyButton";
    private static final boolean VDBG = true;

    KeyguardUpdateMonitorCallback mInfoCallback = new KeyguardUpdateMonitorCallback() {

        @Override
        public void onSimStateChanged(int subId, int slotId, State simState) {
            updateEmergencyCallButton();
        }

        @Override
        public void onPhoneStateChanged(int phoneState) {
            updateEmergencyCallButton();
        }

        /* SPRD: modify by BUG 619741 @{ */
        @Override
        public void onRefreshCarrierInfo() {
            updateEmergencyCallButton();
        }
        /* @} */

    };


    public interface EmergencyButtonCallback {
        public void onEmergencyButtonClickedWhenInCall();
    }

    private LockPatternUtils mLockPatternUtils;
    private PowerManager mPowerManager;
    private EmergencyButtonCallback mEmergencyButtonCallback;

    private final boolean mIsVoiceCapable;
    private final boolean mEnableEmergencyCallWhileSimLocked;

    /* SPRD: FEATURE_SHOW_EMERGENCY_BTN_IN_KEYGUARD @{ */
    private final boolean mShowBtnInLockScreenForCarrier;
    private IntentFilter mIntentFilter;
    private ServiceStateChangeReciver mServiceStateChangeReciver;
    // SPRD: during power on the phone, we cannot read ss from KeyguardUpdateMonitor,
    // we can use mStickySS to save servicestate broadcast the last time.
    private ServiceState mStickySS = null;
    /* @} */

    /* SPRD: add the feature of calling 112 SOS after press long click @{ */
    private final boolean mEnableSimpleSOS;
    /* @} */

    public KeyguardEmergencyButton(Context context) {
        this(context, null);
    }

    public KeyguardEmergencyButton(Context context, AttributeSet attrs) {
        super(context, attrs);
        mIsVoiceCapable = context.getResources().getBoolean(
                com.android.internal.R.bool.config_voice_capable);
        mEnableEmergencyCallWhileSimLocked = mContext.getResources().getBoolean(
                com.android.internal.R.bool.config_enable_emergency_call_while_sim_locked);
        /* SPRD: FEATURE_SHOW_EMERGENCY_BTN_IN_KEYGUARD @{ */
        mShowBtnInLockScreenForCarrier = mContext.getResources().getBoolean(
                R.bool.config_showEmergencyBtnInLockScreen);
        /* @} */
        /* SPRD: add the feature of calling 112 SOS after press long click @{ */
        mEnableSimpleSOS = mContext.getResources().getBoolean(
                R.bool.config_enableSimpleSOS);
        Log.d(TAG, "mEnableSimpleSOS : " + mEnableSimpleSOS);
        /* @} */

    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        KeyguardUpdateMonitor.getInstance(mContext).registerCallback(mInfoCallback);
        mIntentFilter = new IntentFilter(TelephonyIntents.ACTION_SERVICE_STATE_CHANGED);
        if (mServiceStateChangeReciver == null) {
            mServiceStateChangeReciver = new ServiceStateChangeReciver();
        }
        mContext.registerReceiver(mServiceStateChangeReciver, mIntentFilter);
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        KeyguardUpdateMonitor.getInstance(mContext).removeCallback(mInfoCallback);
        if(mServiceStateChangeReciver != null){
            mContext.unregisterReceiver(mServiceStateChangeReciver);
            mServiceStateChangeReciver = null;
        }
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mLockPatternUtils = new LockPatternUtils(mContext);
        mPowerManager = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
        setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                takeEmergencyCallAction();
            }
        });
        /* SPRD: add the feature of calling 112 SOS after press long click @{ */
        setOnLongClickListener(new OnLongClickListener() {
            @Override
            public boolean onLongClick(View arg0) {
                Log.d(TAG, "call onLongClick mcc : " + getMcc());
                if (mEnableSimpleSOS && getMcc() != null
                        && (getMcc().equals("404") || getMcc().equals("405"))) {
                    Intent callIntent = new Intent(
                            Intent.ACTION_CALL_EMERGENCY);
                    callIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    callIntent.setData(Uri.fromParts(PhoneAccount.SCHEME_TEL,
                            "112", null));
                    mContext.startActivity(callIntent);
                }
                return false;
            }
        });
        /* @} */
        updateEmergencyCallButton();
    }

    @Override
    protected void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        updateEmergencyCallButton();
    }

    /* SPRD: add the feature of calling 112 SOS after press long click @{ */
    private String getMcc () {
        String mccMnc = TelephonyManager.getDefault().getSimOperator();
        String mcc = "";
        if (mccMnc != null && mccMnc.length() > 2) {
            mcc = mccMnc.substring(0, 3);
        }
        return mcc;
    }
     /* @} */

    /**
     * Shows the emergency dialer or returns the user to the existing call.
     */
    public void takeEmergencyCallAction() {
        MetricsLogger.action(mContext, MetricsEvent.ACTION_EMERGENCY_CALL);
        // TODO: implement a shorter timeout once new PowerManager API is ready.
        // should be the equivalent to the old userActivity(EMERGENCY_CALL_TIMEOUT)
        mPowerManager.userActivity(SystemClock.uptimeMillis(), true);
        try {
            ActivityManagerNative.getDefault().stopSystemLockTaskMode();
        } catch (RemoteException e) {
            Slog.w(LOG_TAG, "Failed to stop app pinning");
        }
        if (isInCall()) {
            resumeCall();
            if (mEmergencyButtonCallback != null) {
                mEmergencyButtonCallback.onEmergencyButtonClickedWhenInCall();
            }
        } else {
            KeyguardUpdateMonitor.getInstance(mContext).reportEmergencyCallAction(
                    true /* bypassHandler */);
            getContext().startActivityAsUser(INTENT_EMERGENCY_DIAL,
                    ActivityOptions.makeCustomAnimation(getContext(), 0, 0).toBundle(),
                    new UserHandle(KeyguardUpdateMonitor.getCurrentUser()));
        }
    }

    private void updateEmergencyCallButton() {
        boolean visible = false;
        if (mIsVoiceCapable) {
            if (isInsideKeyguardBottomArea()) {
                // SPRD: [CUCC] NO Sim card,Do not show emergency button
                visible = mShowBtnInLockScreenForCarrier
                        && !KeyguardPluginsHelper.getInstance().makeEmergencyInvisible();
            } else {
                // Emergency calling requires voice capability.
                if (isInCall()) {
                    visible = true; // always show "return to call" if phone is off-hook
                } else {
                    final boolean simLocked = KeyguardUpdateMonitor.getInstance(mContext)
                            .isSimPinVoiceSecure();
                    if (simLocked) {
                        // Some countries can't handle emergency calls while SIM is locked.
                        visible = mEnableEmergencyCallWhileSimLocked;
                    } else {
                        // Only show if there is a secure screen (pin/pattern/SIM pin/SIM puk);
                        visible = mLockPatternUtils.isSecure(KeyguardUpdateMonitor.getCurrentUser());
                    }
                }
            }
        }

        if (VDBG) Log.d(TAG, "visible: " + visible + ", showBtn: "
                + mShowBtnInLockScreenForCarrier);

        if (visible) {
            setVisibility(View.VISIBLE);

            int textId;
            if (isInCall()) {
                textId = com.android.internal.R.string.lockscreen_return_to_call;
            } else {
                if (isInsideKeyguardBottomArea() && !isEmergencyCallAllowed()) {
                    setClickable(false);
                    textId = R.string.lockscreen_no_available_network;
                } else {
                    setClickable(true);
                    textId = com.android.internal.R.string.lockscreen_emergency_call;
                }
            }
            setText(textId);
        } else {
            setVisibility(View.GONE);
        }
    }

    public void setCallback(EmergencyButtonCallback callback) {
        mEmergencyButtonCallback = callback;
    }

    /**
     * Resumes a call in progress.
     */
    private void resumeCall() {
        getTelecommManager().showInCallScreen(false);
    }

    /**
     * @return {@code true} if there is a call currently in progress.
     */
    private boolean isInCall() {
        return getTelecommManager().isInCall();
    }

    private TelecomManager getTelecommManager() {
        return (TelecomManager) mContext.getSystemService(Context.TELECOM_SERVICE);
    }

    /* SPRD: FEATURE_SHOW_EMERGENCY_BTN_IN_KEYGUARD @{ */
    private boolean isInsideKeyguardBottomArea() {
        return "emergency_call_btn_in_keyguard".equals(getTag());
    }

    private boolean isEmergencyCallAllowed() {
        List<SubscriptionInfo> subs = KeyguardUpdateMonitor.getInstance(mContext)
                .getSubscriptionInfo(false);
        final int N = subs.size();
        ServiceState ss = null;
        if (VDBG) Log.d(TAG, "isEmergencyCallAllowed, N: " + N);
        if (N == 0) {
            // no active subinfo, it may indicate that there is no sim.
            return true;
        }

        for (int i = 0; i < N; i++) {
            int subId = subs.get(i).getSubscriptionId();
            ss = KeyguardUpdateMonitor.getInstance(mContext)
                    .mServiceStates.get(subId);
            if (VDBG) Log.d(TAG, "subId: " + subId + "SS: " + ss);
            if (ss != null) {
                if (ss.isEmergencyOnly() || hasService(ss)) {
                    return true;
                }
            }
        }
        if(mStickySS != null) {
            if (VDBG) Log.d(TAG, "mStickySS: " + mStickySS);
            if (mStickySS.isEmergencyOnly() || hasService(mStickySS)) {
                return true;
            }
        }

        return false;
    }

    private boolean hasService(ServiceState ss) {
        if (ss != null) {
            switch (ss.getState()) {
                case ServiceState.STATE_OUT_OF_SERVICE:
                    return false;
                case ServiceState.STATE_POWER_OFF:
                    if (WirelessUtils.isAirplaneModeOn(mContext)) {
                        return true;
                    }
                default:
                    return true;
            }
        }
        return false;
    }

    private class ServiceStateChangeReciver extends BroadcastReceiver{
        @Override
        public void onReceive(Context context, Intent intent) {
            if (TelephonyIntents.ACTION_SERVICE_STATE_CHANGED.equals(intent.getAction())) {
                mStickySS = ServiceState.newFromBundle(intent.getExtras());
                if (VDBG) Log.d(TAG, "ServiceStateChangedIntent: " + intent.getAction() +
                        " mStickySS= " + mStickySS);
                updateEmergencyCallButton();
            }
        }
    }
    /* @} */

}
