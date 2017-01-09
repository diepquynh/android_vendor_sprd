/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.MaskFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;

public class NeonBrush extends BaseBrush {
    private float mBlurRadius;
    private Paint mNeonPaint;

    public NeonBrush(int brushMode) {
        mBrushStyle = BrushConstant.NeonBrush;
        mBrushMode = brushMode;
        if(mBrushMode == BrushConstant.BrushModeRandom) {
            mIsRandomColor = true;
        } else {
            mIsRandomColor = false;
        }
        mBrushMaxSize = 7F;
        mBrushMinSize = 1F;
        mBrushSize = 5F;
        mNeonPaint = new Paint();
    }

    public Rect drawStroke(Canvas canvas, Path path) {
        Rect rect = null;
        canvas.drawPath(path, mNeonPaint);
        canvas.drawPath(path, mBrushPaint);
        float size = mBrushSize + mBlurRadius;
        updateDirtyRect(path, size);
        rect = mDirtyRect;

        return rect;
    }

    public void prepareBrush() {
        if(mBrushMode == BrushConstant.BrushModeRandom) {
            if(mRandomColorPicker != null) {
                mBrushColor = mRandomColorPicker.getRandomColor();
            }
        }
        mNeonPaint.setAntiAlias(true);
        mNeonPaint.setStyle(android.graphics.Paint.Style.STROKE);
        mNeonPaint.setStrokeCap(android.graphics.Paint.Cap.ROUND);
        mNeonPaint.setStrokeJoin(android.graphics.Paint.Join.ROUND);
        mNeonPaint.setColor(mBrushColor);
        mBlurRadius = mBrushSize + 6F;
        MaskFilter maskFilter = new BlurMaskFilter(mBlurRadius, android.graphics.BlurMaskFilter.Blur.NORMAL);
        mNeonPaint.setMaskFilter(maskFilter);
        mNeonPaint.setStrokeWidth(mBrushSize * 1.5F);
        mBrushPaint.setAntiAlias(true);
        mBrushPaint.setStyle(android.graphics.Paint.Style.STROKE);
        mBrushPaint.setStrokeCap(android.graphics.Paint.Cap.ROUND);
        mBrushPaint.setStrokeJoin(android.graphics.Paint.Join.ROUND);
        mBrushPaint.setColor(-1);
        mBrushPaint.setStrokeWidth(mBrushSize);
    }
}
