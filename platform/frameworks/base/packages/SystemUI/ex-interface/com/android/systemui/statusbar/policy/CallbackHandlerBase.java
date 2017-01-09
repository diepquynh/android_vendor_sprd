package com.android.systemui.statusbar.policy;

import android.os.Handler;
import android.os.Looper;

public class CallbackHandlerBase extends Handler implements VendorSignalCallback {

    public CallbackHandlerBase() {
        super();
    }

    CallbackHandlerBase(Looper looper) {
        super(looper);
    }

    @Override
    public void setMobileVolteIndicators(final boolean show, final int subId,
            final int resId) {}

    @Override
    public void setMobileRoamingIndicators(final boolean show, final int subId,
            final int resId) {}

    @Override
    public void setMobileHdVoiceIndicators(final boolean show, final int subId,
            final int resId) {}

    @Override
    public void setMobileDataConnectedIndicators(final boolean show,
            final int subId) {}
}
