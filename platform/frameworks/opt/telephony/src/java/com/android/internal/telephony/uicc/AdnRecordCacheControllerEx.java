package com.android.internal.telephony.uicc;

/**
 * {@hide}
 */
public class AdnRecordCacheControllerEx {

    public static AdnRecordCache createAdnRecordCache(IccFileHandler fh) {
            return new AdnRecordCacheEx(fh);
    }
}