/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.util.AttributeSet;
import android.util.FloatMath;
import android.view.View;

public class BalloonView extends View {
    private Context mContext;
    private TextBubbleBox mTextBubbleBox;
    private Matrix mBoxMatrix;
    private BubbleVertex mBubbleVertex;
    private int mBalloonViewWidth;
    private int mBalloonViewHeight;

    private int mBalloonWidth;
    private int mBalloonHeight;
    private int mViewWidth;
    private int mViewHeight;

    public BalloonView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.BalloonView, 0, 0);

        mViewWidth = a.getDimensionPixelSize(R.styleable.BalloonView_viewWidth, 0);
        mViewHeight = a.getDimensionPixelSize(R.styleable.BalloonView_viewHeight, 0);
        mBalloonWidth = a.getDimensionPixelSize(R.styleable.BalloonView_balloonWidth, 0);
        mBalloonHeight = a.getDimensionPixelSize(R.styleable.BalloonView_balloonHeight, 0);
        a.recycle();
    }

    public BalloonView(Context context) {
        super(context);
        mContext = context;
    }

    public void setBalloonStyleByAdapter(ViewAttributes attributes, String inputText, String textSize, int styles) {
        float scaleRatio = ImageEditDesc.getLabelScaleRatio();
        if(scaleRatio < 1) {
            scaleRatio = 1f;
        }
        mViewWidth = (int)FloatMath.ceil(mViewWidth * scaleRatio);
        mViewHeight = (int)FloatMath.ceil(mViewHeight* scaleRatio);
        mBalloonWidth = (int)FloatMath.ceil(mBalloonWidth * scaleRatio);
        mBalloonHeight = (int)FloatMath.ceil(mBalloonHeight * scaleRatio);

        mBalloonViewWidth = (int)FloatMath.ceil(mViewWidth * 1.2f);
        mBalloonViewHeight = mViewHeight * 2;
        if(mBoxMatrix != null) {
            mBoxMatrix.reset();
        }
        if(mBubbleVertex != null) {
            mBubbleVertex.reset();
        }

        mBoxMatrix = new Matrix();
        mBubbleVertex = new BubbleVertex(mContext);

        int left = (mBalloonViewWidth - mBalloonWidth) / 2;
        int top = (mBalloonViewHeight - mBalloonHeight) / 2;
        int right = left + mBalloonWidth;
        int bottom = top + mBalloonHeight;

        mBoxMatrix.setTranslate(left, top);
        mBubbleVertex.initBoxPoints(mBoxMatrix, right - left, bottom - top);
        mBubbleVertex.initArrowPoints();

        TextBubbleBoxAttribute mTextBubbleBoxAttribute = new TextBubbleBoxAttribute(mContext, mBubbleVertex, attributes, inputText, textSize);

        switch (styles) {
            case ImageEditConstants.BALLOON_STYLE_OVAL:
                mTextBubbleBox = new OvalTextBubbleBox(mTextBubbleBoxAttribute, mContext);
                break;
            case ImageEditConstants.BALLOON_STYLE_EXPLOSION:
                mTextBubbleBox = new ExplosionTextBubbleBox(mTextBubbleBoxAttribute, mContext);
                break;
            case ImageEditConstants.BALLOON_STYLE_CLOUND:
                mTextBubbleBox = new CloundTextBubbleBox(mTextBubbleBoxAttribute, mContext);
                break;
            case ImageEditConstants.BALLOON_STYLE_RECT:
                mTextBubbleBox = new RectTextBubbleBox(mTextBubbleBoxAttribute, mContext);
                break;
            case ImageEditConstants.BALLOON_STYLE_CHAMFER:
                mTextBubbleBox = new ChamferTextBubbleBox(mTextBubbleBoxAttribute, mContext);
                break;
            default:
                break;
        }
        mTextBubbleBox.setWillDrawText(true);

        requestLayout();
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if(mTextBubbleBox != null) {
            mTextBubbleBox.draw(canvas, mBoxMatrix, mBubbleVertex);
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        setMeasuredDimension((int)FloatMath.ceil(mViewWidth * 1.2f), mViewHeight * 2);
    }
}
