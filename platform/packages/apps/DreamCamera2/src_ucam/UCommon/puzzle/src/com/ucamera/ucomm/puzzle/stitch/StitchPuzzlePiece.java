/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucomm.puzzle.stitch;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Paint.Style;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;

import com.ucamera.ucomm.puzzle.PuzzlePiece;

public class StitchPuzzlePiece extends PuzzlePiece {

    public StitchPuzzlePiece(Context context, int num, Uri uri) {
        super(context, num ,uri);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        /*
         *BUG FIX:5025
         *BUG CAUSE:trying to use a recycled bitmap
         *FIX DATE:2013-11-12
         **/
        BitmapDrawable drawable = (BitmapDrawable) getDrawable();
        Bitmap bitmap = drawable != null ? drawable.getBitmap() : null;
        if (bitmap != null && !bitmap.isRecycled()) {
            super.onDraw(canvas);
        }
        paintSelection(canvas);
    }

    @Override
    public void setImageBitmap(Bitmap bm) {
        if(!bm.isRecycled()){
            super.setImageBitmap(bm);
        }
    }

    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);
        final Drawable drawable = getDrawable();
        if (drawable != null) {
            height = width * drawable.getIntrinsicHeight() / drawable.getIntrinsicWidth();
        }
        setMeasuredDimension(width, height);
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
        }
    }
}
