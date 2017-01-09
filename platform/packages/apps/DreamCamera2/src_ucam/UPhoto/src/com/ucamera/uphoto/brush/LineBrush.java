/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.Paint;

import java.util.Random;

public class LineBrush extends BaseBrush {
    private Random mRandom;

    public LineBrush() {
        mRandom = new Random();
        /*SPRD: CID 111519 (#1 of 1): UrF: Unread field (FB.URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD)
        mBrushGreyValue = mRandom.nextInt(256);
        */
        mBrushMaxSize = 15F;
        mBrushMinSize = 1F;
        mBrushSize = 5F;
        mBrushStyle = BrushConstant.LineBrush;
//        mBrushMode = BrushConstant.BrushModeRandom;
        mIsRandomColor = true;
    }

    private float randomWidth(float brushSize, Random random) {
        float randomSize = (float)(random.nextInt(5) - 2) * 0.6F;
        float tempSize = brushSize + randomSize;
        float maxSize = Math.max(tempSize, mSizeLowerBound);
        float randomWidth = Math.min(maxSize, mSizeUpperBound);

        return randomWidth;
    }

    public void prepareBrush() {
        mBrushPaint.setAntiAlias(true);
        mBrushPaint.setStrokeCap(android.graphics.Paint.Cap.ROUND);
        mBrushPaint.setStrokeJoin(android.graphics.Paint.Join.ROUND);
        mBrushPaint.setStyle(android.graphics.Paint.Style.STROKE);
        mBrushPaint.setStrokeWidth(mBrushSize);

        if(mBrushMode == BrushConstant.BrushModeRandom && mRandomColorPicker != null) {
            mBrushColor = mRandomColorPicker.getRandomColor();
        }
        mBrushPaint.setColor(mBrushColor);
    }

    public void randomPaint(Paint paint, Random random) {
        mBrushSize = randomWidth(mBrushSize, random);
        mBrushPaint.setColor(mBrushColor);
        mBrushPaint.setStrokeWidth(mBrushSize);
        mBrushPaint.setStyle(android.graphics.Paint.Style.STROKE);
    }

    public void updateBrush() {
        randomPaint(mBrushPaint, mRandom);
    }

}
