package com.android.internal.telephony;

import android.content.Context;
import android.telephony.AbsCarrierConfigManager;
import android.telephony.AbsTelephonyManager;
import android.telephony.CarrierConfigManagerEx;
import android.telephony.TelephonyManagerEx;

public class FrameworkFactoryEx extends FrameworkFactory {

    public AbsCarrierConfigManager createExtraCarrierConfigManager() {
        return new CarrierConfigManagerEx();
    }

    public AbsTelephonyManager createExtraTelephonyManager(Context context) {
        return new TelephonyManagerEx(context);
    }
}
