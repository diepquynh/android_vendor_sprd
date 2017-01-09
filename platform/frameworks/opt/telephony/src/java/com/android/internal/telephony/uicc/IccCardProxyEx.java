/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.internal.telephony.uicc;

import android.app.ActivityManagerNative;
import android.content.Context;
import android.content.Intent;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.UserHandle;
import android.telephony.Rlog;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.IccCardConstantsEx;
import com.android.internal.telephony.IccCardConstantsEx.State;
import com.android.internal.telephony.PhoneConstants;
import com.android.sprd.telephony.RadioInteractor;
import com.android.sprd.telephony.RadioInteractorCallbackListener;
import com.android.sprd.telephony.uicc.IccCardStatusEx;

/**
 * @Deprecated use {@link UiccController}.getUiccCard instead.
 *
 * The Phone App assumes that there is only one icc card, and one icc application
 * available at a time. Moreover, it assumes such object (represented with IccCard)
 * is available all the time (whether {@link RILConstants#RIL_REQUEST_GET_SIM_STATUS} returned
 * or not, whether card has desired application or not, whether there really is a card in the
 * slot or not).
 *
 * UiccController, however, can handle multiple instances of icc objects (multiple
 * {@link UiccCardApplication}, multiple {@link IccFileHandler}, multiple {@link IccRecords})
 * created and destroyed dynamically during phone operation.
 *
 * This class implements the IccCard interface that is always available (right after default
 * phone object is constructed) to expose the current (based on voice radio technology)
 * application on the uicc card, so that external apps won't break.
 */

public class IccCardProxyEx extends IccCardProxy {
    private static final String LOG_TAG = "IccCardProxyEx";
    private RadioInteractor mRadioInteractor;
    private RadioInteractorCallbackListener mCallbackListener;
    private State mExternalStateEx = State.UNKNOWN;
    private Context mContext;
    private Integer mPhoneId = null;
    private final Object mLock = new Object();

    public IccCardProxyEx(Context context, CommandsInterface ci, int phoneId) {
        super(context, ci, phoneId);
        mContext = context;
        mPhoneId = phoneId;
        registerForSimLocked(context, phoneId);
    }

    private void registerForSimLocked(final Context context, int phoneId) {
        context.bindService(new Intent("com.android.sprd.telephony.server.RADIOINTERACTOR_SERVICE")
            .setPackage("com.android.sprd.telephony.server"),
            new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder service) {
                    mRadioInteractor = new RadioInteractor(context);
                    mCallbackListener = new RadioInteractorCallbackListener(mPhoneId) {
                        @Override
                        public void onSimLockNotifyEvent(Message msg) {
                            setSimState(msg);
                        }
                    };
                    mRadioInteractor.listen(mCallbackListener,
                            RadioInteractorCallbackListener.LISTEN_SIMLOCK_NOTIFY_EVENT,
                            false);
                }

                @Override
                public void onServiceDisconnected(ComponentName name) {
                     mRadioInteractor.listen(mCallbackListener,
                            RadioInteractorCallbackListener.LISTEN_NONE);
                }
            }, Context.BIND_AUTO_CREATE);
    }

    private void setSimState(Message msg) {
        int simType = 0;
        State state = State.UNKNOWN;

        if (RadioInteractorCallbackListener.LISTEN_SIMLOCK_NOTIFY_EVENT != msg.what) {
            return;
        }

        AsyncResult ar = (AsyncResult)msg.obj;
        if(ar != null && ar.result != null){
            simType = (int)ar.result;
        }

        switch (simType) {
            case TelephonyManager.SIM_STATE_NETWORK_LOCKED:
                state = State.NETWORK_LOCKED;
                break;
            case IccCardStatusEx.SIM_STATE_NETWORKSUBSET_LOCKED:
                state = State.NETWORK_SUBSET_LOCKED;
                break;
            case IccCardStatusEx.SIM_STATE_SERVICEPROVIDER_LOCKED:
                state = State.SERVICE_PROVIDER_LOCKED;
                break;
            case IccCardStatusEx.SIM_STATE_CORPORATE_LOCKED:
                state = State.CORPORATE_LOCKED;
                break;
            case IccCardStatusEx.SIM_STATE_SIM_LOCKED:
                state = State.SIM_LOCKED;
                break;
            case IccCardStatusEx.SIM_STATE_NETWORK_LOCKED_PUK:
                state = State.NETWORK_LOCKED_PUK;
                break;
            case IccCardStatusEx.SIM_STATE_NETWORK_SUBSET_LOCKED_PUK:
                state = State.NETWORK_SUBSET_LOCKED_PUK;
                break;
            case IccCardStatusEx.SIM_STATE_SERVICE_PROVIDER_LOCKED_PUK:
                state = State.SERVICE_PROVIDER_LOCKED_PUK;
                break;
            case IccCardStatusEx.SIM_STATE_CORPORATE_LOCKED_PUK:
                state = State.CORPORATE_LOCKED_PUK;
                break;
            case IccCardStatusEx.SIM_STATE_SIM_LOCKED_PUK:
                state = State.SIM_LOCKED_PUK;
                break;
            case IccCardStatusEx.SIM_STATE_SIM_LOCKED_FOREVER:
                state = State.SIM_LOCKED_FOREVER;
                break;
            default:
                state = State.UNKNOWN;
                break;
        }
        setExternalStateEx(state);
    }

    private void setExternalStateEx(State newState) {
        synchronized (mLock) {
            if (mPhoneId == null || !SubscriptionManager.isValidSlotId(mPhoneId)) {
                Rlog.e(LOG_TAG, "setExternalStateEx: mPhoneId=" + mPhoneId + " is invalid; Return!!");
                return;
            }

            mExternalStateEx = newState;
            Rlog.d(LOG_TAG, "setExternalStateEx: set mPhoneId=" + mPhoneId + " newState=" + newState);
            TelephonyManager tm = (TelephonyManager) mContext.getSystemService(
                Context.TELEPHONY_SERVICE);
            tm.setSimStateForPhone(mPhoneId, getStateEx().toString());
            broadcastInternalIccStateChangedIntent(IccCardConstants.INTENT_VALUE_ICC_LOCKED,
                    getIccExStateReason(newState));

        }
    }

    private void broadcastInternalIccStateChangedIntent(String value, String reason) {
        synchronized (mLock) {
            if (mPhoneId == null) {
                Rlog.e(LOG_TAG, "broadcastInternalIccStateChangedIntent: Card Index is not set; Return!!");
                return;
            }

            Intent intent = new Intent(ACTION_INTERNAL_SIM_STATE_CHANGED);
            intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT);
            intent.putExtra(PhoneConstants.PHONE_NAME_KEY, "Phone");
            intent.putExtra(IccCardConstants.INTENT_KEY_ICC_STATE, value);
            intent.putExtra(IccCardConstants.INTENT_KEY_LOCKED_REASON, reason);
            intent.putExtra(PhoneConstants.PHONE_KEY, mPhoneId);  // SubId may not be valid.
            Rlog.d(LOG_TAG, "Sending intent ACTION_INTERNAL_SIM_STATE_CHANGED" + " for mPhoneId : " + mPhoneId);
            ActivityManagerNative.broadcastStickyIntent(intent, null, UserHandle.USER_ALL);
        }
    }

    private String getIccExStateReason(State state) {
        switch (state) {
            case NETWORK_LOCKED: return IccCardConstants.INTENT_VALUE_LOCKED_NETWORK;
            case NETWORK_SUBSET_LOCKED: return IccCardConstantsEx.INTENT_VALUE_LOCKED_NS;
            case SERVICE_PROVIDER_LOCKED: return IccCardConstantsEx.INTENT_VALUE_LOCKED_SP;
            case CORPORATE_LOCKED: return IccCardConstantsEx.INTENT_VALUE_LOCKED_CP;
            case SIM_LOCKED: return IccCardConstantsEx.INTENT_VALUE_LOCKED_SIM;
            case NETWORK_LOCKED_PUK: return IccCardConstantsEx.INTENT_VALUE_LOCKED_NS_PUK;
            case NETWORK_SUBSET_LOCKED_PUK: return IccCardConstantsEx.INTENT_VALUE_LOCKED_NS_PUK;
            case SERVICE_PROVIDER_LOCKED_PUK: return IccCardConstantsEx.INTENT_VALUE_LOCKED_SP_PUK;
            case CORPORATE_LOCKED_PUK: return IccCardConstantsEx.INTENT_VALUE_LOCKED_CP_PUK;
            case SIM_LOCKED_PUK: return IccCardConstantsEx.INTENT_VALUE_LOCKED_SIM_PUK;
            case SIM_LOCKED_FOREVER: return IccCardConstantsEx.INTENT_VALUE_LOCKED_FOREVER;
            default: return null;
       }
    }

    public State getStateEx() {
        synchronized (mLock) {
            return mExternalStateEx;
        }
    }
}
