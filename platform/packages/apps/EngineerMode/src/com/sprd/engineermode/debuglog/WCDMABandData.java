package com.sprd.engineermode.debuglog;

import java.util.ArrayList;
import java.util.Arrays;

import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.CheckBoxPreference;
import android.preference.PreferenceCategory;
import android.preference.PreferenceManager;
import android.util.Log;

public class WCDMABandData extends BandData {
    private static final String TAG = "WCDMABandData";

    private final static String ORI_WCDMA_BAND = "ORI_WCDMA_BAND";
    private static final int[] mWBandMap = {1, 2, 5, 8};
    private int mWCDMASupportBand = -1;

    public WCDMABandData(Context context, int phoneID, PreferenceCategory pref) {
        super(context, phoneID, pref);

        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(mContext);
        mWCDMASupportBand = sp.getInt(ORI_WCDMA_BAND, -1);

//        mBandTitle = new String[] {"WCDMA Band1", "WCDMA Band2", "WCDMA Band5", "WCDMA Band8"};
//        mBandFrequency = new String[] {"w1", "w2", "w5", "w8"};
//        mSelecteds = new int[] {0, 0, 0, 0};
//        mPrefCheckBox = new CheckBoxPreference[mWBandMap.length];
    }

    @Override
    protected boolean getSelectedBand() {
//        01-05 12:03:51.303 19124 19124 D IATUtils: <0> mAtChannel = com.sprd.internal.telephony.IAtChannel$Stub$Proxy@41e3c4b0 , and cmd = AT+SPFDDBAND=0,2
//        01-05 12:03:51.313 19124 19124 D IATUtils: <0> AT response +SPFDDBAND: 0,2,1
//        01-05 12:03:51.313 19124 19124 D IATUtils: OK
        int nchecked = 0;
        for (int i = 0; i < mWBandMap.length; i++) {
            String resp = IATUtils.sendATCmd(engconstents.ENG_AT_W_LOCKED_BAND+mWBandMap[i], mChannel);
            Log.d(TAG, "getSelectedBand() resp:"+resp);
            if (resp == null) {
                return false;
            }
            if (!resp.contains(IATUtils.AT_OK)) {
                return false;
            }
            int n = parseWCDMAResp(resp);
            nchecked += n << i;
            //mSelecteds[i] = parseWCDMAResp(resp);
        }

        if ( mWCDMASupportBand == -1 ) {
            mWCDMASupportBand = nchecked;
            SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(mContext);
            SharedPreferences.Editor editor = sp.edit();
            editor.putInt(ORI_WCDMA_BAND, mWCDMASupportBand);
            editor.apply();
        }

        ArrayList<String> title = new ArrayList<String>();
        ArrayList<String> key = new ArrayList<String>();
        ArrayList<Integer> checked = new ArrayList<Integer>();

        for (int i = 0; i < mWBandMap.length; i++) {
            if ( ((mWCDMASupportBand >> i) & (0x01)) == 1 ) {
                title.add("WCDMA Band"+mWBandMap[i]);
                key.add("w"+mWBandMap[i]);
                checked.add((nchecked >> i) & 0x01);
            }
        }

        if ( mBandTitle == null ) {
            mBandTitle = title.toArray(new String[0]);
            mBandFrequency = key.toArray(new String[0]);
            mPrefCheckBox = new CheckBoxPreference[mBandTitle.length];
            mSelecteds = new int[checked.size()];
        }
        for (int i = 0; i < checked.size(); i++) {
            mSelecteds[i] = checked.get(i);
        }

        Log.d(TAG, "getSelectedBand() mWCDMASupportBand:"+mWCDMASupportBand+" nchecked:"+nchecked+" mBandTitle:"+Arrays.toString(mBandTitle)+" mSelecteds:"+Arrays.toString(mSelecteds));
        return true;
    }

    private int parseWCDMAResp(String resp) {
        String str[] = resp.split(":|\n|,");
        Log.d(TAG, "parseWCDMAResp() str:"+Arrays.toString(str));
        if ( str.length >= 3 && str[3].trim().equals("1")) {
            return 1;
        }
        return 0;
    }

    @Override
    protected String[] getSelectATCmd() {
        String [] atcmds = new String[mSelecteds.length];
        for ( int i = 0,n = 0; i < mWBandMap.length; i++ ) {
            if ( ((mWCDMASupportBand >> i) & (0x01)) == 1 ) {
                atcmds[n] = engconstents.ENG_AT_W_LOCK_BAND+mWBandMap[i]+","+mSelecteds[n];
                n++;
            }
        }
        return atcmds;
    }

}
