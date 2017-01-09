package com.android.internal.telephony;

import android.util.Log;

/**
 * Created by lizhong.wu on 2016/9/21.
 * {@hide}
 */
public abstract class AbsIccSmsInterfaceManager {

    public AbsIccSmsInterfaceManager(){

    }

    public int copyMessageToIccEfSprd(String callingPackage, int status, byte[] pdu, byte[] smsc) {
        Log.d("AbsIccSmsInterfaceManager", "enter copyMessageToIccEfSprd");
        return -1;
    }
}
