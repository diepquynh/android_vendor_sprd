/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.Color;
import android.graphics.Paint;

import java.util.Random;

public class PenInkBrush extends BaseBrush {
    private Random mRandom;

    public PenInkBrush() {
        mRandom = new Random();
        // CID 109359 : UrF: Unread field
        // mBrushGreyValue = mRandom.nextInt(256);
        mBrushMaxSize = 9F;
        mBrushMinSize = 1F;
        mBrushSize = 3.5F;
        mBrushColor = Color.BLACK;
        mBrushStyle = BrushConstant.PenInkBrush;
        mBrushMode = BrushConstant.BrushModeRandom;
        mIsRandomColor = true;
        setSizeBound();
    }

    private float randomWidth(float brushSize, Random random) {
        float randomSize = brushSize + (float)(random.nextInt(5) - 2) * 0.6F;
        float tempRandomWidth = Math.max(randomSize, mSizeLowerBound);
        float randomWidth = Math.min(tempRandomWidth, mSizeUpperBound);

        return randomWidth;
    }

    public void prepareBrush() {
        mBrushPaint.setAntiAlias(true);
        mBrushPaint.setStrokeCap(android.graphics.Paint.Cap.ROUND);
        mBrushPaint.setStrokeJoin(android.graphics.Paint.Join.ROUND);
        mBrushPaint.setStyle(android.graphics.Paint.Style.STROKE);
        mBrushPaint.setStrokeWidth(mBrushSize);
        mBrushPaint.setColor(mBrushColor);
    }

    public void randomPaint(Paint paint, Random random) {
        mBrushPaint.setColor(mBrushColor);
        mBrushSize = randomWidth(mBrushSize, random);
        mBrushPaint.setStrokeWidth(mBrushSize);
        mBrushPaint.setStyle(android.graphics.Paint.Style.STROKE);
    }

    public void setColor(int color) {
        mBrushColor = color;
    }

    public void updateBrush() {
        randomPaint(mBrushPaint, mRandom);
    }
}
