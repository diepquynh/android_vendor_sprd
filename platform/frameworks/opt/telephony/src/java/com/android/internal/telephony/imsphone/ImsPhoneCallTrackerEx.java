/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.internal.telephony.imsphone;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.provider.Settings;
import android.preference.PreferenceManager;
import android.telecom.ConferenceParticipant;
import android.telecom.VideoProfile;
import android.telephony.DisconnectCause;
import android.telephony.PhoneNumberUtils;
import android.telephony.Rlog;
import android.telephony.ServiceState;

import com.android.ims.ImsCall;
import com.android.ims.ImsCallProfile;
import com.android.ims.ImsConfig;
import com.android.ims.ImsConnectionStateListener;
import com.android.ims.ImsEcbm;
import com.android.ims.ImsException;
import com.android.ims.ImsManager;
import com.android.ims.ImsReasonInfo;
import com.android.ims.ImsServiceClass;
import com.android.ims.ImsUtInterface;
import com.android.ims.internal.IImsVideoCallProvider;
import com.android.ims.internal.ImsVideoCallProviderWrapper;
import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallStateException;
import com.android.internal.telephony.CallTracker;
import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.TelephonyProperties;
import com.android.internal.telephony.GsmCdmaPhone;
import com.android.ims.internal.ImsCallSession;

/**
 * {@hide}
 */
public final class ImsPhoneCallTrackerEx extends ImsPhoneCallTracker {
    static final String LOG_TAG = "ImsPhoneCallTrackerEx";
    //***** Constants
    private static final int EVENT_HANGUP_PENDINGMO = 18;
    private static final int EVENT_RESUME_BACKGROUND = 19;
    private static final int EVENT_DIAL_PENDINGMO = 20;

    private static final int TIMEOUT_HANGUP_PENDINGMO = 500;

    private Object mSyncHold = new Object();
    private int mServiceId = 0;
    private ImsManager mImsManager;


    //***** Constructors
    public ImsPhoneCallTrackerEx(ImsPhone phone) {
        super(phone);
    }

    @Override
    public Connection
    dial(String dialString, int videoState, Bundle intentExtras) throws CallStateException {
        boolean isConferenceDial =false;
        if(intentExtras != null){
            isConferenceDial= intentExtras.getBoolean("android.intent.extra.IMS_CONFERENCE_REQUEST",false);
        }
        log("dial-> isConferenceDial: " + isConferenceDial + " intentExtras:"+intentExtras);
        if(isConferenceDial){
            return dialConferenceCall(dialString, intentExtras);
        }
        return super.dial(dialString, videoState, intentExtras);
    }

    private void
    internalClearDisconnected() {
        mRingingCall.clearDisconnected();
        mForegroundCall.clearDisconnected();
        mBackgroundCall.clearDisconnected();
        mHandoverCall.clearDisconnected();
    }

    private void setVideoCallProvider(ImsPhoneConnection conn, ImsCall imsCall)
            throws RemoteException {
        IImsVideoCallProvider imsVideoCallProvider =
                imsCall.getCallSession().getVideoCallProvider();
        if (imsVideoCallProvider != null) {
            ImsVideoCallProviderWrapper imsVideoCallProviderWrapper =
                    new ImsVideoCallProviderWrapper(imsVideoCallProvider);
            conn.setVideoProvider(imsVideoCallProviderWrapper);
        }
    }

    /* SPRD: Add for VoLTE @{ */
    public Connection
    dialConferenceCall(String dialString, Bundle extras) throws CallStateException {
        boolean isPhoneInEcmMode = SystemProperties.getBoolean(
                TelephonyProperties.PROPERTY_INECM_MODE, false);
        boolean isEmergencyNumber = PhoneNumberUtils.isEmergencyNumber(dialString);

        // note that this triggers call state changed notif
        clearDisconnected();
        mImsManager = ImsManager.getInstance(mPhone.getContext(), mPhone.getPhoneId());
        mServiceId = mPhone.getPhoneId() + 1;

        if (mImsManager == null) {
            throw new CallStateException("service not available");
        }

        if (isPhoneInEcmMode && isEmergencyNumber) {
            log("dialConferenceCall->isPhoneInEcmMode:" + isPhoneInEcmMode);
        }
        String[] callees = extras.getStringArray("android.intent.extra.IMS_CONFERENCE_PARTICIPANTS");
        if(callees == null){
            throw new CallStateException("dialConferenceCall->callees is null!");
        }

        log("dialConferenceCall->mForegroundCall.getState(): " + mForegroundCall.getState());
        if (mForegroundCall.getState() == ImsPhoneCall.State.ACTIVE
                || mForegroundCall.getState() == ImsPhoneCall.State.DIALING
                || mForegroundCall.getState() == ImsPhoneCall.State.ALERTING) {
            if (!mForegroundCall.isMultiparty()) {
                throw new CallStateException("can not add participant to normal call");
            }
            //add participant
            ImsCall imsCall = mForegroundCall.getImsCall();
            if (imsCall == null) {
                throw new CallStateException("can not add participant: No foreground ims call!");
            } else {
                ImsCallSession imsCallSession = imsCall.getCallSession();
                if (imsCallSession != null) {
                    imsCallSession.inviteParticipants(callees);
                    return null;
                } else {
                    throw new CallStateException("can not add participant: ImsCallSession does not exist!");
                }
            }
        }

        ImsPhoneCall.State fgState = ImsPhoneCall.State.IDLE;
        ImsPhoneCall.State bgState = ImsPhoneCall.State.IDLE;

        synchronized (mSyncHold) {
            setPendingMO(new ImsPhoneConnectionEx(mPhone,
                checkForTestEmergencyNumber(dialString), this, mForegroundCall, isEmergencyNumber));
        }
        addConnectionEx(getPendingMO());

        if ((!isPhoneInEcmMode) || (isPhoneInEcmMode && isEmergencyNumber)) {
            if (getPendingMO() == null) {
                return null;
            }

            if (getPendingMO().getAddress()== null || getPendingMO().getAddress().length() == 0
                    || getPendingMO().getAddress().indexOf(PhoneNumberUtils.WILD) >= 0) {
                // Phone number is invalid
                getPendingMO().setDisconnectCause(DisconnectCause.INVALID_NUMBER);
                sendEmptyMessageDelayed(EVENT_HANGUP_PENDINGMO, TIMEOUT_HANGUP_PENDINGMO);
                return null;
            }

            setMute(false);
            int serviceType = PhoneNumberUtils.isEmergencyNumber(getPendingMO().getAddress()) ?
                    ImsCallProfile.SERVICE_TYPE_EMERGENCY : ImsCallProfile.SERVICE_TYPE_NORMAL;

            try {
                ImsCallProfile profile = mImsManager.createCallProfile(mServiceId,
                        serviceType, ImsCallProfile.CALL_TYPE_VOICE);

                ImsCall imsCall = mImsManager.makeCall(mServiceId, profile,
                        callees, getImsCallListener());
                getPendingMO().setImsCall(imsCall);

                setVideoCallProvider(getPendingMO(), imsCall);
            } catch (ImsException e) {
                loge("dialInternal : " + e);
                getPendingMO().setDisconnectCause(DisconnectCause.ERROR_UNSPECIFIED);
                sendEmptyMessageDelayed(EVENT_HANGUP_PENDINGMO, TIMEOUT_HANGUP_PENDINGMO);
            } catch (RemoteException e) {
            }
        } else {
            try {
                getEcbmInterface().exitEmergencyCallbackMode();
            } catch (ImsException e) {
                e.printStackTrace();
                throw new CallStateException("service not available");
            }
            mPhone.setOnEcbModeExitResponse(this, EVENT_EXIT_ECM_RESPONSE_CDMA, null);
        }

        updatePhoneStateEx();
        mPhone.notifyPreciseCallStateChanged();

        return getPendingMO();
    }


    private int getDisconnectCauseFromReasonInfo(ImsReasonInfo reasonInfo) {
        int cause = DisconnectCause.ERROR_UNSPECIFIED;

        //int type = reasonInfo.getReasonType();
        int code = reasonInfo.getCode();
        switch (code) {
            case ImsReasonInfo.CODE_SIP_BAD_ADDRESS:
            case ImsReasonInfo.CODE_SIP_NOT_REACHABLE:
                return DisconnectCause.NUMBER_UNREACHABLE;

            case ImsReasonInfo.CODE_SIP_BUSY:
                return DisconnectCause.BUSY;

            case ImsReasonInfo.CODE_USER_TERMINATED:
                return DisconnectCause.LOCAL;

            case ImsReasonInfo.CODE_LOCAL_CALL_DECLINE:
            case ImsReasonInfo.CODE_USER_DECLINE:             //SPRD: Add for casue set error when reject incomming call
                return DisconnectCause.INCOMING_REJECTED;

            case ImsReasonInfo.CODE_USER_TERMINATED_BY_REMOTE:
                return DisconnectCause.NORMAL;

            case ImsReasonInfo.CODE_SIP_REDIRECTED:
            case ImsReasonInfo.CODE_SIP_BAD_REQUEST:
            case ImsReasonInfo.CODE_SIP_FORBIDDEN:
            case ImsReasonInfo.CODE_SIP_NOT_ACCEPTABLE:
            case ImsReasonInfo.CODE_SIP_USER_REJECTED:
            case ImsReasonInfo.CODE_SIP_GLOBAL_ERROR:
                return DisconnectCause.SERVER_ERROR;

            case ImsReasonInfo.CODE_SIP_SERVICE_UNAVAILABLE:
            case ImsReasonInfo.CODE_SIP_NOT_FOUND:
            case ImsReasonInfo.CODE_SIP_SERVER_ERROR:
                return DisconnectCause.SERVER_UNREACHABLE;

            case ImsReasonInfo.CODE_LOCAL_NETWORK_ROAMING:
            case ImsReasonInfo.CODE_LOCAL_NETWORK_IP_CHANGED:
            case ImsReasonInfo.CODE_LOCAL_IMS_SERVICE_DOWN:
            case ImsReasonInfo.CODE_LOCAL_SERVICE_UNAVAILABLE:
            case ImsReasonInfo.CODE_LOCAL_NOT_REGISTERED:
            case ImsReasonInfo.CODE_LOCAL_NETWORK_NO_LTE_COVERAGE:
            case ImsReasonInfo.CODE_LOCAL_NETWORK_NO_SERVICE:
            case ImsReasonInfo.CODE_LOCAL_CALL_VCC_ON_PROGRESSING:
                return DisconnectCause.OUT_OF_SERVICE;

            case ImsReasonInfo.CODE_SIP_REQUEST_TIMEOUT:
            case ImsReasonInfo.CODE_TIMEOUT_1XX_WAITING:
            case ImsReasonInfo.CODE_TIMEOUT_NO_ANSWER:
            case ImsReasonInfo.CODE_TIMEOUT_NO_ANSWER_CALL_UPDATE:
                return DisconnectCause.TIMED_OUT;

            case ImsReasonInfo.CODE_LOCAL_LOW_BATTERY:
            case ImsReasonInfo.CODE_LOCAL_POWER_OFF:
                return DisconnectCause.POWER_OFF;

            case ImsReasonInfo.CODE_FDN_BLOCKED:
                return DisconnectCause.FDN_BLOCKED;
            default:
        }

        return cause;
    }

}
