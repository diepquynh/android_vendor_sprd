/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 *
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.NinePatchDrawable;
import android.util.AttributeSet;
import android.view.View;

public class RectView extends View {
    private Rect[] Rectlist;
    private int RectNum;
    private Paint redPaint;
    // CID 109370:SS: Unread field should be static
    // final int MAX_RECT = 30;
//    private Bitmap mDetectBitmap;
    private NinePatchDrawable mDrawable;

    public RectView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public RectView(Context context) {
        super(context);
    }

    public RectView(Context context, AttributeSet attrs) {
        super(context, attrs);

        this.RectNum = 0;
        this.Rectlist = new Rect[30];

        this.redPaint = new Paint();
        this.redPaint.setColor(Color.BLUE);
        this.redPaint.setStyle(Paint.Style.STROKE);
        this.redPaint.setStrokeWidth(2.0F);
        mDrawable = (NinePatchDrawable)getResources().getDrawable(R.drawable.bt_beautiful_frame);
//        mDetectBitmap = BitmapFactory.decodeStream(context.getResources().openRawResource(R.drawable.bt_beautiful_frame));
    }

    protected void onDraw(Canvas canvas) {
        if (this.RectNum > 0) {
            for (int i = 0; i < RectNum; i++) {
//                canvas.drawRect(Rectlist[i], redPaint);
//                canvas.drawBitmap(mDetectBitmap, Rectlist[i], Rectlist[i], redPaint);
                mDrawable.setBounds(Rectlist[i]);
                mDrawable.draw(canvas);
            }
        }
//        super.onDraw(canvas);
    }

    public void SetRect(Rect[] rect, int num) {
        this.RectNum = num;

        num = Math.min(num, 30);
        for (int i = 0; i < num; i++) {
            this.Rectlist[i] = rect[i];
        }
        invalidate();
    }
}
