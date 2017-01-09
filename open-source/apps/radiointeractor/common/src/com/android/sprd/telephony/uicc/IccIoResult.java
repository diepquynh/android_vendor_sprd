package com.android.sprd.telephony.uicc;

/**
 * SPRD: Bug#525009 Add support for Open Mobile API
 */
public class
IccIoResult {
    public int sw1;
    public int sw2;

    public byte[] payload;

    public IccIoResult(int sw1, int sw2, byte[] payload) {
        this.sw1 = sw1;
        this.sw2 = sw2;
        this.payload = payload;
    }

    public IccIoResult(int sw1, int sw2, String hexString) {
        this(sw1, sw2, IccUtils.hexStringToBytes(hexString));
    }

    @Override
    public String toString() {
        return "IccIoResponse sw1:0x" + Integer.toHexString(sw1) + " sw2:0x"
                + Integer.toHexString(sw2);
    }

    /**
     * true if this operation was successful
     * See GSM 11.11 Section 9.4
     * (the fun stuff is absent in 51.011)
     */
    public boolean success() {
        return sw1 == 0x90 || sw1 == 0x91 || sw1 == 0x9e || sw1 == 0x9f;
    }

    /**
     * Returns exception on error or null if success
     */
    public IccException getException() {
        if (success()) return null;

        switch (sw1) {
            case 0x94:
                if (sw2 == 0x08) {
                    return new IccFileTypeMismatch();
                } else {
                    return new IccFileNotFound();
                }
            default:
                return new IccException("sw1:" + sw1 + " sw2:" + sw2);
        }
    }
}
