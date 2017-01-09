
package com.android.sprd.telephony;

import android.telephony.Rlog;

public class UtilLog {
    public static final String TAG = "RadioInteractor";
    private static final boolean DBG = true;

    public static void logd(String cTag, String content) {
        if (DBG) {
            Rlog.d(TAG, "[" + cTag + "] " + content);
        }
    }

    public static void loge(String cTag, String content) {
        if (DBG) {
            Rlog.e(TAG, "[" + cTag + "] " + content);
        }
    }

    public static void loge(String cTag, String content, Throwable tr) {
        if (DBG) {
            Rlog.e(TAG, "[" + cTag + "] " + content, tr);
        }
    }
}
