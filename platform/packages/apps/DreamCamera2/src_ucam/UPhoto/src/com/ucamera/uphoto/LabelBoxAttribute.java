/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.util.Log;

public class LabelBoxAttribute {
    private Paint mTextPaint;
    private String mInputText;
    private Path mPath;
    private float mFontHeight;
    private float mFontWidth;
    private Resources mResources;
    private String mPackageName;
    private BitmapFactory.Options mOptions;
    private int mLabelBodyId;
    private int mLabelHeadId;
    private int mLabelTailId;
    private Bitmap mLabelBodyBitmap;
    private Bitmap mLabelHeadBitmap;
    private Bitmap mLabelTailBitmap;

    private BubbleVertex mBubbleVertex;

    public LabelBoxAttribute(Context context, BubbleVertex bubbleVertex, ViewAttributes attributes, Matrix labelBodyMatrix) {
        mBubbleVertex = bubbleVertex;
        mInputText = attributes.getDrawText();

        mPath = new Path();
        mResources = context.getResources();
        mPackageName = context.getPackageName();
        mOptions = Utils.getNativeAllocOptions();

        mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mTextPaint.setStyle(Paint.Style.FILL);
        mTextPaint.setTextAlign(Paint.Align.CENTER);
        mTextPaint.setTextSize(24);

        int textColorId = 0;
        String textColor = attributes.getTextColor();
        if(textColor != null && textColor.startsWith("#")) {
            textColorId = Color.parseColor(textColor);
        }
        mTextPaint.setColor(textColorId);
        mTextPaint.setTypeface(FontResource.createTypeface(context, attributes.getFont()));

        Rect bounds = new Rect();
        /* FIX BUG : 3940
         * BUG COMMENT : avoid null point
         * DATE : 2013-07-03
         */
        if(mInputText != null) {
            mTextPaint.getTextBounds(mInputText, 0, mInputText.length(), bounds);
            mFontHeight = bounds.height();
            mFontWidth = bounds.width();
        }

        StringBuilder stringbuilder = (new StringBuilder()).append("drawable/");
        String labelBody = attributes.getLabel();
        String label = stringbuilder.append(labelBody).toString();
        mLabelBodyId = mResources.getIdentifier(label, null, mPackageName);
        mLabelBodyBitmap = BitmapFactory.decodeResource(mResources, mLabelBodyId, mOptions);
//        Matrix matrix = new Matrix();
//        matrix.setRotate(-90);
        if(mLabelBodyBitmap != null) {
            mLabelBodyBitmap = Bitmap.createBitmap(mLabelBodyBitmap, 0, 0, mLabelBodyBitmap.getWidth(), mLabelBodyBitmap.getHeight(), null, false);
        }
        if(mLabelBodyBitmap != null && labelBodyMatrix != null) {
            mLabelBodyBitmap = Bitmap.createBitmap(mLabelBodyBitmap, 0, 0, mLabelBodyBitmap.getWidth(), mLabelBodyBitmap.getHeight(), labelBodyMatrix, false);
        }
        float beforeRotateBodyHeight = mLabelBodyBitmap.getWidth();
        float afterWidth = 0;
        int screenWidth = context.getResources().getDisplayMetrics().widthPixels;
        if(mFontWidth > beforeRotateBodyHeight) {
            if (mFontWidth > screenWidth - 10) {
                afterWidth = screenWidth - 10;
            } else {
                afterWidth = mFontWidth;
            }
        } else {
            afterWidth = beforeRotateBodyHeight;
        }
        float scale = afterWidth / (beforeRotateBodyHeight-30);
        if(scale >= 1) {
            Matrix matrix = new Matrix();
            matrix.setScale(scale, 1);
            mLabelBodyBitmap = Bitmap.createBitmap(mLabelBodyBitmap, 0, 0,
                    mLabelBodyBitmap.getWidth(), mLabelBodyBitmap.getHeight(),
                    matrix, false);
        }
//        String labelHead = attributes.getLabelHead();
//        String labelTail = attributes.getLabelTail();
//        if(labelHead != null && labelHead.length() > 0 && labelTail != null && labelTail.length() > 0) {
//            label = label.replace(labelBody, labelHead);
//            mLabelHeadId = mResources.getIdentifier(label, null, mPackageName);
//            mLabelHeadBitmap = BitmapFactory.decodeResource(mResources, mLabelHeadId, mOptions);
//
//            label = label.replace(labelHead, labelTail);
//            mLabelTailId = mResources.getIdentifier(label, null, mPackageName);
//            mLabelTailBitmap = BitmapFactory.decodeResource(mResources, mLabelTailId, mOptions);
//
//            mLabelHeadBitmap = Bitmap.createBitmap(mLabelHeadBitmap, 0, 0, mLabelHeadBitmap.getWidth(), mLabelHeadBitmap.getHeight(), matrix, false);
//            mLabelTailBitmap = Bitmap.createBitmap(mLabelTailBitmap, 0, 0, mLabelTailBitmap.getWidth(), mLabelTailBitmap.getHeight(), matrix, false);
//
//        }
    }

    public BubbleVertex getBubbleVertex() {
        return mBubbleVertex;
    }

    public void setBubbleVertex(BubbleVertex bubbleVertex) {
        mBubbleVertex = bubbleVertex;
    }

    public Paint getTextPaint() {
        return mTextPaint;
    }

    public Path getDrawTextPath() {
        return mPath;
    }

    public float getFontHeight() {
        return mFontHeight;
    }

    public String getInputText() {
        return mInputText;
    }

    public int getLabelBodyId() {
        return mLabelBodyId;
    }

    public int getLabelHeadId() {
        return mLabelHeadId;
    }

    public int getLabelTailId() {
        return mLabelTailId;
    }

    public Bitmap getLabelBodyBitmap() {
        return mLabelBodyBitmap;
    }

    public Bitmap getLabelHeadBitmap() {
        return mLabelHeadBitmap;
    }

    public Bitmap getLabelTailBitmap() {
        return mLabelTailBitmap;
    }
}
