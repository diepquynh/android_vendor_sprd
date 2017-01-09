package com.sprd.engineermode.debuglog;

import java.util.Arrays;

import com.sprd.engineermode.engconstents;
import com.sprd.engineermode.utils.IATUtils;

import android.content.Context;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.util.Log;

public abstract class BandData {
    private static final String TAG = "BandData";
    protected Context mContext;
    protected int mPhoneID;
    protected String mChannel;
    protected PreferenceCategory mParentPref = null;
    protected boolean mSetSuccessful = false;
    private boolean mIsChanged = false;

    protected String[] mBandTitle = null;
    protected String[] mBandFrequency = null;
    protected int[] mSelecteds = null;
    protected CheckBoxPreference[] mPrefCheckBox = null;

    protected abstract boolean getSelectedBand();
    protected abstract String[] getSelectATCmd();

    public BandData(Context context, int phoneID, PreferenceCategory pref) {
        mContext = context;
        mPhoneID = phoneID;
        mChannel = "atchannel"+mPhoneID;
        mParentPref = pref;
    }

    public boolean init() {
        mIsChanged = false;
        if ( !getSelectedBand() ) {
            return false;
        }
//        addAvailableBandsToPerference();
        updateCheckBox();
        return true;
    }

    public void updateCheckBox() {
        for ( int i = 0; i < mPrefCheckBox.length; i++) {
            if ( mPrefCheckBox[i] != null ) {
//                Log.d(TAG, "updateCheckBox mPrefCheckBox[i].setChecked("+(mSelecteds[i] == 1)+")");
                mPrefCheckBox[i].setChecked(mSelecteds[i] == 1);
            }
        }
    }

    public boolean successful() {
        return mSetSuccessful;
    }

    protected CheckBoxPreference[] addAvailableBandsToPerference() {
        Log.d(TAG, "getPerference() mSelecteds:"+Arrays.toString(mSelecteds));
        //Log.d(TAG, " mPrefCheckBox:"+Arrays.toString(mPrefCheckBox));

        for (int i = 0; i < mBandTitle.length; i++) {
            if ( mPrefCheckBox[i] == null ) {
                mPrefCheckBox[i] = new CheckBoxPreference(mContext);
//                mParentPref.addPreference(mPrefCheckBox[i]);
                mPrefCheckBox[i].setTitle(mBandTitle[i]);
                mPrefCheckBox[i].setKey(mBandFrequency[i]);
                mPrefCheckBox[i].setOnPreferenceClickListener(new PerferecnerOnClick());
                //Log.d(TAG, " mBandTitle["+i+"]:"+mBandTitle[i]+" mSelecteds["+i+"]:"+mSelecteds[i]);
            }
            mPrefCheckBox[i].setChecked(mSelecteds[i] == 1);
        }
        return mPrefCheckBox;
    }

    private class PerferecnerOnClick implements Preference.OnPreferenceClickListener {
        @Override
        public boolean onPreferenceClick(Preference preference) {
            String key = preference.getKey();
            for ( int i = 0; i < mBandFrequency.length; i++ ) {
                if ( key.equals(mBandFrequency[i])) {
                    mSelecteds[i] = ((CheckBoxPreference)preference).isChecked()?1:0;
                    mIsChanged = true;
                    Log.d(TAG, mBandTitle[i]+"->"+(mSelecteds[i]==1?"chekced":"unchecked"));
                    break;
                }
            }
            return true;
        }
    }

    public boolean isChanged() {
        return mIsChanged;
    }

    public boolean setSelectedBandToModem() {
        String atCmds[] = getSelectATCmd();
        mSetSuccessful = true;
        for ( int i = 0; i < atCmds.length; i++ ){
            if ( atCmds[i] != null) {
                String response = IATUtils.sendATCmd(atCmds[i], mChannel);
                Log.d(TAG, "<"+mChannel+"> setBandSelect is " + atCmds[i]
                            + " Result:" + response );
                if ( !response.contains(IATUtils.AT_OK) ) {
                    mSetSuccessful = false;
                    break;
                }
            }
        }
        return mSetSuccessful;
    }

    public boolean isCheckOneOrMore() {
        for (int i = 0; i < mSelecteds.length; i++ ) {
            if ( mSelecteds[i] ==1 ) {
                return true;
            }
        }
        return false;
    }
}