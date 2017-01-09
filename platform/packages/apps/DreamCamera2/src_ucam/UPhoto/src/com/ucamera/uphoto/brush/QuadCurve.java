/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.PointF;

public class QuadCurve {
    static final int BEZIER_MAX_SPLINE_DECOMPOSE = 100;
    static final int MAX_POINT = 500;
    int mNumPoints;
    static PointF[] mPolygonPoints = new PointF[500];
    private float mSpacing;
    Bezier[] mBezierBuf;

    public QuadCurve() {
        this.mBezierBuf = new Bezier[100];
        this.mSpacing = 2.5F;
        int i = 0;
        int polygonPointsLen = mPolygonPoints.length;
        do {
            if(i < 100) {
                Bezier bezier = new Bezier();
                mBezierBuf[i] = bezier;
            }
            PointF pointf = new PointF();
            mPolygonPoints[i] = pointf;

            i++;
        } while (i < polygonPointsLen);
    }

    public class Bezier {
        PointF[] point;

        public Bezier() {
            point = new PointF[8];
            int index = 0;
            int len = point.length;
            do {
                PointF localPointF = new PointF();
                this.point[index] = localPointF;

                index++;
            } while (index < len);
        }
    }

    private void bezierSplit(Bezier bezier, Bezier bezier1, Bezier bezier2) {
        float sPointX = bezier.point[0].x;
        float cPointX = bezier.point[1].x;
        float ePointX = bezier.point[3].x;
        float sPointMidX = (sPointX + cPointX) / 2F;
        float ePointMidX = (cPointX + ePointX) / 2F;
        bezier1.point[0].x = sPointX;
        bezier1.point[1].x = sPointMidX;
        bezier1.point[2].x = sPointMidX;

        bezier2.point[1].x = ePointMidX;
        bezier2.point[2].x = ePointMidX;
        bezier2.point[3].x = ePointX;

        PointF eBezier1PointF = bezier1.point[3];
        PointF sBezier2PointF = bezier2.point[0];
        float newCPointMidX = (sPointMidX + ePointMidX) / 2F;
        sBezier2PointF.x = newCPointMidX;
        eBezier1PointF.x = newCPointMidX;

        float sPointY = bezier.point[0].y;
        float cPointY = bezier.point[1].y;
        float ePointY = bezier.point[3].y;
        float sPointMidY = (sPointY + cPointY) / 2F;
        float ePointMidY = (cPointY + ePointY) / 2F;
        bezier1.point[0].y = sPointY;
        bezier1.point[1].y = sPointMidY;
        bezier1.point[2].y = sPointMidY;
        bezier2.point[1].y = ePointMidY;
        bezier2.point[2].y = ePointMidY;
        bezier2.point[3].y = ePointY;
        float newCPointMidY = (sPointMidY + ePointMidY) / 2F;
        sBezier2PointF.y = newCPointMidY;
        eBezier1PointF.y = newCPointMidY;
    }

    private void addPoint(PointF pointf) {
        if(mNumPoints > 0) {
            if(mNumPoints < 500) {
                if(!isIdentical(mPolygonPoints[mNumPoints], pointf)) {
                    mPolygonPoints[mNumPoints].set(pointf);
                    mNumPoints++;
                }
            }
        } else {
            mPolygonPoints[mNumPoints].set(pointf);
            mNumPoints++;
        }
    }

    private float distance(PointF pointf, PointF pointf1) {
        float distanceX = Math.abs(pointf.x - pointf1.x);
        float distanceY = Math.abs(pointf.y - pointf1.y);

        return distanceX + distanceY;
    }

    private boolean isIdentical(PointF pointf, PointF pointf1) {
        if(Math.abs(pointf.x - pointf1.x) < 0.5D) {
            if (Math.abs(pointf.y - pointf1.y) < 0.5D) {
                return true;
            }
        }

        return false;
    }

    private boolean isShortEnough(PointF pointf, PointF pointf1) {
        boolean flag = false;
        float realDistance = distance(pointf, pointf1);
        if(realDistance < mSpacing) {
            flag = true;
        }

        return flag;
    }

    public void decomposePoint(PointF sPointF, PointF cPointF, PointF ePointF) {
        int i = 0;
        Bezier bezier = mBezierBuf[0];
        mNumPoints = 0;
        bezier.point[0].set(sPointF);
        bezier.point[1].set(cPointF);
        bezier.point[2].set(cPointF);
        bezier.point[3].set(ePointF);
        addPoint(sPointF);

        do {
            Bezier bezier1 = mBezierBuf[i];
            PointF sBezierPointF = bezier1.point[0];
            PointF cBezierPointF = bezier1.point[1];
            PointF cBezierPointF2 = bezier1.point[2];
            PointF eBezierPointF = bezier1.point[3];
            if(isShortEnough(sBezierPointF, cBezierPointF) && isShortEnough(cBezierPointF2, eBezierPointF)) {
                addPoint(cBezierPointF2);
                addPoint(eBezierPointF);
                i--;
            } else {
                Bezier abezier[] = mBezierBuf;
                int i1 = i + 1;
                Bezier bezier2 = abezier[i1];
                bezierSplit(bezier1, bezier2, bezier1);
                i++;
            }
        } while (i >= 0);
    }

    public int getPointNum() {
        return mNumPoints;
    }

    public PointF[] getPoints() {
        return mPolygonPoints;
    }

    public void setSpacing(float spacing) {
        this.mSpacing = spacing;
    }
}
