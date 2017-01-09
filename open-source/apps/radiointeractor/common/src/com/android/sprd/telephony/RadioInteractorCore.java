
package com.android.sprd.telephony;

import static com.android.internal.telephony.RILConstants.GENERIC_FAILURE;
import static com.android.internal.telephony.RILConstants.RADIO_NOT_AVAILABLE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_RADIOINTERACTOR;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RADIOINTERACTOR_EMBMS;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_OEM_HOOK_STRINGS;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_SIM_CAPACITY;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ENABLE_RAU_NOTIFY;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_TRANSMIT_APDU_BASIC;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_TRANSMIT_APDU_CHANNEL;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_OPEN_CHANNEL;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_CLOSE_CHANNEL;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_GET_ATR;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_SIM_OPEN_CHANNEL_WITH_P2;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_GET_HD_VOICE_STATE;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_ENABLE_COLP;
import static com.android.sprd.telephony.RIConstants.RI_UNSOL_RI_CONNECTED;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_STORE_SMS_TO_SIM;
import static com.android.sprd.telephony.RIConstants.RI_REQUEST_QUERY_SMS_STORAGE_MODE;

import java.io.IOException;
import java.io.InputStream;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.AsyncResult;
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

    static final int RIL_MAX_COMMAND_BYTES = (8 * 1024);
    private Integer mInstanceId;
    static final String[] SOCKET_NAME_RIL = {
            "oem_socket1", "oem_socket2", "oem_socket3"
    };
    static final int SOCKET_OPEN_RETRY_MILLIS = 4 * 1000;
    LocalSocket mSocket;
    static final int RESPONSE_SOLICITED = 0;
    static final int RESPONSE_UNSOLICITED = 1;
    WakeLock mWakeLock;
    int mWakeLockCount;
    static final int EVENT_SEND = 1;
    static final int EVENT_WAKE_LOCK_TIMEOUT = 2;
    RILSender mSender;
    SparseArray<RIRequest> mRequestList = new SparseArray<RIRequest>();
    Thread mReceiverThread;
    RILReceiver mReceiver;
    final int mWakeLockTimeout;
    private static final int DEFAULT_WAKE_LOCK_TIMEOUT = 60000;
    HandlerThread mSenderThread;
    boolean mIsRiConnected;

    protected RegistrantList mUnsolRadiointeractorRegistrants = new RegistrantList();
    protected RegistrantList mUnsolRIConnectedRegistrants = new RegistrantList();

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
        mWakeLockTimeout = SystemProperties.getInt(TelephonyProperties.PROPERTY_WAKE_LOCK_TIMEOUT,
                DEFAULT_WAKE_LOCK_TIMEOUT);
        mSenderThread = new HandlerThread("RILSender" + mInstanceId);
        mSenderThread.start();
        Looper looper = mSenderThread.getLooper();
        mSender = new RILSender(looper);
        PowerManager pm = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "RadioInteractorCore");
        //SPRD: Modify for Bug560149
        mWakeLock.setReferenceCounted(false);
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
                rr.release();
                decrementWakeLock();
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
            processResponse(Parcel p) {
        int type;

        type = p.readInt();
        if (type == RESPONSE_UNSOLICITED) {
            processUnsolicited(p);
        } else if (type == RESPONSE_SOLICITED) {
            RIRequest rr = processSolicited(p);
            if (rr != null) {
                rr.release();
                decrementWakeLock();
            }
        }
    }

    private void processUnsolicited(Parcel p) {
        int response;
        Object ret;

        response = p.readInt();

        try {
            switch (response) {

                case RI_UNSOL_RI_CONNECTED:
                    ret = responseVoid(p);
                    break;
                case RI_UNSOL_RADIOINTERACTOR_EMBMS:
                    ret = responseVoid(p);
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
                unsljLog(response);
                if (mUnsolRIConnectedRegistrants != null) {
                    mIsRiConnected = true;
                    mUnsolRIConnectedRegistrants.notifyRegistrants(new AsyncResult(null, ret,
                            null));
                }
                break;
            case RI_UNSOL_RADIOINTERACTOR_EMBMS:
                unsljLog(response);
                if (mUnsolRadiointeractorRegistrants != null) {
                    mUnsolRadiointeractorRegistrants.notifyRegistrants(new AsyncResult(null, ret,
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
        if(mIsRiConnected){
            mUnsolRIConnectedRegistrants.notifyRegistrants(new AsyncResult(null, null,
                    null));
        }
    }

    public void unregisterForUnsolRiConnected(Handler h) {
        mUnsolRIConnectedRegistrants.remove(h);
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

    static String
            responseToString(int request) {
        switch (request) {

            case RI_UNSOL_RI_CONNECTED:
                return "UNSOL_RI_CONNECTED";
            case RI_UNSOL_RADIOINTERACTOR_EMBMS:
                return "UNSOL_RADIOINTERACTOR_EMBMS";
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

    private RIRequest processSolicited(Parcel p) {
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

    private void
            decrementWakeLock() {
        synchronized (mWakeLock) {
            if (mWakeLockCount > 1) {
                mWakeLockCount--;
            } else {
                mWakeLockCount = 0;
                mWakeLock.release();
                mSender.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
            }
        }
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
                    try {
                        LocalSocket s;

                        s = mSocket;

                        if (s == null) {
                            rr.onError(RADIO_NOT_AVAILABLE, null);
                            rr.release();
                            decrementWakeLock();
                            return;
                        }

                        synchronized (mRequestList) {
                            mRequestList.append(rr.mSerial, rr);
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
                    } catch (IOException ex) {
                        UtilLog.loge(TAG, "IOException", ex);
                        req = findAndRemoveRequestFromList(rr.mSerial);
                        // make sure this request has not already been handled,
                        // eg, if RILReceiver cleared the list.
                        if (req != null) {
                            rr.onError(RADIO_NOT_AVAILABLE, null);
                            rr.release();
                            decrementWakeLock();
                        }
                    } catch (RuntimeException exc) {
                        UtilLog.loge(TAG, "Uncaught exception ", exc);
                        req = findAndRemoveRequestFromList(rr.mSerial);
                        // make sure this request has not already been handled,
                        // eg, if RILReceiver cleared the list.
                        if (req != null) {
                            rr.onError(GENERIC_FAILURE, null);
                            rr.release();
                            decrementWakeLock();
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
                        if (clearWakeLock()) {

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
            }
        }
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

    private boolean
            clearWakeLock() {
        synchronized (mWakeLock) {
            if (mWakeLockCount == 0 && mWakeLock.isHeld() == false)
                return false;
            Rlog.d(TAG, "NOTE: mWakeLockCount is " + mWakeLockCount + "at time of clearing");
            mWakeLockCount = 0;
            mWakeLock.release();
            mSender.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
            return true;
        }
    }

    public void invokeRequestRadiointeractor(int type, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_RADIOINTERACTOR, response);
        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(type);
        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));
        send(rr);
    }

    public void getSimCapacity(Message response) {

        RIRequest rr = RIRequest.obtain(RI_REQUEST_GET_SIM_CAPACITY, response);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);

    }

    public void invokeOemRILRequestStrings(String[] strings, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_OEM_HOOK_STRINGS, response);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeStringArray(strings);

        send(rr);
    }

    public void enableRauNotify(Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_ENABLE_RAU_NOTIFY,response);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void queryHdVoiceState(Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_GET_HD_VOICE_STATE, response);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }

    public void setCallingNumberShownEnabled(boolean enabled, Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_ENABLE_COLP, response);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(enabled ? 1 : 0);

        send(rr);
    }

    /*SPRD: Bug#542214 Add support for store SMS to Sim card @{*/
    public void storeSmsToSim(boolean enabled,Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_STORE_SMS_TO_SIM, response);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        rr.mParcel.writeInt(1);
        rr.mParcel.writeInt(enabled ? 1 : 0);

        send(rr);
    }

    public void querySmsStorageMode(Message response) {
        RIRequest rr = RIRequest.obtain(RI_REQUEST_QUERY_SMS_STORAGE_MODE, response);

        riljLog(rr.serialString() + "> " + requestToString(rr.mRequest));

        send(rr);
    }
    /* @} */

    private void
            send(RIRequest rr) {
        Message msg;

        if (mSocket == null) {
            rr.onError(RADIO_NOT_AVAILABLE, null);
            rr.release();
            return;
        }

        msg = mSender.obtainMessage(EVENT_SEND, rr);
        acquireWakeLock();
        msg.sendToTarget();
    }

    private void
            acquireWakeLock() {
        synchronized (mWakeLock) {
            mWakeLock.acquire();
            mWakeLockCount++;

            mSender.removeMessages(EVENT_WAKE_LOCK_TIMEOUT);
            Message msg = mSender.obtainMessage(EVENT_WAKE_LOCK_TIMEOUT);
            mSender.sendMessageDelayed(msg, mWakeLockTimeout);
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
}
