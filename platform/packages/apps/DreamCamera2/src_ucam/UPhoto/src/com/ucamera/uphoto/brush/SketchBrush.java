/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.Color;
import android.graphics.Paint;

import java.util.Random;

public class SketchBrush extends BaseBrush {
    private Random mRandom;

    public SketchBrush() {
        mRandom = new Random();
        setupBrush(BrushConstant.BrushModeSelected);
    }

    public SketchBrush(int brushMode) {
        mRandom = new Random();
        setupBrush(brushMode);
    }

    private void setupBrush(int brushMode) {
//        mBrushStyle = BrushConstant.SketchBrush;
        mBrushMode = brushMode;
        mIsRandomColor = false;
        if(mBrushMode == BrushConstant.BrushModeRandom) {
            mIsRandomColor = true;
        }
        mBrushMaxSize = 5F;
        mBrushMinSize = 0.5F;
        mBrushSize = 1.5F;
        mBrushColor = Color.RED;
        mBrushAlphaValue = 140;
    }

    public void prepareBrush() {
        if(mBrushMode == BrushConstant.BrushModeRandom) {
            if(mRandomColorPicker != null) {
                mBrushColor = mRandomColorPicker.getRandomColor();
            }
        }
        mBrushPaint.setAntiAlias(true);
        mBrushPaint.setStyle(android.graphics.Paint.Style.STROKE);
        mBrushPaint.setStrokeCap(android.graphics.Paint.Cap.ROUND);
        mBrushPaint.setStrokeJoin(android.graphics.Paint.Join.ROUND);
        mBrushColor = Color.BLACK;
        mBrushPaint.setColor(mBrushColor);
        mBrushPaint.setAlpha(mBrushAlphaValue);
        mBrushPaint.setStrokeWidth(mBrushSize);
    }

    public void randomPaint(Paint paint) {
        float strokeWidth = randomWidth(mBrushSize);
        paint.setStrokeWidth(strokeWidth);
    }

    protected float randomWidth(float brushSize) {
        float size = brushSize + (float)(mRandom.nextInt(5) - 2) * 0.4F;
        float width = Math.max(size, mSizeLowerBound);
        float stokeWidth = Math.min(width, mSizeUpperBound);

        return stokeWidth;
    }

    public void updateBrush() {
        mBrushPaint.setAlpha(mBrushAlphaValue);

        randomPaint(mBrushPaint);
    }
}
