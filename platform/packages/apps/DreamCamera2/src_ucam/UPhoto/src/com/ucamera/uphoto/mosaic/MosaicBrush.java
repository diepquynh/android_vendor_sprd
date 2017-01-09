/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto.mosaic;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Path;

public class MosaicBrush {
    protected int mBrushSize;

    public MosaicBrush() {
    }
    public void setPaintShader(Bitmap bitmap) {}
    public void paintBegin(Canvas canvas, Path path) {
    }
    public void updatePaintSize() {}
    public void setBrushSize(int size) {
        mBrushSize = size;
        updatePaintSize();
    }
    public void paintWhole(Canvas canvas, Path path) {}
}
