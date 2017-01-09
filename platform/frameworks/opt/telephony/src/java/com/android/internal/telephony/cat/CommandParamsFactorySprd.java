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

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Message;

import com.android.internal.telephony.GsmAlphabet;
import com.android.internal.telephony.uicc.IccFileHandler;
/* SPRD: add here for REFRESH function @{ */
import android.content.res.Resources;
import com.android.internal.R;
/* @} */
/* SPRD: Add here for BIP function @{ */
import com.android.internal.telephony.cat.bip.*;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.cat.CatLog;
/* @{ */

import java.util.Iterator;
import java.util.List;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.
                   SetupEventListConstants.USER_ACTIVITY_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.
                   SetupEventListConstants.IDLE_SCREEN_AVAILABLE_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.
                   SetupEventListConstants.LANGUAGE_SELECTION_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.
                   SetupEventListConstants.BROWSER_TERMINATION_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.
                   SetupEventListConstants.BROWSING_STATUS_EVENT;
import static com.android.internal.telephony.cat.CatCmdMessageSprd.
SetupEventListConstants.LOCATION_STATUS_EVENT;
/**
 * Factory class, used for decoding raw byte arrays, received from baseband,
 * into a CommandParams object.
 *
 */
class CommandParamsFactorySprd extends Handler {
    private static CommandParamsFactorySprd sInstance = null;
    private IconLoader mIconLoader;
    private CommandParamsSprd mCmdParams = null;
    private int mIconLoadState = LOAD_NO_ICON;
    private RilMessageDecoderSprd mCaller = null;
    private boolean mloadIcon = false;

    // constants
    static final int MSG_ID_LOAD_ICON_DONE = 1;

    // loading icons state parameters.
    static final int LOAD_NO_ICON           = 0;
    static final int LOAD_SINGLE_ICON       = 1;
    static final int LOAD_MULTI_ICONS       = 2;

    // Command Qualifier values for refresh command
    static final int REFRESH_NAA_INIT_AND_FULL_FILE_CHANGE  = 0x00;
    static final int REFRESH_NAA_INIT_AND_FILE_CHANGE       = 0x02;
    static final int REFRESH_NAA_INIT                       = 0x03;
    static final int REFRESH_UICC_RESET                     = 0x04;
    /* SPRD: Add here for REFRESH function @{ */
    static final int REFRESH_FILE_CHANGE                    = 0x01;
    static final int REFRESH_NAA_APP_RESET                  = 0x05;
    static final int REFRESH_NAA_SESSION_RESET              = 0x06;
    /* @} */
    static final int REFRESH_UICC_PLMN_CHANGE               = 0x07;

    // Command Qualifier values for PLI command
    static final int DTTZ_SETTING                           = 0x03;
    static final int LANGUAGE_SETTING                       = 0x04;

    // As per TS 102.223 Annex C, Structure of CAT communications,
    // the APDU length can be max 255 bytes. This leaves only 239 bytes for user
    // input string. CMD details TLV + Device IDs TLV + Result TLV + Other
    // details of TextString TLV not including user input take 16 bytes.
    //
    // If UCS2 encoding is used, maximum 118 UCS2 chars can be encoded in 238 bytes.
    // Each UCS2 char takes 2 bytes. Byte Order Mask(BOM), 0xFEFF takes 2 bytes.
    //
    // If GSM 7 bit default(use 8 bits to represent a 7 bit char) format is used,
    // maximum 239 chars can be encoded in 239 bytes since each char takes 1 byte.
    //
    // No issues for GSM 7 bit packed format encoding.

    private static final int MAX_GSM7_DEFAULT_CHARS = 239;
    private static final int MAX_UCS2_CHARS = 118;

    static synchronized CommandParamsFactorySprd getInstance(RilMessageDecoderSprd caller,
            IccFileHandler fh) {
        if (sInstance != null) {
            return sInstance;
        }
        if (fh != null) {
            return new CommandParamsFactorySprd(caller, fh);
        }
        return null;
    }

    private CommandParamsFactorySprd(RilMessageDecoderSprd caller, IccFileHandler fh) {
        mCaller = caller;
        mIconLoader = IconLoader.getInstance(this, fh);
    }

    private CommandDetailsSprd processCommandDetails(List<ComprehensionTlvSprd> ctlvs) {
        CommandDetailsSprd cmdDet = null;

        if (ctlvs != null) {
            // Search for the Command Details object.
            ComprehensionTlvSprd ctlvCmdDet = searchForTag(
                    ComprehensionTlvTagSprd.COMMAND_DETAILS, ctlvs);
            if (ctlvCmdDet != null) {
                try {
                    cmdDet = ValueParserSprd.retrieveCommandDetails(ctlvCmdDet);
                } catch (ResultExceptionSprd e) {
                    CatLog.d(this,
                            "processCommandDetails: Failed to procees command details e=" + e);
                }
            }
        }
        return cmdDet;
    }

    void make(BerTlvSprd berTlv) {
        if (berTlv == null) {
            return;
        }
        // reset global state parameters.
        mCmdParams = null;
        mIconLoadState = LOAD_NO_ICON;
        // only proactive command messages are processed.
        if (berTlv.getTag() != BerTlvSprd.BER_PROACTIVE_COMMAND_TAG) {
            sendCmdParams(ResultCodeSprd.CMD_TYPE_NOT_UNDERSTOOD);
            return;
        }
        boolean cmdPending = false;
        List<ComprehensionTlvSprd> ctlvs = berTlv.getComprehensionTlvs();
        // process command dtails from the tlv list.
        CommandDetailsSprd cmdDet = processCommandDetails(ctlvs);
        if (cmdDet == null) {
            sendCmdParams(ResultCodeSprd.CMD_TYPE_NOT_UNDERSTOOD);
            return;
        }

        // extract command type enumeration from the raw value stored inside
        // the Command Details object.
        AppInterfaceSprd.CommandTypeSprd cmdType = AppInterfaceSprd.CommandTypeSprd
                .fromInt(cmdDet.typeOfCommand);
        if (cmdType == null) {
            // This PROACTIVE COMMAND is presently not handled. Hence set
            // result code as BEYOND_TERMINAL_CAPABILITY in TR.
            mCmdParams = new CommandParamsSprd(cmdDet);
            sendCmdParams(ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY);
            return;
        }

        // proactive command length is incorrect.
        if (!berTlv.isLengthValid()) {
            mCmdParams = new CommandParamsSprd(cmdDet);
            sendCmdParams(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
            return;
        }

        try {
            switch (cmdType) {
            case SET_UP_MENU:
                cmdPending = processSelectItem(cmdDet, ctlvs);
                break;
            case SELECT_ITEM:
                cmdPending = processSelectItem(cmdDet, ctlvs);
                break;
            case DISPLAY_TEXT:
                cmdPending = processDisplayText(cmdDet, ctlvs);
                break;
             case SET_UP_IDLE_MODE_TEXT:
                 cmdPending = processSetUpIdleModeText(cmdDet, ctlvs);
                 break;
             case GET_INKEY:
                cmdPending = processGetInkey(cmdDet, ctlvs);
                break;
             case GET_INPUT:
                 cmdPending = processGetInput(cmdDet, ctlvs);
                 break;
             case SEND_DTMF:
                /* SPRD: Add DTMF function. @{ */
                cmdPending = processDtmfNotify(cmdDet, ctlvs);
                break;
                /* @} */
             case SEND_SMS:
             case SEND_SS:
             case SEND_USSD:
                 cmdPending = processEventNotify(cmdDet, ctlvs);
                 break;
             /* SPRD: Modify here for BIP function @{ */
             case GET_CHANNEL_STATUS:
                 processGetChannelStatus(cmdDet, ctlvs);
                 break;
             /* @{ */
             case SET_UP_CALL:
                 cmdPending = processSetupCall(cmdDet, ctlvs);
                 break;
             case REFRESH:
                processRefresh(cmdDet, ctlvs);
                cmdPending = false;
                break;
             case LAUNCH_BROWSER:
                 cmdPending = processLaunchBrowser(cmdDet, ctlvs);
                 break;
             case PLAY_TONE:
                cmdPending = processPlayTone(cmdDet, ctlvs);
                break;
             case SET_UP_EVENT_LIST:
                 cmdPending = processSetUpEventList(cmdDet, ctlvs);
                 break;
             case PROVIDE_LOCAL_INFORMATION:
                cmdPending = processProvideLocalInfo(cmdDet, ctlvs);
                break;
             /* SPRD: Modify here for BIP function @{ */
             case OPEN_CHANNEL:
                 cmdPending = processOpenChannel(cmdDet, ctlvs);
                 break;
             case CLOSE_CHANNEL:
                 cmdPending = processCloseChannel(cmdDet, ctlvs);
                 break;
             case RECEIVE_DATA:
                 cmdPending = processReceiveData(cmdDet, ctlvs);
                 break;
             case SEND_DATA:
                 //cmdPending = processBIPClient(cmdDet, ctlvs);
                 cmdPending = processSendData(cmdDet, ctlvs);
                 break;
             /* @} */
             /* SPRD: add for USAT 27.22.4.25 LANGUAGE NOTIFICATION  @{ */
             case LANGUAGE_NOTIFACTION:
                processLanguageNotify(cmdDet, ctlvs);
                break;
             /* @}*/
            default:
                // unsupported proactive commands
                mCmdParams = new CommandParamsSprd(cmdDet);
                sendCmdParams(ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY);
                return;
            }
        } catch (ResultExceptionSprd e) {
            CatLog.d(this, "make: caught ResultException e=" + e);
            mCmdParams = new CommandParamsSprd(cmdDet);
            sendCmdParams(e.result());
            return;
        }
        if (!cmdPending) {
            sendCmdParams(ResultCodeSprd.OK);
        }
    }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
        case MSG_ID_LOAD_ICON_DONE:
            sendCmdParams(setIcons(msg.obj));
            break;
        }
    }

    private ResultCodeSprd setIcons(Object data) {
        Bitmap[] icons = null;
        int iconIndex = 0;
        if (data == null) {
            CatLog.d(this, "Optional Icon data is NULL");
            mCmdParams.mLoadIconFailed = true;
            mloadIcon = false;
            /** In case of icon load fail consider the
            ** received proactive command as valid (sending RESULT OK) as
            ** The result code, 'PRFRMD_ICON_NOT_DISPLAYED' will be added in the
            ** terminal response by CatService/StkAppService if needed based on
            ** the value of mLoadIconFailed.
            */
            return ResultCodeSprd.OK;
        }
        switch(mIconLoadState) {
        case LOAD_SINGLE_ICON:
            mCmdParams.setIcon((Bitmap) data);
            break;
        case LOAD_MULTI_ICONS:
            icons = (Bitmap[]) data;
            // set each item icon.
            for (Bitmap icon : icons) {
                mCmdParams.setIcon(icon);
                if (icon == null && mloadIcon) {
                    CatLog.d(this, "Optional Icon data is NULL while loading multi icons");
                    mCmdParams.mLoadIconFailed = true;
                }
            }
            break;
        }
        return ResultCodeSprd.OK;
    }

    private void sendCmdParams(ResultCodeSprd resCode) {
        mCaller.sendMsgParamsDecoded(resCode, mCmdParams);
    }

    /**
     * Search for a COMPREHENSION-TLV object with the given tag from a list
     *
     * @param tag A tag to search for
     * @param ctlvs List of ComprehensionTlv objects used to search in
     *
     * @return A ComprehensionTlv object that has the tag value of {@code tag}.
     *         If no object is found with the tag, null is returned.
     */
    private ComprehensionTlvSprd searchForTag(ComprehensionTlvTagSprd tag,
            List<ComprehensionTlvSprd> ctlvs) {
        Iterator<ComprehensionTlvSprd> iter = ctlvs.iterator();
        return searchForNextTag(tag, iter);
    }

    /**
     * Search for the next COMPREHENSION-TLV object with the given tag from a
     * list iterated by {@code iter}. {@code iter} points to the object next to
     * the found object when this method returns. Used for searching the same
     * list for similar tags, usually item id.
     *
     * @param tag A tag to search for
     * @param iter Iterator for ComprehensionTlv objects used for search
     *
     * @return A ComprehensionTlv object that has the tag value of {@code tag}.
     *         If no object is found with the tag, null is returned.
     */
    private ComprehensionTlvSprd searchForNextTag(ComprehensionTlvTagSprd tag,
            Iterator<ComprehensionTlvSprd> iter) {
        int tagValue = tag.value();
        while (iter.hasNext()) {
            ComprehensionTlvSprd ctlv = iter.next();
            if (ctlv.getTag() == tagValue) {
                return ctlv;
            }
        }
        return null;
    }

    /**
     * Processes DISPLAY_TEXT proactive command from the SIM card.
     *
     * @param cmdDet Command Details container object.
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     * @return true if the command is processing is pending and additional
     *         asynchronous processing is required.
     * @throws ResultException
     */
    private boolean processDisplayText(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs)
            throws ResultExceptionSprd {

        CatLog.d(this, "process DisplayText");

        TextMessage textMsg = new TextMessage();
        IconIdSprd iconId = null;

        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.TEXT_STRING,
                ctlvs);
        if (ctlv != null) {
            textMsg.text = ValueParserSprd.retrieveTextString(ctlv);
        }
        // If the tlv object doesn't exist or the it is a null object reply
        // with command not understood.
        if (textMsg.text == null) {
            throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
        }

        ctlv = searchForTag(ComprehensionTlvTagSprd.IMMEDIATE_RESPONSE, ctlvs);
        if (ctlv != null) {
            textMsg.responseNeeded = false;
        }
        // parse icon identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            textMsg.iconSelfExplanatory = iconId.selfExplanatory;
        }
        // parse tone duration
        ctlv = searchForTag(ComprehensionTlvTagSprd.DURATION, ctlvs);
        if (ctlv != null) {
            textMsg.duration = ValueParserSprd.retrieveDuration(ctlv);
        }

        // Parse command qualifier parameters.
        textMsg.isHighPriority = (cmdDet.commandQualifier & 0x01) != 0;
        textMsg.userClear = (cmdDet.commandQualifier & 0x80) != 0;

        mCmdParams = new DisplayTextParamsSprd(cmdDet, textMsg);

        if (iconId != null) {
            mloadIcon = true;
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

    /**
     * Processes SET_UP_IDLE_MODE_TEXT proactive command from the SIM card.
     *
     * @param cmdDet Command Details container object.
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     * @return true if the command is processing is pending and additional
     *         asynchronous processing is required.
     * @throws ResultException
     */
    private boolean processSetUpIdleModeText(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {

        CatLog.d(this, "process SetUpIdleModeText");

        TextMessage textMsg = new TextMessage();
        IconIdSprd iconId = null;

        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.TEXT_STRING,
                ctlvs);
        if (ctlv != null) {
            textMsg.text = ValueParserSprd.retrieveTextString(ctlv);
        }

        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            textMsg.iconSelfExplanatory = iconId.selfExplanatory;
        }

        /*
         * If the tlv object doesn't contain text and the icon is not self
         * explanatory then reply with command not understood.
         */

        if (textMsg.text == null && iconId != null && !textMsg.iconSelfExplanatory) {
            throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
        }
        mCmdParams = new DisplayTextParamsSprd(cmdDet, textMsg);

        if (iconId != null) {
            mloadIcon = true;
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

    /**
     * Processes GET_INKEY proactive command from the SIM card.
     *
     * @param cmdDet Command Details container object.
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     * @return true if the command is processing is pending and additional
     *         asynchronous processing is required.
     * @throws ResultException
     */
    private boolean processGetInkey(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {

        CatLog.d(this, "process GetInkey");

        InputSprd input = new InputSprd();
        IconIdSprd iconId = null;

        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.TEXT_STRING,
                ctlvs);
        if (ctlv != null) {
            input.text = ValueParserSprd.retrieveTextString(ctlv);
        } else {
            throw new ResultExceptionSprd(ResultCodeSprd.REQUIRED_VALUES_MISSING);
        }
        // parse icon identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            // SPRD: add here for USAT case 27.22.4.3.6
            input.iconSelfExplanatory = iconId.selfExplanatory;
        }

        // parse duration
        ctlv = searchForTag(ComprehensionTlvTagSprd.DURATION, ctlvs);
        if (ctlv != null) {
            input.duration = ValueParserSprd.retrieveDuration(ctlv);
        }

        input.minLen = 1;
        input.maxLen = 1;

        input.digitOnly = (cmdDet.commandQualifier & 0x01) == 0;
        input.ucs2 = (cmdDet.commandQualifier & 0x02) != 0;
        input.yesNo = (cmdDet.commandQualifier & 0x04) != 0;
        input.helpAvailable = (cmdDet.commandQualifier & 0x80) != 0;
        input.echo = true;

        mCmdParams = new GetInputParamsSprd(cmdDet, input);

        if (iconId != null) {
            mloadIcon = true;
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

    /**
     * Processes GET_INPUT proactive command from the SIM card.
     *
     * @param cmdDet Command Details container object.
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     * @return true if the command is processing is pending and additional
     *         asynchronous processing is required.
     * @throws ResultException
     */
    private boolean processGetInput(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {

        CatLog.d(this, "process GetInput");

        InputSprd input = new InputSprd();
        IconIdSprd iconId = null;

        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.TEXT_STRING,
                ctlvs);
        if (ctlv != null) {
            input.text = ValueParserSprd.retrieveTextString(ctlv);
        } else {
            throw new ResultExceptionSprd(ResultCodeSprd.REQUIRED_VALUES_MISSING);
        }

        ctlv = searchForTag(ComprehensionTlvTagSprd.RESPONSE_LENGTH, ctlvs);
        if (ctlv != null) {
            try {
                byte[] rawValue = ctlv.getRawValue();
                int valueIndex = ctlv.getValueIndex();
                input.minLen = rawValue[valueIndex] & 0xff;
                input.maxLen = rawValue[valueIndex + 1] & 0xff;
            } catch (IndexOutOfBoundsException e) {
                throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
            }
        } else {
            throw new ResultExceptionSprd(ResultCodeSprd.REQUIRED_VALUES_MISSING);
        }

        ctlv = searchForTag(ComprehensionTlvTagSprd.DEFAULT_TEXT, ctlvs);
        if (ctlv != null) {
            input.defaultText = ValueParserSprd.retrieveTextString(ctlv);
        }
        // parse icon identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            // SPRD: add here for USAT case 27.22.4.3.6
            input.iconSelfExplanatory = iconId.selfExplanatory;
        }

        input.digitOnly = (cmdDet.commandQualifier & 0x01) == 0;
        input.ucs2 = (cmdDet.commandQualifier & 0x02) != 0;
        input.echo = (cmdDet.commandQualifier & 0x04) == 0;
        input.packed = (cmdDet.commandQualifier & 0x08) != 0;
        input.helpAvailable = (cmdDet.commandQualifier & 0x80) != 0;

        // Truncate the maxLen if it exceeds the max number of chars that can
        // be encoded. Limit depends on DCS in Command Qualifier.
        if (input.ucs2 && input.maxLen > MAX_UCS2_CHARS) {
            CatLog.d(this, "UCS2: received maxLen = " + input.maxLen +
                  ", truncating to " + MAX_UCS2_CHARS);
            input.maxLen = MAX_UCS2_CHARS;
        } else if (!input.packed && input.maxLen > MAX_GSM7_DEFAULT_CHARS) {
            CatLog.d(this, "GSM 7Bit Default: received maxLen = " + input.maxLen +
                  ", truncating to " + MAX_GSM7_DEFAULT_CHARS);
            input.maxLen = MAX_GSM7_DEFAULT_CHARS;
        }

        mCmdParams = new GetInputParamsSprd(cmdDet, input);

        if (iconId != null) {
            mloadIcon = true;
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

    /**
     * Processes REFRESH proactive command from the SIM card.
     *
     * @param cmdDet Command Details container object.
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     */
    private boolean processRefresh(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) {

        CatLog.d(this, "process Refresh");

        // REFRESH proactive command is rerouted by the baseband and handled by
        // the telephony layer. IDLE TEXT should be removed for a REFRESH command
        // with "initialization" or "reset"
        switch (cmdDet.commandQualifier) {
        case REFRESH_NAA_INIT_AND_FULL_FILE_CHANGE:
        case REFRESH_NAA_INIT_AND_FILE_CHANGE:
        case REFRESH_NAA_INIT:
        case REFRESH_UICC_RESET:
        /* SPRD: Add here for REFRESH @{ */
        case REFRESH_FILE_CHANGE:
        case REFRESH_NAA_APP_RESET:
        case REFRESH_NAA_SESSION_RESET:
        case REFRESH_UICC_PLMN_CHANGE:
            TextMessage textMsg = new TextMessage();
            Resources r = Resources.getSystem();
            textMsg.text = r.getString(R.string.stk_refresh_sim_init_message);
            //mCmdParams = new DisplayTextParams(cmdDet, null);
            mCmdParams = new DisplayTextParamsSprd(cmdDet, textMsg);
            break;
        default:
            CatLog.d(this, "process Refresh: wrong commandQualifier");
            break;
        /* @} */
        }
        return false;
    }

    /**
     * Processes SELECT_ITEM proactive command from the SIM card.
     *
     * @param cmdDet Command Details container object.
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     * @return true if the command is processing is pending and additional
     *         asynchronous processing is required.
     * @throws ResultException
     */
    private boolean processSelectItem(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {

        CatLog.d(this, "process SelectItem");

        Menu menu = new Menu();
        IconIdSprd titleIconId = null;
        ItemsIconIdSprd itemsIconId = null;
        Iterator<ComprehensionTlvSprd> iter = ctlvs.iterator();

        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.ALPHA_ID,
                ctlvs);
        if (ctlv != null) {
            menu.title = ValueParserSprd.retrieveAlphaId(ctlv);
        }

        while (true) {
            ctlv = searchForNextTag(ComprehensionTlvTagSprd.ITEM, iter);
            if (ctlv != null) {
                /* SPRD: Don't to add the NULL item to menu for SELECT_ITEM only @{ */
                if (AppInterfaceSprd.CommandTypeSprd.fromInt(cmdDet.typeOfCommand) == AppInterfaceSprd.CommandTypeSprd.SELECT_ITEM) {
                    Item item = ValueParserSprd.retrieveItem(ctlv);
                    if (item != null) {
                        menu.items.add(item);
                    }
                } else {
                    menu.items.add(ValueParserSprd.retrieveItem(ctlv));
                }
                /* @} */
            } else {
                break;
            }
        }

        // We must have at least one menu item.
        if (menu.items.size() == 0) {
            throw new ResultExceptionSprd(ResultCodeSprd.REQUIRED_VALUES_MISSING);
        }

        ctlv = searchForTag(ComprehensionTlvTagSprd.ITEM_ID, ctlvs);
        if (ctlv != null) {
            // CAT items are listed 1...n while list start at 0, need to
            // subtract one.
            menu.defaultItem = ValueParserSprd.retrieveItemId(ctlv) - 1;
        }

        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            mIconLoadState = LOAD_SINGLE_ICON;
            titleIconId = ValueParserSprd.retrieveIconId(ctlv);
            menu.titleIconSelfExplanatory = titleIconId.selfExplanatory;
        }

        ctlv = searchForTag(ComprehensionTlvTagSprd.ITEM_ICON_ID_LIST, ctlvs);
        if (ctlv != null) {
            mIconLoadState = LOAD_MULTI_ICONS;
            itemsIconId = ValueParserSprd.retrieveItemsIconId(ctlv);
            menu.itemsIconSelfExplanatory = itemsIconId.selfExplanatory;
        }

        boolean presentTypeSpecified = (cmdDet.commandQualifier & 0x01) != 0;
        if (presentTypeSpecified) {
            if ((cmdDet.commandQualifier & 0x02) == 0) {
                menu.presentationType = PresentationType.DATA_VALUES;
            } else {
                menu.presentationType = PresentationType.NAVIGATION_OPTIONS;
            }
        }
        menu.softKeyPreferred = (cmdDet.commandQualifier & 0x04) != 0;
        menu.helpAvailable = (cmdDet.commandQualifier & 0x80) != 0;

        mCmdParams = new SelectItemParamsSprd(cmdDet, menu, titleIconId != null);

        // Load icons data if needed.
        switch(mIconLoadState) {
        case LOAD_NO_ICON:
            return false;
        case LOAD_SINGLE_ICON:
            mloadIcon = true;
            mIconLoader.loadIcon(titleIconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            break;
        case LOAD_MULTI_ICONS:
            int[] recordNumbers = itemsIconId.recordNumbers;
            if (titleIconId != null) {
                // Create a new array for all the icons (title and items).
                recordNumbers = new int[itemsIconId.recordNumbers.length + 1];
                recordNumbers[0] = titleIconId.recordNumber;
                System.arraycopy(itemsIconId.recordNumbers, 0, recordNumbers,
                        1, itemsIconId.recordNumbers.length);
            }
            mloadIcon = true;
            mIconLoader.loadIcons(recordNumbers, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            break;
        }
        return true;
    }

    /* SPRD: Add DTMF function. @{ */
    private boolean processDtmfNotify(CommandDetailsSprd cmdDet, List<ComprehensionTlvSprd> ctlvs)
            throws ResultExceptionSprd {

        CatLog.d(this, "process SPRD DtmfNotify");

        String dtmfString = null;
        TextMessage textMsg = new TextMessage();
        IconIdSprd iconId = null;

        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.DTMF, ctlvs);
        if (ctlv != null) {
            dtmfString = ValueParserSprd.retrieveDTMF(ctlv);
            CatLog.d(this, "dtmfString is: " + dtmfString);
        } else {
            CatLog.d(this, "processDtmfNotify and ctlv = null");
            throw new ResultExceptionSprd(ResultCodeSprd.REQUIRED_VALUES_MISSING);
        }

        ctlv = searchForTag(ComprehensionTlvTagSprd.ALPHA_ID, ctlvs);
        if (ctlv != null) {
            textMsg.text = ValueParserSprd.retrieveAlphaId(ctlv);
        }

        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            textMsg.iconSelfExplanatory = iconId.selfExplanatory;
        }

        textMsg.responseNeeded = false;
        mCmdParams = new DtmfParamsSprd(cmdDet, textMsg, dtmfString);

        if (iconId != null) {
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this.obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }
    /* @} */

    /**
     * Processes EVENT_NOTIFY message from baseband.
     *
     * @param cmdDet Command Details container object.
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     * @return true if the command is processing is pending and additional
     *         asynchronous processing is required.
     */
    private boolean processEventNotify(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {

        CatLog.d(this, "process EventNotify");

        TextMessage textMsg = new TextMessage();
        IconIdSprd iconId = null;

        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.ALPHA_ID,
                ctlvs);

        if (ctlv != null) {
            textMsg.text = ValueParserSprd.retrieveAlphaId(ctlv);
        } else {
            textMsg.text = null;
            CatLog.d(this, "alpha id null, set text to null");
        }
        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            textMsg.iconSelfExplanatory = iconId.selfExplanatory;
        }

        textMsg.responseNeeded = false;
        mCmdParams = new DisplayTextParamsSprd(cmdDet, textMsg);

        if (iconId != null) {
            mloadIcon = true;
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

    /**
     * Processes SET_UP_EVENT_LIST proactive command from the SIM card.
     *
     * @param cmdDet Command Details object retrieved.
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     * @return false. This function always returns false meaning that the command
     *         processing is  not pending and additional asynchronous processing
     *         is not required.
     */
    private boolean processSetUpEventList(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) {

        CatLog.d(this, "process SetUpEventList");
        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.EVENT_LIST, ctlvs);
        if (ctlv != null) {
            try {
                byte[] rawValue = ctlv.getRawValue();
                int valueIndex = ctlv.getValueIndex();
                int valueLen = ctlv.getLength();
                int[] eventList = new int[valueLen];
                int eventValue = -1;
                int i = 0;
                while (valueLen > 0) {
                    eventValue = rawValue[valueIndex] & 0xff;
                    valueIndex++;
                    valueLen--;

                    switch (eventValue) {
                        case USER_ACTIVITY_EVENT:
                        case IDLE_SCREEN_AVAILABLE_EVENT:
                        case LANGUAGE_SELECTION_EVENT:
                        case BROWSER_TERMINATION_EVENT:
                        case BROWSING_STATUS_EVENT:
                        case LOCATION_STATUS_EVENT:
                            eventList[i] = eventValue;
                            i++;
                            break;
                        default:
                            break;
                    }

                }
                mCmdParams = new SetEventListParamsSprd(cmdDet, eventList);
            } catch (IndexOutOfBoundsException e) {
                CatLog.e(this, " IndexOutofBoundException in processSetUpEventList");
            }
        }
        return false;
    }

    /**
     * Processes LAUNCH_BROWSER proactive command from the SIM card.
     *
     * @param cmdDet Command Details container object.
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     * @return true if the command is processing is pending and additional
     *         asynchronous processing is required.
     * @throws ResultException
     */
    private boolean processLaunchBrowser(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {

        CatLog.d(this, "process LaunchBrowser");

        TextMessage confirmMsg = new TextMessage();
        IconIdSprd iconId = null;
        String url = null;

        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.URL, ctlvs);
        if (ctlv != null) {
            try {
                byte[] rawValue = ctlv.getRawValue();
                int valueIndex = ctlv.getValueIndex();
                int valueLen = ctlv.getLength();
                if (valueLen > 0) {
                    url = GsmAlphabet.gsm8BitUnpackedToString(rawValue,
                            valueIndex, valueLen);
                } else {
                    url = null;
                }
            } catch (IndexOutOfBoundsException e) {
                throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
            }
        }

        // parse alpha identifier.
        ctlv = searchForTag(ComprehensionTlvTagSprd.ALPHA_ID, ctlvs);
        confirmMsg.text = ValueParserSprd.retrieveAlphaId(ctlv);

        // parse icon identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            confirmMsg.iconSelfExplanatory = iconId.selfExplanatory;
        }

        // parse command qualifier value.
        LaunchBrowserMode mode;
        switch (cmdDet.commandQualifier) {
        case 0x00:
        default:
            mode = LaunchBrowserMode.LAUNCH_IF_NOT_ALREADY_LAUNCHED;
            break;
        case 0x02:
            mode = LaunchBrowserMode.USE_EXISTING_BROWSER;
            break;
        case 0x03:
            mode = LaunchBrowserMode.LAUNCH_NEW_BROWSER;
            break;
        }

        mCmdParams = new LaunchBrowserParamsSprd(cmdDet, confirmMsg, url, mode);

        if (iconId != null) {
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

     /**
     * Processes PLAY_TONE proactive command from the SIM card.
     *
     * @param cmdDet Command Details container object.
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     * @return true if the command is processing is pending and additional
     *         asynchronous processing is required.t
     * @throws ResultException
     */
    private boolean processPlayTone(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {

        CatLog.d(this, "process PlayTone");

        Tone tone = null;
        TextMessage textMsg = new TextMessage();
        Duration duration = null;
        IconIdSprd iconId = null;

        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.TONE, ctlvs);
        if (ctlv != null) {
            // Nothing to do for null objects.
            if (ctlv.getLength() > 0) {
                try {
                    byte[] rawValue = ctlv.getRawValue();
                    int valueIndex = ctlv.getValueIndex();
                    int toneVal = rawValue[valueIndex];
                    tone = Tone.fromInt(toneVal);
                } catch (IndexOutOfBoundsException e) {
                    throw new ResultExceptionSprd(
                            ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
                }
            }
        }
        // parse alpha identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ALPHA_ID, ctlvs);
        if (ctlv != null) {
            textMsg.text = ValueParserSprd.retrieveAlphaId(ctlv);
        }
        // parse tone duration
        ctlv = searchForTag(ComprehensionTlvTagSprd.DURATION, ctlvs);
        if (ctlv != null) {
            duration = ValueParserSprd.retrieveDuration(ctlv);
        }
        // parse icon identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            textMsg.iconSelfExplanatory = iconId.selfExplanatory;
        }

        boolean vibrate = (cmdDet.commandQualifier & 0x01) != 0x00;

        textMsg.responseNeeded = false;
        mCmdParams = new PlayToneParamsSprd(cmdDet, textMsg, tone, duration, vibrate);

        if (iconId != null) {
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

    /**
     * Processes SETUP_CALL proactive command from the SIM card.
     *
     * @param cmdDet Command Details object retrieved from the proactive command
     *        object
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     * @return true if the command is processing is pending and additional
     *         asynchronous processing is required.
     */
    private boolean processSetupCall(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {
        CatLog.d(this, "process SetupCall");

        Iterator<ComprehensionTlvSprd> iter = ctlvs.iterator();
        ComprehensionTlvSprd ctlv = null;
        // User confirmation phase message.
        TextMessage confirmMsg = new TextMessage();
        // Call set up phase message.
        TextMessage callMsg = new TextMessage();
        IconIdSprd confirmIconId = null;
        IconIdSprd callIconId = null;

        // get confirmation message string.
        ctlv = searchForNextTag(ComprehensionTlvTagSprd.ALPHA_ID, iter);
        confirmMsg.text = ValueParserSprd.retrieveAlphaId(ctlv);

        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            confirmIconId = ValueParserSprd.retrieveIconId(ctlv);
            confirmMsg.iconSelfExplanatory = confirmIconId.selfExplanatory;
        }

        // get call set up message string.
        ctlv = searchForNextTag(ComprehensionTlvTagSprd.ALPHA_ID, iter);
        if (ctlv != null) {
            callMsg.text = ValueParserSprd.retrieveAlphaId(ctlv);
        }

        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            callIconId = ValueParserSprd.retrieveIconId(ctlv);
            callMsg.iconSelfExplanatory = callIconId.selfExplanatory;
        }

        mCmdParams = new CallSetupParamsSprd(cmdDet, confirmMsg, callMsg);

        if (confirmIconId != null || callIconId != null) {
            mIconLoadState = LOAD_MULTI_ICONS;
            int[] recordNumbers = new int[2];
            recordNumbers[0] = confirmIconId != null
                    ? confirmIconId.recordNumber : -1;
            recordNumbers[1] = callIconId != null ? callIconId.recordNumber
                    : -1;

            mIconLoader.loadIcons(recordNumbers, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

    private boolean processProvideLocalInfo(CommandDetailsSprd cmdDet, List<ComprehensionTlvSprd> ctlvs)
            throws ResultExceptionSprd {
        CatLog.d(this, "process ProvideLocalInfo");
        switch (cmdDet.commandQualifier) {
            case DTTZ_SETTING:
                CatLog.d(this, "PLI [DTTZ_SETTING]");
                mCmdParams = new CommandParamsSprd(cmdDet);
                break;
            case LANGUAGE_SETTING:
                CatLog.d(this, "PLI [LANGUAGE_SETTING]");
                mCmdParams = new CommandParamsSprd(cmdDet);
                break;
            default:
                CatLog.d(this, "PLI[" + cmdDet.commandQualifier + "] Command Not Supported");
                mCmdParams = new CommandParamsSprd(cmdDet);
                throw new ResultExceptionSprd(ResultCodeSprd.BEYOND_TERMINAL_CAPABILITY);
        }
        return false;
    }

    private boolean processBIPClient(CommandDetailsSprd cmdDet,
                                     List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {
        AppInterfaceSprd.CommandTypeSprd commandType =
                                    AppInterfaceSprd.CommandTypeSprd.fromInt(cmdDet.typeOfCommand);
        if (commandType != null) {
            CatLog.d(this, "process "+ commandType.name());
        }

        TextMessage textMsg = new TextMessage();
        IconIdSprd iconId = null;
        ComprehensionTlvSprd ctlv = null;
        boolean has_alpha_id = false;

        // parse alpha identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ALPHA_ID, ctlvs);
        if (ctlv != null) {
            textMsg.text = ValueParserSprd.retrieveAlphaId(ctlv);
            CatLog.d(this, "alpha TLV text=" + textMsg.text);
            has_alpha_id = true;
        }

        // parse icon identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            textMsg.iconSelfExplanatory = iconId.selfExplanatory;
        }

        textMsg.responseNeeded = false;
        mCmdParams = new BIPClientParamsSprd(cmdDet, textMsg, has_alpha_id);

        if (iconId != null) {
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

    public void dispose() {
        mIconLoader.dispose();
        mIconLoader = null;
        mCmdParams = null;
        mCaller = null;
        sInstance = null;
    }


    /**
     * SPRD: Modify here for BIP function @{
     *
     * Processes OPEN_CHANNEL proactive command from the SIM card.
     *
     * @param cmdDet Command Details object retrieved from the proactive command
     *        object
     * @param ctlvs List of ComprehensionTlv objects following Command Details
     *        object and Device Identities object within the proactive command
     * @return true if the command is processing is pending and additional
     *         asynchronous processing is required.
     */
    private boolean processOpenChannel(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {
        CatLog.d(this, "process OpenChannel");

        Iterator<ComprehensionTlvSprd> iter = ctlvs.iterator();
        IconIdSprd iconId = null;
        OpenChannelData openchanneldata = new OpenChannelData();
        // Alpha identifier
        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.ALPHA_ID, ctlvs);
        if (ctlv != null) {
            openchanneldata.text = ValueParserSprd.retrieveAlphaId(ctlv);
            if (openchanneldata.text != null) {
                openchanneldata.isNullAlphaId = false;
                CatLog.d(this, "OpenChannel Alpha identifier done");
            } else {
                openchanneldata.isNullAlphaId = true;
                CatLog.d(this, "OpenChannel null Alpha id");
            }
        }
        // Icon identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            openchanneldata.iconSelfExplanatory = iconId.selfExplanatory;
            CatLog.d(this, "OpenChannel Icon identifier done");
        }
        // Bearer description
        ctlv = searchForTag(ComprehensionTlvTagSprd.BEARER_DESCRIPTION, ctlvs);
        if (ctlv != null) {
            try {
                byte[] rawValue = ctlv.getRawValue();
                int valueIndex = ctlv.getValueIndex();
                openchanneldata.BearerType = rawValue[valueIndex];
                int length = ctlv.getLength();
                if (length > 1) {
                    openchanneldata.BearerParam = IccUtils.bytesToHexString(ValueParserSprd.retrieveByteArray(ctlv, 1));
                    CatLog.d(this, "OpenChannel Bearer description done");
                }
            } catch (IndexOutOfBoundsException e) {
                CatLog.d(this, "OpenChannel BEARER_DESCRIPTION IndexOutOfBoundsException");
                throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
            }
        } else {
            CatLog.d(this, "OpenChannel BEARER_DESCRIPTION ctlv is null");
            throw new ResultExceptionSprd(ResultCodeSprd.REQUIRED_VALUES_MISSING);
        }
        // Buffer size
        ctlv = searchForTag(ComprehensionTlvTagSprd.BUFFER_SIZE, ctlvs);
        if (ctlv != null) {
            try {
                byte[] rawValue = ctlv.getRawValue();
                int valueIndex = ctlv.getValueIndex();
                openchanneldata.bufferSize = ((rawValue[valueIndex] & 0xff) << 8) |
                                               rawValue[valueIndex + 1] & 0xff;
                CatLog.d(this, "OpenChannel Buffer size done");
            } catch (IndexOutOfBoundsException e) {
                CatLog.d(this, "OpenChannel BUFFER_SIZE IndexOutOfBoundsException");
                throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
            }
        } else {
            CatLog.d(this, "OpenChannel BUFFER_SIZE ctlv is null");
            throw new ResultExceptionSprd(ResultCodeSprd.REQUIRED_VALUES_MISSING);
        }
        // Network Access Name
        ctlv = searchForTag(ComprehensionTlvTagSprd.NETWORK_ACCESS_NAME, ctlvs);
        if (ctlv != null) {
                //openchanneldata.NetAccessName = IccUtils.bytesToHexString(ValueParser.retrieveByteArray(ctlv, 0));
                byte[] raw = ValueParserSprd.retrieveByteArray(ctlv, 0);
                openchanneldata.NetAccessName = convNetworkAccessName(raw);
                CatLog.d(this, "OpenChannel Network Access Name done");
        }
        // Other address (local address)
        ctlv = searchForTag(ComprehensionTlvTagSprd.OTHER_ADDRESS, ctlvs);
        if (ctlv != null) {
            try {
                byte[] rawValue = ctlv.getRawValue();
                int valueIndex = ctlv.getValueIndex();
                int length = ctlv.getLength();
                if (length > 1) {
                    openchanneldata.OtherAddressType = rawValue[valueIndex];
                    byte [] address = ValueParserSprd.retrieveByteArray(ctlv, 1);
                    if (openchanneldata.OtherAddressType == OpenChannelData.ADDRESS_TYPE_IPV4 && address.length == 4) {
                        openchanneldata.OtherAddress = convIpv4Address(address);
                        CatLog.d(this, "OpenChannel local address done");
                    } else {
                        CatLog.d(this, "OpenChannel local Address is not ipv4 format");
                        openchanneldata.OtherAddress = "";
                        openchanneldata.OtherAddressType = 0;
                    }
                } else {
                    CatLog.d(this, "OpenChannel local address tag length error");
                }
            } catch (IndexOutOfBoundsException e) {
                CatLog.d(this, "OpenChannel OtherAddress IndexOutOfBoundsException");
            }
        }
        // Text String (User login)
        ctlv = searchForNextTag(ComprehensionTlvTagSprd.TEXT_STRING, iter);
        if (ctlv != null) {
            openchanneldata.LoginStr = ValueParserSprd.retrieveTextString(ctlv);
            CatLog.d(this, "OpenChannel User login done");
        }
        // Text String (User password)
        ctlv = searchForNextTag(ComprehensionTlvTagSprd.TEXT_STRING, iter);
        if (ctlv != null) {
            openchanneldata.PwdStr = ValueParserSprd.retrieveTextString(ctlv);
            CatLog.d(this, "OpenChannel User password done");
        }
        // SIM ME interface transport level
        ctlv = searchForTag(ComprehensionTlvTagSprd.TRANSPORT_LEVEL, ctlvs);
        if (ctlv != null) {
            try {
                byte[] rawValue = ctlv.getRawValue();
                int valueIndex = ctlv.getValueIndex();
                openchanneldata.transportType = rawValue[valueIndex];
                openchanneldata.portNumber = ((rawValue[valueIndex+1] & 0xff) << 8) |
                                               rawValue[valueIndex + 2] & 0xff;
                CatLog.d(this, "OpenChannel transport level done");
            } catch (IndexOutOfBoundsException e) {
                CatLog.d(this, "OpenChannel TRANSPORT_LEVEL IndexOutOfBoundsException");
            }
        }
        // Data destination address
        ctlv = searchForLastTag(ComprehensionTlvTagSprd.OTHER_ADDRESS, ctlvs);
        if (ctlv != null) {
            try {
                byte[] rawValue = ctlv.getRawValue();
                int valueIndex = ctlv.getValueIndex();
                int length = ctlv.getLength();
                if (length > 1) {
                    openchanneldata.DataDstAddressType = rawValue[valueIndex];
                    byte [] address = ValueParserSprd.retrieveByteArray(ctlv, 1);
                    if (openchanneldata.DataDstAddressType == OpenChannelData.ADDRESS_TYPE_IPV4 && address.length == 4) {
                        openchanneldata.DataDstAddress = convIpv4Address(address);
                        CatLog.d(this, "OpenChannel Data destination address done");
                    } else {
                        CatLog.d(this, "OpenChannel Data destination address is not ipv4 format");
                        openchanneldata.DataDstAddress = "";
                        openchanneldata.DataDstAddressType = 0;
                    }
                } else {
                    CatLog.d(this, "OpenChannel Data destination address tag length error");
                }
            } catch (IndexOutOfBoundsException e) {
                CatLog.d(this, "OpenChannel DataDstAddress IndexOutOfBoundsException");
            }
        }

        mCmdParams = new OpenChannelDataParamsSprd(cmdDet, openchanneldata);
        if (iconId != null) {
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        CatLog.d(this, "Alpha id: " + openchanneldata.text +
                       ", NetAccessName: " + openchanneldata.NetAccessName +
                       ", bufferSize: " + openchanneldata.bufferSize +
                       ", BearerType: " + openchanneldata.BearerType + "\n" +
                       ", BearerParam: " + openchanneldata.BearerParam +
                       ", LocalAddressType: " + openchanneldata.OtherAddressType +
                       ", LocalAddress: " + openchanneldata.OtherAddress +
                       ", LoginStr: " + openchanneldata.LoginStr + "\n" +
                       ", PwdStr: " + openchanneldata.PwdStr +
                       ", transportType: " + openchanneldata.transportType +
                       ", portNumber: " + openchanneldata.portNumber +
                       ", DataDstAddressType: " + openchanneldata.DataDstAddressType + "\n" +
                       ", DataDstAddress: " + openchanneldata.DataDstAddress);
        return false;
    }
    private boolean processCloseChannel(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {
        CatLog.d(this, "process CloseChannel");
        DeviceIdentitiesSprd deviceIdentities = null;
        IconIdSprd iconId = null;
        CloseChannelData closechanneldata = new CloseChannelData();
        //Device identities
        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.DEVICE_IDENTITIES, ctlvs);
        if (ctlv != null) {
            deviceIdentities = ValueParserSprd.retrieveDeviceIdentities(ctlv);
        }
        // Alpha identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ALPHA_ID, ctlvs);
        if (ctlv != null) {
            closechanneldata.text = ValueParserSprd.retrieveAlphaId(ctlv);
        }
        // Icon identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            closechanneldata.iconSelfExplanatory = iconId.selfExplanatory;
        }

        mCmdParams = new CloseChannelDataParamsSprd(cmdDet, closechanneldata, deviceIdentities);
        if (iconId != null) {
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

    private boolean processReceiveData(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {
        CatLog.d(this, "process ReceiveData");

        IconIdSprd iconId = null;
        ReceiveChannelData receivedata = new ReceiveChannelData();
        // Alpha identifier
        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.ALPHA_ID, ctlvs);
        if (ctlv != null) {
            receivedata.text = ValueParserSprd.retrieveAlphaId(ctlv);
        }
        // Icon identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            receivedata.iconSelfExplanatory = iconId.selfExplanatory;
        }
        // Channel data length
        ctlv = searchForTag(ComprehensionTlvTagSprd.CHANNEL_DATA_LENGTH, ctlvs);
        if (ctlv != null) {
            try {
                byte[] rawValue = ctlv.getRawValue();
                int valueIndex = ctlv.getValueIndex();
                receivedata.channelDataLength = rawValue[valueIndex] & 0xff;
            } catch (IndexOutOfBoundsException e) {
                CatLog.d(this, "ReceiveData CHANNEL_DATA_LENGTH IndexOutOfBoundsException");
                throw new ResultExceptionSprd(ResultCodeSprd.CMD_DATA_NOT_UNDERSTOOD);
            }
        } else {
            CatLog.d(this, "ReceiveData CHANNEL_DATA_LENGTH ctlv is null");
            throw new ResultExceptionSprd(ResultCodeSprd.REQUIRED_VALUES_MISSING);
        }

        mCmdParams = new ReceiveChannelDataParamsSprd(cmdDet, receivedata);
        if (iconId != null) {
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

    private boolean processSendData(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {
        CatLog.d(this, "process SendData");

        IconIdSprd iconId = null;
        DeviceIdentitiesSprd deviceIdentities = null;
        SendChannelData senddata = new SendChannelData();
        //Device identities
        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.DEVICE_IDENTITIES, ctlvs);
        if (ctlv != null) {
            deviceIdentities = ValueParserSprd.retrieveDeviceIdentities(ctlv);
        }
        // Alpha identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ALPHA_ID, ctlvs);
        if (ctlv != null) {
            senddata.text = ValueParserSprd.retrieveAlphaId(ctlv);
        }
        // Icon identifier
        ctlv = searchForTag(ComprehensionTlvTagSprd.ICON_ID, ctlvs);
        if (ctlv != null) {
            iconId = ValueParserSprd.retrieveIconId(ctlv);
            senddata.iconSelfExplanatory = iconId.selfExplanatory;
        }
        // Channel data
        ctlv = searchForTag(ComprehensionTlvTagSprd.CHANNEL_DATA, ctlvs);
        if (ctlv != null) {
            senddata.sendDataStr = IccUtils.bytesToHexString(ValueParserSprd.retrieveByteArray(ctlv, 0));
        } else {
            CatLog.d(this, "SendData CHANNEL_DATA ctlv is null");
            throw new ResultExceptionSprd(ResultCodeSprd.REQUIRED_VALUES_MISSING);
        }
        CatLog.d(this, "Alpha id: " + senddata.text + " senddata: " + senddata.sendDataStr);

        mCmdParams = new SendChannelDataParamsSprd(cmdDet, senddata, deviceIdentities);
        if (iconId != null) {
            mIconLoadState = LOAD_SINGLE_ICON;
            mIconLoader.loadIcon(iconId.recordNumber, this
                    .obtainMessage(MSG_ID_LOAD_ICON_DONE));
            return true;
        }
        return false;
    }

    private void processGetChannelStatus(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {
        CatLog.d(this, "process GetChannelStatus");

        GetChannelStatus channelstatus = new GetChannelStatus();
        mCmdParams = new GetChannelStatusParamsSprd(cmdDet, channelstatus);
        return;
    }

    private String convIpv4Address(byte[] address) {
        StringBuffer sb = new StringBuffer("");
        for(int i = 0; i < 3; i ++) {
            sb.append(String.valueOf(address[i] & 0xff));
            sb.append(".");
        }
        sb.append(String.valueOf(address[3] & 0xff));
        return sb.toString();
    }

    private String convNetworkAccessName(byte[] apn) {
        if (apn == null)
            return null;

        int len = apn.length;
        int temp_len = 0;
        int index = 0;
        StringBuilder ret = new StringBuilder(2 * len);

        while (len > 1) {
            temp_len = apn[index++];
            if (temp_len < len) {
                for (int i = 0; i < temp_len; i++) {
                    ret.append((char) apn[index]);
                    index++;
                }
                len = len - (temp_len + 1);
                if (len > 1) {
                    ret.append('.');
                } else {
                    break;
                }
            } else {
                for (int i = 0; i < len - 1; i++) {
                    ret.append((char) apn[index]);
                    index++;
                }
                break;
            }
        }

        return ret.toString();
    }

    private ComprehensionTlvSprd searchForLastTag(ComprehensionTlvTagSprd tag,
            List<ComprehensionTlvSprd> ctlvs) {
        Iterator<ComprehensionTlvSprd> iter = ctlvs.iterator();
        int tagValue = tag.value();
        ComprehensionTlvSprd lastctlv = null;
        while (iter.hasNext()) {
            ComprehensionTlvSprd ctlv = iter.next();
            if (ctlv.getTag() == tagValue) {
                lastctlv = ctlv;
            }
        }
        return lastctlv;
    }
    /* @} */
    /* SPRD: add for USAT 27.22.4.25 LANGUAGE NOTIFICATION  @{ */
    private boolean processLanguageNotify(CommandDetailsSprd cmdDet,
            List<ComprehensionTlvSprd> ctlvs) throws ResultExceptionSprd {

        CatLog.d(this, "processLanguageNotify start");
        String language = null;

        ComprehensionTlvSprd ctlv = searchForTag(ComprehensionTlvTagSprd.LANGUAGE,ctlvs);
        if (ctlv != null) {
             language = ValueParserSprd.retrieveLanguage(ctlv);
        } else {
            CatLog.d(this, "processLanguageNotify language is null");
        }

        mCmdParams = new LanguageParamsSprd(cmdDet, language);

        return false;
    }
    /* @}*/
}
