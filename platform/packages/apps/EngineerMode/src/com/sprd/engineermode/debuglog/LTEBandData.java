package com.sprd.engineermode.debuglog;

import java.util.ArrayList;

import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.CheckBoxPreference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceManager;
import android.util.Log;

public class LTEBandData{
    private static final String TAG = "LTDBandData";
    private final static String ORI_LTE_BAND_TDD = "ORI_LTE_BAND_TDD";
    private final static String ORI_LTE_BAND_FDD = "ORI_LTE_BAND_FDD";
    public static final String KEY_TDD_LTE = "TDD_BAND";
    public static final String KEY_FDD_LTE = "FDD_BAND";
    private int mLTESupportBand_TDD = -1;
    private int mLTESupportBand_FDD = -1;
    private int mLTECheckedBand_TDD = 0;
    private int mLTECheckedBand_FDD = 0;
//    private int mMode = BandSelector.RADIO_MODE_NONE;
    private Context mContext;
    private int mPhoneID;
    private String mChannel;

    private static LTEBandData mLTEBandData;
    public synchronized static LTEBandData getInstance(int phoneID, Context context) {
        if ( mLTEBandData == null ) {
            mLTEBandData = new LTEBandData(phoneID, context);
        }
        return mLTEBandData;
    }

    public LTEBandData(int phoneID, Context context) {
        mContext = context;
        mPhoneID = phoneID;
        mChannel = "atchannel"+mPhoneID;
//        super(context, phoneID, pref);
        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(mContext);
        mLTESupportBand_TDD = sp.getInt(ORI_LTE_BAND_TDD, -1);
        mLTESupportBand_FDD = sp.getInt(ORI_LTE_BAND_FDD, -1);
//        mMode = mode;
    }

    
    public boolean loadSelectedBand() {
        String resp = IATUtils.sendATCmd(engconstents.ENG_AT_GET_LTE_BAND, mChannel);
        if (resp == null) {
            return false;
        }
        if (!resp.contains(IATUtils.AT_OK)) {
            return false;
        }

        String[] result = resp.split("\n");
        String[] result2 = result[0].split("\\:");
        String[] result3 = result2[1].split(",");       
        mLTECheckedBand_TDD = ((Integer.valueOf(result3[0].trim()) << 16) & 0xffff0000) + Integer.valueOf(result3[1].trim());
        mLTECheckedBand_FDD = ((Integer.valueOf(result3[2].trim()) << 16) & 0xffff0000) + Integer.valueOf(result3[3].trim());
        Log.d(TAG, "GET_CHECKED_LTE_BAND mLTESupportBand_TDD=" + mLTESupportBand_TDD + ",mLTESupportBand_FDD="
                + mLTESupportBand_FDD);
        Log.d(TAG, "GET_CHECKED_LTE_BAND mLTECheckBand_TDD=" +mLTECheckedBand_TDD + ",mLTECheckBand_FDD="
                + mLTECheckedBand_FDD);

        if ( mLTESupportBand_TDD == -1 && mLTESupportBand_FDD == -1 ) {
            mLTESupportBand_TDD = mLTECheckedBand_TDD;
            mLTESupportBand_FDD = mLTECheckedBand_FDD;
            Log.d(TAG, " mLTESupportBand_TDD=" + mLTESupportBand_TDD 
                    + ",mLTESupportBand_FDD=" + mLTESupportBand_FDD);
            SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(mContext);
            SharedPreferences.Editor editor = sp.edit();
            editor.putInt(ORI_LTE_BAND_TDD, mLTESupportBand_TDD);
            editor.putInt(ORI_LTE_BAND_FDD, mLTESupportBand_FDD);
            editor.apply();
        }
        return true;
    }

    public  String[] getSelectATCmd(int mode, int selecteds[]) {
        int tdd = 0;
        int fdd = 0;
        int n = 0;
        //Log.d(TAG, "getSelectATCmd() mSelecteds:"+Arrays.toString(mSelecteds));
        if ( mode == BandSelector.RADIO_MODE_LTE_FDD ) {
            tdd = mLTECheckedBand_TDD;
            for ( int i = 0; i < 32; i++ ) {
                if (((mLTESupportBand_FDD >> i) & 0x01) == 1) {
                    fdd += (selecteds[n] << (i));
                    //Log.d(TAG, " i:"+i+" n:"+n+" mSelecteds[n]:"+mSelecteds[n]+" (mSelecteds[n] << (i-1)):"+(mSelecteds[n] << (i))+" fdd:"+fdd);
                    n++;
                }
            }
        } else if ( mode == BandSelector.RADIO_MODE_TD_LTE ) {
            fdd = mLTECheckedBand_FDD;
            for ( int i = 0; i < 32; i++ ) {
                if (((mLTESupportBand_TDD >> i) & 0x01) == 1) {
                    tdd += (selecteds[n] << (i));
                    //Log.d(TAG, " i:"+i+" n:"+n+" mSelecteds[n]:"+mSelecteds[n]+" (mSelecteds[n] << (i-1)):"+(mSelecteds[n] << (i))+" tdd:"+tdd);
                    n++;
                }
            }
        }
        //Log.d(TAG, " tdd:"+tdd+" fdd:"+fdd);

        mLTECheckedBand_TDD = tdd;
        mLTECheckedBand_FDD = fdd;
     
        int tddatCmd_h = (tdd>>16) & 0xFFFF;
        int tddatCmd_l = tdd & 0xFFFF;
        int fddatCmd_h = (fdd>>16) & 0xFFFF;
        int fddatCmd_l = fdd & 0xFFFF;
        
        return new String[] {engconstents.ENG_AT_SET_LTE_BAND + tddatCmd_h + "," + tddatCmd_l + "," + fddatCmd_h + "," + fddatCmd_l};
    }

//    @Override
//    public boolean isCheckOneOrMore() {
//        int n = 0;
//        boolean checkOneFdd = false;
//        boolean checkOneTdd = false;
//        if ( mMode == BandSelector.RADIO_MODE_LTE_FDD ) {
//            for ( int i = 0; i < 32; i++ ) {
//                if (((mLTESupportBand_FDD >> i) & 0x01) == 1) {
//                    if ( mSelecteds[n] == 1 ) {
//                        checkOneFdd = true;
//                    }
//                    n++;
//                }
//            }
//        } else if ( mMode == BandSelector.RADIO_MODE_TD_LTE ) {
//            for ( int i = 0; i < 32; i++ ) {
//                if (((mLTESupportBand_TDD >> i) & 0x01) == 1) {
//                    if ( mSelecteds[n] == 1 ) {
//                        checkOneTdd = true;
//                    }
//                    n++;
//                }
//            }
//        }
//
//        return checkOneFdd || checkOneTdd;
//    }

    public int getSupportBand_TDD() {
        return mLTESupportBand_TDD;
    }

    public int getSupportBand_FDD() {
        return mLTESupportBand_FDD;
    }

    public int getCheckedBand_TDD() {
        return mLTECheckedBand_TDD;
    }

    public int getCheckedBand_FDD() {
        return mLTECheckedBand_FDD;
    }

}
