/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

public class MyQuadTo {
    public MyPoint cPoint;
    public MyPoint ePoint;
    public float[] brushData;

    public MyQuadTo(MyPoint cPoint, MyPoint ePoint) {
        this.cPoint = new MyPoint();
        this.ePoint = new MyPoint();

        this.cPoint.set(cPoint);
        this.ePoint.set(ePoint);

        this.brushData = null;
    }

    public void storeBrushData(float[] brushData) {
        this.brushData = brushData;
    }
}
