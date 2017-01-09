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

package com.android.systemui.statusbar;

import android.annotation.DrawableRes;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.Animatable;
import android.graphics.drawable.AnimatedVectorDrawable;
import android.graphics.drawable.Drawable;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.util.ArraySet;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.telephony.TelephonyManager;

import com.android.systemui.R;
import com.android.systemui.statusbar.phone.StatusBarIconController;
import com.android.systemui.statusbar.policy.NetworkController.IconState;
import com.android.systemui.statusbar.policy.NetworkControllerImpl;
import com.android.systemui.statusbar.policy.SecurityController;
import com.android.systemui.statusbar.policy.TelephonyIconsEx;
import com.android.systemui.tuner.TunerService;
import com.android.systemui.tuner.TunerService.Tunable;

import java.util.ArrayList;
import java.util.List;

// Intimately tied to the design of res/layout/signal_cluster_view.xml
public class SignalClusterViewEx
        /*
        extends LinearLayout
        implements NetworkControllerImpl.SignalCallback,
        SecurityController.SecurityControllerCallback,
        */
        extends SignalClusterViewBase
        implements Tunable {

    static final String TAG = "SignalClusterView";
    static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);

    private static final String SLOT_AIRPLANE = "airplane";
    private static final String SLOT_MOBILE = "mobile";
    private static final String SLOT_WIFI = "wifi";
    private static final String SLOT_ETHERNET = "ethernet";

    NetworkControllerImpl mNC;
    SecurityController mSC;

    private boolean mNoSimsVisible = false;
    private boolean mVpnVisible = false;
    private boolean mEthernetVisible = false;
    private int mEthernetIconId = 0;
    private int mLastEthernetIconId = -1;
    private boolean mWifiVisible = false;
    private int mWifiStrengthId = 0;
    private int mLastWifiStrengthId = -1;
    private boolean mIsAirplaneMode = false;
    private int mAirplaneIconId = 0;
    private int mLastAirplaneIconId = -1;
    private String mAirplaneContentDescription;
    private String mWifiDescription;
    private String mEthernetDescription;
    private ArrayList<PhoneState> mPhoneStates = new ArrayList<PhoneState>();
    private int mIconTint = Color.WHITE;
    private float mDarkIntensity;
    private final Rect mTintArea = new Rect();
    /* SPRD: Add VoLte icon for bug 509601. @{ */
    private boolean mVolteVisible = false;
    private int mVolteIconId = 0;
    private int mLastVolteIconId = -1;
    /* @} */
    /* SPRD: Add HD audio icon in cucc for bug 536924. @{ */
    private boolean mHdVoiceVisible = false;
    private int mHdVoiceIconId = 0;
    private int mLastHdVoiceIconId = -1;
    /* @} */
    //SPRD: modify by BUG 606106
    private int mImsRegisterSubId = -1;
    ViewGroup mEthernetGroup, mWifiGroup;
    View mNoSimsCombo;
    ImageView mVpn, mEthernet, mWifi, mVolte, mHdVoice, mAirplane, mNoSims,
        mEthernetDark, mWifiDark, mNoSimsDark;
    View mWifiAirplaneSpacer;
    View mWifiSignalSpacer;
    LinearLayout mMobileSignalGroup;

    private final int mMobileSignalGroupEndPadding;
    private final int mMobileDataIconStartPadding;
    private final int mWideTypeIconStartPadding;
    private final int mSecondaryTelephonyPadding;
    private final int mEndPadding;
    private final int mEndPaddingNothingVisible;
    private final float mIconScaleFactor;

    private boolean mBlockAirplane;
    private boolean mBlockMobile;
    private boolean mBlockWifi;
    private boolean mBlockEthernet;

    //SPRD: Bug #474463 Add wifi inOut icon in status bar Feature BEG-->
    ImageView mWifiOut, mWifiIn;
    private boolean isWifiIn, isWifiOut;

    private boolean mShowNoSims = mContext.getResources().getBoolean(
            R.bool.config_always_show_all_sim_signalbar);

    //SPRD: Set statusbar icon tint except CUCC no sim icon.
    private boolean mIsSetMobileIconTint = mContext.getResources().getBoolean(
            R.bool.config_set_mobile_icon_tint);

    //SPRD: Always hide volte icon in Vodafone for bug 601604.
    private boolean mShowVolteIcon = mContext.getResources().getBoolean(
            R.bool.config_show_volte_icon);

    //<-- Add wifi inOut icon in status bar Feature END
    public SignalClusterViewEx(Context context) {
        this(context, null);
    }

    public SignalClusterViewEx(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SignalClusterViewEx(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        Resources res = getResources();
        mMobileSignalGroupEndPadding =
                res.getDimensionPixelSize(R.dimen.mobile_signal_group_end_padding);
        mMobileDataIconStartPadding =
                res.getDimensionPixelSize(R.dimen.mobile_data_icon_start_padding);
        mWideTypeIconStartPadding = res.getDimensionPixelSize(R.dimen.wide_type_icon_start_padding);
        mSecondaryTelephonyPadding = res.getDimensionPixelSize(R.dimen.secondary_telephony_padding);
        mEndPadding = res.getDimensionPixelSize(R.dimen.signal_cluster_battery_padding);
        mEndPaddingNothingVisible = res.getDimensionPixelSize(
                R.dimen.no_signal_cluster_battery_padding);

        TypedValue typedValue = new TypedValue();
        res.getValue(R.dimen.status_bar_icon_scale_factor, typedValue, true);
        mIconScaleFactor = typedValue.getFloat();
    }

    @Override
    public void onTuningChanged(String key, String newValue) {
        if (!StatusBarIconController.ICON_BLACKLIST.equals(key)) {
            return;
        }
        ArraySet<String> blockList = StatusBarIconController.getIconBlacklist(newValue);
        boolean blockAirplane = blockList.contains(SLOT_AIRPLANE);
        boolean blockMobile = blockList.contains(SLOT_MOBILE);
        boolean blockWifi = blockList.contains(SLOT_WIFI);
        boolean blockEthernet = blockList.contains(SLOT_ETHERNET);

        if (blockAirplane != mBlockAirplane || blockMobile != mBlockMobile
                || blockEthernet != mBlockEthernet || blockWifi != mBlockWifi) {
            mBlockAirplane = blockAirplane;
            mBlockMobile = blockMobile;
            mBlockEthernet = blockEthernet;
            mBlockWifi = blockWifi;
            // Re-register to get new callbacks.
            mNC.removeSignalCallback(this);
            mNC.addSignalCallback(this);
        }
    }

    public void setNetworkController(NetworkControllerImpl nc) {
        if (DEBUG) Log.d(TAG, "NetworkController=" + nc);
        mNC = nc;
    }

    public void setSecurityController(SecurityController sc) {
        if (DEBUG) Log.d(TAG, "SecurityController=" + sc);
        mSC = sc;
        mSC.addCallback(this);
        mVpnVisible = mSC.isVpnEnabled();
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mVpn            = (ImageView) findViewById(R.id.vpn);
        mEthernetGroup  = (ViewGroup) findViewById(R.id.ethernet_combo);
        mEthernet       = (ImageView) findViewById(R.id.ethernet);
        mEthernetDark   = (ImageView) findViewById(R.id.ethernet_dark);
        mWifiGroup      = (ViewGroup) findViewById(R.id.wifi_combo);
        mWifi           = (ImageView) findViewById(R.id.wifi_signal);
        mWifiDark       = (ImageView) findViewById(R.id.wifi_signal_dark);
        //SPRD: Bug #474463 Add wifi inOut icon in status bar Feature BEG-->
        mWifiIn = (ImageView) findViewById(R.id.wifi_in);
        mWifiOut = (ImageView) findViewById(R.id.wifi_out);
        //<-- Add wifi inOut icon in status bar Feature END
        mAirplane       = (ImageView) findViewById(R.id.airplane);
        mNoSims         = (ImageView) findViewById(R.id.no_sims);
        mNoSimsDark     = (ImageView) findViewById(R.id.no_sims_dark);
        mNoSimsCombo    =             findViewById(R.id.no_sims_combo);
        mWifiAirplaneSpacer =         findViewById(R.id.wifi_airplane_spacer);
        mWifiSignalSpacer =           findViewById(R.id.wifi_signal_spacer);
        // SPRD: Add VoLte icon for bug 509601.
        mVolte          = (ImageView) findViewById(R.id.volte);
        // SPRD: Add HD audio icon in cucc for bug 536924.
        mHdVoice        = (ImageView) findViewById(R.id.hd_voice);
        mMobileSignalGroup = (LinearLayout) findViewById(R.id.mobile_signal_group);

        maybeScaleVpnAndNoSimsIcons();
    }

    /**
     * Extracts the icon off of the VPN and no sims views and maybe scale them by
     * {@link #mIconScaleFactor}. Note that the other icons are not scaled here because they are
     * dynamic. As such, they need to be scaled each time the icon changes in {@link #apply()}.
     */
    private void maybeScaleVpnAndNoSimsIcons() {
        if (mIconScaleFactor == 1.f) {
            return;
        }

        mVpn.setImageDrawable(new ScalingDrawableWrapper(mVpn.getDrawable(), mIconScaleFactor));

        mNoSims.setImageDrawable(
                new ScalingDrawableWrapper(mNoSims.getDrawable(), mIconScaleFactor));
        mNoSimsDark.setImageDrawable(
                new ScalingDrawableWrapper(mNoSimsDark.getDrawable(), mIconScaleFactor));
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();

        for (PhoneState state : mPhoneStates) {
            mMobileSignalGroup.addView(state.mMobileGroup);
        }

        int endPadding = mMobileSignalGroup.getChildCount() > 0 ? mMobileSignalGroupEndPadding : 0;
        mMobileSignalGroup.setPaddingRelative(0, 0, endPadding, 0);

        TunerService.get(mContext).addTunable(this, StatusBarIconController.ICON_BLACKLIST);

        apply();
        applyIconTint();
        mNC.addSignalCallback(this);
    }

    @Override
    protected void onDetachedFromWindow() {
        mMobileSignalGroup.removeAllViews();
        TunerService.get(mContext).removeTunable(this);
        mSC.removeCallback(this);
        mNC.removeSignalCallback(this);

        super.onDetachedFromWindow();
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);

        // Re-run all checks against the tint area for all icons
        applyIconTint();
    }

    // From SecurityController.
    @Override
    public void onStateChanged() {
        post(new Runnable() {
            @Override
            public void run() {
                mVpnVisible = mSC.isVpnEnabled();
                apply();
            }
        });
    }

    @Override
    public void setWifiIndicators(boolean enabled, IconState statusIcon, IconState qsIcon,
            boolean activityIn, boolean activityOut, String description) {
        mWifiVisible = statusIcon.visible && !mBlockWifi;
        mWifiStrengthId = statusIcon.icon;
        mWifiDescription = statusIcon.contentDescription;
        //SPRD: Bug #474463 Add wifi inOut icon in status bar Feature BEG-->
        isWifiIn = activityIn;
        isWifiOut = activityOut;
        //<-- Add wifi inOut icon in status bar Feature END

        apply();
    }

    @Override
    public void setMobileDataIndicators(IconState statusIcon, IconState qsIcon, int statusType,
            int qsType, boolean activityIn, boolean activityOut, String typeContentDescription,
            String description, boolean isWide, int subId) {
        PhoneState state = getState(subId);
        if (state == null) {
            return;
        }
        state.mMobileVisible = statusIcon.visible && !mBlockMobile;
        state.mMobileStrengthId = statusIcon.icon;
        state.mMobileTypeId = statusType;
        state.mMobileDescription = statusIcon.contentDescription;
        state.mMobileTypeDescription = typeContentDescription;
        state.mIsMobileTypeIconWide = statusType != 0 && isWide;
        state.mActivityIn = activityIn;
        state.mActivityOut = activityOut;
        /* SPRD: modify by BUG 607233 @{ */
        int networkType = TelephonyManager.from(mContext).getNetworkType(subId);
        int defaultDataSubId = SubscriptionManager.from(mContext)
                .getDefaultDataSubscriptionId();
        if (mVolteVisible && (subId == defaultDataSubId)
            && !(networkType == TelephonyManager.NETWORK_TYPE_LTE
            ||networkType == TelephonyManager.NETWORK_TYPE_LTE_CA)) {
            Log.d(TAG, "networkType : " + networkType + "; defaultDataSubId : "
                    + defaultDataSubId);
            mVolteIconId = 0;
        } else if (mVolteVisible && (subId == defaultDataSubId)
                && (networkType == TelephonyManager.NETWORK_TYPE_LTE
                || networkType == TelephonyManager.NETWORK_TYPE_LTE_CA)) {
            mVolteIconId = R.drawable.stat_sys_volte;
        }
        /* @} */
        apply();
    }

    @Override
    public void setEthernetIndicators(IconState state) {
        mEthernetVisible = state.visible && !mBlockEthernet;
        mEthernetIconId = state.icon;
        mEthernetDescription = state.contentDescription;

        apply();
    }

    @Override
    public void setNoSims(boolean show) {
        mNoSimsVisible = show && !mBlockMobile;
        apply();
    }

    @Override
    public void setSubs(List<SubscriptionInfo> subs) {
        if (hasCorrectSubs(subs) && !mShowNoSims) {
            return;
        }
        // Clear out all old subIds.
        for (PhoneState state : mPhoneStates) {
            if (state.mMobile != null) {
                state.maybeStopAnimatableDrawable(state.mMobile);
            }
            if (state.mMobileDark != null) {
                state.maybeStopAnimatableDrawable(state.mMobileDark);
            }
        }
        mPhoneStates.clear();
        if (mMobileSignalGroup != null) {
            mMobileSignalGroup.removeAllViews();
        }
        final int phoneCount = TelephonyManager.from(mContext).getPhoneCount();
        //SPRD: modify by BUG 606106
        boolean isImsRegistSubIdValid = false;
        for (int i = 0; i < phoneCount; i++) {
            SubscriptionInfo subInfo = findRecordByPhoneId(subs, i);
            int subId = subInfo != null ? subInfo.getSubscriptionId()
                    : SubscriptionManager.INVALID_SUBSCRIPTION_ID - i;
            if (SubscriptionManager.isValidSubscriptionId(subId)) {
                inflatePhoneState(subId);
                if (subId == mImsRegisterSubId) {
                    isImsRegistSubIdValid = true;
                }
            } else {
                if (mShowNoSims) {
                    inflatePhoneState(subId);
                }
            }
        }
        /* SPRD: modify by BUG 606106 @{ */
        if (!isImsRegistSubIdValid && mVolteVisible) {
            Log.d(TAG, "refresh Volte Icon if SIM InValid");
            mVolteVisible = false;
            mVolteIconId = 0;
            apply();
        }
        /* @} */
        if (isAttachedToWindow()) {
            applyIconTint();
        }
    }

    /* SPRD: modify by bug474984 @{ */
    private SubscriptionInfo findRecordByPhoneId(List<SubscriptionInfo> subs, int phoneId) {
        if (subs != null) {
            final int length = subs.size();
            for (int i=0 ;i <length ; i++ ) {
                final SubscriptionInfo sir = subs.get(i);
                if (sir.getSimSlotIndex() == phoneId) {
                    return sir;
                }
            }
        }
        return null;
    }
    /* @} */

    private boolean hasCorrectSubs(List<SubscriptionInfo> subs) {
        final int N = subs.size();
        if (N != mPhoneStates.size()) {
            return false;
        }
        for (int i = 0; i < N; i++) {
            if (mPhoneStates.get(i).mSubId != subs.get(i).getSubscriptionId()) {
                return false;
            }
        }
        return true;
    }

    private PhoneState getState(int subId) {
        for (PhoneState state : mPhoneStates) {
            if (state.mSubId == subId) {
                return state;
            }
        }
        Log.e(TAG, "Unexpected subscription " + subId);
        return null;
    }

    private PhoneState inflatePhoneState(int subId) {
        PhoneState state = new PhoneState(subId, mContext);
        int phoneId = SubscriptionManager.getPhoneId(subId);
        if (mContext.getResources().getBoolean(R.bool.config_show_stat_sys_card_id)) {
            if (SubscriptionManager.isValidSlotId(phoneId)) {
                state.mMobileCardId = TelephonyIconsEx.SIM_CARD_ID[phoneId];
            } else if (subId == -1) {
                state.mMobileCardId = TelephonyIconsEx.SIM_CARD_ID[0];
            } else if (subId == -2) {
                state.mMobileCardId = TelephonyIconsEx.SIM_CARD_ID[1];
            }
        }
        if (mMobileSignalGroup != null) {
            mMobileSignalGroup.addView(state.mMobileGroup);
        }
        if (mShowNoSims && !SubscriptionManager.isValidSubscriptionId(subId)) {
            state.mMobileVisible = true;
            state.mMobileStrengthId = R.drawable.stat_sys_no_sims;
        }
        mPhoneStates.add(state);
        return state;
    }

    @Override
    public void setMobileVolteIndicators(boolean show, int subId, int resId) {
        mVolteVisible = show;
        mVolteIconId = resId;
        /* SPRD: modify by BUG 606106 @{ */
        if (show) {
            mImsRegisterSubId = SubscriptionManager.from(mContext)
                    .getDefaultDataSubscriptionId();
        }
        /* @} */
        apply();
    }

    @Override
    public void setMobileHdVoiceIndicators(boolean show, int subId, int resId) {
        mHdVoiceVisible = show;
        mHdVoiceIconId = resId;
        apply();
    }

    @Override
    public void setMobileRoamingIndicators(boolean show, int subId, int resId) {
        PhoneState state = getState(subId);
        if (state == null) {
            return;
        }
        state.mMobileRoamId = resId;
    }

    @Override
    public void setMobileDataConnectedIndicators(boolean show, int subId) {
        PhoneState state = getState(subId);
        if (state == null) {
            return;
        }
        state.mDataConnected = show;
    }

    @Override
    public void setIsAirplaneMode(IconState icon) {
        mIsAirplaneMode = icon.visible && !mBlockAirplane;
        mAirplaneIconId = icon.icon;
        mAirplaneContentDescription = icon.contentDescription;

        apply();
    }

    @Override
    public void setMobileDataEnabled(boolean enabled) {
        // Don't care.
    }

    @Override
    public boolean dispatchPopulateAccessibilityEventInternal(AccessibilityEvent event) {
        // Standard group layout onPopulateAccessibilityEvent() implementations
        // ignore content description, so populate manually
        if (mEthernetVisible && mEthernetGroup != null &&
                mEthernetGroup.getContentDescription() != null)
            event.getText().add(mEthernetGroup.getContentDescription());
        if (mWifiVisible && mWifiGroup != null && mWifiGroup.getContentDescription() != null)
            event.getText().add(mWifiGroup.getContentDescription());
        for (PhoneState state : mPhoneStates) {
            state.populateAccessibilityEvent(event);
        }
        return super.dispatchPopulateAccessibilityEventInternal(event);
    }

    @Override
    public void onRtlPropertiesChanged(int layoutDirection) {
        super.onRtlPropertiesChanged(layoutDirection);

        if (mEthernet != null) {
            mEthernet.setImageDrawable(null);
            mEthernetDark.setImageDrawable(null);
            mLastEthernetIconId = -1;
        }

        if (mWifi != null) {
            mWifi.setImageDrawable(null);
            mWifiDark.setImageDrawable(null);
            mLastWifiStrengthId = -1;
        }

        /* SPRD: Add VoLte icon for bug 509601. @{ */
        if (mVolte != null) {
            mVolte.setImageDrawable(null);
            mLastVolteIconId = -1;
        }
        /* @} */

        /* SPRD: Add HD audio icon in cucc for bug 536924. @{ */
        if (mHdVoice != null) {
            mHdVoice.setImageDrawable(null);
            mLastHdVoiceIconId = -1;
        }
        /* @} */

        for (PhoneState state : mPhoneStates) {
            if (state.mMobile != null) {
                state.maybeStopAnimatableDrawable(state.mMobile);
                state.mMobile.setImageDrawable(null);
                state.mLastMobileStrengthId = -1;
            }
            if (state.mMobileDark != null) {
                state.maybeStopAnimatableDrawable(state.mMobileDark);
                state.mMobileDark.setImageDrawable(null);
                state.mLastMobileStrengthId = -1;
            }
            if (state.mMobileType != null) {
                state.mMobileType.setImageDrawable(null);
                state.mLastMobileTypeId = -1;
            }
        }

        if (mAirplane != null) {
            mAirplane.setImageDrawable(null);
            mLastAirplaneIconId = -1;
        }

        apply();
    }

    @Override
    public boolean hasOverlappingRendering() {
        return false;
    }

    // Run after each indicator change.
    private void apply() {
        if (mWifiGroup == null) return;

        mVpn.setVisibility(mVpnVisible ? View.VISIBLE : View.GONE);
        if (DEBUG) Log.d(TAG, String.format("vpn: %s", mVpnVisible ? "VISIBLE" : "GONE"));

        if (mEthernetVisible) {
            if (mLastEthernetIconId != mEthernetIconId) {
                setIconForView(mEthernet, mEthernetIconId);
                setIconForView(mEthernetDark, mEthernetIconId);
                mLastEthernetIconId = mEthernetIconId;
            }
            mEthernetGroup.setContentDescription(mEthernetDescription);
            mEthernetGroup.setVisibility(View.VISIBLE);
        } else {
            mEthernetGroup.setVisibility(View.GONE);
        }

        if (DEBUG) Log.d(TAG,
                String.format("ethernet: %s",
                    (mEthernetVisible ? "VISIBLE" : "GONE")));

        if (mWifiVisible) {
            //SPRD: Bug #474463 Add wifi inOut icon in status bar Feature BEG-->
            mWifiIn.setVisibility(isWifiIn ? View.VISIBLE : View.INVISIBLE);
            mWifiOut.setVisibility(isWifiOut ? View.VISIBLE: View.INVISIBLE);
            //<-- Add wifi inOut icon in status bar Feature END
            if (mWifiStrengthId != mLastWifiStrengthId) {
                setIconForView(mWifi, mWifiStrengthId);
                setIconForView(mWifiDark, mWifiStrengthId);
                mLastWifiStrengthId = mWifiStrengthId;
            }
            mWifiGroup.setContentDescription(mWifiDescription);
            mWifiGroup.setVisibility(View.VISIBLE);
        } else {
            mWifiGroup.setVisibility(View.GONE);
        }

        if (DEBUG) Log.d(TAG,
                String.format("wifi: %s sig=%d",
                    (mWifiVisible ? "VISIBLE" : "GONE"),
                    mWifiStrengthId));

        boolean anyMobileVisible = false;
        int firstMobileTypeId = 0;
        for (PhoneState state : mPhoneStates) {
            if (state.apply(anyMobileVisible)) {
                if (!anyMobileVisible) {
                    firstMobileTypeId = state.mMobileTypeId;
                    anyMobileVisible = true;
                }
            }
        }

        /* SPRD: Add VoLte icon for bug 509601. @{ */
        if (mVolteVisible && !mIsAirplaneMode && mVolteIconId != 0
                && mShowVolteIcon) {
            if (mLastVolteIconId != mVolteIconId) {
                mVolte.setImageResource(mVolteIconId);
                mLastVolteIconId = mVolteIconId;
            }
            mVolte.setVisibility(View.VISIBLE);
        } else {
            mVolte.setVisibility(View.GONE);
        }
        /* @} */

        /* SPRD: Add HD audio icon in cucc for bug 536924. @{ */
        if (mHdVoiceVisible && !mIsAirplaneMode && mHdVoiceIconId != 0) {
            if (mLastHdVoiceIconId != mHdVoiceIconId) {
                mHdVoice.setImageResource(mHdVoiceIconId);
                mLastHdVoiceIconId = mHdVoiceIconId;
            }
            mHdVoice.setVisibility(View.VISIBLE);
        } else {
            mHdVoice.setVisibility(View.GONE);
        }
        /* @} */

        if (mIsAirplaneMode) {
            if (mLastAirplaneIconId != mAirplaneIconId) {
                setIconForView(mAirplane, mAirplaneIconId);
                mLastAirplaneIconId = mAirplaneIconId;
            }
            mAirplane.setContentDescription(mAirplaneContentDescription);
            mAirplane.setVisibility(View.VISIBLE);
        } else {
            mAirplane.setVisibility(View.GONE);
        }

        if (mIsAirplaneMode && mWifiVisible) {
            mWifiAirplaneSpacer.setVisibility(View.VISIBLE);
        } else {
            mWifiAirplaneSpacer.setVisibility(View.GONE);
        }

        if (((anyMobileVisible && firstMobileTypeId != 0) || mNoSimsVisible) && mWifiVisible) {
            mWifiSignalSpacer.setVisibility(View.VISIBLE);
        } else {
            mWifiSignalSpacer.setVisibility(View.GONE);
        }
        if (mShowNoSims) {
            mNoSimsCombo.setVisibility(View.GONE);
        } else {
            /* SPRD: modify by BUG 624887 @{ */
            mNoSimsCombo.setVisibility((!mIsAirplaneMode && mNoSimsVisible)
                    ? View.VISIBLE : View.GONE);
            /* @} */
        }

        boolean anythingVisible = mNoSimsVisible || mWifiVisible || mIsAirplaneMode
                || anyMobileVisible || mVpnVisible || mEthernetVisible;
        setPaddingRelative(0, 0, anythingVisible ? mEndPadding : mEndPaddingNothingVisible, 0);
    }

    /**
     * Sets the given drawable id on the view. This method will also scale the icon by
     * {@link #mIconScaleFactor} if appropriate.
     */
    private void setIconForView(ImageView imageView, @DrawableRes int iconId) {
        // Using the imageView's context to retrieve the Drawable so that theme is preserved.
        Drawable icon = imageView.getContext().getDrawable(iconId);

        if (mIconScaleFactor == 1.f) {
            imageView.setImageDrawable(icon);
        } else {
            imageView.setImageDrawable(new ScalingDrawableWrapper(icon, mIconScaleFactor));
        }
    }

    public void setIconTint(int tint, float darkIntensity, Rect tintArea) {
        boolean changed = tint != mIconTint || darkIntensity != mDarkIntensity
                || !mTintArea.equals(tintArea);
        mIconTint = tint;
        mDarkIntensity = darkIntensity;
        mTintArea.set(tintArea);
        if (changed && isAttachedToWindow()) {
            applyIconTint();
        }
    }

    private void applyIconTint() {
        setTint(mVpn, StatusBarIconController.getTint(mTintArea, mVpn, mIconTint));
        setTint(mAirplane, StatusBarIconController.getTint(mTintArea, mAirplane, mIconTint));
        /* SPRD: modify for bug 607553. @{ */
        setTint(mNoSims, StatusBarIconController.getTint(mTintArea, mNoSims, mIconTint));
        //applyDarkIntensity(
                //StatusBarIconController.getDarkIntensity(mTintArea, mNoSims, mDarkIntensity),
                //mNoSims, mNoSimsDark);
        /* @} */
        applyDarkIntensity(
                StatusBarIconController.getDarkIntensity(mTintArea, mWifi, mDarkIntensity),
                mWifi, mWifiDark);
        applyDarkIntensity(
                StatusBarIconController.getDarkIntensity(mTintArea, mEthernet, mDarkIntensity),
                mEthernet, mEthernetDark);
        for (int i = 0; i < mPhoneStates.size(); i++) {
            mPhoneStates.get(i).setIconTint(mIconTint, mDarkIntensity, mTintArea);
        }
    }

    private void applyDarkIntensity(float darkIntensity, View lightIcon, View darkIcon) {
        lightIcon.setAlpha(1 - darkIntensity);
        darkIcon.setAlpha(darkIntensity);
    }

    private void setTint(ImageView v, int tint) {
        v.setImageTintList(ColorStateList.valueOf(tint));
    }

    private class PhoneState {
        private final int mSubId;
        private boolean mMobileVisible = false;
        private int mMobileStrengthId = 0, mMobileTypeId = 0, mMobileDataInOutId = 0,
                mMobileCardId = 0, mMobileRoamId = 0;
        private int mLastMobileStrengthId = -1;
        private int mLastMobileTypeId = -1;
        private boolean mIsMobileTypeIconWide, mActivityIn, mActivityOut,
                mDataConnected;
        private String mMobileDescription, mMobileTypeDescription;

        private ViewGroup mMobileGroup;
        private ImageView mMobile, mMobileDark, mMobileType, mMobileDataInOut, mMobileCard,
                mMobileRoam;
        // SPRD: add for bug 610587
        private View mMobileTypeSpacer;

        public PhoneState(int subId, Context context) {
            ViewGroup root = (ViewGroup) LayoutInflater.from(context)
                    .inflate(R.layout.mobile_signal_group, null);
            setViews(root);
            mSubId = subId;
        }

        public void setViews(ViewGroup root) {
            mMobileGroup    = root;
            mMobile         = (ImageView) root.findViewById(R.id.mobile_signal);
            mMobileDark     = (ImageView) root.findViewById(R.id.mobile_signal_dark);
            mMobileType     = (ImageView) root.findViewById(R.id.mobile_type);
            mMobileDataInOut = (ImageView) root.findViewById(R.id.mobile_data_in_out);
            mMobileCard = (ImageView) root.findViewById(R.id.mobile_card);
            mMobileRoam = (ImageView) root.findViewById(R.id.mobile_roam_type);
            // SPRD: add for bug 610587
            mMobileTypeSpacer = (View)root.findViewById(R.id.mobile_type_spacer);
        }

        public boolean apply(boolean isSecondaryIcon) {
            if (mMobileVisible && !mIsAirplaneMode) {
                if (mLastMobileStrengthId != mMobileStrengthId) {
                    updateAnimatableIcon(mMobile, mMobileStrengthId);
                    updateAnimatableIcon(mMobileDark, mMobileStrengthId);
                    mLastMobileStrengthId = mMobileStrengthId;
                }

                if (mLastMobileTypeId != mMobileTypeId) {
                    mMobileType.setImageResource(mMobileTypeId);
                    mLastMobileTypeId = mMobileTypeId;
                }

                /* SPRD: FEATURE_SHOW_SPREADTRUM_SIGNAL_CLUSTER_VIEW @{ */
                if (mDataConnected) {
                    if (mActivityIn && mActivityOut) {
                        mMobileDataInOutId = TelephonyIconsEx.ICON_STAT_SYS_DATA_INOUT;
                    } else if (mActivityIn) {
                        mMobileDataInOutId =  TelephonyIconsEx.ICON_STAT_SYS_DATA_IN;
                    } else if (mActivityOut) {
                        mMobileDataInOutId = TelephonyIconsEx.ICON_STAT_SYS_DATA_OUT;
                    } else {
                        mMobileDataInOutId =TelephonyIconsEx.ICON_STAT_SYS_DATA_DEFAULT;
                    }
                } else {
                    mMobileDataInOutId = 0;
                }
                mMobileRoam.setImageResource(mMobileRoamId);
                mMobileDataInOut.setImageResource(mMobileDataInOutId);
                mMobileCard.setImageResource(mMobileCardId);
                /* @} */
                mMobileGroup.setContentDescription(mMobileTypeDescription
                        + " " + mMobileDescription);
                mMobileGroup.setVisibility(View.VISIBLE);
            } else {
                mMobileGroup.setVisibility(View.GONE);
            }

            // When this isn't next to wifi, give it some extra padding between the signals.
            mMobileGroup.setPaddingRelative(isSecondaryIcon ? mSecondaryTelephonyPadding : 0,
                    0, 0, 0);
            mMobile.setPaddingRelative(
                    mIsMobileTypeIconWide ? mWideTypeIconStartPadding : mMobileDataIconStartPadding,
                    0, 0, 0);
            mMobileDark.setPaddingRelative(
                    mIsMobileTypeIconWide ? mWideTypeIconStartPadding : mMobileDataIconStartPadding,
                    0, 0, 0);
            // SPRD: Set the tint of icons for bug 591009.
            mMobileDark.setVisibility(View.GONE);
            mMobileCard.setVisibility(mMobileCardId != 0 ? View.VISIBLE : View.GONE);

            if (DEBUG) Log.d(TAG, String.format("mobile: %s sig=%d typ=%d",
                        (mMobileVisible ? "VISIBLE" : "GONE"), mMobileStrengthId, mMobileTypeId));

            mMobileType.setVisibility(mMobileTypeId != 0 ? View.VISIBLE : View.GONE);
            // SPRD: add for bug 610587
            mMobileTypeSpacer.setVisibility(mMobileTypeId != 0 ? View.VISIBLE : View.GONE);
            mMobileRoam.setVisibility(mMobileRoamId != 0 ? View.VISIBLE : View.GONE);

            return mMobileVisible;
        }

        private void updateAnimatableIcon(ImageView view, int resId) {
            maybeStopAnimatableDrawable(view);
            setIconForView(view, resId);
            maybeStartAnimatableDrawable(view);
        }

        private void maybeStopAnimatableDrawable(ImageView view) {
            Drawable drawable = view.getDrawable();

            // Check if the icon has been scaled. If it has retrieve the actual drawable out of the
            // wrapper.
            if (drawable instanceof ScalingDrawableWrapper) {
                drawable = ((ScalingDrawableWrapper) drawable).getDrawable();
            }

            if (drawable instanceof Animatable) {
                Animatable ad = (Animatable) drawable;
                if (ad.isRunning()) {
                    ad.stop();
                }
            }
        }

        private void maybeStartAnimatableDrawable(ImageView view) {
            Drawable drawable = view.getDrawable();

            // Check if the icon has been scaled. If it has retrieve the actual drawable out of the
            // wrapper.
            if (drawable instanceof ScalingDrawableWrapper) {
                drawable = ((ScalingDrawableWrapper) drawable).getDrawable();
            }

            if (drawable instanceof Animatable) {
                Animatable ad = (Animatable) drawable;
                if (ad instanceof AnimatedVectorDrawable) {
                    ((AnimatedVectorDrawable) ad).forceAnimationOnUI();
                }
                if (!ad.isRunning()) {
                    ad.start();
                }
            }
        }

        public void populateAccessibilityEvent(AccessibilityEvent event) {
            if (mMobileVisible && mMobileGroup != null
                    && mMobileGroup.getContentDescription() != null) {
                event.getText().add(mMobileGroup.getContentDescription());
            }
        }

        /* SPRD: Set the tint of icons for bug 591009. @{ */
        public void setIconTint(int tint, float darkIntensity, Rect tintArea) {
            /* applyDarkIntensity(
                    StatusBarIconController.getDarkIntensity(tintArea, mMobile, darkIntensity),
                    mMobile, mMobileDark); */
            /* SPRD: Set statusbar icon tint except CUCC no sim icon. . @{ */
            if (mIsSetMobileIconTint || mMobileStrengthId != R.drawable.stat_sys_no_sims) {
                setTint(mMobile, StatusBarIconController.getTint(tintArea, mMobileType, tint));
            }
            /* @} */
            setTint(mMobileType, StatusBarIconController.getTint(tintArea, mMobileType, tint));
            setTint(mMobileDataInOut, StatusBarIconController.getTint(tintArea, mMobileType, tint));
            setTint(mMobileCard, StatusBarIconController.getTint(tintArea, mMobileType, tint));
            setTint(mMobileRoam, StatusBarIconController.getTint(tintArea, mMobileType, tint));
            setTint(mWifiOut, StatusBarIconController.getTint(tintArea, mMobileType, tint));
            setTint(mWifiIn, StatusBarIconController.getTint(tintArea, mMobileType, tint));
            setTint(mVolte, StatusBarIconController.getTint(tintArea, mMobileType, tint));
            setTint(mHdVoice, StatusBarIconController.getTint(tintArea, mMobileType, tint));
        }
        /* @} */
    }
}

