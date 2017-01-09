/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucam.modules.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.widget.TextView;

public class MyRotateTextView extends TextView{

    private int mOrientation = 0;
    public MyRotateTextView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public MyRotateTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public MyRotateTextView(Context context) {
        super(context);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, widthMeasureSpec);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        canvas.rotate(-mOrientation,getWidth() / 2, getHeight() / 2);
        super.onDraw(canvas);
    }

    public void setOrientation(int orient){
        mOrientation = orient;
        invalidate();
    }
}
