/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.grid;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PaintFlagsDrawFilter;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.Paint.Style;
import android.graphics.RectF;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;

import com.ucamera.ucomm.puzzle.PuzzlePiece;
import com.ucamera.ucomm.puzzle.util.PuzzleBitmapManager;

public class GridPuzzlePiece extends PuzzlePiece {
    private Bitmap mBitmap;
    private Paint mPaint = new Paint();
    private Rect mRect = new Rect();
    private float mRoundDegree = 0f;
    private Path mCanvasClipPath = new Path();
    public GridPuzzlePiece(Context context, int num, Uri uri) {
        super(context, num, uri);
        mPaint.setAntiAlias(true);
        mPaint.setDither(true);
        mPaint.setStyle(Style.STROKE);
        mPaint.setStrokeWidth(3);
        mPaint.setColor(0xffffffff);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        /*
         *BUG FIX:5724
         *BUG CAUSE:trying to use a recycled bitmap
         *FIX DATE:2013-12-30
         **/
        canvas.setDrawFilter(new PaintFlagsDrawFilter(0, Paint.ANTI_ALIAS_FLAG | Paint.FILTER_BITMAP_FLAG));
        mCanvasClipPath.reset();
        getDrawingRect(mRect);
        mCanvasClipPath.addRoundRect(new RectF(mRect), mRoundDegree, mRoundDegree, Path.Direction.CW);
        canvas.save();
        canvas.clipPath(mCanvasClipPath);

        BitmapDrawable drawable = (BitmapDrawable) getDrawable();
        Bitmap bitmap = drawable != null ? drawable.getBitmap() : null;
        if (bitmap != null && !bitmap.isRecycled()) {
            super.onDraw(canvas);
        }

        canvas.drawRoundRect(new RectF(mRect), mRoundDegree, mRoundDegree, mPaint);
        paintSelection(canvas);
        canvas.restore();
    }

    @Override
    public void setImageBitmap(Bitmap bm) {
        /*
         * FIX BUG: 1247
         * BUG CAUSE: trying to use a recycled bitmap
         * FIX COMMENT: add the judgment for bitmap
         * DATE: 2012-07-16
         */
        if(bm != null && !bm.isRecycled()){
            mBitmap = bm;
            super.setImageBitmap(bm);
        }
    }

    public void setRoundDegree(float degree) {
        mRoundDegree = degree;
    }
    /*
     * Bug fix: 4275
     * Fix comment: Force No layout when image change
     */
    private boolean mForceNoLayout = false;
    @Override
    public void setImageDrawable(Drawable d) {
        BitmapDrawable bitmap = (BitmapDrawable) d;
        mBitmap = bitmap.getBitmap();
        mForceNoLayout = true;
        super.setImageDrawable(d);
        mForceNoLayout = false;
    }

    @Override
    public void requestLayout() {
        if (!mForceNoLayout) {
            super.requestLayout();
        }
    }

    public void setShadow(boolean shadow) {
        if(shadow) {
            mPaint.setMaskFilter(new BlurMaskFilter(3f, BlurMaskFilter.Blur.NORMAL));
        } else {
            mPaint.setMaskFilter(null);
        }
    }
    ///////////////////////////////////////////////////////////////////
    //      USER DEFINED METHODS
    private void paintSelection(Canvas canvas){
        if (isSelected()){
            Rect r = new Rect();
            getDrawingRect(r);
            Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
            p.setStyle(Style.STROKE);
            p.setStrokeWidth(14);
            p.setColor(0xff00aced);
            canvas.save();
            canvas.drawRect(r, p);
            canvas.restore();
            canvas.drawRoundRect(new RectF(r), mRoundDegree, mRoundDegree, p);
//            this.adjustScroll();
        }
    }

    public void adjustScroll() {
        if (getDrawable() == null)
            return;
        /*
         * BUG FIX:
         * FIX COMMENT: adjust the right bottom corner
         */
        final float[] offset = {0, 0, getDrawable().getIntrinsicWidth(), getDrawable().getIntrinsicHeight()};

        getImageMatrix().mapPoints(offset);
        int scrollX = getScrollX();
        int scrollY = getScrollY();
        int destX = scrollX, destY = scrollY;
        if (offset[0] - scrollX > 0) {
            destX = (int)offset[0];
        }
        if (offset[1] - scrollY >0) {
            destY = (int)offset[1];
        }

        if (offset[2] - scrollX < getWidth()) {
            destX = (int) offset[2] - getWidth();
        }

        if (offset[3] - scrollY < getHeight()) {
            destY = (int) offset[3]- getHeight();
        }

        if (destX != scrollX || destY != scrollY) {
            scrollTo(destX, destY);
        }
    }

    @Override
    public void rotateBitmap() {
        if(mBitmap == null || mBitmap.isRecycled()) {
            return;
        }
        Matrix matrix = new Matrix();
        matrix.postRotate(90);
        Bitmap bitmap = null;
        try{
            bitmap = Bitmap.createBitmap(mBitmap, 0, 0, mBitmap.getWidth(), mBitmap.getHeight(), matrix, true);
        } catch(IllegalArgumentException e) {
            bitmap = mBitmap;
        } catch (OutOfMemoryError e) {
            bitmap = mBitmap;
        }
        if(bitmap != mBitmap) {
            mBitmap.recycle();
        }
        setImageBitmap(bitmap);
        this.scrollTo(0, 0);
        PuzzleBitmapManager.getInstance().updateBitmap(getNum(), mBitmap);
    }
    @Override
    public void oprateHorizonFlipBitmap() {
        if(mBitmap == null || mBitmap.isRecycled()) {
            return;
        }
        Bitmap bitmap = null;
        final Matrix m = new Matrix();
        m.setScale(-1, 1);
        try {
            bitmap = Bitmap.createBitmap(mBitmap, 0, 0, mBitmap.getWidth(), mBitmap.getHeight(), m, true);
        } catch (OutOfMemoryError ex) {
            bitmap = mBitmap;
            System.gc();
        }
        setImageBitmap(bitmap);
        this.scrollTo(0, 0);
        PuzzleBitmapManager.getInstance().updateBitmap(getNum(), mBitmap);
    }
}
