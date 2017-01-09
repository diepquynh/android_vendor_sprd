package com.android.internal.telephony.uicc;

/**
 * {@hide}
 */
public class AdnRecordCacheController {

    public static AdnRecordCache createAdnRecordCache(IccFileHandler fh) {
        return new AdnRecordCache(fh);
    }
}