/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.camera.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.View;
import android.view.animation.AnimationUtils;

import com.android.camera2.R;

/**
 * This class encapsulates the logic of drawing different states of the icon in
 * mode drawer for when it is highlighted (to indicate the current module), or when
 * it is selected by the user. It handles the internal state change like a state
 * list drawable. The advantage over a state list drawable is that in the class
 * multiple states can be rendered using the same drawable with some color modification,
 * whereas a state list drawable would require a different drawable for each state.
 */
/*
 * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
 * Original Android code:

public class ModeIconView extends View {

 */
public class ModeIconView extends View implements Rotatable {
    private final GradientDrawable mBackground;

    private final int mIconBackgroundSize;
    private int mHighlightColor;
    private final int mBackgroundDefaultColor;
    private final int mIconDrawableSize;
    private Drawable mIconDrawable = null;

    public ModeIconView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mBackgroundDefaultColor = getResources().getColor(R.color.mode_selector_icon_background);
        mIconBackgroundSize = getResources().getDimensionPixelSize(
                R.dimen.mode_selector_icon_block_width);
        mBackground = (GradientDrawable) getResources()
                .getDrawable(R.drawable.mode_icon_background).mutate();
        mBackground.setBounds(0, 0, mIconBackgroundSize, mIconBackgroundSize);
        mIconDrawableSize = getResources().getDimensionPixelSize(
                R.dimen.mode_selector_icon_drawable_size);
    }

    /**
     * Sets the drawable that shows the icon of the mode.
     *
     * @param drawable drawable of the mode icon
     */
    public void setIconDrawable(Drawable drawable) {
        mIconDrawable = drawable;

        // Center icon in the background.
        if (mIconDrawable != null) {
            mIconDrawable.setBounds(mIconBackgroundSize / 2 - mIconDrawableSize / 2,
                    mIconBackgroundSize / 2 - mIconDrawableSize / 2,
                    mIconBackgroundSize / 2 + mIconDrawableSize / 2,
                    mIconBackgroundSize / 2 + mIconDrawableSize / 2);
            invalidate();
        }
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);
        mBackground.draw(canvas);
        if (mIconDrawable != null) {
            mIconDrawable.draw(canvas);
        }
    }

    /**
     * @return A clone of the icon drawable associated with this view.
     */
    public Drawable getIconDrawableClone() {
        return mIconDrawable.getConstantState().newDrawable();
    }

    /**
     * @return The size of the icon drawable.
     */
    public int getIconDrawableSize() {
        return mIconDrawableSize;
    }

    /**
     * This gets called when the selected state is changed. When selected, the background
     * drawable will use a solid pre-defined color to indicate selection.
     *
     * @param selected true when selected, false otherwise.
     */
    @Override
    public void setSelected(boolean selected) {
        if (selected) {
            mBackground.setColor(mHighlightColor);
        } else {
            mBackground.setColor(mBackgroundDefaultColor);
        }

        invalidate();
    }

    /**
     * Sets the color that will be used in the drawable for highlight state.
     *
     * @param highlightColor color for the highlight state
     */
    public void setHighlightColor(int highlightColor) {
        mHighlightColor = highlightColor;
    }

    /**
     * @return The highlightColor color the the highlight state.
     */
    public int getHighlightColor() {
        return mHighlightColor;
    }

    /*
     * SPRD Bug:519334 Refactor Rotation UI of Camera. @{
     */
    private static final int ANIMATION_SPEED = 270; // deg/sec 134

    private int mCurrentDegree = 0; // [0, 359]
    private int mStartDegree = 0;
    private int mTargetDegree = 0;

    private boolean mClockwise = false, mEnableAnimation = true;

    private long mAnimationStartTime = 0;
    private long mAnimationEndTime = 0;

    // Rotate the view counter-clockwise
    @Override
    public void setOrientation(int degree, boolean animation) {
        mEnableAnimation = animation;
        // make sure in the range of [0, 359]
        degree = degree >= 0 ? degree % 360 : degree % 360 + 360;
        if (degree == mTargetDegree)
            return;

        mTargetDegree = degree;
        if (mEnableAnimation) {
            mStartDegree = mCurrentDegree;
            mAnimationStartTime = AnimationUtils.currentAnimationTimeMillis();

            int diff = mTargetDegree - mCurrentDegree;
            diff = diff >= 0 ? diff : 360 + diff; // make it in range [0, 359]

            // Make it in range [-179, 180]. That's the shorted distance between the
            // two angles 163
            diff = diff > 180 ? diff - 360 : diff;

            mClockwise = diff >= 0;
            mAnimationEndTime = mAnimationStartTime
                    + Math.abs(diff) * 1000 / ANIMATION_SPEED;
        } else {
            mCurrentDegree = mTargetDegree;
        }

        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        Rect bounds = new Rect();
        getBoundsOnScreen(bounds);
        int w = bounds.right - bounds.left;
        int h = bounds.bottom - bounds.top;

        if (w == 0 || h == 0)
            return; // nothing to draw

        if (mCurrentDegree != mTargetDegree) {
            long time = AnimationUtils.currentAnimationTimeMillis();
            if (time < mAnimationEndTime) {
                int deltaTime = (int) (time - mAnimationStartTime);
                int degree = mStartDegree + ANIMATION_SPEED
                        * (mClockwise ? deltaTime : -deltaTime) / 1000;
                degree = degree >= 0 ? degree % 360 : degree % 360 + 360;
                mCurrentDegree = degree;
                invalidate();
            } else {
                mCurrentDegree = mTargetDegree;
            }
        }

        int left = getPaddingLeft();
        int top = getPaddingTop();
        int right = getPaddingRight();
        int bottom = getPaddingBottom();
        int width = getWidth() - left - right;
        int height = getHeight() - top - bottom;

        int saveCount = canvas.getSaveCount();

        canvas.translate(left + width / 2, top + height / 2);
        canvas.rotate(-mCurrentDegree);
        canvas.translate(-w / 2, -h / 2);
        canvas.restoreToCount(saveCount);
    }
    /* @} */
}
