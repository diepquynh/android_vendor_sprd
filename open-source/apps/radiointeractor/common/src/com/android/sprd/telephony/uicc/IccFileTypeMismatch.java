package com.android.sprd.telephony.uicc;

/**
 * SPRD: Bug#525009 Add support for Open Mobile API
 */
public class IccFileTypeMismatch extends IccException {
    public IccFileTypeMismatch() {

    }

    public IccFileTypeMismatch(String s) {
        super(s);
    }
}
