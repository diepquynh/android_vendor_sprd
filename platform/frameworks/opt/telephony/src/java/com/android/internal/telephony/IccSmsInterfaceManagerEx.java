package com.android.internal.telephony;

import android.app.AppOpsManager;
import android.os.AsyncResult;
import android.os.Binder;
import android.os.Handler;
import android.os.Message;
import android.telephony.Rlog;
import android.util.Log;

import com.android.internal.telephony.uicc.IccUtils;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * Created by lizhong.wu on 2016/9/20.
 */
public class IccSmsInterfaceManagerEx extends IccSmsInterfaceManager {

    protected int mIndex;

    private static final int EVENT_UPDATE_DONE_SPRD = 1;

    private static final String TAG = "IccSmsInterfaceManagerEx";

    public IccSmsInterfaceManagerEx(Phone phone) {
        super(phone);
        Log.d(TAG, "IccSmsInterfaceManagerEx super constructed");
    }

    protected Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            AsyncResult ar;

            switch (msg.what) {
                case EVENT_UPDATE_DONE_SPRD:
                    ar = (AsyncResult) msg.obj;
                    synchronized (mLock) {
                        mSuccess = (ar.exception == null);
                        if (ar.result != null && ar.result instanceof int[] &&
                                ((int[]) ar.result) != null && ((int[]) ar.result).length > 0) {
                            mIndex = ((int[]) ar.result)[0];
                            Log.d(TAG, "index : " + mIndex);
                        }
                        //getsmsindex end
                        mLock.notifyAll();
                    }
                    break;
            }
        }
    };


    public int copyMessageToIccEfSprd(String callingPackage, int status, byte[] pdu, byte[] smsc) {
        Log.d(TAG, "copyMessageToIccEf: status=" + status + " ==> " +
                "pdu=("+ Arrays.toString(pdu) +
                "), smsc=(" + Arrays.toString(smsc) +")");
        enforceReceiveAndSend("Copying message to Icc");
        if (mAppOps.noteOp(AppOpsManager.OP_WRITE_ICC_SMS, Binder.getCallingUid(),
                callingPackage) != AppOpsManager.MODE_ALLOWED) {
            return -1;
        }
        synchronized(mLock) {
            mSuccess = false;
            mIndex = -1;
            Message response = mHandler.obtainMessage(EVENT_UPDATE_DONE_SPRD);

            //RIL_REQUEST_WRITE_SMS_TO_SIM vs RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM
            if (PhoneConstants.PHONE_TYPE_GSM == mPhone.getPhoneType()) {
                mPhone.mCi.writeSmsToSim(status, IccUtils.bytesToHexString(smsc),
                        IccUtils.bytesToHexString(pdu), response);
            } else {
                mPhone.mCi.writeSmsToRuim(status, IccUtils.bytesToHexString(pdu),
                        response);
            }

            try {
                mLock.wait();
            } catch (InterruptedException e) {
                log("interrupted while trying to update by index");
            }
        }
        return mIndex;
    }
}
