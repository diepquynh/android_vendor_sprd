/** Created by Spreadst */

package com.android.sprdlauncher2;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.widget.TextView;

public class SprdSearchDropTargetBar extends SearchDropTargetBar {

    private TextView mIndicator;
    private AnimatorSet mDropTargetBarFadeInAnim;
    private boolean mHideIndiTemp = false;
    // SPRD: fix bug274649
    private Launcher mLauncher;

    public SprdSearchDropTargetBar(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public SprdSearchDropTargetBar(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
        // TODO Auto-generated constructor stub
    }

    public void setup(Launcher launcher, DragController dragController) {
        mLauncher = launcher;
        super.setup(launcher, dragController);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        // SPRD: Feature 253522, Remove the application drawer view
        mSysAppIndicator = (TextView) findViewById(R.id.indicator_tx2);
        /* SPRD: Feature 255608,show delete area @{ */
        mIndicator = (TextView) findViewById(R.id.indicator_tx);
        /* @} */
        /* SPRD: Feature 255608,show delete area @{ */
        ObjectAnimator fadeInAlphaAnim = ObjectAnimator.ofFloat(mDropTargetBar,
                "alpha", 1f);
        /* SPRD: Fix bug277786 @{ */
        if (fadeInAlphaAnim.getDurationScale() == 0.0f) {
            fadeInAlphaAnim.setDurationScale(1.0f);
            fadeInAlphaAnim.setDuration(sTransitionInDuration);
        }
        /* @} */
        fadeInAlphaAnim.setInterpolator(new DecelerateInterpolator());
        mDropTargetBarFadeInAnim = new AnimatorSet();
        AnimatorSet.Builder fadeInAnimators = mDropTargetBarFadeInAnim
                .play(fadeInAlphaAnim);
        if (mEnableDropDownDropTargets) {
            mDropTargetBar.setTranslationY(-mBarHeight);
            fadeInAnimators.with(ObjectAnimator.ofFloat(mDropTargetBar,
                    "translationY", 0f));
            if (mIndicator.getVisibility() == View.VISIBLE) {
                fadeInAnimators.with(ObjectAnimator.ofFloat(mIndicator,
                        "translationY", -mBarHeight));
                fadeInAnimators.with(ObjectAnimator.ofFloat(mIndicator,
                        "alpha", 0f));
            }
        }

        /* SPRD: Fix bug277786 @{ */
        // mDropTargetBarFadeInAnim.setDuration(sTransitionInDuration);
        /* @} */
        mDropTargetBarFadeInAnim.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                mDropTargetBar.setVisibility(View.VISIBLE);
                /*
                 * SPRD:fix bug 281617, The mIndicator is gone in the edit mode
                 *
                 * @{
                 */
                if (mIndicator.getVisibility() == View.VISIBLE) {
                    mIndicator.setVisibility(View.GONE);
                    mHideIndiTemp = true;
                }
                /* @} */
            }

            @Override
            public void onAnimationEnd(Animator animation) {
                /*
                 * SPRD:fix bug 281617, The mIndicator is gone in the edit mode
                 *
                 * @{ if (mIndicator.getVisibility() == View.VISIBLE) {
                 * mIndicator.setVisibility(View.GONE); mHideIndiTemp = true; }
                 *
                 * @}
                 */
            }
        });
        /* @} */
        /* SPRD: Feature 253522, Remove the application drawer view @{ */
        ObjectAnimator fadeOutAlphaAnim = ObjectAnimator.ofFloat(
                mDropTargetBar, "alpha", 0f);
        fadeOutAlphaAnim.setInterpolator(new AccelerateInterpolator());
        /* SPRD: Fix bug277786 @{ */
        if (fadeOutAlphaAnim.getDurationScale() == 0.0f) {
            fadeOutAlphaAnim.setDurationScale(1.0f);
            fadeOutAlphaAnim.setDuration(sTransitionOutDuration);
        }
        /* @} */
        mDropTargetBarFadeOutAnim = new AnimatorSet();
        AnimatorSet.Builder fadeOutAnimators = mDropTargetBarFadeOutAnim
                .play(fadeOutAlphaAnim);
        if (mEnableDropDownDropTargets) {
            fadeOutAnimators.with(ObjectAnimator.ofFloat(mDropTargetBar,
                    "translationY", -mBarHeight));
            if (mHideIndiTemp) {
                mIndicator.setTranslationY(-mBarHeight);
                fadeOutAnimators.with(ObjectAnimator.ofFloat(mIndicator,
                        "translationY", 0f));
                fadeOutAnimators.with(ObjectAnimator.ofFloat(mIndicator,
                        "alpha", 1f));
            }
        }

        /* SPRD: Fix bug277786 @{ */
        // mDropTargetBarFadeOutAnim.setDuration(sTransitionOutDuration);
        /* @} */
        mDropTargetBarFadeOutAnim.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                // mDropTargetBar.setVisibility(View.INVISIBLE);
                // mDropTargetBar.setLayerType(View.LAYER_TYPE_NONE, null);
                mDeleteDropTarget.setVisibility(View.VISIBLE);
                mSysAppIndicator.setVisibility(View.GONE);
                if (!mIsSearchBarHidden) {
                    prepareStartAnimation(mQSBSearchBar);
                    mQSBSearchBarAnim.reverse();
                }
            }

            @Override
            public void onAnimationStart(Animator animation) {
                /*
                 * SPRD: fix bug 284381,The mIndicator is visible when exit edit
                 * mode @{
                 */
                if (!mLauncher.isEditingMode()) {
                    mIndicator.setVisibility(View.GONE);
                    mHideIndiTemp = true;
                } else if (mHideIndiTemp) {
                    mIndicator.setVisibility(View.VISIBLE);
                    mHideIndiTemp = false;
                }
                /* @} */

            }
        });
        /* @} */
        /* SPRD: Feature 255608,show delete area @{ */
        mIndicator.setTranslationY(-mBarHeight);
        mIndicatorBarFadeInAnim = ObjectAnimator.ofFloat(mIndicator,
                "translationY", 0f);
        mIndicatorBarFadeInAnim.setDuration(sTransitionInDuration);
        mIndicatorBarFadeInAnim.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                mIndicator.setVisibility(View.VISIBLE);
            }
        });
        mIndicatorBarFadeOutAnim = ObjectAnimator.ofFloat(mIndicator,
                "translationY", -mBarHeight);
        mIndicatorBarFadeOutAnim.setDuration(sTransitionOutDuration);
        mIndicatorBarFadeOutAnim.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                mIndicator.setVisibility(View.INVISIBLE);
            }
        });
        /* @} */
    }

    public void finishAnimations() {
        super.finishAnimations();
        /* SPRD: Feature 255608,show delete area @{ */
        mIndicatorBarFadeInAnim.cancel();
        mIndicatorBarFadeOutAnim.cancel();
        /* @} */
    }

    @Override
    public void onDragStart(DragSource source, Object info, int dragAction) {
        mLauncher.setUninstall(true);// SPRD: fix bug274649
        mDropTargetBarFadeOutAnim.cancel();
        super.onDragStart(source, info, dragAction);
        mDropTargetBarFadeOutAnim.cancel();
        /* SPRD: add for bug272607 @{ */
        getHandler().removeCallbacks(mDelayRunnable);
        mDeleteDropTarget.setVisibility(View.VISIBLE);
        mSysAppIndicator.setVisibility(View.GONE);
        /* @} */
        mDropTargetBarFadeInAnim.start();
    }

    @Override
    public void onDragEnd() {
        super.onDragEnd();
        /* SPRD: fix bug274649 @{ */
        mLauncher.setUninstall(false);
    }

    /* SPRD: Feature 255608,show delete area @{ */
    public void showIndicator() {
        mIndicatorBarFadeOutAnim.cancel();
        mIndicatorBarFadeInAnim.start();
    }

    public void hideIndicator() {
        cancelAnimations();
        mIndicatorBarFadeOutAnim.start();
    }

    public void cancelAnimations() {
        mDropTargetBarFadeInAnim.cancel();
        mDropTargetBarFadeOutAnim.cancel();
        mIndicatorBarFadeInAnim.cancel();
        mIndicatorBarFadeOutAnim.cancel();
    }
    /* @} */
}