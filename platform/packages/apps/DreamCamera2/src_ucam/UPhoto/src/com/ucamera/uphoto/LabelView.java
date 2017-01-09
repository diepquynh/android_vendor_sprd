/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

public class LabelView extends View {
    private Context mContext;
    private Path mPath = null;
    private Paint mTextPaint = null;
    private Resources mResources;
    private String mPackageName;
    private BitmapFactory.Options mOptions;
    private Bitmap mLabelBodyBitmap;
    private String mInputText;
    private int mScreenWidth;
    private int mLabelWidth;
    private int mLabelHeight;
    private Matrix mBodyMatrix;
    // CID 109184 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private float mLabelSize;
    private int mHeadHeight = -1;

    public LabelView(Context context, AttributeSet attrs) {
        super(context, attrs);
        try {
            setLayerType(View.LAYER_TYPE_SOFTWARE, null);
        } catch(NoSuchMethodError error) {
            Log.w("LabelView", "setLayerType method is not exists!!!");
        }
        mContext = context;
        mResources = context.getResources();
        mPackageName = context.getPackageName();
        mOptions = Utils.getNativeAllocOptions();
        mScreenWidth = mContext.getResources().getDisplayMetrics().widthPixels;

        if(mHeadHeight == -1) {
            Drawable d = context.getResources().getDrawable(R.drawable.label_placeholder);
            mHeadHeight = d.getIntrinsicHeight();
        }

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.LabelView, 0, 0);
        // mRotate = a.getInt(R.styleable.LabelView_rotate, 0);
        // CID 109184 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mLabelSize = a.getDimensionPixelSize(R.styleable.LabelView_labelSize, -1);
        a.recycle();
    }

    public LabelView(Context context) {
        super(context);
        mContext = context;
        mResources = context.getResources();
        mPackageName = context.getPackageName();
        mOptions = Utils.getNativeAllocOptions();
        mScreenWidth = mContext.getResources().getDisplayMetrics().widthPixels;
    }

    public Matrix getBodyMatrix() {
        return mBodyMatrix;
    }

    public void setLabelStyle(ViewAttributes attrs, String inputText, int rotate) {
//        mRotate = rotate;
        if (mPath != null) {
            mPath.reset();
        }
        mPath = new Path();

        if (mTextPaint != null) {
            mTextPaint.reset();
        }

//        if (mMatrix != null) {
//            mMatrix.reset();
//        }
//        mMatrix = new Matrix();

        if(mLabelBodyBitmap != null) {
            mLabelBodyBitmap.recycle();
            mLabelBodyBitmap = null;
        }
        /**
         * FIX BUG: 1055
         * BUG CAUSE: Support null pointer
         * FIX COMMENT: set defaulut
         * DATE: 2012-07-18
         */
        if(TextUtils.isEmpty(inputText)) {
            inputText = "Preview";
        }

        mInputText = inputText;
        mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mTextPaint.setStyle(Paint.Style.FILL);
        mTextPaint.setTextAlign(Paint.Align.CENTER);

        int textColorId = 0;
        String textColor = attrs.getTextColor();
        if(textColor != null && textColor.startsWith("#")) {
            textColorId = Color.parseColor(textColor);
        }

        float labelSize = 24.0f;
        //if(mLabelSize > -1) {
        //    labelSize = mLabelSize;
        //}
//        mTextPaint.setTextSize(24);
        mTextPaint.setTextSize(labelSize);
        mTextPaint.setColor(textColorId);
        mTextPaint.setTypeface(FontResource.createTypeface(mContext,attrs.getFont()));

        String drawable = "drawable/";
        String labelBody = attrs.getLabel();
        String label = drawable + labelBody;
        int labelBodyId = mResources.getIdentifier(label, null, mPackageName);
        /*
         * FIX BUG: 6043
         * BUG CAUSE: NotFoundException: Resource ID #0x0
         * DATE: 2012-08-10
         */
        if(labelBodyId == 0) {
            return;
        }
        mLabelBodyBitmap = BitmapFactory.decodeResource(mResources, labelBodyId, mOptions);
        /*
         * FIX BUG: 1442
         * BUG CAUSE: May be the mLabelBodyBitmap is null;
         * FIX COMMENT: If null, it will not draw this label on the preview;
         * DATE: 2012-08-10
         */
        if(mLabelBodyBitmap == null) {
            return;
        }

        /*int textLen = inputText.length();
        mTextPaint.getTextPath(inputText, 0, textLen, 0, 0, mPath);
        RectF mRectF = new RectF();
        mPath.computeBounds(mRectF, true);
        float fontWidth = mRectF.width();*/

        Rect bounds = new Rect();
        mTextPaint.getTextBounds(inputText, 0, inputText.length(), bounds);
        float fontHeight = bounds.height();
        float fontWidth = bounds.width();

        float beforeRotateBodyHeight = mLabelBodyBitmap.getWidth();
        float afterWidth = 0;
        if(fontWidth > beforeRotateBodyHeight) {
            if (fontWidth > mScreenWidth - 10) {
                afterWidth = mScreenWidth - 10;
            } else {
                afterWidth = fontWidth;
            }
        } else {
            afterWidth = beforeRotateBodyHeight;
        }
//        mMatrix.setRotate(-mRotate);
        mLabelBodyBitmap = Bitmap.createBitmap(mLabelBodyBitmap, 0, 0, mLabelBodyBitmap.getWidth(), mLabelBodyBitmap.getHeight(), null, false);
        float scale = afterWidth / (beforeRotateBodyHeight - 30);
        if(scale >= 1) {
            Matrix matrix = new Matrix();
            matrix.setScale(scale, 1);
            if(mBodyMatrix != null) {
                mBodyMatrix.reset();
            }
            mBodyMatrix = matrix;
            mLabelBodyBitmap = Bitmap.createBitmap(mLabelBodyBitmap, 0, 0, mLabelBodyBitmap.getWidth(), mLabelBodyBitmap.getHeight(), matrix, false);
        }
        /*
         * FIX BUG: 6139
         * BUG CAUSE: May be the mLabelBodyBitmap is null;
         * FIX COMMENT: If null, it will not draw this label on the preview;
         * DATE: 2014-03-19
         */
        if(mLabelBodyBitmap == null) {
            return;
        }
        mPath.moveTo(0, mLabelBodyBitmap.getHeight() * 3 / 4 + fontHeight / 3);
        mPath.lineTo(mLabelBodyBitmap.getWidth(), mLabelBodyBitmap.getHeight() * 3 / 4 + fontHeight / 3);

//        mPath.close();
        requestLayout();
        invalidate();
    }

    public void updateColor(ViewAttributes attributes, int rotate) {
        int textColorId = 0;
        String textColor = attributes.getTextColor();
        if(textColor != null && textColor.startsWith("#")) {
            textColorId = Color.parseColor(textColor);
        }
        mTextPaint.setColor(textColorId);
        mTextPaint.setTypeface(FontResource.createTypeface(mContext,attributes.getFont()));
        mInputText = attributes.getDrawText();

        requestLayout();
        invalidate();
    }

    protected void onDraw(Canvas canvas) {
        /*
         * FIX BUG: 5197
         * BUG COMMENT: Avoid null pointer exception
         * DATE: 2013-11-11
         */
        if(mLabelBodyBitmap != null) {
            canvas.drawBitmap(mLabelBodyBitmap, 0, mHeadHeight, null);
        }
        if(mInputText != null) {
            canvas.drawTextOnPath(mInputText, mPath, 0, 0, mTextPaint);
        }
    }

    public int getLabelWidth() {
        return mLabelWidth;
    }

    public int getLabelHeight() {
        return mLabelHeight;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        /*
         * FIX BUG: 4962
         * FIX COMMENT: avoid null point
         * DATE: 2013-10-11
         */
        if(mLabelBodyBitmap == null) return;
        int measureWidth = 0;
        int measureHeight = 0;
        measureWidth = mLabelBodyBitmap.getWidth();
        measureHeight = mLabelBodyBitmap.getHeight();
        measureHeight += mHeadHeight;
        mLabelWidth = measureWidth;
        mLabelHeight = measureHeight;

        setMeasuredDimension(measureWidth, measureHeight);
    }
}
