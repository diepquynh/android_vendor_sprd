/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery.preference;

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
            /*
             * FIX BUG: 1387
             * BUG CAUSE: For lepad_001b this exception is raised, unknown reason
             * FIX COMMENT:Just igore that
             * Date: 2012-08-17
             */
            Log.e(TAG,"Fail bind checkbox view",e);
        }
    }
}
