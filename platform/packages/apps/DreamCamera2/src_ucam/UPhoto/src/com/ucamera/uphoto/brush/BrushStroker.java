/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.Canvas;
import android.graphics.Path;

import java.util.ArrayList;
import java.util.Iterator;

public class BrushStroker {
    private BaseBrush mBrush;
    public MyPoint mFirstPoint;
    private Path mStrokePath;
    private MyPoint mStartPoint;
    private MyPoint mCtrlPoint;
    private MyPoint mEndPoint;
    public ArrayList<MyQuadTo> mMyQuadTos;

    public BrushStroker() {
        mFirstPoint = new MyPoint();
        mStartPoint = new MyPoint();
        mCtrlPoint = new MyPoint();
        mEndPoint = new MyPoint();
        mStrokePath = new Path();
        mMyQuadTos = new ArrayList<MyQuadTo>();
        mBrush = null;
    }

    public void setBrush(BaseBrush brush) {
        mBrush = brush;
    }

    public void strokeFinish() {
        mBrush.endStroke();
    }

    public void strokeFrom(float x, float y) {
        mFirstPoint.mX = x;
        mFirstPoint.mY = y;
        mStartPoint.set(x, y);
        mCtrlPoint.set(x, y);
        mBrush.prepareBrush();
        mStrokePath.reset();
        mStrokePath.moveTo(x, y);
    }

    public void strokeTo(Canvas canvas, float x, float y) {
        mEndPoint.mX = (mCtrlPoint.mX + x) / 2F;
        mEndPoint.mY = (mCtrlPoint.mY + y) / 2F;

        if(mBrush.mBrushStyle == BrushConstant.RainbowBrush) {
            mStrokePath.reset();
            mStrokePath.moveTo(mStartPoint.mX, mStartPoint.mY);
            mStrokePath.quadTo(mCtrlPoint.mX, mCtrlPoint.mY, mEndPoint.mX, mEndPoint.mY);
        } else {
            mStrokePath.quadTo(mCtrlPoint.mX, mCtrlPoint.mY, mEndPoint.mX, mEndPoint.mY);
        }
        mBrush.updateBrush();

        /**
         * FIX BUG: 935
         * BUG CAUSE: Control point coordinate sets incorrectly.
         * FIX COMMENT: At first save the current control point coordinates, and then reset the control point coordinates
         * DATE: 2012-04-26
         */
        MyQuadTo myQuadTo = new MyQuadTo(mCtrlPoint, mEndPoint);
        myQuadTo.storeBrushData(mBrush.getBrushData());
        mMyQuadTos.add(myQuadTo);

        if(mBrush.mBrushStyle < BrushConstant.DividingLine) {
            mBrush.drawStroke(canvas, mStrokePath);
            mStartPoint.set(mEndPoint);
            mCtrlPoint.set(x, y);
        } else {
            mBrush.drawStroke(canvas, mStartPoint, mCtrlPoint, mEndPoint);
            mStartPoint.set(mEndPoint);
            mCtrlPoint.set(x, y);
        }

    }

    public void reDraw(Canvas canvas) {
        if(mBrush.mBrushStyle > BrushConstant.DividingLine || mBrush.mBrushStyle == BrushConstant.RainbowBrush) {
            Iterator<MyQuadTo> iterator = mMyQuadTos.iterator();
            mStartPoint.set(mFirstPoint);
            while (iterator.hasNext()) {
                MyQuadTo myQuadTo = (MyQuadTo)iterator.next();
                redrawStrokeTo(canvas, myQuadTo);
            }
            mBrush.endStroke();
        } else {
            mBrush.drawStroke(canvas, mStrokePath);
            mBrush.endStroke();
        }
    }

    public void redrawStrokeTo(Canvas canvas, MyQuadTo myQuadTo) {
        /**
         * FIX BUG: 937
         * BUG CAUSE: Between the general brush and rainbow brush, use the same method of painting
         * FIX COMMENT: Rainbow brush draws line repeatedly.
         * DATE: 2012-04-26
         */
        if(mBrush.mBrushStyle == BrushConstant.RainbowBrush) {
            mStrokePath.reset();
            mStrokePath.moveTo(mStartPoint.mX, mStartPoint.mY);
            mStrokePath.quadTo(myQuadTo.cPoint.mX, myQuadTo.cPoint.mY, myQuadTo.ePoint.mX, myQuadTo.ePoint.mY);

            mBrush.restoreBrush(myQuadTo.brushData);
            mBrush.drawStroke(canvas, mStrokePath);
        } else {
            mBrush.drawStroke(canvas, mStartPoint, myQuadTo.cPoint, myQuadTo.ePoint);
        }

        mBrush.endStroke();
        mStartPoint.set(myQuadTo.ePoint.mX, myQuadTo.ePoint.mY);
    }
}
