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
/* SPRD: Add here for BIP function @{ */
import com.android.internal.telephony.cat.bip.*;
/* @{ */

/**
 * Container class for proactive command parameters.
 *
 */
class CommandParamsSprd {
    CommandDetailsSprd mCmdDet;
    // Variable to track if an optional icon load has failed.
    boolean mLoadIconFailed = false;

    CommandParamsSprd(CommandDetailsSprd cmdDet) {
        mCmdDet = cmdDet;
    }

    AppInterfaceSprd.CommandTypeSprd getCommandType() {
        return AppInterfaceSprd.CommandTypeSprd.fromInt(mCmdDet.typeOfCommand);
    }

    boolean setIcon(Bitmap icon) { return true; }

    @Override
    public String toString() {
        return mCmdDet.toString();
    }
}

class DisplayTextParamsSprd extends CommandParamsSprd {
    TextMessage mTextMsg;

    DisplayTextParamsSprd(CommandDetailsSprd cmdDet, TextMessage textMsg) {
        super(cmdDet);
        mTextMsg = textMsg;
    }

    @Override
    boolean setIcon(Bitmap icon) {
        if (icon != null && mTextMsg != null) {
            mTextMsg.icon = icon;
            return true;
        }
        return false;
    }

    @Override
    public String toString() {
        return "TextMessage=" + mTextMsg + " " + super.toString();
    }
}

class LaunchBrowserParamsSprd extends CommandParamsSprd {
    TextMessage mConfirmMsg;
    LaunchBrowserMode mMode;
    String mUrl;

    LaunchBrowserParamsSprd(CommandDetailsSprd cmdDet, TextMessage confirmMsg,
            String url, LaunchBrowserMode mode) {
        super(cmdDet);
        mConfirmMsg = confirmMsg;
        mMode = mode;
        mUrl = url;
    }

    @Override
    boolean setIcon(Bitmap icon) {
        if (icon != null && mConfirmMsg != null) {
            mConfirmMsg.icon = icon;
            return true;
        }
        return false;
    }

    @Override
    public String toString() {
        return "TextMessage=" + mConfirmMsg + " " + super.toString();
    }
}

class SetEventListParamsSprd extends CommandParamsSprd {
    int[] mEventInfo;
    SetEventListParamsSprd(CommandDetailsSprd cmdDet, int[] eventInfo) {
        super(cmdDet);
        this.mEventInfo = eventInfo;
    }
}

class PlayToneParamsSprd extends CommandParamsSprd {
    TextMessage mTextMsg;
    ToneSettings mSettings;

    PlayToneParamsSprd(CommandDetailsSprd cmdDet, TextMessage textMsg,
            Tone tone, Duration duration, boolean vibrate) {
        super(cmdDet);
        mTextMsg = textMsg;
        mSettings = new ToneSettings(duration, tone, vibrate);
    }

    @Override
    boolean setIcon(Bitmap icon) {
        if (icon != null && mTextMsg != null) {
            mTextMsg.icon = icon;
            return true;
        }
        return false;
    }
}

class CallSetupParamsSprd extends CommandParamsSprd {
    TextMessage mConfirmMsg;
    TextMessage mCallMsg;

    CallSetupParamsSprd(CommandDetailsSprd cmdDet, TextMessage confirmMsg,
            TextMessage callMsg) {
        super(cmdDet);
        mConfirmMsg = confirmMsg;
        mCallMsg = callMsg;
    }

    @Override
    boolean setIcon(Bitmap icon) {
        if (icon == null) {
            return false;
        }
        if (mConfirmMsg != null && mConfirmMsg.icon == null) {
            mConfirmMsg.icon = icon;
            return true;
        } else if (mCallMsg != null && mCallMsg.icon == null) {
            mCallMsg.icon = icon;
            return true;
        }
        return false;
    }
}

class SelectItemParamsSprd extends CommandParamsSprd {
    Menu mMenu = null;
    boolean mLoadTitleIcon = false;

    SelectItemParamsSprd(CommandDetailsSprd cmdDet, Menu menu, boolean loadTitleIcon) {
        super(cmdDet);
        mMenu = menu;
        mLoadTitleIcon = loadTitleIcon;
    }

    @Override
    boolean setIcon(Bitmap icon) {
        if (icon != null && mMenu != null) {
            if (mLoadTitleIcon && mMenu.titleIcon == null) {
                mMenu.titleIcon = icon;
            } else {
                for (Item item : mMenu.items) {
                    if (item.icon != null) {
                        continue;
                    }
                    item.icon = icon;
                    break;
                }
            }
            return true;
        }
        return false;
    }
}

class GetInputParamsSprd extends CommandParamsSprd {
    InputSprd mInput = null;

    GetInputParamsSprd(CommandDetailsSprd cmdDet, InputSprd input) {
        super(cmdDet);
        mInput = input;
    }

    @Override
    boolean setIcon(Bitmap icon) {
        if (icon != null && mInput != null) {
            mInput.icon = icon;
        }
        return true;
    }
}

/* SPRD: Add DTMF function. @{ */
class DtmfParamsSprd extends CommandParamsSprd {
    TextMessage textMsg;
    String dtmfString;

    DtmfParamsSprd(CommandDetailsSprd cmdDet, TextMessage textMsg, String dtmf) {
        super(cmdDet);
        this.textMsg = textMsg;
        dtmfString = dtmf;
    }

    boolean setIcon(Bitmap icon) {
        if (icon != null && textMsg != null) {
            textMsg.icon = icon;
            return true;
        }
        return false;
    }
}

/* @} */

/*
 * BIP (Bearer Independent Protocol) is the mechanism for SIM card applications
 * to access data connection through the mobile device.
 *
 * SIM utilizes proactive commands (OPEN CHANNEL, CLOSE CHANNEL, SEND DATA and
 * RECEIVE DATA to control/read/write data for BIP. Refer to ETSI TS 102 223 for
 * the details of proactive commands procedures and their structures.
 */
class BIPClientParamsSprd extends CommandParamsSprd {
    TextMessage mTextMsg;
    boolean mHasAlphaId;

    BIPClientParamsSprd(CommandDetailsSprd cmdDet, TextMessage textMsg, boolean has_alpha_id) {
        super(cmdDet);
        mTextMsg = textMsg;
        mHasAlphaId = has_alpha_id;
    }

    @Override
    boolean setIcon(Bitmap icon) {
        if (icon != null && mTextMsg != null) {
            mTextMsg.icon = icon;
            return true;
        }
        return false;
    }
}

/* SPRD: Add here for BIP function @{ */
class OpenChannelDataParamsSprd extends CommandParamsSprd {
    OpenChannelData openchanneldata;

    OpenChannelDataParamsSprd(CommandDetailsSprd cmdDet, OpenChannelData opendata) {
        super(cmdDet);
        openchanneldata = opendata;
        openchanneldata.setChannelType(cmdDet.typeOfCommand);
    }

    boolean setIcon(Bitmap icon) {
        if (icon != null && openchanneldata != null) {
            openchanneldata.icon = icon;
            return true;
        }
        return false;
    }
}

class CloseChannelDataParamsSprd extends CommandParamsSprd {
    CloseChannelData closechanneldata;
    DeviceIdentitiesSprd deviceIdentities;

    CloseChannelDataParamsSprd(CommandDetailsSprd cmdDet, CloseChannelData closedata,
            DeviceIdentitiesSprd identities) {
        super(cmdDet);
        closechanneldata = closedata;
        closechanneldata.setChannelType(cmdDet.typeOfCommand);
        deviceIdentities = identities;
    }

    boolean setIcon(Bitmap icon) {
        if (icon != null && closechanneldata != null) {
            closechanneldata.icon = icon;
            return true;
        }
        return false;
    }
}

class ReceiveChannelDataParamsSprd extends CommandParamsSprd {
    ReceiveChannelData receivedata;

    ReceiveChannelDataParamsSprd(CommandDetailsSprd cmdDet, ReceiveChannelData rdata) {
        super(cmdDet);
        receivedata = rdata;
        receivedata.setChannelType(cmdDet.typeOfCommand);
    }

    boolean setIcon(Bitmap icon) {
        if (icon != null && receivedata != null) {
            receivedata.icon = icon;
            return true;
        }
        return false;
    }
}

class SendChannelDataParamsSprd extends CommandParamsSprd {
    SendChannelData senddata;
    DeviceIdentitiesSprd deviceIdentities;

    SendChannelDataParamsSprd(CommandDetailsSprd cmdDet, SendChannelData sdata, DeviceIdentitiesSprd identities) {
        super(cmdDet);
        senddata = sdata;
        senddata.setChannelType(cmdDet.typeOfCommand);
        deviceIdentities = identities;
    }

    boolean setIcon(Bitmap icon) {
        if (icon != null && senddata != null) {
            senddata.icon = icon;
            return true;
        }
        return false;
    }
}

class GetChannelStatusParamsSprd extends CommandParamsSprd {
    GetChannelStatus channelstatus;

    GetChannelStatusParamsSprd(CommandDetailsSprd cmdDet, GetChannelStatus status) {
        super(cmdDet);
        channelstatus = status;
        channelstatus.setChannelType(cmdDet.typeOfCommand);
    }
}
/* @} */
/* SPRD: add for USAT 27.22.4.25 LANGUAGE NOTIFICATION  @{ */
class LanguageParamsSprd extends CommandParamsSprd{

    String languageString;

    LanguageParamsSprd(CommandDetailsSprd cmdDet,String language) {
        super(cmdDet);
        languageString = language;
    }

}
/* @}*/
