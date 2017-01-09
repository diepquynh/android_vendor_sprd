/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.sprdlauncher2;


import com.android.sprdlauncher2.DeleteDropTarget.EndState;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.view.animation.AccelerateInterpolator;
import android.widget.FrameLayout;

import android.widget.TextView;
import android.animation.AnimatorSet;
import android.util.Log;
import android.view.animation.DecelerateInterpolator;

/*
 * Ths bar will manage the transition between the QSB search bar and the delete drop
 * targets so that each of the individual IconDropTargets don't have to.
 */
public abstract class SearchDropTargetBar extends FrameLayout implements DragController.DragListener {

    protected static final int sTransitionInDuration = 200;
    protected static final int sTransitionOutDuration = 175;

    private ObjectAnimator mDropTargetBarAnim;
    protected ObjectAnimator mQSBSearchBarAnim;
    private static final AccelerateInterpolator sAccelerateInterpolator =
            new AccelerateInterpolator();

    protected boolean mIsSearchBarHidden;
    protected View mQSBSearchBar;
    protected View mDropTargetBar;
    private ButtonDropTarget mInfoDropTarget;
    protected ButtonDropTarget mDeleteDropTarget;
    protected int mBarHeight;
    private boolean mDeferOnDragEnd = false;

    private Drawable mPreviousBackground;
    protected boolean mEnableDropDownDropTargets;
    // SPRD: fix bug272607
    protected Runnable mDelayRunnable;
    // SPRD: fix bug274649
    private Launcher mLauncher;
    // SPRD: Feature 253522, Remove the application drawer view
    protected TextView mSysAppIndicator;
    // SPRD: Feature 253522, Remove the application drawer view
    protected AnimatorSet mDropTargetBarFadeOutAnim;
    /* SPRD: Feature 255608,show delete area @{ */
    protected ObjectAnimator mIndicatorBarFadeInAnim;
    protected ObjectAnimator mIndicatorBarFadeOutAnim;
    /* @} */
    public abstract void showIndicator();
    public abstract void hideIndicator();
    public abstract void cancelAnimations();

    public SearchDropTargetBar(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SearchDropTargetBar(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public void setup(Launcher launcher, DragController dragController) {
        dragController.addDragListener(this);
        dragController.addDragListener(mInfoDropTarget);
        dragController.addDragListener(mDeleteDropTarget);
        dragController.addDropTarget(mInfoDropTarget);
        dragController.addDropTarget(mDeleteDropTarget);
        dragController.setFlingToDeleteDropTarget(mDeleteDropTarget);
        mInfoDropTarget.setLauncher(launcher);
        mDeleteDropTarget.setLauncher(launcher);
        mQSBSearchBar = launcher.getQsbBar();
        if (mEnableDropDownDropTargets) {
            mQSBSearchBarAnim = LauncherAnimUtils.ofFloat(mQSBSearchBar, "translationY", 0,
                    -mBarHeight);
        } else {
            mQSBSearchBarAnim = LauncherAnimUtils.ofFloat(mQSBSearchBar, "alpha", 1f, 0f);
        }
        setupAnimation(mQSBSearchBarAnim, mQSBSearchBar);
    }

    protected void prepareStartAnimation(View v) {
        // Enable the hw layers before the animation starts (will be disabled in the onAnimationEnd
        // callback below)
        v.setLayerType(View.LAYER_TYPE_HARDWARE, null);
    }

    private void setupAnimation(ObjectAnimator anim, final View v) {
        anim.setInterpolator(sAccelerateInterpolator);
        anim.setDuration(sTransitionInDuration);
        anim.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                v.setLayerType(View.LAYER_TYPE_NONE, null);
            }
        });
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        // Get the individual components
        mDropTargetBar = findViewById(R.id.drag_target_bar);
        mInfoDropTarget = (ButtonDropTarget) mDropTargetBar.findViewById(R.id.info_target_text);
        mDeleteDropTarget = (ButtonDropTarget) mDropTargetBar.findViewById(R.id.delete_target_text);

        mInfoDropTarget.setSearchDropTargetBar(this);
        mDeleteDropTarget.setSearchDropTargetBar(this);

        mEnableDropDownDropTargets =
            getResources().getBoolean(R.bool.config_useDropTargetDownTransition);

        // Create the various fade animations
        if (mEnableDropDownDropTargets) {
            LauncherAppState app = LauncherAppState.getInstance();
            DeviceProfile grid = app.getDynamicGrid().getDeviceProfile();
            mBarHeight = grid.searchBarSpaceHeightPx;
            mDropTargetBar.setTranslationY(-mBarHeight);
            mDropTargetBarAnim = LauncherAnimUtils.ofFloat(mDropTargetBar, "translationY",
                    -mBarHeight, 0f);

        } else {
            mDropTargetBar.setAlpha(0f);
            mDropTargetBarAnim = LauncherAnimUtils.ofFloat(mDropTargetBar, "alpha", 0f, 1f);
        }
        setupAnimation(mDropTargetBarAnim, mDropTargetBar);

    }

    public void finishAnimations() {
        prepareStartAnimation(mDropTargetBar);
        mDropTargetBarAnim.reverse();
        prepareStartAnimation(mQSBSearchBar);
        mQSBSearchBarAnim.reverse();
    }

    /*
     * Shows and hides the search bar.
     */
    public void showSearchBar(boolean animated) {
        boolean needToCancelOngoingAnimation = mQSBSearchBarAnim.isRunning() && !animated;
        /* SPRD: fix bug 263871,260431,home screen display abnormal
        if (!mIsSearchBarHidden && !needToCancelOngoingAnimation) return;*/
        if (!needToCancelOngoingAnimation) return;
        /* @} */
        if (animated) {
            cancelAnimations();
            prepareStartAnimation(mQSBSearchBar);
            mQSBSearchBarAnim.reverse();
            /* SPRD: Feature 255608,show delete area @{ */
            mIndicatorBarFadeInAnim.start();
            /* @} */
        } else {
            mQSBSearchBarAnim.cancel();
            if (mEnableDropDownDropTargets) {
                mQSBSearchBar.setTranslationY(0);
            } else {
                mQSBSearchBar.setAlpha(1f);
            }
        }
        mIsSearchBarHidden = false;
    }
    public void hideSearchBar(boolean animated) {
        // SPRD: Feature 253522, Remove the application drawer view
        mDropTargetBarFadeOutAnim.cancel();
        boolean needToCancelOngoingAnimation = mQSBSearchBarAnim.isRunning() && !animated;
        if (mIsSearchBarHidden && !needToCancelOngoingAnimation) return;
        if (animated) {
            prepareStartAnimation(mQSBSearchBar);
            mQSBSearchBarAnim.start();
            /* SPRD: Feature 255608,show delete area @{ */
            mIndicatorBarFadeOutAnim.start();
            /* @} */
        } else {
            mQSBSearchBarAnim.cancel();
            if (mEnableDropDownDropTargets) {
                mQSBSearchBar.setTranslationY(-mBarHeight);
            } else {
                mQSBSearchBar.setAlpha(0f);
            }
        }
        mIsSearchBarHidden = true;
    }

    /*
     * Gets various transition durations.
     */
    public int getTransitionInDuration() {
        return sTransitionInDuration;
    }
    public int getTransitionOutDuration() {
        return sTransitionOutDuration;
    }

    /*
     * DragController.DragListener implementation
     */
    @Override
    public void onDragStart(DragSource source, Object info, int dragAction) {
        // Animate out the QSB search bar, and animate in the drop target bar
        prepareStartAnimation(mDropTargetBar);
        mDropTargetBarAnim.start();
        /* SPRD: add for bug272607 @{
       if (!mIsSearchBarHidden) {
            prepareStartAnimation(mQSBSearchBar);
            mQSBSearchBarAnim.start();
        @} */
    }

    public void deferOnDragEnd() {
        mDeferOnDragEnd = true;
    }

    @Override
    public void onDragEnd() {
        if (!mDeferOnDragEnd) {
            // Restore the QSB search bar, and animate out the drop target bar
            prepareStartAnimation(mDropTargetBar);
            /* SPRD: Feature 253522, Remove the application drawer view @{
             mDropTargetBarAnim.reverse();
            if (!mIsSearchBarHidden) {
                prepareStartAnimation(mQSBSearchBar);
                mQSBSearchBarAnim.reverse();
            }*/
            onDragRemoveDrawView();
            /* @} */
        } else {
            mDeferOnDragEnd = false;
        }
    }

    public void onSearchPackagesChanged(boolean searchVisible, boolean voiceVisible) {
        if (mQSBSearchBar != null) {
            Drawable bg = mQSBSearchBar.getBackground();
            if (bg != null && (!searchVisible && !voiceVisible)) {
                // Save the background and disable it
                mPreviousBackground = bg;
                mQSBSearchBar.setBackgroundResource(0);
            } else if (mPreviousBackground != null && (searchVisible || voiceVisible)) {
                // Restore the background
                mQSBSearchBar.setBackground(mPreviousBackground);
            }
        }
    }

    public Rect getSearchBarBounds() {
        if (mQSBSearchBar != null) {
            final int[] pos = new int[2];
            mQSBSearchBar.getLocationOnScreen(pos);

            final Rect rect = new Rect();
            rect.left = pos[0];
            rect.top = pos[1];
            rect.right = pos[0] + mQSBSearchBar.getWidth();
            rect.bottom = pos[1] + mQSBSearchBar.getHeight();
            return rect;
        } else {
            return null;
        }
    }

    /* SPRD: Feature 253522, Remove the application drawer view @{
     *
     */
    private void onDragRemoveDrawView(){
        if (((DeleteDropTarget) mDeleteDropTarget).mState == EndState.SYSTEM_APP
                || ((DeleteDropTarget) mDeleteDropTarget).mState == EndState.NOT_EMPTY_FOLDER) {
            mSysAppIndicator
                    .setText(((DeleteDropTarget) mDeleteDropTarget).mState == EndState.SYSTEM_APP ? R.string.uninstall_system_app_text
                            : R.string.folder_not_empty);
            mSysAppIndicator.setVisibility(View.VISIBLE);
            mDeleteDropTarget.setVisibility(View.INVISIBLE);
            mDelayRunnable = new Runnable() {

                public void run() {
                    mDropTargetBarFadeOutAnim.start();
                }
            };
            postDelayed(mDelayRunnable, 1000);
            ((DeleteDropTarget) mDeleteDropTarget).mState = EndState.NORMAL;
        } else {
            mDropTargetBarFadeOutAnim.start();
        }
    }
    /* @} */
}
