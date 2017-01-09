/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Shader;
import android.graphics.Path;
import android.graphics.RectF;

public abstract class TextBubbleBox {
    protected ImageEditDesc mImageEditDesc;
    protected RectF mRectf;
    protected String mInputText;
    protected Paint mStrokePaint;
    protected Paint mBoxPaint;
    protected Paint mTextPaint;
    protected Path mBoxPath;
    protected Path mHandlePath;
    protected BubbleVertex mBubbleVertex;
    protected TextBubbleUtil mTextBubbleUtil;
    protected boolean mWillDrawText;
    protected Context mContext;
    protected ViewAttributes mAttributes;
    protected int mPanelFillColor;
    protected int mGradientColor;

    protected LabelBoxAttribute mLabelBoxAttribute;
    protected Path mPath;
    protected float mFontHeight;
    protected int mLabelBodyId;
    protected int mLabelHeadId;
    protected int mLabelTailId;
    protected Bitmap mLabelBodyBitmap;
    protected Bitmap mLabelHeadBitmap;
    protected Bitmap mLabelTailBitmap;

    private Paint tempPaint = new Paint();

    /**
     * BalloonBoX--- BalloonView
     * @param textBubbleBoxAttribute
     * @param context
     */
    public TextBubbleBox(TextBubbleBoxAttribute textBubbleBoxAttribute, Context context) {
        mContext = context;
        mImageEditDesc = ImageEditDesc.getInstance();

        mStrokePaint = textBubbleBoxAttribute.getFramePaint();
        mBoxPaint = textBubbleBoxAttribute.getBoxPaint();
        mTextPaint = textBubbleBoxAttribute.getTextPaint();
        mInputText = textBubbleBoxAttribute.getTextString();
        mBubbleVertex = textBubbleBoxAttribute.getBubbleVertex();
        mPanelFillColor = textBubbleBoxAttribute.getPanelFillColor();
        mGradientColor = textBubbleBoxAttribute.getGradientEndColor();
        mTextBubbleUtil = new TextBubbleUtil(mInputText, mTextPaint);

        tempPaint.setColor(Color.RED);

        initializeData();
    }

    /**
     * for TitleBox -- TitleView
     * @param context
     * @param attributes
     */
    public TextBubbleBox(Context context, ViewAttributes attributes) {
        mContext = context;
        mAttributes = attributes;
        initializeVertex();
//        buildTitlePath();
    }

    /**
     * for LabelBox -- LabelView
     * @param labelBoxAttribute
     * @param context
     */
    public TextBubbleBox(LabelBoxAttribute labelBoxAttribute, Context context) {
        mContext = context;
//        mLabelBoxAttribute = labelBoxAttribute;
        mPath = labelBoxAttribute.getDrawTextPath();
        mFontHeight = labelBoxAttribute.getFontHeight();
        mInputText = labelBoxAttribute.getInputText();
        mTextPaint = labelBoxAttribute.getTextPaint();
        mLabelBodyId = labelBoxAttribute.getLabelBodyId();
        mLabelHeadId = labelBoxAttribute.getLabelHeadId();
        mLabelTailId = labelBoxAttribute.getLabelTailId();
        mLabelBodyBitmap = labelBoxAttribute.getLabelBodyBitmap();
        mLabelHeadBitmap = labelBoxAttribute.getLabelHeadBitmap();
        mLabelTailBitmap = labelBoxAttribute.getLabelTailBitmap();

        initializeVertex();
    }

    protected void initializeData() {
        buildBoxPath();
        buildHandlePath();
    }

    protected abstract void initializeVertex();
    protected abstract void buildBoxPath();
    protected abstract void buildHandlePath();
    protected abstract void buildTitlePath();

    public abstract void drawFrame(Canvas canvas, Matrix matrix, BubbleVertex bubbleVertex);
    public abstract void drawBox(Canvas canvas, Matrix matrix, BubbleVertex bubbleVertex);
    public abstract RectF getTextRect();

    public void draw(Canvas canvas, Matrix matrix, BubbleVertex bubbleVertex) {
        setGradientPaint(matrix, bubbleVertex);
        drawFrame(canvas, matrix, bubbleVertex);
        drawBox(canvas, matrix, bubbleVertex);
        RectF rectf = getTextRect();
        /* SPRD: CID 108983 : Dereference null return value (NULL_RETURNS) @{ */
        if(rectf == null){
            return;
        } else {
            mTextBubbleUtil.setTextRect(rectf);
        }
        // mTextBubbleUtil.setTextRect(rectf);
        /* @} */
        if(mWillDrawText) {
            mTextBubbleUtil.drawText(canvas, matrix, bubbleVertex);
        }
    }

    private void setGradientPaint(Matrix matrix, BubbleVertex bubbleVertex) {
        if(mGradientColor != 0) {
            int[] colors = new int[] {mPanelFillColor, mGradientColor};
            LinearGradient linearGradient = new LinearGradient(bubbleVertex.p1.x, bubbleVertex.p1.y, bubbleVertex.p4.x, bubbleVertex.p4.y,
                    colors, null, Shader.TileMode.CLAMP);
            Paint paint = mBoxPaint;
            paint.setShader(linearGradient);
        } else {
            Paint paint = mBoxPaint;
            paint.setColor(mPanelFillColor);
        }
    }

    public void setText(String text) {
        mInputText = text;
        mTextBubbleUtil.setText(text);
    }

    public String getText() {
        return mInputText;
    }

    public boolean getWillDrawText() {
        return mWillDrawText;
    }

    public void setWillDrawText(boolean willDrawText) {
        mWillDrawText = willDrawText;
    }
}
