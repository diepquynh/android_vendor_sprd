
package com.android.sprd.telephony;

import static com.android.internal.telephony.RILConstants.GENERIC_FAILURE;
import static com.android.internal.telephony.RILConstants.RADIO_NOT_AVAILABLE;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_EXPLICIT_CALL_TRANSFER;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_GET_IMSI;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SIM_IO;
import static com.android.internal.telephony.RILConstants.RIL_RESPONSE_ACKNOWLEDGEMENT;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_GET_SIM_STATUS;
import static com.android.internal.telephony.RILConstants.RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_RADIOINTERACTOR;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_OEM_HOOK_STRINGS;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_SIM_CAPACITY;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_DEFAULT_NAN;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ENABLE_RAU_NOTIFY;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_TRANSMIT_APDU_BASIC;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_TRANSMIT_APDU_CHANNEL;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_OPEN_CHANNEL;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_CLOSE_CHANNEL;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_GET_ATR;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_OPEN_CHANNEL_WITH_P2;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_HD_VOICE_STATE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ENABLE_COLP;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_STORE_SMS_TO_SIM;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_QUERY_SMS_STORAGE_MODE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SWITCH_MULTI_CALL;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_DIAL;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_CODEC;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_FALLBACK;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_STRING;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_LOCAL_MEDIA;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_VIDEOPHONE_CONTROL_IFRAME;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_DC_TRAFFIC_CLASS;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ENABLE_LTE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ATTACH_DATACONN;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ABORT_SEARCH_NETWORK;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_DC_FORCE_DETACH;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SHUTDOWN;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_POWER;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_UPDATE_REAL_ECCLIST;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_BAND_INFO;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_BAND_INFO_MODE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_SPECIAL_RATCAP;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_QUERY_COLP;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_QUERY_COLR;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_MMI_ENTER_SIM;

import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RADIOINTERACTOR_EMBMS;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RI_CONNECTED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_CODEC;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_DSCI;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_STRING;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_REMOTE_MEDIA;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_MM_RING;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_RELEASING;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_RECORD_VIDEO;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_VIDEOPHONE_MEDIA_START;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RADIOINTERACTOR_EMBMS;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RI_CONNECTED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_ECC_NETWORKLIST_CHANGED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RAU_NOTIFY;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_CLEAR_CODE_FALLBACK;
import static com.android.sprd.telephony.RIConstants.RI_RESPONSE_ACKNOWLEDGEMENT;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_FACILITY_LOCK_FOR_USER;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_SIMLOCK_REMAIN_TIMES;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_SIMLOCK_STATUS;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_SIMLOCK_STATUS_CHANGED;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_BAND_INFO;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SET_SINGLE_PDN;

import java.io.IOException;
import java.io.InputStream;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.Parcel;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.SystemProperties;
import android.telephony.Rlog;
import android.util.SparseArray;

import com.android.internal.telephony.TelephonyProperties;
import com.android.sprd.telephony.uicc.IccCardApplicationStatusEx;
import com.android.sprd.telephony.uicc.IccCardStatusEx;
import com.android.sprd.telephony.uicc.IccIoResult;

/**
 * {@hide}
 */
class RIRequest {
    static final String LOG_TAG = "RIRequest";

    // ***** Class Variables
    static Random sRandom = new Random();
    static AtomicInteger sNextSerial = new AtomicInteger(0);
    private static Object sPoolSync = new Object();
    private static RIRequest sPool = null;
    private static int sPoolSize = 0;
    private static final int MAX_POOL_SIZE = 4;
    private Context mContext;

    // ***** Instance Variables
    int mSerial;
    int mRequest;
    Message mResult;
    Parcel mParcel;
    RIRequest mNext;
    int mWakeLockType;

    /**
     * Retrieves a new RIRequest instance from the pool.
     *
     * @param request RI_REQUEST_*
     * @param result sent when operation completes
     * @return a RIRequest instance from the pool.
     */
    static RIRequest obtain(int request, Message result) {
        RIRequest rr = null;

        synchronized (sPoolSync) {
            if (sPool != null) {
                rr = sPool;
                sPool = rr.mNext;
                rr.mNext = null;
                sPoolSize--;
            }
        }

        if (rr == null) {
            rr = new RIRequest();
        }

        rr.mSerial = sNextSerial.getAndIncrement();

        rr.mRequest = request;
        rr.mResult = result;
        rr.mParcel = Parcel.obtain();

        rr.mWakeLockType = RadioInteractorCore.INVALID_WAKELOCK;
        if (result != null && result.getTarget() == null) {
            throw new NullPointerException("Message target must not be null");
        }

        // first elements in any RIL Parcel
        rr.mParcel.writeInt(request);
        rr.mParcel.writeInt(rr.mSerial);

        return rr;
    }

    /**
     * Returns a RIRequest instance to the pool. Note: This should only be called once per use.
     */
    void release() {
        synchronized (sPoolSync) {
            if (sPoolSize < MAX_POOL_SIZE) {
                mNext = sPool;
                sPool = this;
                sPoolSize++;
                mResult = null;
                if(mWakeLockType != RadioInteractorCore.INVALID_WAKELOCK) {
                    //This is OK for some wakelock types and not others
                    if(mWakeLockType == RadioInteractorCore.FOR_WAKELOCK) {
                        Rlog.e(LOG_TAG, "RIRequest releasing with held wake lock: "
                                + serialString());
                    }
                }
            }
        }
    }

    private RIRequest() {
    }

    static void
            resetSerial() {
        // use a random so that on recovery we probably don't mix old requests
        // with new.
        sNextSerial.set(sRandom.nextInt());
    }

    String
            serialString() {
        // Cheesy way to do %04d
        StringBuilder sb = new StringBuilder(8);
        String sn;

        long adjustedSerial = (((long) mSerial) - Integer.MIN_VALUE) % 10000;

        sn = Long.toString(adjustedSerial);

        sb.append('[');
        for (int i = 0, s = sn.length(); i < 4 - s; i++) {
            sb.append('0');
        }

        sb.append(sn);
        sb.append(']');
        return sb.toString();
    }

    void
            onError(int error, Object ret) {
        CommandException ex;

        ex = CommandException.fromRilErrno(error);

        UtilLog.logd(LOG_TAG, serialString() + "< "
                + RadioInteractorCore.requestToString(mRequest)
                + " error: " + ex + " ret=" + RadioInteractorCore.retToString(mRequest, ret));

        if (mResult != null) {
            AsyncResult.forMessage(mResult, ret, ex);
            mResult.sendToTarget();
        }

        if (mParcel != null) {
            mParcel.recycle();
            mParcel = null;
        }
    }
}

public class RadioInteractorCore {
    public static final String TAG = "RadioInteractor";
    // Have a separate wakelock instance for Ack
    static final String RILJ_ACK_WAKELOCK_NAME = "RI_ACK_WL";
    static final boolean RIJ_LOGD = true;

    static final int RIL_MAX_COMMAND_BYTES = (8 * 1024);
    private Integer mInstanceId;
    static final String[] SOCKET_NAME_RIL = {
            "oem_socket1", "oem_socket2", "oem_socket3"
    };
    static final int SOCKET_OPEN_RETRY_MILLIS = 4 * 1000;
    LocalSocket mSocket;
    static final int RESPONSE_SOLICITED = 0;
    static final int RESPONSE_UNSOLICITED = 1;
    static final int RESPONSE_SOLICITED_ACK = 2;
    static final int RESPONSE_SOLICITED_ACK_EXP = 3;
    static final int RESPONSE_UNSOLICITED_ACK_EXP = 4;
    final WakeLock mWakeLock;           // Wake lock associated with request/response
    final WakeLock mAckWakeLock;        // Wake lock associated with ack sent
    final int mWakeLockTimeout;         // Timeout associated with request/response
    final int mAckWakeLockTimeout;      // Timeout associated with ack sent
    int mWakeLockCount;
    // Variables used to identify releasing of WL on wakelock timeouts
    volatile int mWlSequenceNum = 0;
    volatile int mAckWlSequenceNum = 0;
    static final int EVENT_SEND = 1;
    static final int EVENT_WAKE_LOCK_TIMEOUT = 2;
    static final int EVENT_SEND_ACK             = 3;
    static final int EVENT_ACK_WAKE_LOCK_TIMEOUT    = 4;
    static final int EVENT_BLOCKING_RESPONSE_TIMEOUT = 5;
    RILSender mSender;
    SparseArray<RIRequest> mRequestList = new SparseArray<RIRequest>();
    Thread mReceiverThread;
    RILReceiver mReceiver;
    private static final int DEFAULT_WAKE_LOCK_TIMEOUT_MS = 60000;
    // Wake lock default timeout associated with ack
    private static final int DEFAULT_ACK_WAKE_LOCK_TIMEOUT_MS = 200;

    private static final int DEFAULT_BLOCKING_MESSAGE_RESPONSE_TIMEOUT_MS = 2000;

    // Variables used to differentiate ack messages from request while calling clearWakeLock()
    public static final int INVALID_WAKELOCK = -1;
    public static final int FOR_WAKELOCK = 0;
    public static final int FOR_ACK_WAKELOCK = 1;

    HandlerThread mSenderThread;
    boolean mIsRiConnected;

    // RIL Version
    private int mRilVersion = -1;

    protected RegistrantList mUnsolRadiointeractorRegistrants = new RegistrantList();
    protected RegistrantList mUnsolRIConnectedRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPCodecRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPFailRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPFallBackRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPStrsRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPRemoteMediaRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPMMRingRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPRecordVideoRegistrants = new RegistrantList();
    protected RegistrantList mUnsolVPMediaStartRegistrants = new RegistrantList();
    protected RegistrantList mUnsolEccNetChangedRegistrants = new RegistrantList();
    protected RegistrantList mUnsolRauSuccessRegistrants = new RegistrantList();
    protected RegistrantList mUnsolClearCodeFallbackRegistrants = new RegistrantList();
    protected RegistrantList mSimlockStatusChangedRegistrants = new RegistrantList();
    protected RegistrantList mUnsolBandInfoRegistrants = new RegistrantList();

    public RadioInteractorCore(Context context, Integer instanceId) {
        mInstanceId = instanceId;
        ConnectivityManager cm = (ConnectivityManager) context.getSystemService(
                Context.CONNECTIVITY_SERVICE);
        if (cm.isNetworkSupported(ConnectivityManager.TYPE_MOBILE) == false) {
            UtilLog.logd(TAG, "Not starting RILReceiver: wifi-only");
        } else {
            UtilLog.logd(TAG, "Starting RILReceiver" + mInstanceId);
            mReceiver = new RILReceiver();
            mReceiverThread = new Thread(mReceiver, "RILReceiver" + mInstanceId);
            mReceiverThread.start();
        }
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "RadioInteractorCore");
        mWakeLock.setReferenceCounted(false);
        mAckWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, RILJ_ACK_WAKELOCK_NAME);
        mAckWakeLock.setReferenceCounted(false);
        mWakeLockTimeout = SystemProperties.getInt(TelephonyProperties.PROPERTY_WAKE_LOCK_TIMEOUT,
                DEFAULT_WAKE_LOCK_TIMEOUT_MS);
        mAckWakeLockTimeout = SystemProperties.getInt(
                TelephonyProperties.PROPERTY_WAKE_LOCK_TIMEOUT, DEFAULT_ACK_WAKE_LOCK_TIMEOUT_MS);
        mWakeLockCount = 0;

        mSenderThread = new HandlerThread("RILSender" + mInstanceId);
        mSenderThread.start();
        Looper looper = mSenderThread.getLooper();
        mSender = new RILSender(looper);
    }

    class RILReceiver implements Runnable {
        byte[] buffer;

        RILReceiver() {
            buffer = new byte[RIL_MAX_COMMAND_BYTES];
        }

        @Override
        public void
                run() {
            int retryCount = 0;
            String rilSocket = "oem_socket1";

            try {
                for (;;) {
                    LocalSocket s = null;
                    LocalSocketAddress l;

                    if (mInstanceId == null || mInstanceId == 0) {
                        rilSocket = SOCKET_NAME_RIL[0];
                    } else {
                        rilSocket = SOCKET_NAME_RIL[mInstanceId];
                    }

                    try {
                        s = new LocalSocket();
                        l = new LocalSocketAddress(rilSocket,
                                LocalSocketAddress.Namespace.ABSTRACT);
                        s.connect(l);
                    } catch (IOException ex) {
                        try {
                            if (s != null) {
                                s.close();
                            }
                        } catch (IOException ex2) {
                            // ignore failure to close after failure to connect
                        }

                        // don't print an error message after the the first time
                        // or after the 8th time

                        if (retryCount == 8) {
                            UtilLog.loge(TAG,
                                    "Couldn't find '" + rilSocket
                                            + "' socket after " + retryCount
                                            + " times, continuing to retry silently");
                        } else if (retryCount >= 0 && retryCount < 8) {
                            UtilLog.logd(TAG,
                                    "Couldn't find '" + rilSocket
                                            + "' socket; retrying after timeout");
                        }

                        try {
                            Thread.sleep(SOCKET_OPEN_RETRY_MILLIS);
                        } catch (InterruptedException er) {
                        }

                        retryCount++;
                        continue;
                    }

                    retryCount = 0;

                    mSocket = s;
                    UtilLog.logd(TAG, "(" + mInstanceId + ") Connected to '"
                            + rilSocket + "' socket");

                    int length = 0;
                    try {
                        InputStream is = mSocket.getInputStream();
                        for (;;) {
                            Parcel p;
                            length = readRilMessage(is, buffer);
                            if (length < 0) {
                                // End-of-stream reached
                                break;
                            }

                            p = Parcel.obtain();
                            p.unmarshall(buffer, 0, length);
                            p.setDataPosition(0);

                            processResponse(p);
                            p.recycle();
                        }
                    } catch (java.io.IOException ex) {
                        UtilLog.loge(TAG, "'" + rilSocket + "' socket closed",
                                ex);
                    } catch (Throwable tr) {
                        UtilLog.loge(TAG, "Uncaught exception read length=" + length +
                                "Exception:" + tr.toString());
                    }

                    UtilLog.logd(TAG, "(" + mInstanceId + ") Disconnected from '" + rilSocket
                            + "' socket");

                    mIsRiConnected = false;

                    setRadioState(RadioState.RADIO_UNAVAILABLE);

                    try {
                        mSocket.close();
                    } catch (IOException ex) {
                    }

                    mSocket = null;
                    RIRequest.resetSerial();

                    // Clear request list on close
                    clearRequestList(RADIO_NOT_AVAILABLE, false);
                }
            } catch (Throwable tr) {
                UtilLog.loge(TAG, "Uncaught exception", tr);
            }

            /* We're disconnected so we don't know the ril version */
            notifyRegistrantsRilConnectionChanged(-1);
        }
    }

    private void notifyRegistrantsRilConnectionChanged(int rilVer) {
        mRilVersion = rilVer;
        if (mUnsolRIConnectedRegistrants != null) {
            mUnsolRIConnectedRegistrants.notifyRegistrants(
                                new AsyncResult (null, new Integer(rilVer), null));
        }
    }

    protected void setRadioState(RadioState newState) {

    }

    enum RadioState {
        RADIO_OFF, /* Radio explicitly powered off (eg CFUN=0) */
        RADIO_UNAVAILABLE, /* Radio unavailable (eg, resetting or not booted) */
        RADIO_ON; /* Radio is on */

        public boolean isOn() /* and available... */{
            return this == RADIO_ON;
        }

        public boolean isAvailable() {
            return this != RADIO_UNAVAILABLE;
        }
    }

    enum SuppService {
        UNKNOWN, SWITCH, SEPARATE, TRANSFER, CONFERENCE, REJECT, HANGUP, RESUME;
    }

    private void clearRequestList(int error, boolean loggable) {
        RIRequest rr;
        synchronized (mRequestList) {
            int count = mRequestList.size();
            if (loggable) {
                Rlog.d(TAG, "clearRequestList " +
                        " mWakeLockCount=" + mWakeLockCount +
                        " mRequestList=" + count);
            }

            for (int i = 0; i < count; i++) {
                rr = mRequestList.valueAt(i);
                if (loggable) {
                    Rlog.d(TAG, i + ": [" + rr.mSerial + "] " +
                            requestToString(rr.mRequest));
                }
                rr.onError(error, null);
                decrementWakeLock(rr);
                rr.release();
            }
            mRequestList.clear();
        }
    }

    private static int readRilMessage(InputStream is, byte[] buffer)
            throws IOException {
        int countRead;
        int offset;
        int remaining;
        int messageLength;

        // First, read in the length of the message
        offset = 0;
        remaining = 4;
        do {
            countRead = is.read(buffer, offset, remaining);
            if (countRead < 0) {
                UtilLog.loge(TAG, "Hit EOS reading message length");
                return -1;
            }
            offset += countRead;
            remaining -= countRead;
        } while (remaining > 0);

        messageLength = ((buffer[0] & 0xff) << 24)
                | ((buffer[1] & 0xff) << 16)
                | ((buffer[2] & 0xff) << 8)
                | (buffer[3] & 0xff);

        // Then, re-use the buffer and read in the message itself
        offset = 0;
        remaining = messageLength;
        do {
            countRead = is.read(buffer, offset, remaining);

            if (countRead < 0) {
                UtilLog.loge(TAG, "Hit EOS reading message.  messageLength=" + messageLength
                        + " remaining=" + remaining);
                return -1;
            }

            offset += countRead;
            remaining -= countRead;
        } while (remaining > 0);

        return messageLength;
    }

    private void
    processResponse (Parcel p) {
        int type;

        type = p.readInt();

        if (type == RESPONSE_UNSOLICITED || type == RESPONSE_UNSOLICITED_ACK_EXP) {
            processUnsolicited (p, type);
        } else if (type == RESPONSE_SOLICITED || type == RESPONSE_SOLICITED_ACK_EXP) {
            RIRequest rr = processSolicited (p, type);
            if (rr != null) {
                if (type == RESPONSE_SOLICITED) {
                    decrementWakeLock(rr);
                }
                rr.release();
                return;
            }
        } else if (type == RESPONSE_SOLICITED_ACK) {
            int serial;
            serial = p.readInt();

            RIRequest rr;
            synchronized (mRequestList) {
                rr = mRequestList.get(serial);
            }
            if (rr == null) {
                Rlog.w(TAG, "Unexpected solicited ack response! sn: " + serial);
            } else {
                decrementWakeLock(rr);
                if (RIJ_LOGD) {
                    riljLog(rr.serialString() + " Ack < " + requestToString(rr.mRequest));
                }
            }
        }
    }

    private void processUnsolicited(Parcel p, int type) {
        int response;
        Object ret;
        int[] params;

        response = p.readInt();

        // Follow new symantics of sending an Ack starting from RIL version 13
        if (mRilVersion >= 13 && type == RESPONSE_UNSOLICITED_ACK_EXP) {
            Message msg;
            RIRequest rr = RIRequest.obtain(RIL_RESPONSE_ACKNOWLEDGEMENT, null);
            msg = mSender.obtainMessage(EVENT_SEND_ACK, rr);
            acquireWakeLock(rr, FOR_ACK_WAKELOCK);
            msg.sendToTarget();
            if (RIJ_LOGD) {
                riljLog("Unsol response received for " + responseToString(response) +
                        " Sending ack to ril.cpp");
            }
        }

        try {
            switch (response) {

                case RI_UNSOL_RI_CONNECTED:
                    ret = responseVoid(p);
                    break;
                case RI_UNSOL_RADIOINTERACTOR_EMBMS:
                    ret = responseVoid(p);
                    break;
                case RI_UNSOL_VIDEOPHONE_CODEC:
                    ret = responseInts(p);
                    break;
                case RI_UNSOL_VIDEOPHONE_DSCI:
                    ret = responseDSCI(p);
                    break;
                case RI_UNSOL_VIDEOPHONE_STRING:
                    ret = responseString(p);
                    break;
                case RI_UNSOL_VIDEOPHONE_REMOTE_MEDIA:
                    ret = responseInts(p);
                    break;
                case RI_UNSOL_VIDEOPHONE_MM_RING:
                    ret = responseInts(p);
                    break;
                case RI_UNSOL_VIDEOPHONE_RELEASING:
                    ret = responseString(p);
                    break;
                case RI_UNSOL_VIDEOPHONE_RECORD_VIDEO:
                    ret =responseInts(p);
                    break;
                case RI_UNSOL_VIDEOPHONE_MEDIA_START:
                    ret = responseInts(p);
                    break;
                case RI_UNSOL_ECC_NETWORKLIST_CHANGED:
                    ret = responseString(p);
                    break;
                case RI_UNSOL_RAU_NOTIFY:
                    ret = responseVoid(p);
                    break;
                case RI_UNSOL_CLEAR_CODE_FALLBACK:
                    ret = responseVoid(p);
                    break;
                case RI_UNSOL_SIMLOCK_STATUS_CHANGED:
                    ret =  responseVoid(p);
                    break;
                case RI_UNSOL_BAND_INFO:
                    ret = responseString(p);
                    break;
                default:
                    throw new RuntimeException("Unrecognized unsol response: " + response);
            }
        } catch (Throwable tr) {
            UtilLog.loge(TAG, "Exception processing unsol response: " + response +
                    "Exception:" + tr.toString());
            return;
        }

        switch (response) {

            case RI_UNSOL_RI_CONNECTED:
                if (RIJ_LOGD)
                    unsljLog(response);
                if (mUnsolRIConnectedRegistrants != null) {
                    mIsRiConnected = true;
                    mUnsolRIConnectedRegistrants.notifyRegistrants(new AsyncResult(null, ret,
                            null));
                }
                break;
            case RI_UNSOL_RADIOINTERACTOR_EMBMS:
                if (RIJ_LOGD)
                    unsljLog(response);
                if (mUnsolRadiointeractorRegistrants != null) {
                    mUnsolRadiointeractorRegistrants.notifyRegistrants(new AsyncResult(null, ret,
                            null));
                }
                break;
            case RI_UNSOL_VIDEOPHONE_CODEC:
                if (RIJ_LOGD)
                    unsljLogRet(response,ret);
                params = (int[]) ret;
                if (mUnsolVPCodecRegistrants != null) {
                    mUnsolVPCodecRegistrants.notifyRegistrants(new AsyncResult(null, params,
                            null));
                }
                break;
            case RI_UNSOL_VIDEOPHONE_DSCI:
                if (RIJ_LOGD)
                    unsljLogRet(response, ret);

                DSCIInfo info = (DSCIInfo) ret;
                if (info.cause > 0) {
                    if (RIJ_LOGD)
                        riljLog(" RI_UNSOL_VIDEOPHONE_DSCI number: " + info.number + ", cause: "
                                + info.cause + ", location: " + info.location);
                    if ((info.cause == 47) || (info.cause == 57) || (info.cause == 50) || (info.cause == 58)
                            || (info.cause == 88) || (info.cause == 69)) {
                        if (mUnsolVPFallBackRegistrants != null) {
                            if ((info.cause == 57 ||info.cause == 50) && (info.location <= 2)) { // if  cause = 57 and location <= 2, it mean current sim hasn't start vt service
                                mUnsolVPFallBackRegistrants.notifyRegistrants(new AsyncResult(null,new AsyncResult(info.idr,
                                        new AsyncResult(info.number, (info.location==2?(info.cause + 200):(info.cause + 100)), null), null),null));//SPRD modify for 303916
                            } else  {
                                mUnsolVPFallBackRegistrants.notifyRegistrants(new AsyncResult(null,new AsyncResult(info.idr,
                                        new AsyncResult(info.number, info.cause, null), null),null));
                            }
                        }
                    } else {
                        if (mUnsolVPFailRegistrants != null) {
                            mUnsolVPFailRegistrants.notifyRegistrants(new AsyncResult(null,new AsyncResult(info.idr,
                                    new AsyncResult(info.number, info.cause, null), null),null));
                        }
                    }
                }
                break;
            case RI_UNSOL_VIDEOPHONE_STRING:
                if (RIJ_LOGD)
                    unsljLog(response);

                if (mUnsolVPStrsRegistrants != null) {
                    mUnsolVPStrsRegistrants.notifyRegistrants(new AsyncResult(null, ret, null));
                }
                break;
            case RI_UNSOL_VIDEOPHONE_REMOTE_MEDIA:
                if (RIJ_LOGD)
                    unsljLogRet(response, ret);

                params = (int[]) ret;

                if (params.length >= 2) {
                    if (mUnsolVPRemoteMediaRegistrants != null) {
                        mUnsolVPRemoteMediaRegistrants.notifyRegistrants(new AsyncResult(null, params,
                                null));
                    }
                } else {
                    if (RIJ_LOGD)
                        riljLog(" RI_UNSOL_VIDEOPHONE_REMOTE_MEDIA ERROR with wrong length "
                                + params.length);
                }
                break;
            case RI_UNSOL_VIDEOPHONE_MM_RING:
                if (RIJ_LOGD)
                    unsljLogRet(response, ret);

                params = (int[]) ret;

                if (params.length == 1) {
                    if (mUnsolVPMMRingRegistrants != null) {
                        mUnsolVPMMRingRegistrants.notifyRegistrants(new AsyncResult(null, params,
                                null));
                    }
                } else {
                    if (RIJ_LOGD)
                        riljLog(" RI_UNSOL_VIDEOPHONE_MM_RING ERROR with wrong length "
                                + params.length);
                }
                break;
            case RI_UNSOL_VIDEOPHONE_RELEASING:
                if (RIJ_LOGD) unsljLogRet(response, ret);

                String str = (String)ret;
                if (mUnsolVPFailRegistrants != null) {
                    mUnsolVPFailRegistrants.notifyRegistrants(new AsyncResult(null,new AsyncResult(null, new AsyncResult(str, 1000, null), null),null));
                }
                break;
            case RI_UNSOL_VIDEOPHONE_RECORD_VIDEO:
                if (RIJ_LOGD)
                    unsljLogRet(response, ret);

                params = (int[]) ret;

                if (params.length == 1) {
                    if (mUnsolVPRecordVideoRegistrants != null) {
                        mUnsolVPRecordVideoRegistrants.notifyRegistrants(new AsyncResult(null, params,
                                null));
                    }
                } else {
                    if (RIJ_LOGD)
                        riljLog(" RI_UNSOL_VIDEOPHONE_RECORD_VIDEO ERROR with wrong length "
                                + params.length);
                }
                break;
            case RI_UNSOL_VIDEOPHONE_MEDIA_START:
                if (RIJ_LOGD)unsljLog(response);

                if (mUnsolVPMediaStartRegistrants != null) {
                    params = (int[]) ret;
                    if (params[0] == 1) {
                        mUnsolVPMediaStartRegistrants.notifyRegistrants(new AsyncResult(null, params, null));
                    }
                }
                break;
            case RI_UNSOL_ECC_NETWORKLIST_CHANGED:
                unsljLog(response);
                if (mUnsolEccNetChangedRegistrants != null) {
                    mUnsolEccNetChangedRegistrants.notifyRegistrants(new AsyncResult(null, ret,
                            null));
                }
                break;
            case RI_UNSOL_RAU_NOTIFY:
                unsljLog(response);
                if (mUnsolRauSuccessRegistrants != null) {
                    mUnsolRauSuccessRegistrants.notifyRegistrants(new AsyncResult(null, ret,
                            null));
                }
                break;
            case RI_UNSOL_CLEAR_CODE_FALLBACK:
                unsljLog(response);
                if (mUnsolClearCodeFallbackRegistrants != null) {
                    mUnsolClearCodeFallbackRegistrants.notifyRegistrants(new AsyncResult(null, ret,
                            null));
                }
                break;
            case RI_UNSOL_SIMLOCK_STATUS_CHANGED:
                if (RIJ_LOGD) unsljLog(response);

                if (mSimlockStatusChangedRegistrants != null) {
                    mSimlockStatusChangedRegistrants.notifyRegistrants();
                }
                break;
            case RI_UNSOL_BAND_INFO:
                if (RIJ_LOGD)
                    unsljLog(response);
                if (mUnsolBandInfoRegistrants != null) {
                    mUnsolBandInfoRegistrants.notifyRegistrants(new AsyncResult(null, ret,
                            null));
                }
                break;
        }
    }

    public void registerForUnsolRadioInteractor(Handler h, int what, Object obj) {

        Registrant r = new Registrant(h, what, obj);
        mUnsolRadiointeractorRegistrants.add(r);
    }

    public void unregisterForUnsolRadioInteractor(Handler h) {
        mUnsolRadiointeractorRegistrants.remove(h);
    }

    public void registerForUnsolRiConnected(Handler h, int what, Object obj) {

        Registrant r = new Registrant(h, what, obj);
        mUnsolRIConnectedRegistrants.add(r);
        if(mIsRiConnected){ // FIXME add mRilVersion != -1 ?
            mUnsolRIConnectedRegistrants
                    .notifyRegistrants(new AsyncResult(null, new Integer(mRilVersion), null));
        }
    }

    public void unregisterForUnsolRiConnected(Handler h) {
        mUnsolRIConnectedRegistrants.remove(h);
    }

    public void registerForsetOnVPCodec(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPCodecRegistrants.add(r);
    }

    public void unregisterForsetOnVPCodec(Handler h) {
        mUnsolVPCodecRegistrants.remove(h);
    }

    public void registerForsetOnVPFallBack(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPFallBackRegistrants.add(r);
    }

    public void unregisterForsetOnVPFallBack(Handler h) {
        mUnsolVPFallBackRegistrants.remove(h);
    }

    public void registerForsetOnVPString(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPStrsRegistrants.add(r);
    }

    public void unregisterForsetOnVPString(Handler h) {
        mUnsolVPStrsRegistrants.remove(h);
    }

    public void registerForsetOnVPRemoteMedia(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPRemoteMediaRegistrants.add(r);
    }

    public void unregisterForsetOnVPRemoteMedia(Handler h) {
        mUnsolVPRemoteMediaRegistrants.remove(h);
    }

    public void registerForsetOnVPMMRing(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPMMRingRegistrants.add(r);
    }

    public void unregisterForsetOnVPMMRing(Handler h) {
        mUnsolVPMMRingRegistrants.remove(h);
    }

    public void registerForsetOnVPFail(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPFailRegistrants.add(r);
    }

    public void unregisterForsetOnVPFail(Handler h) {
        mUnsolVPFailRegistrants.remove(h);
    }

    public void registerForsetOnVPRecordVideo(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPRecordVideoRegistrants.add(r);
    }

    public void unregisterForsetOnVPRecordVideo(Handler h) {
        mUnsolVPRecordVideoRegistrants.remove(h);
    }

    public void registerForsetOnVPMediaStart(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolVPMediaStartRegistrants.add(r);
    }

    public void unregisterForsetOnVPMediaStart(Handler h) {
        mUnsolVPMediaStartRegistrants.remove(h);
    }

    public void registerForEccNetChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolEccNetChangedRegistrants.add(r);
    }

    public void unregisterForEccNetChanged(Handler h) {
        mUnsolEccNetChangedRegistrants.remove(h);
    }

    public void registerForRauSuccess(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolRauSuccessRegistrants.add(r);
    }

    public void unregisterForRauSuccess(Handler h) {
        mUnsolRauSuccessRegistrants.remove(h);
    }

    public void registerForClearCodeFallback(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mUnsolClearCodeFallbackRegistrants.add(r);
    }

    public void unregisterForClearCodeFallback(Handler h) {
        mUnsolClearCodeFallbackRegistrants.remove(h);
    }

    private void riljLogv(String msg) {
        Rlog.v(TAG, msg
                + (mInstanceId != null ? (" [SUB" + mInstanceId + "]") : ""));
    }

    private void unsljLogvRet(int response, Object ret) {
        riljLogv("[UNSL]< " + responseToString(response) + " " + retToString(response, ret));
    }

    private void unsljLogMore(int response, String more) {
        riljLog("[UNSL]< " + responseToString(response) + " " + more);
    }

    private RadioState getRadioStateFromInt(int stateInt) {
        RadioState state;

        /* RIL_RadioState ril.h */
        switch (stateInt) {
            case 0:
                state = RadioState.RADIO_OFF;
                break;
            case 1:
                state = RadioState.RADIO_UNAVAILABLE;
                break;
            case 10:
                state = RadioState.RADIO_ON;
                break;

            default:
                throw new RuntimeException(
                            "Unrecognized RIL_RadioState: " + stateInt);
        }
        return state;
    }

    private void unsljLog(int response) {
        riljLog("[UNSL]< " + responseToString(response));
    }

    private void riljLog(String msg) {
        Rlog.d(TAG, msg
                + (mInstanceId != null ? (" [SUB" + mInstanceId + "]") : ""));
    }

    private void unsljLogRet(int response, Object ret) {
        riljLog("[UNSL]< " + responseToString(response) + " " + retToString(response, ret));
    }

    static String
            responseToString(int request) {
        switch (request) {

            case RI_UNSOL_RI_CONNECTED:
                return "UNSOL_RI_CONNECTED";
            case RI_UNSOL_RADIOINTERACTOR_EMBMS:
                return "UNSOL_RADIOINTERACTOR_EMBMS";
            case RI_UNSOL_VIDEOPHONE_CODEC:
                return "UNSOL_VIDEOPHONE_CODEC";
            case RI_UNSOL_VIDEOPHONE_DSCI:
                return "UNSOL_VIDEOPHONE_DSCI";
            case RI_UNSOL_VIDEOPHONE_STRING:
                return "UNSOL_VIDEOPHONE_STRING";
            case RI_UNSOL_VIDEOPHONE_REMOTE_MEDIA:
                return "UNSOL_VIDEOPHONE_REMOTE_MEDIA";
            case RI_UNSOL_VIDEOPHONE_MM_RING:
                return "UNSOL_VIDEOPHONE_MM_RING";
            case RI_UNSOL_VIDEOPHONE_RELEASING:
                return "UNSOL_VIDEOPHONE_RELEASING";
            case RI_UNSOL_VIDEOPHONE_RECORD_VIDEO:
                return "UNSOL_VIDEOPHONE_RECORD_VIDEO";
            case RI_UNSOL_VIDEOPHONE_MEDIA_START:
                return "UNSOL_VIDEOPHONE_MEDIA_START";
            case RI_UNSOL_ECC_NETWORKLIST_CHANGED:
                return "UNSOL_ECC_NETWORKLIST_CHANGED";
            case RI_UNSOL_RAU_NOTIFY:
                return "UNSOL_RAU_NOTIFY";
            case RI_UNSOL_CLEAR_CODE_FALLBACK:
                return "UNSOL_CLEAR_CODE_FALLBACK";
            case RI_UNSOL_SIMLOCK_STATUS_CHANGED:
                return "UNSOL_SIMLOCK_STATUS_CHANGED";
            case RI_UNSOL_BAND_INFO:
                return "UNSOL_BAND_INFO";
            default:
                return "<unknown response>";
        }
    }

    private Object
            responseVoid(Parcel p) {
        return null;
    }

    private Object
            responseRaw(Parcel p) {
        int num;
        byte response[];

        response = p.createByteArray();

        return response;
    }

    private RIRequest processSolicited(Parcel p, int type) {
        // testProcessUnsolicited();
        int serial, error;
        boolean found = false;

        serial = p.readInt();
        error = p.readInt();

        RIRequest rr;

        rr = findAndRemoveRequestFromList(serial);

        if (rr == null) {
            Rlog.w(TAG, "Unexpected solicited response! sn: "
                            + serial + " error: " + error);
            return null;
        }

        if (mRilVersion >= 13 && type == RESPONSE_SOLICITED_ACK_EXP) {
            Message msg;
            RIRequest response = RIRequest.obtain(RIL_RESPONSE_ACKNOWLEDGEMENT, null);
            msg = mSender.obtainMessage(EVENT_SEND_ACK, response);
            acquireWakeLock(rr, FOR_ACK_WAKELOCK);
            msg.sendToTarget();
            if (RIJ_LOGD) {
                riljLog("Response received for " + rr.serialString() + " " +
                        requestToString(rr.mRequest) + " Sending ack to ril.cpp");
            }
        }

        Object ret = null;

        if (error == 0 || p.dataAvail() > 0) {
            // either command succeeds or command fails but with data payload
            try {
                switch (rr.mRequest) {

                    case RI_REQUEST_RADIOINTERACTOR:
                        ret = responseInts(p);
                        break;
                    case RI_REQUEST_OEM_HOOK_STRINGS:
                        ret = responseStrings(p);
                        break;
                    case RI_REQUEST_GET_SIM_CAPACITY:
                        ret = responseStrings(p);
                        break;
                    case RI_REQUEST_GET_DEFAULT_NAN:
                        ret = responseString(p);
                        break;
                    case RI_REQUEST_ENABLE_RAU_NOTIFY:
                        ret = responseVoid(p);
                        break;
                    /*SPRD: Bug#525009 Add support for Open Mobile API @{*/
                    case RI_REQUEST_SIM_TRANSMIT_APDU_BASIC:
                        ret = responseICC_IO(p);
                        break;
                    case RI_REQUEST_SIM_TRANSMIT_APDU_CHANNEL:
                        ret = responseICC_IO(p);
                        break;
                    case RI_REQUEST_SIM_OPEN_CHANNEL:
                        ret = responseInts(p);
                        break;
                    case RI_REQUEST_SIM_CLOSE_CHANNEL:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_SIM_OPEN_CHANNEL_WITH_P2:
                        ret = responseInts(p);
                        break;
                    case RI_REQUEST_SIM_GET_ATR:
                        ret = responseString(p);
                        break;
                    /* @} */
                    case RI_REQUEST_GET_HD_VOICE_STATE:
                        ret = responseInts(p);
                        break;
                    case RI_REQUEST_ENABLE_COLP:
                        ret = responseString(p);
                        break;
                    /* SPRD: Bug#542214 Add support for store SMS to Sim card @{ */
                    case RI_REQUEST_STORE_SMS_TO_SIM:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_QUERY_SMS_STORAGE_MODE:
                        ret = responseString(p);
                        break;
                     /* @} */
                    /* add for TV @{*/
                    case RI_REQUEST_VIDEOPHONE_DIAL:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_VIDEOPHONE_CODEC:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_VIDEOPHONE_FALLBACK:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_VIDEOPHONE_STRING:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_VIDEOPHONE_LOCAL_MEDIA:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_VIDEOPHONE_CONTROL_IFRAME:
                        ret = responseVoid(p);
                        break;
                    /* @} */
                    // Explicit Transfer Call REFACTORING
                    case RIL_REQUEST_EXPLICIT_CALL_TRANSFER:
                        ret = responseVoid(p);
                    // Multi Part Call
                    case RI_REQUEST_SWITCH_MULTI_CALL:
                        ret = responseInts(p);
                        break;
                    case RI_REQUEST_DC_TRAFFIC_CLASS:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_ENABLE_LTE:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_ATTACH_DATACONN:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_ABORT_SEARCH_NETWORK:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_DC_FORCE_DETACH:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_SHUTDOWN:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_SET_FACILITY_LOCK_FOR_USER:
                        ret =  responseVoid(p);
                        break;
                    case RI_REQUEST_GET_SIMLOCK_REMAIN_TIMES:
                        ret = responseInts(p);
                        break;
                    case RIL_REQUEST_GET_SIM_STATUS:
                        ret = responseIccCardStatus(p);
                        break;
                    case RI_REQUEST_GET_SIMLOCK_STATUS:
                        ret = responseInts(p);
                        break;
                    case RI_REQUEST_SIM_POWER:
                        ret = responseInts(p);
                        break;
                    case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_UPDATE_REAL_ECCLIST:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_GET_BAND_INFO:
                        ret = responseString(p);
                        break;
                    case RI_REQUEST_SET_BAND_INFO_MODE:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_SET_SINGLE_PDN:
                        ret = responseVoid(p);
                        break;
                    case RI_REQUEST_SET_SPECIAL_RATCAP:
                        ret = responseVoid(p);
                        break;
                    case RIL_REQUEST_SIM_IO:
                        ret = responseICC_IO(p);
                        break;
                    case RIL_REQUEST_GET_IMSI:
                        ret =  responseString(p);
                        break;
                    case RI_REQUEST_QUERY_COLP:
                        ret = responseInts(p);
                        break;
                    case RI_REQUEST_QUERY_COLR:
                        ret = responseInts(p);
                        break;
                    case RI_REQUEST_MMI_ENTER_SIM:
                        ret = responseVoid(p);
                        break;
                    default:
                        throw new RuntimeException("Unrecognized solicited response: "
                                + rr.mRequest);
                }
            } catch (Throwable tr) {
                // Exceptions here usually mean invalid RIL responses

                Rlog.w(TAG, rr.serialString() + "< "
                            + requestToString(rr.mRequest)
                            + " exception, possible invalid RIL response", tr);

                if (rr.mResult != null) {
                    AsyncResult.forMessage(rr.mResult, null, tr);
                    rr.mResult.sendToTarget();
                }
                return rr;
            }
        }
        if (error == 0) {

            Rlog.d(TAG, rr.serialString() + "< " + requestToString(rr.mRequest)
                        + " " + retToString(rr.mRequest, ret));

            if (rr.mResult != null) {
                AsyncResult.forMessage(rr.mResult, ret, null);
                rr.mResult.sendToTarget();
            }
        }

        if (error != 0)
            rr.onError(error, ret);
        return rr;

    }

    private Object
            responseStrings(Parcel p) {
        int num;
        String response[];

        response = p.readStringArray();

        return response;
    }

    private Object
            responseString(Parcel p) {
        String response;

        response = p.readString();

        return response;
}

    private Object
            responseInts(Parcel p) {
        int numInts;
        int response[];

        numInts = p.readInt();

        response = new int[numInts];

        for (int i = 0; i < numInts; i++) {
            response[i] = p.readInt();
        }

        return response;
    }

    private Object
    responseICC_IO(Parcel p) {
        int sw1, sw2;

        sw1 = p.readInt();
        sw2 = p.readInt();

        String s = p.readString();

        riljLog("< iccIO: "
                + " 0x" + Integer.toHexString(sw1)
                + " 0x" + Integer.toHexString(sw2) + " "
                + s);

        return new IccIoResult(sw1, sw2, s);
    }

    static String
            requestToString(int request) {
        switch (request) {

            case RI_REQUEST_RADIOINTERACTOR:
                return "REQUEST_RADIOINTERACTOR";
            case RI_REQUEST_OEM_HOOK_STRINGS:
                return "OEM_HOOK_STRINGS";
            case RI_REQUEST_GET_SIM_CAPACITY:
                return "GET_SIM_CAPACITY";
            case RI_REQUEST_GET_DEFAULT_NAN:
                return "GET_DEFAULT_NETWORK_ACCESS_NAME";
            case RI_REQUEST_ENABLE_RAU_NOTIFY:
                return "ENABLE_RAU_NOTIFY";
            /*SPRD: Bug#525009 Add support for Open Mobile API @{*/
            case RI_REQUEST_SIM_OPEN_CHANNEL:
                return "REQUEST_SIM_OPEN_CHANNEL";
            case RI_REQUEST_SIM_OPEN_CHANNEL_WITH_P2:
                return "REQUEST_SIM_OPEN_CHANNEL_WITH_P2";
            case RI_REQUEST_SIM_CLOSE_CHANNEL:
                return "REQUEST_SIM_CLOSE_CHANNEL";
            case RI_REQUEST_SIM_TRANSMIT_APDU_BASIC:
                return "REQUEST_SIM_TRANSMIT_APDU_BASIC";
            case RI_REQUEST_SIM_TRANSMIT_APDU_CHANNEL:
                return "REQUEST_SIM_TRANSMIT_APDU_CHANNEL";
            case RI_REQUEST_SIM_GET_ATR:
                return "REQUEST_SIM_GET_ATR";
            /* @} */
            case RI_REQUEST_GET_HD_VOICE_STATE:
                return "REQUEST_GET_HD_VOICE_STATE";
            case RI_REQUEST_ENABLE_COLP:
                return "REQUEST_ENABLE_COLP";
            /*SPRD: Bug#542214 Add support for store SMS to Sim card @{*/
            case RI_REQUEST_STORE_SMS_TO_SIM:
                return "RI_REQUEST_STORE_SMS_TO_SIM";
            case RI_REQUEST_QUERY_SMS_STORAGE_MODE:
                return "REQUEST_QUERY_SMS_STORAGE_MODE";
            /* @} */
            // Explicit Transfer Call REFACTORING
            case RIL_REQUEST_EXPLICIT_CALL_TRANSFER:
                return "REQUEST_EXPLICIT_CALL_TRANSFER";
            case RI_REQUEST_SWITCH_MULTI_CALL:
                return "REQUEST_SWITCH_MULTI_CALL";
            /* add for TV @{*/
            case RI_REQUEST_VIDEOPHONE_DIAL:
                return "VIDEOPHONE_DIAL";
            case RI_REQUEST_VIDEOPHONE_CODEC:
                return "VIDEOPHONE_CODEC";
            case RI_REQUEST_VIDEOPHONE_FALLBACK:
                return "VIDEOPHONE_FALLBACK";
            case RI_REQUEST_VIDEOPHONE_STRING:
                return "VIDEOPHONE_STRING";
            case RI_REQUEST_VIDEOPHONE_LOCAL_MEDIA:
                return "VIDEOPHONE_LOCAL_MEDIA";
            case RI_REQUEST_VIDEOPHONE_CONTROL_IFRAME:
                return "VIDEOPHONE_CONTROL_IFRAME";
            /* @} */
            case RI_REQUEST_DC_TRAFFIC_CLASS:
                return "REQUEST_DC_TRAFFIC_CLASS";
            case RI_REQUEST_ENABLE_LTE:
                return "REQUEST_ENABLE_LTE";
            case RI_REQUEST_ATTACH_DATACONN:
                return "REQUEST_ATTACH_DATACONN";
            case RI_REQUEST_ABORT_SEARCH_NETWORK:
                return "REQUEST_ABORT_SEARCH_NETWORK";
            case RI_REQUEST_DC_FORCE_DETACH:
                return "REQUEST_DC_FORCE_DETACH";
            case RI_REQUEST_SHUTDOWN:
                return "REQUEST_SHUTDOWN";
            case RI_RESPONSE_ACKNOWLEDGEMENT:
                return "RIL_RESPONSE_ACKNOWLEDGEMENT";
            case RI_REQUEST_SET_FACILITY_LOCK_FOR_USER:
                return "SET_FACILITY_LOCK_BY_USER";
            case RI_REQUEST_GET_SIMLOCK_REMAIN_TIMES:
                return "GET_SIMLOCK_REMAIN_TIMES";
            case RIL_REQUEST_GET_SIM_STATUS:
                return "GET_SIM_STATUS";
            case RI_REQUEST_GET_SIMLOCK_STATUS:
                return "GET_SIMLOCK_STATUS";
            case RI_REQUEST_SIM_POWER:
                return "SIM_POWER";
            case RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE:
                return "SET_PREFERRED_NETWORK_TYPE";
            case RI_REQUEST_UPDATE_REAL_ECCLIST:
                return"UPDATE_REAL_ECCLIST";
            case RI_REQUEST_GET_BAND_INFO:
                return "REQUEST_GET_BAND_INFO";
            case RI_REQUEST_SET_BAND_INFO_MODE:
                return "REQUEST_SET_BAND_INFO_MODE";
            case RI_REQUEST_SET_SINGLE_PDN:
                return "RI_REQUEST_SET_SINGLE_PDN";
            case RI_REQUEST_SET_SPECIAL_RATCAP:
                return "REQUEST_SET_NETWORK_RAT_PREFERRED";
            case RIL_REQUEST_SIM_IO:
                return "REQUEST_SIM_IO";
            case RIL_REQUEST_GET_IMSI:
                return "REQUEST_GET_IMSI";
            case RI_REQUEST_QUERY_COLP:
                return "REQUEST_QUERY_COLP";
            case RI_REQUEST_QUERY_COLR:
                return "REQUEST_QUERY_COLR";
            case RI_REQUEST_MMI_ENTER_SIM:
                return "REQUEST_MMI_ENTER_SIM";
            default:
                return "<unknown request>";
        }
    }

    static String
            retToString(int req, Object ret) {
        if (ret == null)
            return "";
        StringBuilder sb;
        String s;
        int length;
        if (ret instanceof int[]) {
            int[] intArray = (int[]) ret;
            length = intArray.length;
            sb = new StringBuilder("{");
            if (length > 0) {
                int i = 0;
                sb.append(intArray[i++]);
                while (i < length) {
                    sb.append(", ").append(intArray[i++]);
                }
            }
            sb.append("}");
            s = sb.toString();
        } else if (ret instanceof String[]) {
            String[] strings = (String[]) ret;
            length = strings.length;
            sb = new StringBuilder("{");
            if (length > 0) {
                int i = 0;
                sb.append(strings[i++]);
                while (i < length) {
                    sb.append(", ").append(strings[i++]);
                }
            }
            sb.append("}");
            s = sb.toString();
        } else {
            s = ret.toString();
        }
        return s;
    }

    private final class DSCIInfo {
        int id;
        int idr;
        int stat;
        int type;
        int mpty;
        String number;
        int num_type;
        int bs_type;
        int cause;
        int location; // if  cause = 57 and location <= 2, it mean current sim hasn't start vt service
    }

    protected Object
    responseDSCI(Parcel p) {
        DSCIInfo info = new DSCIInfo();

        info.id = p.readInt();//id
        info.idr = p.readInt();//idr
        info.stat = p.readInt();//stat
        if (info.stat == 6) {
            info.type = p.readInt();//type
            info.mpty = p.readInt();//mpty
            info.number = p.readString();//number
            info.num_type = p.readInt();//num_type
            info.bs_type = p.readInt();//bs_type
            if (info.type == 1) {
                info.cause = p.readInt();
                info.location = p.readInt();//my side or other side
            }
        }
        riljLog("responseDSCI(), number: " + info.number + ", status: " + info.stat + ", type: " + info.type + ", cause: " + info.cause + ", location: " + info.location);

        return info;
    }

    class RILSender extends Handler implements Runnable {
        public RILSender(Looper looper) {
            super(looper);
        }

        // Only allocated once
        byte[] dataLength = new byte[4];

        // ***** Runnable implementation
        @Override
        public void
                run() {
            // setup if needed
        }

        // ***** Handler implementation
        @Override
        public void
                handleMessage(Message msg) {
            RIRequest rr = (RIRequest) (msg.obj);
            RIRequest req = null;

            switch (msg.what) {
                case EVENT_SEND:
                case EVENT_SEND_ACK:
                    try {
                        LocalSocket s;

                        s = mSocket;

                        if (s == null) {
                            rr.onError(RADIO_NOT_AVAILABLE, null);
                            decrementWakeLock(rr);
                            rr.release();
                            return;
                        }

                        // Acks should not be stored in list before sending
                        if (msg.what != EVENT_SEND_ACK) {
                            synchronized (mRequestList) {
                                mRequestList.append(rr.mSerial, rr);
                            }
                        }

                        byte[] data;

                        data = rr.mParcel.marshall();
                        rr.mParcel.recycle();
                        rr.mParcel = null;

                        if (data.length > RIL_MAX_COMMAND_BYTES) {
                            throw new RuntimeException(
                                    "Parcel larger than max bytes allowed! "
                                                          + data.length);
                        }

                        // parcel length in big endian
                        dataLength[0] = dataLength[1] = 0;
                        dataLength[2] = (byte) ((data.length >> 8) & 0xff);
                        dataLength[3] = (byte) ((data.length) & 0xff);

                        s.getOutputStream().write(dataLength);
                        s.getOutputStream().write(data);
                        if (msg.what == EVENT_SEND_ACK) {
                            rr.release();
                            return;
                        }
                    } catch (IOException ex) {
                        UtilLog.loge(TAG, "IOException", ex);
                        req = findAndRemoveRequestFromList(rr.mSerial);
                        // make sure this request has not already been handled,
                        // eg, if RILReceiver cleared the list.
                        if (req != null) {
                            rr.onError(RADIO_NOT_AVAILABLE, null);
                            decrementWakeLock(rr);
                            rr.release();
                            return;
                        }
                    } catch (RuntimeException exc) {
                        UtilLog.loge(TAG, "Uncaught exception ", exc);
                        req = findAndRemoveRequestFromList(rr.mSerial);
                        // make sure this request has not already been handled,
                        // eg, if RILReceiver cleared the list.
                        if (req != null) {
                            rr.onError(GENERIC_FAILURE, null);
                            decrementWakeLock(rr);
                            rr.release();
                            return;
                        }
                    }

                    break;

                case EVENT_WAKE_LOCK_TIMEOUT:
                    // Haven't heard back from the last request. Assume we're
                    // not getting a response and release the wake lock.

                    // The timer of WAKE_LOCK_TIMEOUT is reset with each
                    // new send request. So when WAKE_LOCK_TIMEOUT occurs
                    // all requests in mRequestList already waited at
                    // least DEFAULT_WAKE_LOCK_TIMEOUT but no response.
                    //
                    // Note: Keep mRequestList so that delayed response
                    // can still be handled when response finally comes.

                    synchronized (mRequestList) {
                        if (msg.arg1 == mWlSequenceNum && clearWakeLock(FOR_WAKELOCK)) {

                            int count = mRequestList.size();
                            Rlog.d(TAG, "WAKE_LOCK_TIMEOUT " +
                                        " mRequestList=" + count);
                            for (int i = 0; i < count; i++) {
                                rr = mRequestList.valueAt(i);
                                Rlog.d(TAG, i + ": [" + rr.mSerial + "] "
                                            + requestToString(rr.mRequest));
                            }

                        }
                    }
                    break;
                case EVENT_ACK_WAKE_LOCK_TIMEOUT:
                    if (msg.arg1 == mAckWlSequenceNum && clearWakeLock(FOR_ACK_WAKELOCK)) {
                        if (RIJ_LOGD) {
                            Rlog.d(TAG, "ACK_WAKE_LOCK_TIMEOUT");
                        }
                    }
                    break;

                case EVENT_BLOCKING_RESPONSE_TIMEOUT:
                    int serial = msg.arg1;
                    rr = findAndRemoveRequestFromList(serial);
                    // If the request has already been processed, do nothing
                    if(rr == null) {
                        break;
                    }

                    //build a response if expected
                    if (rr.mResult != null) {
                        Object timeoutResponse = getResponseForTimedOutRILRequest(rr);
                        AsyncResult.forMessage( rr.mResult, timeoutResponse, null);
                        rr.mResult.sendToTarget();
                    }

                    decrementWakeLock(rr);
                    rr.release();
                    break;
            }
        }
    }

    /**
     * In order to prevent calls to Telephony from waiting indefinitely
     * low-latency blocking calls will eventually time out. In the event of
     * a timeout, this function generates a response that is returned to the
     * higher layers to unblock the call. This is in lieu of a meaningful
     * response.
     * @param rr The RI Request that has timed out.
     * @return A default object, such as the one generated by a normal response
     * that is returned to the higher layers.
     **/
    private static Object getResponseForTimedOutRILRequest(RIRequest rr) {
        if (rr == null ) return null;

        Object timeoutResponse = null;
        // TODO
        return timeoutResponse;
    }

    private RIRequest findAndRemoveRequestFromList(int serial) {
        RIRequest rr = null;
        synchronized (mRequestList) {
            rr = mRequestList.get(serial);
            if (rr != null) {
                mRequestList.remove(serial);
            }
        }

        return rr;
    }

    public void invokeRequestRadiointeractor(int type, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_RADIOINTERACTOR, response);
        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(type);
        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));
        send(rr);
    }

    public void getSimCapacity(Message response) {

        RIRequest rr = RIRequest.obtain(RI_REQUEST_GET_SIM_CAPACITY, response);

        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);

    }

    public void getDefaultNetworkAccessName(Message response) {

        RIRequest rr = RIRequest.obtain(RI_REQUEST_GET_DEFAULT_NAN, response);

        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);

    }

    public void invokeOemRILRequestStrings(String[] strings, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_OEM_HOOK_STRINGS, response);

        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeStringArray(strings);

        send(rr);
    }

    public void enableRauNotify(Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_ENABLE_RAU_NOTIFY,response);

        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void queryHdVoiceState(Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_GET_HD_VOICE_STATE, response);

        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void setCallingNumberShownEnabled(boolean enabled, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_ENABLE_COLP, response);

        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(enabled ? 1 : 0);

        send(rr);
    }

    /*SPRD: Bug#542214 Add support for store SMS to Sim card @{*/
    public void storeSmsToSim(boolean enabled,Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_STORE_SMS_TO_SIM, response);

        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(enabled ? 1 : 0);

        send(rr);
    }

    public void querySmsStorageMode(Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_QUERY_SMS_STORAGE_MODE, response);

        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }
    /* @} */

    public void dialVP (String address, String sub_address, int clirMode, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_VIDEOPHONE_DIAL, response);

        rr.mParcel.writeString(address);
        rr.mParcel.writeString(sub_address);
        rr.mParcel.writeInt(clirMode);

        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void codecVP(int type, Bundle param, Message response){
        RIRequest rr = RIRequest.obtain(RI_REQUEST_VIDEOPHONE_CODEC, response);

        rr.mParcel.writeInt(type);
        rr.mParcel.writeBundle(param);

        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest)
                + " " + type + " " + param);

        send(rr);
    }

    public void fallBackVP(Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_VIDEOPHONE_FALLBACK, response);

        if(RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void sendVPString(String str, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_VIDEOPHONE_STRING, response);

        rr.mParcel.writeString(str);

        if (RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest)
                + " " + str);

        send(rr);
    }

    public void controlVPLocalMedia(int datatype, int sw, boolean bReplaceImg, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_VIDEOPHONE_LOCAL_MEDIA, response);

        rr.mParcel.writeInt(3);

        rr.mParcel.writeInt(datatype);
        rr.mParcel.writeInt(sw);

        rr.mParcel.writeInt(bReplaceImg?1:0);

        if (RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest)
                + " " + datatype + " " + sw + " " + bReplaceImg);

        send(rr);
    }

    public void controlIFrame(boolean isIFrame, boolean needIFrame, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_VIDEOPHONE_CONTROL_IFRAME, response);

        rr.mParcel.writeInt(2);
        rr.mParcel.writeInt(isIFrame ? 1 : 0);
        rr.mParcel.writeInt(needIFrame ? 1 : 0);

        if (RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void requestDCTrafficClass(int type, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_DC_TRAFFIC_CLASS, response);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(type);

        send(rr);
    }
    /*SPRD: bug618350 add single pdp allowed by plmns feature@{*/
    public void requestSetSinglePDNByNetwork(boolean isSinglePDN, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_SET_SINGLE_PDN, response);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(isSinglePDN ? 1 : 0);

        send(rr);
    }
    /* @} */
    public void setLteEnabled(boolean enabled, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_ENABLE_LTE, response);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(enabled ? 1 : 0);

        send(rr);
    }

    public void attachDataConn(boolean enabled, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_ATTACH_DATACONN, response);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(enabled ? 1 : 0);

        send(rr);
    }

    public void abortSearchNetwork(Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_ABORT_SEARCH_NETWORK, response);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void forceDetachDataConn(Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_DC_FORCE_DETACH, response);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void requestShutdown(Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_SHUTDOWN, response);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void setPreferredNetworkType(int networkType , Message response) {
        RIRequest rr = RIRequest.obtain(RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE, response);

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(networkType);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest) + " : " + networkType);

        send(rr);
    }

    public void updateRealEccList(String realEccList, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_UPDATE_REAL_ECCLIST, response);

        rr.mParcel.writeString(realEccList);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest) + " : " + realEccList);

        send(rr);
    }

    public void getBandInfo(Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_GET_BAND_INFO, response);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void setBandInfoMode(int type,Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_SET_BAND_INFO_MODE, response);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(type);

        send(rr);
    }

    public void setNetworkSpecialRATCap(int type,Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_SET_SPECIAL_RATCAP, response);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(type);

        send(rr);
    }

    public void getIccID(Message response ,int command,int size) {
        RIRequest rr = RIRequest.obtain(RIL_REQUEST_SIM_IO, response);

        rr.mParcel.writeInt(command);
        rr.mParcel.writeInt(0x2fe2);
        rr.mParcel.writeString("3F00");
        rr.mParcel.writeInt(0);
        rr.mParcel.writeInt(0);
        rr.mParcel.writeInt(size);
        rr.mParcel.writeString(null);
        rr.mParcel.writeString(null);
        rr.mParcel.writeString(null);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void queryColp(Message response){
        RIRequest rr = RIRequest.obtain(RI_REQUEST_QUERY_COLP, response);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void getHomePLMN(Message response) {
        RIRequest rr = RIRequest.obtain(RIL_REQUEST_GET_IMSI, response);

        rr.mParcel.writeInt(1);
        rr.mParcel.writeString(null);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void queryColr(Message response){
        RIRequest rr = RIRequest.obtain(RI_REQUEST_QUERY_COLR, response);

        if (RIJ_LOGD)
            riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void mmiEnterSim(String data,Message response){
        RIRequest rr = RIRequest.obtain(RI_REQUEST_MMI_ENTER_SIM, response);

        rr.mParcel.writeString(data);

        if (RIJ_LOGD) riljLog(rr.serialString() + "> " + requestToString(rr.mRequest)
                + " " + data);

        send(rr);
    }

    private void
            send(RIRequest rr) {
        Message msg;

        if (mSocket == null) {
            rr.onError(RADIO_NOT_AVAILABLE, null);
            rr.release();
            return;
        }

        msg = mSender.obtainMessage(EVENT_SEND, rr);
        acquireWakeLock(rr, FOR_WAKELOCK);
        msg.sendToTarget();
    }

    private void
    acquireWakeLock(RIRequest rr, int wakeLockType) {
        synchronized(rr) {
            if(rr.mWakeLockType != INVALID_WAKELOCK) {
                Rlog.d(TAG, "Failed to aquire wakelock for " + rr.serialString());
                return;
            }

            switch(wakeLockType) {
                case FOR_WAKELOCK:
                    synchronized (mWakeLock) {
                        mWakeLock.acquire();
                        mWakeLockCount++;
                        mWlSequenceNum++;

                        Message msg = mSender.obtainMessage(EVENT_WAKE_LOCK_TIMEOUT);
                        msg.arg1 = mWlSequenceNum;
                        mSender.sendMessageDelayed(msg, mWakeLockTimeout);
                    }
                    break;
                case FOR_ACK_WAKELOCK:
                    synchronized (mAckWakeLock) {
                        mAckWakeLock.acquire();
                        mAckWlSequenceNum++;

                        Message msg = mSender.obtainMessage(EVENT_ACK_WAKE_LOCK_TIMEOUT);
                        msg.arg1 = mAckWlSequenceNum;
                        mSender.sendMessageDelayed(msg, mAckWakeLockTimeout);
                    }
                    break;
                default: //WTF
                    Rlog.w(TAG, "Acquiring Invalid Wakelock type " + wakeLockType);
                    return;
            }
            rr.mWakeLockType = wakeLockType;
        }
    }

    private void
    decrementWakeLock(RIRequest rr) {
        synchronized(rr) {
            switch(rr.mWakeLockType) {
                case FOR_WAKELOCK:
                    synchronized (mWakeLock) {
                        if (mWakeLockCount > 1) {
                            mWakeLockCount--;
                        } else {
                            mWakeLockCount = 0;
                            mWakeLock.release();
                        }
                    }
                    break;
                case FOR_ACK_WAKELOCK:
                    //We do not decrement the ACK wakelock
                    break;
                case INVALID_WAKELOCK:
                    break;
                default:
                    Rlog.w(TAG, "Decrementing Invalid Wakelock type " + rr.mWakeLockType);
            }
            rr.mWakeLockType = INVALID_WAKELOCK;
        }
    }

    private boolean
    clearWakeLock(int wakeLockType) {
        if (wakeLockType == FOR_WAKELOCK) {
            synchronized (mWakeLock) {
                if (mWakeLockCount == 0 && mWakeLock.isHeld() == false) return false;
                Rlog.d(TAG, "NOTE: mWakeLockCount is " + mWakeLockCount
                        + "at time of clearing");
                mWakeLockCount = 0;
                mWakeLock.release();
                return true;
            }
        } else {
            synchronized (mAckWakeLock) {
                if (mAckWakeLock.isHeld() == false) return false;
                mAckWakeLock.release();
                return true;
            }
        }
    }

    public int getPhoneId() {
        return mInstanceId;
    }

    /*SPRD: Bug#525009 Add support for Open Mobile API @{*/
    public void iccOpenLogicalChannel(String AID, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_SIM_OPEN_CHANNEL, response);
        rr.mParcel.writeString(AID);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void iccOpenLogicalChannel(String AID, byte p2, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_SIM_OPEN_CHANNEL_WITH_P2, response);
        rr.mParcel.writeInt(2);
        rr.mParcel.writeString(AID);
        rr.mParcel.writeString(Integer.toHexString(0x100 + (p2 & 0xff)).substring(1));

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void iccCloseLogicalChannel(int channel, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_SIM_CLOSE_CHANNEL, response);
        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(channel);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void iccTransmitApduLogicalChannel(int channel, int cla, int instruction,
            int p1, int p2, int p3, String data, Message response) {
        if (channel <= 0) {
            throw new RuntimeException(
                "Invalid channel in iccTransmitApduLogicalChannel: " + channel);
        }

        iccTransmitApduHelper(RI_REQUEST_SIM_TRANSMIT_APDU_CHANNEL, channel, cla,
                instruction, p1, p2, p3, data, response);
    }

    public void iccTransmitApduBasicChannel(int cla, int instruction, int p1, int p2,
            int p3, String data, Message response) {
        iccTransmitApduHelper(RI_REQUEST_SIM_TRANSMIT_APDU_BASIC, 0, cla, instruction,
                p1, p2, p3, data, response);
    }

    /*
     * Helper function for the iccTransmitApdu* commands above.
     */
    private void iccTransmitApduHelper(int command, int channel, int cla,
            int instruction, int p1, int p2, int p3, String data, Message response) {
        RIRequest rr = RIRequest.obtain(command, response);
        rr.mParcel.writeInt(channel);
        rr.mParcel.writeInt(cla);
        rr.mParcel.writeInt(instruction);
        rr.mParcel.writeInt(p1);
        rr.mParcel.writeInt(p2);
        rr.mParcel.writeInt(p3);
        rr.mParcel.writeString(data);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

   public void iccGetAtr(Message result) {
       RIRequest rr = RIRequest.obtain(RI_REQUEST_SIM_GET_ATR, result);

       riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

       send(rr);
   }
   /* @} */

   /**
    * Explicit Transfer Call REFACTORING
    * @param result
    */
   public void
   explicitCallTransfer (Message result) {
       RIRequest rr
               = RIRequest.obtain(RIL_REQUEST_EXPLICIT_CALL_TRANSFER, result);

       riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

       send(rr);
   }

   public void switchMultiCalls(int mode, Message response) {
       RIRequest rr = RIRequest.obtain(RI_REQUEST_SWITCH_MULTI_CALL, response);

       rr.mParcel.writeInt(1);
       rr.mParcel.writeInt(mode);
       riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

       send(rr);
   }

    public void setFacilityLockByUser(String facility, boolean lockState, Message response) {
        String lockString;
        RIRequest rr
                = RIRequest.obtain(RI_REQUEST_SET_FACILITY_LOCK_FOR_USER, response);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest)
                + " [" + facility + " " + lockState + "]");

        // count strings
        rr.mParcel.writeInt(2);

        rr.mParcel.writeString(facility);
        lockString = (lockState)?"1":"0";
        rr.mParcel.writeString(lockString);

        send(rr);
    }

    public void getSimLockRemainTimes(int type, Message response) {
        RIRequest rr
            = RIRequest.obtain(RI_REQUEST_GET_SIMLOCK_REMAIN_TIMES, response);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest)
            + "[" + type + "]");

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(type);

        send(rr);
    }

    public void getSimLockStatus(int type, Message result) {
        RIRequest rr
            = RIRequest.obtain(RI_REQUEST_GET_SIMLOCK_STATUS, result);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest)
            + "[" + type + "]");

        rr.mParcel.writeInt(2);
        rr.mParcel.writeInt(type);
        rr.mParcel.writeInt(1);

        send(rr);
    }

    public void
    getIccCardStatus(Message result) {
        //Note: This RIL request has not been renamed to ICC,
        //       but this request is also valid for SIM and RUIM
        RIRequest rr = RIRequest.obtain(RIL_REQUEST_GET_SIM_STATUS, result);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }


    public void registerForSimlockStatusChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant (h, what, obj);
        mSimlockStatusChangedRegistrants.add(r);
    }

    public void unregisterForSimlockStatusChanged(Handler h) {
        mSimlockStatusChangedRegistrants.remove(h);
    }

    public void registerForBandInfo(Handler h, int what, Object obj) {

        Registrant r = new Registrant(h, what, obj);
        mUnsolBandInfoRegistrants.add(r);
    }

    public void unregisterForBandInfo(Handler h) {
        mUnsolBandInfoRegistrants.remove(h);
    }

    private Object
    responseIccCardStatus(Parcel p) {

        riljLog("responseIccCardStatus");
        IccCardApplicationStatusEx appStatus;

        IccCardStatusEx cardStatus = new IccCardStatusEx();
        cardStatus.setCardState(p.readInt());
        cardStatus.setUniversalPinState(p.readInt());
        cardStatus.mGsmUmtsSubscriptionAppIndex = p.readInt();
        cardStatus.mCdmaSubscriptionAppIndex = p.readInt();
        cardStatus.mImsSubscriptionAppIndex = p.readInt();
        int numApplications = p.readInt();

        // limit to maximum allowed applications
        if (numApplications > IccCardStatusEx.CARD_MAX_APPS) {
            numApplications = IccCardStatusEx.CARD_MAX_APPS;
        }
        riljLog("responseIccCardStatus numApplications = " + numApplications);
        cardStatus.mApplications = new IccCardApplicationStatusEx[numApplications];
        for (int i = 0 ; i < numApplications ; i++) {
            appStatus = new IccCardApplicationStatusEx();
            appStatus.app_type       = appStatus.AppTypeFromRILInt(p.readInt());
            appStatus.app_state      = appStatus.AppStateFromRILInt(p.readInt());
            appStatus.perso_substate = appStatus.PersoSubstateFromRILInt(p.readInt());
            appStatus.aid            = p.readString();
            appStatus.app_label      = p.readString();
            appStatus.pin1_replaced  = p.readInt();
            appStatus.pin1           = appStatus.PinStateFromRILInt(p.readInt());
            appStatus.pin2           = appStatus.PinStateFromRILInt(p.readInt());
            cardStatus.mApplications[i] = appStatus;
        }
        return cardStatus;
    }

   public void setSimPower(boolean enabled, Message response) {
       RIRequest rr = RIRequest.obtain(RI_REQUEST_SIM_POWER, response);

       riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

       rr.mParcel.writeInt(1);
       rr.mParcel.writeInt(enabled ? 1 : 0);

       send(rr);
   }
}
