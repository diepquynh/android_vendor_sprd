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

public class CloundTextBubbleBox extends TextBubbleBox {
    public CloundTextBubbleBox(TextBubbleBoxAttribute attributes, Context context) {
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
        getArrowPath();
    }

    @Override
    public void drawBox(Canvas canvas, Matrix matrix, BubbleVertex bubbleVertex) {
        Paint paint = mBoxPaint;
        Path tempPath = new Path();
        mBoxPath.transform(matrix, tempPath);
        tempPath.close();
        canvas.drawPath(tempPath, paint);

        RectF rectf = new RectF();
        mBoxPath.computeBounds(rectf, false);
        float left = mBubbleVertex.p1.x;
        float right = mBubbleVertex.p3.x;
        float width = right - left;
        float top = mBubbleVertex.p1.y;
        float bottom = mBubbleVertex.p3.y;
        float height = bottom - top;

        float left_01 = left;
        float top_01 = top - height * 0.2f;
        float right_01 = left + width / 3;
        float bottom_01 = top + height * 0.2f;
        RectF rectf_01 = new RectF(left_01, top_01, right_01, bottom_01);
        canvas.drawOval(rectf_01, paint);

        float left_02 = left + width * 3 / 10;
        float top_02 = top - height * 0.3f;
        float right_02 = left + width * 7 / 10;
        float bottom_02 = top + height * 0.1f;
        RectF rectf_02 = new RectF(left_02, top_02, right_02, bottom_02);
        canvas.drawOval(rectf_02, paint);

        float left_03 = left + width * 2 / 3;
        float top_03 = top - height * 0.2f;
        float right_03 = right;
        float bottom_03 = top + height * 0.2f;
        RectF rectf_03 = new RectF(left_03, top_03, right_03, bottom_03);
        canvas.drawOval(rectf_03, paint);

        float left_04 = right - width * 0.1f;
        float top_04 = top;
        float right_04 = right + width * 0.1f;
        float bottom_04 = top + height * 0.5f;
        RectF rectf_04 = new RectF(left_04, top_04, right_04, bottom_04);
        canvas.drawOval(rectf_04, paint);

        float left_05 = right - width * 0.1f;
        float top_05 = top + height * 0.5f;
        float right_05 = right + width * 0.1f;
        float bottom_05 = bottom;
        RectF rectf_05 = new RectF(left_05, top_05, right_05, bottom_05);
        canvas.drawOval(rectf_05, paint);

        float left_06 = left + width * 2 / 3;
        float top_06 = bottom - height * 0.2f;
        float right_06 = right;
        float bottom_06 = bottom + height * 0.2f;
        RectF rectf_06 = new RectF(left_06, top_06, right_06, bottom_06);
        canvas.drawOval(rectf_06, paint);

        float left_07 = left + width * 3 / 10;
        float top_07 = bottom - height * 0.1f;
        float right_07 = left + width * 7 / 10;
        float bottom_07 = bottom + height * 0.3f;
        RectF rectf_07 = new RectF(left_07, top_07, right_07, bottom_07);
        canvas.drawOval(rectf_07, paint);

        float left_08 = left;
        float top_08 = bottom - height * 0.2f;
        float right_08 = left + width / 3;
        float bottom_08 = bottom + height * 0.2f;
        RectF rectf_08 = new RectF(left_08, top_08, right_08, bottom_08);
        canvas.drawOval(rectf_08, paint);

        float left_09 = left - width * 0.1f;
        float top_09 = top + height * 0.5f;
        float right_09 = left + width * 0.1f;
        float bottom_09 = bottom;
        RectF rectf_09 = new RectF(left_09, top_09, right_09, bottom_09);
        canvas.drawOval(rectf_09, paint);

        float left_10 = left - width * 0.1f;
        float top_10 = top;
        float right_10 = left + width * 0.1f;
        float bottom_10 = top + height * 0.5f;
        RectF rectf_10 = new RectF(left_10, top_10, right_10, bottom_10);
        canvas.drawOval(rectf_10, paint);

        canvas.drawPath(mHandlePath, paint);
    }

    @Override
    public void drawFrame(Canvas canvas, Matrix matrix, BubbleVertex bubbleVertex) {
        Paint paint = mStrokePaint;
        RectF rectf = new RectF();
        mBoxPath.computeBounds(rectf, false);
        float left = mBubbleVertex.p1.x;
        float right = mBubbleVertex.p3.x;
        float width = right - left;
        float top = mBubbleVertex.p1.y;
        float bottom = mBubbleVertex.p3.y;
        float height = bottom - top;

        float left_01 = left;
        float top_01 = top - height * 0.2f;
        float right_01 = left + width / 3;
        float bottom_01 = top + height * 0.2f;
        RectF rectf_01 = new RectF(left_01, top_01, right_01, bottom_01);
        canvas.drawOval(rectf_01, paint);

        float left_02 = left + width * 3 / 10;
        float top_02 = top - height * 0.3f;
        float right_02 = left + width * 7 / 10;
        float bottom_02 = top + height * 0.1f;
        RectF rectf_02 = new RectF(left_02, top_02, right_02, bottom_02);
        canvas.drawOval(rectf_02, paint);

        float left_03 = left + width * 2 / 3;
        float top_03 = top - height * 0.2f;
        float right_03 = right;
        float bottom_03 = top + height * 0.2f;
        RectF rectf_03 = new RectF(left_03, top_03, right_03, bottom_03);
        canvas.drawOval(rectf_03, paint);

        float left_04 = right - width * 0.1f;
        float top_04 = top;
        float right_04 = right + width * 0.1f;
        float bottom_04 = top + height * 0.5f;
        RectF rectf_04 = new RectF(left_04, top_04, right_04, bottom_04);
        canvas.drawOval(rectf_04, paint);

        float left_05 = right - width * 0.1f;
        float top_05 = top + height * 0.5f;
        float right_05 = right + width * 0.1f;
        float bottom_05 = bottom;
        RectF rectf_05 = new RectF(left_05, top_05, right_05, bottom_05);
        canvas.drawOval(rectf_05, paint);

        float left_06 = left + width * 2 / 3;
        float top_06 = bottom - height * 0.2f;
        float right_06 = right;
        float bottom_06 = bottom + height * 0.2f;
        RectF rectf_06 = new RectF(left_06, top_06, right_06, bottom_06);
        canvas.drawOval(rectf_06, paint);

        float left_07 = left + width * 3 / 10;
        float top_07 = bottom - height * 0.1f;
        float right_07 = left + width * 7 / 10;
        float bottom_07 = bottom + height * 0.3f;
        RectF rectf_07 = new RectF(left_07, top_07, right_07, bottom_07);
        canvas.drawOval(rectf_07, paint);

        float left_08 = left;
        float top_08 = bottom - height * 0.2f;
        float right_08 = left + width / 3;
        float bottom_08 = bottom + height * 0.2f;
        RectF rectf_08 = new RectF(left_08, top_08, right_08, bottom_08);
        canvas.drawOval(rectf_08, paint);

        float left_09 = left - width * 0.1f;
        float top_09 = top + height * 0.5f;
        float right_09 = left + width * 0.1f;
        float bottom_09 = bottom;
        RectF rectf_09 = new RectF(left_09, top_09, right_09, bottom_09);
        canvas.drawOval(rectf_09, paint);

        float left_10 = left - width * 0.1f;
        float top_10 = top;
        float right_10 = left + width * 0.1f;
        float bottom_10 = top + height * 0.5f;
        RectF rectf_10 = new RectF(left_10, top_10, right_10, bottom_10);
        canvas.drawOval(rectf_10, paint);


        canvas.drawPath(getArrowPath(), mStrokePaint);
    }

    @Override
    public RectF getTextRect() {
        RectF textRectf = new RectF();
        textRectf.left = mBubbleVertex.p1.x;
        textRectf.top = mBubbleVertex.p1.y;
        textRectf.right = mBubbleVertex.p3.x;
        textRectf.bottom = mBubbleVertex.p3.y;

        return textRectf;
    }

    private Path getArrowPath() {
        float left = mBubbleVertex.p1.x;
        float top = mBubbleVertex.p1.y;
        float right = mBubbleVertex.p3.x;
        float bottom = mBubbleVertex.p3.y;
        float arrowX = mBubbleVertex.p5.x;
        float arrowY = mBubbleVertex.p5.y;

        Path path = new Path();
        mHandlePath = path;
        path.moveTo(left, top);
        float centerX = (left + right) / 2F;
        float centerY = (top + bottom) / 2F;
        float distance = (float) Math.sqrt((centerX - arrowX) * (centerX - arrowX) + (centerY - arrowY) * (centerY - arrowY));

        float radius = 0F;
        final float CD = 8F; //Common Difference
        float tempDistance = 0F;
        float ratioX = (centerX - arrowX) / distance;
        float ratioY = (centerY - arrowY) / distance;
        do {
            radius += CD;
            float coordinateX = arrowX + tempDistance * ratioX;
            float coordinateY = arrowY + tempDistance * ratioY;
            path.addCircle(coordinateX, coordinateY, radius, Path.Direction.CCW);
            float distanceCD = radius * 4F + CD;
            tempDistance += distanceCD;
        } while(tempDistance < distance);

        return mHandlePath;
    }

    @Override
    protected void buildTitlePath() {

    }

    @Override
    protected void initializeVertex() {

    }
}
