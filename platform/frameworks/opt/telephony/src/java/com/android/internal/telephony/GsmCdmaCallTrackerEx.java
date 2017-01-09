/* Created by Spreadtrum */

package com.android.internal.telephony;

import com.android.sprd.telephony.RadioInteractor;

import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Message;
import android.os.RegistrantList;
import android.os.SystemProperties;
import android.telecom.VideoProfile;
import android.telephony.DisconnectCause;
import android.telephony.PhoneNumberUtils;
import android.telephony.ServiceState;
import android.telephony.Rlog;

import java.util.List;

public final class GsmCdmaCallTrackerEx extends GsmCdmaCallTracker {

    private static final String LOG_TAG = "GsmCdmaCallTrackerEx";

    private static final boolean DBG_POLL = false;

    private GsmCdmaPhone mPhone;
    private int mMoVideoState = 0;
    RadioInteractor mRi;

    public GsmCdmaCallTrackerEx (GsmCdmaPhone phone) {
        super(phone);
        this.mPhone = phone;
        mRi = new RadioInteractor(mPhone.getContext());
    }

  //GSM
    /**
     * clirMode is one of the CLIR_ constants
     */
    public synchronized Connection dial(String dialString, int clirMode, UUSInfo uusInfo,
                                        Bundle intentExtras)
            throws CallStateException {
        // note that this triggers call state changed notif
        clearDisconnected();

        if (!canDial()) {
            throw new CallStateException("cannot dial in current state");
        }

        String origNumber = dialString;
        dialString = convertNumberIfNecessary(mPhone, dialString);

        // The new call must be assigned to the foreground call.
        // That call must be idle, so place anything that's
        // there on hold
        if (mForegroundCall.getState() == GsmCdmaCall.State.ACTIVE) {
            // this will probably be done by the radio anyway
            // but the dial might fail before this happens
            // and we need to make sure the foreground call is clear
            // for the newly dialed connection
            switchWaitingOrHoldingAndActive();
            // This is a hack to delay DIAL so that it is sent out to RIL only after
            // EVENT_SWITCH_RESULT is received. We've seen failures when adding a new call to
            // multi-way conference calls due to DIAL being sent out before SWITCH is processed
            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
                // do nothing
            }

            // Fake local state so that
            // a) foregroundCall is empty for the newly dialed connection
            // b) hasNonHangupStateChanged remains false in the
            // next poll, so that we don't clear a failed dialing call
            fakeHoldForegroundBeforeDial();
        }

        if (mForegroundCall.getState() != GsmCdmaCall.State.IDLE) {
            //we should have failed in !canDial() above before we get here
            throw new CallStateException("cannot dial in current state");
        }

        mMoVideoState = intentExtras.getShort(GsmCdmaPhoneEx.VIDEO_STATE,
                (short) VideoProfile.STATE_AUDIO_ONLY);
        Rlog.d(LOG_TAG, "====callTracker-dial: videoState = " + mMoVideoState);
        setPendingMO(new GsmCdmaConnectionEx(mPhone, checkForTestEmergencyNumber(dialString),
                this, mForegroundCall, VideoProfile.isBidirectional(mMoVideoState)));
        setHangupPendingMO(false);

        if (getPendingMO().getAddress() == null || getPendingMO().getAddress().length() == 0
                || getPendingMO().getAddress().indexOf(PhoneNumberUtils.WILD) >= 0) {
            // Phone number is invalid
            getPendingMO().mCause = DisconnectCause.INVALID_NUMBER;

            // handlePollCalls() will notice this call not present
            // and will mark it as dropped.
            pollCallsWhenSafe();
        } else {
            // Always unmute when initiating a new call
            setMute(false);
            boolean supportVT = SystemProperties.getBoolean("persist.sys.csvt", false);
            Rlog.d(LOG_TAG, "====callTrack-dial supportVT:"+supportVT);

            if (mMoVideoState != VideoProfile.STATE_AUDIO_ONLY && supportVT) {
                Rlog.d(LOG_TAG, "====callTrack-dial video");
                mRi.dialVP(getPendingMO().getAddress(), null, 0, obtainCompleteMessage(), mPhone.getPhoneId());
            } else {
                Rlog.d(LOG_TAG, "====callTrack-dial audio");
                mCi.dial(getPendingMO().getAddress(), clirMode, uusInfo, obtainCompleteMessage());
            }
        }

        if (mNumberConverted) {
            getPendingMO().setConverted(origNumber);
            mNumberConverted = false;
        }

        updatePhoneStateEx();
        mPhone.notifyPreciseCallStateChanged();

        return getPendingMO();
    }

    private boolean canDial() {
        boolean ret;
        int serviceState = mPhone.getServiceState().getState();
        String disableCall = SystemProperties.get(
                TelephonyProperties.PROPERTY_DISABLE_CALL, "false");

        ret = (serviceState != ServiceState.STATE_POWER_OFF)
                && getPendingMO() == null
                && !mRingingCall.isRinging()
                && !disableCall.equals("true")
                && (!mForegroundCall.getState().isAlive()
                    || !mBackgroundCall.getState().isAlive()
                    || (!isPhoneTypeGsm()
                        && mForegroundCall.getState() == GsmCdmaCall.State.ACTIVE));

        if (!ret) {
            log(String.format("canDial is false\n" +
                            "((serviceState=%d) != ServiceState.STATE_POWER_OFF)::=%s\n" +
                            "&& pendingMO == null::=%s\n" +
                            "&& !ringingCall.isRinging()::=%s\n" +
                            "&& !disableCall.equals(\"true\")::=%s\n" +
                            "&& (!foregroundCall.getState().isAlive()::=%s\n" +
                            "   || foregroundCall.getState() == GsmCdmaCall.State.ACTIVE::=%s\n" +
                            "   ||!backgroundCall.getState().isAlive())::=%s)",
                    serviceState,
                    serviceState != ServiceState.STATE_POWER_OFF,
                            getPendingMO() == null,
                    !mRingingCall.isRinging(),
                    !disableCall.equals("true"),
                    !mForegroundCall.getState().isAlive(),
                    mForegroundCall.getState() == GsmCdmaCall.State.ACTIVE,
                    !mBackgroundCall.getState().isAlive()));
        }

        return ret;
    }

    private void fakeHoldForegroundBeforeDial() {
        List<Connection> connCopy;

        // We need to make a copy here, since fakeHoldBeforeDial()
        // modifies the lists, and we don't want to reverse the order
        connCopy = (List<Connection>) mForegroundCall.mConnections.clone();

        for (int i = 0, s = connCopy.size() ; i < s ; i++) {
            GsmCdmaConnection conn = (GsmCdmaConnection)connCopy.get(i);

            conn.fakeHoldBeforeDial();
        }
    }

    private Message obtainCompleteMessage() {
        return obtainCompleteMessage(EVENT_OPERATION_COMPLETE);
    }

    private Message obtainCompleteMessage(int what) {
        mPendingOperations++;
        mLastRelevantPoll = null;
        mNeedsPoll = true;

        if (DBG_POLL) log("obtainCompleteMessage: pendingOperations=" +
                mPendingOperations + ", needsPoll=" + mNeedsPoll);

        return obtainMessage(what);
    }

    private void operationComplete() {
        mPendingOperations--;

        if (DBG_POLL) log("operationComplete: pendingOperations=" +
                mPendingOperations + ", needsPoll=" + mNeedsPoll);

        if (mPendingOperations == 0 && mNeedsPoll) {
            mLastRelevantPoll = obtainMessage(EVENT_POLL_CALLS_RESULT);
            mCi.getCurrentCalls(mLastRelevantPoll);
        } else if (mPendingOperations < 0) {
            // this should never happen
            Rlog.e(LOG_TAG,"GsmCdmaCallTracker.pendingOperations < 0");
            mPendingOperations = 0;
        }
    }

    void fallBack() throws CallStateException {
        log("fall back");
        if (mRingingCall.getState().isRinging()) {
            try {
                mRi.fallBackVP(null, mPhone.getPhoneId());
            } catch (IllegalStateException ex) {
                log("fallBack failed");
            }
        } else {
            throw new CallStateException("phone not ringing");
        }
    }

    private boolean isPhoneTypeGsm() {
        return mPhone.getPhoneType() == PhoneConstants.PHONE_TYPE_GSM;
    }
}
