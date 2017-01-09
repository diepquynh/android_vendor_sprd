/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.uscenery;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Paint.Style;
import android.util.AttributeSet;
import android.view.View;

public class FlashView extends View{
    // CID 124826 : MF: Masked Field (FB.MF_CLASS_MASKS_FIELD)
    public Context mPrivContext;
    private Rect mRect;
    public FlashView(Context context){
         super(context);
         init(context);
   }
    public FlashView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init(context);
    }

    public FlashView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }
    public void setDrawRect(Rect rect) {
        mRect = rect;
    }
    private void init(Context context) {
       // CID 124826 : MF: Masked Field (FB.MF_CLASS_MASKS_FIELD)
       mPrivContext= context;
       mRect = new Rect();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        //super.onDraw(canvas);
        Paint paint = new Paint();
        paint.setColor(0xff0f96d5);
        paint.setStyle(Style.STROKE);
        // CID 124826 : MF: Masked Field (FB.MF_CLASS_MASKS_FIELD)
        int scale = (int)mPrivContext.getResources().getDisplayMetrics().density;
        paint.setStrokeWidth(4 * scale);
        canvas.drawRect(mRect, paint);
    }
 }
