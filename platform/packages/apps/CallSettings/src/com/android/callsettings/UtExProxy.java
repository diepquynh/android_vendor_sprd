package com.android.callsettings;

import java.util.HashMap;

import com.android.ims.internal.IImsUt;
import com.android.ims.ImsUt;
import com.android.ims.ImsCallForwardInfo;
import com.android.ims.ImsException;
import com.android.ims.ImsReasonInfo;
import com.android.ims.ImsSsInfo;
import android.os.RemoteException;

import android.content.Context;
import android.os.IBinder;
import android.os.Message;
import android.os.Bundle;
import android.os.ServiceManager;
import android.os.AsyncResult;

import com.android.ims.internal.ImsManagerEx;
import com.android.ims.internal.IImsUtListenerEx;
import com.android.ims.internal.IImsUtEx;
import com.android.ims.internal.ImsCallForwardInfoEx;

public class UtExProxy {

    private static final String LOG_TAG = "UtExProxy";
    // For synchronization of private variables
    private Object mLockObj = new Object();
    private HashMap<Integer, Message> mPendingCmds =
            new HashMap<Integer, Message>();

    private IImsUtEx mIImsUtEx;
    //TODO:get correct phone id
    private int mPhoneId = 0;
    private static UtExProxy mUtExProxy = null;

    private UtExProxy(){
    }

    public static UtExProxy getInstance() {
        if (mUtExProxy == null) {
            mUtExProxy = new UtExProxy();
        }
        return mUtExProxy;
    }

    public void setPhoneId(int phoneId) {
        if (mPhoneId != phoneId) {
            mPhoneId = phoneId;
            mIImsUtEx = null;
        }
    }

    private IImsUtEx getIImsUtEx(){
        if(mIImsUtEx == null){
            mIImsUtEx = ImsManagerEx.getIImsUtEx();
            try{
                mIImsUtEx.setListenerEx(mPhoneId, mImsUtListenerExBinder);
            } catch (RemoteException e) {
            }
        }
        return mIImsUtEx;
    }

    public void setCallForwardingOption(int phoneId, int commandInterfaceCFAction,
            int commandInterfaceCFReason,int serviceClass, String dialingNumber,
            int timerSeconds, String ruleSet, Message result){
        synchronized(mLockObj) {
            try {
                int id = getIImsUtEx().setCallForwardingOption(phoneId, commandInterfaceCFAction,
                        commandInterfaceCFReason, serviceClass, dialingNumber,
                        timerSeconds, ruleSet);

                if (id < 0) {
                    sendFailureReport(result,
                            new ImsReasonInfo(ImsReasonInfo.CODE_UT_SERVICE_UNAVAILABLE, 0));
                    return;
                }

                mPendingCmds.put(Integer.valueOf(id), result);
            } catch (RemoteException e) {
                sendFailureReport(result,
                        new ImsReasonInfo(ImsReasonInfo.CODE_UT_SERVICE_UNAVAILABLE, 0));
            }
        }
    }

    public void getCallForwardingOption(int commandInterfaceCFReason, int serviceClass,
            String ruleSet, Message result) {
        synchronized(mLockObj) {
            try {
                int id = getIImsUtEx().getCallForwardingOption(mPhoneId, commandInterfaceCFReason, serviceClass,
                        ruleSet);
                if (id < 0) {
                    sendFailureReport(result,
                            new ImsReasonInfo(ImsReasonInfo.CODE_UT_SERVICE_UNAVAILABLE, 0));
                    return;
                }

                mPendingCmds.put(Integer.valueOf(id), result);
            } catch (RemoteException e) {
                sendFailureReport(result,
                        new ImsReasonInfo(ImsReasonInfo.CODE_UT_SERVICE_UNAVAILABLE, 0));
            }
        }
    }


    private final IImsUtListenerEx.Stub mImsUtListenerExBinder = new IImsUtListenerEx.Stub(){
        /**
         * Notifies the result of the supplementary service configuration udpate.
         */
        @Override
        public void utConfigurationUpdated(IImsUt ut, int id) {
            Integer key = Integer.valueOf(id);

            synchronized(mLockObj) {
                sendSuccessReport(mPendingCmds.get(key));
                mPendingCmds.remove(key);
            }
        }

        @Override
        public void utConfigurationUpdateFailed(IImsUt ut, int id, ImsReasonInfo error) {
            Integer key = Integer.valueOf(id);

            synchronized(mLockObj) {
                sendFailureReport(mPendingCmds.get(key), error);
                mPendingCmds.remove(key);
            }
        }

        /**
         * Notifies the result of the supplementary service configuration query.
         */
        @Override
        public void utConfigurationQueried(IImsUt ut, int id, Bundle ssInfo) {
            Integer key = Integer.valueOf(id);

            synchronized(mLockObj) {
                sendSuccessReport(mPendingCmds.get(key), ssInfo);
                mPendingCmds.remove(key);
            }
        }

        @Override
        public void utConfigurationQueryFailed(IImsUt ut, int id, ImsReasonInfo error) {
            Integer key = Integer.valueOf(id);

            synchronized(mLockObj) {
                sendFailureReport(mPendingCmds.get(key), error);
                mPendingCmds.remove(key);
            }
        }

        /**
         * Notifies the status of the call barring supplementary service.
         */
        @Override
        public void utConfigurationCallBarringQueried(IImsUt ut,
                int id, ImsSsInfo[] cbInfo) {
            Integer key = Integer.valueOf(id);

            synchronized(mLockObj) {
                sendSuccessReport(mPendingCmds.get(key), cbInfo);
                mPendingCmds.remove(key);
            }
        }

        /**
         * Notifies the status of the call forwarding supplementary service.
         */
        @Override
        public void utConfigurationCallForwardQueried(IImsUt ut,
                int id, ImsCallForwardInfoEx[] cfInfo) {
            Integer key = Integer.valueOf(id);

            synchronized(mLockObj) {
                sendSuccessReport(mPendingCmds.get(key), cfInfo);
                mPendingCmds.remove(key);
            }
        }

        /**
         * Notifies the status of the call waiting supplementary service.
         */
        @Override
        public void utConfigurationCallWaitingQueried(IImsUt ut,
                int id, ImsSsInfo[] cwInfo) {
            Integer key = Integer.valueOf(id);

            synchronized(mLockObj) {
                sendSuccessReport(mPendingCmds.get(key), cwInfo);
                mPendingCmds.remove(key);
            }
        }
    };

    private void sendFailureReport(Message result, ImsReasonInfo error) {
        if (result == null || error == null) {
            return;
        }

        String errorString;
        // If ImsReasonInfo object does not have a String error code, use a
        // default error string.
        if (error.mExtraMessage == null) {
            errorString = new String("IMS UT exception");
        }
        else {
            errorString = new String(error.mExtraMessage);
        }
        AsyncResult.forMessage(result, null, new ImsException(errorString, error.mCode));
        result.sendToTarget();
    }

    private void sendSuccessReport(Message result) {
        if (result == null) {
            return;
        }

        AsyncResult.forMessage(result, null, null);
        result.sendToTarget();
    }

    private void sendSuccessReport(Message result, Object ssInfo) {
        if (result == null) {
            return;
        }

        AsyncResult.forMessage(result, ssInfo, null);
        result.sendToTarget();
    }

}
