package com.sprd.engineermode.debuglog;

import java.util.Arrays;

import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

import android.content.Context;
import android.preference.CheckBoxPreference;
import android.preference.PreferenceCategory;
import android.util.Log;

public class TDBandData extends BandData {
    private static final String TAG = "TDBandData";

    public TDBandData(Context context, int phoneID, PreferenceCategory pref) {
        super(context, phoneID, pref);
        mBandTitle = new String[] {"TD_SCDMA A Band", "TD_SCDMA F Band"};
        mBandFrequency = new String[] {"td_a_frequency", "td_f_frequency"};
        mSelecteds = new int[] {0, 0};
        mPrefCheckBox = new CheckBoxPreference[mBandTitle.length];
    }

    protected boolean getSelectedBand() {
        String resp = IATUtils.sendATCmd(engconstents.ENG_AT_TD_LOCKED_BAND, mChannel);
        if (resp == null) {
            return false;
        }
        if (!resp.contains(IATUtils.AT_OK)) {
            return false;
        }
        // resp e.g. <0> AT response +SPLOCKBAND: A+F band��TDSCDMA 2010 - 2025/1880 - 1920MHz
        String str[] = resp.split(":|\n|,");
        Log.d(TAG, "str:"+Arrays.toString(str));
        String bandstr = str[1].trim();
        if ( bandstr.startsWith("A ") ) {
            mSelecteds[0] = 1;
            mSelecteds[1] = 0;
        } else if (bandstr.startsWith("F ")) {
            mSelecteds[0] = 0;
            mSelecteds[1] = 1;
        } else if (bandstr.startsWith("A+F ")) {
            mSelecteds[0] = 1;
            mSelecteds[1] = 1;
        }
        return true;
    }

    @Override
    protected String[] getSelectATCmd() {
        // e.g.
        // 01-05 07:12:00.206 13443 13541 D IATUtils: <0> mAtChannel = com.sprd.internal.telephony.IAtChannel$Stub$Proxy@41e1ea78 , and cmd = AT+SPLOCKBAND="A"
        // 01-05 07:12:00.236 13443 13541 D IATUtils: <0> AT response OK
        String para = "";
        if ( mSelecteds[0] == 1 && mSelecteds[1] == 1) {
            para = "\"A+F\"";
        } else if ( mSelecteds[0] == 1 ) {
            para = "\"A\"";
        } else if ( mSelecteds[1] == 1 ) {
            para = "\"F\"";
        }
        return new String[] {engconstents.ENG_AT_TD_SET_BAND+para};
    }

}
