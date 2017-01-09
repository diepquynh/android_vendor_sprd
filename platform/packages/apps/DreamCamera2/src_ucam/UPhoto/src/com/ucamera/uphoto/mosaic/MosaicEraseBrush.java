/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto.mosaic;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Paint.Style;

public class MosaicEraseBrush extends MosaicBrush{
    private Paint mPaint;
    public MosaicEraseBrush() {
        init();
    }
    private void init() {
        mPaint = new Paint();
        mPaint.setAntiAlias(true);
        mPaint.setStyle(Style.STROKE);
        mPaint.setStrokeWidth(mBrushSize);
        mPaint.setStrokeJoin(Paint.Join.ROUND);
        mPaint.setStrokeCap(Paint.Cap.ROUND);
        mPaint.setAlpha(0);
        mPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
    }
    @Override
    public void paintBegin(Canvas canvas, Path path) {
        canvas.drawPath(path, mPaint);
    }
    @Override
    public void updatePaintSize() {
        mPaint.setStrokeWidth(mBrushSize);
    }
    @Override
    public void paintWhole(Canvas canvas, Path path) {
        canvas.drawPath(path, mPaint);
    }
}
