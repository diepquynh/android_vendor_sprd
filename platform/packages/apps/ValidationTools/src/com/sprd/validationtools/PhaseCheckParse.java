
package com.sprd.validationtools;

import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import android.util.Log;
import android.os.SystemProperties;

import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.Parcel;
import android.os.ParcelFileDescriptor;

/*Parse the phasecheck as the little endian*/
public class PhaseCheckParse {
    private static String TAG = "PhaseCheckParse";
    private static int MAX_SN_LEN = 24;
    private static int SP09_MAX_SN_LEN = MAX_SN_LEN;
    private static int SP09_MAX_STATION_NUM = 15;
    private static int SP09_MAX_STATION_NAME_LEN = 10;
    private static int SP09_SPPH_MAGIC_NUMBER = 0x53503039;
    private static int SP05_SPPH_MAGIC_NUMBER = 0x53503035;
    private static int SP09_MAX_LAST_DESCRIPTION_LEN = 32;

    private static int SN1_START_INDEX = 4;
    private static int SN2_START_INDEX = SN1_START_INDEX + SP09_MAX_SN_LEN;

    private static int STATION_START_INDEX = 56;
    private static int TESTFLAG_START_INDEX = 252;
    private static int RESULT_START_INDEX = 254;

    private static int TYPE_GET_SN1 = 0;
    private static int TYPE_GET_SN2 = 1;
    private static int TYPE_WRITE_STATION_TESTED = 2;
    private static int TYPE_WRITE_STATION_PASS = 3;
    private static int TYPE_WRITE_STATION_FAIL = 4;
    private static int TYPE_GET_PHASECHECK = 5;
    private static int TYPE_WRITE_CHARGE_SWITCH = 6;

    private static String PHASE_CHECKE_FILE = "miscdata";

    private byte[] stream = new byte[300];
    private IBinder binder;

    public PhaseCheckParse() {

        FileInputStream in = null;
        String filePath = SystemProperties.get("ro.product.partitionpath") + PHASE_CHECKE_FILE;
        File fp = new File(filePath);
        if (!fp.exists()) {
            Log.d(TAG, filePath + "not exist!");
            stream = null;
            return;
        }

        try {
            in = new FileInputStream(fp);
            if (in != null) {
                in.read(stream, 0, stream.length);
                Log.d(TAG, " ");
            }
        } catch (Exception e) {

        } finally {
            try {
                if (in != null) {
                    in.close();
                    in = null;
                }
            } catch (IOException io) {
                Log.e(TAG, "close in err");
            }
        }

        if (!checkPhaseCheck()) {
            stream = null;
        }

        binder = ServiceManager.getService("phasechecknative");

        if(binder != null)
            Log.e(TAG, "Get The service connect!");
        else
            Log.e(TAG, "connect Error!!");
    }

    private boolean hasDigit(String content) {
        boolean flag = false;
        Pattern p = Pattern.compile(".*\\d+.*");
        Matcher m = p.matcher(content);
        if (m.matches())
            flag = true;
        return flag;
    }

    /*SPRD: Add for bug556367, Equipment serial number are unintelligible string {@ */
    private boolean isInvalid(String content) {
        boolean flag = true;
        Pattern p = Pattern.compile("^[A-Za-z0-9]+$");
        Matcher m = p.matcher(content);
        if (m.matches())
            flag = false;
        return flag;
    }
    /* {@ */

    private String StationTested(char testSign, char item) {
        if(testSign=='0' && item=='0') return "PASS";
        if(testSign=='0' && item=='1') return "FAIL";
        return "UnTested";
    }

    private boolean checkPhaseCheck() {
        Log.d(TAG, " " + stream[0] + stream[1] + stream[2] + stream[3]);
        if ((stream[0] == '9' || stream[0] == '5')
                && stream[1] == '0'
                && stream[2] == 'P'
                && stream[3] == 'S') {
            return true;
        }

        return false;
    }

    public String getSn() {
        String result = null;
        try{
            Parcel data = Parcel.obtain();
            Parcel reply = Parcel.obtain();
            binder.transact(0, data, reply, 0);
            Log.e(TAG, "transact end");
            String sn1 = reply.readString();
            for(int i = 0; i < 5; i++) {
                if(hasDigit(sn1)) {
                    break;
                }
                binder.transact(TYPE_GET_SN1, data, reply, 0);
                sn1 = reply.readString();
            }
            binder.transact(TYPE_GET_SN2, data, reply, 0);
            String sn2 = reply.readString();
            for(int i = 0; i < 5; i++) {
                if(hasDigit(sn2)) {
                    break;
                }
                binder.transact(1, data, reply, 0);
                sn2 = reply.readString();
            }
            /*SPRD: Add for bug556367, Equipment serial number are unintelligible string {@ */
            if (!sn1.isEmpty() && isInvalid(sn1)) {
                sn1 = "invalid";
            }
            if (!sn2.isEmpty() && isInvalid(sn2)) {
                sn2 = "invalid";
            }
            /* {@ */
            result = "SN1:" + sn1 + "\n" + "SN2:" + sn2;
            Log.e(TAG, "SN1 = " +  sn1 + " SN2=" + sn2);
            data.recycle();
            reply.recycle();
        }catch (Exception ex) {
            Log.e(TAG, "Exception " + ex.getMessage());
            result = "get SN fail:" + ex.getMessage();
        }
        return result;
    }

    public boolean writeStationTested(int station) {
        try{
            Parcel data = Parcel.obtain();
            data.writeInt(station);
            binder.transact(TYPE_WRITE_STATION_TESTED, data, null, 0);
            Log.e(TAG, "data = " + data.readString() + " SUCESS!!");
            data.recycle();
            return true;
        }catch (Exception ex) {
            Log.e(TAG, "Exception " + ex.getMessage());
            return false;
        }
    }

    public boolean writeStationPass(int station) {
        try{
            Parcel data = Parcel.obtain();
            data.writeInt(station);
            binder.transact(TYPE_WRITE_STATION_PASS, data, null, 0);
            Log.e(TAG, "data = " + data.readString() + " SUCESS!!");
            data.recycle();
            return true;
        }catch (Exception ex) {
            Log.e(TAG, "Exception " + ex.getMessage());
            return false;
        }
    }

    public boolean writeChargeSwitch(int value) {
        try{
            Parcel data = Parcel.obtain();
            Parcel reply = Parcel.obtain();
            data.writeInt(value);
            binder.transact(TYPE_WRITE_CHARGE_SWITCH, data, reply, 0);
            Log.e(TAG, "writeChargeSwitch data = " + reply.readString() + " SUCESS!!");
            data.recycle();
            return true;
        }catch (Exception ex) {
            Log.e(TAG, "Exception " , ex);
            return false;
        }
    }

    public boolean writeStationFail(int station) {
        try{
            Parcel data = Parcel.obtain();
            data.writeInt(station);
            binder.transact(TYPE_WRITE_STATION_FAIL, data, null, 0);
            Log.e(TAG, "data = " + data.readString() + " SUCESS!!");
            data.recycle();
            return true;
        }catch (Exception ex) {
            Log.e(TAG, "Exception " + ex.getMessage());
            return false;
        }
    }

    public String getPhaseCheck() {
        String result = null;
        try{
            Parcel data = Parcel.obtain();
            Parcel reply = Parcel.obtain();
            binder.transact(TYPE_GET_PHASECHECK, data, reply, 0);
            Log.e(TAG, "transact SUCESS!!");
            int testSign = reply.readInt();
            int item = reply.readInt();
            String stationName = reply.readString();
            String []str = stationName.split(Pattern.quote("|"));
            String strTestSign = Integer.toBinaryString(testSign);
            String strItem = Integer.toBinaryString(item);
            char[] charSign = strTestSign.toCharArray();
            char[] charItem = strItem.toCharArray();
            StringBuffer sb = new StringBuffer();
            Log.e(TAG, "strTestSign = " + strTestSign + " strItem = " + strItem);
            for(int i=0; i<str.length; i++) {
                sb.append(str[i]+":"+StationTested(charSign[charSign.length-i-1], charItem[charItem.length-i-1])+"\n");
            }
            result = sb.toString();
            data.recycle();
            reply.recycle();
        }catch (Exception ex) {
            Log.e(TAG, "huasong Exception " + ex.getMessage());
            result = "get phasecheck fail:" + ex.getMessage();
        }
        return result;
    }

    public String getSn1() {
        if (stream == null) {
            return "Invalid Sn1!";
        }
        if (!isAscii(stream[SN1_START_INDEX])) {
            Log.d(TAG, "Invalid Sn1!");
            return "Invalid Sn1!";
        }

        String sn1 = new String(stream, SN1_START_INDEX, SP09_MAX_SN_LEN);
        Log.d(TAG, sn1);
        return sn1;
    }

    public String getSn2() {
        if (stream == null) {
            return "Invalid Sn2!";
        }
        if (!isAscii(stream[SN2_START_INDEX])) {
            Log.d(TAG, "Invalid Sn2!");
            return "Invalid Sn2!";
        }
        String sn2 = new String(stream, SN2_START_INDEX, SP09_MAX_SN_LEN);
        Log.d(TAG, sn2);
        return sn2;
    }

    private boolean isAscii(byte b) {
        if (b >= 0 && b <= 127) {
            return true;
        }
        return false;
    }

    public String getTestsAndResult() {
        if (stream == null) {
            return "Invalid Phase check!";
        }

        if (!isAscii(stream[STATION_START_INDEX])) {
            Log.d(TAG, "Invalid Phase check!");
            return "Invalid Phase check!";
        }
        String testResult = null;
        String allResult = "";

        int flag = 1;
        for (int i = 0; i < SP09_MAX_STATION_NUM; i++) {
            if (0 == stream[STATION_START_INDEX + i * SP09_MAX_STATION_NAME_LEN]) {
                Log.d(TAG, "break " + i);
                break;
            }
            testResult = new String(stream, STATION_START_INDEX + i * SP09_MAX_STATION_NAME_LEN,
                    SP09_MAX_STATION_NAME_LEN);
            if (!isStationTest(i)) {
                testResult += " Not test";
            } else if (isStationPass(i)) {
                testResult += " Pass";
            } else {
                testResult += " Failed";
            }
            flag = flag << 1;
            Log.d(TAG, i + " " + testResult);
            allResult += testResult + "\n";
        }
        return allResult;
    }

    private boolean isStationTest(int station) {
        byte flag = 1;
        if (station < 8) {
            return (0 == ((flag << station) & stream[TESTFLAG_START_INDEX]));
        } else if (station >= 8 && station < 16) {
            return (0 == ((flag << (station - 8)) & stream[TESTFLAG_START_INDEX + 1]));
        }
        return false;
    }

    private boolean isStationPass(int station) {
        byte flag = 1;
        if (station < 8) {
            return (0 == ((flag << station) & stream[RESULT_START_INDEX]));
        } else if (station >= 8 && station < 16) {
            return (0 == ((flag << (station - 8)) & stream[RESULT_START_INDEX + 1]));
        }
        return false;
    }
    /* SPRD: 435125 The serial number shows invalid in ValidationTools @{*/
    public static String getSerialNumber(){
        return android.os.Build.SERIAL;
    }
    /* @}*/
}