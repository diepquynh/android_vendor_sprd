package com.android.phone;

import android.app.AddonManager;
import android.app.AlertDialog;
import android.content.Context;
import android.os.Message;
import android.util.Log;
import android.widget.TextView;

import com.android.internal.telephony.MmiCode;
import com.android.internal.telephony.Phone;
import com.android.phone.R;

public class TelephonyOrangeHelper {
    static TelephonyOrangeHelper sInstance;
    private static final String LOG_TAG = "TelephonyOrangeHelper";

    public static TelephonyOrangeHelper getInstance(Context context) {
        if (sInstance != null) return sInstance;
        AddonManager addonManager = new AddonManager(context);
        sInstance = (TelephonyOrangeHelper) addonManager.getAddon(
                R.string.telephony_orange_plugin, TelephonyOrangeHelper.class);
        return sInstance;
    }

    public TelephonyOrangeHelper() {
    }

    public void dismissMMIDialog() {
        Log.d(LOG_TAG, "dismissMMIDialog");
    }

    public void setAlertDialog(AlertDialog dialog) {
        Log.d(LOG_TAG, "setAlertDialog");
    }

    public AlertDialog getAlertDialog() {
        Log.d(LOG_TAG, "getAlertDialog");
        return null;
    }

    public void setSpan(Context context) {
        Log.d(LOG_TAG, "setSpan on the default addon, do nothing");
    }

    public boolean isSupportUSSDCall() {
        return false;
    }

    public TextView getTextView(Context context, CharSequence text) {
        return null;
    }

    public void handleMMIDialogDismiss(final Phone phone, Context context, final MmiCode mmiCode,
            Message dismissCallbackMessage,
            AlertDialog previousAlert) {
        PhoneUtils.displayMMIComplete(mmiCode.getPhone(),
                PhoneGlobals.getInstance(), mmiCode, null, null);
    }
    /* SPRD: add for bug620380 @{ */
    public boolean isIncomingCallDialogHide() {
        return false;
    }
    /* @} */
}
