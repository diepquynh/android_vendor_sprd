package com.android.internal.telephony;
interface ISmsEx {
    //sprd:add for cb
    boolean commonInterfaceForMessaging(int commonType, long szSubId, String szString, inout int[] data);

    int copyMessageToIccEfForSubscriber(in int subId, String callingPkg, int status, in byte[] pdu, in byte[] smsc);
}