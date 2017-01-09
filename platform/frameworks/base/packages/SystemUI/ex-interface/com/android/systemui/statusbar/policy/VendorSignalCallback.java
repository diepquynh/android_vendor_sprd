
package com.android.systemui.statusbar.policy;

public interface VendorSignalCallback {
    void setMobileVolteIndicators(boolean show, int subId, int resId);
    void setMobileRoamingIndicators(boolean show, int subId, int resId);
    void setMobileHdVoiceIndicators(boolean show, int subId, int resId);
    void setMobileDataConnectedIndicators(boolean show, int subId);
}
