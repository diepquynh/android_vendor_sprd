/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PaintFlagsDrawFilter;
import android.graphics.RectF;
import android.graphics.Bitmap.Config;

public class LabelBox extends TextBubbleBox {
    private Bitmap mNewBitmap;
    public LabelBox(LabelBoxAttribute labelBoxAttribute, Context context) {
        super(labelBoxAttribute, context);
    }

    @Override
    protected void initializeVertex() {
        int newWidth;
        int newHeight;

        /*
         * FIX BUG: 5551
         * FIX COMMENT: Avoid the null pointer exception
         * DATE: 2013-12-10
         */
        int bodyWidth = mLabelBodyBitmap != null ? mLabelBodyBitmap.getWidth() : 0;
        int bodyHeight = mLabelBodyBitmap != null ? mLabelBodyBitmap.getHeight() : 0;

        int headWidth = 0;

        int tailWidth;
        if (mLabelHeadBitmap != null && mLabelTailBitmap != null) {
            headWidth = mLabelHeadBitmap.getWidth();
            tailWidth = mLabelTailBitmap.getWidth();
            newWidth = bodyWidth + headWidth + tailWidth;
            newHeight = bodyHeight;
        } else {
            newWidth = bodyWidth;
            newHeight = bodyHeight;
        }

        mNewBitmap = Bitmap.createBitmap(newWidth, newHeight, Config.ARGB_8888);
        Canvas canvas = new Canvas(mNewBitmap);
        canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG));
        if(mLabelHeadBitmap != null && mLabelTailBitmap != null) {
            canvas.drawBitmap(mLabelHeadBitmap, 0, 0, new Paint());
            if(mLabelBodyBitmap != null) {
                canvas.drawBitmap(mLabelBodyBitmap, (int)headWidth, 0, new Paint());
            }
            canvas.drawBitmap(mLabelTailBitmap, (int)(headWidth + bodyWidth), 0, new Paint());

            if(mPath != null) {
                mPath.moveTo(headWidth, bodyHeight / 2 + mFontHeight / 3);
                mPath.lineTo(headWidth + bodyWidth, bodyHeight / 2 + mFontHeight / 3);
                canvas.drawTextOnPath(mInputText, mPath, 0, 0, mTextPaint);
            }
        } else {
            if(mLabelBodyBitmap != null) {
                canvas.drawBitmap(mLabelBodyBitmap, 0, 0, new Paint());
            }
            if(mPath != null) {
                mPath.moveTo(0, bodyHeight / 2 + mFontHeight / 3);
                mPath.lineTo(bodyWidth, bodyHeight / 2 + mFontHeight / 3);
                canvas.drawTextOnPath(mInputText, mPath, 0, 0, mTextPaint);
            }
        }

        if(mLabelHeadBitmap != null && mLabelTailBitmap != null) {
            mLabelHeadBitmap.recycle();
            mLabelTailBitmap.recycle();
        }
        if(mLabelBodyBitmap != null) {
            mLabelBodyBitmap.recycle();
        }
        if(mPath != null) {
            mPath.close();
        }

        canvas.save(Canvas.ALL_SAVE_FLAG);
        canvas.restore();
    }

    public void drawLabel(Canvas canvas, BubbleVertex bubbleVertex, Matrix matrix) {
        canvas.drawBitmap(mNewBitmap, matrix, new Paint());
    }

    @Override
    protected void buildTitlePath() {

    }

    @Override
    public void drawFrame(Canvas canvas, Matrix matrix, BubbleVertex bubbleVertex) {

    }

    @Override
    public void drawBox(Canvas canvas, Matrix matrix, BubbleVertex bubbleVertex) {

    }

    @Override
    public RectF getTextRect() {
        return null;
    }

    @Override
    protected void buildBoxPath() {

    }

    @Override
    protected void buildHandlePath() {

    }
}
