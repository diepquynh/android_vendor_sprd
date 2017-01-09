/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.Log;

public abstract class BaseBrush {
    protected static String TAG = "BaseBrush";
    public Context mContext;
    public int mBrushAlphaValue;
    public int mBrushColor;
    public int mBrushMode;
    protected Paint mBrushPaint;
    public Rect mDirtyRect;
    public boolean mIsRandomColor;
    protected float mSizeBias;
    public float mBrushSize;
    public float mBrushMaxSize;
    public float mBrushMinSize;
    public float mSizeLowerBound;
    public float mSizeUpperBound;
    public Bitmap mBrushBitmap;
    protected RandomColorPicker mRandomColorPicker;
    public ImageBrushManager mImageBrushManager;
    public int mBrushBitmapWidth;
    public int mBrushBitmapHeight;
    public int mBrushStyle;
    public QuadCurve mQuadCurve;
    public int mBrushGreyValue;
    // CID 124604 : UuF: Unused field (FB.UUF_UNUSED_PUBLIC_OR_PROTECTED_FIELD)
    // public boolean mBrushHasAlpha;

    public BaseBrush() {
        mSizeBias = 1F;
        mBrushAlphaValue = 255;
        mBrushColor = -1;
        mIsRandomColor = false;
        mDirtyRect = new Rect();
        mBrushPaint = new Paint();
        mBrushMode = BrushConstant.BrushModeSelected;
    }

    public static BaseBrush createBrush(int brushStyle) {
        BaseBrush brush = null;
        switch (brushStyle) {
            case BrushConstant.RainbowBrush:
                brush = new RainbowBrush();
                break;
            case BrushConstant.GradientBrush:
                brush = new GradientBrush();
                break;
            case BrushConstant.PenInkBrush:
                brush = new PenInkBrush();
                break;
            case BrushConstant.NeonBrush:
                brush = new NeonBrush(BrushConstant.BrushModeRandom);
                break;
            /*case BrushConstant.SketchBrush:
                brush = new SketchBrush(BrushConstant.BrushModeRandom);
                break;*/
            case BrushConstant.LineBrush:
                brush = new LineBrush();
                break;
            case BrushConstant.EmbossBrush:
                brush = new EmbossBrush();
                break;
            case BrushConstant.EraserBrush:
                brush = new EraserBrush();
                break;
            case BrushConstant.RectThinBrush:
            case BrushConstant.RectDenseBrush:
            case BrushConstant.RectFilamentousBrush:
            case BrushConstant.RectBlurBrush:
            case BrushConstant.RectThornsBrush:
            case BrushConstant.RectOilyBrush:
            case BrushConstant.RoundMistBrush:
            case BrushConstant.RoundThinBrush:
            case BrushConstant.RoundDenseBrush:
            case BrushConstant.OvalBrush:
            case BrushConstant.RectMeshBrush:
            case BrushConstant.RectMistBrush:
                brush = new ImageBrush(brushStyle, null);
                break;
            default:
                brush = new LineBrush();
                break;
        }

        return brush;
    }

    public static BaseBrush createBrush(int burshStyle, ImageBrushManager imageBurshManager) {
        return new ImageBrush(burshStyle, imageBurshManager);
    }

    private void setRGB(Bitmap bitmap, int brushColor) {
        if(bitmap == null) {
            return;
        }

        int y = 0;
        int x = 0;
        int bitmapWidth = bitmap.getWidth();
        do{
            if(x > bitmapWidth) {
                break;
            }
            y++;
            int bitmapPixel = bitmap.getPixel(x, y);
            int xorBrushColor = Color.WHITE & brushColor;
            int xorBitmapPixel = Color.BLACK & bitmapPixel;
            int color = xorBrushColor | xorBitmapPixel;
            bitmap.setPixel(x, y, color);
            x++;
        } while(true);
    }


    protected void clampBrushSize() {
        mBrushSize = Math.max(mBrushMinSize, Math.min(mBrushMaxSize, mBrushSize));
    }

    public Rect drawStroke(Canvas canvas, Path path) {
        Rect rect = null;
        try {
            Paint paint = mBrushPaint;
            canvas.drawPath(path, paint);
            updateDirtyRect(path);
            rect = mDirtyRect;
        } catch(NullPointerException nullPointerException) {
            rect = null;
        }

        return rect;
    }

    public Rect drawStroke(Canvas canvas, MyPoint sPoint, MyPoint cPoint, MyPoint ePoint) {
        return null;
    }


    public void endStroke() {

    }

    public int getAlpha() {
        return mBrushAlphaValue;
    }

    public int getColor() {
        return mBrushColor;
    }

    public Rect getDirtyRect() {
        return mDirtyRect;
    }

    public Paint getPaint() {
        return mBrushPaint;
    }

    public RandomColorPicker getRandomColorPicker() {
        return mRandomColorPicker;
    }

    public float getSize() {
        return mBrushSize;
    }

    public void prepareBrush() {

    }

    public void restoreSize(float brushSize) {
        mBrushSize = brushSize;
    }

    public void setAlpha(int brushAlphaValue) {
        mBrushAlphaValue = brushAlphaValue;
    }

    public void setColor(int brushColor) {
        mBrushColor = brushColor;
        if(mBrushBitmap != null) {
            setRGB(mBrushBitmap, brushColor);
        }
    }

    public void setContext(Context context) {
        mContext = context;
    }

    public void setMode(int brushMode) {
        boolean isRandom = false;
        mBrushMode = brushMode;
        if(mBrushMode == BrushConstant.BrushModeRandom) {
            isRandom = true;
        }

        mIsRandomColor = isRandom;
    }

    public void setImageBurshManager(ImageBrushManager imageBrushManager) {
        mImageBrushManager = imageBrushManager;
    }

    public void setQuadDecompositor(QuadCurve quadCurve) {
        mQuadCurve = quadCurve;
    }

    public void setRandomColorPicker(RandomColorPicker randomColorPicker) {
        mRandomColorPicker = randomColorPicker;
    }

    public void setScale(float scale) {

    }

    public void setSize(float brushSize) {
        mBrushSize = brushSize;
        clampBrushSize();
        setSizeBound();
    }

    protected void setSizeBound() {
        mSizeLowerBound = Math.max((mBrushSize - mSizeBias), mBrushMinSize);
        mSizeUpperBound = Math.min((mBrushSize + mSizeBias), mBrushMaxSize);
    }

    public void updateBrush() {

    }

    public float[] getBrushData() {
        return null;
    }

    public void restoreBrush(float[] brushData) {

    }

    protected void updateDirtyRect(Path path) {
        RectF rectf = new RectF();
        path.computeBounds(rectf, false);
        float strokeWidth = mBrushPaint.getStrokeWidth();

        float newLeft = rectf.left - strokeWidth;
        float newRight = rectf.right + strokeWidth;
        float newTop = rectf.top - strokeWidth;
        float newBottom = rectf.bottom + strokeWidth;

        int left = Math.max((int)newLeft, 0);
        int right = (int)newRight;
        int top = Math.max((int)newTop, 0);
        int bottom = (int)newBottom;

        mDirtyRect.set(left, top, right, bottom);
    }

    //EmbossBrush, NeonBrush
    protected void updateDirtyRect(Path path, float size) {
        RectF rectf = new RectF();
        path.computeBounds(rectf, false);
        float newLeft = rectf.left - size;
        float newRight = rectf.right + size;
        float newTop = rectf.top - size;
        float newBottom = rectf.bottom + size;

        int left = Math.max((int)newLeft, 0);
        int right = (int)newRight;
        int top = Math.max((int)newTop, 0);
        int bottom = (int)newBottom;

        mDirtyRect.set(left, top, right, bottom);
    }
}
