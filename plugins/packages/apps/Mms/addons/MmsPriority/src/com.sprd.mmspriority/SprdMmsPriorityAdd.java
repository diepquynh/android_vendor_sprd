package com.sprd.mmspriority;

import android.preference.Preference;
import android.preference.PreferenceCategory;
import android.util.Log;

import com.sprd.mms.ui.SprdMmsPriorityStub;


public class SprdMmsPriorityAdd extends SprdMmsPriorityStub{
    private static final String TAG = "SprdMmsPriorityAdd";
    @Override
    public void addMmsPriority(PreferenceCategory parPre, Preference ChildPre) {
        Log.d(TAG, "addMmsPriority()");
        parPre.addPreference(ChildPre);
    }
}
