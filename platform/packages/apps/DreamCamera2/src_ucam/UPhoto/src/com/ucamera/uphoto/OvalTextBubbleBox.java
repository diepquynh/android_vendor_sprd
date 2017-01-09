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

public class OvalTextBubbleBox extends TextBubbleBox {

    public OvalTextBubbleBox(TextBubbleBoxAttribute textBubbleBoxAttribute, Context context) {
        super(textBubbleBoxAttribute, context);
    }

    @Override
    protected void buildBoxPath() {
        Path path = new Path();
        mBoxPath = path;
        RectF rectF = new RectF(0, 0, mBubbleVertex.p3.x - mBubbleVertex.p1.x, mBubbleVertex.p3.y - mBubbleVertex.p1.y);
        path.addOval(rectF, Path.Direction.CCW);
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
        float centerX = (left + right) / 2F;
        float centerY = (top + bottom) / 2F;

        double ratio = Math.cos(Math.toRadians(45));

        float halfWidth = width / 2F;
        float tempWidth = (float) (halfWidth * ratio);
        float halfHeight = height / 2F;
        float tempHeight = (float) (halfHeight * ratio);

        textRectf.left = centerX - tempWidth;
        textRectf.right = centerX + tempWidth;
        textRectf.top = centerY - tempHeight;
        textRectf.bottom = centerY + tempHeight;

        return textRectf;
    }

    @Override
    protected void buildTitlePath() {

    }

    @Override
    protected void initializeVertex() {

    }
}
