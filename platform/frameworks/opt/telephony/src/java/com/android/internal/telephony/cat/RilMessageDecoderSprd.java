/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.android.internal.telephony.cat;

import android.os.Handler;
import android.os.Message;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.util.State;
import com.android.internal.util.StateMachine;
import com.android.internal.telephony.cat.CatLog;

/**
 * Class used for queuing raw ril messages, decoding them into CommanParams
 * objects and sending the result back to the CAT Service.
 */
class RilMessageDecoderSprd extends StateMachine {

    // constants
    private static final int CMD_START = 1;
    private static final int CMD_PARAMS_READY = 2;

    // members
    private CommandParamsFactorySprd mCmdParamsFactory = null;
    private RilMessageSprd mCurrentRilMessage = null;
    private Handler mCaller = null;
    private static int mSimCount = 0;
    private static RilMessageDecoderSprd[] mInstance = null;

    // States
    private StateStart mStateStart = new StateStart();
    private StateCmdParamsReady mStateCmdParamsReady = new StateCmdParamsReady();

    /**
     * Get the singleton instance, constructing if necessary.
     *
     * @param caller
     * @param fh
     * @return RilMesssageDecoder
     */
    public static synchronized RilMessageDecoderSprd getInstance(Handler caller, IccFileHandler fh,
            int slotId) {
        if (null == mInstance) {
            mSimCount = TelephonyManager.getDefault().getSimCount();
            mInstance = new RilMessageDecoderSprd[mSimCount];
            for (int i = 0; i < mSimCount; i++) {
                mInstance[i] = null;
            }
        }

        if (slotId != SubscriptionManager.INVALID_SIM_SLOT_INDEX && slotId < mSimCount) {
            if (null == mInstance[slotId]) {
                mInstance[slotId] = new RilMessageDecoderSprd(caller, fh);
            }
        } else {
            CatLog.d("RilMessageDecoder", "invaild slot id: " + slotId);
            return null;
        }

        return mInstance[slotId];
    }

    /**
     * Start decoding the message parameters,
     * when complete MSG_ID_RIL_MSG_DECODED will be returned to caller.
     *
     * @param rilMsg
     */
    public void sendStartDecodingMessageParams(RilMessageSprd rilMsg) {
        Message msg = obtainMessage(CMD_START);
        msg.obj = rilMsg;
        sendMessage(msg);
    }

    /**
     * The command parameters have been decoded.
     *
     * @param resCode
     * @param cmdParams
     */
    public void sendMsgParamsDecoded(ResultCodeSprd resCode, CommandParamsSprd cmdParams) {
        Message msg = obtainMessage(RilMessageDecoderSprd.CMD_PARAMS_READY);
        msg.arg1 = resCode.value();
        msg.obj = cmdParams;
        sendMessage(msg);
    }

    private void sendCmdForExecution(RilMessageSprd rilMsg) {
        Message msg = mCaller.obtainMessage(CatServiceSprd.MSG_ID_RIL_MSG_DECODED,
                new RilMessageSprd(rilMsg));
        msg.sendToTarget();
    }

    private RilMessageDecoderSprd(Handler caller, IccFileHandler fh) {
        super("RilMessageDecoder");

        addState(mStateStart);
        addState(mStateCmdParamsReady);
        setInitialState(mStateStart);

        mCaller = caller;
        mCmdParamsFactory = CommandParamsFactorySprd.getInstance(this, fh);
    }

    private RilMessageDecoderSprd() {
        super("RilMessageDecoder");
    }

    private class StateStart extends State {
        @Override
        public boolean processMessage(Message msg) {
            if (msg.what == CMD_START) {
                if (decodeMessageParams((RilMessageSprd)msg.obj)) {
                    transitionTo(mStateCmdParamsReady);
                }
            } else {
                CatLog.d(this, "StateStart unexpected expecting START=" +
                         CMD_START + " got " + msg.what);
            }
            return true;
        }
    }

    private class StateCmdParamsReady extends State {
        @Override
        public boolean processMessage(Message msg) {
            if (msg.what == CMD_PARAMS_READY) {
                mCurrentRilMessage.mResCode = ResultCodeSprd.fromInt(msg.arg1);
                mCurrentRilMessage.mData = msg.obj;
                sendCmdForExecution(mCurrentRilMessage);
                transitionTo(mStateStart);
            } else {
                CatLog.d(this, "StateCmdParamsReady expecting CMD_PARAMS_READY="
                         + CMD_PARAMS_READY + " got " + msg.what);
                deferMessage(msg);
            }
            return true;
        }
    }

    private boolean decodeMessageParams(RilMessageSprd rilMsg) {
        boolean decodingStarted;

        mCurrentRilMessage = rilMsg;
        switch(rilMsg.mId) {
        case CatServiceSprd.MSG_ID_SESSION_END:
        case CatServiceSprd.MSG_ID_CALL_SETUP:
            mCurrentRilMessage.mResCode = ResultCodeSprd.OK;
            sendCmdForExecution(mCurrentRilMessage);
            decodingStarted = false;
            break;
        case CatServiceSprd.MSG_ID_PROACTIVE_COMMAND:
        case CatServiceSprd.MSG_ID_EVENT_NOTIFY:
        case CatServiceSprd.MSG_ID_REFRESH:
            byte[] rawData = null;
            try {
                rawData = IccUtils.hexStringToBytes((String) rilMsg.mData);
            } catch (Exception e) {
                // zombie messages are dropped
                CatLog.d(this, "decodeMessageParams dropping zombie messages");
                decodingStarted = false;
                break;
            }
            try {
                // Start asynch parsing of the command parameters.
                mCmdParamsFactory.make(BerTlvSprd.decode(rawData));
                decodingStarted = true;
            } catch (ResultExceptionSprd e) {
                // send to Service for proper RIL communication.
                CatLog.d(this, "decodeMessageParams: caught ResultException e=" + e);
                mCurrentRilMessage.mResCode = e.result();
                sendCmdForExecution(mCurrentRilMessage);
                decodingStarted = false;
            }
            break;
        default:
            decodingStarted = false;
            break;
        }
        return decodingStarted;
    }

    public void dispose() {
        quitNow();
        mStateStart = null;
        mStateCmdParamsReady = null;
        mCmdParamsFactory.dispose();
        mCmdParamsFactory = null;
        mCurrentRilMessage = null;
        mCaller = null;
        mInstance = null;
    }
}
