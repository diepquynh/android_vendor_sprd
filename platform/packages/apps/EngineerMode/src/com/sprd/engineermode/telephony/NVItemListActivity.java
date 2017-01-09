package com.sprd.engineermode.telephony;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


import com.sprd.engineermode.R;
import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

import android.app.AlertDialog;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.widget.SimpleAdapter;

public class NVItemListActivity extends ListActivity {
    private static final String TAG = "NVItemList";
    private static final String ATFailRes = "AT cmd send fail";
    private static final String ATError = "ERROR";

    private static final String[] mItemNamesList = new String[] {
//        "[Common Features]",
        "Network mode",        // 0
        "GSM A5 algorithms",
        "Network preference",
        "Network select mode",
        "Network domain",
        "UMTS integrity",       // 5
        "UMTS ciphering",
        "GPRS/GEA algorithms",
        "DTM",
        "3G WB AMR",
//        "[Rel-7 and Rel-8 WCDMA Features]",
        "E-FACH",       // 10
        "E-RACH",
        "eDRX",
        "E-PCH",
        "MAC_I",
        "CPC",          // 15
        "E-FDPCH",
        "SRBoHS",
        "Fast dormancy",
        "DC-HSDPA",
        "DC-HSUPA",     // 20
        "Rx diversity",
        "HSUPA category",
        "WCDMA version",
        "Receiver type 3i",
//        "[2G Feature]",
        "VAMOS",       //25
//         [FDD Info]
        "LTE Category", // 查询 Category
        "LTE Band",  // 查询 支持的 Band 信息
        "VoLTE", // 查询是否支持VoLTE
        "SMS Bearer",//29
        "SMS retry interval",
        "AGPS QoS Timeout"
    };
    private List<Map<String, Object>> myData;

    private static final String AT_SYSCONFIG    = engconstents.ENG_AT_NETMODE1;// "AT^SYSCONFIG?"
    private static final String AT_COPS         = engconstents.ENG_AT_COPS;// "AT+COPS?"
    private static final String AT_SPENGMD007   = engconstents.ENG_AT_SPENGMD_007;// "AT+SPENGMD=0,0,7"
    private static final String AT_SPENGMD008   = engconstents.ENG_AT_SPENGMD_008;// "AT+SPENGMD=0,0,8"

    private static final String AT_LTE_CATEGORY   = engconstents.ENG_AT_LTE_CATEGORY;// "AT+SPUECAT=0"
    private static final String AT_LTE_BAND   = engconstents.ENG_AT_LTE_BAND;// "AT+SPBANDCTRL=0,5"  //只查询FDD LTE BAND
    private static final String AT_VOLTE_SUPPOERTED   = engconstents.ENG_AT_VOLTE_SUPPOERTED;// "AT+CAVIMS?"

    private static final String AT_SMS_BEARER   = engconstents.ENG_AT_SMS_GW_BEARER_PREF;// "AT+CGSMS?"
    private static final String AT_AGPS_QOS_TIME  = engconstents.ENG_AT_AGPS_DEFAULT_QOS_TIME;// "AT+SPTEST=26"

    private static final String SPLIT = "\n|: |,";
    private static final int LOAD_ITEMS = 1;
    private static final int REFRASH_SCREEN = 1;

    private BGHandler mBGHandler;
    private Handler mUiThread = new Handler();
    private ProgressDialog mProgressDialog;
    private int mPhoneCount;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = getIntent();
        mPhoneCount = TelephonyManager.from(this).getPhoneCount();
        myData = new ArrayList<Map<String, Object>>();

        HandlerThread thread = new HandlerThread(TAG);
        thread.start();
        mBGHandler = new BGHandler(thread.getLooper());

        setListAdapter(new SimpleAdapter(this, myData,
                android.R.layout.simple_list_item_1, new String[] { "title" },
                new int[] { android.R.id.text1 }));
        getListView().setTextFilterEnabled(true);
        mBGHandler.sendEmptyMessage(LOAD_ITEMS);
    }

    @Override
    protected void onDestroy() {
        if (mBGHandler != null) {
            mBGHandler.getLooper().quit();
        }
        super.onDestroy();
    }

    static final Map SYSCONFIG_MODE;
    static final Map SYSCONFIG_ACQORDER;
    static final Map SYSCONFIG_SRVDOMAIM;
    static final Map COPS_MODE;

//    static final Map SPENGMD007_UMTS_CIPHERING;
//    static final Map SPENGMD007_GPRS_ALGORITHMS;
    static final Map SPENGMD007_WB_AMR;
//    static final Map SPENGMD007_E_FACH;
    static final Map SPENGMD007_VAMOS;
    static final Map SPENGMD007_CPC;

    static final Map SPENGMD008_UMTS_INTEGRITY;
    static final Map SPENGMD008_DTM;
    static final Map SPENGMD008_ERACH;
    static final Map SPENGMD008_EDRX;
    static final Map SPENGMD008_EPCH;
    static final Map SPENGMD008_MAC_I;
    static final Map SPENGMD008_E_FDPCH;
    static final Map SPENGMD008_SRBOHS;
    //static final Map SPENGMD008_FAST_DORMANCY;
    static final Map SPENGMD008_RX_DIVERSITY;
    static final Map SPENGMD008_HSUPA_CATEGORY;
    static final Map SPENGMD008_WCDMA_VERSION;
    static final Map SPENGMD008_RECEIVER_TYPE;
    static final Map SPENGMD008_DC_HSDPA;
    static final Map SPENGMD008_DC_HSUPA;
    static final Map SPENGMD008_UMTS_CIPHER;
    static final Map SPENGMD008_UE_FACH;

    static final Map SMS_SERVICE_TYPE;

    static {
        /*
            mode  Description
            2   Auto Selection
            12  LTE ONLY
            13  GSM ONLY
            14  WCDMA ONLY
            15  TDSCDMA ONLY
            17  LTE NON Lte prefer
        */
        SYSCONFIG_MODE = new HashMap();      //Modified parameters according to fiona.huang on 0730
        SYSCONFIG_MODE.put("2", "Auto");
        SYSCONFIG_MODE.put("12", "4G only");
        SYSCONFIG_MODE.put("13", "2G only");
        SYSCONFIG_MODE.put("14", "3G only");
        SYSCONFIG_MODE.put("15", "3G only");
        SYSCONFIG_MODE.put("17", "4G preferred");

        /*
            acqorder    Description
            0   Auto
            1   GSM perferred to UTRAN
            2   UTRAN perferred to GSM
            3   No change
            4   GSM prefer
            5   LTE prefer
            6           2/3G prefer
        */
        SYSCONFIG_ACQORDER = new HashMap(); //Modified parameters according to fiona.huang on 0730
        SYSCONFIG_ACQORDER.put("0", "Auto");
        SYSCONFIG_ACQORDER.put("1", "2G preferred");
        SYSCONFIG_ACQORDER.put("2", "3G preferred");
        SYSCONFIG_ACQORDER.put("3", "4G");
        SYSCONFIG_ACQORDER.put("4", "2G preferred");
        SYSCONFIG_ACQORDER.put("5", "4G preferred");
        SYSCONFIG_ACQORDER.put("6", "2G/3G preferred");

        /*
            srvdomain   Description
            0   CS_ONLY
            1   PS_ONLY
            2   CS_PS
            3   ANY
            4   No change
         */
        SYSCONFIG_SRVDOMAIM = new HashMap();
        SYSCONFIG_SRVDOMAIM.put("0", "CS_only");
        SYSCONFIG_SRVDOMAIM.put("1", "PS_only");
        SYSCONFIG_SRVDOMAIM.put("2", "CS and PS");
        SYSCONFIG_SRVDOMAIM.put("3", "Any");
//        SYSCONFIG_SRVDOMAIM.put("4", "No change");

        //////////////////////////////////////////////////////////////////
        /*
            mode    Description
            0   Select automatically (<oper> is ignored)
            1   Select manually (<oper> field shall be present, and <AcT> optionally)
            2   Deregister from network
            3   set only <format> (for read command +COPS?), do not attempt registration/deregistration (<oper> and < AcT> fields are ignored); this value is not applicable in read command response
            4   manual/automatic (<oper> field shall be present); if manual selection fails, automatic mode (<mode>=0) is entered
        */
        COPS_MODE = new HashMap();
        COPS_MODE.put("0", "Auto");
        COPS_MODE.put("1", "Manual");
//        COPS_MODE.put("2", "Deregister from network");
//        COPS_MODE.put("3", "Set only");
//        COPS_MODE.put("4", "Manual/Auto");

        //////////////////////////////////////////////////////////////////
        /*
            0 AMR WB：0  Not Support；  1  Support   
            1 BSS_PAGING_COORDINATION:  0  Not Support；  1  Support   
            2 GPRS Encryption Algorithm:  0  Unknown;  1 GEA1 ;   2  GEA2 ;  3  GEA3  
            3 GSM Cipher with Algorithm:  uint8  bit0-bit6分别对应 (A51- A57) 
            4 UMTS Cipher:  0  Unknown;  1  UEA0;  2  UEA1  
            5 Hsdpa/Hsupa: 0 Not support Hsdpa and Hsupa; 1 Support Hsdpa;   2  Support Hsupa;  3  Support Hsdpa and Hsupa;
            6 CBS:  0  Not support;  1  support  
            7 VAMOS: 0 Not support;  1  VAMOS1;   2  VAMOS2  
            8 Efach:  0 Not support; 1  support  
            9 Repeat Sacch Facch:  0 Not support; 1  support   
            10 R8/R9:   0 others;   1  R8;  2 R9   
            11 Less mode:  0 Not support;  1  support      
            12 4G_Cipher: 0  Unknown;  1 eea0 ;  2 eea1 ;  3 eea2    
            13 4G_Integerity: 0 Unknown;  1 eia0 ;  2 eia1 ;  3 eia2  
            14 CPC: 0 Not support; 1  support
         */
//        SPENGMD007_UMTS_CIPHERING = new HashMap();
//        SPENGMD007_UMTS_CIPHERING.put("0", "Unknown");
//        SPENGMD007_UMTS_CIPHERING.put("1", "UEA0");
//        SPENGMD007_UMTS_CIPHERING.put("2", "UEA1");

//        SPENGMD007_GPRS_ALGORITHMS = new HashMap();
//        SPENGMD007_GPRS_ALGORITHMS.put("0", "Unknown");
//        SPENGMD007_GPRS_ALGORITHMS.put("1", "GEA1");
//        SPENGMD007_GPRS_ALGORITHMS.put("2", "GEA2");
//        SPENGMD007_GPRS_ALGORITHMS.put("3", "GEA3");

        SPENGMD007_WB_AMR = new HashMap();
        SPENGMD007_WB_AMR.put("0", "Not support");
        SPENGMD007_WB_AMR.put("1", "Support");

//        SPENGMD007_E_FACH = new HashMap();
//        SPENGMD007_E_FACH.put("0", "Not support");
//        SPENGMD007_E_FACH.put("1", "Support");

        SPENGMD007_VAMOS = new HashMap();
        SPENGMD007_VAMOS.put("0", "Not support");// surprise
//        SPENGMD007_VAMOS.put("1", "VAMOS1 supported");
//        SPENGMD007_VAMOS.put("2", "VAMOS2 supported");
        SPENGMD007_VAMOS.put("1", "Support");
        SPENGMD007_VAMOS.put("2", "Support");

        SPENGMD007_CPC = new HashMap();
        SPENGMD007_CPC.put("0", "Not support");
        SPENGMD007_CPC.put("1", "Support");
        //////////////////////////////////////////////////////////////////
        /*
            0 UMTS integrity: 0  Not support; 1  UEA0; 2  UEA1; 3  UEA0 and UEA1
            1 DTM: 0  Not support ; 1  support
            2 ERACH: 0  Not support ; 1  support
            3 eDRX: 0  Not support ; 1  support
            4 EPCH: 0  Not support ; 1  support
            5 MAC_I: 0  Not support ; 1  support
            6 E-FDPCH: 0  Not support ; 1  support
            7 SRBoHS: 0  Not support ; 1  support
            8 Fast Dormancy: 255  Not support ; Other values  support
            9 Rx Diversity: 0  Not support ; 1  support
            10 HSUPA Category: 5  Category 5; 6  Category 6
            11 WCDMA Version: 0  rel_4; 1  rel_5; 2  rel_6; 3  rel_7
            12 Receiver Type 3i: 0  Not support ; 1  support
            13 DC_HSDPA: 0  Not support ; 1  support
            14 DC_HSUPA: 0  Not support ; 1  support
            15 gea_encryption_algo1: 0  Not support ; 1  support
            16 gea_algo2: 0  Not support ; 1  support
            17 gea_algo3: 0  Not support ; 1  support
            18 gea_algo4: 0  Not support ; 1  support
            19 gea_algo5: 0  Not support ; 1  support
            20 gea_algo6: 0  Not support ; 1  support
            21 gea_algo7: 0  Not support ; 1  support
            22 UMTS Cipher capability: 1 UEA0; 2 UEA1; 3 UEA0和UEA1
         */
        SPENGMD008_UMTS_INTEGRITY = new HashMap();
        SPENGMD008_UMTS_INTEGRITY.put("0", "Not support");
        SPENGMD008_UMTS_INTEGRITY.put("1", "UIA0");
        SPENGMD008_UMTS_INTEGRITY.put("2", "UIA1");
        SPENGMD008_UMTS_INTEGRITY.put("3", "UIA0, UIA1");
        SPENGMD008_UMTS_INTEGRITY.put("6", "UIA1, UIA2");

        SPENGMD008_DTM = new HashMap();
        SPENGMD008_DTM.put("0", "Not support");
        SPENGMD008_DTM.put("1", "Support");

        SPENGMD008_ERACH = new HashMap();
        SPENGMD008_ERACH.put("0", "Not support");
        SPENGMD008_ERACH.put("1", "Support");

        SPENGMD008_EDRX = new HashMap();
        SPENGMD008_EDRX.put("0", "Not support");
        SPENGMD008_EDRX.put("1", "Support");

        SPENGMD008_EPCH = new HashMap();
        SPENGMD008_EPCH.put("0", "Not support");
        SPENGMD008_EPCH.put("1", "Support");

        SPENGMD008_MAC_I = new HashMap();
        SPENGMD008_MAC_I.put("0", "Not support");
        SPENGMD008_MAC_I.put("1", "Support");

        SPENGMD008_E_FDPCH = new HashMap();
        SPENGMD008_E_FDPCH.put("0", "Not support");
        SPENGMD008_E_FDPCH.put("1", "Support");

        SPENGMD008_SRBOHS = new HashMap();
        SPENGMD008_SRBOHS.put("0", "Not support");
        SPENGMD008_SRBOHS.put("1", "Support");

        SPENGMD008_RX_DIVERSITY = new HashMap();
        SPENGMD008_RX_DIVERSITY.put("0", "Not support");
        SPENGMD008_RX_DIVERSITY.put("1", "Support");

        SPENGMD008_HSUPA_CATEGORY = new HashMap();
        SPENGMD008_HSUPA_CATEGORY.put("5", "Cat 5");
        SPENGMD008_HSUPA_CATEGORY.put("6", "Cat 6");

        SPENGMD008_WCDMA_VERSION = new HashMap();
        SPENGMD008_WCDMA_VERSION.put("0", "R4");
        SPENGMD008_WCDMA_VERSION.put("1", "R5");
        SPENGMD008_WCDMA_VERSION.put("2", "R6");
        SPENGMD008_WCDMA_VERSION.put("3", "R7");
        //begin bug566242 add by suyan.yang 2016-05-27
        SPENGMD008_WCDMA_VERSION.put("4", "R8");
        SPENGMD008_WCDMA_VERSION.put("5", "R9");
        //end bug566242 add by suyan.yang 2016-05-27

        SPENGMD008_RECEIVER_TYPE = new HashMap();
        SPENGMD008_RECEIVER_TYPE.put("0", "Not support");
        SPENGMD008_RECEIVER_TYPE.put("1", "Support");

        SPENGMD008_DC_HSDPA = new HashMap();
        SPENGMD008_DC_HSDPA.put("0", "Not support");
        SPENGMD008_DC_HSDPA.put("1", "Support");

        SPENGMD008_DC_HSUPA = new HashMap();
        SPENGMD008_DC_HSUPA.put("0", "Not support");
        SPENGMD008_DC_HSUPA.put("1", "Support");

        SPENGMD008_UMTS_CIPHER = new HashMap();
        SPENGMD008_UMTS_CIPHER.put("1", "UEA0");
        SPENGMD008_UMTS_CIPHER.put("2", "UEA1");
        SPENGMD008_UMTS_CIPHER.put("3", "UEA0,UEA1");
        SPENGMD008_UMTS_CIPHER.put("7", "UEA0,UEA1,UEA2");

        SPENGMD008_UE_FACH = new HashMap();
        SPENGMD008_UE_FACH.put("0", "Not support");
        SPENGMD008_UE_FACH.put("1", "Support");

//////////////////////////////////////////////////////////////////
/*
            MN_SMS_PS_DOMAIN_SERVICE = 0,  //PS
            MN_SMS_CS_DOMAIN_SERVICE = 1,    // CS
            MN_SMS_PS_DOMAIN_SERVICE_PREF = 2,  //PS PREFER
            MN_SMS_CS_DOMAIN_SERVICE_PREF = 3,  //CS PREFER
            MN_SMS_NO_DOMAIN_SERVICE = 4
 */
        SMS_SERVICE_TYPE = new HashMap();
        SMS_SERVICE_TYPE.put("0", "PS");
        SMS_SERVICE_TYPE.put("1", "CS");
        SMS_SERVICE_TYPE.put("2", "PS preferred");
        SMS_SERVICE_TYPE.put("3", "CS preferred");
        SMS_SERVICE_TYPE.put("4", "No service");

    }

    private void processSysConfig() {
        String outstr = "";
        String atresult = AT_CMD(AT_SYSCONFIG, 0);
        String[] sysconfig = atresult.split(SPLIT);
        String atresult1 = AT_CMD(AT_SYSCONFIG, 1);
        String[] sysconfig1 = atresult1.split(SPLIT);
        if ( sysconfig.length < 5 || sysconfig1.length < 5 ) {
            addItem("Parse error", AT_SYSCONFIG);
            return;
        }

        Log.d(TAG, "sysconfig:"+Arrays.toString(sysconfig));
        Log.d(TAG, "sysconfig1:"+Arrays.toString(sysconfig1));
        if ( mPhoneCount > 1 ) {
            outstr = "\nSIM1("+getval(SYSCONFIG_MODE,sysconfig[1]) + "), \nSIM2("+getval(SYSCONFIG_MODE,sysconfig1[1])+")";
        } else {
            outstr = getval(SYSCONFIG_MODE,sysconfig[1]);
        }
        addItem(mItemNamesList[0], outstr);
        if ( mPhoneCount > 1 ) {
            outstr = "\nSIM1("+getval(SYSCONFIG_ACQORDER,sysconfig[2]) + "), \nSIM2("+getval(SYSCONFIG_ACQORDER,sysconfig1[2])+")";
        } else {
            outstr = getval(SYSCONFIG_ACQORDER,sysconfig[2]);
        }
        addItem(mItemNamesList[2], outstr);
        if ( mPhoneCount > 1 ) {
            outstr = "\nSIM1("+getval(SYSCONFIG_SRVDOMAIM,sysconfig[4])+ "), \nSIM2("+getval(SYSCONFIG_SRVDOMAIM,sysconfig1[4])+")";
        } else {
            outstr = getval(SYSCONFIG_SRVDOMAIM,sysconfig[4]);
        }
        addItem(mItemNamesList[4], outstr);
    }

    private void processCops() {
        String outstr = "";
        String atresult = AT_CMD(AT_COPS, 0);
        String[] cops = atresult.split(SPLIT);
        String atresult1 = AT_CMD(AT_COPS, 1);
        String[] cops1 = atresult1.split(SPLIT);
        if ( cops.length < 2 || cops1.length < 2 ) {
            addItem("Parse error", AT_COPS);
            return;
        }

        Log.d(TAG, "cops:"+Arrays.toString(cops));
        Log.d(TAG, "cops1:"+Arrays.toString(cops1));
        if ( mPhoneCount > 1 ) {
            outstr = "\nSIM1("+getval(COPS_MODE,cops[1])+ "), \nSIM2("+getval(COPS_MODE,cops1[1])+")";
        } else {
            outstr = getval(COPS_MODE,cops[1]);
        }
        addItem(mItemNamesList[3], outstr);
    }

    private void processSpengmd007() {
        String atresult = AT_CMD(AT_SPENGMD007, 0);
        String[] spengmd = atresult.split("-|\n");
        if ( spengmd.length < 15) {
            addItem("Parse error", AT_SPENGMD007);
            return;
        }

        Log.d(TAG, "spengmd:"+Arrays.toString(spengmd));
//        addItem(mItemNamesList[6], getval(SPENGMD007_UMTS_CIPHERING,spengmd[4]));
//        addItem(mItemNamesList[7], getval(SPENGMD007_GPRS_ALGORITHMS,spengmd[2]));
        addItem(mItemNamesList[9], getval(SPENGMD007_WB_AMR,spengmd[0]));
        addItem(mItemNamesList[25], getval(SPENGMD007_VAMOS,spengmd[7]));
        addItem(mItemNamesList[15], getval(SPENGMD007_CPC,spengmd[14]));

        String tmp = "";
        int n = Integer.parseInt(spengmd[3]);
        for (int i = 0; i < 7; i++ ) {
            if ( ((n >> i) & 0x01) == 1 ) {
                if ( !tmp.isEmpty() ) { tmp += ",";}
                tmp += "a5/"+(i+1);
            }
        }
        addItem(mItemNamesList[1], tmp);

//        switch( Integer.parseInt(spengmd[5]) ) {
//            case 0:
//                addItem(mItemNamesList[19], "Not Support");
//                addItem(mItemNamesList[20], "Not Support");
//                break;
//            case 1:
//                addItem(mItemNamesList[19], "Support");
//                addItem(mItemNamesList[20], "Not Support");
//                break;
//            case 2:
//                addItem(mItemNamesList[19], "Not Support");
//                addItem(mItemNamesList[20], "Support");
//                break;
//            case 3:
//                addItem(mItemNamesList[19], "Support");
//                addItem(mItemNamesList[20], "Support");
//                break;
//        }
    }

    private void processSpengmd008() {
        String atresult = AT_CMD(AT_SPENGMD008, 0);
        //atresult = "3-1-0-1-0-1-0-1-200-1-6-3-1-1-1-1-1-1-1-1-1-1-3-1";
        String[] spengmd = atresult.split("-|\n|,");
        Log.d(TAG, "spengmd:"+Arrays.toString(spengmd));
        if ( spengmd.length < 22) {
            addItem("Parse error", AT_SPENGMD008);
            return;
        }
        addItem(mItemNamesList[5], getval(SPENGMD008_UMTS_INTEGRITY,spengmd[0]));
        addItem(mItemNamesList[8], getval(SPENGMD008_DTM,spengmd[1]));
        addItem(mItemNamesList[11], getval(SPENGMD008_ERACH,spengmd[2]));
        addItem(mItemNamesList[12], getval(SPENGMD008_EDRX,spengmd[3]));
        addItem(mItemNamesList[13], getval(SPENGMD008_EPCH,spengmd[4]));
        addItem(mItemNamesList[14], getval(SPENGMD008_MAC_I,spengmd[5]));
        addItem(mItemNamesList[16], getval(SPENGMD008_E_FDPCH,spengmd[6]));
        addItem(mItemNamesList[17], getval(SPENGMD008_SRBOHS,spengmd[7]));

        if ( spengmd[8].equals("255")) {
            addItem(mItemNamesList[18], "Not support");
        } else {
            addItem(mItemNamesList[18], "Support");
        }

        addItem(mItemNamesList[21], getval(SPENGMD008_RX_DIVERSITY,spengmd[9]));
        addItem(mItemNamesList[22], getval(SPENGMD008_HSUPA_CATEGORY,spengmd[10]));
        addItem(mItemNamesList[23], getval(SPENGMD008_WCDMA_VERSION,spengmd[11]));
        addItem(mItemNamesList[24], getval(SPENGMD008_RECEIVER_TYPE,spengmd[12]));
        addItem(mItemNamesList[19], getval(SPENGMD008_DC_HSDPA,spengmd[13]));
        addItem(mItemNamesList[20], getval(SPENGMD008_DC_HSUPA,spengmd[14]));
        String tmp = "";
        int n = Integer.parseInt(spengmd[3]);
        for (int i = 15; i <= 21; i++ ) {
            if ( spengmd[i].equals("1") ) {
                if ( !tmp.isEmpty() ) { tmp += ",";}
                tmp += "GEA"+(i-14);
            }
        }
        addItem(mItemNamesList[7], tmp);
        addItem(mItemNamesList[6], getval(SPENGMD008_UMTS_CIPHER,spengmd[23]));
        addItem(mItemNamesList[10], getval(SPENGMD008_UE_FACH,spengmd[24]));
    }
    
    private void processSpengmdltecategory(){
    	 /* category value
 	     +SPUECAT：N  (N:1-15)
 	     OK
 	    */	
        String atresult = AT_CMD(AT_LTE_CATEGORY, 0);
        Log.d(TAG,"atresult check AT_LTE_CATEGORY: " + atresult);
        if (atresult.isEmpty()){     //check the return value
        	addItem("Parse error", AT_LTE_CATEGORY);
        	return;
        }

        // add code to handle fail-return based on non-LTE platform(7731)
        if (atresult.contains(ATFailRes)){
            addItem(mItemNamesList[26],"UNKNOWN");
            return;
        }
        if (atresult.contains(ATError)){
            addItem(mItemNamesList[26],"not support");
            return;
        }
        String[] temp = atresult.split("\n");
        String[] volte_category = temp[0].split(":");
        if ( volte_category.length < 2 ) {
            addItem("Parse error", AT_LTE_CATEGORY);
            return;
        }
        Log.d(TAG, "volte_category:"+Arrays.toString(volte_category));
        addItem(mItemNamesList[26], volte_category[1]);
    }

    private void processSpengmdlteband(){
    	/* VOLTE band value,（the value need to be check？？0730）
 	      +SPBANDCTRL：5,5  //+SPBANDCTRL：5 means FDD LTE is not supported,5 means FDD LTE
 	      OK  +SPBANDCTRL: 5,3,7,20
 	    */
    	String outstr = "";
        String atresult = AT_CMD(AT_LTE_BAND, 0);
        Log.d(TAG,"atresult check AT_LTE_BAND: " + atresult);
        if (atresult == null){    //check the return value
        	addItem("Parse error", AT_LTE_BAND);
        	return;
        }

        // add code to handle fail-return based on non-LTE platform(7731)
        if (atresult.contains(ATFailRes)){
            addItem(mItemNamesList[27],"UNKNOWN");
            return;
        }

        if (atresult.contains(ATError)) {
            addItem(mItemNamesList[27], "Not support");
            return;
        }

        String[] temp = atresult.split("\n");
        String[] volte_band = temp[0].split(":");
        if(volte_band.length < 2){
            return;
        }
        String[] volte_band1 = volte_band[1].split(",");
        int n = volte_band1.length;
        Log.d(TAG,"volte_band1" + volte_band1[0] + "volte_band1.length" + n);
        String tmp = "";
        if ( (!("5".equals(volte_band1[0].trim()))) ||  (n < 1) ){
        	addItem("Parse error", AT_LTE_BAND);
            return;
        }else if(n == 1){
        	addItem(mItemNamesList[27], "Not support");
        }else{
            for (int i = 1; i < n; i++ ) {
                if ( !volte_band1[i].isEmpty() ) {
                    if ( !tmp.isEmpty() ) { tmp += ", ";}
                    tmp += volte_band1[i];
                }
            }
         addItem(mItemNamesList[27], tmp);
        }
    }


    private void processSpengmdvolte(){
        /* VOLTE value  if one card return value is 1,volte is supported by the phone
        +CAVIMS：0/1
        OK
        */
        String atresult = AT_CMD(AT_VOLTE_SUPPOERTED, 0);
        Log.d(TAG,"atresult check AT_VOLTE_SUPPOERTED: " + atresult);
        if (atresult.isEmpty()){     //check the return value
        	addItem("Parse error", AT_VOLTE_SUPPOERTED);
        	return;
        }

        // add code to handle fail-return based on non-LTE platform(7731)
        if (atresult.contains(ATFailRes)){
            addItem(mItemNamesList[28],"UNKNOWN");
            return;
        }

        String[] temp = atresult.split("\n");
        String[] volte_supported = temp[0].split(":");
        
        String atresult1 = AT_CMD(AT_VOLTE_SUPPOERTED, 1);
        if (atresult.isEmpty()){      //check the return value
        	addItem("Parse error", AT_VOLTE_SUPPOERTED);
        	return;
        }
        String[] temp1 = atresult.split("\n");
        String[] volte_supported1 = temp[0].split(":");        
        
        if ( volte_supported.length < 2 || volte_supported1.length < 2 ) {
            addItem("Parse error", AT_VOLTE_SUPPOERTED);
            return;
        }

        Log.d(TAG, "volte_supported:"+Arrays.toString(volte_supported));
        Log.d(TAG, "volte_supported1:"+Arrays.toString(volte_supported1));

        if ( (volte_supported[1].contains("1"))||(volte_supported1[1].contains("1"))) {
            addItem(mItemNamesList[28], "Support");
        } else {
            addItem(mItemNamesList[28], "Not support");
        }

    }

    private void processSMSBearer(){
        //+CGSMS: value    //+CGSMS: 3
        String atresult = AT_CMD(AT_SMS_BEARER, 0);
        Log.d(TAG,"atresult check AT_SMS_BEARER: " + atresult);
        if (atresult.isEmpty()){     //check the return value
            addItem("Parse error", AT_SMS_BEARER);
            return;
        }
        if (atresult.contains(ATFailRes)) {
            addItem(mItemNamesList[29], "UNKNOWN");
            return;
        }
        String[] temp = atresult.split("\n");
        String[] temp1 = temp[0].split(":");
        String sms_bearer = temp1[1].trim();

        Log.d(TAG, "sms_bearer: "+sms_bearer);

        Log.d(TAG, "getval:"+getval(SMS_SERVICE_TYPE,sms_bearer));
        addItem(mItemNamesList[29], getval(SMS_SERVICE_TYPE,sms_bearer));
    }

    private void processSMSBRetryInterval(){
        addItem(mItemNamesList[30],"0"+ " ms");
    }

    private void processAGPSQosTime(){
        //+SPTEST: 26,time   //ms  //+SPTEST: 26,140
        Log.d(TAG, " processAGPSQosTime()");
        String atresult = AT_CMD(AT_AGPS_QOS_TIME, 0);
        Log.d(TAG,"atresult check AT_AGPS_QOS_TIME: " + atresult);
        if (atresult.isEmpty()){     //check the return value
            addItem("Parse error",AT_AGPS_QOS_TIME);
            return;
        }

        // add code to handle fail-return based on non-LTE platform(7731)
        if (atresult.contains(ATFailRes)){
            addItem(mItemNamesList[31],"UNKNOWN");
            return;
        }

        String[] temp = atresult.split("\n");
        String[] temp1 = temp[0].split(":");
        String[] agps_qostime = temp1[1].split(",");
        Log.d(TAG, "agps_qostime[1]: "+agps_qostime[1]);
        addItem(mItemNamesList[31],agps_qostime[1]+ " s");
    }

    private String getval(Map map, String key) {
        if (map == SPENGMD008_HSUPA_CATEGORY) {
            return "Cat " + key;
        }
        String val = (String)map.get(key.trim());
        if ( val == null ) {
            val = "Unknow";
     }
        return val;
    }

    protected void addItem(String item, String value) {
        Map<String, Object> temp = new HashMap<String, Object>();
        temp.put("title", item+": "+value);
        myData.add(temp);
    }

    private String AT_CMD(String cmd, int channelid) {
        String res = IATUtils.sendATCmd(cmd, "atchannel"+channelid);
        Log.d(TAG, "AT_CMD["+channelid+"] >> "+cmd);
        Log.d(TAG, "AT_CMD["+channelid+"] << "+res);
        return res;
    }


    private void showProgressDialog() {
        mUiThread.post(new Runnable() {
            @Override
            public void run() {
                mProgressDialog = ProgressDialog.show(
                        NVItemListActivity.this, "Loading...",
                        "Please wait...", true, false);
            }
        });
    }

    private void dismissProgressDialog() {
        mUiThread.post(new Runnable() {
            @Override
            public void run() {
                if (mProgressDialog != null && mProgressDialog.isShowing()) {
                    mProgressDialog.dismiss();
                }
            }
        });
    }
    private void showErrorDialog(String message) {
        AlertDialog alertDialog = new AlertDialog.Builder(
                NVItemListActivity.this)
                .setTitle(TAG)
                .setMessage(message)
                .setPositiveButton(R.string.alertdialog_cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(
                                    DialogInterface dialog,
                                    int which) {
                            }
                        }).create();
        alertDialog.show();
    }

    class BGHandler extends Handler {
        public BGHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            String values = null;
            String response;
            String atCmd;
            String dialogMessage;
            Log.d(TAG, "handleMessage:"+msg.what);
            switch (msg.what) {
                case LOAD_ITEMS:
//                    showProgressDialog();
//                    processSysConfig();
//                    processCops();
//                    processSpengmd007();
//                    processSpengmd008();
//                    dismissProgressDialog();

                    mUiThread.post(new Runnable() {
                        @Override
                        public void run() {
                            processSysConfig();
                            processCops();
                            processSpengmd007();
                            processSpengmd008();
                            processSpengmdltecategory();
                            processSpengmdlteband();
                            processSpengmdvolte();
                            processSMSBearer();
                            processSMSBRetryInterval();
                            processAGPSQosTime();

                            setListAdapter(new SimpleAdapter(NVItemListActivity.this, myData,
                                    android.R.layout.simple_list_item_1, new String[] { "title" },
                                    new int[] { android.R.id.text1 }));
                            getListView().setTextFilterEnabled(true);
                        }
                    });
                    break;
            }
        }
    }
}
