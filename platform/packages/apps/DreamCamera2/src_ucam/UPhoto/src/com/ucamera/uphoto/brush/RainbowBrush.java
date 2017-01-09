/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.SweepGradient;

import java.util.Random;

public class RainbowBrush extends BaseBrush {
    /*SPRD: CID 109219 (#1 of 1): UuF: Unused field (FB.UUF_UNUSED_PUBLIC_OR_PROTECTED_FIELD)
    protected boolean mBrushHasAlpha;
    */
    protected SweepGradient mRainbow;
    protected Random mRandom;

    public RainbowBrush() {
        mRandom = new Random();
        mBrushAlphaValue = 255;
        mBrushMaxSize = 30F;
        mBrushMinSize = 1F;
        mBrushSize = 6F;
        mSizeBias = 2F;
        setSizeBound();
        mBrushStyle = BrushConstant.RainbowBrush;

        mBrushPaint.setAntiAlias(true);
        mBrushPaint.setStrokeCap(android.graphics.Paint.Cap.ROUND);
        mBrushPaint.setStrokeJoin(android.graphics.Paint.Join.ROUND);
        mBrushPaint.setStyle(android.graphics.Paint.Style.STROKE);
        mBrushPaint.setStrokeWidth(mBrushSize);
        mIsRandomColor = true;
    }

    public void prepareBrush() {
        randomPaint(mBrushPaint, mRandom);
    }

    protected int randomAlpha(int alpha, Random random) {
        int randomAlpha = random.nextInt(21) - 10;
        alpha += randomAlpha;
        if(alpha <= 100) {
            alpha = 100;
        }
        if(alpha > 255) {
            alpha = 255;
        }

        return alpha;
    }

    protected int randomColor(Random random) {
        int color = Color.RED;
        if(mRandomColorPicker != null) {
            color = mRandomColorPicker.getNextColor();
        }

        return color;
    }

    protected void randomGradient(Random random) {

    }

    public void randomPaint(Paint paint, Random random) {
//        mBrushSize = randomWidth(mBrushSize, random);
        mBrushColor = randomColor(mRandom);
        paint.setAlpha(255);
        paint.setStrokeWidth(mBrushSize);
        paint.setColor(mBrushColor);
    }

    protected float randomWidth(float size, Random random) {
        float bSize = size + (float)(random.nextInt(5) - 2) * 0.4F;
        float f4 = Math.max(bSize, mSizeLowerBound);
        float randomWidth = Math.min(f4, mSizeUpperBound);
        return randomWidth;
    }

    public void updateBrush() {
        randomPaint(mBrushPaint, mRandom);
    }

    public float[] getBrushData() {
        float[] brushData = new float[2];
        brushData[0] = mBrushSize;
        brushData[1] = mBrushColor;

        return brushData;
    }

    public void restoreBrush(float brushData[]) {
        mBrushSize = brushData[0];
        mBrushColor = (int)brushData[1];

        restorePaint();
    }

    public void restorePaint() {
        mBrushPaint.setAntiAlias(true);
        mBrushPaint.setStrokeCap(android.graphics.Paint.Cap.ROUND);
        mBrushPaint.setStrokeJoin(android.graphics.Paint.Join.ROUND);
        mBrushPaint.setAlpha(255);
        mBrushPaint.setStrokeWidth(mBrushSize);
        mBrushPaint.setColor(mBrushColor);
        if(mRainbow != null) {
            mBrushPaint.setShader(mRainbow);
        }
    }
}
