
package com.sprd.omacp.transaction;

import android.content.Intent;
import java.security.Key;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import com.sprd.omacp.R;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Message;
import android.os.RemoteException;
import com.android.internal.telephony.ISub;
import android.os.ServiceManager;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
//import android.telephony.TelephonyManagerSprd;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.sprd.xml.parser.prv.Define;
import com.sprd.xml.parser.prv.OmacpUtils;

public class OtaPreParser {

 // private Intent mIntent;
    private int mSec;
    private String mMac;
    private byte[] mData;
    private byte[] mHeader;
    private int mRetryTimes;
    private final int mSubId;
    private final int mServiceId;
    private final String mMimeType;
    private final int mPduType;
    private static final boolean DEBUG = true;
    private static final String TAG = "OtaPreParser";

    /*public static final int OK = 0;
    public static final int ERROR_PIN_ERROR = 1;
    public static final int ERROR_PIN_EXCEPTION = 2;
    public static final int ERROR_MIME_ERROR = 3;
    public static final int ERROR_RETRY = 4;
    public static final int ERROR_NO_APN = 5;*/

    /*
     * Media Type Parameter From [OMA-WAP-TS-ProvCont-V1_1] section 4.3
     */
    public static final int SEC_TYPE_NETWPIN = 0;
    public static final int SEC_TYPE_USERPIN = 1;
    public static final int SEC_TYPE_USERNETWPIN = 2;
    public static final int SEC_TYPE_USERPINMAC = 3;
    public static final int SEC_TYPE_UNINITIALIZED = -1;
    public static final int MAX_PIN_INPUT_TIMES = 3;

    /*
     * see [wap-230-wsp-20010705-a] table 38.
     */
    private final int ANDROID_M_VERSION = 23;
    private static int INVALID_SUBSCRIPTION_ID = -1;
    public static final String WELL_KNOW_PARAMETER_SEC = "SEC";
    public static final String WELL_KNOW_PARAMETER_MAC = "MAC";


    public OtaPreParser(Intent intent, int serviceId) {

        mRetryTimes = 0;
        // mIntent = intent;
        //add for upgrade to Android N
        mSubId = intent.getIntExtra(PhoneConstants.SUBSCRIPTION_KEY,/*SubscriptionManager.getDefaultSmsSubId()*/getDefaultSmsSubId());
        //add for upgrade to Android N
        OmacpUtils.setSubId(mSubId);
        mServiceId = serviceId;

        mMimeType = intent.getType();
        mData = intent.getByteArrayExtra("data");
        mHeader = intent.getByteArrayExtra("header");
        mPduType = intent.getIntExtra("pduType", -12345);

        HashMap<String, String> contentTypeParameters = (HashMap<String, String>) intent
                .getExtra("contentTypeParameters");
        String sec = null;
        if (contentTypeParameters != null) {
            sec = contentTypeParameters.get(WELL_KNOW_PARAMETER_SEC);
            mMac = contentTypeParameters.get(WELL_KNOW_PARAMETER_MAC);
        }
        if (sec != null) {
            try {
                mSec = Integer.parseInt(sec);
            } catch (NumberFormatException e) {
                mSec = SEC_TYPE_UNINITIALIZED;
            }
        } else {
            mSec = SEC_TYPE_UNINITIALIZED;
        }
        if (DEBUG) {
            Log.d(TAG, "ota : " + this);
            Log.d(TAG, " header : " + OmacpUtils.bytesToHexString(mHeader));
            Log.d(TAG, " data : " + OmacpUtils.bytesToHexString(mData));
            Log.d(TAG, " sec = " + sec + "\n mMac = " + mMac);
            Log.d(TAG, " mSubId = " + mSubId);
        }
    }

    public boolean requiredInput() {
        return (mSec == SEC_TYPE_USERPIN || mSec == SEC_TYPE_USERNETWPIN);
    }

    private byte[] processImsi(String strImsi) {
        // add for bug 508133 begin
        if (null == strImsi) {
            System.out.println("the strImsi is null,will return null");
            return null;
        }
        // add for bug 508133 end
        int strLen = strImsi.length();
        if ((strLen / 2) == 0) {
            strImsi = "1" + strImsi + "F";
        } else {
            strImsi = "9" + strImsi;
        }
        char[] charImsi = strImsi.toCharArray();
        int len = charImsi.length / 2;
        byte[] byteImsi = new byte[len];
        for (int i = 0; i < len; i++) {
            int index = i * 2;
            int temp1 = Integer.parseInt(String.valueOf(charImsi[index]));
            int temp0 = Integer.parseInt(String.valueOf(charImsi[index + 1]));
            byteImsi[i] = (byte) ((temp0 << 4) | temp1);
        }
        return byteImsi;
    }

    // private static String getHexChar(String num) {
    // StringBuffer result = new StringBuffer("");
    // for (int i = 0; i < num.length(); i += 2) {
    // result.append((char) Integer.parseInt(num.substring(i, i + 2), 16));
    // }
    // return result.toString();
    // }

    private static String byteToChar(byte b) {
        StringBuilder result = new StringBuilder();
        int a1 = (b >>> 4);
        result.append("0123456789ABCDEF".charAt((int) (a1 % 16)));
        a1 = (b & 0x0f);
        result.append("0123456789ABCDEF".charAt((int) (a1 % 16)));
        return result.toString();
    }

    private int checkPin(byte[] localPin, byte[] pushData, String macData) {
        Mac mac;
        try {
            mac = Mac.getInstance("HmacSHA1");
            Key key = new SecretKeySpec(localPin, "HmacSHA1");
            mac.init(key);
            byte[] hashValue = mac.doFinal(pushData);
            StringBuilder sbuilder = new StringBuilder();
            for (byte b : hashValue) {
                sbuilder.append(byteToChar(b));
            }
            String rr = sbuilder.toString();
            // macData = getHexChar(macData);

            boolean result = (macData.compareTo(rr) == 0);
            if (DEBUG) {
                Log.d(TAG, "checkPin localPin:" + OmacpUtils.bytesToHexString(localPin));
                Log.d(TAG, " MAC:" + macData);
                Log.d(TAG, " hashValue:" + rr);
                Log.d(TAG, " result:" + result);
            }
            if (result) {
                return Define.STATE_OK;
            } else {
                return Define.ERROR_PIN_ERROR;
            }
        } catch (Exception e) {
            Log.w(TAG, "exception check pin ", e);
            return Define.ERROR_PIN_EXCEPTION;
        }
    }

    // add for bug 524981 begin
    public int securityCheck(Context context, String inputPin) {
    // add for bug 524981 end
        if (mSec == SEC_TYPE_UNINITIALIZED) {
            return Define.STATE_OK;
        }
        byte[] key = null;
        boolean macFlag = false;
        // add for bug 524981 begin
        System.out.println("use TelephonyManager.from(context)");
        TelephonyManager tm = TelephonyManager.from(context);
        // add for bug 524981 end
        String strImsi = "";
        switch (mSec) {
            case SEC_TYPE_NETWPIN: // 80
                macFlag = true;
                strImsi = tm.getSubscriberId(mSubId);
                // add for bug 508133 begin
                if (strImsi == null) {
                    System.out.println("the strImsi is null when SEC_TYPE_NETWPIN type");
                    return Define.ERROR_MIME_ERROR;
                }
                // add for bug 508133 end
                key = processImsi(strImsi);
                break;
            case SEC_TYPE_USERPIN: // 81
                macFlag = true;
                key = inputPin.getBytes();
                break;
            case SEC_TYPE_USERNETWPIN: // 82
                macFlag = true;
                strImsi = tm.getSubscriberId(mSubId);
                //add for bug 508133 begin
                if (strImsi == null) {
                    System.out.println("the strImsi is null when SEC_TYPE_USERNETWPIN type");
                    return Define.ERROR_MIME_ERROR;
                }
                //add for bug 508133 end
                byte[] byteImsi = processImsi(strImsi);
                byte[] bytePin = inputPin.getBytes();
                key = new byte[byteImsi.length + inputPin.length()];
                System.arraycopy(byteImsi, 0, key, 0, byteImsi.length);
                System.arraycopy(bytePin, 0, key, byteImsi.length, bytePin.length);
                break;
            case SEC_TYPE_USERPINMAC: // 83
                macFlag = false;
                break;
            default:
                return Define.ERROR_MIME_ERROR;
        }
        if (macFlag && key != null) {
            int result = checkPin(key, mData, mMac);
            mRetryTimes++;
            if (DEBUG) {
                Log.d(TAG, "securityCheck imsi:" + strImsi + " retryTimes:" + mRetryTimes
                        + " result:" + decodeErrorType(result));
            }
            if (result != Define.STATE_OK) {
                if (mSec == SEC_TYPE_NETWPIN || mRetryTimes >= MAX_PIN_INPUT_TIMES) {
                    return Define.ERROR_PIN_ERROR;
                } else {
                    return Define.ERROR_RETRY;
                }
            }
        }
        return Define.STATE_OK;
    }

    public void handleError(int flag, Context context) {
        switch (flag) {
            case Define.STATE_OK: // process ota message ok
                Toast.makeText(context, R.string.OTAConfiy_Success, Toast.LENGTH_SHORT).show();
                break;
            case Define.ERROR_PIN_ERROR: // pin error
                Toast.makeText(context, R.string.ota_pin_error, Toast.LENGTH_SHORT).show();
                break;
            case Define.ERROR_RETRY:
                Toast.makeText(context, R.string.ota_retry, Toast.LENGTH_SHORT).show();
                break;
            case Define.ERROR_PIN_EXCEPTION: // exception
                Toast.makeText(context, R.string.ota_pin_exception, Toast.LENGTH_SHORT).show();
                break;
            case Define.ERROR_MIME_ERROR: // error mimetype
                Toast.makeText(context, R.string.ota_mime_error, Toast.LENGTH_SHORT).show();
                break;
            case Define.ERROR_NO_APN:
            case Define.STATE_ERROR:    
                Toast.makeText(context, R.string.no_apn, Toast.LENGTH_SHORT).show();
                break;
        }
    }

    public String decodeErrorType(int result) {
        switch (result) {
            case Define.STATE_OK:
                return "OK";
            case Define.ERROR_PIN_ERROR:
                return "ERROR_PIN_ERROR";
            case Define.ERROR_PIN_EXCEPTION:
                return "ERROR_PIN_EXCEPTION";
            case Define.ERROR_MIME_ERROR:
                return "ERROR_MIME_ERROR";
            case Define.ERROR_RETRY:
                return "ERROR_RETRY";
            case Define.ERROR_NO_APN:
                return "ERROR_NO_APN";
            default:
                return "unknown result";
        }
    }

    public String getMimeType() {
        return mMimeType;
    }

    public byte[] getData() {
        return mData;
    }

    public int getSubId(){
        return mSubId;
    }

    public String toString() {
        StringBuilder builder = new StringBuilder();
        builder.append("serviceId:").append(mServiceId).append(" mimeType:").append(mMimeType)
                .append(" pduType:").append(mPduType).append(" SEC:").append(mSec).append(" MAC:")
                .append(mMac);
        // .append(" header:").append(IccUtils.bytesToHexString(mHeader))
        // .append(" data:").append(IccUtils.bytesToHexString(mData));
        return builder.toString();
    }

    public static int getDefaultSmsSubId() {
        int subId = INVALID_SUBSCRIPTION_ID;
        try {
            ISub iSub = ISub.Stub.asInterface(ServiceManager.getService("isub"));
            if (iSub != null) {
                subId = iSub.getDefaultSmsSubId();
            }
        } catch (RemoteException ex) {
            // ignore it
            Log.e(TAG, "getDefaultSmsSubId() Exception = ["+ex.toString()+"]");
        }
        Log.d(TAG,"getDefaultSmsSubId, sub id = " + subId);
        return subId;
    }

}
