/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle;

import android.content.Context;
import android.net.Uri;
import android.widget.ImageView;

public abstract class PuzzlePiece extends ImageView {

    protected int mPuzzleNum;
    protected Uri mBitmapUri;
    public PuzzlePiece(Context context, int num, Uri uri) {
        super(context);
        this.mPuzzleNum = num;
        this.mBitmapUri = uri;
    }

    ///////////////////////////////////////////////////////////////////
    //      USER DEFINED METHODS
    public void adjustScroll() {}

    public boolean isPointInView(int x, int y) {
        return (x > mLeft && x < mRight ) && (y >= mTop && y < mBottom);
    }

    public void borrowBitmap(PuzzlePiece src){
        setImageDrawable(src.getDrawable());
    }
    public void rotateBitmap() {}
    public void oprateHorizonFlipBitmap() {}
    public int getNum() {
        return mPuzzleNum;
    }
    public Uri getUri() {
        return mBitmapUri;
    }
    public void updateUri(Uri uri) {
        mBitmapUri = uri;
    }
    public void updateNum(int num) {
        mPuzzleNum = num;
    }
}
