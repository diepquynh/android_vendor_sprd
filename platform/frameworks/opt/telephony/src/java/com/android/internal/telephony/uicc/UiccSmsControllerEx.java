package com.android.internal.telephony;

import android.os.ServiceManager;
import android.os.RemoteException;
import android.telephony.Rlog;

import com.android.internal.telephony.ISmsEx;

import java.lang.ArrayIndexOutOfBoundsException;
import java.lang.NullPointerException;
import java.util.List;

/**
 * {@hide}
 */
public class UiccSmsControllerEx extends ISmsEx.Stub {
    private static final String TAG = "UiccSmsControllerEx";
    private Phone[] mPhone;
    static final int ENABLE_CHANNEL_LANG = 1;

    /* only one UiccSmsControllerEx exists */
    public UiccSmsControllerEx(Phone[] phone) {
        if (ServiceManager.getService("ismsEx") == null) {
            ServiceManager.addService("ismsEx", this);
        }
        mPhone = phone;
    }

    /**
     * get Icc Sms  interface manager object based on subscription.
     **/
    private IccSmsInterfaceManager getIccSmsInterfaceManager(int subId) {

        int phoneId = SubscriptionController.getInstance().getPhoneId(subId);
        try {
            return mPhone[phoneId].getIccSmsInterfaceManager();
        } catch (NullPointerException e) {
            Rlog.e(TAG, "Exception is :" + e.toString() + " For subscription :" + subId);
            e.printStackTrace(); // To print stack trace
            return null;
        } catch (ArrayIndexOutOfBoundsException e) {
            Rlog.e(TAG, "Exception is :" + e.toString() + " For subscription :" + subId);
            e.printStackTrace();
            return null;
        }
    }
    
  //method to send AT command. bug 489257
    public boolean setCellBroadcastSmsConfig(IccSmsInterfaceManager iccSmsIntMgr, long subId, int[] data) {
        if (iccSmsIntMgr != null) {
            Rlog.d(TAG, "Add for Disabled channel.");
            iccSmsIntMgr.setCellBroadcastSmsConfig(data);
            return true;
        } else {
            Rlog.e(TAG, "Disabled channel iccSmsIntMgr is null for"
                    + " Subscription: " + subId);
        }
        return false;
    }
    
  //a common Interface for app to call skip adding aidl bug 489257
    @Override
    public boolean commonInterfaceForMessaging(int commonType, long szSubId, String szString, int[] data){
        Rlog.d(TAG, "Enter commonInterfaceForMessaging.");
        IccSmsInterfaceManager iccSmsIntMgr = getIccSmsInterfaceManager((int)szSubId);
        switch (commonType) {
        case ENABLE_CHANNEL_LANG:
            setCellBroadcastSmsConfig(iccSmsIntMgr, szSubId, data);
            return true;

        default:
            break;
        }
        return false;
    }

    @Override
    public int copyMessageToIccEfForSubscriber(int subId, String callingPackage, int status,
                                                   byte[] pdu, byte[] smsc) throws android.os.RemoteException {
        IccSmsInterfaceManager iccSmsIntMgr = getIccSmsInterfaceManager(subId);
        if (iccSmsIntMgr != null) {
            return iccSmsIntMgr.copyMessageToIccEfSprd(callingPackage, status, pdu, smsc);
        } else {
            Rlog.e(TAG,"copyMessageToIccEfForSubscriber iccSmsIntMgr is null" +
                    " for Subscription: " + subId);
            return 0;
        }
    }
}
