/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Paint;

public class TextBubbleBoxAttribute {
    private Paint mBoxPaint;
    private Paint mFramePaint;
    private Paint mTextPaint;
    private String mInputText;
    private int mFillColorId;
    private int mGradientEndId;

    private BubbleVertex mBubbleVertex;

    public TextBubbleBoxAttribute(Context context, BubbleVertex bubbleVertex, ViewAttributes attributes, String inputText, String textSize) {
        mBubbleVertex = bubbleVertex;
        mInputText = inputText;

        if(mTextPaint != null) {
            mTextPaint.reset();
        }
        mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mTextPaint.setStyle(Paint.Style.FILL);
//        mTextPaint.setTextAlign(Paint.Align.CENTER);
//        mTextPaint.setUnderlineText(true);
        int textColorId = 0;
        String textColor = attributes.getTextColor();
        if(textColor != null && textColor.startsWith("#")) {
            textColorId = Color.parseColor(textColor);
        }
        mTextPaint.setColor(textColorId);
//        mTextPaint.setTextSize(Float.parseFloat(attributes.getTextSize()));
        float textSizeF = Float.parseFloat(textSize) * ImageEditDesc.getLabelScaleRatio();
        mTextPaint.setTextSize(textSizeF);
//        mTextPaint.setTextSize(Float.parseFloat(textSize));
        mTextPaint.setTypeface(FontResource.createTypeface(context, attributes.getFont()));

        if(mBoxPaint != null) {
            mBoxPaint.reset();
        }
        mBoxPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mBoxPaint.setStyle(Paint.Style.FILL);
        String panelFill = attributes.getPanelFill();
        if(panelFill != null && panelFill.startsWith("#")) {
            mFillColorId = Color.parseColor(panelFill);
        }
        String gradientEnd = attributes.getGradientEnd();
        if(gradientEnd != null && gradientEnd.startsWith("#")) {
            mGradientEndId = Color.parseColor(gradientEnd);
        }

        if(mFramePaint != null) {
            mFramePaint.reset();
        }
        mFramePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mFramePaint.setStyle(Paint.Style.STROKE);
        mFramePaint.setStrokeJoin(Paint.Join.ROUND);
        mFramePaint.setTextAlign(Paint.Align.CENTER);
        int outlineColorId = 0;
        String outline = attributes.getOutline();
        if(outline != null && outline.startsWith("#")) {
            outlineColorId = Color.parseColor(outline);
        }
        int shadowColorId = 0;
        String shadowColor = attributes.getPanelShadow();
        if(shadowColor != null && shadowColor.startsWith("#")) {
            shadowColorId = Color.parseColor(shadowColor);
        }
        int strokeWidth = attributes.getStrokeWidth();
        mFramePaint.setColor(outlineColorId);
        if(shadowColorId != 0) {
            mFramePaint.setShadowLayer(5, 0, 0, shadowColorId);
        }
        mFramePaint.setStrokeWidth(strokeWidth);
    }

    public BubbleVertex getBubbleVertex() {
        return mBubbleVertex;
    }

    public void setBubbleVertex(BubbleVertex bubbleVertex) {
        mBubbleVertex = bubbleVertex;
    }

    public Paint getFramePaint() {
        return mFramePaint;
    }

    public void setFramePaint(Paint framePaint) {
        mFramePaint = framePaint;
    }

    public Paint getBoxPaint() {
        return mBoxPaint;
    }

    public void setBoxPaint(Paint boxPaint) {
        mBoxPaint = boxPaint;
    }

    public String getTextString() {
        return mInputText;
    }

    public void setTextString(String textString) {
        mInputText = textString;
    }

    public Paint getTextPaint() {
        return mTextPaint;
    }

    public void setTextPaint(Paint textPaint) {
        mTextPaint = textPaint;
    }

    public int getPanelFillColor() {
        return mFillColorId;
    }

    public int getGradientEndColor() {
        return mGradientEndId;
    }
}
