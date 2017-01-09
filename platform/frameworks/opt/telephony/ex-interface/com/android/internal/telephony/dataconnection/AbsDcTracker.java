/* Created by Spreadst */

package com.android.internal.telephony.dataconnection;

import android.os.Handler;

public abstract class AbsDcTracker extends Handler {
    public AbsDcTracker() {
        super();
    }

    /**
     * Whether the data is suspended.
     */
    public boolean isSuspended() {
        return false;
    }

    public boolean isDeepSleep() {
         return false;
    }

    public void prepareDataCall(ApnSetting apnSetting) {
        // No-op
    }
    /*SPRD: bug608747 add esm flag feature @{*/
    public int getEsmFlag(String operator) {
        return 0;
    }

    public boolean getAttachApnEnable() {
        return true;
    }
    /* @} */
    /*SPRD: bug618350 add single pdp allowed by plmns feature@{*/
    public boolean isSinglePDNAllowedByNetwork() {
        return false;
    }

    public void setSinglePDNAllowedByNetwork(String plmn) {
        //No-op
    }
    /* @} */
    /*
     * Handle special clear codes.
     */
    protected boolean handleSpecialClearCode(DcFailCause cause, ApnContext apnContext) {
        return false;
    }
}
