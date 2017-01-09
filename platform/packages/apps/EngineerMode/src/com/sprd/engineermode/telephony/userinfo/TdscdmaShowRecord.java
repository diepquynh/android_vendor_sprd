package com.sprd.engineermode.telephony.userinfo;

import android.content.Context;
import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.IATUtils;

import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;
import android.util.Log;

public class TdscdmaShowRecord {

    private static final String TAG = "TdscdmaShowRecord";
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

    public TdscdmaShowRecord(Context context){
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

        values = mContext.getResources().getStringArray(
                R.array.Tdscdma_sering_name);
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,6,1", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,6,1: " + atResponse);
        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll(",-", ",+");
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < str2.length; i++) {
                if (i == 0) {
                    values[2] = values[2] + ": "
                            + str2[i].replace("+", "-");
                    NetinfoRecordService.serverFreq[simid]=str2[i].replace("+", "-");
                }
                if (i == 1) {
                    values[3] = values[3] + ": "
                            + str2[i].replace("+", "-");
                }
                if (i == 2) {
                    int temp = Integer.parseInt(str2[i].replace("+", "-").trim()) - 116;
                    values[5] = values[5] + ": "
                            + String.valueOf(temp) + "dBm";
                }
                if (i == 15) {
                    values[4] = values[4] + ": "
                            + str2[i].replace("+", "-");
                }
                if (i == 16) {
                    values[0] = values[0] + ": "
                            + str2[i].replace("+", "-");
                }
                if (i == 17) {
                    values[1] = values[1] + ": "
                            + str2[i].replace("+", "-");
                }
            }
        } else {
            for (int i = 0; i < 6; i++) {
                values[i] = values[i] + ": NA";
            }
        }
        int num = 6;
        atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,0,5", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,0,5: " + atResponse);
        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll(",-", ",+");
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            if ("".equals(str2[0])) {
                for (int i = 1; i < str2.length; i++) {
                    if (i == 1) {
                        if (str2[1].contains(",")) {
                            String[] str3 = str2[1].split(",");
                            for (int j = 0; j < 3; j++) {
                                if (j==0) {
                                    values[num] = values[num] + ": "
                                            + "-" + str3[0];
                                } else {
                                    values[num] = values[num] + ": "
                                            + str3[j].replace("+", "-");
                                }
                                num++;
                            }
                        } else {
                            for (int j = 0; j < 3; j++) {
                                values[num] = values[num] + ": "
                                        + str2[1].replace("+", "-");
                                num++;
                            }
                        }
                    } else {
                        if (num < values.length) {
                            values[num] = values[num] + ": "
                                    + str2[i].replace("+", "-");
                            num++;
                        }
                    }
                }
            } else {
                for (int i = 0; i < str2.length; i++) {
                    if (i == 0) {
                        if (str2[0].contains(",")) {
                            String[] str3 = str2[0].split(",");
                            for (int j = 0; j < 3; j++) {
                                values[num] = values[num] + ": "
                                        + str3[j].replace("+", "-");
                                num++;
                            }
                        } else {
                            for (int j = 0; j < 3; j++) {
                                values[num] = values[num] + ": "
                                        + str2[0].replace("+", "-");
                                num++;
                            }
                        }
                    } else {
                        if (num < values.length) {
                            values[num] = values[num] + ": "
                                    + str2[i].replace("+", "-");
                            num++;
                        }
                    }
                }
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
        COL = 3;
        mTextValue = new String[ROW][COL];
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,6,2", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,6,2: " + atResponse);

        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll(",-", ",+");
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < 5; i++) {
                if (i < str2.length) {
                    if (str2[i].contains(",")) {
                        String[] str3 = str2[i].split(",");
                        for (int j = 0; j < 3; j++) {
                            if (j == 2) {
                                int temp = Integer.parseInt(str3[j].replace("+", "-").trim()) - 116;
                                mTextValue[i][j] = String.valueOf(temp) + "dBm";
                            } else {
                                mTextValue[i][j] = str3[j]
                                        .replace("+", "-");
                            }
                        }
                    }
                } else {
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

    public String getAdjacent2GData(int simid){
        String result="";
        ROW = 5;
        COL = 3;
        mTextValue = new String[ROW][COL];
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,6,4", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,6,4: " + atResponse);

        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll(",-", ",+");
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < 5; i++) {
                if (i < (str2.length - 3)) {
                    if (str2[i + 3].contains(",")) {
                        String[] str3 = str2[i + 3].split(",");
                        for (int j = 1; j < 4; j++) {
                            if (j == 3) {
                                int temp = Integer.parseInt(str3[j].replace("+", "-").trim()) - 111;
                                mTextValue[i][j - 1] = String.valueOf(temp) + "dBm";
                            } else {
                                mTextValue[i][j - 1] = str3[j].replace("+",
                                        "-");
                            }
                        }
                    }
                } else {
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

    public String getAdjacent4GData(int simid){
        String result="";
        ROW = 5;
        COL = 4;
        mTextValue = new String[ROW][COL];
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,0,4", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,0,4: " + atResponse);
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
                        for (int j = 0; j < 4; j++) {
                            if (j == 3 || j == 2) {
                                int temp;
                                if(j == 2) {
                                    temp = Integer.parseInt(str3[j].replace("+", "-").trim()) - 141;
                                } else {
                                    temp = (Integer.parseInt(str3[j].replace("+", "-").trim()) - 40)/2;
                                }
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

    public String getOutfieldData(int simid){
        String result="";
        String values[] = mContext.getResources().getStringArray(
                R.array.Tdscdma_Outfield_Network);
        int num = 0;
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,0,7", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,0,7: " + atResponse);
        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < str2.length; i++) {
                if (i == 4 || i == 5 || i == 10) {
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
