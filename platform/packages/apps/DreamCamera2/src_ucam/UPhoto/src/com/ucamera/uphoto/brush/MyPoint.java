/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

public class MyPoint {
    public MyPoint() {

    }

    public MyPoint(float x, float y) {
        mX = x;
        mY = y;
    }

    public void set(float x, float y) {
        mX = x;
        mY = y;
    }

    public void set(MyPoint point) {
        mX= point.mX;
        mY = point.mY;
    }

    public float mX;
    public float mY;
}
