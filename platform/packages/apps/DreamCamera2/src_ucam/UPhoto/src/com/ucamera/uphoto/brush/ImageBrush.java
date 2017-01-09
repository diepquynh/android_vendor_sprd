/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PointF;
import android.graphics.Rect;

import java.util.Random;

public class ImageBrush extends BaseBrush {
    /*SPRD: CID 109379 (#1 of 1): UrF: Unread field (FB.URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD)
    protected Random mRandom;
    */
    private int mAdjustedAlpha;
    public int mSpacing;
    Rect mBound;
    PointF sp;
    PointF cp;
    PointF ep;

    public ImageBrush(int brushStyle, ImageBrushManager imageBrushManager) {
        /*SPRD: CID 109379 (#1 of 1): UrF: Unread field (FB.URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD)
        mRandom = new Random();
        */
        mBound = new Rect();
        sp = new PointF();
        cp = new PointF();
        ep = new PointF();
        mBrushStyle = brushStyle;
        mBrushSize = 24F;
        if(imageBrushManager != null) {
            mImageBrushManager = imageBrushManager;
            mBrushMaxSize = imageBrushManager.getMaxSize(brushStyle);
            mBrushMinSize = imageBrushManager.getMinSize(brushStyle);
        } else {
            mBrushMaxSize = 50;
            mBrushMinSize = 10;
        }
        mBrushAlphaValue = 50;
        mAdjustedAlpha = 50;
    }

    private void loadBrushBitmap() {
        if(mBrushBitmap == null) {
            mBrushBitmap = mImageBrushManager.getImageBrushBtimap(mBrushStyle, (int)mBrushSize, mBrushColor);
            if(mBrushBitmap != null) {
                mBrushBitmapWidth = mBrushBitmap.getWidth();
                mBrushBitmapHeight = mBrushBitmap.getHeight();
            }

            mBound.set(0, 0, mBrushBitmapWidth, mBrushBitmapHeight);
        }
    }

    public Rect drawStroke(Canvas canvas, MyPoint sPoint, MyPoint cPoint, MyPoint ePoint) {
        sp.set(sPoint.mX, sPoint.mY);
        cp.set(cPoint.mX, cPoint.mY);
        ep.set(ePoint.mX, ePoint.mY);

        if(mQuadCurve == null) {
            mQuadCurve = new QuadCurve();
        }
        mSpacing= mImageBrushManager.getSpacing(mBrushStyle);
        mQuadCurve.setSpacing(mSpacing);
        mQuadCurve.decomposePoint(sp, cp, ep);
        int pointNum = mQuadCurve.getPointNum();
        PointF pointF[] = mQuadCurve.getPoints();
        mDirtyRect.setEmpty();
        loadBrushBitmap();
        if(mBrushBitmap == null) {
            return null;
        }

        int index = 0;
        int offsetX = 0;
        int offsetY = 0;
        do {
            offsetX = (int)(pointF[index].x - mBrushBitmapWidth / 2);
            offsetY = (int)(pointF[index].y - mBrushBitmapHeight / 2);
            canvas.drawBitmap(mBrushBitmap, offsetX, offsetY, mBrushPaint);
            mBound.offsetTo(offsetX, offsetY);
            mDirtyRect.union(mBound);
            index++;
        } while(index < pointNum);

        return mDirtyRect;
    }

    public void prepareBrush() {
        if(mBrushMode == BrushConstant.BrushModeRandom) {
            mBrushColor = randomColor();
        }
        int spacing = mImageBrushManager.getSpacing(mBrushStyle);
        float alphaScale = mImageBrushManager.getAlphaScale(mBrushStyle);

        if(mBrushAlphaValue > 210) {
            mAdjustedAlpha = mBrushAlphaValue;
        } else if(spacing > mBrushSize) {
            mAdjustedAlpha = mBrushAlphaValue;
        } else {
            mAdjustedAlpha = (int)((float)mBrushAlphaValue / alphaScale);
        }

        mBrushPaint.setAlpha(mAdjustedAlpha);
        mBrushPaint.setStrokeWidth(mBrushSize);
    }

    protected int randomColor() {
        int brushColor = Color.RED;
        if(mRandomColorPicker != null) {
            brushColor = mRandomColorPicker.getRandomColor();
        }

        return brushColor;
    }

    public void endStroke() {
//        mBrushBtimap.recycle();
        mBrushBitmap = null;
    }

    public void setAlpha(int brushAlphaValue) {
        mBrushAlphaValue = brushAlphaValue;
    }

    public void setImageBurshManager(ImageBrushManager imageBrushManager) {
        mImageBrushManager = imageBrushManager;
        mBrushMaxSize = imageBrushManager.getMaxSize(mBrushStyle);
        mBrushMinSize = imageBrushManager.getMinSize(mBrushStyle);
    }
}
