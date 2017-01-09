/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery;


import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.widget.ImageView;

public class ThumbnailView extends ImageView{
    private Paint mPaint;
    public ThumbnailView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        mPaint = new Paint();
        mPaint.setStyle(Style.STROKE);
        mPaint.setStrokeWidth(7);
        mPaint.setColor(0xff00aced);
    }
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if(isSelected()) {
            Rect rect = new Rect();
            this.getDrawingRect(rect);
            canvas.save();
            canvas.drawRect(rect, mPaint);
            canvas.restore();
        }
    }
}
