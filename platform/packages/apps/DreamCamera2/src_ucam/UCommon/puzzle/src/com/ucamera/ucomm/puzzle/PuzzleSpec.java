/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ucomm.puzzle;

import android.graphics.Rect;

import java.util.Arrays;

/**
 * <pre>
 *  0xFF FF FF FF
 *    |  |  |  |
 *    |  |  |  +-- Bottom
 *    |  |  +----- Right
 *    |  +-------- TOP
 *    +----------- Left
 * </pre>
 */
public class PuzzleSpec {

    private static final int MASK = 0xFF;
    private final int[] mPattern;
    private final int[] mRotate;
    private int mWidth = 0;
    private int mHeight = 0;

    private PuzzleSpec(int size){
        mPattern = new int[size];
        mRotate  = new int[size];
        Arrays.fill(mPattern,0);
        Arrays.fill(mRotate,0);
    }

    public static PuzzleSpec create(int size){
        return new PuzzleSpec(size);
    }

    public int getLeft(int index)  {
        /*
         * FIX BUG: 5204
         * BUG CAUSE: java.lang.ArrayIndexOutOfBoundsException
         * Data :2013-11-05
         */
       if(index >= mPattern.length || index < 0){
           return 0;
       }else{
           return (mPattern[index] >> 24) & MASK;
       }
    }
    public int getTop(int index)   {
        /*
         * FIX BUG: 5505
         * BUG CAUSE: java.lang.ArrayIndexOutOfBoundsException
         * Data :2013-12-09
         */
        if(index >= mPattern.length || index < 0){
            return 0;
        }
        return (mPattern[index] >> 16) & MASK;
    }

    public int getRight(int index) {
        if(index >= mPattern.length || index < 0){
            return 0;
        }
        return (mPattern[index] >> 8)  & MASK;
    }

    public int getBottom(int index){
        if(index >= mPattern.length || index < 0){
            return 0;
        }
        return (mPattern[index]) & MASK;
    }

    public int getRotate(int index){
        if(index >= mRotate.length || index < 0){
            return 0;
        }
        return mRotate[index];
    }

    public int width(int index) { return mWidth == 0 ? (getRight(index) - getLeft(index)): mWidth;}
    public int height(int index){ return mHeight == 0 ? (getBottom(index)- getTop(index)) : mHeight;}

    public int length() { return mPattern.length; }

    public Rect getRect() {
        Rect rect = new Rect();
        if (mWidth != 0 || mHeight != 0) {
            rect.set(0, 0, mWidth, mHeight);
        } else {
            int left=0, top=0, right=0, bottom=0;
            for (int i=0; i<length(); i++){
                left  = Math.min(left, getLeft(i));
                top   = Math.min(top, getTop(i));
                right = Math.max(right, getRight(i));
                bottom= Math.max(bottom, getBottom(i));
            }
            rect.set(left, top, right, bottom);
        }
        return rect;
    }

    public SpecInfo getSpecInfo(int index){
       return new PuzzleSpec.SpecInfo(getLeft(index), getTop(index), getRight(index),
                getBottom(index), width(index), height(index), getRotate(index));
    }

    ////////////////////////////////////////////////////////////////
    // SETTERS
    public void set(int index, int l, int t, int r, int b){
        int value = l & MASK;
        value = (value << 8) | (t & MASK);
        value = (value << 8) | (r & MASK);
        value = (value << 8) | (b & MASK);
        mPattern[index] = value;
    }

    public void setWidthAndHeight(int width, int height) {
        mWidth = width;
        mHeight = height;
    }

    public void set(int index, int l, int t, int r, int b, int angle){
        set(index,l,t,r,b);
        mRotate[index]   = angle;
    }

    public void reset(){
        Arrays.fill(mPattern, 0);
        Arrays.fill(mRotate,0);
        mWidth = 0;
        mHeight = 0;
    }

    public void setLeft(int index, int l)  {set(index,l,0); }
    public void setTop(int index, int t)   {set(index,t,8); }
    public void setRight(int index, int r) {set(index,r,16);}
    public void setBottom(int index, int b){set(index,b,24);}
    public void setRotate(int index, int a){mRotate[index] = a;}

    private void set(int index, int v, int bits){
        int value = mPattern[index];
        value &= ~(MASK << bits);
        value |= (v & MASK) << bits;
        mPattern[index] = value;
    }

    public static class SpecInfo {
        public final int mLeft;
        public final int mTop;
        public final int mRight;
        public final int mBottom;
        public final int mWidth;
        public final int mHeight;
        public final int mRotateDegree;
        private SpecInfo(int l, int t, int r, int b, int w, int h, int d) {
            mLeft = l;
            mTop = t;
            mRight = r;
            mBottom = b;
            mWidth = w;
            mHeight = h;
            mRotateDegree = d;
        }
        public String toString() {
            return String.format("(%d,%d)-(%d,%d) {%dx%d@%d}", mLeft,mTop,mRight,mBottom,mWidth,mHeight,mRotateDegree);
        }
    }
}
