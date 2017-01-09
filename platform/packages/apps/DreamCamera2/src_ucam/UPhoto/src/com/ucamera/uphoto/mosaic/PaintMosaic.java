/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto.mosaic;

import java.util.ArrayList;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.graphics.RectF;

public class PaintMosaic {
    private MosaicBrush mBrush;
    private Canvas mCanvas;
    private int mBrushType;
    private MosaicStroke mCurrentStroke;
    private Bitmap mCurrentShaderBmp;
    private int mBrushSize;
    private ArrayList<MosaicStroke> mMosaicStrokes = new ArrayList<MosaicStroke>();
    public PaintMosaic() {
        initClearPaint();
    }
    private Paint mPaint;
    public void createCanvas(Bitmap bitmap) {
        mCanvas = new Canvas(bitmap);
    }
    private void initClearPaint() {
        mPaint = new Paint();
        mPaint.setAntiAlias(true);
        mPaint.setStyle(Style.FILL);
        mPaint.setAlpha(0);
        mPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
    }
    public void createBrush(int type) {
        switch (type) {
        case MosaicConstant.MOSAIC_DRAW:
            mBrush = new MosaicDrawBrush();
            break;
        case MosaicConstant.MOSAIC_ERASE:
            mBrush = new MosaicEraseBrush();
            break;
        default:
            break;
        }
        mBrush.setPaintShader(mCurrentShaderBmp);
        mBrush.setBrushSize(mBrushSize);
    }
    public void paintBegin(float x, float y) {
        createBrush(mBrushType);
        mCurrentStroke = new MosaicStroke();
        mCurrentStroke.setBrush(mBrush);
        mCurrentStroke.paintBegin(mCanvas, x, y);
    }
    public void paintMove(float x1, float y1, float x2, float y2) {
        mCurrentStroke.paintMove(mCanvas, x1, y1, x2, y2);
    }
    public void paintEnd(float x, float y) {
        /*
         * FIX BUG: 6186
         * BUG COMMENT: nullpointer
         * DATE: 2014-03-28
         */
        if(mCurrentStroke != null) {
            mCurrentStroke.paintEnd(mCanvas, x, y);
        }
        mMosaicStrokes.add(mCurrentStroke);
    }
    public void setShaderBitmap(Bitmap bmp) {
        mCurrentShaderBmp = bmp;
    }
    public void updateMosaicDrawShader(Bitmap bitmap) {
        if(mCurrentShaderBmp != null && !mCurrentShaderBmp.isRecycled()) {
            mCurrentShaderBmp.recycle();
        }
        mCurrentShaderBmp = bitmap;
        //please do not delete next line.Before draw path,should clear canvas.
        mCanvas.drawRect(new Rect(0, 0, mCanvas.getWidth(), mCanvas.getHeight()), mPaint);
        for(int i = 0; i < mMosaicStrokes.size(); i++) {
            mMosaicStrokes.get(i).updateShader(bitmap);
            mMosaicStrokes.get(i).paintWholePath(mCanvas);
        }
    }
    public void setBrushSize(int size) {
        mBrushSize = size;
    }
    public void setBrushType(int type) {
        mBrushType = type;
    }
    public void clear() {
        if(mMosaicStrokes != null) {
            mMosaicStrokes.clear();
            mMosaicStrokes = null;
        }
        if(mCurrentShaderBmp != null && !mCurrentShaderBmp.isRecycled()) {
            mCurrentShaderBmp.recycle();
            mCurrentShaderBmp = null;
        }
    }
}
