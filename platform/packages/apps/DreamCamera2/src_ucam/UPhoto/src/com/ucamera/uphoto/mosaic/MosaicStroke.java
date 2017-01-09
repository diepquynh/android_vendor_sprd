/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto.mosaic;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Path;

public class MosaicStroke {
    private MosaicBrush mBrush;
    private Path mPath = new Path();
    public MosaicStroke() {}
    public void paintBegin(Canvas canvas, float x, float y) {
        mPath.moveTo(x, y);
        mBrush.paintBegin(canvas, mPath);
    }
    public void paintMove(Canvas canvas, float x1, float y1, float x2, float y2) {
        mPath.quadTo(x1, y1, x2, y2);
        mBrush.paintBegin(canvas, mPath);
    }
    public void paintEnd(Canvas canvas, float x, float y) {
        mPath.lineTo(x, y);
        mBrush.paintBegin(canvas, mPath);
    }
    public void setBrush(MosaicBrush brush) {
        mBrush = brush;
    }
    public void updateShader(Bitmap bitmap) {
        mBrush.setPaintShader(bitmap);
    }
    public void paintWholePath(Canvas canvas) {
        mBrush.paintWhole(canvas, mPath);
    }
}
