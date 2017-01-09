package com.android.internal.telephony;


public class IccPhoneBookOperationException extends RuntimeException {
    private static final long serialVersionUID = 1L;

    public static final int WRITE_OPREATION_FAILED = -1;
    public static final int EMAIL_CAPACITY_FULL = -2;
    public static final int ADN_CAPACITY_FULL = -3;
    public static final int OVER_NAME_MAX_LENGTH = -4;
    public static final int OVER_NUMBER_MAX_LENGTH = -5;
    public static final int LOAD_ADN_FAIL = -6;
    public static final int OVER_GROUP_NAME_MAX_LENGTH = -7;
    public static final int GROUP_CAPACITY_FULL = -8;
    public static final int ANR_CAPACITY_FULL = -9;
    public static final int GRP_RECORD_MAX_LENGTH = -10;
    public static final int OVER_AAS_MAX_LENGTH = -11;
    public static final int AAS_CAPACITY_FULL = -12;

    public int mErrorCode = -1;

    public  IccPhoneBookOperationException() {

    }

    public  IccPhoneBookOperationException(String detailMessage) {
        super(detailMessage);
        // TODO Auto-generated constructor stub
    }

    public IccPhoneBookOperationException(Throwable throwable) {
        super(throwable);
        // TODO Auto-generated constructor stub
    }

    public IccPhoneBookOperationException(String detailMessage, Throwable throwable) {
        super(detailMessage, throwable);
        // TODO Auto-generated constructor stub
    }
    public IccPhoneBookOperationException(int errorCode, String detailMessage) {
        super(detailMessage);
        this.mErrorCode = errorCode;
    }

    public IccPhoneBookOperationException(int errorCode, String detailMessage, Throwable throwable) {
        super(detailMessage, throwable);
        this.mErrorCode = errorCode;
    }

}
