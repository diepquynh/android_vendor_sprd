/** Create by Spreadst */

package com.spreadst.lockscreen;

import android.content.Context;
import android.util.Log;
import android.widget.AbsLockScreen;
import android.widget.Button;
import android.widget.ILockScreenListener;
import android.widget.TextView;

import com.android.internal.telephony.IccCardConstants.State;

public class Lockscreen extends AbsLockScreen {

    protected ILockScreenListener mLockScreenListener = null;

    protected Button unlockButton = null;

    protected TextView unlockText = null;

    protected Context mcontext = null;

    public Lockscreen(Context context, ILockScreenListener listener) {
        super(context, listener);
        mLockScreenListener = listener;
        mcontext = context;
        Log.d(Constants.CURRENT_PACKAGE_NAME, "Lockscreen init...");
    }

    @Override
    public void onRefreshBatteryInfo(boolean showBatteryInfo,
            boolean pluggedIn, int batteryLevel) {

    }

    @Override
    public void onTimeChanged() {
        // TODO Auto-generated method stub

    }

    @Override
    public boolean needsInput() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void onRefreshCarrierInfo(CharSequence plmn, CharSequence spn,
            int subscription) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onRingerModeChanged(int state) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onSimStateChanged(State simState, int subscription) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onPause() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onResume() {
        // TODO Auto-generated method stub
        Log.d(Constants.CURRENT_PACKAGE_NAME, "Lockscreen onResume...");
    }

    @Override
    public void cleanUp() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onStartAnim() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onStopAnim() {
        // TODO Auto-generated method stub

    }

	@Override
	public void onPhoneStateChanged(int phoneState) {
		// TODO Auto-generated method stub
	}

    @Override
    public void onClockVisibilityChanged() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onDeviceProvisioned() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onMessageCountChanged(int messagecount) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onDeleteMessageCount(int messagecount) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onMissedCallCountChanged(int count) {
        // TODO Auto-generated method stub

    }

    /* SPRD: Modify 20140207 Spreadst of Bug 267015 add owner info text for UUI lockscreen @{ */
    @Override
    public void onScreenTurnedOn() {
        // TODO Auto-generated method stub

    }

    @Override
    public void onScreenTurnedOff(int why) {
        // TODO Auto-generated method stub

    }
    /* @} */
}
