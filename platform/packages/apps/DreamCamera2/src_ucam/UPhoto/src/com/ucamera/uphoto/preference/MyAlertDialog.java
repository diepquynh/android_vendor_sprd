/*
 *   Copyright (C) 2010,2011 Thundersoft Corporation
 *   All rights Reserved
 */

package com.ucamera.uphoto.preference;

import android.app.AlertDialog;
import android.content.Context;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;

import com.ucamera.uphoto.R;

public class MyAlertDialog extends AlertDialog {
    private View contentView;

    public MyAlertDialog(Context context) {
        super(context);
        LayoutInflater inflate = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        contentView = inflate.inflate(R.layout.custom_alert_dialog, null);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
//        super.onCreate(savedInstanceState);
        this.getWindow().requestFeature(Window.FEATURE_NO_TITLE);
        this.setContentView(contentView);
    }

    public View getContentView() {
        return contentView;
    }

    /** FIX BUG: 60
     * BUG CAUSE: LG Device modified AlertDialog code about respondingin UI events in the framework layer
     * FIX COMMENT: call Call the super class Dialog's method dispatchTouchEvent explicitly
     * Date: 2012-04-12
     */
    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if (getWindow().superDispatchTouchEvent(ev)) {
            return true;
        }
        return onTouchEvent(ev);
    }

}
