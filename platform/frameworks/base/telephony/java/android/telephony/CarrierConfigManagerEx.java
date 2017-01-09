package android.telephony;

import com.android.internal.telephony.ICarrierConfigLoaderEx;

import android.os.PersistableBundle;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.telephony.Rlog;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.SystemApi;
import android.content.Context;

public class CarrierConfigManagerEx extends AbsCarrierConfigManager {
    private final static String TAG = "CarrierConfigManagerEx";

    /**
     * @hide
     */
    public CarrierConfigManagerEx() {
    }

    /** {@hide} */
    public static CarrierConfigManagerEx from(Context context) {
        return (CarrierConfigManagerEx) context.getSystemService("carrier_config_ex");
    }

    /**
     * Extra value with broadcast "android.telephony.action.CARRIER_CONFIG_CHANGED"
     * It's means that config changed from feature , network or subinfo.
     *  By filtering the changed type value to obtain the source of the changes in the broadcast
     *  int configFeature = 0;
     *  int configNetwork = 1;
     *  int configSubInfo = 2;
     *  int configAll = 3;
     */
    public static final String CARRIER_CONFIG_CHANGED_TYPE = "carrier_changed_type";

    /* SPRD: Add global config variables. @{ */
    public static final String KEY_GLO_CONF_VOICEMAIL_NUMBER = "vmnumber";
    public static final String KEY_GLO_CONF_VOICEMAIL_TAG = "vmtag";
    public static final String KEY_GLO_CONF_ROAMING_VOICEMAIL_NUMBER = "roamingvm";
    public static final String KEY_GLO_CONF_ECC_LIST_NO_CARD = "ecclist_nocard";
    public static final String KEY_GLO_CONF_ECC_LIST_WITH_CARD = "ecclist_withcard";
    public static final String KEY_GLO_CONF_FAKE_ECC_LIST_WITH_CARD = "fake_ecclist_withcard";
    public static final String KEY_GLO_CONF_NUM_MATCH = "num_match";
    public static final String KEY_GLO_CONF_NUM_MATCH_RULE = "num_match_rule";
    public static final String KEY_GLO_CONF_NUM_MATCH_SHORT = "num_match_short";
    public static final String KEY_GLO_CONF_SPN = "spn";
    public static final String KEY_GLO_CONF_SMS_7BIT_ENABLED = "sms_7bit_enabled";
    public static final String KEY_GLO_CONF_SMS_CODING_NATIONAL = "sms_coding_national";
    public static final String KEY_GLO_CONF_MVNO = "mvno";
    public static final String KEY_NETWORK_NOT_ALLOW_3G_AND_2G_BOOL = "network_not_allow_3g_and_2g";

    /** {@hide} */
    public static final String KEY_Network_RAT_Prefer_INT = "networkRATPrefer";
    /**
     *Add for setting homepage via MCC\MNC
     *@{
     */
    public static final String KEY_GLO_CONF_HOMEPAGE_URL = "homepage_url";
    /*@}*/

    /* @} */
    /* SPRD: FEATURE_OF_APN_AND_DATA_POP_UP @{ */
    public static final String KEY_FEATURE_DATA_AND_APN_POP_BOOL =
            "feature_data_and_apn_pop_bool";
    public static final String KEY_FEATURE_DATA_AND_APN_POP_OPERATOR_STRING =
            "feature_data_and_apn_pop_operator_string";
    /* @} */
    /**
     * SPRD: [Bug475782] Flag for fixed primary slot. -1 means not support fixed primary slot.
     */
    /** {@hide} */
    public static final String KEY_FIXED_PRIMARY_SLOT_INT = "fixed_primary_slot_int";

    // SPRD Add for Bug 570658: Heartbeat interval operator network adaptation.
    public static final String KEY_NETWORK_NAT_OVERTIME_2G = "network_nat_overtime_2g";
    public static final String KEY_NETWORK_NAT_OVERTIME_3G = "network_nat_overtime_3g";
    public static final String KEY_NETWORK_NAT_OVERTIME_4G = "network_nat_overtime_4g";

    /**
     * SPRD: screen off 5s after call connection. See bug #503700
     * {@hide}
     */
    public static final String KEY_SCREEN_OFF_IN_ACTIVE_CALL_STATE_BOOL =
            "screen_off_in_active_call_state_bool";
    /* SPRD: Add for double press on the headset key feature. @{ */
    /**
     * Flag specifying whether Double press on the headset key feature is supported.
     * {@hide}
     */
    public static final String KEY_FEATURE_DOUBLE_PRESS_ON_HEADSET_KEY_BOOL =
            "double_press_on_headset_key_bool";
    /* @} */

    /* SPRD: Vibrate when call connected or disconnected feature. @{ */
    /**
     * Flag specifying whether Vibrate when call connected or disconnected feature is supported.
     */
    public static final String KEY_FEATURE_VIBRATE_FOR_CALL_CONNECTION_BOOL =
            "vibrate_for_call_connection_bool";
    /* @} */

    /* SPRD: bug#474283, add for IP-DIAL FEATURE @{ */
    public static final String KEY_FEATURE_IP_DIAL_ENABLED_BOOL = "ip_dial_enabled_bool";
    /* @{ */

    /* SPRD: bug#476317, FEATURE_MANUAL_QUERY_NETWORK @{ */
    public static final String KEY_FEATURE_SHOW_NETWORK_SELECTION_WARNING_BOOL =
            "show_network_selection_warning_bool";
    /* @{ */
    /**
     * SPRD: Flip to silence from incoming calls. See bug473877
     * {@hide}
     */
    public static final String KEY_FEATURE_FLIP_SILENT_INCOMING_CALL_ENABLED_BOOL =
            "flip_to_silent_incoming_call_enabled_bool";

    /* Add for feature: Query Available Networks @{ */
    public static final String KEY_FEATURE_QUERY_NETWORK_RESULT_SHOW_TYPE_SUFFIX = "query_network_result_show_type_suffix";
    public static final String KEY_FEATURE_QUERY_NETWORK_RESULT_SHOW_STATE_SUFFIX = "query_network_result_show_state_suffix";
    /* @} */

     /**
      * Boolean to decide whether to use #KEY_CARRIER_NAME_STRING from CarrierConfig app.
      * @hide
      */
     public static final String KEY_CARRIER_NAME_OVERRIDE_BOOL = "carrier_name_override_bool";

     /**
      * String to identify carrier name in CarrierConfig app. This string is used only if
      * #KEY_CARRIER_NAME_OVERRIDE_BOOL is true
      * @hide
      */
     public static final String KEY_CARRIER_NAME_STRING = "carrier_name_string";

    /**
     * SPRD: Flag specifying fade-in feature s supported. See bug530524
     * {@hide}
     */
    public static final String KEY_FEATURE_FADE_IN_ENABLED_BOOL = "fade_in_enabled_bool";

    /* SPRD: Add switch for automatic call record feature. @{ */
    /**
     * Flag specifying whether automatic call record feature is supported.
     * {@hide}
     */
    public static final  String KEY_FEATURE_AUTOMATIC_CALL_RECORD_ENABLED_BOOL =
            "automatic_call_record_enabled_bool";
     /* @} */

    /**
     * If false,do not automatically set primary card according to IccPolicy after hot swap if
     * current primary card is active.
     */
    public static final String KEY_FORCE_AUTO_SET_PRIMARY_CARD_AFTER_HOT_SWAP = "key_force_auto_set_primary_card_after_hot_swap";

    /*SPRD: add for esm flag feature @{ */
    public static final  String KEY_FEATURE_ATTACH_APN_ENABLE_BOOL =
            "attach_apn_enable";
    public static final  String KEY_FEATURE_PLMNS_ESM_FLAG_STRING =
            "plmns_esm_flag";
    /* @} */
    /*SPRD: add for single pdn feature @{ */
    public static final String KEY_PLMNS_SINGLE_PDN = "plmns_single_pdn";
    /* @} */

    // SPRD: add for bug626699
    public static final String KEY_PLMNS_CALLFORWARD_ONLY = "plmns_callforward_only";

    /**
     * SPRD: Add for cmcc product,if true,disable/enable sim card only power off/on radio. See bug636909
     * {@hide}
     */
    public static final String KEY_FEATURE_SET_FAKE_SIM_ENABLE_BOOL = "set_fake_sim_enable_boolean";
    /** The default value for every variable. */
    private final static PersistableBundle sDefaultsEx;
    static {
        sDefaultsEx = new PersistableBundle();
        /* SPRD: [bug475223] Add global config defaults. @{ */
        sDefaultsEx.putString(KEY_GLO_CONF_VOICEMAIL_NUMBER, "");
        sDefaultsEx.putString(KEY_GLO_CONF_VOICEMAIL_TAG, "");
        sDefaultsEx.putString(KEY_GLO_CONF_ROAMING_VOICEMAIL_NUMBER, "");
        sDefaultsEx.putString(KEY_GLO_CONF_ECC_LIST_NO_CARD, "");
        sDefaultsEx.putString(KEY_GLO_CONF_ECC_LIST_WITH_CARD, "");
        sDefaultsEx.putString(KEY_GLO_CONF_FAKE_ECC_LIST_WITH_CARD, "");
        sDefaultsEx.putInt(KEY_GLO_CONF_NUM_MATCH, 7);
        sDefaultsEx.putInt(KEY_GLO_CONF_NUM_MATCH_RULE, -1);
        sDefaultsEx.putInt(KEY_GLO_CONF_NUM_MATCH_SHORT, -1);
        sDefaultsEx.putString(KEY_GLO_CONF_SPN, "");
        sDefaultsEx.putBoolean(KEY_GLO_CONF_SMS_7BIT_ENABLED, false);
        sDefaultsEx.putString(KEY_GLO_CONF_SMS_CODING_NATIONAL, "");
        sDefaultsEx.putBoolean(KEY_GLO_CONF_MVNO, false);
        /* @} */
        sDefaultsEx.putBoolean(KEY_FIXED_PRIMARY_SLOT_INT, false);
        /* SPRD: FEATURE_OF_APN_AND_DATA_POP_UP @{ */
        sDefaultsEx.putBoolean(KEY_FEATURE_DATA_AND_APN_POP_BOOL, false);
        sDefaultsEx.putString(KEY_FEATURE_DATA_AND_APN_POP_OPERATOR_STRING, "");
        /* @} */
        sDefaultsEx.putBoolean(KEY_SCREEN_OFF_IN_ACTIVE_CALL_STATE_BOOL, false);

        /**
         *Add for setting homepage via MCC\MNC
         *@{
         */
        sDefaultsEx.putString(KEY_GLO_CONF_HOMEPAGE_URL, "");
        /*@}*/
        //SPRD: add for esm flag feature
        sDefaultsEx.putString(KEY_FEATURE_PLMNS_ESM_FLAG_STRING, "");
        //SPRD: add for single pdn feature
        sDefaultsEx.putString(KEY_PLMNS_SINGLE_PDN, "");
        // SPRD: Vibrate when call connected or disconnected feature.
        sDefaultsEx.putBoolean(KEY_FEATURE_VIBRATE_FOR_CALL_CONNECTION_BOOL, true);
        // SPRD: Add for double press on the headset key feature.
        sDefaultsEx.putBoolean(KEY_FEATURE_DOUBLE_PRESS_ON_HEADSET_KEY_BOOL, false);
        /* SPRD: bug#474283, add for IP-DIAL FEATURE @{ */
        sDefaultsEx.putBoolean(KEY_FEATURE_IP_DIAL_ENABLED_BOOL, true);
        /* @} */
        /*  SPRD: bug#476317, FEATURE_MANUAL_QUERY_NETWORK @{ */
        sDefaultsEx.putBoolean(KEY_FEATURE_SHOW_NETWORK_SELECTION_WARNING_BOOL, true);
        /* @} */
        sDefaultsEx.putBoolean(KEY_FEATURE_FLIP_SILENT_INCOMING_CALL_ENABLED_BOOL, true);
        sDefaultsEx.putBoolean(KEY_FEATURE_QUERY_NETWORK_RESULT_SHOW_TYPE_SUFFIX, true);
        sDefaultsEx.putBoolean(KEY_FEATURE_QUERY_NETWORK_RESULT_SHOW_STATE_SUFFIX, true);
        //SPRD: Add global Global parameter adjust for carrier config
        sDefaultsEx.putBoolean(KEY_CARRIER_NAME_OVERRIDE_BOOL, false);
        sDefaultsEx.putString(KEY_CARRIER_NAME_STRING, "");
        // SPRD: add for bug626699
        sDefaultsEx.putString(KEY_PLMNS_CALLFORWARD_ONLY, "");
        sDefaultsEx.putBoolean(KEY_FEATURE_FADE_IN_ENABLED_BOOL, true);
        // SPRD: Add switch for automatic call record feature.
        sDefaultsEx.putBoolean(KEY_FEATURE_AUTOMATIC_CALL_RECORD_ENABLED_BOOL, true);
        sDefaultsEx.putBoolean(KEY_NETWORK_NOT_ALLOW_3G_AND_2G_BOOL, false);
        sDefaultsEx.putBoolean(KEY_FORCE_AUTO_SET_PRIMARY_CARD_AFTER_HOT_SWAP, false);
        //SPRD: add for esm flag feature
        sDefaultsEx.putBoolean(KEY_FEATURE_ATTACH_APN_ENABLE_BOOL, true);
        // SPRD Add for Bug 570658: Heartbeat interval operator network adaptation.
        sDefaultsEx.putInt(KEY_NETWORK_NAT_OVERTIME_2G, 5);
        sDefaultsEx.putInt(KEY_NETWORK_NAT_OVERTIME_3G, 15);
        sDefaultsEx.putInt(KEY_NETWORK_NAT_OVERTIME_4G, 30);
        sDefaultsEx.putInt(KEY_Network_RAT_Prefer_INT, 0);
        sDefaultsEx.putBoolean(KEY_FEATURE_SET_FAKE_SIM_ENABLE_BOOL,false);
    }

    /**
     * Gets the configuration values for a particular subscription, which is associated with a
     * specific SIM card. If an invalid subId is used, the returned config will contain default
     * values.
     *
     * <p>Requires Permission:
     * {@link android.Manifest.permission#READ_PHONE_STATE READ_PHONE_STATE}
     *
     * @param subId the subscription ID, normally obtained from {@link SubscriptionManager}.
     * @return A {@link PersistableBundle} containing the config for the given subId, or default
     *         values for an invalid subId.
     */
    @Nullable
    public PersistableBundle getConfigForSubId(int subId) {
        try {
            ICarrierConfigLoaderEx loader = getICarrierConfigLoaderEx();
            if (loader == null) {
                Rlog.w(TAG, "Error getting config for subId " + subId + " ICarrierConfigLoaderEx is null");
                return null;
            }
            return loader.getConfigForSubId(subId);
        } catch (RemoteException ex) {
            Rlog.e(TAG, "Error getting config for subId " + subId + ": " + ex.toString());
        }
        return null;
    }

    /**
     * SPRD: Gets the configuration values for a particular phoneId.
     * The biggest difference with "getConfigForSubId(int subId)" is that network preferred
     * config will still be returned as long as register any network no matter whether sim
     * card inserted or not.If an invalid phoneId is used, the returned config will contain
     * default values.
     *
     * <p>Requires Permission:
     * {@link android.Manifest.permission#READ_PHONE_STATE READ_PHONE_STATE}
     *
     * @return A {@link PersistableBundle} containing the config for the given phoneId, or default
     *         values for an invalid phoneId.
     */
    @Nullable
    public PersistableBundle getConfigForPhoneId(int phoneId) {
        try {
            return getICarrierConfigLoaderEx().getConfigForPhoneId(phoneId);
        } catch (RemoteException ex) {
            Rlog.e(TAG, "Error getting config for phoneId " + Integer.toString(phoneId) + ": "
                    + ex.toString());
        } catch (NullPointerException ex) {
            Rlog.e(TAG, "Error getting config for phoneId " + Integer.toString(phoneId) + ": "
                    + ex.toString());
        }
        return null;
    }

    /**
     * SPRD: Gets the configuration values for the default phoneId which will be phone 0 if no sim exit.
     * It is especially used for getting feature config which has nothing to do with a particular phoneId or subId.
     *
     * <p>Requires Permission:
     * {@link android.Manifest.permission#READ_PHONE_STATE READ_PHONE_STATE}
     *
     * @return A {@link PersistableBundle} containing the config for the default phoneId.
     */
    @Nullable
    public PersistableBundle getConfigForDefaultPhone() {
        int defaultPhoneId = SubscriptionManager.getPhoneId(SubscriptionManager.getDefaultSubscriptionId());
        if (!SubscriptionManager.isValidPhoneId(defaultPhoneId)) {
            defaultPhoneId = 0;
        }
        try {
            return getICarrierConfigLoaderEx().getConfigForPhoneId(defaultPhoneId);
        } catch (RemoteException ex) {
            Rlog.e(TAG, "Error getting config for default phone " + Integer.toString(defaultPhoneId) + ": "
                    + ex.toString());
        } catch (NullPointerException ex) {
            Rlog.e(TAG, "Error getting config for default phone " + Integer.toString(defaultPhoneId) + ": "
                    + ex.toString());
        }
        return null;
    }

    /**
     * Gets the configuration values for the default subscription.
     *
     * <p>Requires Permission:
     * {@link android.Manifest.permission#READ_PHONE_STATE READ_PHONE_STATE}
     *
     * @see #getConfigForSubId
     */
    @Nullable
    public PersistableBundle getConfig() {
        return getConfigForSubId(SubscriptionManager.getDefaultSubscriptionId());
    }

    /**
     * Calling this method triggers telephony services to fetch the current carrier configuration.
     * <p>
     * Normally this does not need to be called because the platform reloads config on its own.
     * This should be called by a carrier service app if it wants to update config at an arbitrary
     * moment.
     * </p>
     * <p>Requires that the calling app has carrier privileges.
     * @see #hasCarrierPrivileges
     * <p>
     * This method returns before the reload has completed, and
     * {@link android.service.carrier.CarrierService#onLoadConfig} will be called from an
     * arbitrary thread.
     * </p>
     */
    public void notifyConfigChangedForSubId(int subId) {
        try {
            ICarrierConfigLoaderEx loader = getICarrierConfigLoaderEx();
            if (loader == null) {
                Rlog.w(TAG, "Error reloading config for subId=" + subId + " ICarrierConfigLoaderEx is null");
                return;
            }
            loader.notifyConfigChangedForSubId(subId);
        } catch (RemoteException ex) {
            Rlog.e(TAG, "Error reloading config for subId=" + subId + ": " + ex.toString());
        }
    }

    /**
     * Request the carrier config loader to update the cofig for phoneId.
     * <p>
     * Depending on simState, the config may be cleared or loaded from config app. This is only used
     * by SubscriptionInfoUpdater.
     * </p>
     *
     * @hide
     */
    @SystemApi
    public void updateConfigForPhoneId(int phoneId, String simState) {
        try {
            ICarrierConfigLoaderEx loader = getICarrierConfigLoaderEx();
            if (loader == null) {
                Rlog.w(TAG, "Error updating config for phoneId=" + phoneId + " ICarrierConfigLoaderEx is null");
                return;
            }
            loader.updateConfigForPhoneId(phoneId, simState);
        } catch (RemoteException ex) {
            Rlog.e(TAG, "Error updating config for phoneId=" + phoneId + ": " + ex.toString());
        }
    }

    /**
     * Returns a new bundle with the default value for every supported configuration variable.
     *
     * @hide
     */
    @NonNull
    @SystemApi
    public static PersistableBundle getDefaultConfig() {
        return new PersistableBundle(sDefaultsEx);
    }

    /** @hide */
    private ICarrierConfigLoaderEx getICarrierConfigLoaderEx() {
        return ICarrierConfigLoaderEx.Stub
                .asInterface(ServiceManager.getService("carrier_config_ex"));
    }
}
