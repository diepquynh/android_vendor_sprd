package com.sprd.engineermode.telephony.userinfo;

import android.content.Context;
import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.IATUtils;

import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;
import android.util.Log;

public class LteShowRecord {

    private static final String TAG = "LteShowRecord";
    private Context mContext;
    public boolean isRoaming;
    public String mNetworkOperator;
    public String mNetworkOperatorName;
    private int mSubId;

    private TelephonyManager mTelephonyManager;
    private SubscriptionManager mSubscriptionManager;
    private String SaveName = "atchannel";
    private String[][] mTextValue;
    public int ROW;
    public int COL;

    private String[][] Description_name = {
            { "Not Support", "Support" },
            { "Not Support", "Support" },
            { "Unknown", "GEA1", "GEA2", "GEA3" },
            { "A51", "A52", "A53", "A54", "A55", "A56", "A57" },
            { "Unknown", "UEA0", "UEA1" },
            { "Not support Hsdpa and Hsupa", "Support Hsdpa", "Support Hsupa",
                    "Support Hsdpa and Hsupa" }, { "Not Support", "Support" },
            { "Not Support", "VAMOS1", "VAMOS2" },
            { "Not Support", "Support" }, { "Not Support", "Support" },
            { "other", "R8", "R9" }, { "Not Support", "Support" },
            { "Unknown", "eea0", "eea1", "eea2" },
            { "Unknown", "eia0 ", "eia1", "eia2" } };

    int[] cell_name=new int[]{R.string.netinfo_cell_0,R.string.netinfo_cell_1,
            R.string.netinfo_cell_2,R.string.netinfo_cell_3,
            R.string.netinfo_cell_4};

    public LteShowRecord(Context context){
        mContext=context;
        mTelephonyManager = (TelephonyManager) TelephonyManager
                .from(mContext);
        mSubscriptionManager = (SubscriptionManager) SubscriptionManager
                .from(mContext);
    }

    public String getServerData(int simid){
        String result="";
        mSubId=slotIdToSubId(simid);

        mNetworkOperatorName = mTelephonyManager.getNetworkOperatorName(mSubId);
        mNetworkOperator = mTelephonyManager.getNetworkOperatorForPhone(simid);
        isRoaming = mTelephonyManager.isNetworkRoaming(mSubId);
        String[] values = mContext.getResources().getStringArray(R.array.operators_sering_name);
        if (mNetworkOperator != null) {
            values[0] = values[0] + ": " + mNetworkOperator;
        } else {
            values[0] = values[0] + ": NA";
        }
        if (mNetworkOperatorName != null) {
            values[1] = values[1] + ": " + mNetworkOperatorName;
        } else {
            values[1] = values[1] + ": NA";
        }
        if (isRoaming) {
            values[2] = values[2] + ": " + mContext.getResources().getString(R.string.is_roaming);
        } else {
            values[2] = values[2] + ": " + mContext.getResources().getString(R.string.non_roaming);
        }
        result+="\"";
        for(int i=0; i<values.length; i++){
            result+=values[i]+"\r\n";
        }

        values = mContext.getResources()
                .getStringArray(R.array.Lte_sering_name);
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,6,0", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,6,0: " + atResponse);

        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll(",-", ",+");
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            // 559502 modify by alisa.li 2016.05.05
            for (int i = 0; i < str2.length; i++) {
                if (i == 3 || i == 4) {
                    if (str2[i].contains(",")) {
                        values[i] = values[i] + ": ";
                        String[] str3 = str2[i].split(",");
                        int[] tempArray = new int[str3.length];
                        for (int j = 0; j < str3.length; j++) {
                            tempArray[j] = Integer.parseInt(str3[j].replace("+", "-").trim()) / 100;
                            values[i] = values[i] + String.valueOf(tempArray[j]) + "dBm ";
                        }
                    } else {
                        int temp = Integer.parseInt(str2[i].replace("+", "-").trim()) / 100;
                        values[i] = values[i] + ": "
                                + String.valueOf(temp) + "dBm";
                    }
                } else {
                    values[i] = values[i] + ": "
                            + str2[i].replace("+", "-");
                    if(i==1){
                        NetinfoRecordService.serverFreq[simid]=str2[i].replace("+", "-");
                    }
                }
            }
            //559502 modify by alisa.li 2016.05.05
        } else {
            for (int i = 0; i < 5; i++) {
                values[i] = values[i] + ": NA";
            }
        }
        int num = 5;
        atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,0,6", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,0,6: " + atResponse);

        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll(",-", ",+");
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < str2.length; i++) {
                if (!(str2[i].contains(","))) {
                    if (num < values.length) {
                        values[num] = values[num] + ": "
                                + str2[i].replace("+", "-");
                        num++;
                    }
                }
            }
            for (int i = num; i < values.length; i++) {
                values[i] = values[i] + ":NA";
            }
        } else {
            for (int i = 6; i < values.length; i++) {
                values[i] = values[i] + ": NA";
            }
        }
        for(int i=0; i<values.length; i++){
            result+=values[i]+"\r\n";
        }
        result+="\"";
        return result;
    }

    public String getNeighbourData(int simid){
        String result="";
        ROW = 5;
        COL = 4;
        mTextValue = new String[ROW][COL];
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,6,6", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,6,6: " + atResponse);

        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll(",-", ",+");
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < 5; i++) {
                if (i < str2.length) {
                    if (str2[i].contains(",")) {
                        String[] str3 = str2[i].split(",");
                        for (int j = 0; j < 4; j++) {
                            if (j == 2 || j == 3) {
                                int temp = Integer.parseInt(str3[j].replace("+", "-").trim())/100;
                                mTextValue[i][j] = String.valueOf(temp) + "dBm";
                            } else {
                                mTextValue[i][j] = str3[j]
                                        .replace("+", "-");
                            }
                        }
                    }
                } else {
                    for (int j = 0; j < 4; j++) {
                        mTextValue[i][j] = "NA";
                    }
                }
            }
        } else {
            for (int i = 0; i < ROW; i++) {
                for (int j = 0; j < COL; j++) {
                    mTextValue[i][j] = "NA";
                }
            }
        }

        result+="\"";
        String title="     "+mContext.getResources().getString(R.string.netinfo_frequency)+"  "+mContext.getResources().getString(R.string.netinfo_pci)+
                "  "+mContext.getResources().getString(R.string.netinfo_rsrp)+"  "+mContext.getResources().getString(R.string.netinfo_rsrq)+"\r\n";
        result+=title;

        for (int i = 0; i < ROW; i++) {
            result+=mContext.getResources().getString(cell_name[i])+"  ";
            for (int j = 0; j < COL; j++) {
                result+=mTextValue[i][j]+"     ";
            }
            result+="\r\n";
        }
        result+="\"";
        return result;
    }

    public String getAdjacent2GData(int simid){
        String result="";
        ROW = 5;
        COL = 3;
        mTextValue = new String[ROW][COL];
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,6,7", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,6,7: " + atResponse);

        int num = 0;
        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll(",-", ",+");
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < str2.length; i++) {
                if (num < 5) {
                    if (str2[i].contains(",")) {
                        String[] str3 = str2[i].split(",");
                        for (int j = 0; j < 3; j++) {
                            if (j == 2) {
                                int temp = Integer.parseInt(str3[j].replace("+", "-").trim())/100;
                                mTextValue[num][j] = String.valueOf(temp) + "dBm";
                            } else {
                                mTextValue[num][j] = str3[j].replace("+",
                                        "-");
                            }
                        }
                        num++;
                    }
                } else {
                    break;
                }
            }
            if (num < 5) {
                for (int i = num; i < 5; i++) {
                    for (int j = 0; j < 3; j++) {
                        mTextValue[i][j] = "NA";
                    }
                }
            }
        } else {
            for (int i = 0; i < ROW; i++) {
                for (int j = 0; j < COL; j++) {
                    mTextValue[i][j] = "NA";
                }
            }
        }

        result+="\"";
        String title="     "+mContext.getResources().getString(R.string.netinfo_frequency)+"  "+mContext.getResources().getString(R.string.netinfo_cpid)+
                "  "+mContext.getResources().getString(R.string.netinfo_signal_strength)+"\r\n";
        result+=title;

        for (int i = 0; i < ROW; i++) {
            result+=mContext.getResources().getString(cell_name[i])+"  ";
            for (int j = 0; j < COL; j++) {
                result+=mTextValue[i][j]+"     ";
            }
            result+="\r\n";
        }
        result+="\"";
        return result;
    }

    public String getAdjacent3GData(int simid){
        String result="";
        ROW = 5;
        COL = 3;
        mTextValue = new String[ROW][COL];
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,6,8", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,6,8: " + atResponse);
        int num_4g = 0;

        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll(",-", ",+");
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < str2.length; i++) {
                if (num_4g < 5) {
                    if (str2[i].contains(",")) {
                        String[] str3 = str2[i].split(",");
                        for (int j = 0; j < 3; j++) {
                            if (j == 2) {
                                int temp = Integer.parseInt(str3[j].replace("+", "-").trim())/100;
                                mTextValue[num_4g][j] = String.valueOf(temp) + "dBm";
                            } else {
                                mTextValue[num_4g][j] = str3[j].replace(
                                        "+", "-");
                            }
                        }
                        num_4g++;
                    }
                } else {
                    break;
                }
            }
            if (num_4g < 5) {
                for (int i = num_4g; i < 5; i++) {
                    for (int j = 0; j < 3; j++) {
                        mTextValue[i][j] = "NA";
                    }
                }
            }
        } else {
            for (int i = 0; i < ROW; i++) {
                for (int j = 0; j < COL; j++) {
                    mTextValue[i][j] = "NA";
                }
            }
        }

        result+="\"";
        String title="     "+mContext.getResources().getString(R.string.netinfo_frequency)+"  "+mContext.getResources().getString(R.string.netinfo_cpid)+
                "  "+mContext.getResources().getString(R.string.netinfo_signal_strength)+"\r\n";
        result+=title;

        for (int i = 0; i < ROW; i++) {
            result+=mContext.getResources().getString(cell_name[i])+"  ";
            for (int j = 0; j < COL; j++) {
                result+=mTextValue[i][j]+"     ";
            }
            result+="\r\n";
        }
        result+="\"";
        return result;
    }

    public String getOutfieldData(int simid){
        String result="";
        String[] values = mContext.getResources().getStringArray(
                R.array.Lte_Outfield_Network);
        int num = 0;
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,0,7", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,0,7: " + atResponse);

        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2;
            if (str1.length > 3) {
                str2 = str1[2].split("-");
            } else {
                str2 = str1[0].split("-");
            }
            for (int i = 0; i < str2.length; i++) {
                if (i == 10 || i == 4) {
                    if (num < values.length) {
                        values[num] = values[num]
                                + ": "
                                + Description_name[i][Integer
                                        .valueOf(str2[i].substring(0, 1))];
                        num++;
                    }
                }
            }
        } else {
            for (int i = 0; i < values.length; i++) {
                values[i] = values[i] + ": NA";
            }
        }

        result+="\"";
        for (int i = 0; i < values.length; i++) {
            result+=values[i]+"\r\n";
        }
        result+="\"";
        return result;
    }

    public int slotIdToSubId(int phoneId) {
        int subId;
        SubscriptionInfo mSubscriptionInfo = mSubscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(phoneId);
        if (mSubscriptionInfo == null) {
            Log.d(TAG,"mSubscriptionInfo is null");
            subId = SubscriptionManager.getDefaultSubscriptionId();
        } else {
            subId = mSubscriptionInfo.getSubscriptionId();
        }
        return subId;
     }
}
