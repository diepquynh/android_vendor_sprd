package com.android.internal.telephony;


public class IccPBForRecordException extends IccPhoneBookOperationException {
    private static final long serialVersionUID = 1L;
    public static final int WRITE_RECORD_FAILED = -1;
    public static final int ADN_RECORD_CAPACITY_FULL = -4;
    public static final int LOAD_RECORD_FAILED = -5;
    public static final int ANR_RECORD_CAPACITY_FULL = -6;
    public static final int OVER_GRP_MAX_LENGTH = -7;

    public int mErrorCode = -1;

    public  IccPBForRecordException() {

    }

    public  IccPBForRecordException(String detailMessage) {
        super(detailMessage);
        // TODO Auto-generated constructor stub
    }

    public IccPBForRecordException(Throwable throwable) {
        super(throwable);
        // TODO Auto-generated constructor stub
    }

    public IccPBForRecordException(String detailMessage, Throwable throwable) {
        super(detailMessage, throwable);
        // TODO Auto-generated constructor stub
    }
    public IccPBForRecordException(int errorCode, String detailMessage) {
        super(detailMessage);
        this.mErrorCode = errorCode;
    }

}
