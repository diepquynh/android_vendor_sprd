
package com.android.internal.telephony.uicc;

public interface IccConstantsEx extends IccConstants {

    static final int EF_DIR = 0x2F00;

    static final int EF_LND = 0x6F44;
    static final int EF_CC = 0x4f23;
    // Add for TS31.121 8.1.2
    static final int EF_PSC = 0x4f22;
    static final int EF_PUID = 0x4f24;
    static final int EF_ECC = 0x6FB7;
    // ISIM access
    static final String DF_ADFISIM = "7FFF";
    // Orange request
    static final int EF_SMSP = 0x6F42; // Short Message Service Parameters
    // UPLMN
    static final int EF_PLMN_ACT = 0x6F60;
    static final int EF_PLMN_SEL = 0X6F30;
}
