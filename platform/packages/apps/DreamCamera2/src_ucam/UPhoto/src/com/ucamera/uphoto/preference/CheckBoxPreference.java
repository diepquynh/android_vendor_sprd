/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto.preference;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

public class CheckBoxPreference extends android.preference.CheckBoxPreference {
    private static final String TAG = "CheckBoxPreference";

    public CheckBoxPreference(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public CheckBoxPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public CheckBoxPreference(Context context) {
        super(context);
    }

    @Override
    protected void onBindView(View view) {
        try {
            super.onBindView(view);
        } catch (NullPointerException e) {
            Log.e(TAG,"Fail bind checkbox view",e);
        }
    }
}
