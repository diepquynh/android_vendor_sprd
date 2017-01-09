/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Path;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

public class PreviewBrushView extends View {
    private BaseBrush mBrush;
    private Path mPath;
    private float mYdensity;

    public PreviewBrushView(Context context, AttributeSet attributeset) {
        super(context, attributeset);

        mPath = new Path();
        getDisplayDensity(context);
    }

    public PreviewBrushView(Context context, AttributeSet attributeset, int i) {
        super(context, attributeset, i);

        mPath = new Path();
        getDisplayDensity(context);
    }

    private static void generateRainbowBrush(BaseBrush brush, Canvas canvas, int width, int height) {
        Path path = new Path();
        float drawWidth = width - 10;
        float space = drawWidth / 32;
        float index = 5F;
        do {
            path.reset();
            path.moveTo(index, height / 2);
            path.lineTo(index + space, height / 2);
            brush.updateBrush();
            brush.drawStroke(canvas, path);
            index += space;
        } while(index < drawWidth);
    }


    public void getDisplayDensity(Context context) {
        mYdensity = context.getApplicationContext().getResources().getDisplayMetrics().density;
    }

    protected void onDraw(Canvas canvas) {
        float height = 70F * mYdensity;

        float halfHeight = height / 2F;
        /*SPRD: CID 111159 (#1 of 1): Result is not floating-point (UNINTENDED_INTEGER_DIVISION)
        float endX1 = getMeasuredWidth() / 2;
        */
        float endX1 = getMeasuredWidth() / 2F;
        /* @} */
        float endX2 = getMeasuredWidth() - 20F;

        float controlX1 = (20F + endX1) / 2F;
        float controlY1 = halfHeight / 2F;
        float controlX2 = (endX1 + endX2) / 2F;
        float controlY2 = (halfHeight + height) / 2F;

        int canvasWidth = canvas.getWidth();

        if(mBrush != null) {
            int brushStyle = mBrush.mBrushStyle;

            if(brushStyle == BrushConstant.RainbowBrush) {
                generateRainbowBrush(mBrush, canvas, canvasWidth, (int)height);
            } else {
                if(mBrush.getRandomColorPicker() != null) {
                    mBrush.getRandomColorPicker().resetPicker();
                }
                mBrush.setAlpha(255);
                mBrush.prepareBrush();
                if(brushStyle > BrushConstant.DividingLine) {
                    MyPoint sPoint = new MyPoint();
                    sPoint.set(20, halfHeight);

                    MyPoint cPoint = new MyPoint();
                    cPoint.set(controlX1, controlY1);

                    MyPoint ePoint = new MyPoint();
                    ePoint.set(endX1, halfHeight);

                    mBrush.drawStroke(canvas, sPoint, cPoint, ePoint);

                    MyPoint sPoint2 = new MyPoint();
                    sPoint2.set(endX1, halfHeight);

                    MyPoint cPoint2 = new MyPoint();
                    cPoint2.set(controlX2, controlY2);

                    MyPoint ePoint2 = new MyPoint();
                    ePoint2.set(endX2, halfHeight);

                    mBrush.drawStroke(canvas, sPoint2, cPoint2, ePoint2);
                } else {
                    mPath.reset();

                    mPath.moveTo(20F, halfHeight);
                    mPath.quadTo(controlX1, controlY1, endX1, halfHeight);
                    mPath.quadTo(controlX2, controlY2, endX2, halfHeight);

                    mBrush.drawStroke(canvas, mPath);
                }
                mBrush.endStroke();
            }
        }
    }

    public void setBrush(BaseBrush brush) {
        mBrush = brush;
    }
}
