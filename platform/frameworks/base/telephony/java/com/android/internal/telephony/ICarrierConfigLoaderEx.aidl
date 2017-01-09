package com.android.internal.telephony;

import android.os.PersistableBundle;

/**
 * Interface used to interact with the CarrierConfigLoaderEx
 */
interface ICarrierConfigLoaderEx {

    PersistableBundle getConfigForSubId(int subId);

    void notifyConfigChangedForSubId(int subId);

    void updateConfigForPhoneId(int phoneId, String simState);

    PersistableBundle getConfigForPhoneId(int phoneId);
}