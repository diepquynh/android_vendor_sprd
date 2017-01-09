/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto.mosaic;

import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Shader;
import android.graphics.Shader.TileMode;

public class MosaicDrawBrush extends MosaicBrush {
    private Paint mPaint;
    private Shader mShader;
    private Bitmap mBitmap;
    public MosaicDrawBrush() {
        initPaint();
    }
    private void initPaint() {
        mPaint = new Paint();
        mPaint.setStrokeWidth(mBrushSize);
        mPaint.setAntiAlias(true);
        mPaint.setStyle(Paint.Style.STROKE);
        mPaint.setStrokeJoin(Paint.Join.ROUND);
        mPaint.setStrokeCap(Paint.Cap.ROUND);
    }
    @Override
    public void setPaintShader(Bitmap bitmap) {
        mBitmap = bitmap;
        mShader = new BitmapShader(mBitmap, TileMode.REPEAT, TileMode.REPEAT);
        mPaint.setShader(mShader);
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
