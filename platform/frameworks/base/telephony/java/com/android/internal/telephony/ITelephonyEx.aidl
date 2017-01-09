package com.android.internal.telephony;

import android.os.Bundle;
import android.net.Uri;

interface ITelephonyEx {

    /**
     * Attention!!! These two interfaces must be put at the top.
     * So please do not add interface in front of these two, otherwise
     * it will cause the failure of PLMN/NetworkList functions.
     */
    String getHighPriorityPlmn(int phoneId, String mccmnc);
    String updateNetworkList(int phoneId, in String[] operatorInfo);

    void setSimEnabled(int phoneId, boolean turnOn);

        /** 
         * SPRD: 474587 Feature for PhoneBook
         * @return true if a IccFdn enabled
        */
         boolean getIccFdnEnabledForSubscriber(int subId);
         boolean isUsimCard(int phoneId);
    /**
     * SPRD: add for set line 1 number without permission
     * Return the result of set line 1 number.
     *
     */
    boolean setLine1NumberForDisplayForSubscriberEx(int subId, String alphaTag, String number);
    String getSmsc();
    String getSmscForSubscriber(int subId);
    boolean setSmsc(String smscAddr);
    boolean setSmscForSubscriber(String smscAddr, int subId);
    //SPRD: Add for Bug588409
    Bundle getCellLocationForPhone(int phoneId);
    //SPRD: add for bug589362
    boolean isRingtongUriAvailable(in Uri uri);
    boolean isDeviceSupportLte();
    //SPRD:Add for bug602746
    String getPnnHomeName(int subId);
    boolean isInternationalNetworkRoaming(int subId);
    int getDataState(int subId);
    boolean isSimSlotSupportLte(int phoneId);
    void setDefaultDataSubId(int subId);
    boolean isVolteEnabledByPlatform();
    boolean isEnhanced4gLteModeSettingEnabledByUser();
    void setEnhanced4gLteModeSetting(boolean enabled);
}
