package com.sprd.engineermode.telephony.userinfo;

import android.content.Context;
import com.sprd.engineermode.R;
import com.sprd.engineermode.utils.IATUtils;

import android.telephony.TelephonyManager;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionInfo;
import android.util.Log;

public class GsmShowRecord {
    private static final String TAG = "GsmShowRecord";
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

    public GsmShowRecord(Context context){
        mContext=context;
        mTelephonyManager = (TelephonyManager) TelephonyManager
                .from(mContext);
        mSubscriptionManager = (SubscriptionManager) SubscriptionManager
                .from(mContext);
    }

    private int[] activityLab = new int[] { R.string.netinfo_server_cell,
            R.string.netinfo_neighbour_cell, R.string.netinfo_adjacent_cell_3g,
            R.string.netinfo_adjacent_cell_4g,
            R.string.netinfo_outfield_information };

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

        String[] values2 = mContext.getResources()
                .getStringArray(R.array.Gsm_sering_name);
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,0,0", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,0,0: " + atResponse);
        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < str2.length; i++) {
                if (i == 0) {
                    values2[1] = values2[1] + ": "
                            + str2[i].replace("+", "-");
                    NetinfoRecordService.serverFreq[simid]=str2[i].replace("+", "-");
                }
                if (i == 1) {
                    values2[2] = values2[2] + ": "
                            + str2[i].replace("+", "-");
                }
                if (i == 2) {
                    values2[0] = values2[0] + ": "
                            + str2[i].replace("+", "-");
                }
                if (i == 3) {
                    values2[3] = values2[3] + ": "
                            + str2[i].replace("+", "-");
                }
                if (i == 4) {
                    int temp = Integer.parseInt(str2[i].replace("+", "-").trim()) - 110;
                    values2[4] = values2[4] + ": "
                            + String.valueOf(temp) + "dBm";
                }
            }
        } else {
            for (int i = 0; i < values2.length; i++) {
                values2[i] = values2[i] + ": NA";
            }
        }

        for(int i=0; i<values2.length; i++){
            result+=values2[i]+"\r\n";
        }
        result+="\"";
        return result;
    }

    public String getNeighbourData(int simid){
        String result="";
        ROW = 5;
        COL = 5;
        mTextValue = new String[ROW][COL];
        String response = IATUtils.sendATCmd("AT+SPENGMD=0,0,1", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,0,1: " + response);

        if (response.contains(IATUtils.AT_OK)) {
            response = response.replaceAll(",-", ",+");
            response = response.replaceAll("--", "-+");
            String[] str1 = response.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < str2.length; i++) {
                if (str2[i].contains(",")) {
                    String[] str3 = str2[i].split(",");
                    for (int j = 0; j < 5; j++) {
                        if (i == 0) {
                            if (j >= str3.length) {
                                mTextValue[j][1] = "NA";
                            } else {
                                mTextValue[j][1] = str3[j]
                                        .replace("+", "-");
                            }
                        }
                        if (i == 1) {
                            if (j >= str3.length) {
                                mTextValue[j][2] = "NA";
                            } else {
                                mTextValue[j][2] = str3[j]
                                        .replace("+", "-");
                            }
                        }
                        if (i == 2) {
                            if (j >= str3.length) {
                                mTextValue[j][0] = "NA";
                            } else {
                                mTextValue[j][0] = str3[j]
                                        .replace("+", "-");
                            }
                        }
                        if (i == 3) {
                            if (j >= str3.length) {
                                mTextValue[j][3] = "NA";
                            } else {
                                mTextValue[j][3] = str3[j]
                                        .replace("+", "-");
                            }
                        }
                        if (i == 4) {
                            if (j >= str3.length) {
                                mTextValue[j][4] = "NA";
                            } else {
                                int temp = Integer.parseInt(str3[j].replace("+", "-").trim()) - 110;
                                mTextValue[j][4] = String.valueOf(temp) + "dBm";
                            }
                        }
                    }
                } else {
                    for (int j = 0; j < 5; j++) {
                        if (i == 0) {
                            if (j > 0) {
                                mTextValue[j][1] = "NA";
                            } else {
                                mTextValue[j][1] = str2[i]
                                        .replace("+", "-");
                            }
                        }
                        if (i == 1) {
                            if (j > 0) {
                                mTextValue[j][2] = "NA";
                            } else {
                                mTextValue[j][2] = str2[i]
                                        .replace("+", "-");
                            }
                        }
                        if (i == 2) {
                            if (j > 0) {
                                mTextValue[j][0] = "NA";
                            } else {
                                mTextValue[j][0] = str2[i]
                                        .replace("+", "-");
                            }
                        }
                        if (i == 3) {
                            if (j > 0) {
                                mTextValue[j][3] = "NA";
                            } else {
                                mTextValue[j][3] = str2[i]
                                        .replace("+", "-");
                            }
                        }
                        if (i == 4) {
                            if (j > 0) {
                                mTextValue[j][4] = "NA";
                            } else {
                                int temp = Integer.parseInt(str2[i].replace("+", "-").trim()) - 110;
                                mTextValue[j][4] = String.valueOf(temp) + "dBm";
                            }
                        }
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
        String title="     "+mContext.getResources().getString(R.string.netinfo_cellid)+"  "+mContext.getResources().getString(R.string.netinfo_frequency)+
                "  "+mContext.getResources().getString(R.string.netinfo_bsic)+"  "+mContext.getResources().getString(R.string.netinfo_interference_value)+
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
        String response = IATUtils.sendATCmd("AT+SPENGMD=0,0,2", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,0,2: " + response);

        if (response.contains(IATUtils.AT_OK)) {
            response = response.replaceAll(",-", ",+");
            response = response.replaceAll("--", "-+");
            String[] str1 = response.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < str2.length; i++) {
                if (str2[i].contains(",")) {
                    String[] str3 = str2[i].split(",");
                    for (int j = 0; j < 5; j++) {
                        if (i < 3) {
                            if (j >= str3.length) {
                                if (i == 2) {
                                    mTextValue[j][i] = "NA";
                                } else {
                                    mTextValue[j][i] = "NA";
                                }
                            } else {
                                if (i == 2) {
                                    int temp = Integer.parseInt(str3[j].replace("+","-").trim()) - 116;
                                    mTextValue[j][i] = String.valueOf(temp) + "dBm";
                                } else {
                                    mTextValue[j][i] = str3[j].replace("+",
                                            "-");
                                }
                            }
                        }
                    }
                } else {
                    for (int j = 0; j < 5; j++) {
                        if (i < 3) {
                            if (j > 0) {
                                if (i == 2) {
                                    mTextValue[j][i] = "NA";
                                } else {
                                    mTextValue[j][i] = "NA";
                                }
                            } else {
                                if (i == 2) {
                                    int temp = Integer.parseInt(str2[i].replace("+","-").trim()) - 116;
                                    mTextValue[j][i] = String.valueOf(temp) + "dBm";
                                } else {
                                    mTextValue[j][i] = str2[i].replace("+",
                                            "-");
                                }
                            }
                        }
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
        String response = IATUtils.sendATCmd("AT+SPENGMD=0,0,3", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,0,3: " + response);

        if (response.contains(IATUtils.AT_OK)) {
            response = response.replaceAll(",-", ",+");
            response = response.replaceAll("--", "-+");
            String[] str1 = response.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < 4; i++) {
                if (str2[i].contains(",")) {
                    String[] str3 = str2[i].split(",");
                    for (int j = 0; j < 5; j++) {
                        if (i < 5) {
                            if (j >= str3.length) {
                                if (i == 2 || i == 3) {
                                    mTextValue[j][i] = "NA";
                                } else {
                                    mTextValue[j][i] = "NA";
                                }
                            } else {
                                if (i == 2 || i == 3) {
                                    int temp = Integer.parseInt(str3[j].replace("+","-").trim())/100;
                                    mTextValue[j][i] = String.valueOf(temp) + "dBm";
                                } else {
                                    mTextValue[j][i] = str3[j].replace("+",
                                            "-");
                                }
                            }
                        }
                    }
                } else {
                    for (int j = 0; j < 5; j++) {
                        if (i < 5) {
                            if (j > 0) {
                                if (i == 2 || i == 3) {
                                    mTextValue[j][i] = "NA";
                                } else {
                                    mTextValue[j][i] = "NA";
                                }
                            } else {
                                if (i == 2 || i == 3) {
                                    int temp = Integer.parseInt(str2[i].replace("+","-").trim())/100;
                                    mTextValue[j][i] = String.valueOf(temp) + "dBm";
                                } else {
                                    mTextValue[j][i] = str2[i].replace("+",
                                            "-");
                                }
                            }
                        }
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
        final String[] values;
        values = mContext.getResources().getStringArray(
                R.array.Gsm_Outfield_Network);
        int num = 0;
        String atResponse = IATUtils.sendATCmd("AT+SPENGMD=0,0,7", SaveName+simid);
        Log.d(TAG, "AT+SPENGMD=0,0,7: " + atResponse);

        if (atResponse.contains(IATUtils.AT_OK)) {
            atResponse = atResponse.replaceAll("--", "-+");
            String[] str1 = atResponse.split("\n");
            String[] str2 = str1[0].split("-");
            for (int i = 0; i < str2.length; i++) {
                if (i == 0 || i == 1 || i == 2 || i == 3 || i == 6
                        || i == 7 || i == 9) {
                    if (i == 3) {
                        String string = "";
                        int response = Integer
                                .valueOf(str2[i].substring(0));
                        int[] bite = new int[7];
                        bite[6] = response % 128 / 64;
                        bite[5] = response % 64 / 32;
                        bite[4] = response % 32 / 16;
                        bite[3] = response % 16 / 8;
                        bite[2] = response % 8 / 4;
                        bite[1] = response % 4 / 2;
                        bite[0] = response % 2;
                        for (int j = 0; j < 6; j++) {
                            if (bite[j] == 1) {
                                string = string + Description_name[3][j]
                                        + " ";
                            }
                        }
                        values[num] = values[num] + ": " + string;
                    } else {
                        values[num] = values[num]
                                + ": "
                                + Description_name[i][Integer
                                        .valueOf(str2[i].substring(0, 1))];
                    }
                    num++;
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

    // phoneId get subid
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
