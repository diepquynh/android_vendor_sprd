
package com.android.sprd.telephony;

import android.content.Context;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.telephony.IccOpenLogicalChannelResponse;

import com.android.sprd.telephony.UtilLog;
import com.android.sprd.telephony.IRadioInteractor;

import static com.android.sprd.telephony.RIConstants.RADIOINTERACTOR_SERVER;

public class RadioInteractorProxy extends IRadioInteractor.Stub {
    private static RadioInteractorProxy sInstance;

    private RadioInteractorNotifier mRadioInteractorNotifier;
    private RadioInteractorHandler[] mRadioInteractorHandler;

    public static RadioInteractorProxy init(Context context,
            RadioInteractorNotifier radioInteractorNotifier) {
        return init(context, radioInteractorNotifier, null);
    }

    public static RadioInteractorProxy init(Context context,
            RadioInteractorNotifier radioInteractorNotifier,
            RadioInteractorHandler[] radioInteractorHandler) {
        synchronized (RadioInteractorProxy.class) {
            if (sInstance == null) {
                sInstance = new RadioInteractorProxy(context,
                        radioInteractorNotifier, radioInteractorHandler);
            } else {
                UtilLog.loge("RadioInteractorProxy", "init() called multiple times!  sInstance = "
                        + sInstance);
            }
            return sInstance;
        }
    }

    public static RadioInteractorProxy getInstance() {
        return sInstance;
    }

    private RadioInteractorProxy(Context context, RadioInteractorNotifier radioInteractorNotifier,
            RadioInteractorHandler[] radioInteractorHandler) {
        mRadioInteractorNotifier = radioInteractorNotifier;
        mRadioInteractorHandler = radioInteractorHandler;
        ServiceManager.addService(RADIOINTERACTOR_SERVER, this);
    }

    private RadioInteractorProxy(Context context) {
        this(context, null, null);
    }

    @Override
    public void listenForSlot(int slotId, IRadioInteractorCallback callback, int events,
            boolean notifyNow) throws RemoteException {
        mRadioInteractorNotifier.listenForSlot(slotId, callback, events, notifyNow);
    }

    @Override
    public int getRequestRadioInteractor(int type, int slotId) {
        if (checkHandlerValid(slotId)) {
            return mRadioInteractorHandler[slotId].getRequestRadioInteractor(type);
        }
        return 0;
    }

    @Override
    public int sendAtCmd(String[] oemReq, String[] oemResp, int slotId) {
        if (checkHandlerValid(slotId)) {
            return mRadioInteractorHandler[slotId].invokeOemRILRequestStrings(oemReq, oemResp);
        }
        return 0;
    }

    @Override
    public String getSimCapacity(int slotId) {
        if (checkHandlerValid(slotId)) {
            return mRadioInteractorHandler[slotId].getSimCapacity();
        }
        return null;
    }

    @Override
    public String getDefaultNetworkAccessName(int slotId) {
        if (checkHandlerValid(slotId)) {
            return mRadioInteractorHandler[slotId].getDefaultNetworkAccessName();
        }
        return null;
    }

    @Override
    public void enableRauNotify(int slotId) {
        if (checkHandlerValid(slotId)) {
            mRadioInteractorHandler[slotId].enableRauNotify();
        }
    }

    /* SPRD: Bug#525009 Add support for Open Mobile API @{*/
    @Override
    public IccOpenLogicalChannelResponse iccOpenLogicalChannel(String AID, int slotId) {
        if (checkHandlerValid(slotId)) {
            IccOpenLogicalChannelResponse response = mRadioInteractorHandler[slotId]
                    .iccOpenLogicalChannel(AID);
            return response;
        }
        return null;
    }

    @Override
    public IccOpenLogicalChannelResponse iccOpenLogicalChannelP2(String AID, byte p2, int slotId) {
        if (checkHandlerValid(slotId)) {
            IccOpenLogicalChannelResponse response = mRadioInteractorHandler[slotId]
                    .iccOpenLogicalChannelP2(AID, p2);
            return response;
        }
        return null;
    }

    @Override
    public boolean iccCloseLogicalChannel(int channel, int slotId) {
        if (checkHandlerValid(slotId)) {
            boolean response = mRadioInteractorHandler[slotId]
                    .iccCloseLogicalChannel(channel);
            return response;
        }
        return false;
    }

    @Override
    public String iccTransmitApduLogicalChannel(int channel, int cla,
            int command, int p1, int p2, int p3, String data, int slotId) {
        if (checkHandlerValid(slotId)) {
            String response = mRadioInteractorHandler[slotId]
                    .iccTransmitApduLogicalChannel(channel, cla, command, p1, p2, p3, data);
            return response;
        }
        return null;
    }

    @Override
    public String iccTransmitApduBasicChannel(int cla, int command, int p1, int p2,
            int p3, String data, int slotId) {
        if (checkHandlerValid(slotId)) {
            String response = mRadioInteractorHandler[slotId]
                    .iccTransmitApduBasicChannel(cla, command, p1, p2, p3, data);
            return response;
        }
        return null;
    }

    @Override
    public String iccGetAtr(int slotId){
        if (checkHandlerValid(slotId)) {
            String response = (String) mRadioInteractorHandler[slotId].iccGetAtr();
            return response;
        }
        return null;
    }

    /**
     * Check the handler is not null
     * @param slotId int
     * @return true if the handler is not null
     */
    private boolean checkHandlerValid(int slotId){
        try {
            if(mRadioInteractorHandler != null && mRadioInteractorHandler[slotId] != null){
                return true;
            }
            return false;
        } catch (ArrayIndexOutOfBoundsException e){
            e.printStackTrace();
            UtilLog.loge("RadioInteractorProxy", "ArrayIndexOutOfBoundsException occured" +
                    " probably invalid slotId " + slotId);
            return false;
        }
    }
    /* @} */

    public boolean queryHdVoiceState(int slotId) {
        if (checkHandlerValid(slotId)) {
            return mRadioInteractorHandler[slotId].queryHdVoiceState();
        }
        return false;
    }

    public void setCallingNumberShownEnabled(int slotId, boolean enabled){
        if (checkHandlerValid(slotId)) {
            mRadioInteractorHandler[slotId].setCallingNumberShownEnabled(enabled);
        }
    }
    public boolean storeSmsToSim(boolean enable, int slotId) {
        if (checkHandlerValid(slotId)) {
            return mRadioInteractorHandler[slotId].storeSmsToSim(enable);
        }
        return false;
    }

    public String querySmsStorageMode(int slotId) {
        if (checkHandlerValid(slotId)) {
            return mRadioInteractorHandler[slotId].querySmsStorageMode();
        }
        return null;
    }

    /**
     * Explicit Transfer Call REFACTORING
     */
    public void explicitCallTransfer(int slotId) {
        if (checkHandlerValid(slotId)) {
            mRadioInteractorHandler[slotId].explicitCallTransfer();
        }
    }

    /**
     * Multi Part Call
     */
    public void switchMultiCalls(int slotId, int mode) {
        if (checkHandlerValid(slotId)) {
            mRadioInteractorHandler[slotId].switchMultiCalls(mode);
        }
    }

    public boolean requestShutdown(int slotId) {
        if (checkHandlerValid(slotId)) {
            return mRadioInteractorHandler[slotId].requestShutdown();
        }
        return false;
    }

    public int getSimLockRemainTimes(int type, int slotId) {
        if (checkHandlerValid(slotId)) {
            return mRadioInteractorHandler[slotId].getSimLockRemainTimes(type);
        }
        return -1;
    }

    public int getSimLockStatus(int type, int slotId) {
        if (checkHandlerValid(slotId)) {
            return mRadioInteractorHandler[slotId].getSimLockStatus(type);
        }
        return -1;
    }

    public void setSimPower(String packageName,int phoneId, boolean enabled) {
        if (checkHandlerValid(phoneId)) {
            mRadioInteractorHandler[phoneId].setSimPower(packageName,enabled);
        }
    }

    public void setPreferredNetworkType(int phoneId, int networkType) {
        if (checkHandlerValid(phoneId)) {
            mRadioInteractorHandler[phoneId].setPreferredNetworkType(networkType);
        }
    }

    public String getBandInfo(int slotId) {
        if (checkHandlerValid(slotId)) {
            return mRadioInteractorHandler[slotId].getBandInfo();
        }
        return null;
    }

    public void setBandInfoMode(int type,int slotId){
        if (checkHandlerValid(slotId)) {
            mRadioInteractorHandler[slotId].setBandInfoMode(type);
        }
    }

    public String getIccID(int phoneId){
        if (checkHandlerValid(phoneId)) {
            return mRadioInteractorHandler[phoneId].getIccID();
        }
        return null;
    }

    public String getHomePLMN(int phoneId){
        if (checkHandlerValid(phoneId)) {
            return mRadioInteractorHandler[phoneId].getHomePLMN();
        }
        return null;
    }
}
