/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.ugallery;

import android.util.DisplayMetrics;

/**
 * There are two cell size we will use. It can be set by setSizeChoice().
 * The mLeftEdgePadding fields is filled in onLayout(). See the comments
 * in onLayout() for details.
 */
public class LayoutSpec {
    int mCellWidth, mCellHeight;
    int mCellSpacing;
    int mLeftEdgePadding;
    int mCountSize;
    int mTitleSize;
    DisplayMetrics mMetrics;

    public LayoutSpec(int w, int h, int intercellSpacing, int leftEdgePadding, DisplayMetrics metrics) {
        mMetrics = metrics;
        mCellWidth = dpToPx(w, metrics);
        mCellHeight = dpToPx(h, metrics);
        mCellSpacing = dpToPx(intercellSpacing, metrics);
        mLeftEdgePadding = dpToPx(leftEdgePadding, metrics);
        mCountSize = dpToPx(36, metrics);
        mTitleSize = dpToPx(14, metrics);
    }
    public void setColumns (int columns) {
        mCellWidth = mCellHeight = (mMetrics.widthPixels - dpToPx(5*(columns+1), mMetrics)) / columns ;
    }
    public int getImageWidth() {
        return mCellWidth;
    }
    public int getImageHeight() {
        return mCellHeight;
    }
    // Converts dp to pixel.
    private static int dpToPx(int dp, DisplayMetrics metrics) {
        return (int) (metrics.density * dp + 0.5f);
    }
}
