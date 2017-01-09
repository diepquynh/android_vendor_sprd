/*
 * SPRD: FEATURE_SHOW_CARRIER_LABEL_IN_PANEL
 *
 */

package com.android.keyguard;

import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.HashMap;


import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.telephony.ServiceState;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.util.Log;
import android.view.View;
import android.view.Gravity;
import android.widget.TextView;
import android.view.LayoutInflater;
import android.widget.LinearLayout;

import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.IccCardConstants.State;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TelephonyIntents;
import com.android.settingslib.WirelessUtils;
import com.sprd.keyguard.KeyguardPluginsHelper;

public class CarrierTextLayout extends LinearLayout {
    private static final boolean DEBUG = /*KeyguardConstants.DEBUG*/ true;
    private static final String TAG = "CarrierTextLayout";

    private static CharSequence mSeparator;

    private final boolean mIsEmergencyCallCapable, mShowSpnAppendPlmn;

    private KeyguardUpdateMonitor mKeyguardUpdateMonitor;

    private WifiManager mWifiManager;
    private TelephonyManager mTelephonyManager;
    HashMap<Integer, ServiceState> mServiceStates = new HashMap<Integer, ServiceState>();
    private IntentFilter mIntentFilter;
    private ServiceStateChangeReciver mServiceStateChangeReciver;
    private ServiceState mServiceState;

    private KeyguardUpdateMonitorCallback mCallback = new KeyguardUpdateMonitorCallback() {
        @Override
        public void onRefreshCarrierInfo() {
            updateCarrierText();
        }

        public void onFinishedGoingToSleep(int why) {
            setSelected(false);
        };

        public void onStartedWakingUp() {
            setSelected(true);
        };
    };

    /**
     * The status of this lock screen. Primarily used for widgets on LockScreen.
     */
    private static enum StatusMode {
        Normal, // Normal case (sim card present, it's not locked)
        NetworkLocked, // SIM card is 'network locked'.
        SimMissing, // SIM card is missing.
        SimMissingLocked, // SIM card is missing, and device isn't provisioned; don't allow access
        SimPukLocked, // SIM card is PUK locked because SIM entered wrong too many times
        SimLocked, // SIM card is currently locked
        SimPermDisabled, // SIM card is permanently disabled due to PUK unlock failure
        SimNotReady; // SIM is not ready yet. May never be on devices w/o a SIM.
    }

    public CarrierTextLayout(Context context) {
        this(context, null);
    }

    public CarrierTextLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mIsEmergencyCallCapable = context.getResources().getBoolean(
                com.android.internal.R.bool.config_voice_capable);

        mTelephonyManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        mWifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        /* SPRD: FEATURE_SHOW_SPN_APPEND_PLMN @{ */
        mShowSpnAppendPlmn = mContext.getResources().getBoolean(
                R.bool.config_showSpnAppendPlmn);
        Log.d(TAG, "mShowSpnAppendPlmn : " + mShowSpnAppendPlmn);
        /* @} */

    }

    protected void updateCarrierText() {
        boolean allSimsMissing = true;
        boolean anySimReadyAndInService = false;
        //CharSequence displayText = null;
        int count = mTelephonyManager.getPhoneCount();
        CharSequence[] displayText = new CharSequence[count];
        /* SPRD: modify by BUG 601753 @{ */
        boolean showEmergencyOnly = mContext.getResources().getBoolean(
                com.android.internal.R.bool.config_showEmergencyCallOnly);
        CharSequence emergencyCall = getContext()
                .getText(com.android.internal.R.string.emergency_calls_only);
        CharSequence noService = getContext()
                .getText(com.android.internal.R.string.lockscreen_carrier_default);
        /* @} */
        // SPRD: modify by BUG 532196
        boolean isShowFlightMode = mContext.getResources().getBoolean(
                R.bool.config_show_airplane_mode);
        // SPRD: modify for bug643004
        List<SubscriptionInfo> subs = mKeyguardUpdateMonitor.getSubscriptionInfo(true);
        final int N = subs.size();
        if (DEBUG) Log.d(TAG, "updateCarrierText(): " + N);
        /* SPRD: modify by BUG 613319 @{ */
        try{
            for (int i = 0; i < N; i++) {
                int subId = subs.get(i).getSubscriptionId();
                mServiceState = mKeyguardUpdateMonitor.mServiceStates.get(subId);
                if (mServiceState == null) {
                    int phoneId = SubscriptionManager.getPhoneId(subId);
                    mServiceState = mServiceStates.get(phoneId);
                }
                State simState = mKeyguardUpdateMonitor.getSimState(subId);
                CharSequence carrierName = formatCarrierName(subs.get(i).getCarrierName());
                if (carrierName != null && !showEmergencyOnly && carrierName.toString()
                    .contains(emergencyCall)) {
                    carrierName = carrierName.toString().replace(emergencyCall,noService);
                }
                CharSequence carrierTextForSimState = getCarrierTextForSimState(simState, carrierName);
                if (State.UNKNOWN == simState) {
                    String simlock = KeyguardSupportSimlock.getInstance().getSimLockStatusString(SubscriptionManager.getSlotId(subId));
                    if (simlock != null) {
                        carrierTextForSimState = makeCarrierStringOnEmergencyCapable(simlock, carrierName);
                    }
                }
                if (DEBUG) {
                    Log.d(TAG, "Handling (subId=" + subId + "): " + simState + " " + carrierName);
                }
                if (carrierTextForSimState != null) {
                    allSimsMissing = false;
                    displayText[i] = concatenate(displayText[i], carrierTextForSimState);
                }
                if (simState == IccCardConstants.State.READY) {
                    if (mServiceState != null && mServiceState.getDataRegState()
                            == ServiceState.STATE_IN_SERVICE) {
                        // hack for WFC (IWLAN) not turning off immediately once
                        // Wi-Fi is disassociated or disabled
                        if (mServiceState.getRilDataRadioTechnology() != ServiceState.RIL_RADIO_TECHNOLOGY_IWLAN
                                || (mWifiManager.isWifiEnabled()
                                        && mWifiManager.getConnectionInfo() != null
                                        && mWifiManager.getConnectionInfo().getBSSID() != null)) {
                            if (DEBUG) {
                                Log.d(TAG, "SIM ready and in service: subId=" + subId + ", ss=" + mServiceState);
                            }
                            anySimReadyAndInService = true;
                        }
                    }
                }
            }
            if (allSimsMissing) {
                if (N != 0) {
                    // Shows "No SIM card | Emergency calls only" on devices that are voice-capable.
                    // This depends on mPlmn containing the text "Emergency calls only" when the radio
                    // has some connectivity. Otherwise, it should be null or empty and just show
                    // "No SIM card"
                    // Grab the first subscripton, because they all should contain the emergency text,
                    // described above.
                    displayText[0] =  makeCarrierStringOnEmergencyCapable(
                            getContext().getText(R.string.keyguard_missing_sim_message_short),
                            subs.get(0).getCarrierName());
                } else {
                    // We don't have a SubscriptionInfo to get the emergency calls only from.
                    // Grab it from the old sticky broadcast if possible instead. We can use it
                    // here because no subscriptions are active, so we don't have
                    // to worry about MSIM clashing.
                    //SPRD: modify by BUG 601753
                    CharSequence text = showEmergencyOnly ? emergencyCall : "";
                    displayText[0] =  makeCarrierStringOnEmergencyCapable(
                            getContext().getText(R.string.keyguard_missing_sim_message_short), text);
                }
            }

            // APM (airplane mode) != no carrier state. There are carrier services
            // (e.g. WFC = Wi-Fi calling) which may operate in APM.
            if (!anySimReadyAndInService && WirelessUtils.isAirplaneModeOn(mContext)) {
                /* SPRD: modify by BUG 532196 @{ */
                if (isShowFlightMode) {
                    displayText[0] = getContext().getString(R.string.airplane_mode);
                }
                /* @} */
                // display single carrier label for airplane mode, clear others.
                for (int i = 1; i < N; i++) {
                    displayText[i] = "";
                }
            }
            for (int i = 0; i < count; i++) {
                TextView view = (TextView) getChildAt(i);
                view.setText(displayText[i]);
                view.setVisibility(TextUtils.isEmpty(displayText[i]) ? View.INVISIBLE : View.VISIBLE);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        /* @} */
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mSeparator = getResources().getString(
                com.android.internal.R.string.kg_text_message_separator);

        LayoutInflater inflater = LayoutInflater.from(mContext);
        int N = TelephonyManager.from(mContext).getPhoneCount();
        for (int i = 0; i < N; i++) {
            addTextView();
        }
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        if (ConnectivityManager.from(mContext).isNetworkSupported(
                ConnectivityManager.TYPE_MOBILE)) {
            mKeyguardUpdateMonitor = KeyguardUpdateMonitor.getInstance(mContext);
            mKeyguardUpdateMonitor.registerCallback(mCallback);
            mIntentFilter = new IntentFilter(TelephonyIntents.ACTION_SERVICE_STATE_CHANGED);
            if (mServiceStateChangeReciver == null) {
                mServiceStateChangeReciver = new ServiceStateChangeReciver();
            }
            mContext.registerReceiver(mServiceStateChangeReciver, mIntentFilter);
        } else {
            // Don't listen and clear out the text when the device isn't a phone.
            mKeyguardUpdateMonitor = null;
        }
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        if (mKeyguardUpdateMonitor != null) {
            mKeyguardUpdateMonitor.removeCallback(mCallback);
        }
        if(mServiceStateChangeReciver != null){
            mContext.unregisterReceiver(mServiceStateChangeReciver);
            mServiceStateChangeReciver = null;
        }
    }

    /**
     * Top-level function for creating carrier text. Makes text based on simState, PLMN
     * and SPN as well as device capabilities, such as being emergency call capable.
     *
     * @param simState
     * @param text
     * @param spn
     * @return Carrier text if not in missing state, null otherwise.
     */
    private CharSequence getCarrierTextForSimState(IccCardConstants.State simState,
            CharSequence text) {
        CharSequence carrierText = null;
        StatusMode status = getStatusForIccState(simState);
        switch (status) {
            case Normal:
                //carrierText = text;
                carrierText = KeyguardPluginsHelper.getInstance()
                        .parseOperatorName(mContext,mServiceState,text);
                break;

            case SimNotReady:
                // Null is reserved for denoting missing, in this case we have nothing to display.
                carrierText = ""; // nothing to display yet.
                break;

            case NetworkLocked:
                carrierText = makeCarrierStringOnEmergencyCapable(
                        mContext.getText(R.string.keyguard_network_locked_message), text);
                break;

            case SimMissing:
                carrierText = null;
                break;

            case SimPermDisabled:
                carrierText = getContext().getText(
                        R.string.keyguard_permanent_disabled_sim_message_short);
                break;

            case SimMissingLocked:
                carrierText = null;
                break;

            case SimLocked:
                carrierText = makeCarrierStringOnEmergencyCapable(
                        getContext().getText(R.string.keyguard_sim_locked_message),
                        text);
                break;

            case SimPukLocked:
                carrierText = makeCarrierStringOnEmergencyCapable(
                        getContext().getText(R.string.keyguard_sim_puk_locked_message),
                        text);
                break;
        }

        return carrierText;
    }

    /*
     * Add emergencyCallMessage to carrier string only if phone supports emergency calls.
     */
    private CharSequence makeCarrierStringOnEmergencyCapable(
            CharSequence simMessage, CharSequence emergencyCallMessage) {
        if (mIsEmergencyCallCapable) {
            return concatenate(simMessage, emergencyCallMessage);
        }
        return simMessage;
    }

    /**
     * Determine the current status of the lock screen given the SIM state and other stuff.
     */
    private StatusMode getStatusForIccState(IccCardConstants.State simState) {
        // Since reading the SIM may take a while, we assume it is present until told otherwise.
        if (simState == null) {
            return StatusMode.Normal;
        }

        final boolean missingAndNotProvisioned =
                !KeyguardUpdateMonitor.getInstance(mContext).isDeviceProvisioned()
                && (simState == IccCardConstants.State.ABSENT ||
                        simState == IccCardConstants.State.PERM_DISABLED);

        // Assume we're NETWORK_LOCKED if not provisioned
        simState = missingAndNotProvisioned ? IccCardConstants.State.NETWORK_LOCKED : simState;
        switch (simState) {
            case ABSENT:
                return StatusMode.SimMissing;
            case NETWORK_LOCKED:
                return StatusMode.NetworkLocked;
            case NOT_READY:
                return StatusMode.SimNotReady;
            case PIN_REQUIRED:
                return StatusMode.SimLocked;
            case PUK_REQUIRED:
                return StatusMode.SimPukLocked;
            case READY:
                return StatusMode.Normal;
            case PERM_DISABLED:
                return StatusMode.SimPermDisabled;
            case UNKNOWN:
                return StatusMode.SimMissing;
        }
        return StatusMode.SimMissing;
    }

    private CharSequence concatenate(CharSequence plmn, CharSequence spn) {
        final boolean plmnValid = !TextUtils.isEmpty(plmn);
        final boolean spnValid = !TextUtils.isEmpty(spn);
        if (plmnValid && spnValid) {
            /* SPRD: FEATURE_SHOW_SPN_APPEND_PLMN @{ */
            if (mShowSpnAppendPlmn) {
                return new StringBuilder().append(plmn).append(mSeparator).append(spn)
                        .toString();
            } else {
                return plmn;
            }
            /* @} */
        } else if (plmnValid) {
            return plmn;
        } else if (spnValid) {
            return spn;
        } else {
            return "";
        }
    }

    /* SPRD: FEATURE_SHOW_SPN_APPEND_PLMN @{ */
    private CharSequence formatCarrierName(CharSequence carrierName) {
        CharSequence plmn = carrierName;
        if (mShowSpnAppendPlmn || carrierName == null) return plmn;
        if (plmn.toString().contains(mSeparator)) {
            String[] operators = plmn.toString().split(mSeparator.toString());
            return operators[0];
        }
        return plmn;
    }
    /* @} */

    private void addTextView() {
        TextView view = new TextView(mContext);
        view.setGravity(Gravity.CENTER_HORIZONTAL);
        view.setSingleLine();
        view.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 14);
        view.setTextAppearance(mContext, android.R.attr.textAppearanceMedium);

        addView(view);
    }

    private class ServiceStateChangeReciver extends BroadcastReceiver{
        @Override
        public void onReceive(Context context, Intent intent) {
            if (TelephonyIntents.ACTION_SERVICE_STATE_CHANGED.equals(intent.getAction())) {
                int phoneId = intent.getIntExtra(PhoneConstants.SLOT_KEY,0);
                ServiceState serviceState = ServiceState.newFromBundle(intent.getExtras());
                mServiceStates.put(phoneId, serviceState);
            }
        }
    }
}

