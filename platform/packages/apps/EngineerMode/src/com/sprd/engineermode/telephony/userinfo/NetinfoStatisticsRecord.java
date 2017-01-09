package com.sprd.engineermode.telephony.userinfo;

import com.sprd.engineermode.utils.IATUtils;

import android.content.Context;
import android.os.SystemProperties;
import java.text.NumberFormat;
import android.util.Log;
import android.telephony.TelephonyManager;
import com.sprd.engineermode.telephony.TelephonyManagerSprd;
import com.sprd.engineermode.R;

public class NetinfoStatisticsRecord {
    private static final String TAG = "NetinfoStatisticsRecord";

    private Context mContext;
    private int mPhoneCount;
    public int ROW;
    public int COL;
    private String[][] mTextValue;
    private String[] mFirstcol;
    private final int HANDOVER=0;
    private final int RESELECT=1;
    NumberFormat format = NumberFormat.getPercentInstance();

    String mLTEType = SystemProperties.get("persist.radio.ssda.mode");
    private boolean mIsSupportLTE = mLTEType.equals("svlte")
            || mLTEType.equals("tdd-csfb") || mLTEType.equals("fdd-csfb") || mLTEType.equals("csfb");

    public NetinfoStatisticsRecord(Context context){
        mContext=context;
    }

    public String getData(int simid){
        String result="";

        if (mIsSupportLTE) {
            result=parseReselectlte(simid)+","+parseHandOverlte(simid);
        } else {
            result=parseNetStatistic(simid,RESELECT)+","+parseNetStatistic(simid,HANDOVER);
        }
        result+=","+parseAttachTime(simid)+","+parseDropTimes(simid);
        return result;
    }

    private String parseReselectlte(int simindex) {
        String result="";
        ROW = 10;
        COL = 5;
        mTextValue = new String[ROW][COL];
        String atRSP = IATUtils.sendATCmd("AT+SPENGMD=0,7,1", "atchannel"
                + simindex);
        mFirstcol = mContext.getResources().getStringArray(
                R.array.Reselect_LTE);

        for (int index = 0; index < mFirstcol.length; index++) {
            mTextValue[index][0] = mFirstcol[index];
        }

        if (atRSP != null && atRSP.contains(IATUtils.AT_OK)) {
            String[] str = atRSP.split("-");
            int array_len = str.length;
            for (int i = 0; i < array_len; i++) {
                str[i] = str[i].replaceAll("\r|\n", "");
            }
            str[array_len - 1] = str[array_len - 1].substring(0,
                    str[array_len - 1].length() - 2);
            int temp = 0;
            for (int j = 1; j < ROW; j++) {
                for (int k = 1; k < COL - 1; k++) {
                    mTextValue[j][k] = str[temp++];
                    if (temp == 12) {
                        temp = temp + 2;
                    } // str[12],str[13] is useless,need to delete
                }
            }
            for (int z = 1; z < ROW; z++) {
                try {
                    if (Integer.parseInt(mTextValue[z][3]) == 0) {
                        format.setMinimumFractionDigits(2);
                        mTextValue[z][3] = format.format(1.0);
                    } else {
                        format.setMinimumFractionDigits(2);
                        mTextValue[z][3] = format.format((double) Integer
                                .parseInt(mTextValue[z][1])
                                / Integer.parseInt(mTextValue[z][3]));
                    }
                } catch (RuntimeException ex) {
                    Log.d(TAG, "shengyi parse at response fail");
                }
            }

            /* reselect delay according to "at+spengmd =0,7,1" response */
            mTextValue[1][COL - 1] = str[32];
            mTextValue[2][COL - 1] = str[33];
            mTextValue[3][COL - 1] = str[29];
            mTextValue[4][COL - 1] = str[30];
            mTextValue[5][COL - 1] = str[34];
            mTextValue[6][COL - 1] = str[31];
            mTextValue[7][COL - 1] = str[35];
            mTextValue[8][COL - 1] = str[36];
            mTextValue[9][COL - 1] = str[37];
        } else {
            for(int i=1;i<ROW;i++){
                for(int j=0;j<COL;j++){
                    mTextValue[i][j]="NA";
                }
            }
        }

        result+="\"";
        result+="   "+"Success"+" "+"Fail"+" "+"Pass ratio"+" "+"Delay\r\n";
        for(int i=1;i<ROW;i++){
            for(int j=0; j<COL; j++){
                result+=mTextValue[i][j];
                if(j != COL-1){
                    result+="    ";
                }
            }
            if(i != ROW){
                result+="\r\n";
            }
        }
        result+="\"";
        return result;
    }

    private String parseHandOverlte(int simindex) {
        String result="";
        ROW = 15;
        COL = 5;
        mTextValue = new String[ROW][COL];
        String cmd="AT+SPENGMD=0,7,2";
        String atRSP = IATUtils.sendATCmd(cmd, "atchannel"
                + simindex);
        mFirstcol = mContext
                .getResources().getStringArray(R.array.Handover_LTE);

        for (int index = 0; index < mFirstcol.length; index++) {
            mTextValue[index][0] = mFirstcol[index];
        }

        if (atRSP != null && atRSP.contains(IATUtils.AT_OK)) {
            String[] str = atRSP.split("-");
            int array_len = str.length;
            for (int i = 0; i < str.length; i++) {
                str[i] = str[i].replaceAll("\r|\n", "");
            }
            str[array_len - 1] = str[array_len - 1].substring(0,
                    str[array_len - 1].length() - 2);
            int temp = 0;
            for (int j = 1; j < ROW; j++) {
                for (int k = 1; k < COL - 1; k++) {
                    mTextValue[j][k] = str[temp++];
                }
            }
            for (int z = 1; z < ROW; z++) {
                try {
                    if (Integer.parseInt(mTextValue[z][3]) == 0) {
                        format.setMinimumFractionDigits(2);
                        mTextValue[z][3] = format.format(1.0);
                    } else {
                        format.setMinimumFractionDigits(2);
                        mTextValue[z][3] = format.format((double) Integer
                                .parseInt(mTextValue[z][1])
                                / Integer.parseInt(mTextValue[z][3]));
                    }
                } catch (RuntimeException ex) {
                    Log.d(TAG, "parse at response fail");
                }
            }

            /* handover delay according to "AT+SPENGMD=0,7,2" response */
            mTextValue[1][COL - 1] = str[48];
            mTextValue[2][COL - 1] = str[49];
            mTextValue[3][COL - 1] = str[42];
            mTextValue[4][COL - 1] = str[44];
            mTextValue[5][COL - 1] = str[50];
            mTextValue[6][COL - 1] = str[43];
            mTextValue[7][COL - 1] = str[45];
            mTextValue[8][COL - 1] = str[47];
            mTextValue[9][COL - 1] = str[54];
            mTextValue[10][COL - 1] = str[46];
            mTextValue[11][COL - 1] = str[51];
            mTextValue[12][COL - 1] = str[52];
            mTextValue[13][COL - 1] = str[55];
            mTextValue[14][COL - 1] = str[53];
        } else {
            for(int i=1;i<ROW;i++){
                for(int j=0;j<COL;j++){
                    mTextValue[i][j]="NA";
                }
            }
        }

        result+="\"";
        result+="   "+"Success"+" "+"Fail"+" "+"Pass ratio"+" "+"Delay\r\n";
        for(int i=1;i<ROW;i++){
            for(int j=0; j<COL; j++){
                result+=mTextValue[i][j];
                if(j != COL-1){
                    result+="    ";
                }
            }
            if(i != ROW){
                result+="\r\n";
            }
        }
        result+="\"";
        return result;
    }

    private String parseNetStatistic(int simindex,int type) {
        String result="";
        ROW = 5;
        COL = 5;
        mTextValue = new String[ROW][COL];
        String cmd="";

        if(HANDOVER==type){
            cmd="AT+SPENGMD=0,7,2";
        }else if(RESELECT==type){
            cmd="AT+SPENGMD=0,7,1";
        }
        String atRSP = IATUtils.sendATCmd(cmd, "atchannel"
                + simindex);

        if (atRSP != null && atRSP.contains(IATUtils.AT_OK)) {
            String[] str = atRSP.split("\n");
            String[] str1 = str[0].split("-");
            for (int i = 0; i < str1.length; i++) {
                str1[i] = str1[i].replaceAll("\r|\n", "");
            }
            int modemType = TelephonyManagerSprd.getModemType();
            Log.d(TAG, "modemType==" + modemType);
            if (modemType == TelephonyManagerSprd.MODEM_TYPE_WCDMA) {
                mTextValue[1][0] = "W-W";
                mTextValue[2][0] = "W-G";
                mTextValue[3][0] = "G-G";
                mTextValue[4][0] = "G-W";
            } else if (modemType == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
                mTextValue[1][0] = "TD-TD";
                mTextValue[2][0] = "TD-G";
                mTextValue[3][0] = "G-G";
                mTextValue[4][0] = "G-TD";
            }
            int temp = 0;
            for (int j = 1; j < 5; j++) {
                for (int k = 1; k < 4; k++) {
                    mTextValue[j][k] = str1[temp++];
                }
            }
            if(HANDOVER==type){
                mTextValue[1][COL - 1] = str1[48];
                mTextValue[2][COL - 1] = str1[49];
                mTextValue[3][COL - 1] = str1[42];
                mTextValue[4][COL - 1] = str1[44];
            }else if(RESELECT==type){
                mTextValue[1][COL - 1] = str1[32];
                mTextValue[2][COL - 1] = str1[33];
                mTextValue[3][COL - 1] = str1[29];
                mTextValue[4][COL - 1] = str1[30];
            }

            for (int z = 1; z < 5; z++) {
                try {
                    if (Integer.parseInt(mTextValue[z][3]) == 0) {
                        format.setMinimumFractionDigits(2);
                        mTextValue[z][3] = format.format(1.0);
                    } else {
                        format.setMinimumFractionDigits(2);
                        mTextValue[z][3] = format.format((double) Integer
                                .parseInt(mTextValue[z][1])
                                / Integer.parseInt(mTextValue[z][3]));
                    }
                } catch (RuntimeException ex) {
                    Log.d(TAG, "parse at response fail");
                }
            }
        } else {
            for(int i=1;i<ROW;i++){
                for(int j=0;j<COL;j++){
                    mTextValue[i][j]="NA";
                }
            }
        }

        result+="\"";
        result+="   "+"Success"+" "+"Fail"+" "+"Pass ratio"+" "+"Delay\r\n";
        for(int i=1;i<ROW;i++){
            for(int j=0; j<COL; j++){
                result+=mTextValue[i][j];
                if(j != COL-1){
                    result+="    ";
                }
            }
            if(i != ROW){
                result+="\r\n";
            }
        }
        result+="\"";
        return result;
    }

    private String parseAttachTime(int simindex) {
        String result="";
        ROW = 4;
        COL = 2;
        mTextValue = new String[ROW][COL];
        String atRSP = IATUtils.sendATCmd("AT+SPENGMD=0,7,7", "atchannel"
                + simindex);

        if (atRSP != null && atRSP.contains(IATUtils.AT_OK)) {
            String[] str = atRSP.split("-");
            int array_len = str.length;
            for (int i = 0; i < str.length; i++) {
                str[i] = str[i].replaceAll("\r|\n", "");
            }
            str[array_len - 1] = str[array_len - 1].substring(0,
                    str[array_len - 1].length() - 2);
            mTextValue[3][1] = str[4];
            if (!mIsSupportLTE) {
                mTextValue[3][1] = "NA";
            }
            mTextValue[0][1] = str[0];
            mTextValue[1][1] = str[1];
            mTextValue[2][1] = str[3];
            int modemType = TelephonyManagerSprd.getModemType();
            Log.d(TAG, "modemType==" + modemType);
            if (modemType == TelephonyManagerSprd.MODEM_TYPE_WCDMA) {
                mTextValue[0][0] = "Time on GSM";
                mTextValue[1][0] = "Time on WCDMA";
                mTextValue[2][0] = "All the Time";
                mTextValue[3][0] = "Time on FDD-LTE";
            } else if (modemType == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
                mTextValue[0][0] = "Time on GSM";
                mTextValue[1][0] = "Time on TD";
                mTextValue[2][0] = "All the Time";
                mTextValue[3][0] = "Time on TD-LTE";
            } else if (mIsSupportLTE) {
                mTextValue[0][0] = "Time on 2G";
                mTextValue[1][0] = "Time on 3G";
                mTextValue[2][0] = "All the Time";
                mTextValue[3][0] = "Time on 4G";
            }
        } else {
            for(int i=0;i<ROW;i++){
                for(int j=0;j<COL;j++){
                    mTextValue[i][j]="NA";
                }
            }
        }

        result+="\"";
        for(int i=0;i<ROW;i++){
            for(int j=0; j<COL; j++){
                result+=mTextValue[i][j];
                if(j != COL-1){
                    result+="  ";
                }
            }
            if(i != ROW){
                result+="\r\n";
            }
        }
        result+="\"";
        return result;
    }

    private String parseDropTimes(int simindex) {
        String result="";
        ROW = 3;
        COL = 2;
        mTextValue = new String[ROW][COL];
        String atRSP = IATUtils.sendATCmd("AT+SPENGMD=0,7,0", "atchannel"
                    + simindex);

        if (atRSP != null && atRSP.contains(IATUtils.AT_OK)) {
            String[] str = atRSP.split("-");
            int array_len = str.length;
            for (int i = 0; i < str.length; i++) {
                str[i] = str[i].replaceAll("\r|\n", "");
            }
            str[array_len - 1] = str[array_len - 1].substring(0,
                    str[array_len - 1].length() - 2);
            mTextValue[2][1] = str[2];
            if (!mIsSupportLTE) {
                mTextValue[2][1] = "NA";
            }
            mTextValue[0][1] = str[0];
            mTextValue[1][1] = str[1];
            int modemType = TelephonyManagerSprd.getModemType();
            Log.d(TAG, "modemType==" + modemType);
            if (modemType == TelephonyManagerSprd.MODEM_TYPE_WCDMA) {
                mTextValue[0][0] = "Drop times on GSM";
                mTextValue[1][0] = "Drop times on WCDMA";
                mTextValue[2][0] = "Drop times on FDD-LTE";
            } else if (modemType == TelephonyManagerSprd.MODEM_TYPE_TDSCDMA) {
                mTextValue[0][0] = "Drop times on GSM";
                mTextValue[1][0] = "Drop times on TD";
                mTextValue[2][0] = "Drop times on TD-LTE";
            } else if (mIsSupportLTE) {
                mTextValue[0][0] = "Drop times on 2G";
                mTextValue[1][0] = "Drop times on 3G";
                mTextValue[2][0] = "Drop times on 4G";
            }
        } else {
            for(int i=0;i<ROW;i++){
                for(int j=0;j<COL;j++){
                    mTextValue[i][j]="NA";
                }
            }
        }

        result+="\"";
        for(int i=0;i<ROW;i++){
            for(int j=0; j<COL; j++){
                result+=mTextValue[i][j];
                if(j != COL-1){
                    result+="  ";
                }
            }
            if(i < ROW-1){
                result+="\r\n";
            }
        }
        result+="\"";
        return result;
    }
    private boolean isSimExist(int simIndex) {
        if (TelephonyManager.from(mContext).getSimState(simIndex) == TelephonyManager.SIM_STATE_READY) {
            return true;
        }
        return false;
    }
}
