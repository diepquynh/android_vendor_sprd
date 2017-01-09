package com.android.sprd.telephony.uicc;

/**
 * SPRD: Bug#525009 Add support for Open Mobile API
 */
public class IccException extends Exception {
    public IccException() {

    }

    public IccException(String s) {
        super(s);
    }
}