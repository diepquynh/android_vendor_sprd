
package com.android.sprd.telephony;

import com.android.sprd.telephony.uicc.IccIoResult;
import com.android.sprd.telephony.uicc.IccUtils;

import android.os.AsyncResult;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.telephony.IccOpenLogicalChannelResponse;

public class RadioInteractorHandler extends Handler {
    public static final String TAG = "RadioInteractorHandler";

    RadioInteractorCore mRadioInteractorCore;
    RadioInteractorNotifier mRadioInteractorNotifier;
    SyncHandler mHandler;

 /*
  * This section defines all requests for events
  */
    protected static final int EVENT_GET_REQUEST_RADIOINTERACTOR_DONE = 1;
    protected static final int EVENT_INVOKE_OEM_RIL_REQUEST_STRINGS_DONE = 2;
    protected static final int EVENT_INVOKE_GET_SIM_CAPACITY_DONE = 3;
    protected static final int EVENT_INVOKE_ENABLE_RAU_NOTIFY_DONE = 4;
    /* SPRD: Bug#525009 Add support for Open Mobile API @{*/
    protected static final int EVENT_OPEN_CHANNEL_DONE = 5;
    protected static final int EVENT_GET_ATR_DONE = 6;
    protected static final int EVENT_CLOSE_CHANNEL_DONE = 7;
    protected static final int EVENT_TRANSMIT_APDU_BASIC_CHANNEL_DONE = 8;
    protected static final int EVENT_TRANSMIT_APDU_LOGICAL_CHANNEL_DONE = 9;
    /* @} */
    // SPRD: add for HIGH_DEF_AUDIO
    protected static final int EVENT_GET_HD_VOICE_STATE_DONE = 10;
    // SPRD: Send request to set call forward number whether shown
    protected static final int EVENT_REQUEST_SET_COLP = 11;
    /*SPRD: Bug#542214 Add support for store SMS to Sim card @{*/
    protected static final int EVENT_REQUEST_STORE_SMS_TO_SIM_DONE = 12;
    protected static final int EVENT_QUERY_SMS_STORAGE_MODE_DONE = 13;
    /* @} */

    protected static final int EVENT_UNSOL_RADIOINTERACTOR = 100;
    /**
     *  Listen for update the list of embms programs.
     */
    protected static final int EVENT_UNSOL_RADIOINTERACTOR_EMBMS = 101;
    /**
     *  Listen for RI has connected.
     */
    protected static final int EVENT_UNSOL_RI_CONNECTED = 102;

    public RadioInteractorHandler(RadioInteractorCore RadioInteractorCore,
            RadioInteractorNotifier RadioInteractorNotifier) {
        mRadioInteractorCore = RadioInteractorCore;
        mRadioInteractorNotifier = RadioInteractorNotifier;
        unsolicitedRegisters(this, EVENT_UNSOL_RADIOINTERACTOR);
        registerForRiConnected(this, EVENT_UNSOL_RI_CONNECTED);
        registerForRadioInteractorEmbms(this, EVENT_UNSOL_RADIOINTERACTOR_EMBMS);
        HandlerThread thread = new HandlerThread("RadioInteractor:SyncSender");
        thread.start();
        mHandler = new SyncHandler(thread.getLooper());

    }

    public int getRequestRadioInteractor(int type) {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_GET_REQUEST_RADIOINTERACTOR_DONE,
                    request);
            mRadioInteractorCore.invokeRequestRadiointeractor(type, response);
            waitForResult(request);
        }
        try {
            return (Integer) request.result;
        } catch (ClassCastException e) {
            return -1;
        }
    }

    public int invokeOemRILRequestStrings(String[] oemReq, String[] oemResp) {
        ThreadRequest request = new ThreadRequest(oemResp);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_INVOKE_OEM_RIL_REQUEST_STRINGS_DONE,
                    request);
            mRadioInteractorCore.invokeOemRILRequestStrings(oemReq, response);
            waitForResult(request);
        }
        try {
            return (Integer) request.result;
        } catch (ClassCastException e) {
            return -1;
        }
    }

    public String getSimCapacity() {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_INVOKE_GET_SIM_CAPACITY_DONE, request);
            mRadioInteractorCore.getSimCapacity(response);
            waitForResult(request);
        }
        try {
            return (String) request.result;
        } catch (ClassCastException e) {
            return null;
        }
    }

    public void enableRauNotify() {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_INVOKE_ENABLE_RAU_NOTIFY_DONE, request);
            mRadioInteractorCore.enableRauNotify(response);
            waitForResult(request);
        }
    }

    /* SPRD: Bug#525009 Add support for Open Mobile API @{*/
    public IccOpenLogicalChannelResponse iccOpenLogicalChannel(String AID) {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_OPEN_CHANNEL_DONE, request);
            mRadioInteractorCore.iccOpenLogicalChannel(AID, response);
            waitForResult(request);
        }
        try {
            return (IccOpenLogicalChannelResponse) request.result;
        } catch (ClassCastException e) {
            return null;
        }
    }

    public IccOpenLogicalChannelResponse iccOpenLogicalChannelP2(String AID, byte p2) {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_OPEN_CHANNEL_DONE, request);
            mRadioInteractorCore.iccOpenLogicalChannel(AID, p2, response);
            waitForResult(request);
        }
        try {
            return (IccOpenLogicalChannelResponse) request.result;
        } catch (ClassCastException e) {
            return null;
        }
    }

    public boolean iccCloseLogicalChannel(int channel) {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_CLOSE_CHANNEL_DONE, request);
            mRadioInteractorCore.iccCloseLogicalChannel(channel, response);
            waitForResult(request);
        }
        try {
            return (Boolean) request.result;
        } catch (ClassCastException e) {
            return false;
        }
    }

    public String iccTransmitApduLogicalChannel(int channel, int cla, int command, int p1, int p2,
            int p3, String data) {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_TRANSMIT_APDU_LOGICAL_CHANNEL_DONE, request);
            mRadioInteractorCore.iccTransmitApduLogicalChannel(channel, cla, command, p1, p2, p3, data, response);
            waitForResult(request);
        }
        try {
            return (String) request.result;
        } catch (ClassCastException e) {
            return null;
        }
    }

    public String iccTransmitApduBasicChannel(int cla, int command, int p1, int p2,
            int p3, String data) {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_TRANSMIT_APDU_BASIC_CHANNEL_DONE, request);
            mRadioInteractorCore.iccTransmitApduBasicChannel(cla, command, p1, p2, p3, data, response);
            waitForResult(request);
        }
        try {
            return (String) request.result;
        } catch (ClassCastException e) {
            return null;
        }
    }

    public String iccGetAtr() {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_GET_ATR_DONE, request);
            mRadioInteractorCore.iccGetAtr(response);
            waitForResult(request);
        }
        try {
            return (String) request.result;
        } catch (ClassCastException e) {
            return null;
        }
    }
    /* @} */

    public boolean queryHdVoiceState() {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_GET_HD_VOICE_STATE_DONE, request);
            mRadioInteractorCore.queryHdVoiceState(response);
            waitForResult(request);
        }
        try {
            return (boolean) request.result;
        } catch (ClassCastException e) {
            return false;
        }
    }

    public void setCallingNumberShownEnabled(boolean enabled) {
        Message response = mHandler.obtainMessage(EVENT_REQUEST_SET_COLP);
        mRadioInteractorCore.setCallingNumberShownEnabled(enabled, response);
    }

    /* SPRD: Bug#542214 Add support for store SMS to Sim card @{ */
    public boolean storeSmsToSim(boolean enable) {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_REQUEST_STORE_SMS_TO_SIM_DONE, request);
            mRadioInteractorCore.storeSmsToSim(enable, response);
            waitForResult(request);
        }
        try {
            return (boolean) request.result;
        } catch (ClassCastException e) {
            return false;
        }
    }

    public String querySmsStorageMode() {
        ThreadRequest request = new ThreadRequest(null);
        synchronized (request) {
            Message response = mHandler.obtainMessage(EVENT_QUERY_SMS_STORAGE_MODE_DONE, request);
            mRadioInteractorCore.querySmsStorageMode(response);
            waitForResult(request);
        }
        try {
            return (String) request.result;
        } catch (ClassCastException e) {
            return null;
        }
    }
    /* @} */

    class SyncHandler extends Handler {
        SyncHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            AsyncResult ar;
            ThreadRequest request;
            String strCapacity[];
            UtilLog.logd(TAG, " handleMessage msg.what:" + msg.what);
            switch (msg.what) {

                case EVENT_GET_REQUEST_RADIOINTERACTOR_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (ThreadRequest) ar.userObj;
                    UtilLog.logd(TAG, "handleMessage EVENT_GET_REQUEST_RADIOINTERACTOR_DONE");
                    synchronized (request) {
                        if (ar.exception == null) {
                            request.result = (((int[]) ar.result))[0];
                        } else {
                            UtilLog.loge(TAG, "handleMessage registration state error!");
                            request.result = -1;
                        }
                        request.notifyAll();
                    }
                    break;
                case EVENT_INVOKE_OEM_RIL_REQUEST_STRINGS_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (ThreadRequest) ar.userObj;
                    String[] oemResp = (String[]) request.argument;
                    int returnOemValue = -1;
                    UtilLog.logd(TAG, "handleMessage EVENT_INVOKE_OEM_RIL_REQUEST_STRINGS_DONE");
                    synchronized (request) {
                        try {
                            if (ar.exception == null) {
                                if (ar.result != null) {
                                    String[] responseData = (String[]) (ar.result);
                                    if (responseData.length > oemResp.length) {
                                        UtilLog.logd(TAG,
                                                "Buffer to copy response too small: Response length is "
                                                        +
                                                        responseData.length
                                                        + "bytes. Buffer Size is " +
                                                        oemResp.length + "bytes.");
                                    }
                                    System.arraycopy(responseData, 0, oemResp, 0,
                                            responseData.length);
                                    returnOemValue = responseData.length;
                                }
                            } else {
                                CommandException ex = (CommandException) ar.exception;
                                returnOemValue = ex.getCommandError().ordinal();
                                if (returnOemValue > 0)
                                    returnOemValue *= -1;
                            }
                        } catch (RuntimeException e) {
                            UtilLog.loge(TAG, "sendOemRilRequestRaw: Runtime Exception");
                            returnOemValue = (CommandException.Error.GENERIC_FAILURE.ordinal());
                            if (returnOemValue > 0)
                                returnOemValue *= -1;
                        }
                        request.result = returnOemValue;
                        request.notifyAll();
                    }
                    break;
                case EVENT_INVOKE_GET_SIM_CAPACITY_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (ThreadRequest) ar.userObj;
                    UtilLog.logd(TAG, "handleMessage EVENT_INVOKE_GET_SIM_CAPACITY_DONE");
                    synchronized (request) {
                        if (ar.exception == null) {
                            strCapacity = (String[]) ar.result;
                            if (strCapacity != null && strCapacity.length >= 2) {
                                UtilLog.logd(TAG, "[sms]sim used:" + strCapacity[0] + " total:"
                                        + strCapacity[1]);
                                request.result = strCapacity[0] + ":" + strCapacity[1];
                                UtilLog.logd(TAG, "[sms]simCapacity: " + request.result);
                            } else{
                                request.result = "ERROR";
                            }
                        } else {
                            request.result = "ERROR";
                            UtilLog.loge(TAG, "[sms]get sim capacity fail");
                        }
                        request.notifyAll();
                    }
                    break;
                case EVENT_INVOKE_ENABLE_RAU_NOTIFY_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (ThreadRequest) ar.userObj;
                    UtilLog.logd(TAG, "handleMessage EVENT_INVOKE_ENABLE_RAU_NOTIFY_DONE");
                    synchronized (request) {
                        if (ar.exception == null) {
                            request.result = ar;
                            UtilLog.logd(TAG, "enable rau: " + request.result);
                        } else {
                            UtilLog.loge(TAG, "enable rau:fail");
                        }
                        request.notifyAll();
                    }
                    break;

                    /* SPRD: Bug#525009 Add support for Open Mobile API @{*/
                    case EVENT_OPEN_CHANNEL_DONE:
                        ar = (AsyncResult) msg.obj;
                        request = (ThreadRequest) ar.userObj;
                        UtilLog.logd(TAG, "handleMessage EVENT_OPEN_CHANNEL_DONE");
                        synchronized (request) {
                            if (ar.exception == null && ar.result != null) {
                                int[] result = (int[]) ar.result;
                                int channelId = result[0];
                                byte[] selectResponse = null;
                                if (result.length > 1) {
                                    selectResponse = new byte[result.length - 1];
                                    for (int i = 1; i < result.length; ++i) {
                                        selectResponse[i - 1] = (byte) result[i];
                                    }
                                }
                                request.result = new IccOpenLogicalChannelResponse(channelId,
                                        IccOpenLogicalChannelResponse.STATUS_NO_ERROR, selectResponse);
                            } else {
                                if (ar.result == null) {
                                    UtilLog.loge(TAG, "iccOpenLogicalChannel: Empty response");
                                }
                                if (ar.exception != null) {
                                    UtilLog.loge(TAG, "iccOpenLogicalChannel: Exception: " + ar.exception);
                                }

                                int errorCode = IccOpenLogicalChannelResponse.STATUS_UNKNOWN_ERROR;
                                if (ar.exception instanceof CommandException) {
                                    CommandException.Error error = ((CommandException) (ar.exception)).getCommandError();
                                    if (error == CommandException.Error.MISSING_RESOURCE) {
                                        errorCode = IccOpenLogicalChannelResponse.STATUS_MISSING_RESOURCE;
                                    } else if (error == CommandException.Error.NO_SUCH_ELEMENT) {
                                        errorCode = IccOpenLogicalChannelResponse.STATUS_NO_SUCH_ELEMENT;
                                    }
                                }
                                request.result = new IccOpenLogicalChannelResponse(
                                        IccOpenLogicalChannelResponse.INVALID_CHANNEL, errorCode, null);
                            }
                            request.notifyAll();
                        }
                        break;

                    case EVENT_TRANSMIT_APDU_LOGICAL_CHANNEL_DONE:
                        ar = (AsyncResult) msg.obj;
                        request = (ThreadRequest) ar.userObj;
                        IccIoResult logicalResponse = null;
                        UtilLog.logd(TAG, "handleMessage EVENT_TRANSMIT_APDU_LOGICAL_CHANNEL_DONE");
                        synchronized (request) {
                            if (ar.exception == null) {
                                logicalResponse = (IccIoResult) ar.result;
                            } else {
                                logicalResponse = new IccIoResult(0x6F, 0, (byte[])null);
                                if (ar.result == null) {
                                    UtilLog.loge(TAG, "iccTransmitApduLogicalChannel: Empty response");
                                } else if (ar.exception instanceof CommandException) {
                                    UtilLog.loge(TAG, "iccTransmitApduLogicalChannel: CommandException: " +
                                            ar.exception);
                                } else {
                                    UtilLog.loge(TAG, "iccTransmitApduLogicalChannel: Unknown exception");
                                }
                            }
                            // Append the returned status code to the end of the response payload.
                            request.result = Integer.toHexString(
                                    (logicalResponse.sw1 << 8) + logicalResponse.sw2 + 0x10000).substring(1);
                            if (logicalResponse.payload != null) {
                                request.result = IccUtils.bytesToHexString(logicalResponse.payload)
                                        + request.result;
                            }
                            request.notifyAll();
                        }
                        break;

                    case EVENT_TRANSMIT_APDU_BASIC_CHANNEL_DONE:
                        ar = (AsyncResult) msg.obj;
                        request = (ThreadRequest) ar.userObj;
                        IccIoResult basicResponse = null;
                        UtilLog.logd(TAG, "handleMessage EVENT_TRANSMIT_APDU_BASIC_CHANNEL_DONE");
                        synchronized (request) {
                            if (ar.exception == null) {
                                basicResponse = (IccIoResult) ar.result;
                            } else {
                                basicResponse = new IccIoResult(0x6F, 0, (byte[])null);
                                if (ar.result == null) {
                                    UtilLog.loge(TAG, "iccTransmitApduLogicalChannel: Empty response");
                                } else if (ar.exception instanceof CommandException) {
                                    UtilLog.loge(TAG, "iccTransmitApduLogicalChannel: CommandException: " +
                                            ar.exception);
                                } else {
                                    UtilLog.loge(TAG, "iccTransmitApduLogicalChannel: Unknown exception");
                                }
                            }
                            // Append the returned status code to the end of the response payload.
                            request.result = Integer.toHexString(
                                    (basicResponse.sw1 << 8) + basicResponse.sw2 + 0x10000).substring(1);
                            if (basicResponse.payload != null) {
                                request.result = IccUtils.bytesToHexString(basicResponse.payload)
                                        + request.result;
                            }
                            request.notifyAll();
                        }
                        break;

                    case EVENT_CLOSE_CHANNEL_DONE:
                        ar = (AsyncResult) msg.obj;
                        request = (ThreadRequest) ar.userObj;
                        UtilLog.logd(TAG, "handleMessage EVENT_CLOSE_CHANNEL_DONE");
                        synchronized (request) {
                            if (ar.exception == null) {
                                request.result = true;
                            } else {
                                request.result = false;
                                if (ar.exception instanceof CommandException) {
                                    UtilLog.loge(TAG, "EVENT_CLOSE_CHANNEL_DONE : CommandException: " + ar.exception);
                                } else {
                                    UtilLog.loge(TAG, "EVENT_CLOSE_CHANNEL_DONE : Unknown exception");
                                }
                            }
                            request.notifyAll();
                        }
                        break;

                    case EVENT_GET_ATR_DONE:
                        ar = (AsyncResult) msg.obj;
                        request = (ThreadRequest) ar.userObj;
                        UtilLog.logd(TAG, "handleMessage EVENT_GET_ATR_DONE");
                        synchronized (request) {
                            if (ar.exception == null && ar.result != null) {
                                request.result = (String) ar.result;
                            } else {
                                request.result = "ERROR";
                                if (ar.result == null) {
                                    UtilLog.loge(TAG, "iccOpenLogicalChannel: Empty response");
                                }
                                if (ar.exception != null) {
                                    UtilLog.loge(TAG, "iccOpenLogicalChannel: Exception: " + ar.exception);
                                }
                            }
                            request.notifyAll();
                        }
                        break;
                    /* @} */
                case EVENT_GET_HD_VOICE_STATE_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (ThreadRequest) ar.userObj;
                    UtilLog.logd(TAG, "handleMessage EVENT_GET_HD_VOICE_STATE_DONE");
                    synchronized (request) {
                        if (ar.exception == null && ar.result != null) {
                            int resultArray[] = (int[]) ar.result;
                            request.result = (resultArray[0] == 1);
                        } else {
                            request.result = false;
                            if (ar.exception != null) {
                                UtilLog.loge(TAG, "get HD Voice state fail: " + ar.exception);
                            }
                        }
                        request.notifyAll();
                    }
                    break;

                case EVENT_REQUEST_SET_COLP:
                    ar = (AsyncResult) msg.obj;
                    UtilLog.logd(TAG, "handleMessage EVENT_REQUEST_SET_COLP");
                    if (ar.exception == null) {
                        UtilLog.logd(TAG, "set colp　:success"+ ar.result);
                    } else {
                        UtilLog.loge(TAG, "set colp　:fail");
                    }
                    break;

                /* SPRD: Bug#542214 Add support for store SMS to Sim card @{ */
                case EVENT_REQUEST_STORE_SMS_TO_SIM_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (ThreadRequest) ar.userObj;
                    UtilLog.logd(TAG, "handleMessage EVENT_REQUEST_STORE_SMS_TO_SIM_DONE");
                    synchronized (request) {
                        if (ar.exception == null) {
                            request.result = true;
                            UtilLog.logd(TAG, "store sms to sim: " + request.result);
                        } else {
                            request.result = false;
                            UtilLog.loge(TAG, "store sms to sim:fail" + ar.exception);
                        }
                        request.notifyAll();
                    }
                    break;

                case EVENT_QUERY_SMS_STORAGE_MODE_DONE:
                    ar = (AsyncResult) msg.obj;
                    request = (ThreadRequest) ar.userObj;
                    UtilLog.logd(TAG, "handleMessage EVENT_QUERY_SMS_STORAGE_MODE_DONE");
                    synchronized (request) {
                        if (ar.exception == null && ar.result != null) {
                            request.result = (String) ar.result;
                        } else {
                            request.result = "ERROR";
                            if (ar.result == null) {
                                UtilLog.loge(TAG, "query sms storage mode: Empty response");
                            }
                            if (ar.exception != null) {
                                UtilLog.loge(TAG,
                                        "query sms storage mode: Exception: " + ar.exception);
                            }
                        }
                        request.notifyAll();
                    }
                    break;
                /* @} */

                default:
                    throw new RuntimeException("Unrecognized request event radiointeractor: " + msg.what);
            }
        }
    }

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;
        switch (msg.what) {

            case EVENT_UNSOL_RADIOINTERACTOR:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    UtilLog.logd(TAG, "EVENT_UNSOL_RADIOINTERACTOR");
                    mRadioInteractorNotifier
                            .notifyRadiointeractorEventForSubscriber(mRadioInteractorCore
                                    .getPhoneId());
                } else {
                    UtilLog.loge(TAG, "unsolicitedRegisters exception: " + ar.exception);
                }
                break;

            case EVENT_UNSOL_RI_CONNECTED:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    UtilLog.logd(TAG, "EVENT_UNSOL_RI_CONNECTED");
                } else {
                    UtilLog.loge(TAG, "unsolicitedRiConnected exception: " + ar.exception);
                }
                break;

            case EVENT_UNSOL_RADIOINTERACTOR_EMBMS:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    UtilLog.logd(TAG, "EVENT_UNSOL_RADIOINTERACTOR_EMBMS");
                    mRadioInteractorNotifier
                            .notifyRadiointeractorEventForEmbms(mRadioInteractorCore
                                    .getPhoneId());
                } else {
                    UtilLog.loge(TAG, "unsolicitedRadioInteractorEmbms exception: " + ar.exception);
                }
                break;

            default:
                throw new RuntimeException("Unrecognized event unsol radiointeractor: " + msg.what);

        }
    }

    public void unsolicitedRegisters(Handler h, int what) {
        mRadioInteractorCore
                .registerForUnsolRadioInteractor(h, what, null);
    }

    public void unregisterForUnsolRadioInteractor(Handler h) {
        mRadioInteractorCore.unregisterForUnsolRadioInteractor(h);
    }

    public void registerForRiConnected(Handler h, int what) {
        mRadioInteractorCore.registerForUnsolRiConnected(h, what, null);
    }

    public void unregisterForRiConnected(Handler h) {
        mRadioInteractorCore.unregisterForUnsolRiConnected(h);
    }

    public void registerForRadioInteractorEmbms(Handler h, int what) {
        mRadioInteractorCore.registerForUnsolRadioInteractor(h, what, null);
    }

    public void unregisterForRadioInteractorEmbms(Handler h) {
        mRadioInteractorCore.unregisterForUnsolRadioInteractor(h);
    }

    private static final class ThreadRequest {
        public Object argument;
        public Object result;

        public ThreadRequest(Object argument) {
            this.argument = argument;
        }
    }

    private void waitForResult(ThreadRequest request) {
            try {
                request.wait();
            } catch (InterruptedException e) {
                UtilLog.logd(TAG, "interrupted while trying to get remain times");
            }
    }

}
