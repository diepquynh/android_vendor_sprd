/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.Color;

import java.util.Random;

public class RandomColorPicker {
    private int mColumnColorCount;
    int mCurColorIndex;
    float[] mHSV;
    private HSV mHsvTable[];
    int mNumColors;
    Random mRandom;
    private int mRandomColorColumn;

    class HSV {
        public float h;
        public float s;
        public float v;
    }


    public RandomColorPicker(int numColors) {
        mHSV = new float[3];
        mNumColors = numColors;
        mRandom = new Random();
        mRandom.setSeed(System.currentTimeMillis());
        mCurColorIndex = 0;
        mRandomColorColumn = 0;
        mColumnColorCount = numColors / 6;
        createColorTable();
    }

    public int convertToColor(HSV hsv) {
        mHSV[0] = hsv.h;
        mHSV[1] = hsv.s;
        mHSV[2] = hsv.v;

        return Color.HSVToColor(mHSV);
    }

    public void createColorTable() {
        int i = 0;
        HSV[] ahsv = new HSV[mNumColors];
        do {
            HSV hsv = new HSV();
            hsv.s = 1F;
            hsv.v = 1F;
            hsv.h = 360F / mNumColors * i;
            ahsv[i] = hsv;
            i++;
        } while (i < mNumColors);

        mHsvTable = ahsv;
    }

    public int getNextColor() {
        mCurColorIndex = mCurColorIndex + 1;
        if(mCurColorIndex >= mNumColors) {
            mCurColorIndex = 0;
        }
        HSV hsv = mHsvTable[mCurColorIndex];

        return convertToColor(hsv);
    }

    public int getRandomColor() {
        int totalColor = mRandomColorColumn * mColumnColorCount;
        int currentRandomColor = mRandom.nextInt(mColumnColorCount);
        mCurColorIndex = totalColor + currentRandomColor;
        mCurColorIndex = Math.min(mNumColors - 1, mCurColorIndex);
        mRandomColorColumn = mRandomColorColumn + 1;
        if(mRandomColorColumn > 5) {
            mRandomColorColumn = 0;
        }

        return convertToColor(mHsvTable[mCurColorIndex]);
    }

    public void resetPicker() {
        mCurColorIndex = 0;
    }
}
