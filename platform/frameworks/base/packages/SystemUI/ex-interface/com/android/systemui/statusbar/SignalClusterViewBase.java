
package com.android.systemui.statusbar;

import android.content.Context;
import android.graphics.Rect;
import android.widget.LinearLayout;
import android.util.AttributeSet;

import com.android.systemui.R;
import com.android.systemui.statusbar.policy.NetworkController.IconState;
import com.android.systemui.statusbar.policy.NetworkControllerImpl;
import com.android.systemui.statusbar.policy.SecurityController;

import com.android.systemui.statusbar.policy.VendorSignalCallback;

public abstract class SignalClusterViewBase
        extends LinearLayout
        implements NetworkControllerImpl.SignalCallback,
        VendorSignalCallback,
        SecurityController.SecurityControllerCallback {

    public SignalClusterViewBase(Context context) {
        this(context, null);
    }

    public SignalClusterViewBase(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SignalClusterViewBase(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public void setNetworkController(NetworkControllerImpl nc) {
    }

    public void setSecurityController(SecurityController sc) {
    }

    public void setIconTint(int tint, float darkIntensity, Rect tintArea) {
    }

    @Override
    public void setMobileVolteIndicators(boolean show, int subId, int resId) {
    }

    @Override
    public void setMobileRoamingIndicators(boolean show, int subId, int resId) {
    }

    @Override
    public void setMobileHdVoiceIndicators(boolean show, int subId, int resId) {
    }

    @Override
    public void setMobileDataConnectedIndicators(boolean show, int subId) {
    }
}
