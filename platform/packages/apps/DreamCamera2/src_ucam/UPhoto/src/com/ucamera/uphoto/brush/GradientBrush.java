/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.SweepGradient;

import java.util.Random;

public class GradientBrush extends RainbowBrush {
    private int mColors[];

    public GradientBrush() {
        mColors = new int[5];
        mBrushStyle = BrushConstant.GradientBrush;
    }

    public void prepareBrush() {
        randomPaint(mBrushPaint, mRandom);
    }

    protected int randomColor(Random random) {
        int color = Color.RED;
        if(mRandomColorPicker != null) {
            color = mRandomColorPicker.getRandomColor();
        }
        return color;
    }

    protected void randomGradient(Random random) {
        int i = 0;
        int len = mColors.length;
        do {
            mColors[i] = randomColor(random);
            i++;
        } while(i < len);

        mRainbow = new SweepGradient(0F, 0F, mColors, null);
    }

    public void randomPaint(Paint paint, Random random) {
//        mBrushSize = randomWidth(mBrushSize, random);
        randomGradient(mRandom);
        paint.setStrokeWidth(mBrushSize);
        if(mRainbow != null) {
            paint.setShader(mRainbow);
        }
    }

    public void updateBrush() {
        mBrushSize = randomWidth(mBrushSize, mRandom);
        mBrushPaint.setStrokeWidth(mBrushSize);
    }
}
