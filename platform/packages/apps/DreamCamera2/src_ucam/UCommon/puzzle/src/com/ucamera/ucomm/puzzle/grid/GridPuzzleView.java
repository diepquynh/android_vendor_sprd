/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.grid;

import java.util.ArrayList;

import android.R.color;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Rect;
import android.net.Uri;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzlePiece;
import com.ucamera.ucomm.puzzle.PuzzleSpec;
import com.ucamera.ucomm.puzzle.PuzzleUtils;
import com.ucamera.ucomm.puzzle.PuzzleView;

public class GridPuzzleView extends PuzzleView {

    private static final int MARGIN = 6;

    public GridPuzzleView(Context context) {
        super(context);
    }

    public GridPuzzleView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public GridPuzzleView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public void borrowBitmapsFrom(PuzzleView src) {
        super.borrowBitmapsFrom(src);
        for (int i = 0; i < src.getChildCount(); i++) {
            getChildAt(i).updateUri(src.getChildAt(i).getUri());
        }
    }

    // /////////////////////////////////////////////////////////////////////////
    // Methods from super class or interfaces
    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        final float width = getMeasuredWidth();
        final float height = getMeasuredHeight();
        PuzzleSpec spec /*= mPuzzle.getSpec();*/;
        spec = mPuzzleSpec;
        Rect rect = spec.getRect();
        float xratio = width / rect.width();
        float yratio = height / rect.height();

        for (int i = 0, max = Math.min(spec.length(), getChildCount()); i < max; i++) {
            int left = (int) (spec.getLeft(i) * xratio) + MARGIN;
            int top = (int) (spec.getTop(i) * yratio) + MARGIN;
            int right = (int) (spec.getRight(i) * xratio);
            if (width - right < MARGIN) {
                right -= MARGIN;
            }
            int bottom = (int) (spec.getBottom(i) * yratio);
            if (height - bottom < MARGIN) {
                bottom -= MARGIN;
            }
            getChildAt(i).layout(left, top, right, bottom);
            /*
             * FIX BUG: 4298
             * BUG COMMENT: scoll puzzlepiece to (0,0)
             * FIX DATE: 2013-06-21
             */
            getChildAt(i).scrollTo(0, 0);
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        float width = getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec);
        float height = getDefaultSize(getSuggestedMinimumHeight(), heightMeasureSpec);

        PuzzleSpec spec /*= mPuzzle.getSpec();*/;
        spec = mPuzzleSpec;
        Rect rect = spec.getRect();
        float xratio = width / rect.width();
        float yratio = height / rect.height();

        for (int i = 0, max = Math.min(spec.length(), getChildCount()); i < max; i++) {
            int childWidthSpec = MeasureSpec.makeMeasureSpec(
                    (int) (spec.width(i) * xratio - MARGIN), MeasureSpec.AT_MOST);
            int childHeightSpec = MeasureSpec.makeMeasureSpec(
                    (int) (spec.height(i) * yratio - MARGIN), MeasureSpec.AT_MOST);
            getChildAt(i).measure(childWidthSpec, childHeightSpec);
        }
        setMeasuredDimension((int) width, (int) height);
    }

    @Override
    public void createPuzzle(int count) {
        //count = Math.min(9, Math.max(2, count));
        /*mPuzzle = Puzzle.create(Puzzle.Type.GRID, count).random(0);*/
        //mPuzzle = Puzzle.create(Puzzle.Type.GRID, count);
        //mPuzzle.getPuzzleIndex(0);
    }

    @Override
    public PuzzleView addPiece(Bitmap bitmap, int num, Uri uri) {
        PuzzlePiece piece = new GridPuzzlePiece(getContext(), num, uri);
        piece.setImageBitmap(bitmap);
        piece.setScaleType(ScaleType.CENTER_CROP);
        addView(piece);
        return this;
    }
}
