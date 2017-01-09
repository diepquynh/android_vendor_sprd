package com.spreadst.validator;

import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import android.telephony.TelephonyManager;
import com.spreadst.validator.SprdSltNative;

public class IATUtils {
    private static final String TAG = "IATUtils";
    private static String strTmp = null;
    public static String AT_FAIL = "AT FAILED";
    public static String AT_OK = "OK";
    public static String AT_CONNECT = "CONNECT";
    public static int mPhoneCount = 0;
    private static TelephonyManager mTelephonyManager;
    private static int phoneId = 0;

    public static synchronized String sendATCmd(String cmd, String serverName) {
        strTmp = "error service can't get";
        String[] oemReq = new String[1];
        String[] oemResp = new String[1];
        oemReq[0] = cmd;
        mTelephonyManager = TelephonyManager.getDefault();
        mPhoneCount = mTelephonyManager.getPhoneCount();
        if (mPhoneCount == 1) {
            Log.d(TAG, "phone count is 1");
            serverName = "atchannel";
        }
        if (serverName.contains("atchannel0")) {
            phoneId = 0;
            Log.d(TAG, "<0> mAtChannel = " + phoneId + " , and cmd = " + cmd);
        } else if (serverName.contains("atchannel1")) {
            phoneId = 1;
            Log.d(TAG, "<1> mAtChannel = " + phoneId + " , and cmd = " + cmd);
        } else {
            phoneId = 0;
            Log.d(TAG, "<atchannel> mAtChannel = " + phoneId + " , and cmd = "
                    + cmd);
        }
        strTmp = SprdSltNative.native_sendATCmd(
                phoneId, cmd);
        if (serverName.contains("atchannel0")) {
            Log.d(TAG, "<0> AT response " + strTmp);
        } else if (serverName.contains("atchannel1")) {
            Log.d(TAG, "<1> AT response " + strTmp);
        } else {
            Log.d(TAG, "<atchannel> AT response " + strTmp);
        }
        return strTmp;
    }
}