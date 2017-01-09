package com.android.internal.telephony;

public class IccPBForEncodeException extends Exception {
    public IccPBForEncodeException() {
        super();
    }

    public IccPBForEncodeException(String s) {
        super(s);
    }

    public IccPBForEncodeException(char c) {
        super("Unencodable char: '" + c + "'");
    }
}