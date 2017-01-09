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

public class ExplosionTextBubbleBox extends TextBubbleBox {
    public ExplosionTextBubbleBox(TextBubbleBoxAttribute attributes, Context context) {
        super(attributes, context);
    }

    /*public ExplosionTextBubbleBox(ViewAttributes attributes, Context context, String inputText, BubbleVertex bubbleVertex) {
        super(attributes, context, inputText, bubbleVertex);
    }*/

    @Override
    protected void buildBoxPath() {
        Path path = new Path();
        mBoxPath = path;
        path.addRect(0, 0, mBubbleVertex.p3.x - mBubbleVertex.p1.x, mBubbleVertex.p3.y - mBubbleVertex.p1.y, Path.Direction.CCW);
        mBoxPath.close();
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
        Path tempPath = getShapePath();
//        mBoxPath.transform(matrix, tempPath);
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
        Path tempPath = getShapePath();
//        mBoxPath.transform(matrix, tempPath);
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
        /*float left = mBubbleVertex.p1.x;
        float top = mBubbleVertex.p1.y;
        float right = mBubbleVertex.p3.x;
        float bottom = mBubbleVertex.p3.y;
        float width = right - left;
        float height = bottom - top;

        textRectf.left = left + width / 12;
        textRectf.right = right - width / 12;
        textRectf.top = top + height / 12;
        textRectf.bottom = bottom - height / 12;*/

        textRectf.left = mBubbleVertex.p1.x;
        textRectf.top = mBubbleVertex.p1.y;
        textRectf.right = mBubbleVertex.p3.x;
        textRectf.bottom = mBubbleVertex.p3.y;

        return textRectf;
    }

    private Path getShapePath() {
        RectF rectf = new RectF();
        mBoxPath.computeBounds(rectf, false);
        float left = mBubbleVertex.p1.x;
        float right = mBubbleVertex.p3.x;
        float width = right - left;
        float top = mBubbleVertex.p1.y;
        float bottom = mBubbleVertex.p3.y;
        float height = bottom - top;
        Path path = new Path();

        float point_01_y = height / 6F + top;
        path.moveTo(left, point_01_y);

        float point_02_x = left - width / 10F;
        float point_02_y = top - height / 10F;
        path.lineTo(point_02_x, point_02_y);

        float point_03_x = left + width / 6F;
        float point_03_y = top;
        path.lineTo(point_03_x, point_03_y);

        float point_04_x = left + width / 3F;
        float point_04_y = top - height / 5F;
        path.lineTo(point_04_x, point_04_y);

        float point_05_x = width / 2F + left;
        path.lineTo(point_05_x, point_03_y);

        float point_06_x = right - width / 3F;
        float point_06_y = top - height / 5F;
        path.lineTo(point_06_x, point_06_y);

        float point_07_x = right - width / 6F;
        path.lineTo(point_07_x, top);

        float point_08_x = right + width / 10F;
        float point_08_y = top - height / 10F;
        path.lineTo(point_08_x, point_08_y);

        float point_09_y = top + height / 4F;
        path.lineTo(right, point_09_y);

        float point_10_x = right + width / 5F;
        float point_10_y = top + height * 2 / 5F;
        path.lineTo(point_10_x, point_10_y);

        float point_11_y = top + (height * 2F) / 3F;
        path.lineTo(right, point_11_y);

        float point_12_x = right + width / 10F;
        float point_12_y = bottom + height / 8F;
        path.lineTo(point_12_x, point_12_y);

        float point_13_x = right - width / 4F;
        path.lineTo(point_13_x, bottom);

        float point_14_x = right - width / 3F;
        float point_14_y = bottom + height / 4F;
        path.lineTo(point_14_x, point_14_y);

        float point_15_x = left + width / 2F;
        path.lineTo(point_15_x, bottom);

        float point_16_x = left + width / 3F;
        float point_16_y = bottom + height / 4F;
        path.lineTo(point_16_x, point_16_y);

        float point_17_x = left + width / 5F;
        path.lineTo(point_17_x, bottom);

        float point_18_x = left - width / 8F;
        float point_18_y = bottom + height / 6F;
        path.lineTo(point_18_x, point_18_y);

        float point_19_y = bottom - height / 3F;
        path.lineTo(left, point_19_y);

        float point_20_x = left - width / 5F;
        float point_20_y = height / 2F + top;
        path.lineTo(point_20_x, point_20_y);

        path.close();

        return path;
    }

    @Override
    protected void buildTitlePath() {

    }

    /*@Override
    public void drawTitle(Canvas canvas, BubbleVertex bubbleVertex, Matrix matrix) {

    }
*/
    @Override
    protected void initializeVertex() {

    }

}
