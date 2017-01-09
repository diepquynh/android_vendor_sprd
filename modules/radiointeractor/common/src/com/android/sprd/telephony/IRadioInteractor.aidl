package com.android.sprd.telephony;

import com.android.sprd.telephony.IRadioInteractorCallback;
import android.telephony.IccOpenLogicalChannelResponse;

interface IRadioInteractor
{
    int getRequestRadioInteractor(int type, int slotId);
    void listenForSlot(in int slotId, IRadioInteractorCallback callback, int events, boolean notifyNow);
    int sendAtCmd(in String[] oemReq, out String[] oemResp, int slotId);
    String getSimCapacity(int slotId);
    String getDefaultNetworkAccessName(int slotId);
    void enableRauNotify(int slotId);
    boolean queryHdVoiceState(int slotId);
    void setCallingNumberShownEnabled(int slotId, boolean enabled);
    boolean storeSmsToSim(boolean enable,int slotId);
    String querySmsStorageMode(int slotId);
    boolean requestShutdown(int slotId);
    String getBandInfo(int slotId);
    void setBandInfoMode(int type,int slotId);
    String getIccID(int phoneId);
    String getHomePLMN(int phoneId);

    /* SPRD: Bug#525009 Add support for Open Mobile API @{*/
    IccOpenLogicalChannelResponse iccOpenLogicalChannel(String AID, int slotId);
    IccOpenLogicalChannelResponse iccOpenLogicalChannelP2(String AID, byte p2, int slotId);
    boolean iccCloseLogicalChannel(int channel, int slotId);
    String iccTransmitApduLogicalChannel(int channel, int cla,
        int command, int p1, int p2, int p3, String data, int slotId);
    String iccTransmitApduBasicChannel(int cla, int command, int p1, int p2,
        int p3, String data, int slotId);
    String iccGetAtr(int slotId);
    /* @} */
    // Explicit Transfer Call REFACTORING
    void explicitCallTransfer(int slotId);
    void switchMultiCalls(int slotId, int mode);
    int getSimLockRemainTimes(int type, int slotId);
    int getSimLockStatus(int type, int slotId);
    void setSimPower(String pkgname,int phoneId, boolean enabled);
    void setPreferredNetworkType(int phoneId, int networkType);
}
