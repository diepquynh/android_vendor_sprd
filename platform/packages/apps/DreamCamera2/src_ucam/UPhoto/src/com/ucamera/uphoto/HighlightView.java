/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 *
 *  Copyright (C) 2007 The Android Open Source Project
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

package com.ucamera.uphoto;

import com.ucamera.uphoto.CropImage.CROPMODE;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.DashPathEffect;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PathEffect;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;

// This class is used by CropImage to display a highlighted cropping rectangle
// overlayed with the image. There are two coordinate spaces in use. One is
// image, another is screen. computeLayout() uses mMatrix to map from image
// space to screen space.
class HighlightView {

    private static final String TAG = "HighlightView";
    View mContext;  // The View displaying the image.

    public static final int GROW_NONE        = (1 << 0);
    public static final int GROW_LEFT_EDGE   = (1 << 1);
    public static final int GROW_RIGHT_EDGE  = (1 << 2);
    public static final int GROW_TOP_EDGE    = (1 << 3);
    public static final int GROW_BOTTOM_EDGE = (1 << 4);
    public static final int MOVE             = (1 << 5);
    public static final int GROW_LEFT_TOP_CORNER = (1 << 6);
    public static final int GROW_RIGHT_TOP_CORNER = (1 << 7);
    public static final int GROW_LEFT_BOTTOM_CORNER = (1 << 8);
    public static final int GROW_RIGHT_BOTTOM_CORNER = (1 << 9);

    public HighlightView(View ctx) {
        mContext = ctx;
    }

    private void init() {
        android.content.res.Resources resources = mContext.getResources();
        mResizeDrawableWidth =
                resources.getDrawable(R.drawable.camera_crop_width);
        mResizeDrawableHeight =
                resources.getDrawable(R.drawable.camera_crop_height);
        mResizeDrawableDiagonal =
                resources.getDrawable(R.drawable.indicator_autocrop);
    }

    boolean mIsFocused;
    boolean mHidden;

    public boolean hasFocus() {
        return mIsFocused;
    }

    public void setFocus(boolean f) {
        mIsFocused = f;
    }

    public void setHidden(boolean hidden) {
        mHidden = hidden;
    }

    protected void draw(Canvas canvas) {
        if (mHidden) {
            return;
        }
        canvas.save();
        initLineRect();
        Path path = new Path();
        if (!hasFocus()) {
            mOutlinePaint.setColor(0xFF000000);
            canvas.drawRect(mDrawRect, mOutlinePaint);

        } else {
            Rect viewDrawingRect = new Rect();
            mContext.getDrawingRect(viewDrawingRect);
            if (mCircle) {
                float width  = mDrawRect.width();
                float height = mDrawRect.height();
                path.addCircle(mDrawRect.left + (width  / 2),
                               mDrawRect.top + (height / 2),
                               width / 2,
                               Path.Direction.CW);
                mOutlinePaint.setColor(0xFFEF04D6);
            } else {
                path.addRect(new RectF(mDrawRect), Path.Direction.CW);
                //mOutlinePaint.setColor(0xFFFF8A00);
                mOutlinePaint.setColor(Color.WHITE);
            }
            try{
                /*
                 * BUG FIX: 618
                 * FIX COMMENT: this method is not support in hardware acceleration mode
                 *   see: android.view.GLES20Canvas for more info
                 * Date: 2012-08-30
                 */
                //canvas.clipPath(path, Region.Op.DIFFERENCE);
            }catch(Exception e){
                Log.e(TAG,"Error calling 'clipPath' of class:" + canvas.getClass().getName());
            }
            /*
             * BUG FIX: 3903, 3892
             * BUG COMMENT: draw gray rect
             * FIX DATE: 2013-05-24
             * */
            canvas.drawRect(new Rect(viewDrawingRect.left, viewDrawingRect.top, viewDrawingRect.right, mDrawRect.top), hasFocus() ? mFocusPaint : mNoFocusPaint);
            canvas.drawRect(new Rect(viewDrawingRect.left, mDrawRect.top, mDrawRect.left, mDrawRect.bottom), hasFocus() ? mFocusPaint : mNoFocusPaint);
            canvas.drawRect(new Rect(mDrawRect.right, mDrawRect.top, viewDrawingRect.right, mDrawRect.bottom), hasFocus() ? mFocusPaint : mNoFocusPaint);
            canvas.drawRect(new Rect(viewDrawingRect.left, mDrawRect.bottom, viewDrawingRect.right, viewDrawingRect.bottom), hasFocus() ? mFocusPaint : mNoFocusPaint);
            /*canvas.drawRect(viewDrawingRect,
                    hasFocus() ? mFocusPaint : mNoFocusPaint);*/

            canvas.restore();
            drawLineRect(canvas);
            drawCorner(canvas);
            canvas.drawPath(path, mOutlinePaint);
            /*Paint p = new Paint();
            p.setColor(Color.RED);
            p.setTextSize(16);
            canvas.drawText(Integer.toString(mDrawRect.width()) + "*" +Integer.toString(mDrawRect.height()),
                    mDrawRect.left + (mDrawRect.width()  / 2), mDrawRect.top + (mDrawRect.height() / 2), p);*/

            /*if (mMode == ModifyMode.Grow) {
                if (mCircle) {
                    int width  = mResizeDrawableDiagonal.getIntrinsicWidth();
                    int height = mResizeDrawableDiagonal.getIntrinsicHeight();

                    int d  = (int) Math.round(Math.cos(45degMath.PI / 4D)
                            * (mDrawRect.width() / 2D));
                    int x  = mDrawRect.left
                            + (mDrawRect.width() / 2) + d - width / 2;
                    int y  = mDrawRect.top
                            + (mDrawRect.height() / 2) - d - height / 2;
                    mResizeDrawableDiagonal.setBounds(x, y,
                            x + mResizeDrawableDiagonal.getIntrinsicWidth(),
                            y + mResizeDrawableDiagonal.getIntrinsicHeight());
                    mResizeDrawableDiagonal.draw(canvas);
                } else {
                    int left    = mDrawRect.left   + 1;
                    int right   = mDrawRect.right  + 1;
                    int top     = mDrawRect.top    + 4;
                    int bottom  = mDrawRect.bottom + 3;

                    int widthWidth   =
                            mResizeDrawableWidth.getIntrinsicWidth() / 2;
                    int widthHeight  =
                            mResizeDrawableWidth.getIntrinsicHeight() / 2;
                    int heightHeight =
                            mResizeDrawableHeight.getIntrinsicHeight() / 2;
                    int heightWidth  =
                            mResizeDrawableHeight.getIntrinsicWidth() / 2;

                    int xMiddle = mDrawRect.left
                            + ((mDrawRect.right  - mDrawRect.left) / 2);
                    int yMiddle = mDrawRect.top
                            + ((mDrawRect.bottom - mDrawRect.top) / 2);

                    mResizeDrawableWidth.setBounds(left - widthWidth,
                                                   yMiddle - widthHeight,
                                                   left + widthWidth,
                                                   yMiddle + widthHeight);
                    mResizeDrawableWidth.draw(canvas);

                    mResizeDrawableWidth.setBounds(right - widthWidth,
                                                   yMiddle - widthHeight,
                                                   right + widthWidth,
                                                   yMiddle + widthHeight);
                    mResizeDrawableWidth.draw(canvas);

                    mResizeDrawableHeight.setBounds(xMiddle - heightWidth,
                                                    top - heightHeight,
                                                    xMiddle + heightWidth,
                                                    top + heightHeight);
                    mResizeDrawableHeight.draw(canvas);

                    mResizeDrawableHeight.setBounds(xMiddle - heightWidth,
                                                    bottom - heightHeight,
                                                    xMiddle + heightWidth,
                                                    bottom + heightHeight);
                    mResizeDrawableHeight.draw(canvas);
                }
            }*/
        }
    }

    public void setMode(ModifyMode mode) {
        if (mode != mMode) {
            mMode = mode;
            mContext.invalidate();
        }
    }

    // Determines which edges are hit by touching at (x, y).
    public int getHit(float x, float y) {
        Rect r = computeLayout();
        final float hysteresis = 20F;
        int retval = GROW_NONE;


        // verticalCheck makes sure the position is between the top and
        // the bottom edge (with some tolerance). Similar for horizCheck.
        boolean verticalCheck = (y >= r.top - hysteresis)
                && (y < r.bottom + hysteresis);
        boolean horizCheck = (x >= r.left - hysteresis)
                && (x < r.right + hysteresis);

        // Check whether the position is near some edge(s).
        if ((Math.abs(r.left - x)     < hysteresis)  &&  verticalCheck) {
            retval |= GROW_LEFT_EDGE;
        }
        if ((Math.abs(r.right - x)    < hysteresis)  &&  verticalCheck) {
            retval |= GROW_RIGHT_EDGE;
        }
        if ((Math.abs(r.top - y)      < hysteresis)  &&  horizCheck) {
            retval |= GROW_TOP_EDGE;
        }
        if ((Math.abs(r.bottom - y)   < hysteresis)  &&  horizCheck) {
            retval |= GROW_BOTTOM_EDGE;
        }

        if (((Math.abs(r.left - x) < hysteresis)  &&  verticalCheck) && ((Math.abs(r.top - y) < hysteresis)  &&  horizCheck)) {
            retval |= GROW_LEFT_TOP_CORNER;
        }
        if (((Math.abs(r.left - x)  < hysteresis)  &&  verticalCheck) && ((Math.abs(r.bottom - y) < hysteresis)  &&  horizCheck)) {
            retval |= GROW_LEFT_BOTTOM_CORNER;
        }
        if (((Math.abs(r.right - x) < hysteresis)  &&  verticalCheck) && ((Math.abs(r.top - y) < hysteresis)  &&  horizCheck)) {
            retval |= GROW_RIGHT_TOP_CORNER;
        }
        if (((Math.abs(r.right - x) < hysteresis)  &&  verticalCheck) && ((Math.abs(r.bottom - y)   < hysteresis)  &&  horizCheck)) {
            retval |= GROW_RIGHT_BOTTOM_CORNER;
        }
        // Not near any edge but inside the rectangle: move.
        if (retval == GROW_NONE && r.contains((int) x, (int) y)) {
            retval = MOVE;
        }
        return retval;
    }

    // Handles motion (dx, dy) in screen space.
    // The "edge" parameter specifies which edges the user is dragging.
    void handleMotion(int edge, float dx, float dy) {
        Rect r = computeLayout();
        if (edge == GROW_NONE) {
            return;
        } else if (edge == MOVE) {
            // Convert to image space before sending to moveBy().
            moveBy(dx * (mCropRect.width() / r.width()),
                   dy * (mCropRect.height() / r.height()));
        } else {
            if (((GROW_LEFT_EDGE | GROW_RIGHT_EDGE) & edge) == 0) {
                dx = 0;
            }

            if (((GROW_TOP_EDGE | GROW_BOTTOM_EDGE) & edge) == 0) {
                dy = 0;
            }

            if(((GROW_LEFT_BOTTOM_CORNER | GROW_LEFT_TOP_CORNER | GROW_RIGHT_BOTTOM_CORNER | GROW_RIGHT_TOP_CORNER) & edge) == 0) {
                Log.d("YYYY", "isnotcorner");
                if(CropImage.mMode == CROPMODE.RATIO) {
                    return;
                }
            }
            float xDelta = dx * (mCropRect.width() / r.width());
            float yDelta = dy * (mCropRect.height() / r.height());
            growBy((((edge & GROW_LEFT_EDGE) != 0) ? -1 : 1) * xDelta,
                    (((edge & GROW_TOP_EDGE) != 0) ? -1 : 1) * yDelta);
        }
    }

    // Grows the cropping rectange by (dx, dy) in image space.
    void moveBy(float dx, float dy) {
//        Rect invalRect = new Rect(mDrawRect);

        mCropRect.offset(dx, dy);

        // Put the cropping rectangle inside image rectangle.
        mCropRect.offset(
                Math.max(0, mImageRect.left - mCropRect.left),
                Math.max(0, mImageRect.top  - mCropRect.top));

        mCropRect.offset(
                Math.min(0, mImageRect.right  - mCropRect.right),
                Math.min(0, mImageRect.bottom - mCropRect.bottom));

        mDrawRect = computeLayout();
//        invalRect.union(mDrawRect);
//        invalRect.inset(-10, -10);
        mContext.invalidate();
    }

    // Grows the cropping rectange by (dx, dy) in image space.
    void growBy(float dx, float dy) {
        if (mMaintainAspectRatio) {
            if (dx != 0) {
                dy = dx / mInitialAspectRatio;
            } else if (dy != 0) {
                dx = dy * mInitialAspectRatio;
            }
        }

        // Don't let the cropping rectangle grow too fast.
        // Grow at most half of the difference between the image rectangle and
        // the cropping rectangle.
        RectF r = new RectF(mCropRect);
        if (dx > 0F && r.width() + 2 * dx > mImageRect.width()) {
            float adjustment = (mImageRect.width() - r.width()) / 2F;
            dx = adjustment;
            if (mMaintainAspectRatio) {
                dy = dx / mInitialAspectRatio;
            }
        }
        if (dy > 0F && r.height() + 2 * dy > mImageRect.height()) {
            float adjustment = (mImageRect.height() - r.height()) / 2F;
            dy = adjustment;
            if (mMaintainAspectRatio) {
                dx = dy * mInitialAspectRatio;
            }
        }

        r.inset(-dx, -dy);

        // Don't let the cropping rectangle shrink too fast.
        float widthCap = 25F;
        float heightCap = widthCap;
        if(mMaintainAspectRatio) {
            if(mInitialAspectRatio >= 1){
                heightCap = 25F;
                widthCap = heightCap * mInitialAspectRatio;
            } else {
                widthCap = 25F;
                heightCap = widthCap / mInitialAspectRatio;
            }
        } /*else {
            widthCap = 25F;
            heightCap = 25F;
        }*/
        /*if(mInitialAspectRatio >= 1){
            widthCap = 25F;
            heightCap = mMaintainAspectRatio ? (widthCap / mInitialAspectRatio) : widthCap;
        } else {
            heightCap = 25F;
            widthCap = mMaintainAspectRatio ? (heightCap * mInitialAspectRatio) : heightCap;
        }*/
        /*final float widthCap = 25F;
        float heightCap = mMaintainAspectRatio
                ? (widthCap / mInitialAspectRatio)
                : widthCap;*/
        if (r.width() < widthCap) {
            r.inset(-(widthCap - r.width()) / 2F, 0F);
        }

        if (r.height() < heightCap) {
            r.inset(0F, -(heightCap - r.height()) / 2F);
        }

        // Put the cropping rectangle inside the image rectangle.
        if (r.left < mImageRect.left) {
            r.offset(mImageRect.left - r.left, 0F);
        } else if (r.right > mImageRect.right) {
            r.offset(-(r.right - mImageRect.right), 0);
        }
        if (r.top < mImageRect.top) {
            r.offset(0F, mImageRect.top - r.top);
        } else if (r.bottom > mImageRect.bottom) {
            r.offset(0F, -(r.bottom - mImageRect.bottom));
        }

        mCropRect.set(r);
        mDrawRect = computeLayout();
        mContext.invalidate();
    }

    // Returns the cropping rectangle in image space.
    public Rect getCropRect() {
        return new Rect((int) mCropRect.left, (int) mCropRect.top,
                        (int) mCropRect.right, (int) mCropRect.bottom);
    }

    // Maps the cropping rectangle from image space to screen space.
    private Rect computeLayout() {
        RectF r = new RectF(mCropRect.left, mCropRect.top,
                            mCropRect.right, mCropRect.bottom);
        mMatrix.mapRect(r);
        return new Rect(Math.round(r.left), Math.round(r.top),
                        Math.round(r.right), Math.round(r.bottom));
    }

    public void invalidate() {
        mDrawRect = computeLayout();
    }

    public void setup(Matrix m, Rect imageRect, RectF cropRect, boolean circle,
                      boolean maintainAspectRatio/*, int width, int height*/) {
        /*mViewHeight = height;
        mViewWidth = width;*/
        if (circle) {
            maintainAspectRatio = true;
        }
        mMatrix = new Matrix(m);

        mCropRect = cropRect;
        mImageRect = new RectF(imageRect);
        mMaintainAspectRatio = maintainAspectRatio;
        mCircle = circle;

        mInitialAspectRatio = mCropRect.width() / mCropRect.height();
        mDrawRect = computeLayout();

        mFocusPaint.setARGB(125, 50, 50, 50);
        mNoFocusPaint.setARGB(125, 50, 50, 50);
        mOutlinePaint.setStrokeWidth(5F);
        mOutlinePaint.setStyle(Paint.Style.STROKE);
        mOutlinePaint.setAntiAlias(true);

        mMode = ModifyMode.None;
        init();
    }
    private void initLineRect()
    {
        int left = mDrawRect.left + mDrawRect.width() / 3;
        int right = mDrawRect.left + mDrawRect.width() / 3 * 2;
        int top = mDrawRect.top + mDrawRect.height() / 3;
        int bottom = mDrawRect.top + mDrawRect.height() / 3 *2;
        mDrawLeftLineRect = new Rect(left, mDrawRect.top, left + 1, mDrawRect.bottom);
        mDrawRightLineRect = new Rect(right, mDrawRect.top, right + 1, mDrawRect.bottom);
        mDrawTopLineRect = new Rect(mDrawRect.left, top, mDrawRect.right, top + 1);
        mDrawBottomLineRect = new Rect(mDrawRect.left, bottom, mDrawRect.right, bottom + 1);
    }
    private void drawLineRect(Canvas canvas)
    {
        Paint linePaint = new Paint();
        linePaint.setColor(Color.WHITE);
        linePaint.setStrokeWidth(1);
        linePaint.setAntiAlias(true);
        PathEffect effects = new DashPathEffect(
                new float[]{50, 10}, 1);
        linePaint.setPathEffect(effects);

        canvas.drawRect(mDrawLeftLineRect, linePaint);
        canvas.drawRect(mDrawRightLineRect, linePaint);
        canvas.drawRect(mDrawTopLineRect, linePaint);
        canvas.drawRect(mDrawBottomLineRect, linePaint);
    }
    private void drawCorner(Canvas canvas) {
        Paint linePaint = new Paint();
        linePaint.setColor(Color.WHITE);
        linePaint.setStrokeWidth(5);
        linePaint.setAntiAlias(true);
        /*PathEffect effects = new DashPathEffect(
                new float[]{50, 10}, 1);
        linePaint.setPathEffect(effects);*/

        canvas.drawCircle(mDrawRect.left, mDrawRect.top, 15, linePaint);
        canvas.drawCircle(mDrawRect.right, mDrawRect.top, 15, linePaint);
        canvas.drawCircle(mDrawRect.left, mDrawRect.bottom, 15, linePaint);
        canvas.drawCircle(mDrawRect.right, mDrawRect.bottom, 15, linePaint);
    }

    enum ModifyMode { None, Move, Grow }

    private ModifyMode mMode = ModifyMode.None;

    Rect mDrawRect;  // in screen space
    private RectF mImageRect;  // in image space
    RectF mCropRect;  // in image space
    Matrix mMatrix;
    private Rect mDrawLeftLineRect;
    private Rect mDrawRightLineRect;
    private Rect mDrawTopLineRect;
    private Rect mDrawBottomLineRect;
    // CID 109263 : UuF: Unused field (FB.UUF_UNUSED_FIELD)
    // private int mViewWidth;
    // private int mViewHeight;

    private boolean mMaintainAspectRatio = false;
    private float mInitialAspectRatio;
    private boolean mCircle = false;

    private Drawable mResizeDrawableWidth;
    private Drawable mResizeDrawableHeight;
    private Drawable mResizeDrawableDiagonal;

    private final Paint mFocusPaint = new Paint();
    private final Paint mNoFocusPaint = new Paint();
    private final Paint mOutlinePaint = new Paint();
}
