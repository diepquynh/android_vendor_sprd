package com.sprd.validationtools;

import com.sprd.validationtools.utils.Native;

import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;

import android.telephony.TelephonyManager;

public class IATUtils {
    private static final String TAG = "IATUtils";
    //private static IAtChannel mAtChannel = null;
    private static String strTmp = null;
    public static String AT_FAIL = "AT FAILED";
    public static String AT_OK = "OK";
    public static int mPhoneCount = 0;


    public static String sendATCmd(String cmd, String serverName) {
        try {
            String result = sendAtCmd(cmd);
            return result;
        }catch (Exception e){
            Log.d("AT Exception",e.toString());
        }
        return "error service can't get";
    }

    public static String sendAtCmd(String cmd){
        //if (mTelephonyManager.invokeOemRilRequestStrings(0,command,result) >= 0) {
        String result = Native.native_sendATCmd(0, cmd);
        if(!result.contains("OK")){
           result = IATUtils.AT_FAIL;
        }
	    Log.d(TAG, "result = " + result);
        return result;
    }
}
