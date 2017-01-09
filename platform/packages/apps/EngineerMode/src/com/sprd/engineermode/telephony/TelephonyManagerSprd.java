
package com.sprd.engineermode.telephony;

import android.os.SystemProperties;
import android.util.Log;
import java.util.EnumSet;
import android.telephony.SubscriptionManager;
import android.content.Context;
import android.telephony.TelephonyManager;
import android.provider.Settings;
import com.android.sprd.telephony.RadioInteractor;

public class TelephonyManagerSprd {

    static final String TAG = "TelephonyManagerSprd";
    static final String MODEM_TYPE = "ro.radio.modemtype";
    private static String PROP_SSDA_MODE = "persist.radio.modem.config";

    // ssda mode
    private static String MODE_SVLTE = "svlte";
    private static String MODE_TDD_CSFB = "TL_TD_G,G";
    private static String MODE_FDD_CSFB = "TL_LF_W_G,G";
    private static String MODE_CSFB = "TL_LF_TD_W_G,G";

    //modem type
    public static final int MODEM_TYPE_GSM = 0;
    public static final int MODEM_TYPE_TDSCDMA = 1;
    public static final int MODEM_TYPE_WCDMA = 2;
    public static final int MODEM_TYPE_LTE = 3;

    private Context mContext;

    private SubscriptionManager mSubscriptionManager;
    private TelephonyManager mTelephonyManager;
    private RadioInteractor mRadioInteractor;

    public TelephonyManagerSprd(Context context) {
        mContext = context;
        mSubscriptionManager = SubscriptionManager.from(mContext);
        mTelephonyManager = TelephonyManager.from(mContext);
        mRadioInteractor = new RadioInteractor(mContext);
    }

    /**
     * Returns the type of modem
     * for 0:GSM;1:TDSCDMA;2:WCDMA;3:LTE
     */
    public static int getModemType() {
        String modeType = SystemProperties.get(MODEM_TYPE, "");
        Log.d(TAG,"getModemType: modemType=" + modeType);
        if ("t".equals(modeType)) {
            return MODEM_TYPE_TDSCDMA;
        } else if ("w".equals(modeType)) {
            return MODEM_TYPE_WCDMA;
        } else if ("tl".equals(modeType) || "lf".equals(modeType) || "l".equals(modeType)) {
            return MODEM_TYPE_LTE;
        } else {
            return MODEM_TYPE_GSM;
        }
    }

    /**
     * Returns the slotId of Primary Card. Returns 1 for the frist slot is Primary Card Returns 2
     * for the second slot is Primary Card
     */
    public int getPrimaryCard() {
        int phoneId = mSubscriptionManager.getDefaultDataPhoneId();
        Log.d(TAG, "getPrimaryCard: " + phoneId);
        return phoneId;
    }

    public static final int NT_UNKNOWN = -1;
    /** {@hide} */
    public static final int NT_TD_LTE = 51; // TD-LTE only
    /** {@hide} */
    public static final int NT_LTE_FDD = 52; // LTE-FDD only
    /** {@hide} */
    public static final int NT_LTE_FDD_TD_LTE = 53; // LTE-FDD/TD-LTE
    /** {@hide} */
    public static final int NT_LTE_FDD_WCDMA_GSM = 54; // LTE-FDD/WCDMA/GSM
    /** {@hide} */
    public static final int NT_TD_LTE_WCDMA_GSM = 55; // TD-LTE/WCDMA/GSM
    /** {@hide} */
    public static final int NT_LTE_FDD_TD_LTE_WCDMA_GSM = 56; // LTE-FDD/TD-LTE/WCDMA/GSM
    /** {@hide} */
    public static final int NT_TD_LTE_TDSCDMA_GSM = 57; // TD-LTE/TD/GSM
    /** {@hide} */
    public static final int NT_LTE_FDD_TD_LTE_TDSCDMA_GSM = 58; // LTE-FDD/TD-LTE/TD/GSM
    /** {@hide} */
    public static final int NT_LTE_FDD_TD_LTE_WCDMA_TDSCDMA_GSM = 59; // LTE-FDD/TD-LTE/WCDMA/TD/GSM
    /** {@hide} */
    public static final int NT_GSM = 60; // GSM only
    /** {@hide} */
    public static final int NT_WCDMA = 61; // WCDMA only
    /** {@hide} */
    public static final int NT_TDSCDMA = 62; // TD only
    /** {@hide} */
    public static final int NT_TDSCDMA_GSM = 63; // TD/GSM
    /** {@hide} */
    public static final int NT_WCDMA_GSM = 64; // WCDMA/GSM
    /** {@hide} */
    public static final int NT_WCDMA_TDSCDMA_GSM = 65; // WCDMA/TD/GSM

    /**
     * Get the preferred network type.
     * Used for device configuration by some CDMA operators.
     * @param slotIdx the id of the subscription to get the preferred network type for
     * @return the preferred network type, defined in RILConstants.java
     */
    public int getPreferredNetworkType(int slotIdx) {
        if (!SubscriptionManager.isValidPhoneId(slotIdx)) {
            Log.d(TAG, "the slotIdx is not Valid");
            return -1;
        }
        int[] subId = SubscriptionManager.getSubId(slotIdx);
        Log.d(TAG, "the NetworkType of the subId: " + subId);
        int networkType = mTelephonyManager.getPreferredNetworkType(subId[0]);
        Log.d(TAG, "the NetworkType of the slot[" + slotIdx + "]: " + networkType);
        return networkType;
    }

    /**
     * Get the primary card of network type.
     * @return the network type of the primary card
     */
    public int getPreferredNetworkType() {
        return getPreferredNetworkType(getPrimaryCard());
    }

    /**
     * Set the preferred network type.
     * @param slotIdx the id of the subscription to set the preferred network type for
     * @param networkType the preferred network type.
     * @return true on success; false on any failure.
     */
    private boolean setPreferredNetworkType(int slotIdx, int networkType) {
        if (!SubscriptionManager.isValidPhoneId(slotIdx)) {
            Log.d(TAG, "the slotIdx is not Valid");
            return false;
        }
        mRadioInteractor.setPreferredNetworkType(slotIdx,networkType);
        return true;
    }

    /**
     * Set the network type of the primary card.
     * @return true on success; false on any failure.
     */
    public boolean setPreferredNetworkType(int networkType) {
        return setPreferredNetworkType(getPrimaryCard(), networkType);
    }

    public static enum RadioCapbility {
        NONE, TDD_SVLTE,FDD_CSFB, TDD_CSFB, CSFB
    }

    public static RadioCapbility getRadioCapbility() {
        String ssdaMode = SystemProperties.get(PROP_SSDA_MODE);
        Log.d(TAG, "getRadioCapbility: ssdaMode=" + ssdaMode);
        if (ssdaMode.contains(MODE_TDD_CSFB)) {
            return RadioCapbility.TDD_CSFB;
        } else if (ssdaMode.contains(MODE_FDD_CSFB)) {
            return RadioCapbility.FDD_CSFB;
        } else if (ssdaMode.contains(MODE_CSFB)) {
            return RadioCapbility.CSFB;
        }
        return RadioCapbility.NONE;
    }

    public static final String SIM_STANDBY = "sim_standby";

    public static boolean isSimStandby(int phoneId, Context context) {
        if (context == null)
        return true;
        return Settings.Global.getInt(context.getContentResolver(),
                SIM_STANDBY + phoneId, 1) == 1;
         }
}
