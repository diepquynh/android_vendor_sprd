/**
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */
/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


package com.ucamera.uphoto.brush;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ComposeShader;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.Bitmap.Config;
import android.graphics.Shader.TileMode;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import com.ucamera.uphoto.R;

public class BrushColorPickerView extends View{
    private Context mContext;
    private int mViewHeight;
    private int mViewWidth;
    private int mPanelWidth;
    private int mIndicatorWidth;
    private float mIndicatorHeight;
    private static int SPACING;
    private Bitmap mIndicator;
    private Bitmap mPanelBitmap;
    private Paint mBitmapPaint;
    private Paint mIndicatorPaint;
    private int[] mIndicatorColors;
    private boolean mTouchPanel = false;
    private boolean mTouchIndicator = false;
    private PointF mPanelPointF;
    private PointF mIndicatorPointF;
    private OnColorChangedListener mListener;
    private int mCurrentColor = Integer.MAX_VALUE;
    private DisplayMetrics mMetrics;
    private BitmapFactory.Options mOptions = null;
    private boolean mIsFirstDrawn = true;

    public interface OnColorChangedListener {
        void onColorChanged(int color);
    }

    public void reset() {
        mIndicatorColors = null;
        mPanelPointF = null;
        mIsFirstDrawn = true;
        mCurrentColor = Integer.MAX_VALUE;
        init();
        invalidate();
    }

    public int getCurrentColor() {
        return mCurrentColor;
    }

    public BrushColorPickerView(Context context) {
        this(context, null);
    }

    public BrushColorPickerView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        init();
    }

    public void setListener(OnColorChangedListener listener) {
        mListener = listener;
    }

    private void init() {
        mIndicatorPaint = new Paint();
        mIndicatorPaint.setStyle(Paint.Style.FILL);
        mIndicatorPaint.setStrokeWidth(1);
        mIndicatorColors = new int[3];
        mIndicatorColors[0] = Color.WHITE;
        mIndicatorColors[2] = Color.BLACK;
        mBitmapPaint = new Paint();
        mPanelPointF = new PointF(SPACING, SPACING);
        mIndicator = BitmapFactory.decodeResource(mContext.getResources(), R.drawable.ic_colors_selecter);
        mIndicatorPointF = new PointF(SPACING, SPACING);
        mIndicatorHeight = mIndicator.getHeight();
        mIndicatorWidth = mIndicator.getWidth();
        mOptions = new BitmapFactory.Options();
        mOptions.inJustDecodeBounds = true;
        BitmapFactory.decodeResource(mContext.getResources(), R.drawable.edit_brush_color_bg, mOptions);

        mMetrics = new DisplayMetrics();
        WindowManager wm = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
        wm.getDefaultDisplay().getMetrics(mMetrics);
        SPACING = mMetrics.widthPixels / 20;
    }

    @Override
    protected void onDraw(Canvas canvas) {
        canvas.drawBitmap(getGradual(), null, new Rect(SPACING / 2, SPACING, mPanelWidth
                + SPACING / 2, mViewHeight - SPACING), mBitmapPaint);
        mIndicatorColors[1] = mIndicatorPaint.getColor();
        Shader indicatorShader = new LinearGradient(mViewWidth - SPACING - mIndicatorWidth / 2,
                SPACING, mViewWidth - SPACING - mIndicatorWidth / 2, mViewHeight - SPACING,
                mIndicatorColors, null, Shader.TileMode.MIRROR);
        mIndicatorPaint.setShader(indicatorShader);
        canvas.drawRect(new Rect(mViewWidth - SPACING * 3 / 2 - mIndicatorWidth, SPACING, mViewWidth
                - SPACING * 3 / 2, mViewHeight - SPACING), mIndicatorPaint);
        if(mIsFirstDrawn) {
            mIsFirstDrawn = false;
            mIndicatorPointF.y = SPACING - mIndicatorHeight / 2;
        }
        canvas.drawBitmap(mIndicator, mViewWidth - SPACING * 3 / 2 - mIndicatorWidth, mIndicatorPointF.y, mBitmapPaint);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int widthMode = MeasureSpec.getMode(widthMeasureSpec);
        int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);
        if (widthMode == MeasureSpec.EXACTLY) {
            mViewWidth = width;
        } else {
            mViewWidth = mMetrics.widthPixels;
        }
        if (heightMode == MeasureSpec.EXACTLY) {
            mViewHeight = height;
        } else {
            mViewHeight = (int) (mOptions.outHeight * mMetrics.density);
        }
        mPanelWidth = mViewWidth - SPACING * 3 - mIndicatorWidth;
        setMeasuredDimension(mViewWidth, mViewHeight);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_MOVE:
                mTouchPanel = isInPanel(x, y);
                mTouchIndicator = isInIndicator(x, y);
                if (mTouchPanel) {
                    resetPanelPoint(x, y);
                    mIndicatorPaint.setColor(getPanelColor(mPanelPointF.x - SPACING  / 2,
                            mPanelPointF.y - SPACING));
                } else if (mTouchIndicator) {
                    resetIndicatorPoint(x, y);
                }
                invalidate();
                if(mTouchIndicator) {
                    int selectedColor = getIndicatorColor(mIndicatorPointF.y);
                    if (mCurrentColor == Integer.MAX_VALUE || mCurrentColor != selectedColor) {
                        mCurrentColor = selectedColor;
                    } else {
                        break;
                    }
                    if (mListener != null) {
                        mListener.onColorChanged(mCurrentColor);
                    }
                }
                break;
            case MotionEvent.ACTION_UP:
                invalidate();
                if (mTouchIndicator && mListener != null) {
                    mListener.onColorChanged(getIndicatorColor(mIndicatorPointF.y));
                }
                mTouchPanel = false;
                mTouchIndicator = false;
        }
        return true;
    }

    @Override
    protected void onDetachedFromWindow() {
        if (mPanelBitmap != null) {
            mPanelBitmap.recycle();
        }
        if (mIndicator != null) {
            mIndicator.recycle();
        }
        super.onDetachedFromWindow();
    }

    private Bitmap getGradual() {
        if (mPanelBitmap == null) {
            Paint panelPaint = new Paint();
            panelPaint.setStrokeWidth(1);
            mPanelBitmap = Bitmap.createBitmap(mPanelWidth, mViewHeight - 2 * SPACING, Config.RGB_565);
            Canvas canvas = new Canvas(mPanelBitmap);
            int bitmapWidth = mPanelBitmap.getWidth();
            mPanelWidth = bitmapWidth;
            int bitmapHeight = mPanelBitmap.getHeight();
            int[] panelColors = new int[] {
                    Color.RED, Color.YELLOW, Color.GREEN, Color.CYAN, Color.BLUE, Color.MAGENTA
            };

            Shader panelShader = new LinearGradient(0, bitmapHeight / 2, bitmapWidth, bitmapHeight / 2, panelColors, null, TileMode.REPEAT);
            LinearGradient shadowShader = new LinearGradient(bitmapWidth / 2, 0, bitmapWidth / 2, bitmapHeight, Color.WHITE, Color.BLACK, Shader.TileMode.CLAMP);

            ComposeShader shader = new ComposeShader(panelShader, shadowShader, PorterDuff.Mode.SCREEN);
            panelPaint.setShader(shader);
            canvas.drawRect(0, 0, bitmapWidth, bitmapHeight, panelPaint);
        }
        return mPanelBitmap;
    }

    private boolean isInPanel(float x, float y) {
        if (0 < x && x < SPACING + mPanelWidth + SPACING / 2 && 0 < y && y < mViewWidth) {
            return true;
        } else {
            return false;
        }
    }

    private boolean isInIndicator(float x, float y) {
        if (mViewWidth - SPACING * 3 / 2 - mIndicatorWidth < x && x < mViewWidth && y > SPACING - mIndicatorHeight / 2
                && y < mViewHeight - SPACING - mIndicatorHeight / 2) {
            return true;
        } else {
            return false;
        }
    }

    private void resetPanelPoint(float x, float y) {
        if (x < SPACING) {
            mPanelPointF.x = SPACING;
        } else if (x > (SPACING + mPanelWidth)) {
            mPanelPointF.x = SPACING + mPanelWidth;
        } else {
            mPanelPointF.x = x;
        }
        if (y < SPACING) {
            mPanelPointF.y = SPACING;
        } else if (y > (mViewHeight - SPACING)) {
            mPanelPointF.y = mViewHeight - SPACING;
        } else {
            mPanelPointF.y = y;
        }
    }

    private void resetIndicatorPoint(float x, float y) {
        if (x < SPACING) {
            mIndicatorPointF.x = SPACING;
        } else if (x > (SPACING + mPanelWidth)) {
            mIndicatorPointF.x = SPACING + mPanelWidth;
        } else {
            mIndicatorPointF.x = x;
        }
        if (y < SPACING) {
            mIndicatorPointF.y = SPACING - mIndicatorHeight / 2;
        } else if (y > (mViewHeight - SPACING)) {
            mIndicatorPointF.y = mViewHeight - SPACING + mIndicatorHeight / 2;
        } else {
            mIndicatorPointF.y = y;
        }
        mIndicatorPointF.y = y;
    }

    private int getPanelColor(float x, float y) {
        Bitmap bmp = getGradual();
        int intX = (int) x;
        int intY = (int) y;
        if (intY >= bmp.getHeight()) {
            intY = bmp.getHeight() - 1;
        }
        if (intX >= bmp.getWidth()) {
            intX = bmp.getWidth() - 1;
        }
        return bmp.getPixel(intX, intY);
    }

    private int getIndicatorColor(float y) {
        int a, r, g, b, so, dst;
        float p;
        float halfH = (mViewHeight - (float) SPACING * 2 + mIndicatorHeight) / 2;
        if (y < halfH) {
            so = mIndicatorColors[0];
            dst = mIndicatorColors[1];
            p = y / halfH;
        } else {
            so = mIndicatorColors[1];
            dst = mIndicatorColors[2];
            p = (y - halfH) / halfH;
        }
        a = ave(Color.alpha(so), Color.alpha(dst), p);
        r = ave(Color.red(so), Color.red(dst), p);
        g = ave(Color.green(so), Color.green(dst), p);
        b = ave(Color.blue(so), Color.blue(dst), p);
        return Color.argb(a, r, g, b);
    }

    private int ave(int s, int d, float p) {
        return s + Math.round(p * (d - s));
    }
}
