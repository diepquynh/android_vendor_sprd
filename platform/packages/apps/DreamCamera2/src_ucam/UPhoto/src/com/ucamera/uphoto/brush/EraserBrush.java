/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;

public class EraserBrush extends BaseBrush {
    public EraserBrush() {
        mBrushMaxSize = 48F;
        mBrushMinSize = 1F;
        mBrushSize = 24F;
        /*SPRD: CID 109253 (#1 of 1): UrF: Unread field (FB.URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD)
        mBrushHasAlpha = false;
        */
        mBrushStyle = BrushConstant.EraserBrush;
        mBrushMode = BrushConstant.BrushModeSelected;
        mIsRandomColor = false;
    }

    public void prepareBrush() {
        mBrushPaint.setAntiAlias(true);
        mBrushPaint.setStrokeCap(android.graphics.Paint.Cap.ROUND);
        mBrushPaint.setStrokeJoin(android.graphics.Paint.Join.ROUND);
        mBrushPaint.setStyle(android.graphics.Paint.Style.STROKE);
        mBrushPaint.setXfermode(new PorterDuffXfermode(PorterDuff.Mode.CLEAR));
        mBrushPaint.setStrokeWidth(mBrushSize);
        mBrushPaint.setColor(mBrushColor);
    }
}
