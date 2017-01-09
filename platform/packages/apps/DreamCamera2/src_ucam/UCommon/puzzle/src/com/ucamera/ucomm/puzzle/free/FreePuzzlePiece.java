/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.free;

import com.ucamera.ucomm.puzzle.PuzzlePiece;
import com.ucamera.ucomm.puzzle.PuzzleSpec;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BlurMaskFilter;
import android.graphics.BlurMaskFilter.Blur;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.PaintFlagsDrawFilter;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;

public class FreePuzzlePiece extends PuzzlePiece {
    private Bitmap mOriginBitmap = null;
    private PuzzleSpec.SpecInfo mSpec;
    private int mParentWidth = 0;
    private int mParentHeight = 0;
    private RectF mMarginRectF = new RectF();
    private Paint mPaint;

    public FreePuzzlePiece(Context context,int num, Uri uri) {
        super(context, num, uri);
        mPaint = new Paint();
        mPaint.setStyle(Style.STROKE);
        mPaint.setStrokeWidth(15);
        mPaint.setColor(0xffffffff);
        mPaint.setAntiAlias(true);
        mPaint.setMaskFilter(new BlurMaskFilter(3, Blur.SOLID));
    }

    public void setImageBitmap(Bitmap bitmap) {
        initFreePieceInfo(bitmap);
        super.setImageBitmap(bitmap);
    }

    private void initFreePieceInfo(Bitmap bitmap) {
        mOriginBitmap = bitmap;
        mMarginRectF.set(0, 0, mOriginBitmap.getWidth(), mOriginBitmap.getHeight());
    }

    @Override
    protected void onDraw(Canvas canvas) {
        try {
            canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG
                    | Paint.FILTER_BITMAP_FLAG));
            super.onDraw(canvas);
            canvas.save();
            canvas.concat(getImageMatrix());
            canvas.drawRect(mMarginRectF, mPaint);
            canvas.restore();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void setImageDrawable(Drawable drawable) {
        super.setImageDrawable(drawable);
        /*
         * FIX BUG: 970
         * BUG CAUSE: do not init piece info when switching from gridpuzzle
         * after changing two pieces
         * FIX COMMENT: reinit pieceinfo
         * DATE: 2012-05-04
         */
        initFreePieceInfo(((BitmapDrawable)drawable).getBitmap());
    }

    public void setParentWidthAndHeight(int width, int height) {
        mParentHeight = height;
        mParentWidth = width;
    }

    public Bitmap getImageBitmap() {
        return mOriginBitmap;
    }

    public boolean isPointInView(int x, int y) {
        Matrix inverse = new Matrix();
        if (getImageMatrix().invert(inverse)) {
            float[] points = new float[2];
            points[0] = x;
            points[1] = y;
            inverse.mapPoints(points);
            return new RectF(0, 0, mOriginBitmap.getWidth(), mOriginBitmap.getHeight()).contains(points[0], points[1]);
        }
        else {
            if (mOriginBitmap != null) {
                return getBitmapBound().contains(x, y);
            }
        }
        return false;
    }

    private void setDstMatrix() {
        Matrix matrix = getImageMatrix();
        /*
         * GUB FIX:
         * FIX COMMENT: zoom by a solid ratio
         */
        float sx = ((float) (mSpec.mRight - mSpec.mLeft) / mSpec.mWidth * mParentWidth)
                / mOriginBitmap.getWidth();
        float sy = ((float) (mSpec.mBottom - mSpec.mTop) / mSpec.mHeight * mParentHeight)
                / mOriginBitmap.getHeight();
        if(sy < sx) {
            sx = sy;
        }
        matrix.postScale(sx, sx);
        /*matrix.postRotate(mSpec.mRotateDegree, mOriginBitmap.getWidth() * sx / 2, mOriginBitmap.getHeight() * sy / 2);*/
        matrix.postRotate(mSpec.mRotateDegree, mOriginBitmap.getWidth() * sx / 2, mOriginBitmap.getHeight() * sx / 2);


        float xratio = (float) mParentWidth / mSpec.mWidth;
        float yratio = (float) mParentHeight / mSpec.mHeight;
        float l = mSpec.mLeft * xratio;
        float t = mSpec.mTop * yratio;
        matrix.postTranslate(l, t);

        setImageMatrix(matrix);
    }

    public void refreshInfo(PuzzleSpec.SpecInfo spec) {
        mSpec = spec;
        refreshInfo();
    }

    private void refreshInfo() {
        if (mSpec == null) {
            return ;
        }
        if (mParentWidth == 0 || mParentHeight == 0) {
            return ;
        }
        Matrix matrix = getImageMatrix();
        matrix.reset();
        setImageMatrix(matrix);
        setDstMatrix();
        /*
         * GUB FIX: 6162
         * FIX DATE: 2014-03-28
         */
//        setViewLeftTop();
    }

    private RectF getBitmapBound() {
        RectF boundRectF = new RectF(0, 0, mOriginBitmap.getWidth(), mOriginBitmap.getHeight());
        getImageMatrix().mapRect(boundRectF);
        return boundRectF;
    }

    public PointF getBitmapCenter() {
        return new PointF(getBitmapBound().centerX(), getBitmapBound().centerY());
    }

    private void setViewLeftTop() {
        if (mSpec == null) {
            return;
        }
        float xratio = (float) mParentWidth / mSpec.mWidth;
        float yratio = (float) mParentHeight / mSpec.mHeight;

        float l = mSpec.mLeft * xratio;
        float t = mSpec.mTop * yratio;

        Matrix matrix = getImageMatrix();
        matrix.postTranslate(l, t);
        setImageMatrix(matrix);
    }

    @Override
    public void setImageMatrix(Matrix matrix) {
        super.setImageMatrix(matrix);
    }

}
