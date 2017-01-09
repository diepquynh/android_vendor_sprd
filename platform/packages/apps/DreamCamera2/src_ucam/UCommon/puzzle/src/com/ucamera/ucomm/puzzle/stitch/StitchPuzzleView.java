/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.stitch;

import android.content.Context;
import android.graphics.Bitmap;
import android.net.Uri;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView.ScaleType;

import com.ucamera.ucomm.puzzle.Puzzle;
import com.ucamera.ucomm.puzzle.PuzzlePiece;
import com.ucamera.ucomm.puzzle.PuzzleView;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

public class StitchPuzzleView extends PuzzleView {
    private static final int MARGIN = 6;
    public StitchPuzzleView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public StitchPuzzleView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public StitchPuzzleView(Context context) {
        super(context);
    }

    @Override
    protected void createPuzzle(int count) {
        count = Math.min(9, Math.max(2, count));
        mPuzzle = Puzzle.create(Puzzle.Type.STITCH, count).random(0);
    }

    @Override
    public PuzzleView addPiece(Bitmap bitmap, int num, Uri uri) {
        PuzzlePiece piece = new StitchPuzzlePiece(getContext(), num, uri);
        piece.setImageBitmap(bitmap);
        piece.setScaleType(ScaleType.CENTER_CROP);
        addView(piece, new ViewGroup.LayoutParams(MATCH_PARENT, WRAP_CONTENT));
        return this;
    }
    @Override
    public void borrowBitmapsFrom(PuzzleView src) {
        super.borrowBitmapsFrom(src);
        for (int i = 0; i < src.getChildCount(); i++) {
            getChildAt(i).updateUri(src.getChildAt(i).getUri());
        }
    }
    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        /*
         * BUG FIX:
         * FIX COMMENT: add margin
         **/
        final int count = getChildCount();
        int childTop = 0, childBottom = MARGIN, firstTop = MARGIN;
        for (int i=0; i<count; i++){
            if(i > 0) {
                firstTop = 0;
            }
            final View child = getChildAt(i);
            final int childWidth = child.getMeasuredWidth();
            final int childHeight = child.getMeasuredHeight();
            child.layout(MARGIN, childTop + firstTop, childWidth - MARGIN, childTop + childHeight-childBottom);
            childTop += childHeight;
        }
    }

    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        final int count = getChildCount();
        int totalHeight = 0;
        for (int i = 0; i < count; ++i) {
            final View child = getChildAt(i);
            measureChild(child, widthMeasureSpec, heightMeasureSpec);
            totalHeight += child.getMeasuredHeight();
        }

        setMeasuredDimension(getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec),
                getDefaultSize(totalHeight,heightMeasureSpec));
    }

    public Bitmap exportBitmap() {
        /*
         * BUG FIX:4069
         * BUG COMMENT:solve for oom
         * FIX DATE:2013-05-29
         */
        return exportScaledBitmap(getWidth(), Math.min(getHeight(), 4000), true);
    }
}
