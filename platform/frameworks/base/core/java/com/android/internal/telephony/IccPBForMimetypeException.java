package com.android.internal.telephony;


public class IccPBForMimetypeException extends IccPhoneBookOperationException {
    private static final long serialVersionUID = 1L;
    public static final int COMMON_OPREATION_FAILED = -1;
    public static final int CAPACITY_FULL = -2;
    public static final int OVER_LENGTH_LIMIT = -3;
    public static final int WRONG_FORMAT = -4;

    public int mErrorCode = -1;
    public String mMimeType = null;

    public  IccPBForMimetypeException() {

    }

    public  IccPBForMimetypeException(String detailMessage) {
        super(detailMessage);
        // TODO Auto-generated constructor stub
    }

    public IccPBForMimetypeException(Throwable throwable) {
        super(throwable);
        // TODO Auto-generated constructor stub
    }

    public IccPBForMimetypeException(String detailMessage, Throwable throwable) {
        super(detailMessage, throwable);
        // TODO Auto-generated constructor stub
    }
    public IccPBForMimetypeException(int errorCode, String detailMessage) {
        super(detailMessage);
        this.mErrorCode = errorCode;
    }
    public IccPBForMimetypeException(int errorCode, String mimeType , String detailMessage) {
        super(detailMessage);
        this.mErrorCode = errorCode;
        this.mMimeType = mimeType;
    }

}
