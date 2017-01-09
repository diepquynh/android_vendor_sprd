/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Shader;
import android.util.AttributeSet;
import android.util.FloatMath;
import android.view.View;

public class TitleView extends View {
    private Context mContext;
    private Path mPath = null;
    private Paint mTextPaint = null;
    private Paint mStrokePaint = null;
    private RectF mRectF = new RectF();
    private boolean mIsCommonTitleItem;
    private int mIndex; //0: title; 1: bolloon: 2: label
    private float mScaleRatio;
    // CID 109189 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private int mCommonTitleWidth;
    private float mTitleSize;

    public TitleView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TitleView, 0, 0);

        mTitleSize = a.getDimensionPixelSize(R.styleable.TitleView_titleSize, -1);
        a.recycle();

    }

    public TitleView(Context context) {
        super(context);
        mContext = context;
    }

    public void setTitleStyle(ViewAttributes attrs, String inputText, boolean isCommonTitleItem, int index) {
        mIsCommonTitleItem = isCommonTitleItem;
        mIndex = index;
        mScaleRatio = ImageEditDesc.getLabelScaleRatio();
        // CID 109189 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mCommonTitleWidth = (int)(84 * mScaleRatio);
        if(mPath != null) {
            mPath.reset();
        }
        mPath = new Path();
        mPath.isSimplePath = false;

        if(mTextPaint != null) {
            mTextPaint.reset();
        }
        mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mTextPaint.setStyle(Paint.Style.FILL);
        mTextPaint.setTextAlign(Paint.Align.CENTER);
        float textSize = 42;
        if(mTitleSize > -1) {
            textSize = mTitleSize;
        } else {
            String textSizeStr = attrs.getTextSize();
            if(textSizeStr != null && textSizeStr.length() > 0) {
                textSize = Float.parseFloat(textSizeStr);
            }
        }

        textSize = textSize * mScaleRatio;

        int textColorId = 0;
        String textColor = attrs.getTextColor();
        if(textColor != null && textColor.startsWith("#")) {
            textColorId = Color.parseColor(textColor);
        }
        int gradientEndId = 0;
        String gradientEnd = attrs.getGradientEnd();
        if(gradientEnd != null && gradientEnd.startsWith("#")) {
            gradientEndId = Color.parseColor(gradientEnd);
        }
        mTextPaint.setTextSize(textSize);
        mTextPaint.setColor(textColorId);
        mTextPaint.setTypeface(FontResource.createTypeface(mContext, attrs.getFont()));

        if(index == 0) {
            if (mStrokePaint != null) {
                mStrokePaint.reset();
            }
            mStrokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
            mStrokePaint.setStyle(Paint.Style.STROKE);
            mStrokePaint.setStrokeJoin(Paint.Join.ROUND);
            mStrokePaint.setTextAlign(Paint.Align.CENTER);

            int outlineColorId = 0;
            String outline = attrs.getOutline();
            if(outline != null && outline.startsWith("#")) {
                outlineColorId = Color.parseColor(outline);
            }
            int strokeWidth = attrs.getStrokeWidth();
            mStrokePaint.setColor(outlineColorId);
            mStrokePaint.setStrokeWidth(strokeWidth);
        }

        setInputText(inputText, textColorId, gradientEndId);

    }

    public void setInputText(String inputText, int textColorId, int gradientEnd) {
        mPath.setFillType(Path.FillType.WINDING);

        int textLen = inputText.length();
        mTextPaint.getTextPath(inputText, 0, textLen, 0, 0, mPath);
        mPath.computeBounds(mRectF, true);

        int limitWidth = ImageEditDesc.getScreenWidth() - 30;
        if(mRectF.width() > limitWidth) {
            StringBuilder stringBuilder = new StringBuilder();
            int index = 0;
            float tempMaxWidth = 0;
            /*
             * FIX BUG: 1393
             * BUG CAUSE: index value is more than inputText length.
             * FIX COMMENT: If index value is more than inputText length, will stop loop;
             * DATE: 2012-08-01
             */
            while(tempMaxWidth < limitWidth && index < textLen) {
                char character = inputText.charAt(index);
                String singleStr = String.valueOf(character);
                stringBuilder.append(singleStr);
                tempMaxWidth += mTextPaint.measureText(singleStr);
                index++;
            }
            mPath.reset();
            inputText = stringBuilder.toString();
            textLen = inputText.length();
            mTextPaint.getTextPath(inputText, 0, textLen, 0, 0, mPath);
            mPath.computeBounds(mRectF, true);
        }

        mRectF.inset(-2.5f, -2.5f);
        float rectLeft = -mRectF.left;
        float rectTop = -mRectF.top;
        mPath.offset(rectLeft, rectTop);

        float rectHeight = mRectF.height();

        if(gradientEnd != 0) {
            int[] colors = new int[] {textColorId, gradientEnd};
            LinearGradient linearGradient = new LinearGradient(0, 0, 0, rectHeight, colors, null, Shader.TileMode.CLAMP);
            Paint paint = mTextPaint;
            paint.setShader(linearGradient);
        } else {
            Paint paint = mTextPaint;
            paint.setShader(null);
        }

        mPath.close();
        requestLayout();
        invalidate();
    }

    protected void onDraw(Canvas canvas) {
        if(mIndex == 0) {
            canvas.drawPath(mPath, mStrokePaint);
        }
        canvas.drawPath(mPath, mTextPaint);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        if(mIsCommonTitleItem) {
            if(mRectF.width() > UiUtils.screenWidth() * 3 / 4) {
                setMeasuredDimension(UiUtils.screenWidth() * 7 / 10, (int)FloatMath.ceil(mRectF.height()));
            } else {
                setMeasuredDimension((int)FloatMath.ceil(mRectF.width()), (int)FloatMath.ceil(mRectF.height()));
            }
        } else {
            setMeasuredDimension((int)FloatMath.ceil(mRectF.width()), (int)FloatMath.ceil(mRectF.height()));
        }
    }
}
