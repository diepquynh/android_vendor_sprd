package com.android.internal.telephony;

public class TelephonyIntentsEx {

    /**
     * SPRD: 474587 Feature for PhoneBook STK_REFRESH
     */
    public static final String ACTION_STK_REFRESH_SIM_CONTACTS
                               = "android.intent.action.ACTION_STK_REFRESH_SIM_CONTACTS";

     /**
      * SPRD: 474587 Feature for PhoneBook FDN_STATUS
      */
     public static final String INTENT_KEY_FDN_STATUS = "fdn_status";

    /**
     * SPRD: add for HIGH_DEF_AUDIO_SUPPORT
     */
    public static final String ACTION_HIGH_DEF_AUDIO_SUPPORT = "android.intent.action.HIGH_DEF_AUDIO_SUPPORT";
    public static final String EXTRA_HIGH_DEF_AUDIO = "isHdVoiceSupport";
    /** SPRD: added for sim lock @{ */
    public static final String SHOW_SIMLOCK_UNLOCK_SCREEN_ACTION
            = "android.intent.action.ACTION_SHOW_SIMLOCK_UNLOCK";
    public static final String EXTRA_SIMLOCK_UNLOCK = "android.intent.extra.SIMLOCK_UNLOCK";
    public static final String SHOW_SIMLOCK_UNLOCK_SCREEN_BYNV_ACTION
            = "android.intent.action.ACTION_SHOW_SIMLOCK_UNLOCK_BYNV";
    /** @} */
}
