/*
 * Copyright (C) 2013 The Android Open Source Project
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
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.view.animation.AnimationUtils;
import android.widget.FrameLayout;
import android.widget.TextView;
import com.android.camera.debug.Log;
import com.android.camera.ui.Rotatable;
import com.android.camera.util.ApiHelper;
import com.android.camera2.R;

/**
 * This is a package private class, as it is not intended to be visible or used
 * outside of this package. ModeSelectorItem is a FrameLayout that contains an
 * ImageView to display the icon for the corresponding mode, a TextView that
 * explains what the mode is, and a GradientDrawable at the end of the TextView.
 * The purpose of this class is to encapsulate different drawing logic into its
 * own class. There are two drawing mode, <code>FLY_IN</code> and
 * <code>FLY_OUT</code>. They define how we draw the view when we display the
 * view partially.
 */
class ModeSelectorItem extends FrameLayout implements Rotatable {
    private TextView mText;
    private ModeIconView mIcon;
    private int mVisibleWidth = 0;
    private int mMinVisibleWidth;
    private VisibleWidthChangedListener mListener = null;

    private int mWidth;
    private int mModeId;

    private static final Log.Tag TAG = new Log.Tag("ModeSelectorItem");

    /**
     * A listener that gets notified when the visible width of the current item
     * is changed.
     */
    public interface VisibleWidthChangedListener {
        public void onVisibleWidthChanged(int width);
    }

    public ModeSelectorItem(Context context, AttributeSet attrs) {
        super(context, attrs);
        setWillNotDraw(false);
        setClickable(true);
        // mMinVisibleWidth = getResources().getDimensionPixelSize(
        // R.dimen.mode_selector_icon_block_width);
        WindowManager wm = (WindowManager) getContext().getSystemService(
                Context.WINDOW_SERVICE);

        int width = wm.getDefaultDisplay().getWidth()
                - getResources().getDimensionPixelSize(
                        R.dimen.mode_selector_margin_left_right) * 2;

        mMinVisibleWidth = width / 3;
    }

    @Override
    public void onFinishInflate() {
        mIcon = (ModeIconView) findViewById(R.id.selector_icon);
        mText = (TextView) findViewById(R.id.selector_text);
        Typeface typeface;
        if (ApiHelper.HAS_ROBOTO_MEDIUM_FONT) {
            typeface = Typeface.create("sans-serif-medium", Typeface.NORMAL);
        } else {
            // Load roboto_light typeface from assets.
            typeface = Typeface.createFromAsset(getResources().getAssets(),
                    "Roboto-Medium.ttf");
        }
        mText.setTypeface(typeface);
    }

    public void setDefaultBackgroundColor(int color) {
        setBackgroundColor(color);
    }

    /**
     * Sets a listener that receives a callback when the visible width of this
     * selector item changes.
     */
    public void setVisibleWidthChangedListener(
            VisibleWidthChangedListener listener) {
        mListener = listener;
    }

    @Override
    public void setSelected(boolean selected) {
        mIcon.setSelected(selected);
        mIcon.setIconDrawable(selected ? mIcon.getIconSelectedDrawable()
                : mIcon.getIconUnSelectedDrawable());
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        // Do not dispatch any touch event, so that all the events that are
        // received
        // in onTouchEvent() are only through forwarding.
        return false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        super.onTouchEvent(ev);
        return false;
    }

    /**
     * When swiping in, we truncate the end of the item if the visible width is
     * not enough to show the whole item. When swiping out, we truncate the
     * front of the text (i.e. offset the text).
     * 
     * @param swipeIn
     *            whether swiping direction is swiping in (i.e. from left to
     *            right)
     */
    public void onSwipeModeChanged(boolean swipeIn) {
        mText.setTranslationX(0);
    }

    public void setText(CharSequence text) {
        mText.setText(text);
    }

    @Override
    public void onLayout(boolean changed, int left, int top, int right,
            int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        mWidth = right - left;
        if (changed && mVisibleWidth > 0) {
            // Reset mode list to full screen
            setVisibleWidth(mWidth);
        }
    }

    /**
     * Sets image resource as the icon for the mode. By default, all drawables
     * instances loaded from the same resource share a common state; if you
     * modify the state of one instance, all the other instances will receive
     * the same modification. In order to modify properties of this icon
     * drawable without affecting other drawables, here we use a mutable
     * drawable which is guaranteed to not share states with other drawables.
     * 
     * @param resource
     *            resource id of the asset to be used as icon
     */
    public void setImageResource(int resource) {
        Drawable drawableIcon = getResources().getDrawable(resource);
        if (drawableIcon != null) {
            drawableIcon = drawableIcon.mutate();
        }
        mIcon.setIconDrawable(drawableIcon);
    }

    /**
     * Sets the visible width preferred for the item. The item can then decide
     * how to draw itself based on the visible width and whether it's being
     * swiped in or out. This function will be called on every frame during
     * animation. It should only do minimal work required to get the animation
     * working.
     * 
     * @param newWidth
     *            new visible width
     */
    public void setVisibleWidth(int newWidth) {
        int fullyShownIconWidth = getMaxVisibleWidth();
        newWidth = Math.max(newWidth, 0);
        // Visible width should not be greater than view width
        newWidth = Math.min(newWidth, fullyShownIconWidth);

        if (mVisibleWidth != newWidth) {
            mVisibleWidth = newWidth;
            if (mListener != null) {
                mListener.onVisibleWidthChanged(newWidth);
            }
        }
        invalidate();
    }

    /**
     * Getter for visible width. This function will get called during animation
     * as well.
     * 
     * @return The visible width of this item
     */
    public int getVisibleWidth() {
        return mVisibleWidth;
    }

    /**
     * Draw the view based on the drawing mode. Clip the canvas if necessary.
     * 
     * @param canvas
     *            The Canvas to which the View is rendered.
     */
    @Override
    public void draw(Canvas canvas) {
        float transX = 0f;
        // If the given width is less than the icon width, we need to translate
        // icon
        if (mVisibleWidth < mMinVisibleWidth + mIcon.getLeft()) {
            transX = mMinVisibleWidth - mVisibleWidth;
        }
        canvas.save();
        // canvas.translate(-transX, 0);
        super.draw(canvas);
        canvas.restore();
    }

    /**
     * Sets the color that will be used in the drawable for highlight state.
     * 
     * @param highlightColor
     *            color for the highlight state
     */
    public void setHighlightColor(int highlightColor) {
        mIcon.setHighlightColor(highlightColor);
    }

    /**
     * @return highlightColor color for the highlight state
     */
    public int getHighlightColor() {
        return mIcon.getHighlightColor();
    }

    /**
     * Gets the maximum visible width of the mode icon. The mode item will be
     * full shown when the mode icon has max visible width.
     */
    public int getMaxVisibleWidth() {
        return mMinVisibleWidth;
    }

    /**
     * Gets the position of the icon center relative to the window.
     * 
     * @param loc
     *            integer array of size 2, to hold the position x and y
     */
    public void getIconCenterLocationInWindow(int[] loc) {
        mIcon.getLocationInWindow(loc);
        int iconCenterX = loc[0]
                + getResources().getDimensionPixelSize(
                        R.dimen.mode_selector_icon_block_width) / 2;
        int iconCenterY = loc[1]
                + getResources().getDimensionPixelSize(
                        R.dimen.mode_selector_icon_block_width) / 2;

        int[] itemLoc = new int[2];
        getLocationInWindow(itemLoc);
        int itemCenterX = itemLoc[0] + getWidth() / 2;
        int itemCenterY = itemLoc[1] + getHeight() / 2;

        int distance = itemCenterY - iconCenterY;

        if (mCurrentDegree == 0) {
            loc[0] = iconCenterX;
            loc[1] = iconCenterY;
        }

        if (mCurrentDegree == 90) {
            loc[0] = itemCenterX - distance;
            loc[1] = itemCenterY;
        }
        if (mCurrentDegree == 180) {
            loc[0] = itemCenterX;
            loc[1] = itemCenterY + distance;
        }
        if (mCurrentDegree == 270) {
            loc[0] = itemCenterX + distance;
            loc[1] = itemCenterY;
        }

    }

    /**
     * Sets the mode id of the current item.
     * 
     * @param modeId
     *            id of the mode represented by current item.
     */
    public void setModeId(int modeId) {
        mModeId = modeId;
    }

    /**
     * Gets the mode id of the current item.
     */
    public int getModeId() {
        return mModeId;
    }

    /**
     * @return The {@link ModeIconView} attached to this item.
     */
    public ModeIconView getIcon() {
        return mIcon;
    }

    /**
     * Sets the alpha on the mode text.
     */
    public void setTextAlpha(float alpha) {
        mText.setAlpha(alpha);
    }

    // mode and camera
    private int[] mc = { 0, 0 };

    public void setMC(int[] mc) {
        this.mc = mc;
    }

    public boolean isSupportMC(int mode, int camera) {
        if (mc[0] != -1 && mc[0] != mode)
            return false;
        if (mc[1] != -1 && mc[1] != camera)
            return false;
        return true;
    }

    /**
     * when the mode is be selected,set the icon drawable here just save the
     * icon's selected drawable. copy fron set ImageResouce
     * 
     * @param resource
     *            resource id of the asset to be used as icon
     */
    public void setSelectedImageResource(int resource) {
        Drawable drawableIcon = getResources().getDrawable(resource);
        if (drawableIcon != null) {
            drawableIcon = drawableIcon.mutate();
        }
        mIcon.setIconSelectedDrawable(drawableIcon);
    }

    /**
     * when the mode is be selected,set the icon drawable here just save the
     * icon's selected drawable. copy fron set ImageResouce
     * 
     * @param resource
     *            resource id of the asset to be used as icon
     */
    public void setUnSelectedImageResource(int resource) {
        Drawable drawableIcon = getResources().getDrawable(resource);
        if (drawableIcon != null) {
            drawableIcon = drawableIcon.mutate();
        }
        mIcon.setIconUnSelectedDrawable(drawableIcon);
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
        Log.e(TAG, " setOrientation mCurrentDegree = " + degree);
        mEnableAnimation = animation;
        // make sure in the range of [0, 359]
        degree = degree >= 0 ? degree % 360 : degree % 360 + 360;
        if (degree == mTargetDegree)
            return;
        setClickable(false);
        mTargetDegree = degree;
        if (mEnableAnimation) {
            mStartDegree = mCurrentDegree;
            mAnimationStartTime = AnimationUtils.currentAnimationTimeMillis();

            int diff = mTargetDegree - mCurrentDegree;
            diff = diff >= 0 ? diff : 360 + diff; // make it in range [0, 359]

            // Make it in range [-179, 180]. That's the shorted distance between
            // the
            // two angles 163
            diff = diff > 180 ? diff - 360 : diff;

            mClockwise = diff >= 0;
            mAnimationEndTime = mAnimationStartTime + Math.abs(diff) * 1000
                    / ANIMATION_SPEED;
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

        if (w == 0 || h == 0){
            return; // nothing to draw
        }

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
                setClickable(true);
                Log.e(TAG, " mCurrentDegree = " + mCurrentDegree);
            }
        }else {
            setClickable(true);
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
