/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.Canvas;
import android.graphics.EmbossMaskFilter;
import android.graphics.MaskFilter;
import android.graphics.Path;
import android.graphics.Rect;

public class EmbossBrush extends BaseBrush {
    private float mBlurRadius;
    private MaskFilter mEmbossFilter;

    public EmbossBrush() {
        TAG = "EmbossBrush";
        mBrushStyle = BrushConstant.EmbossBrush;
        mBrushMode = BrushConstant.BrushModeRandom;
        if(mBrushMode == BrushConstant.BrushModeRandom) {
            mIsRandomColor = true;
        } else {
            mIsRandomColor = false;
        }
        mBrushMaxSize = 18F;
        mBrushMinSize = 4F;
        mBrushSize = 8F;
    }

    public Rect drawStroke(Canvas canvas, Path path) {
        Object obj = null;
        try {
            canvas.drawPath(path, mBrushPaint);
            float size = mBrushSize + mBlurRadius + 5F;
            updateDirtyRect(path, size);
            obj = mDirtyRect;
        } catch(NullPointerException nullPointerException) {
            obj = 0;
        }

        return ((Rect) (obj));
    }

    public void prepareBrush() {
        if(mBrushMode == BrushConstant.BrushModeRandom) {
            if(mRandomColorPicker != null) {
                mBrushColor = mRandomColorPicker.getRandomColor();
            }
        }
        float[] attrs = new float[] {0x4f800000, 0x4f800000, 0x4f800000};
        mBlurRadius = Math.max(1F, mBrushSize / 3F);
        mEmbossFilter = new EmbossMaskFilter(attrs, 0.3F, 6F, mBlurRadius);
        mBrushPaint.setAntiAlias(true);
        mBrushPaint.setStyle(android.graphics.Paint.Style.STROKE);
        mBrushPaint.setStrokeCap(android.graphics.Paint.Cap.ROUND);
        mBrushPaint.setStrokeJoin(android.graphics.Paint.Join.ROUND);
        mBrushPaint.setColor(mBrushColor);
        mBrushPaint.setStrokeWidth(mBrushSize);
        mBrushPaint.setMaskFilter(mEmbossFilter);
    }
}
