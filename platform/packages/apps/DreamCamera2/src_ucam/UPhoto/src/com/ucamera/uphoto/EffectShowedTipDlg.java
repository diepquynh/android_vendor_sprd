/*
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.app.Dialog;
import android.content.Context;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;

import com.ucamera.uphoto.R;
import com.ucamera.uphoto.util.Models;

public class EffectShowedTipDlg extends Dialog {

    public EffectShowedTipDlg(Context context) {
        super(context);
    }

    public EffectShowedTipDlg(Context context, int theme) {
        super(context, theme);
    }

    public boolean onTouchEvent(MotionEvent event){
        cancel();
        return super.onTouchEvent(event);
    }

//    public static Dialog createDlg(Context context, int dlgX, int dlgY, int dlgW, int dlgH,
//            int layoutId, int msgTxtVId, int stringMsgId) {
    public static Dialog createDlg(Context context, int dlgX, int dlgY, int dlgW, int dlgH, int layoutId) {
        LayoutInflater lytInflt = LayoutInflater.from(context);
        View rootV = lytInflt.inflate(layoutId, null);
//        TextView msgV = (TextView)rootV.findViewById(msgTxtVId);
//        if(null != msgV){
//            if(stringMsgId != -1){
//                msgV.setText(stringMsgId);
//            }else{
//                msgV.setText("Message");
//            }
//        }
        Dialog dlg = new EffectShowedTipDlg(context,R.style.tip_dlg_style);
        dlg.setContentView(rootV);
        dlg.setCanceledOnTouchOutside(true);
        WindowManager.LayoutParams winLp = dlg.getWindow().getAttributes();
        winLp.x = dlgX;
        winLp.y = dlgY;
        winLp.width = dlgW;
        winLp.height = dlgH;
        return dlg;
    }

    public static Dialog createEffectShowedDlg(Context context, int dlgX, int dlgY, int dlgW, int dlgH) {
        Dialog dlg = createDlg(context, dlgX, dlgY, dlgW, dlgH, R.layout.effect_showed_tip_dlg_layout);
//        if(Build.MODEL)
        if(Models.getModel().equals(Models.AMAZON_KFTT)) {
            dlg.getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        }
        return dlg;
    }

    public static Dialog createEffectSelectedDlg(Context context, int dlgX, int dlgY, int dlgW, int dlgH) {
        Dialog dlg = createDlg(context, dlgX, dlgY, dlgW, dlgH, R.layout.effect_selected_tip_dlg_layout);
        return dlg;
    }
}
