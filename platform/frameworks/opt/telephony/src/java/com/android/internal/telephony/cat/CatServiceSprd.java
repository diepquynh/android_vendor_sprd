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

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources.NotFoundException;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemProperties;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.view.IWindowManager;
/* SPRD: add for USAT 27.22.4.25 LANGUAGE NOTIFICATION  @{ */
import android.app.ActivityManager.RunningTaskInfo;
import android.app.backup.BackupManager;
import android.app.IActivityManager;
import android.content.res.Configuration;
import java.util.List;
import java.util.Locale;
import android.app.ActivityManagerNative;
/* @}*/

import com.android.internal.telephony.cat.ResultCodeSprd;
import com.android.internal.telephony.cat.CatService;
import com.android.internal.telephony.cat.CatLog;

import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.SubscriptionController;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.uicc.UiccCard;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.IccCardStatus.CardState;
import com.android.internal.telephony.uicc.IccRefreshResponse;
import com.android.internal.telephony.uicc.UiccController;

import java.io.ByteArrayOutputStream;
import java.util.List;
import java.util.Locale;

import static com.android.internal.telephony.cat.CatCmdMessageSprd.
                   SetupEventListConstants.IDLE_SCREEN_AVAILABLE_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.
                   SetupEventListConstants.LANGUAGE_SELECTION_EVENT;
/* SPRD: add here for EVENTDOWNLOAD function @{ */
import static com.android.internal.telephony.cat.CatCmdMessageSprd.SetupEventListConstants.DATA_AVAILABLE_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.SetupEventListConstants.CHANNEL_STATUS_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.SetupEventListConstants.USER_ACTIVITY_EVENT;
/* @} */
/* SPRD: USAT case 27.22.4.16.1 @{ */
import static com.android.internal.telephony.cat.CatCmdMessageSprd.SetupEventListConstants.MT_CALL_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.SetupEventListConstants.CALL_CONNECTED_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.SetupEventListConstants.CALL_DISCONNECTED_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.SetupEventListConstants.LOCATION_STATUS_EVENT;
/* @} */
/* SPRD:for USAT 27.22.4.1.1/2 DISPLAY TEXT normal priority @{ */
import android.content.ComponentName;
import java.util.ArrayList;

/* @} */
import android.telephony.TelephonyManagerEx;

class RilMessageSprd {
    int mId;
    Object mData;
    ResultCodeSprd mResCode;

    RilMessageSprd(int msgId, String rawData) {
        mId = msgId;
        mData = rawData;
    }

    RilMessageSprd(RilMessageSprd other) {
        mId = other.mId;
        mData = other.mData;
        mResCode = other.mResCode;
    }
}

/**
 * Class that implements SIM Toolkit Telephony Service. Interacts with the RIL
 * and application.
 *
 * {@hide}
 */
public class CatServiceSprd extends CatService implements AppInterfaceSprd {
    private static final boolean DBG = false;

    // Class members
    private static IccRecords mIccRecords;
    private static UiccCardApplication mUiccApplication;

    // Service members.
    // Protects singleton instance lazy initialization.
    private static final Object sInstanceLock = new Object();
    private static CatServiceSprd[] sInstance = null;
    private CommandsInterface mCmdIf;
    private Context mContext;
    private CatCmdMessageSprd mCurrntCmd = null;
    private CatCmdMessageSprd mMenuCmd = null;

    private RilMessageDecoderSprd mMsgDecoder = null;
    private boolean mStkAppInstalled = false;

    private UiccController mUiccController;
    private CardState mCardState = CardState.CARDSTATE_ABSENT;

    // Service constants.
    protected static final int MSG_ID_SESSION_END              = 1;
    protected static final int MSG_ID_PROACTIVE_COMMAND        = 2;
    protected static final int MSG_ID_EVENT_NOTIFY             = 3;
    protected static final int MSG_ID_CALL_SETUP               = 4;
    static final int MSG_ID_REFRESH                  = 5;
    static final int MSG_ID_RESPONSE                 = 6;
    static final int MSG_ID_SIM_READY                = 7;

    protected static final int MSG_ID_ICC_CHANGED    = 8;
    protected static final int MSG_ID_ALPHA_NOTIFY   = 9;

    static final int MSG_ID_RIL_MSG_DECODED          = 10;

    // Events to signal SIM presence or absent in the device.
    private static final int MSG_ID_ICC_RECORDS_LOADED       = 20;

    //Events to signal SIM REFRESH notificatations
    private static final int MSG_ID_ICC_REFRESH  = 30;
    /* SPRD: Add here for BIP function @{ */
    private static final int MSG_ID_EVENT_DOWNLOAD = 40;
    /* @} */
    /* SPRD: Add DTMF function. @{ */
    private static final int MSG_ID_SEND_SECOND_DTMF = 50;
    private static final int MSG_ID_SEND_SERIAL_DTMF = 51;
    private static final int DTMF_INTERVAL = 2500;
    /* @} */
    private static final int MSG_ID_SETUPMENU_DELAY = 60;
    private static final int SETUPMENU_INTERVAL = 500;

    private static final int DEV_ID_KEYPAD      = 0x01;
    private static final int DEV_ID_DISPLAY     = 0x02;
    private static final int DEV_ID_UICC        = 0x81;
    private static final int DEV_ID_TERMINAL    = 0x82;
    private static final int DEV_ID_NETWORK     = 0x83;

    static final String STK_DEFAULT = "Default Message";

    private HandlerThread mHandlerThread;
    private int mSlotId;
    /* SPRD: Add here for REFRESH function @{ */
    private int mRefreshResult = 0;
    private boolean mRefreshHandled = true;
    // bug348594
    private boolean mRefreshReset = false;
    // bug374989
    private boolean mRefreshNeedTR = false;
    /* @} */

    /* SPRD: add for USAT 27.22.4.25 LANGUAGE NOTIFICATION  @{ */
    private Configuration mConfigBak = null;
    /* @}*/

    /* For multisim catservice should not be singleton */
    private CatServiceSprd(CommandsInterface ci, UiccCardApplication ca, IccRecords ir,
            Context context, IccFileHandler fh, UiccCard ic, int slotId) {
        if (ci == null || ca == null || ir == null || context == null || fh == null
                || ic == null) {
            throw new NullPointerException(
                    "Service: Input parameters must not be null");
        }
        mCmdIf = ci;
        mContext = context;
        mSlotId = slotId;
        mHandlerThread = new HandlerThread("Cat Telephony service" + slotId);
        mHandlerThread.start();

        // Get the RilMessagesDecoder for decoding the messages.
        mMsgDecoder = RilMessageDecoderSprd.getInstance(this, fh, slotId);
        if (null == mMsgDecoder) {
            CatLog.d(this, "Null RilMessageDecoder instance");
            return;
        }
        mMsgDecoder.start();

        // Register ril events handling.
        mCmdIf.setOnCatSessionEnd(this, MSG_ID_SESSION_END, null);
        mCmdIf.setOnCatProactiveCmd(this, MSG_ID_PROACTIVE_COMMAND, null);
        mCmdIf.setOnCatEvent(this, MSG_ID_EVENT_NOTIFY, null);
        mCmdIf.setOnCatCallSetUp(this, MSG_ID_CALL_SETUP, null);
        //mCmdIf.setOnSimRefresh(this, MSG_ID_REFRESH, null);

        mCmdIf.registerForIccRefresh(this, MSG_ID_ICC_REFRESH, null);
        mCmdIf.setOnCatCcAlphaNotify(this, MSG_ID_ALPHA_NOTIFY, null);

        mIccRecords = ir;
        mUiccApplication = ca;

        // Register for SIM ready event.
        mIccRecords.registerForRecordsLoaded(this, MSG_ID_ICC_RECORDS_LOADED, null);
        CatLog.d(this, "registerForRecordsLoaded slotid=" + mSlotId + " instance:" + this);


        mUiccController = UiccController.getInstance();
        mUiccController.registerForIccChanged(this, MSG_ID_ICC_CHANGED, null);

        // Check if STK application is available
        mStkAppInstalled = isStkAppInstalled();

        CatLog.d(this, "Running CAT service on Slotid: " + mSlotId +
                ". STK app installed:" + mStkAppInstalled);
    }

    /**
     * Used for instantiating the Service from the Card.
     *
     * @param ci CommandsInterface object
     * @param context phone app context
     * @param ic Icc card
     * @param slotId to know the index of card
     * @return The only Service object in the system
     */
    public static CatServiceSprd getInstance(CommandsInterface ci,
            Context context, UiccCard ic, int slotId) {
        UiccCardApplication ca = null;
        IccFileHandler fh = null;
        IccRecords ir = null;
        if (ic != null) {
            /* Since Cat is not tied to any application, but rather is Uicc application
             * in itself - just get first FileHandler and IccRecords object
             */
            ca = ic.getApplicationIndex(0);
            if (ca != null) {
                fh = ca.getIccFileHandler();
                ir = ca.getIccRecords();
            }
        }

        synchronized (sInstanceLock) {
            if (sInstance == null) {
                int simCount = TelephonyManager.getDefault().getSimCount();
                sInstance = new CatServiceSprd[simCount];
                for (int i = 0; i < simCount; i++) {
                    sInstance[i] = null;
                }
            }

            if (slotId >= sInstance.length) {
                return null;
            }

            if (sInstance[slotId] == null) {
                if (ci == null || ca == null || ir == null || context == null || fh == null
                        || ic == null) {
                    return null;
                }

                sInstance[slotId] = new CatServiceSprd(ci, ca, ir, context, fh, ic, slotId);
            } else if ((ir != null) && (mIccRecords != ir)) {
                if (mIccRecords != null) {
                    mIccRecords.unregisterForRecordsLoaded(sInstance[slotId]);
                }

                mIccRecords = ir;
                mUiccApplication = ca;

                mIccRecords.registerForRecordsLoaded(sInstance[slotId],
                        MSG_ID_ICC_RECORDS_LOADED, null);
                CatLog.d(sInstance[slotId], "registerForRecordsLoaded slotid=" + slotId
                        + " instance:" + sInstance[slotId]);
            }
            return sInstance[slotId];
        }
    }

    public void dispose() {
        if(!TelephonyManagerEx.from(mContext).isSimEnabled(mSlotId)){
            return;
        }
        synchronized (sInstanceLock) {
            CatLog.d(this, "Disposing CatService object");
            mIccRecords.unregisterForRecordsLoaded(this);

            // Clean up stk icon if dispose is called
            broadcastCardStateAndIccRefreshResp(CardState.CARDSTATE_ABSENT, null);

            mCmdIf.unSetOnCatSessionEnd(this);
            mCmdIf.unSetOnCatProactiveCmd(this);
            mCmdIf.unSetOnCatEvent(this);
            mCmdIf.unSetOnCatCallSetUp(this);
            mCmdIf.unSetOnCatCcAlphaNotify(this);

            mCmdIf.unregisterForIccRefresh(this);
            if (mUiccController != null) {
                mUiccController.unregisterForIccChanged(this);
                mUiccController = null;
            }
            mMsgDecoder.dispose();
            mMsgDecoder = null;
            mHandlerThread.quit();
            mHandlerThread = null;
            removeCallbacksAndMessages(null);
            if (sInstance != null) {
                if (SubscriptionManager.isValidSlotId(mSlotId)) {
                    sInstance[mSlotId] = null;
                } else {
                    CatLog.d(this, "error: invaild slot id: " + mSlotId);
                }
            }
        }
    }

    @Override
    protected void finalize() {
        CatLog.d(this, "Service finalized");
    }

    private void handleRilMsg(RilMessageSprd rilMsg) {
        if (rilMsg == null) {
            return;
        }

        // dispatch messages
        CommandParamsSprd cmdParams = null;
        switch (rilMsg.mId) {
        case MSG_ID_EVENT_NOTIFY:
            if (rilMsg.mResCode == ResultCodeSprd.OK) {
                cmdParams = (CommandParamsSprd) rilMsg.mData;
                if (cmdParams != null) {
                    handleCommand(cmdParams, false);
                }
            }
            break;
        case MSG_ID_PROACTIVE_COMMAND:
            try {
                cmdParams = (CommandParamsSprd) rilMsg.mData;
            } catch (ClassCastException e) {
                // for error handling : cast exception
                CatLog.d(this, "Fail to parse proactive command");
                // Don't send Terminal Resp if command detail is not available
                if (mCurrntCmd != null) {
                    sendTerminalResponse(mCurrntCmd.mCmdDet, ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD,
                                     false, 0x00, null);
                }
                break;
            }
            if (cmdParams != null) {
                if (rilMsg.mResCode == ResultCodeSprd.OK) {
                    handleCommand(cmdParams, true);
                } else {
                    // for proactive commands that couldn't be decoded
                    // successfully respond with the code generated by the
                    // message decoder.
                    sendTerminalResponse(cmdParams.mCmdDet, rilMsg.mResCode,
                            false, 0, null);
                }
            }
            break;
        case MSG_ID_REFRESH:
            cmdParams = (CommandParamsSprd) rilMsg.mData;
            if (cmdParams != null) {
                handleCommand(cmdParams, false);
            }
            break;
        case MSG_ID_SESSION_END:
            handleSessionEnd();
            break;
        case MSG_ID_CALL_SETUP:
            // prior event notify command supplied all the information
            // needed for set up call processing.
            break;
        }
    }

    /**
     * This function validates the events in SETUP_EVENT_LIST which are currently
     * supported by the Android framework. In case of SETUP_EVENT_LIST has NULL events
     * or no events, all the events need to be reset.
     */
    private boolean isSupportedSetupEventCommand(CatCmdMessageSprd cmdMsg) {
        boolean flag = true;
        // SPRD: add for STK 27.22.7.5.1
        IWindowManager wm = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));

        for (int eventVal: cmdMsg.getSetEventList().eventList) {
            CatLog.d(this,"Event: " + eventVal);
            switch (eventVal) {
                /* Currently android is supporting only the below events in SetupEventList
                 * Language Selection.  */
                case IDLE_SCREEN_AVAILABLE_EVENT:
                    /* SPRD: add for STK 27.22.7.5.1 @{ */
                    /*try {
                        wm.setEventIdleScreenNeeded(true);
                    } catch (RemoteException e) {
                        CatLog.d(this, "<" + mSlotId + ">"
                                + "Exception when set IDLE_SCREEN flag in WindowManager");
                    } catch (NullPointerException e2) {
                        CatLog.d(this, "<" + mSlotId + ">" + "wm is null");
                    }*/
                    break;
                    /* @} */
                case LANGUAGE_SELECTION_EVENT:
                    break;
                /* SPRD: add for STK 27.22.7.5.1 @{ */
                case USER_ACTIVITY_EVENT:
                    /*try {
                        wm.setEventUserActivityNeeded(true);
                    } catch (RemoteException e) {
                        CatLog.d(this, "<" + mSlotId + ">"
                                + "Exception when set USER_ACTIVITY flag in WindowManager");
                    } catch (NullPointerException e2) {
                        CatLog.d(this, "<" + mSlotId + ">" + "wm is null");
                    }*/
                    break;
                /* @} */
                /* SPRD: USAT case 27.22.4.16.1 @{ */
                case MT_CALL_EVENT:
                case CALL_CONNECTED_EVENT:
                case CALL_DISCONNECTED_EVENT:
                case LOCATION_STATUS_EVENT:
                    break;
                /* @} */
                default:
                    flag = false;
            }
        }
        return flag;
    }

    /**
     * Handles RIL_UNSOL_STK_EVENT_NOTIFY or RIL_UNSOL_STK_PROACTIVE_COMMAND command
     * from RIL.
     * Sends valid proactive command data to the application using intents.
     * RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE will be send back if the command is
     * from RIL_UNSOL_STK_PROACTIVE_COMMAND.
     */
    private void handleCommand(CommandParamsSprd cmdParams, boolean isProactiveCmd) {
        CatLog.d(this, cmdParams.getCommandType().name());

        // Log all proactive commands.
        if (isProactiveCmd) {
            if (mUiccController != null) {
                mUiccController.addCardLog("ProactiveCommand mSlotId=" + mSlotId +
                        " cmdParams=" + cmdParams);
            }
        }

        CharSequence message;
        ResultCodeSprd resultCode;
        CatCmdMessageSprd cmdMsg = new CatCmdMessageSprd(cmdParams);
        /* SPRD: Add here for BIP function @{ */
        DeviceIdentitiesSprd deviceIdentities = null;
        /* @} */
        switch (cmdParams.getCommandType()) {
            case SET_UP_MENU:
                if (removeMenu(cmdMsg.getMenu())) {
                    mMenuCmd = null;
                } else {
                    mMenuCmd = cmdMsg;
                }
                resultCode = cmdParams.mLoadIconFailed ? ResultCodeSprd.PRFRMD_ICON_NOT_DISPLAYED
                                                                            : ResultCodeSprd.OK;
                sendTerminalResponse(cmdParams.mCmdDet, resultCode, false, 0, null);
                break;
            case DISPLAY_TEXT:
                /* SPRD:for USAT 27.22.4.1.1/2 DISPLAY TEXT normal priority @{ */
                if (cmdMsg.geTextMessage().isHighPriority == false) {
                    CatLog.d(this,  "<" + mSlotId + ">" + "[stk] DISPLAY_TEXT is normal Priority");
                    boolean display_flag = isCurrentCanDisplayText();
                    CatLog.d(this,  "<" + mSlotId + ">" + "[stkapp]display_flag = " + display_flag);
                    if (!display_flag) {
                        sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.TERMINAL_CRNTLY_UNABLE_TO_PROCESS,
                                         true, AddinfoMeProblem.SCREEN_BUSY.value(), null);
                        return;
                    }
                }
                /* @} */
                break;
            case REFRESH:
                /* @orig
                 * SPRD: Remove here for REFRESH function @{
                // ME side only handles refresh commands which meant to remove IDLE
                // MODE TEXT.
                cmdParams.mCmdDet.typeOfCommand = CommandType.SET_UP_IDLE_MODE_TEXT.value();
                @} */
                /* SPRD: Add here for REFRESH function @{ */
                mCurrntCmd = cmdMsg;
                // bug374989 USAT 27.22.4.7 2.2
                if (mRefreshHandled == false) {
                    if (mCurrntCmd.mCmdDet.commandQualifier == CommandParamsFactorySprd.REFRESH_NAA_APP_RESET) {
                        CatLog.d(this, "<" + mSlotId + ">"
                                + "[stk] handleProactiveCommand: REFRESH REFRESH_NAA_APP_RESET");
                        mRefreshNeedTR = true;
                    } else {
                        handleRefreshCmdResponse(mRefreshResult);
                        mRefreshHandled = true;
                    }
                }
                /* @} */
                break;
            case SET_UP_IDLE_MODE_TEXT:
                /* SPRD: Modify for STK case 27.22.4.22.2/4 begin. @{ */
                /* @orig
                 * SPRD: Remove for STK case 27.22.4.22.2/4. @{
                sendTerminalResponse(cmdParams.mCmdDet, ResultCode.OK, false, 0, null);
                @} */
                CatLog.d(this,  "<" + mSlotId+ ">" +
                    "icon = "                 + ((DisplayTextParamsSprd)cmdParams).mTextMsg.icon +
                    " iconSelfExplanatory = " + ((DisplayTextParamsSprd)cmdParams).mTextMsg.iconSelfExplanatory +
                    " text = "                + ((DisplayTextParamsSprd)cmdParams).mTextMsg.text);
                if(((DisplayTextParamsSprd)cmdParams).mTextMsg.icon != null
                    && ((DisplayTextParamsSprd)cmdParams).mTextMsg.iconSelfExplanatory == false
                    && ((DisplayTextParamsSprd)cmdParams).mTextMsg.text == null){
                    sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD, false,0, null);
                    return;
                } else {
                    resultCode = cmdParams.mLoadIconFailed ? ResultCodeSprd.PRFRMD_ICON_NOT_DISPLAYED
                                                                            : ResultCodeSprd.OK;
                    sendTerminalResponse(cmdParams.mCmdDet, resultCode, false,0, null);
                }
                /* SPRD: Modify for STK case 27.22.4.22.2/4 end. @} */
                break;
            case SET_UP_EVENT_LIST:
                if (isSupportedSetupEventCommand(cmdMsg)) {
                    sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.OK, false, 0, null);
                } else {
                    sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY,
                            false, 0, null);
                }
                break;
            case PROVIDE_LOCAL_INFORMATION:
                ResponseDataSprd resp;
                switch (cmdParams.mCmdDet.commandQualifier) {
                    case CommandParamsFactorySprd.DTTZ_SETTING:
                        resp = new DTTZResponseDataSprd(null);
                        sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.OK, false, 0, resp);
                        break;
                    case CommandParamsFactorySprd.LANGUAGE_SETTING:
                        resp = new LanguageResponseDataSprd(Locale.getDefault().getLanguage());
                        sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.OK, false, 0, resp);
                        break;
                    default:
                        sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.OK, false, 0, null);
                }
                // No need to start STK app here.
                return;
            case LAUNCH_BROWSER:
                if ((((LaunchBrowserParamsSprd) cmdParams).mConfirmMsg.text != null)
                        && (((LaunchBrowserParamsSprd) cmdParams).mConfirmMsg.text.equals(STK_DEFAULT))) {
                    message = mContext.getText(com.android.internal.R.string.launchBrowserDefault);
                    ((LaunchBrowserParamsSprd) cmdParams).mConfirmMsg.text = message.toString();
                }
                break;
            case SELECT_ITEM:
            case GET_INPUT:
            case GET_INKEY:
                break;
            case SEND_DTMF:
                /* SPRD: Add DTMF function. @{ */
                DtmfMessage dtmfMessage = cmdMsg.getDtmfMessage();
                retrieveDtmfString(cmdParams, dtmfMessage.mdtmfString);
                break;
                /* @} */
            case SEND_SMS:
            case SEND_SS:
            case SEND_USSD:
                if ((((DisplayTextParamsSprd)cmdParams).mTextMsg.text != null)
                        && (((DisplayTextParamsSprd)cmdParams).mTextMsg.text.equals(STK_DEFAULT))) {
                    message = mContext.getText(com.android.internal.R.string.sending);
                    ((DisplayTextParamsSprd)cmdParams).mTextMsg.text = message.toString();
                }
                break;
            case PLAY_TONE:
                break;
            case SET_UP_CALL:
                if ((((CallSetupParamsSprd) cmdParams).mConfirmMsg.text != null)
                        && (((CallSetupParamsSprd) cmdParams).mConfirmMsg.text.equals(STK_DEFAULT))) {
                    message = mContext.getText(com.android.internal.R.string.SetupCallDefault);
                    ((CallSetupParamsSprd) cmdParams).mConfirmMsg.text = message.toString();
                }
                break;
            /* SPRD: Modify here for BIP function @{ */
            case OPEN_CHANNEL:
            case RECEIVE_DATA:
            case GET_CHANNEL_STATUS:
                // nothing to do telephony!
                break;
            case CLOSE_CHANNEL:
                CatLog.d(this, "<" + mSlotId + ">" + "handleCommand: CLOSE_CHANNEL");
                deviceIdentities = cmdMsg.getDeviceIdentities();
                if (deviceIdentities != null) {
                    int channelId = deviceIdentities.destinationId & 0x0f;
                    CatLog.d(this, "<" + mSlotId + ">" + "CLOSE_CHANNEL channelId = " + channelId);
                    if (channelId != AppInterfaceSprd.DEFAULT_CHANNELID) {
                        CatLog.d(this, "<" + mSlotId + ">" + "CLOSE_CHANNEL CHANNEL_ID_INVALID");
                        sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.BIP_ERROR, true,
                                AddinfoBIPProblem.CHANNEL_ID_INVALID.value(), null);
                        return;
                    }
                } else {
                    CatLog.d(this, "<" + mSlotId + ">" + "deviceIdentities is null, send CHANNEL_ID_INVALID");
                    sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.BIP_ERROR, true,
                            AddinfoBIPProblem.CHANNEL_ID_INVALID.value(), null);
                    return;
                }
                break;
            case SEND_DATA:
                CatLog.d(this, "<" + mSlotId + ">" + "handleProactiveCommand: SEND_DATA");
                deviceIdentities = cmdMsg.getDeviceIdentities();
                if (deviceIdentities != null) {
                    int channelId = deviceIdentities.destinationId & 0x0f;
                    CatLog.d(this, "<" + mSlotId + ">" + "SEND_DATA channelId = " + channelId);
                    if (channelId != AppInterfaceSprd.DEFAULT_CHANNELID) {
                        sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.BIP_ERROR, true,
                                AddinfoBIPProblem.CHANNEL_ID_INVALID.value(), null);
                        return;
                    }
                } else {
                    CatLog.d(this, "<" + mSlotId + ">" + "deviceIdentities is null, do nothing");
                }
                break;
            /* @} */
                /**
                 * @orig
                 * SPRD: Remove this code. @{
                BIPClientParams cmd = (BIPClientParams) cmdParams;
                @} */
                /* Per 3GPP specification 102.223,
                 * if the alpha identifier is not provided by the UICC,
                 * the terminal MAY give information to the user
                 * noAlphaUsrCnf defines if you need to show user confirmation or not
                 */
                /**
                 * @orig
                 * SPRD: Remove this code. @{
                boolean noAlphaUsrCnf = false;
                try {
                    noAlphaUsrCnf = mContext.getResources().getBoolean(
                            com.android.internal.R.bool.config_stkNoAlphaUsrCnf);
                } catch (NotFoundException e) {
                    noAlphaUsrCnf = false;
                }
                if ((cmd.mTextMsg.text == null) && (cmd.mHasAlphaId || noAlphaUsrCnf)) {
                    CatLog.d(this, "cmd " + cmdParams.getCommandType() + " with null alpha id");
                    // If alpha length is zero, we just respond with OK.
                    if (isProactiveCmd) {
                        sendTerminalResponse(cmdParams.mCmdDet, ResultCode.OK, false, 0, null);
                    } else if (cmdParams.getCommandType() == CommandType.OPEN_CHANNEL) {
                        mCmdIf.handleCallSetupRequestFromSim(true, null);
                    }
                    return;
                }
                // Respond with permanent failure to avoid retry if STK app is not present.
                if (!mStkAppInstalled) {
                    CatLog.d(this, "No STK application found.");
                    if (isProactiveCmd) {
                        sendTerminalResponse(cmdParams.mCmdDet,
                                             ResultCode.BEYOND_TERMINAL_CAPABILITY,
                                             false, 0, null);
                        return;
                    }
                }
                @} */
                /*
                 * CLOSE_CHANNEL, RECEIVE_DATA and SEND_DATA can be delivered by
                 * either PROACTIVE_COMMAND or EVENT_NOTIFY.
                 * If PROACTIVE_COMMAND is used for those commands, send terminal
                 * response here.
                 */
                /**
                 * @orig
                 * SPRD: Remove this code. @{
                if (isProactiveCmd &&
                    ((cmdParams.getCommandType() == CommandType.CLOSE_CHANNEL) ||
                     (cmdParams.getCommandType() == CommandType.RECEIVE_DATA) ||
                     (cmdParams.getCommandType() == CommandType.SEND_DATA))) {
                    sendTerminalResponse(cmdParams.mCmdDet, ResultCode.OK, false, 0, null);
                }
                break;
                @} */
            /* SPRD: add for USAT 27.22.4.25 LANGUAGE NOTIFICATION  @{ */
            case LANGUAGE_NOTIFACTION:
                String language = cmdMsg.getLanguageMessage().languageString;
                String country = CatLanguageDecoder.getInstance().getCountryFromLanguage(language);
                try {
                    IActivityManager am = ActivityManagerNative.getDefault();
                    Configuration config = am.getConfiguration();
                    if(country != null) {
                        mConfigBak = new Configuration(config);
                        Locale locale = new Locale(language,country);
                        config.locale = locale;
                        config.userSetLocale = true;
                        CatLog.d(this,"LANGUAGE_NOTIFACTION country = " + country + " locale = " + locale);
                    } else {
                        CatLog.d(this,"LANGUAGE_NOTIFACTION country is null");
                        if (mConfigBak != null) {
                            config = mConfigBak;
                            CatLog.d(this,"LANGUAGE_NOTIFACTION use backup config. locale = " + config.locale);
                        } else {
                            CatLog.d(this,"LANGUAGE_NOTIFACTION mConfigBak is null, do nothing");
                        }
                    }
                    am.updateConfiguration(config);
                    BackupManager.dataChanged("com.android.providers.settings");
                }catch(RemoteException e) {
                    CatLog.d(this,"LANGUAGE_NOTIFACTION exception: " + e);
                }
                sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.OK, false, 0,null);
                return;
            /* @}*/

            default:
                CatLog.d(this, "Unsupported command");
                return;
        }
        mCurrntCmd = cmdMsg;
        broadcastCatCmdIntent(cmdMsg);
    }

    /* SPRD: Add here for REFRESH function @{ */
    private void handleRefreshCmdResponse(int result) {
        CatLog.d(this, "<" + mSlotId + ">" + "handleRefreshCmdResponse enter" + " result = "
                + result);
        if (mCurrntCmd == null || (mCurrntCmd != null
                && AppInterfaceSprd.CommandTypeSprd.fromInt(mCurrntCmd.getCmdDet().typeOfCommand) != AppInterfaceSprd.CommandTypeSprd.REFRESH)) {
            CatLog.d(this, "<" + mSlotId + ">" + "[stk]handleRefreshCmdResponse mCurrntCmd is NULL");
            mRefreshResult = result;
            mRefreshHandled = false;
            return;
        }
        CommandDetailsSprd cmdDet = mCurrntCmd.getCmdDet();

        switch (AppInterfaceSprd.CommandTypeSprd.fromInt(cmdDet.typeOfCommand)) {
            case REFRESH:
                ResultCodeSprd resCode = ResultCodeSprd.OK;
                if (0 == result) {
                    resCode = ResultCodeSprd.OK;
                    // bug348594 and bug374989
                    if (mRefreshReset == true
                            && mCurrntCmd.mCmdDet.commandQualifier != CommandParamsFactorySprd.REFRESH_NAA_APP_RESET) {
                        //RemoveLastCmd();
                    } else {
                        sendTerminalResponse(cmdDet, resCode, false, 0, null);
                    }
                    mRefreshReset = false;
                } else {
                    resCode = ResultCodeSprd.TERMINAL_CRNTLY_UNABLE_TO_PROCESS;
                    sendTerminalResponse(cmdDet, resCode, true,
                            AddinfoMeProblem.SCREEN_BUSY.value(), null);
                }
                mCurrntCmd = null;
                break;
            default:
                CatLog.d(this, "<" + mSlotId + ">"
                        + "[stk]handleRefreshCmdResponse CommandType is wrong");
                return;
        }
    }
    /* @} */

    private void broadcastCatCmdIntentDelay(CatCmdMessageSprd cmdMsg) {
        Message msg = this.obtainMessage(MSG_ID_SETUPMENU_DELAY);
        msg.obj = cmdMsg;
        this.sendMessageDelayed(msg, SETUPMENU_INTERVAL);
    }

    private void broadcastCatCmdIntent(CatCmdMessageSprd cmdMsg) {
        CatLog.d(this, "broadcastCatCmdIntent isStkAppInstalled: " + isStkAppInstalled());
        if (isStkAppInstalled()) {
            Intent intent = new Intent(AppInterfaceSprd.CAT_CMD_ACTION);
            intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
            intent.putExtra("STK CMD", cmdMsg);
            intent.putExtra("SLOT_ID", mSlotId);
            CatLog.d(this, "Sending CmdMsg: " + cmdMsg + " on slotid:" + mSlotId);
            mContext.sendBroadcast(intent, AppInterfaceSprd.STK_PERMISSION);
        } else {
            broadcastCatCmdIntentDelay(cmdMsg);
        }
    }

    /**
     * Handles RIL_UNSOL_STK_SESSION_END unsolicited command from RIL.
     *
     */
    private void handleSessionEnd() {
        CatLog.d(this, "SESSION END on "+ mSlotId);

        mCurrntCmd = mMenuCmd;
        Intent intent = new Intent(AppInterfaceSprd.CAT_SESSION_END_ACTION);
        intent.putExtra("SLOT_ID", mSlotId);
        mContext.sendBroadcast(intent, AppInterfaceSprd.STK_PERMISSION);
    }


    private void sendTerminalResponse(CommandDetailsSprd cmdDet,
            ResultCodeSprd resultCode, boolean includeAdditionalInfo,
            int additionalInfo, ResponseDataSprd resp) {

        if (cmdDet == null) {
            return;
        }
        ByteArrayOutputStream buf = new ByteArrayOutputStream();

        InputSprd cmdInput = null;
        if (mCurrntCmd != null) {
            cmdInput = mCurrntCmd.geInput();
        }

        // command details
        int tag = ComprehensionTlvTagSprd.COMMAND_DETAILS.value();
        if (cmdDet.compRequired) {
            tag |= 0x80;
        }
        buf.write(tag);
        buf.write(0x03); // length
        buf.write(cmdDet.commandNumber);
        buf.write(cmdDet.typeOfCommand);
        buf.write(cmdDet.commandQualifier);

        // device identities
        // According to TS102.223/TS31.111 section 6.8 Structure of
        // TERMINAL RESPONSE, "For all SIMPLE-TLV objects with Min=N,
        // the ME should set the CR(comprehension required) flag to
        // comprehension not required.(CR=0)"
        // Since DEVICE_IDENTITIES and DURATION TLVs have Min=N,
        // the CR flag is not set.
        tag = 0x80 | ComprehensionTlvTagSprd.DEVICE_IDENTITIES.value();
        //tag = ComprehensionTlvTag.DEVICE_IDENTITIES.value();
        buf.write(tag);
        buf.write(0x02); // length
        buf.write(DEV_ID_TERMINAL); // source device id
        buf.write(DEV_ID_UICC); // destination device id

        // result
        tag = ComprehensionTlvTagSprd.RESULT.value();
        if (cmdDet.compRequired) {
            tag |= 0x80;
        }
        buf.write(tag);
        int length = includeAdditionalInfo ? 2 : 1;
        buf.write(length);
        buf.write(resultCode.value());

        // additional info
        if (includeAdditionalInfo) {
            buf.write(additionalInfo);
        }

        // Fill optional data for each corresponding command
        if (resp != null) {
            resp.format(buf);
        } else {
            encodeOptionalTags(cmdDet, resultCode, cmdInput, buf);
        }

        byte[] rawData = buf.toByteArray();
        String hexString = IccUtils.bytesToHexString(rawData);
        if (DBG) {
            CatLog.d(this, "TERMINAL RESPONSE: " + hexString);
        }

        mCmdIf.sendTerminalResponse(hexString, null);
    }

    private void encodeOptionalTags(CommandDetailsSprd cmdDet,
            ResultCodeSprd resultCode, InputSprd cmdInput, ByteArrayOutputStream buf) {
        CommandTypeSprd cmdType = AppInterfaceSprd.CommandTypeSprd.fromInt(cmdDet.typeOfCommand);
        if (cmdType != null) {
            switch (cmdType) {
                case GET_INKEY:
                    // ETSI TS 102 384,27.22.4.2.8.4.2.
                    // If it is a response for GET_INKEY command and the response timeout
                    // occured, then add DURATION TLV for variable timeout case.
                    if ((resultCode.value() == ResultCodeSprd.NO_RESPONSE_FROM_USER.value()) &&
                        (cmdInput != null) && (cmdInput.duration != null)) {
                        getInKeyResponse(buf, cmdInput);
                    }
                    break;
                case PROVIDE_LOCAL_INFORMATION:
                    if ((cmdDet.commandQualifier == CommandParamsFactorySprd.LANGUAGE_SETTING) &&
                        (resultCode.value() == ResultCodeSprd.OK.value())) {
                        getPliResponse(buf);
                    }
                    break;
                default:
                    CatLog.d(this, "encodeOptionalTags() Unsupported Cmd details=" + cmdDet);
                    break;
            }
        } else {
            CatLog.d(this, "encodeOptionalTags() bad Cmd details=" + cmdDet);
        }
    }

    private void getInKeyResponse(ByteArrayOutputStream buf, InputSprd cmdInput) {
        int tag = ComprehensionTlvTag.DURATION.value();

        buf.write(tag);
        buf.write(0x02); // length
        buf.write(cmdInput.duration.timeUnit.SECOND.value()); // Time (Unit,Seconds)
        buf.write(cmdInput.duration.timeInterval); // Time Duration
    }

    private void getPliResponse(ByteArrayOutputStream buf) {
        // Locale Language Setting
        final String lang = Locale.getDefault().getLanguage();

        if (lang != null) {
            // tag
            int tag = ComprehensionTlvTagSprd.LANGUAGE.value();
            buf.write(tag);
            ResponseDataSprd.writeLength(buf, lang.length());
            buf.write(lang.getBytes(), 0, lang.length());
        }
    }

    private void sendMenuSelection(int menuId, boolean helpRequired) {

        ByteArrayOutputStream buf = new ByteArrayOutputStream();

        // tag
        int tag = BerTlv.BER_MENU_SELECTION_TAG;
        buf.write(tag);

        // length
        buf.write(0x00); // place holder

        // device identities
        tag = 0x80 | ComprehensionTlvTagSprd.DEVICE_IDENTITIES.value();
        buf.write(tag);
        buf.write(0x02); // length
        buf.write(DEV_ID_KEYPAD); // source device id
        buf.write(DEV_ID_UICC); // destination device id

        // item identifier
        tag = 0x80 | ComprehensionTlvTagSprd.ITEM_ID.value();
        buf.write(tag);
        buf.write(0x01); // length
        buf.write(menuId); // menu identifier chosen

        // help request
        if (helpRequired) {
            tag = ComprehensionTlvTagSprd.HELP_REQUEST.value();
            buf.write(tag);
            buf.write(0x00); // length
        }

        byte[] rawData = buf.toByteArray();

        // write real length
        int len = rawData.length - 2; // minus (tag + length)
        rawData[1] = (byte) len;

        String hexString = IccUtils.bytesToHexString(rawData);

        mCmdIf.sendEnvelope(hexString, null);
    }

    private void eventDownload(int event, int sourceId, int destinationId,
            byte[] additionalInfo, boolean oneShot) {

        ByteArrayOutputStream buf = new ByteArrayOutputStream();

        // tag
        int tag = BerTlv.BER_EVENT_DOWNLOAD_TAG;
        buf.write(tag);

        // length
        buf.write(0x00); // place holder, assume length < 128.

        // event list
        tag = 0x80 | ComprehensionTlvTagSprd.EVENT_LIST.value();
        buf.write(tag);
        buf.write(0x01); // length
        buf.write(event); // event value

        // device identities
        tag = 0x80 | ComprehensionTlvTagSprd.DEVICE_IDENTITIES.value();
        buf.write(tag);
        buf.write(0x02); // length
        buf.write(sourceId); // source device id
        buf.write(destinationId); // destination device id

        /*
         * Check for type of event download to be sent to UICC - Browser
         * termination,Idle screen available, User activity, Language selection
         * etc as mentioned under ETSI TS 102 223 section 7.5
         */

        /*
         * Currently the below events are supported:
         * Language Selection Event.
         * Other event download commands should be encoded similar way
         */
        /* TODO: eventDownload should be extended for other Envelope Commands */
        switch (event) {
            case IDLE_SCREEN_AVAILABLE_EVENT:
                CatLog.d(sInstance, " Sending Idle Screen Available event download to ICC");
                break;
            case LANGUAGE_SELECTION_EVENT:
                CatLog.d(sInstance, " Sending Language Selection event download to ICC");
                tag = 0x80 | ComprehensionTlvTag.LANGUAGE.value();
                buf.write(tag);
                // Language length should be 2 byte
                buf.write(0x02);
                break;
            default:
                break;
        }

        // additional information
        if (additionalInfo != null) {
            for (byte b : additionalInfo) {
                buf.write(b);
            }
        }

        byte[] rawData = buf.toByteArray();

        // write real length
        int len = rawData.length - 2; // minus (tag + length)
        rawData[1] = (byte) len;

        String hexString = IccUtils.bytesToHexString(rawData);

        CatLog.d(this, "ENVELOPE COMMAND: " + hexString);

        mCmdIf.sendEnvelope(hexString, null);
    }

    /**
     * Used by application to get an AppInterface object.
     *
     * @return The only Service object in the system
     */
    //TODO Need to take care for MSIM
    public static AppInterfaceSprd getInstance() {
        int slotId = PhoneConstants.DEFAULT_CARD_INDEX;
        SubscriptionController sControl = SubscriptionController.getInstance();
        if (sControl != null) {
            slotId = sControl.getSlotId(sControl.getDefaultSubId());
        }
        return getInstance(null, null, null, slotId);
    }

    /**
     * Used by application to get an AppInterface object.
     *
     * @return The only Service object in the system
     */
    public static AppInterfaceSprd getInstance(int slotId) {
        return getInstance(null, null, null, slotId);
    }

    @Override
    public void handleMessage(Message msg) {
        CatLog.d(this, "handleMessage[" + msg.what + "]");

        switch (msg.what) {
        case MSG_ID_SESSION_END:
        case MSG_ID_PROACTIVE_COMMAND:
        case MSG_ID_EVENT_NOTIFY:
        case MSG_ID_REFRESH:
            CatLog.d(this, "ril message arrived,slotid:" + mSlotId);
            String data = null;
            if (msg.obj != null) {
                AsyncResult ar = (AsyncResult) msg.obj;
                if (ar != null && ar.result != null) {
                    try {
                        data = (String) ar.result;
                    } catch (ClassCastException e) {
                        break;
                    }
                }
            }
            mMsgDecoder.sendStartDecodingMessageParams(new RilMessageSprd(msg.what, data));
            break;
        case MSG_ID_CALL_SETUP:
            mMsgDecoder.sendStartDecodingMessageParams(new RilMessageSprd(msg.what, null));
            break;
        case MSG_ID_ICC_RECORDS_LOADED:
            break;
        case MSG_ID_RIL_MSG_DECODED:
            handleRilMsg((RilMessageSprd) msg.obj);
            break;
        case MSG_ID_RESPONSE:
            handleCmdResponse((CatResponseMessageSprd) msg.obj);
            break;
        case MSG_ID_ICC_CHANGED:
            CatLog.d(this, "MSG_ID_ICC_CHANGED");
            updateIccAvailability();
            break;
        case MSG_ID_ICC_REFRESH:
            if (msg.obj != null) {
                AsyncResult ar = (AsyncResult) msg.obj;
                if (ar != null && ar.result != null) {
                    /* SPRD: Add here for REFRESH function @{ */
                    IccRefreshResponse response = (IccRefreshResponse) ar.result;
                    CatLog.d(this, "<" + mSlotId + ">" + "[stk]MSG_ID_ICC_REFRESH result = "
                            + response.refreshResult);
                    if (3 != response.refreshResult) {
                        // bug348594
                        if (2 == response.refreshResult) {
                            mRefreshReset = true;
                        }
                        handleRefreshCmdResponse(0);// Success
                    } else {
                        handleRefreshCmdResponse(1);// Fail
                    }
                    /* @} */
                    broadcastCardStateAndIccRefreshResp(CardState.CARDSTATE_PRESENT,
                                  (IccRefreshResponse) ar.result);
                } else {
                    CatLog.d(this,"Icc REFRESH with exception: " + ar.exception);
                }
            } else {
                CatLog.d(this, "IccRefresh Message is null");
            }
            break;
        case MSG_ID_ALPHA_NOTIFY:
            CatLog.d(this, "Received CAT CC Alpha message from card");
            if (msg.obj != null) {
                AsyncResult ar = (AsyncResult) msg.obj;
                if (ar != null && ar.result != null) {
                    broadcastAlphaMessage((String)ar.result);
                } else {
                    CatLog.d(this, "CAT Alpha message: ar.result is null");
                }
            } else {
                CatLog.d(this, "CAT Alpha message: msg.obj is null");
            }
            break;
        /* SPRD: Add DTMF function. @{ */
        case MSG_ID_SEND_SECOND_DTMF:
            CommandParamsSprd cmdParams = (CommandParamsSprd) msg.obj;
            String str = msg.getData().getString("dtmf");
            retrieveDtmfString(cmdParams, str);
            break;
        case MSG_ID_SEND_SERIAL_DTMF:
            AsyncResult dtmfAr = (AsyncResult) msg.obj;
            Message msgAr = (Message) dtmfAr.userObj;
            CommandParamsSprd cmdParamAr = (CommandParamsSprd) msgAr.obj;
            String arStr = msgAr.getData().getString("dtmf");
            retrieveDtmfString(cmdParamAr, arStr);
            break;
        /* @} */
        case MSG_ID_SETUPMENU_DELAY:
            CatCmdMessageSprd cmdMsg = (CatCmdMessageSprd)msg.obj;
            broadcastCatCmdIntent(cmdMsg) ;
            break;
        default:
            throw new AssertionError("Unrecognized CAT command: " + msg.what);
        }
    }

    /**
     ** This function sends a CARD status (ABSENT, PRESENT, REFRESH) to STK_APP.
     ** This is triggered during ICC_REFRESH or CARD STATE changes. In case
     ** REFRESH, additional information is sent in 'refresh_result'
     **
     **/
    private void  broadcastCardStateAndIccRefreshResp(CardState cardState,
            IccRefreshResponse iccRefreshState) {
        Intent intent = new Intent(AppInterfaceSprd.CAT_ICC_STATUS_CHANGE);
        boolean cardPresent = (cardState == CardState.CARDSTATE_PRESENT);

        if (iccRefreshState != null) {
            //This case is when MSG_ID_ICC_REFRESH is received.
            intent.putExtra(AppInterfaceSprd.REFRESH_RESULT, iccRefreshState.refreshResult);
            CatLog.d(this, "Sending IccResult with Result: "
                    + iccRefreshState.refreshResult);
        }

        // This sends an intent with CARD_ABSENT (0 - false) /CARD_PRESENT (1 - true).
        intent.putExtra(AppInterfaceSprd.CARD_STATUS, cardPresent);
        /* SPRD: SIM standby changed install/uninstall  support @{*/
        intent.putExtra("SLOT_ID", mSlotId);
        CatLog.d(this,"SLOT_ID ="+mSlotId);
        /* @} */

        CatLog.d(this, "Sending Card Status: "
                + cardState + " " + "cardPresent: " + cardPresent);
        mContext.sendBroadcast(intent, AppInterfaceSprd.STK_PERMISSION);
    }

    private void broadcastAlphaMessage(String alphaString) {
        CatLog.d(this, "Broadcasting CAT Alpha message from card: " + alphaString);
        Intent intent = new Intent(AppInterfaceSprd.CAT_ALPHA_NOTIFY_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
        intent.putExtra(AppInterfaceSprd.ALPHA_STRING, alphaString);
        intent.putExtra("SLOT_ID", mSlotId);
        mContext.sendBroadcast(intent, AppInterfaceSprd.STK_PERMISSION);
    }

    @Override
    public synchronized void onCmdResponse(CatResponseMessageSprd resMsg) {
        if (resMsg == null) {
            return;
        }
        // queue a response message.
        Message msg = obtainMessage(MSG_ID_RESPONSE, resMsg);
        msg.sendToTarget();
    }

    private boolean validateResponse(CatResponseMessageSprd resMsg) {
        boolean validResponse = false;
        if ((resMsg.mCmdDet.typeOfCommand == CommandTypeSprd.SET_UP_EVENT_LIST.value())
                || (resMsg.mCmdDet.typeOfCommand == CommandTypeSprd.SET_UP_MENU.value())) {
            CatLog.d(this, "CmdType: " + resMsg.mCmdDet.typeOfCommand);
            validResponse = true;
        } else if (mCurrntCmd != null) {
            validResponse = resMsg.mCmdDet.compareTo(mCurrntCmd.mCmdDet);
            CatLog.d(this, "isResponse for last valid cmd: " + validResponse);
        }
        return validResponse;
    }

    private boolean removeMenu(Menu menu) {
        try {
            if (menu.items.size() == 1 && menu.items.get(0) == null) {
                return true;
            }
        } catch (NullPointerException e) {
            CatLog.d(this, "Unable to get Menu's items size");
            return true;
        }
        return false;
    }

    private void handleCmdResponse(CatResponseMessageSprd resMsg) {
        // Make sure the response details match the last valid command. An invalid
        // response is a one that doesn't have a corresponding proactive command
        // and sending it can "confuse" the baseband/ril.
        // One reason for out of order responses can be UI glitches. For example,
        // if the application launch an activity, and that activity is stored
        // by the framework inside the history stack. That activity will be
        // available for relaunch using the latest application dialog
        // (long press on the home button). Relaunching that activity can send
        // the same command's result again to the CatService and can cause it to
        // get out of sync with the SIM. This can happen in case of
        // non-interactive type Setup Event List and SETUP_MENU proactive commands.
        // Stk framework would have already sent Terminal Response to Setup Event
        // List and SETUP_MENU proactive commands. After sometime Stk app will send
        // Envelope Command/Event Download. In which case, the response details doesn't
        // match with last valid command (which are not related).
        // However, we should allow Stk framework to send the message to ICC.
        if (!validateResponse(resMsg)) {
            return;
        }
        ResponseDataSprd resp = null;
        boolean helpRequired = false;
        CommandDetailsSprd cmdDet = resMsg.getCmdDetails();
        AppInterfaceSprd.CommandTypeSprd type = AppInterfaceSprd.CommandTypeSprd.fromInt(cmdDet.typeOfCommand);

        switch (resMsg.mResCode) {
        case HELP_INFO_REQUIRED:
            helpRequired = true;
            // fall through
        case OK:
        case PRFRMD_WITH_PARTIAL_COMPREHENSION:
        case PRFRMD_WITH_MISSING_INFO:
        case PRFRMD_WITH_ADDITIONAL_EFS_READ:
        case PRFRMD_ICON_NOT_DISPLAYED:
        case PRFRMD_MODIFIED_BY_NAA:
        case PRFRMD_LIMITED_SERVICE:
        case PRFRMD_WITH_MODIFICATION:
        case PRFRMD_NAA_NOT_ACTIVE:
        case PRFRMD_TONE_NOT_PLAYED:
        case LAUNCH_BROWSER_ERROR:
        case TERMINAL_CRNTLY_UNABLE_TO_PROCESS:
            switch (type) {
            case SET_UP_MENU:
                helpRequired = resMsg.mResCode == ResultCodeSprd.HELP_INFO_REQUIRED;
                sendMenuSelection(resMsg.mUsersMenuSelection, helpRequired);
                return;
            case SELECT_ITEM:
                resp = new SelectItemResponseDataSprd(resMsg.mUsersMenuSelection);
                break;
            case GET_INPUT:
            case GET_INKEY:
                InputSprd input = mCurrntCmd.geInput();
                if (!input.yesNo) {
                    // when help is requested there is no need to send the text
                    // string object.
                    if (!helpRequired) {
                        resp = new GetInkeyInputResponseDataSprd(resMsg.mUsersInput,
                                input.ucs2, input.packed);
                    }
                } else {
                    resp = new GetInkeyInputResponseDataSprd(
                            resMsg.mUsersYesNoSelection);
                }
                break;
            case DISPLAY_TEXT:
                if (resMsg.mResCode == ResultCodeSprd.TERMINAL_CRNTLY_UNABLE_TO_PROCESS) {
                    // For screenbusy case there will be addtional information in the terminal
                    // response. And the value of the additional information byte is 0x01.
                    resMsg.setAdditionalInfo(0x01);
                } else {
                    resMsg.mIncludeAdditionalInfo = false;
                    resMsg.mAdditionalInfo = 0;
                }
                break;
            case LAUNCH_BROWSER:
                break;
            // 3GPP TS.102.223: Open Channel alpha confirmation should not send TR
            /* SPRD: Modify here for BIP function @{ */
            case OPEN_CHANNEL:
                if (resMsg.mResCode == ResultCodeSprd.TERMINAL_CRNTLY_UNABLE_TO_PROCESS) {
                    CatLog.d(this, "<" + mSlotId + ">" + "OPEN_CHANNEL RES TERMINAL_CRNTLY_UNABLE_TO_PROCESS");
                    resMsg.mIncludeAdditionalInfo = true;
                    resMsg.setAdditionalInfo(AddinfoMeProblem.BUSY_ON_CALL.value());
                } else {
                    CatLog.d(this, "<" + mSlotId + ">" + "OPEN_CHANNEL RES OK");
                }
                resp = new OpenChannelResponseDataSprd(resMsg.BearerType, resMsg.BearerParam,
                        resMsg.bufferSize, resMsg.ChannelId, resMsg.LinkStatus);

                break;
            case SEND_DATA:
                CatLog.d(this, "<" + mSlotId + ">" + "SEND_DATA RES OK");
                resp = new SendDataResponseDataSprd(resMsg.channelDataLen);
                break;
            case RECEIVE_DATA:
                CatLog.d(this, "<" + mSlotId + ">" + "RECEIVE_DATA RES OK");
                resp = new ReceiveDataResponseDataSprd(resMsg.channelDataLen,
                        resMsg.channelData);
                break;
            case GET_CHANNEL_STATUS:
                CatLog.d(this, "<" + mSlotId + ">" + "GET_CHANNEL_STATUS RES OK");
                resp = new ChannelStatusResponseDataSprd(resMsg.ChannelId, resMsg.LinkStatus);
                break;
            /* @} */
            case SET_UP_CALL:
                mCmdIf.handleCallSetupRequestFromSim(resMsg.mUsersConfirm, null);
                // No need to send terminal response for SET UP CALL. The user's
                // confirmation result is send back using a dedicated ril message
                // invoked by the CommandInterface call above.
                mCurrntCmd = null;
                return;
            case SET_UP_EVENT_LIST:
                // SPRD: add for STK 27.22.7.5.1
                IWindowManager wm = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));
                if (IDLE_SCREEN_AVAILABLE_EVENT == resMsg.mEventValue) {
                    /* SPRD: add for STK 27.22.7.5.1 @{ */
                    /*try {
                        wm.setEventIdleScreenNeeded(false);
                    } catch (RemoteException e) {
                        CatLog.d(this, "<" + mSlotId + ">" + "Exception when set IDLE_SCREEN_AVAILABLE_EVENT flag in WM");
                    } catch (NullPointerException e2) {
                        CatLog.d(this, "<" + mSlotId + ">" + "wm is null");
                    }*/
                    /* @}*/
                    eventDownload(resMsg.mEventValue, DEV_ID_DISPLAY, DEV_ID_UICC,
                            resMsg.mAddedInfo, false);
                } else {
                    /* SPRD: add here for EVENTDOWNLOAD function @{ */
                    if (resMsg.mAddedInfo == null) {
                        byte[] addedInfo = null;
                        ByteArrayOutputStream buf = new ByteArrayOutputStream();
                        int tag;
                        switch (resMsg.mEventValue) {
                            case DATA_AVAILABLE_EVENT:
                                tag = 0x80 | ComprehensionTlvTagSprd.CHANNEL_STATUS.value();
                                buf.write(tag); // tag
                                buf.write(2); // length
                                buf.write(resMsg.ChannelId | (resMsg.LinkStatus ? 0x80 : 0));
                                buf.write(resMsg.mMode);
                                tag = 0x80 | ComprehensionTlvTagSprd.CHANNEL_DATA_LENGTH.value();
                                buf.write(tag);
                                buf.write(1);
                                buf.write(resMsg.channelDataLen);
                                addedInfo = buf.toByteArray();
                                break;
                            case CHANNEL_STATUS_EVENT:
                                tag = 0x80 | ComprehensionTlvTagSprd.CHANNEL_STATUS.value();
                                buf.write(tag); // tag
                                buf.write(2); // length
                                buf.write(resMsg.ChannelId | (resMsg.LinkStatus ? 0x80 : 0));
                                buf.write(resMsg.mMode);
                                addedInfo = buf.toByteArray();
                                break;
                            /* SPRD: add for STK 27.22.7.5.1 @{ */
                            case USER_ACTIVITY_EVENT:
                                /*try {
                                    wm.setEventUserActivityNeeded(false);
                                } catch (RemoteException e) {
                                    CatLog.d(this, "<" + mSlotId + ">" + "Exception when set USER_ACTIVITY_EVENT flag in WM");
                                } catch (NullPointerException e2) {
                                    CatLog.d(this, "<" + mSlotId + ">" + "wm is null");
                                }*/
                                break;
                            /* @} */
                            default:
                                CatLog.d(this, "<" + mSlotId + ">" + "unknown event");
                                return;
                        }
                        eventDownload(resMsg.mEventValue, DEV_ID_TERMINAL, DEV_ID_UICC,
                                addedInfo, false);
                    } else {
                        eventDownload(resMsg.mEventValue, DEV_ID_TERMINAL, DEV_ID_UICC,
                                resMsg.mAddedInfo, false);
                    }
                }
                /* @} */
                // No need to send the terminal response after event download.
                return;
            default:
                break;
            }
            break;
        case BACKWARD_MOVE_BY_USER:
        case USER_NOT_ACCEPT:
            /**
             * SPRD: Change the way to achieve and add BIP. @{
             * @orig
             *
            // if the user dismissed the alert dialog for a
            // setup call/open channel, consider that as the user
            // rejecting the call. Use dedicated API for this, rather than
            // sending a terminal response.
            if (type == CommandType.SET_UP_CALL || type == CommandType.OPEN_CHANNEL) {
                mCmdIf.handleCallSetupRequestFromSim(false, null);
                mCurrntCmd = null;
                return;
            } else {
                resp = null;
            }
            break;
            @} */

            /* SPRD: Change the way to achieve and add BIP. @{ */
            switch (type) {
            case SET_UP_CALL:
                mCmdIf.handleCallSetupRequestFromSim(false, null);
                mCurrntCmd = null;
                return;
            case OPEN_CHANNEL:
                CatLog.d(this, "<" + mSlotId + ">" + "OPEN_CHANNEL USER_NOT_ACCEPT");
                resp = new OpenChannelResponseDataSprd(resMsg.BearerType, resMsg.BearerParam,
                        resMsg.bufferSize, resMsg.ChannelId, resMsg.LinkStatus);
                break;
            default:
                resp = null;
                break;
            }
            break;
            /* @} */
        case NO_RESPONSE_FROM_USER:
        case UICC_SESSION_TERM_BY_USER:
            resp = null;
            break;
        /* SPRD: Modify here for BIP function @{ */
        case BEYOND_TERMINAL_CAPABILITY:
            switch (type) {
            case OPEN_CHANNEL:
                CatLog.d(this, "<" + mSlotId + ">" + "OPEN_CHANNEL BEYOND_TERMINAL_CAPABILITY");
                resp = new OpenChannelResponseDataSprd(resMsg.BearerType, resMsg.BearerParam,
                        resMsg.bufferSize, resMsg.ChannelId, resMsg.LinkStatus);
                break;
            case SEND_DATA:
                CatLog.d(this, "<" + mSlotId + ">" + "SEND_DATA BEYOND_TERMINAL_CAPABILITY");
                resMsg.setAdditionalInfo(AddinfoBIPProblem.TRANSPORT_LEVEL_NOT_AVAILABLE.value());
                break;
            case RECEIVE_DATA:
                CatLog.d(this, "<" + mSlotId + ">" + "RECEIVE_DATA BEYOND_TERMINAL_CAPABILITY");
                resMsg.setAdditionalInfo(AddinfoBIPProblem.NO_SPECIFIC_CAUSE.value());
                break;
            }
            break;
        case BIP_ERROR:
            switch (type) {
            case SEND_DATA:
                CatLog.d(this, "<" + mSlotId + ">" + "SEND_DATA BIP_ERROR");
                resMsg.setAdditionalInfo(AddinfoBIPProblem.CHANNEL_ID_INVALID.value());
                break;
            case CLOSE_CHANNEL:
                CatLog.d(this, "<" + mSlotId + ">" + "CLOSE_CHANNEL BIP_ERROR");
                resMsg.setAdditionalInfo(AddinfoBIPProblem.CHANNEL_CLOSED.value());
                break;
            }
            break;
        /* @} */
        default:
            return;
        }
        sendTerminalResponse(cmdDet, resMsg.mResCode, resMsg.mIncludeAdditionalInfo,
                resMsg.mAdditionalInfo, resp);
        mCurrntCmd = null;
    }

    private boolean isStkAppInstalled() {
        Intent intent = new Intent(AppInterfaceSprd.CAT_CMD_ACTION);
        PackageManager pm = mContext.getPackageManager();
        List<ResolveInfo> broadcastReceivers =
                            pm.queryBroadcastReceivers(intent, PackageManager.GET_META_DATA);
        int numReceiver = broadcastReceivers == null ? 0 : broadcastReceivers.size();

        return (numReceiver > 0);
    }

    public void update(CommandsInterface ci,
            Context context, UiccCard ic) {
        UiccCardApplication ca = null;
        IccRecords ir = null;

        if (ic != null) {
            /* Since Cat is not tied to any application, but rather is Uicc application
             * in itself - just get first FileHandler and IccRecords object
             */
            ca = ic.getApplicationIndex(0);
            if (ca != null) {
                ir = ca.getIccRecords();
            }
        }

        synchronized (sInstanceLock) {
            if ((ir != null) && (mIccRecords != ir)) {
                if (mIccRecords != null) {
                    mIccRecords.unregisterForRecordsLoaded(this);
                }

                CatLog.d(this,
                        "Reinitialize the Service with SIMRecords and UiccCardApplication");
                mIccRecords = ir;
                mUiccApplication = ca;

                // re-Register for SIM ready event.
                mIccRecords.registerForRecordsLoaded(this, MSG_ID_ICC_RECORDS_LOADED, null);
                CatLog.d(this, "registerForRecordsLoaded slotid=" + mSlotId + " instance:" + this);
            }
        }
    }

    void updateIccAvailability() {
        if (null == mUiccController) {
            return;
        }

        CardState newState = CardState.CARDSTATE_ABSENT;
        UiccCard newCard = mUiccController.getUiccCard(mSlotId);
        if (newCard != null) {
            newState = newCard.getCardState();
        }
        CardState oldState = mCardState;
        mCardState = newState;
        CatLog.d(this,"New Card State = " + newState + " " + "Old Card State = " + oldState);
        if (oldState == CardState.CARDSTATE_PRESENT &&
                newState != CardState.CARDSTATE_PRESENT) {
            if(!TelephonyManagerEx.from(mContext).isSimEnabled(mSlotId)){
                return;
            }
            broadcastCardStateAndIccRefreshResp(newState, null);
        } else if (oldState != CardState.CARDSTATE_PRESENT &&
                newState == CardState.CARDSTATE_PRESENT) {
            // Card moved to PRESENT STATE.
            mCmdIf.reportStkServiceIsRunning(null);
            /* SPRD: Add here for REFRESH function @{ */
            if (mRefreshHandled == false && mRefreshNeedTR == true) {
                handleRefreshCmdResponse(mRefreshResult);
                mRefreshHandled = true;
                mRefreshNeedTR = false;
            }
            /* @} */
        }
    }

    /* SPRD: Add DTMF function. @{ */
    private void retrieveDtmfString(CommandParamsSprd cmdParams, String dtmf) {
        if (phoneIsIdle()) {
            sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.TERMINAL_CRNTLY_UNABLE_TO_PROCESS,
                    true, AddinfoMeProblem.NOT_IN_SPEECH_CALL.value(), null);
        } else {
            String dtmfTemp = new String(dtmf);
            CatLog.d(this, "dtmfTemp = " + dtmfTemp);

            if (dtmfTemp != null && dtmfTemp.length() > 0) {
                String firstStr = dtmfTemp.substring(0, 1);
                Message msg = new Message();
                Bundle bundle = new Bundle();
                bundle.putString("dtmf", dtmf.substring(1, dtmf.length()));
                msg.what = MSG_ID_SEND_SECOND_DTMF;
                msg.obj = cmdParams;
                msg.setData(bundle);

                if (firstStr.equals("P")) {
                    this.sendMessageDelayed(msg, DTMF_INTERVAL);
                    return;
                } else {
                    mCmdIf.sendDtmf(firstStr.charAt(0), obtainMessage(MSG_ID_SEND_SERIAL_DTMF, msg));
                }
            } else {
                sendTerminalResponse(cmdParams.mCmdDet, ResultCodeSprd.OK, false, 0, null);
            }
        }
    }

    public boolean isIdleBySubId(int subId) {
        final Phone phone = PhoneFactory.getPhone(subId);
        if (phone != null) {
            return (phone.getState() == PhoneConstants.State.IDLE);
        } else {
            return false;
        }
    }

    private boolean phoneIsIdle() {
        boolean isIdle = true;
        for (int i = 0; i < TelephonyManager.getDefault().getSimCount(); i++) {
            isIdle = isIdleBySubId(i);
            if (false == isIdle) {
                return isIdle;
            }
        }
        return isIdle;
    }

    /* SPRD:for USAT 27.22.4.1.1/2 DISPLAY TEXT normal priority @{ */
    private boolean isCurrentCanDisplayText() {
        try {
            List<RunningTaskInfo> mRunningTaskInfoList = (List<RunningTaskInfo>)ActivityManagerNative.getDefault().getTasks(1, 0);
            if(null == mRunningTaskInfoList || mRunningTaskInfoList.isEmpty()){
                CatLog.d(this, "<" + mSlotId+ ">" + "mRunningTaskInfoList is NULL!");
                return false;
            }
            int mListSize = mRunningTaskInfoList.size();
            CatLog.d(this, "<" + mSlotId+ ">" + "[stk]isCurrentCanDisplayText trace mListSize = " + mListSize);
            if(mListSize > 0) {
                ComponentName cn = mRunningTaskInfoList.get(0).topActivity;
                CatLog.d(this, "<" + mSlotId + ">" + "[stk]isCurrentCanDisplayText cn is " + cn);
                boolean result = ((cn.getClassName().indexOf("com.android.stk") != -1))
                        || isHome(cn);
                return result;
            }
        } catch (RemoteException e) {
            CatLog.d(this, "<" + mSlotId+ ">" + "[stk]isCurrentCanDisplayText exception");
        }
        return false;
    }
    private List<String> getHomes() {
        List<String> names = new ArrayList<String>();
        PackageManager packageManager = mContext.getPackageManager();
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_HOME);
        List<ResolveInfo> resolveInfo = packageManager.queryIntentActivities(
                intent, PackageManager.MATCH_DEFAULT_ONLY);
        for (ResolveInfo ri : resolveInfo) {
            names.add(ri.activityInfo.packageName);
            System.out.println(ri.activityInfo.packageName);
        }
        return names;
    }

    public boolean isHome(ComponentName m) {
        String packagename = m.getPackageName();
        List<String> name = getHomes();
        for (String i : name) {
            if (packagename.equals(i))
                return true;
        }
        return false;
    }
    /* @} */

}
