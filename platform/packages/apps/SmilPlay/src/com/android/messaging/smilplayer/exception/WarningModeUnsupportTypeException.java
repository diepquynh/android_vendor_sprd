package com.android.messaging.smilplayer.exception;

//import com.android.messaging.smilplayer.exception.ContentRestrictionException;

public final class WarningModeUnsupportTypeException extends ContentRestrictionException {
    private static final long serialVersionUID = 2684128059358481234L;

    public WarningModeUnsupportTypeException() {
        super();
    }

    public WarningModeUnsupportTypeException(String msg) {
        super(msg);
    }
}