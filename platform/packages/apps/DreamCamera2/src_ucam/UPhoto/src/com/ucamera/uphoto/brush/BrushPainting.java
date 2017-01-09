/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto.brush;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;

import com.ucamera.uphoto.ImageEditControlActivity;
import com.ucamera.uphoto.ImageEditViewGraffitiView;

import java.util.ArrayList;
import java.util.Stack;

public class BrushPainting {
    public int mBrushAlpha;
    public int mBrushColor;
    public int mBrushMode;
    public float mBrushSize;
    public int mBrushStyle;
    private Canvas mCanvas;
    private BrushStroker mCurrentStroker;
    private BaseBrush mCurrentBrush;
    /*SPRD: CID 109325 (#1 of 2): UrF: Unread field (FB.URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD)
    public int mPaintingHeight;
    public int mPaintingWidth;
    */
    private ImageEditControlActivity mContext;
    public ImageBrushManager mImageBurshManager;
    private QuadCurve mQuadCurve;
    private RandomColorPicker mRandomColorPicker;
    private MyPoint mStartPoint;
    private MyPoint mStopPoint;
    private ArrayList<BrushStroker> mStoreStrokerList;
    private Stack<BrushStroker> mUndoStrokerStack;
    private Bitmap mBitmap;
    private ImageEditViewGraffitiView mGraffitiView;
    private int mCurrentStrokeIndex = -1;
    private int mCanvasColor = 0;
    private PaintChangedListener mListener;
    public BrushPainting(ImageEditControlActivity context) {
        mCanvas = null;
        mBrushMode = BrushConstant.BrushModeRandom;
        mBrushColor = Color.RED;
        mBrushSize = 5F;
        mBrushAlpha = 255;
        mStartPoint = new MyPoint();
        mStopPoint = new MyPoint();
        mRandomColorPicker = new RandomColorPicker(32);
        mQuadCurve = new QuadCurve();
        mContext = context;
        mImageBurshManager = new ImageBrushManager(mContext);
        mStoreStrokerList = new ArrayList<BrushStroker>();
        mUndoStrokerStack = new Stack<BrushStroker>();
        mBitmap = null;
    }

    public interface PaintChangedListener {
        public void onChanged();
    }

    public void setListener(PaintChangedListener listener) {
        mListener = listener;
    }

    public BaseBrush createBrush() {
        BaseBrush brush = BaseBrush.createBrush(mBrushStyle);
        brush.setContext(mContext);
        brush.setQuadDecompositor(mQuadCurve);
        brush.setImageBurshManager(mImageBurshManager);
        float density = mContext.getResources().getDisplayMetrics().density;
        brush.setScale(density);
        brush.setRandomColorPicker(mRandomColorPicker);
        brush.setColor(mBrushColor);
        brush.setAlpha(mBrushAlpha);
        brush.setSize(mBrushSize);
        brush.setMode(mBrushMode);

        return brush;
    }

    public void createCanvas(int width, int height, Canvas canvas, Bitmap bitmap, ImageEditViewGraffitiView view, int canvasColor) {
        /*SPRD: CID 109325 (#1 of 2): UrF: Unread field (FB.URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD)
        mPaintingWidth = width;
        mPaintingHeight = height;
        */

        if(mBitmap != null) {
            mBitmap.recycle();
            mBitmap = null;
        }
        mBitmap = bitmap;

        mCanvas = canvas;
        mGraffitiView = view;
        mCanvasColor = canvasColor;
    }

    public BaseBrush getBrush() {
        return mCurrentBrush;
    }

    public int getBrushStyle() {
        return mBrushStyle;
    }

    public void setAlpha(int brushAlpha) {
        mBrushAlpha = brushAlpha;
    }

    public void setBrushColor(int brushColor) {
        mBrushColor = brushColor;
    }

    public void setBrushMode(int brushMode) {
        mBrushMode = brushMode;
    }

    public void setBrushSize(float brushSize) {
        mBrushSize = brushSize;
    }

    public void setBrushStyle(int brushStyle) {
        mBrushStyle = brushStyle;
    }

    public void strokeFrom(float x, float y) {
        mStartPoint.mX = x;
        mStartPoint.mY = y;
        mCurrentBrush = createBrush();
        mCurrentStroker = new BrushStroker();
        mCurrentStroker.setBrush(mCurrentBrush);
        mCurrentStroker.strokeFrom(x, y);
    }

    public void strokeTo(float x, float y) {
        mCurrentStroker.strokeTo(mCanvas, x, y);
    }

    public void strokeEnd(float x, float y) {
        mStopPoint.mX = x;
        mStopPoint.mY = y;
        strokeTo(mStopPoint.mX, mStopPoint.mY);
        mCurrentStroker.strokeFinish();

        mStoreStrokerList.add(mCurrentStroker);
        mCurrentStrokeIndex = mCurrentStrokeIndex + 1;
        if(mListener != null){
            mListener.onChanged();
        }
    }

    public void undoStroke() {
        BrushStroker stroker = getLastBrushStroke();
        if(stroker != null) {
            mStoreStrokerList.remove(stroker);
            mCurrentStrokeIndex = mCurrentStrokeIndex - 1;
            mUndoStrokerStack.push(stroker);
            if(mCanvasColor == 0) {
                mBitmap.eraseColor(Color.TRANSPARENT);
            } else {
                mBitmap.eraseColor(mCanvasColor);
            }
            mCanvas.drawBitmap(mBitmap, 0f, 0f, null);
            drawStrokes(mStoreStrokerList.size());
            mGraffitiView.invalidate();
            if(mListener != null){
                mListener.onChanged();
            }
        }
    }

    public void redoStroke() {
        if(!mUndoStrokerStack.isEmpty()) {
            BrushStroker stroker = (BrushStroker)mUndoStrokerStack.pop();
            mStoreStrokerList.add(stroker);
            mCurrentStrokeIndex = mCurrentStrokeIndex + 1;
            stroker.reDraw(mCanvas);
            mGraffitiView.invalidate();
            if(mListener != null){
                mListener.onChanged();
            }
        }
    }

    private BrushStroker getLastBrushStroke() {
        BrushStroker stroker = null;
        if(mStoreStrokerList.isEmpty()) {
            stroker = null;
        } else {
            stroker = mStoreStrokerList.get(mStoreStrokerList.size() - 1);
        }

        return stroker;
    }

    private void drawStrokes(int strokesCount) {
        int index = 0;
        while (index < strokesCount) {
            BrushStroker stroker = (BrushStroker)mStoreStrokerList.get(index);
            stroker.reDraw(mCanvas);
            index++;
        }
    }


    public int getStrokeCount() {
        return mStoreStrokerList.size();
    }

    public int getCurrentStrokeIndex() {
        return mCurrentStrokeIndex;
    }

    public boolean hasStrokesInStack() {
        return !mUndoStrokerStack.isEmpty();
    }

    public int getStackCount() {
        return mUndoStrokerStack.size();
    }

    public boolean stackIsEmpty() {
        return mUndoStrokerStack.isEmpty();
    }

    public void clearUndoStrokers() {
        mUndoStrokerStack.clear();
        if(mListener != null){
            mListener.onChanged();
        }
    }

    public void clearAllStrokers() {
        mStoreStrokerList.clear();
        mUndoStrokerStack.clear();
        mCurrentStrokeIndex = -1;
        if(mListener != null){
            mListener.onChanged();
        }
    }
}
