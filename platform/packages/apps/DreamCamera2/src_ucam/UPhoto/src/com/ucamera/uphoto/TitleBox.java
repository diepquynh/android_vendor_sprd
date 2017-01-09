/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Shader;

public class TitleBox extends TextBubbleBox {
    private Path mPath;
    private Paint mTextPaint;
    private Paint mStrokePaint;
    private RectF mPathRectF;

    private int mTextColor;
    private int mGradientColor;

    public TitleBox(Context context, ViewAttributes attributes) {
        super(context, attributes);
    }

    protected void initializeVertex() {
        mPath = new Path();

        mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mTextPaint.setStyle(Paint.Style.FILL);
        mTextPaint.setTextAlign(Paint.Align.CENTER);
        float textSize = 42 * ImageEditDesc.getLabelScaleRatio();
        mTextPaint.setTextSize(textSize);

        mStrokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mStrokePaint.setStyle(Paint.Style.STROKE);
        mStrokePaint.setStrokeJoin(Paint.Join.ROUND);
        mStrokePaint.setTextAlign(Paint.Align.CENTER);

        String textColor = mAttributes.getTextColor();
        if(textColor != null && textColor.startsWith("#")) {
            mTextColor = Color.parseColor(textColor);
        }
        mTextPaint.setColor(mTextColor);
        mTextPaint.setTypeface(FontResource.createTypeface(mContext, mAttributes.getFont()));

        int outlineColorId = 0;
        String outline = mAttributes.getOutline();
        if(outline != null && outline.startsWith("#")) {
            outlineColorId = Color.parseColor(outline);
        }
        mStrokePaint.setColor(outlineColorId);
        mStrokePaint.setStrokeWidth(mAttributes.getStrokeWidth());

        String gradientEnd = mAttributes.getGradientEnd();
        if(gradientEnd != null && gradientEnd.startsWith("#")) {
            mGradientColor = Color.parseColor(gradientEnd);
        }

        mPath.setFillType(Path.FillType.WINDING);
        String inputText = mAttributes.getDrawText();
        int textLen = inputText.length();
        mTextPaint.getTextPath(inputText, 0, textLen, 0, 0, mPath);
        mPathRectF = new RectF();
        mPath.computeBounds(mPathRectF, true);

        int limitWidth = ImageEditDesc.getScreenWidth() - 30;
        if(mPathRectF.width() > limitWidth) {
            StringBuilder stringBuilder = new StringBuilder();
            int index = 0;
            float tempMaxWidth = 0;
            /*
             * FIX BUG: 1439
             * BUG CAUSE: index value is more than inputText length.
             * FIX COMMENT: If index value is more than inputText length, will stop loop;
             * DATE: 2012-08-10
             */
            while (tempMaxWidth < limitWidth && index < textLen) {
                char character = inputText.charAt(index);
                String singleStr = String.valueOf(character);
                stringBuilder.append(singleStr);
                tempMaxWidth += mTextPaint.measureText(singleStr);
                index++;
            }
            mPath.reset();
            inputText = stringBuilder.toString();
            mAttributes.setDrawText(inputText);
            textLen = inputText.length();
            mTextPaint.getTextPath(inputText, 0, textLen, 0, 0, mPath);
            mPath.computeBounds(mPathRectF, true);
        }

        mPathRectF.inset(-2f, -2f);
        float rectLeft = -mPathRectF.left;
        float rectTop = -mPathRectF.top;
        mPath.offset(rectLeft, rectTop);
    }

    public void drawTitle(Canvas canvas, BubbleVertex bubbleVertex, Matrix matrix) {
        Path tempPath = new Path();
        mPath.transform(matrix, tempPath);
        tempPath.close();
        setTextGradient(bubbleVertex);
        canvas.drawPath(tempPath, mStrokePaint);
        canvas.drawPath(tempPath, mTextPaint);
    }

    private void setTextGradient(BubbleVertex bubbleVertex) {
        if(mGradientColor != 0) {
            int[] colors = new int[] {mTextColor, mGradientColor};
            LinearGradient linearGradient = new LinearGradient(bubbleVertex.p1.x, bubbleVertex.p1.y, bubbleVertex.p4.x, bubbleVertex.p4.y,
                    colors, null, Shader.TileMode.CLAMP);
            Paint paint = mTextPaint;
            paint.setShader(linearGradient);
        } else {
            Paint paint = mTextPaint;
            paint.setShader(null);
        }
    }

    public RectF getPathRectF() {
        return mPathRectF;
    }

    @Override
    protected void buildBoxPath() {

    }

    @Override
    protected void buildHandlePath() {

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
    protected void buildTitlePath() {

    }

}
