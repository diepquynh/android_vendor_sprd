/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;

public class RectTextBubbleBox extends TextBubbleBox {
    public RectTextBubbleBox(TextBubbleBoxAttribute attributes, Context context) {
        super(attributes, context);
    }

    @Override
    protected void buildBoxPath() {
        Path path = new Path();
        mBoxPath = path;
        path.addRect(0, 0, mBubbleVertex.p3.x - mBubbleVertex.p1.x, mBubbleVertex.p3.y - mBubbleVertex.p1.y, Path.Direction.CCW);
    }

    @Override
    protected void buildHandlePath() {
        Path path = new Path();
        mHandlePath = path;
        mHandlePath.moveTo(mBubbleVertex.p5.x, mBubbleVertex.p5.y);
        mHandlePath.lineTo(mBubbleVertex.p6.x, mBubbleVertex.p6.y);
        mHandlePath.lineTo(mBubbleVertex.p7.x, mBubbleVertex.p7.y);

        mHandlePath.close();
    }

    @Override
    public void drawBox(Canvas canvas, Matrix matrix, BubbleVertex bubbleVertex) {
        Paint paint = mBoxPaint;
        Paint paint1 = new Paint(paint);

        Path tempPath = new Path();
        mBoxPath.transform(matrix, tempPath);
        tempPath.close();
        canvas.drawPath(tempPath, paint1);

        Path tempPath2 = new Path();
        mHandlePath = tempPath2;
        mHandlePath.moveTo(mBubbleVertex.p5.x, mBubbleVertex.p5.y);
        mHandlePath.lineTo(mBubbleVertex.p6.x, mBubbleVertex.p6.y);
        mHandlePath.lineTo(mBubbleVertex.p7.x, mBubbleVertex.p7.y);
        mHandlePath.close();
        canvas.drawPath(tempPath2, paint1);
    }

    @Override
    public void drawFrame(Canvas canvas, Matrix matrix, BubbleVertex bubbleVertex) {
        Path tempPath = new Path();
        mBoxPath.transform(matrix, tempPath);
        tempPath.close();
        canvas.drawPath(tempPath, mStrokePaint);

        Path tempPath2 = new Path();
        mHandlePath = tempPath2;
        mHandlePath.moveTo(mBubbleVertex.p5.x, mBubbleVertex.p5.y);
        mHandlePath.lineTo(mBubbleVertex.p6.x, mBubbleVertex.p6.y);
        mHandlePath.lineTo(mBubbleVertex.p7.x, mBubbleVertex.p7.y);
        mHandlePath.close();
        canvas.drawPath(tempPath2, mStrokePaint);
    }

    @Override
    public RectF getTextRect() {
        RectF textRectf = new RectF();
        float left = mBubbleVertex.p1.x;
        float top = mBubbleVertex.p1.y;
        float right = mBubbleVertex.p3.x;
        float bottom = mBubbleVertex.p3.y;
        float width = right - left;
        float height = bottom - top;

        textRectf.left = left + width / 30;
        textRectf.right = right - width / 30;
        textRectf.top = top + height / 30;
        textRectf.bottom = bottom - height / 30;

        return textRectf;
    }

    @Override
    protected void buildTitlePath() {

    }

    @Override
    protected void initializeVertex() {

    }

}
