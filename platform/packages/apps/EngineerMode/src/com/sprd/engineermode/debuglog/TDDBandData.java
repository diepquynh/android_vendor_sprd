package com.sprd.engineermode.debuglog;

import java.util.ArrayList;

import android.content.Context;
import android.preference.CheckBoxPreference;
import android.preference.PreferenceCategory;

public class TDDBandData extends BandData {
    private static final String TAG = "TDDBandData";
    LTEBandData mLTEBandData;

    public TDDBandData(Context context, int phoneID, PreferenceCategory pref) {
        super(context, phoneID, pref);
        mLTEBandData = LTEBandData.getInstance(phoneID, context);
    }

    @Override
    protected boolean getSelectedBand() {
        ArrayList<String> title = new ArrayList<String>();
        ArrayList<String> key = new ArrayList<String>();
        ArrayList<Integer> checked = new ArrayList<Integer>();

        mLTEBandData.loadSelectedBand();
        int supportband = mLTEBandData.getSupportBand_TDD();
        int checkedband = mLTEBandData.getCheckedBand_TDD();

        for ( int i = 0; i < 32; i++ ) {
            if (((supportband >> i) & 0x01) == 1) {
                title.add(LTEBandData.KEY_TDD_LTE + (i+33));
                key.add(LTEBandData.KEY_TDD_LTE + (i+33));
                checked.add((checkedband >> i) & 0x01);
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
        return true;
    }

    @Override
    protected String[] getSelectATCmd() {
        return mLTEBandData.getSelectATCmd(BandSelector.RADIO_MODE_TD_LTE, mSelecteds);
    }

}
