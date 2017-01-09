package com.sprd.engineermode.debuglog;

import java.util.Arrays;

import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

import android.content.Context;
import android.preference.CheckBoxPreference;
import android.preference.PreferenceCategory;
import android.util.Log;

public class GSMBandData extends BandData {
    private static final String TAG = "GSMBandData";

    private static final int[] mSBANDMap = {
        0x01, // 0  - 0000 0001
        0x02, // 1  - 0000 0010
        0x04, // 2  - 0000 0100
        0x08, // 3  - 0000 1000
        0x03, // 4  - 0000 0011
        0x09, // 5  - 0000 1001
        0x0A, // 6  - 0000 1010
        0x0C, // 7  - 0000 1100
        0x05, // 8  - 0000 0101
        0x0B, // 9  - 0000 1011
        0x0D, // 10 - 0000 1101
        0x06, // 11 - 0000 0110
        0x0E, // 12 - 0000 1110
        0x07, // 13 - 0000 0111
        0x0F, // 14 - 0000 1111
    };

    public GSMBandData(Context context, int phoneID, PreferenceCategory pref) {
        super(context, phoneID, pref);
        mBandTitle = new String[] {"GSM850", "PCS1900", "DCS1800", "GSM900"};
        mBandFrequency = new String[] {"g_gsm_frequency", "g_dcs_frequency", "g_pcs_frequency", "gsm_frequency"};
        mSelecteds = new int[] {0, 0, 0, 0};
        mPrefCheckBox = new CheckBoxPreference[mBandTitle.length];
    }

    protected boolean getSelectedBand() {
        String resp = IATUtils.sendATCmd(engconstents.ENG_AT_CURRENT_GSMBAND, mChannel);
        if (resp == null) {
            return false;
        }
        if (!resp.contains(IATUtils.AT_OK)) {
            return false;
        }
        // resp e.g. +SBAND: 13,14
        String str[] = resp.split(":|\n|,");
        Log.d(TAG, "str:"+Arrays.toString(str));
        int band = Integer.valueOf(str[2].trim()).intValue();
        if ( band >= 0 && band <= mSBANDMap.length ) {
            mSelecteds[3] = ((mSBANDMap[band] & 0x01));
            mSelecteds[2] = ((mSBANDMap[band]>>1) & 0x01);
            mSelecteds[1] = ((mSBANDMap[band]>>2) & 0x01);
            mSelecteds[0] = ((mSBANDMap[band]>>3) & 0x01);
        }
        //Log.d(TAG, "getGSMBandSelect() mSelecteds:"+Arrays.toString(mSelecteds));
        return true;
    }

    protected String[] getSelectATCmd() {
        // 01-05 07:11:42.399 13443 13443 D GSMBandData: <0>LOCK_GSM_BAND AT is AT+SBAND=1,13,3 Result:OK
        return new String[] {engconstents.ENG_AT_SELECT_GSMBAND + getSBANDParameter()};
    }

    private int getSBANDParameter() {
        int band = (mSelecteds[0] << 3) + (mSelecteds[1] << 2) + (mSelecteds[2] << 1) + (mSelecteds[3]);
        for ( int i = 0; i < mSBANDMap.length; i++ ) {
            if ( mSBANDMap[i] == band ) {
                return i;
            }
        }
        return -1;
    }
}
