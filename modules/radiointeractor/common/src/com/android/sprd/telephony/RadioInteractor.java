
package com.android.sprd.telephony;

import android.content.Context;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.telephony.IccOpenLogicalChannelResponse;

import static com.android.sprd.telephony.RIConstants.RADIOINTERACTOR_SERVER;

public class RadioInteractor {

    IRadioInteractor mRadioInteractorProxy;
    private static RadioInteractorFactory sRadioInteractorFactory;
    private static final String TAG = "RadioInteractor";

    Context mContext;
    public RadioInteractor(Context context) {
        mContext = context;
        mRadioInteractorProxy = IRadioInteractor.Stub
                .asInterface(ServiceManager.getService(RADIOINTERACTOR_SERVER));
    }

    public void listen(RadioInteractorCallbackListener radioInteractorCallbackListener,
            int events) {
        this.listen(radioInteractorCallbackListener, events, true);
    }

    public void listen(RadioInteractorCallbackListener radioInteractorCallbackListener,
            int events, boolean notifyNow) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(radioInteractorCallbackListener.mSlotId);
            if (rih != null) {
                if (events == RadioInteractorCallbackListener.LISTEN_NONE) {
                    rih.unregisterForUnsolRadioInteractor(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForRiConnected(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForRadioInteractorEmbms(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForsetOnVPCodec(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForsetOnVPFallBack(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForsetOnVPString(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForsetOnVPRemoteMedia(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForsetOnVPMMRing(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForsetOnVPFail(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForsetOnVPRecordVideo(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForsetOnVPMediaStart(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForEccNetChanged(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForClearCodeFallback(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForRauSuccess(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForPersonalisationLocked(radioInteractorCallbackListener.mHandler);
                    rih.unregisterForBandInfo(radioInteractorCallbackListener.mHandler);
                    return;
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_RADIOINTERACTOR_EVENT) != 0) {
                    rih.unsolicitedRegisters(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_RADIOINTERACTOR_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_RI_CONNECTED_EVENT) != 0) {
                    rih.registerForRiConnected(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_RI_CONNECTED_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_RADIOINTERACTOR_EMBMS_EVENT) != 0) {
                    rih.registerForRadioInteractorEmbms(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_RADIOINTERACTOR_EMBMS_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_CODEC_EVENT) != 0) {
                    rih.registerForsetOnVPCodec(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_CODEC_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_DSCI_EVENT) != 0) {
                    rih.registerForsetOnVPFallBack(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_DSCI_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_STRING_EVENT) != 0) {
                    rih.registerForsetOnVPString(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_STRING_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_REMOTE_MEDIA_EVENT) != 0) {
                    rih.registerForsetOnVPRemoteMedia(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_REMOTE_MEDIA_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_MM_RING_EVENT) != 0) {
                    rih.registerForsetOnVPMMRing(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_MM_RING_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_RELEASING_EVENT) != 0) {
                    rih.registerForsetOnVPFail(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_RELEASING_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_RECORD_VIDEO_EVENT) != 0) {
                    rih.registerForsetOnVPRecordVideo(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_RECORD_VIDEO_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_MEDIA_START_EVENT) != 0) {
                    rih.registerForsetOnVPMediaStart(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_VIDEOPHONE_MEDIA_START_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_ECC_NETWORK_CHANGED_EVENT) != 0) {
                    rih.registerForEccNetChanged(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_ECC_NETWORK_CHANGED_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_RAU_SUCCESS_EVENT) != 0) {
                    rih.registerForRauSuccess(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_RAU_SUCCESS_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_CLEAR_CODE_FALLBACK_EVENT) != 0) {
                    rih.registerForClearCodeFallback(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_CLEAR_CODE_FALLBACK_EVENT);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_SIMLOCK_NOTIFY_EVENT) != 0) {
                    rih.registerForPersonalisationLocked(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_SIMLOCK_NOTIFY_EVENT,
                            radioInteractorCallbackListener.mSlotId);
                }
                if ((events & RadioInteractorCallbackListener.LISTEN_BAND_INFO_EVENT) != 0) {
                    rih.registerForBandInfo(radioInteractorCallbackListener.mHandler,
                            RadioInteractorCallbackListener.LISTEN_BAND_INFO_EVENT);
                }
                return;
            }
            if (mRadioInteractorProxy != null) {
                mRadioInteractorProxy.listenForSlot(radioInteractorCallbackListener.mSlotId,
                        radioInteractorCallbackListener.mCallback,
                        events, notifyNow);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    private RadioInteractorHandler getRadioInteractorHandler(int slotId) {
        UtilLog.logd(TAG, "RadioInteractorFactory:  " + sRadioInteractorFactory
                + "  RadioInteractorFactory class " + RadioInteractor.class.hashCode());
        sRadioInteractorFactory = RadioInteractorFactory.getInstance();
        if (sRadioInteractorFactory != null) {
            return sRadioInteractorFactory.getRadioInteractorHandler(slotId);
        }
        return null;
    }

    // This interface will be eliminated
    public int sendAtCmd(String[] oemReq, String[] oemResp, int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.invokeOemRILRequestStrings(oemReq, oemResp);
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.sendAtCmd(oemReq, oemResp, slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return 0;
    }

    public String getDefaultNetworkAccessName(int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.getDefaultNetworkAccessName();
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.getDefaultNetworkAccessName(slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    public String getSimCapacity(int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.getSimCapacity();
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.getSimCapacity(slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    public int getRequestRadioInteractor(int type, int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.getRequestRadioInteractor(type);
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.getRequestRadioInteractor(type, slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return 0;
    }

    public void enableRauNotify(int slotId){
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                rih.enableRauNotify();
                return;
            }
            if (mRadioInteractorProxy != null) {
                mRadioInteractorProxy.enableRauNotify(slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    /* SPRD: Bug#525009 Add support for Open Mobile API @{*/

    /**
     * Opens a logical channel to the ICC card.
     *
     * @param AID Application id. See ETSI 102.221 and 101.220.
     * @param slotId int
     * @return an IccOpenLogicalChannelResponse object.
     */
    public IccOpenLogicalChannelResponse iccOpenLogicalChannel(String AID, int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.iccOpenLogicalChannel(AID);
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.iccOpenLogicalChannel(AID, slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Opens a logical channel to the ICC card.
     *
     * @param AID Application id. See ETSI 102.221 and 101.220.
     * @param p2 byte.
     * @param slotId int
     * @return an IccOpenLogicalChannelResponse object.
     */
    public IccOpenLogicalChannelResponse iccOpenLogicalChannel(String AID, byte p2, int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.iccOpenLogicalChannelP2(AID, p2);
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.iccOpenLogicalChannelP2(AID, p2, slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Closes a previously opened logical channel to the ICC card.
     *
     * Input parameters equivalent to TS 27.007 AT+CCHC command.
     * @param channel is the channel id to be closed as retruned by a successful
     *            iccOpenLogicalChannel.
     * @param slotId int
     * @return true if the channel was closed successfully.
     */
    public boolean iccCloseLogicalChannel(int channel, int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.iccCloseLogicalChannel(channel);
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.iccCloseLogicalChannel(channel, slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return false;
    }

    /**
     * Transmit an APDU to the ICC card over a logical channel.
     *
     * Input parameters equivalent to TS 27.007 AT+CGLA command.
     * @param channel is the channel id to be closed as returned by a successful
     *            iccOpenLogicalChannel.
     * @param cla Class of the APDU command.
     * @param instruction Instruction of the APDU command.
     * @param p1 P1 value of the APDU command.
     * @param p2 P2 value of the APDU command.
     * @param p3 P3 value of the APDU command. If p3 is negative a 4 byte APDU
     *            is sent to the SIM.
     * @param data Data to be sent with the APDU.
     * @param slotId int
     * @return The APDU response from the ICC card with the status appended at
     *            the end.
     */
    public String iccTransmitApduLogicalChannel(int channel, int cla,
            int instruction, int p1, int p2, int p3, String data, int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.iccTransmitApduLogicalChannel(channel, cla, instruction, p1, p2, p3, data);
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.iccTransmitApduLogicalChannel(channel, cla,
                    instruction, p1, p2, p3, data, slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Transmit an APDU to the ICC card over the basic channel.
     *
     * Input parameters equivalent to TS 27.007 AT+CSIM command.
     * @param cla Class of the APDU command.
     * @param instruction Instruction of the APDU command.
     * @param p1 P1 value of the APDU command.
     * @param p2 P2 value of the APDU command.
     * @param p3 P3 value of the APDU command. If p3 is negative a 4 byte APDU
     *            is sent to the SIM.
     * @param data Data to be sent with the APDU.
     * @param slotId int
     * @return The APDU response from the ICC card with the status appended at
     *            the end.
     */
    public String iccTransmitApduBasicChannel(int cla,
            int instruction, int p1, int p2, int p3, String data, int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.iccTransmitApduBasicChannel(cla, instruction, p1, p2, p3, data);
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.iccTransmitApduBasicChannel(cla,
                    instruction, p1, p2, p3, data, slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Returns the ATR of the UICC if available.
     * @param slotId int
     * @return The ATR of the UICC if available.
     */
    public String iccGetAtr(int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.iccGetAtr();
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.iccGetAtr(slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }
    /* @} */

    public boolean queryHdVoiceState(int slotId){
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.queryHdVoiceState();
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.queryHdVoiceState(slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return false;
    }

    /**
     * Send request to set call forward number whether shown.
     *
     * @param slotId int
     * @param enabled CMCC is true,other is false
     */
    public void setCallingNumberShownEnabled(int slotId, boolean enabled) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                rih.setCallingNumberShownEnabled(enabled);
                return;
            }
            if (mRadioInteractorProxy != null) {
                mRadioInteractorProxy.setCallingNumberShownEnabled(slotId, enabled);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    /**
     * Store Sms To Sim
     *
     * @param enable True is store SMS to SIM card,false is store to phone.
     * @param slotId int
     * @return whether successful store Sms To Sim
     */
    public boolean storeSmsToSim(boolean enable, int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.storeSmsToSim(enable);
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.storeSmsToSim(enable, slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return false;
    }

    /**
     * Return SMS Storage Mode.
     *
     * @param slotId int
     * @return SMS Storage Mode
     */
    public String querySmsStorageMode(int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.querySmsStorageMode();
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.querySmsStorageMode(slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * Explicit Transfer Call REFACTORING
     * @param slotId
     */
    public void explicitCallTransfer(int slotId) {
        try {
            if (mRadioInteractorProxy != null) {
                mRadioInteractorProxy.explicitCallTransfer(slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    /**
     * Multi Part Call
     * @param slotId
     * @param mode
     */
    public void switchMultiCalls(int slotId, int mode) {
        UtilLog.logd(TAG, "switchMultiCalls slotId = " + slotId + " mode = " + mode);
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                rih.switchMultiCalls(mode);
            }
            if (mRadioInteractorProxy != null) {
                mRadioInteractorProxy.switchMultiCalls(slotId, mode);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }
    /* add for TV use in phone process@{*/
    public void dialVP(String address, String sub_address, int clirMode, Message result,
            int phoneId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(phoneId);
        if (rih != null) {
            rih.dialVP(address, sub_address, clirMode, result);
        }
    }

    public void codecVP(int type, Bundle param, Message result, int phoneId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(phoneId);
        if (rih != null) {
            rih.codecVP(type, param, result);
        }
    }

    public void fallBackVP(Message result, int phoneId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(phoneId);
        if (rih != null) {
            rih.fallBackVP(result);
        }
    }

    public void sendVPString(String str, Message result, int phoneId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(phoneId);
        if (rih != null) {
            rih.sendVPString(str, result);
        }
    }

    public void controlVPLocalMedia(int datatype, int sw, boolean bReplaceImg, Message result,
            int phoneId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(phoneId);
        if (rih != null) {
            rih.controlVPLocalMedia(datatype, sw, bReplaceImg, result);
        }
    }

    public void controlIFrame(boolean isIFrame, boolean needIFrame, Message result, int phoneId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(phoneId);
        if (rih != null) {
            rih.controlIFrame(isIFrame, needIFrame, result);
        }
    }
    /* @} */

    /* Add for trafficClass @{ */
    public void requestDCTrafficClass(int type, int slotId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            rih. requestDCTrafficClass(type);
        }
    }
    /* @} */

    /*SPRD: bug618350 add single pdp allowed by plmns feature@{*/
    public void requestSetSinglePDNByNetwork(boolean isSinglePDN, int slotId){
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            rih.requestSetSinglePDNByNetwork(isSinglePDN);
        }
    }
    /* @} */

    /* Add for Data Clear Code from Telcel @{ */
    public void setLteEnabled(boolean enable, int slotId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            rih.setLteEnabled(enable);
        }
    }

    public void attachDataConn(boolean enable, int slotId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            rih.attachDataConn(enable);
        }
    }
    /* @} */

    /* Add for query network @{ */
    public void abortSearchNetwork(Message result, int slotId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            rih.abortSearchNetwork(result);
        }
    }

    public void forceDetachDataConn(Message result, int slotId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            rih.forceDetachDataConn(result);
        }
    }
    /* @} */

    /* Add for shutdown optimization @{*/
    public boolean requestShutdown(int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.requestShutdown();
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.requestShutdown(slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return false;
    }
    /* @} */

    /**
     * Get simlock remain times
     *
     * @param type ref to IccCardStatusEx.UNLOCK_XXXX
     * @param slotId int
     * @return remain times
     */
    public int getSimLockRemainTimes(int type, int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.getSimLockRemainTimes(type);
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.getSimLockRemainTimes(type, slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return -1;
    }

    /**
     * Get simlock status by nv
     *
     * @param type ref to IccCardStatusEx.UNLOCK_XXXX
     * @param slotId int
     * @return status: unlocked: 0 locked: 1
     */
    public int getSimLockStatus(int type, int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.getSimLockStatus(type);
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.getSimLockStatus(type, slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return -1;
    }

    /**
     * lock/unlock for one key simlock
     *
     * @param facility String
     * @param lockState boolean: lock:true unlock: false
     * @param response Message
     * @param slotId int
     */
    public void setFacilityLockByUser(String facility, boolean lockState,
            Message response,int slotId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            rih.setFacilityLockByUser(facility, lockState, response);
            return;
        }
        AsyncResult.forMessage(response, null, new Throwable(
               "simlock lock/unlock should be called in phone process!"));
        response.sendToTarget();
    }

    public void getSimStatus(int slotId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            rih.getSimStatus();
            return;
        }
    }

    /**
     * Set SIM power
     */
    public void setSimPower(int phoneId, boolean enabled) {
        if (mContext != null) {
            try {
                RadioInteractorHandler rih = getRadioInteractorHandler(phoneId);
                if (rih != null) {
                    rih.setSimPower(mContext.getPackageName(), enabled);
                } else {
                    if (mRadioInteractorProxy != null) {
                        mRadioInteractorProxy.setSimPower(mContext.getPackageName(),phoneId, enabled);
                    }
                }
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
    }

    public void setPreferredNetworkType(int phoneId, int networkType) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(phoneId);
            if (rih != null) {
                rih.setPreferredNetworkType(networkType);
            } else {
                if (mRadioInteractorProxy != null) {
                    mRadioInteractorProxy.setPreferredNetworkType(phoneId, networkType);
                }
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    public void updateRealEccList(String realEccList,int phoneId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(phoneId);
        if (rih != null) {
            rih.updateRealEccList(realEccList);
        }
    }

    public String getBandInfo(int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                return rih.getBandInfo();
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.getBandInfo(slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    public void setBandInfoMode(int type,int slotId) {
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
            if (rih != null) {
                rih.setBandInfoMode(type);
            }
            if (mRadioInteractorProxy != null) {
                mRadioInteractorProxy.setBandInfoMode(type, slotId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    /**
     * set Preferred Network RAT
     */
    public void setNetworkSpecialRATCap(int type, int slotId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            rih.setNetworkSpecialRATCap(type);
        }
    }

    public String getIccID(int phoneId){
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(phoneId);
            if (rih != null) {
                return rih.getIccID();
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.getIccID(phoneId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    public String getHomePLMN(int phoneId){
        try {
            RadioInteractorHandler rih = getRadioInteractorHandler(phoneId);
            if (rih != null) {
                return rih.getHomePLMN();
            }
            if (mRadioInteractorProxy != null) {
                return mRadioInteractorProxy.getHomePLMN(phoneId);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return null;
    }

    public int queryColp(int slotId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            return rih.queryColp();
        }
        return -1;
    }

    public int queryColr(int slotId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            return rih.queryColr();
        }
        return -1;
    }

    public int mmiEnterSim(String data,int slotId) {
        RadioInteractorHandler rih = getRadioInteractorHandler(slotId);
        if (rih != null) {
            return rih.mmiEnterSim(data);
        }
        return -1;
    }
}
