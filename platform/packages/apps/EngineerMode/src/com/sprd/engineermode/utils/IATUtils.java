package com.sprd.engineermode.utils;

import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.utils.EngineerModeNative;

public class IATUtils {
    private static final String TAG = "IATUtils";
    public static String AT_FAIL = "AT FAILED";
    public static String AT_OK = "OK";
    public static String AT_CONNECT = "CONNECT";

    /**
     * send AT cmd to modem.
     * @param cmd:specific AT commands to be sent.
     * @param serverName:phoneId.
     * @return at command return value.
     * @hide
     */
    public static synchronized String sendATCmd(String cmd, String serverName) {
        String strTmp = "error service can't get";
        int mPhoneCount = TelephonyManager.getDefault().getPhoneCount();
        int phoneId = 0;
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
        strTmp = EngineerModeNative.native_sendATCmd(phoneId, cmd);
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
