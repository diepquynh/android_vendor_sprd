package com.android.internal.telephony;

import android.os.Message;
import android.os.Handler;

/**
 * {@hide}
 */
public abstract class AbsIccFileHandler extends Handler {

    public AbsIccFileHandler() {
        super();
    }

    public void loadEFLinearFixed(int fileid, int recordNum, int recordSize, Message onLoaded) {

    }

    public void updateEFCYCLICLinearFixed(int fileid, int recordNum, byte[] data,
            String pin2, Message onComplete) {

    }
}
