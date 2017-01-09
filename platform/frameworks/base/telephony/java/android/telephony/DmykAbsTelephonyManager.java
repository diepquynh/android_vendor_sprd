
package com.dmyk.android.telephony;

import android.content.Context;
import android.net.Uri;
import android.telephony.TelephonyManager;
import android.util.Log;

public abstract class DmykAbsTelephonyManager {

    protected static DmykAbsTelephonyManager mInstance;
    static final String TAG = "DmykAbsTelephonyManager";
    protected Context mContext;

    private DmykAbsTelephonyManager() {
        mContext = null;
    }

    protected DmykAbsTelephonyManager(Context context) {
        Context appContext = context.getApplicationContext();
        if (appContext != null) {
            mContext = appContext;
        } else {
            mContext = context;
        }
    }

    public static DmykAbsTelephonyManager getDefault(Context context) {
        synchronized (DmykAbsTelephonyManager.class) {
            if (mInstance == null) {
                mInstance = new DmykTelephonyManager(context);
            } else {
                Log.wtf(TAG, "called multiple times!  mInstance = " + mInstance);
            }
        }
        return mInstance;
    }

    /**
     * Data connection state: Unknown. Used before we know the state.
     */
    public static final int DATA_UNKNOWN = -1;
    /** Data connection state: Disconnected. IP traffic not available. */
    public static final int DATA_DISCONNECTED = 0;
    /** Data connection state: Currently setting up a data connection. */
    public static final int DATA_CONNECTING = 1;
    /** Data connection state: Connected. IP traffic should be available. */
    public static final int DATA_CONNECTED = 2;
    /**
     * Data connection state: Suspended. The connection is up, but IP traffic is temporarily
     * unavailable. For example, in a 2G network, data activity may be suspended when a voice call
     * arrives.
     */
    public static final int DATA_SUSPENDED = 3;

    /**
     * SIM card state: Unknown. Signifies that the SIM is in transition between states. For example,
     * when the user inputs the SIM pin under PIN_REQUIRED state, a query for sim status returns
     * this state before turning to SIM_STATE_READY. These are the ordinal value of
     * IccCardConstants.State.
     */
    public static final int SIM_STATE_UNKNOWN = 0;
    /** SIM card state: no SIM card is available in the device */
    public static final int SIM_STATE_ABSENT = 1;
    /** SIM card state: Locked: requires the user's SIM PIN to unlock */
    public static final int SIM_STATE_PIN_REQUIRED = 2;
    /** SIM card state: Locked: requires the user's SIM PUK to unlock */
    public static final int SIM_STATE_PUK_REQUIRED = 3;
    /** SIM card state: Locked: requires a network PIN to unlock */
    public static final int SIM_STATE_NETWORK_LOCKED = 4;
    /** SIM card state: Ready */
    public static final int SIM_STATE_READY = 5;
    /** SIM card state: not ready */
    public static final int SIM_STATE_NOT_READY = 6;
    /** SIM card state: disabled for ever */
    public static final int SIM_STATE_PERM_DISABLED = 7;
    /** SIM card state: IO error */
    public static final int SIM_STATE_CARD_IO_ERROR = 8;

    /** Device type : Unknown.Undefined device type. */
    public static final int DEVICE_TYPE_UNKNOWN = 0;
    /** Device type : Cellphone.the device is a Mobile phone */
    public static final int DEVICE_TYPE_CELLPHONE = 1;
    /** Device type : Pad.the device is a tablet computer */
    public static final int DEVICE_TYPE_PAD = 2;
    /** Device type : Stb.the device is a set-top box */
    public static final int DEVICE_TYPE_STB = 3;
    /** Device type : Watch.the device is watch */
    public static final int DEVICE_TYPE_WATCH = 4;
    /** Device type : Bracelet.the device is bracelet */
    public static final int DEVICE_TYPE_BRACELET = 5;
    /** VoLTE switch state: slot0 VoLTE state*/
    public static final String VOLTE_DMYK_STATE_0 = "volte_dmyk_state_0";
    /** VoLTE switch state: slot1 VoLTE state*/
    public static final String VOLTE_DMYK_STATE_1 = "volte_dmyk_state_1";
    /** VoLTE switch state: VoLTE state on*/
    public static final int VOLTE_STATE_ON = 1;
    /** VoLTE switch state: VoLTE state off*/
    public static final int VOLTE_STATE_OFF = 0;
    /** VoLTE switch state: getVoLTEState(int phoneId) return this value
     * when this phone not support VoLTE on not support dual VoLTE */
    public static final int VOLTE_STATE_UNKNOWN = -1;
    /**Broadcast action: used when VoLTE state change */
    public static final String ACTION_VOLTE_STATE_CHANGE =
            "com.dmyk.android.telephony.action.VOLTE_STATE_CHANGE";
    /**Broadcast action: used when VoLTE state change */
    public static final String ACTION_VOLTE_STATE_SETTING =
            "com.dmyk.android.telephony.action.VOLTE_STATE_SETTING";
    /**Broadcast action: used when APN state change */
    public static final String ACTION_APN_STATE_CHANGE =
            "com.dmyk.android.telephony.action.APN_STATE_CHANGE";
    /**Broadcast action: SIM state change when hot plug */
    public static final String ACTION_SIM_STATE_CHANGED =
            "com.dmyk.android.telephony.action.SIM_STATE_CHANGED";
    /**Extras: deliver phone id */
    public static final String EXTRA_SIM_PHONEID =
            "com.dmyk.android.telephony.extra.SIM_PHONEID";
    /**Extras: deliver sim state */
    public static final String EXTRA_SIM_STATE =
            "com.dmyk.android.telephony.extra.SIM_STATE";

    public abstract int getPhoneCount();

    public abstract String getGsmDeviceId(int phoneId);

    public abstract String getCdmaDeviceId();

    public abstract String getSubscriberId(int phoneId);

    public abstract String getIccId(int phoneId);

    public abstract int getDataState(int phoneId);

    public abstract int getSimState(int phoneId);

    public abstract int getNetworkType(int phoneId);

    public abstract String getDeviceSoftwareVersion();

    public abstract int getDeviceType();

    public abstract int getMasterPhoneId();

    public abstract boolean isInternationalNetworkRoaming(int phoneId);

    public abstract int getVoLTEState(int phoneId);

    public abstract Uri getAPNContentUri(int phoneId);

    public abstract int getSlotId (int phoneId);

    public abstract int getCellId (int phoneId);

    public abstract int getLac (int phoneId);
}
