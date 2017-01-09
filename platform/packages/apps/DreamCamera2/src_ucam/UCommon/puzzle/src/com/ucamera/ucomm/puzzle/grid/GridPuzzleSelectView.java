/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.grid;

import android.content.Context;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;

import com.ucamera.ucomm.puzzle.PuzzleSpec;
import com.ucamera.ucomm.puzzle.util.Models;
public class GridPuzzleSelectView extends ViewGroup{
    private static final int MARGIN = Models.HTC_DOPOD.equals(Models.getModel()) ? 2 : 4;
    private PuzzleSpec mPuzzleSpec;
    public GridPuzzleSelectView(Context context) {
        super(context);
    }
    public GridPuzzleSelectView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        final float width = getMeasuredWidth();
        final float height = getMeasuredHeight();
        PuzzleSpec spec/* = mPuzzle.getSpec();*/;
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
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        float width = getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec);
        setMeasuredDimension((int) width, (int) width);
    }

    /*public void setPuzzle(Puzzle puzzle) {
        mPuzzle = puzzle;
        removeAllViews();
        for(int i = 0; i < mPuzzle.getSpec().length(); i++) {
            addPiece();
        }
        requestLayout();
    }*/
    public void setPuzzleSpec(PuzzleSpec mspec) {
        mPuzzleSpec = mspec;
        removeAllViews();
        for(int i = 0; i < mPuzzleSpec.length(); i++) {
            addPiece();
        }
        requestLayout();
    }
    private void addPiece() {
        View iv = new View(getContext());
        iv.setBackgroundColor(0xff2f3032);
        addView(iv);
        requestLayout();
    }
}
