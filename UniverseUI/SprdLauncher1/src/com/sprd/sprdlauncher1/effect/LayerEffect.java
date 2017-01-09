/** Created by Spreadst */
package com.sprd.launcher3.effect;

import android.animation.TimeInterpolator;
import android.content.Context;
import android.graphics.Camera;
import android.graphics.Matrix;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class LayerEffect extends EffectInfo {

    public static class ZInterpolator implements TimeInterpolator {
        private float focalLength;

        public ZInterpolator(float foc) {
            focalLength = foc;
        }

        public float getInterpolation(float input) {
            return (1.0f - focalLength / (focalLength + input))
                    / (1.0f - focalLength / (focalLength + 1.0f));
        }
    }

    private static float TRANSITION_SCALE_FACTOR = 0.74f;
    private ZInterpolator mZInterpolator = new ZInterpolator(0.5f);
    private AccelerateInterpolator mAlphaInterpolator = new AccelerateInterpolator(
            0.9f);
    private DecelerateInterpolator mLeftScreenAlphaInterpolator = new DecelerateInterpolator(
            4);
    private static float TRANSITION_PIVOT = 0.65f;
    private static float TRANSITION_MAX_ROTATION = 22;

    public LayerEffect(int id) {
        super(id);
    }

    @Override
    public boolean getCellLayoutChildStaticTransformation(ViewGroup viewGroup,
            View viewiew, Transformation transformation, Camera camera,
            float offset) {
        return false;
    }

    @Override
    public boolean getWorkspaceChildStaticTransformation(ViewGroup viewGroup,
            View viewiew, Transformation transformation, Camera camera,
            float offset) {
        Matrix tMatrix = transformation.getMatrix();
        float mViewWidth = viewiew.getMeasuredWidth();
        float mViewHeight = viewiew.getMeasuredHeight();
        if (offset == 0.00F || offset >= 0.99F || offset <= -0.99F)
            return false;
        transformation.setAlpha(1.0F - offset);
        // viewiew.setAlpha(1.0F - offset);
        float level = 0.4F * (1.0F - offset);
        float scale = 0.6F + level;
        tMatrix.setScale(scale, scale);
        float xPost = 0.4F * offset * mViewWidth * 3.0F;
        float yPost = 0.4F * offset * mViewHeight * 0.5F;
        tMatrix.postTranslate(xPost, yPost);
        transformation.setTransformationType(Transformation.TYPE_BOTH);
        return true;
    }

    /* SPRD: Fix bug257614 @{ */
    @Override
    public void getTransformationMatrix(View v, float offset, int pageWidth,
            int pageHeight, boolean overScroll, boolean overScrollLeft) {
        float scrollProgress = offset;
        boolean isRtl = false;

        float interpolatedProgress;
        float translationX;
        float maxScrollProgress = Math.max(0, scrollProgress);
        float minScrollProgress = Math.min(0, scrollProgress);

        if (isRtl) {
            translationX = maxScrollProgress * v.getMeasuredWidth();
            interpolatedProgress = mZInterpolator.getInterpolation(Math
                    .abs(maxScrollProgress));
        } else {
            translationX = minScrollProgress * v.getMeasuredWidth();
            interpolatedProgress = mZInterpolator.getInterpolation(Math
                    .abs(minScrollProgress));
        }
        float scale = (1 - interpolatedProgress) + interpolatedProgress
                * TRANSITION_SCALE_FACTOR;

        float alpha;
        if (isRtl && (scrollProgress > 0)) {
            alpha = mAlphaInterpolator.getInterpolation(1 - Math
                    .abs(maxScrollProgress));
        } else if (!isRtl && (scrollProgress < 0)) {
            alpha = mAlphaInterpolator.getInterpolation(1 - Math
                    .abs(scrollProgress));
        } else {
            // On large screens we need to fade the page as it nears its
            // leftmost position
            alpha = mLeftScreenAlphaInterpolator
                    .getInterpolation(1 - scrollProgress);
        }

        float xPivot = isRtl ? 1f - TRANSITION_PIVOT : TRANSITION_PIVOT;

        if (overScroll && overScrollLeft) {
            // Overscroll to the left
            v.setPivotX(xPivot * pageWidth);
            v.setRotationY(-TRANSITION_MAX_ROTATION * scrollProgress);
            scale = 1.0f;
            alpha = 1.0f;
            // On the first page, we don't want the page to have any lateral
            // motion
            translationX = 0;
        } else if (overScroll && !overScrollLeft) {
            // Overscroll to the right
            v.setPivotX((1 - xPivot) * pageWidth);
            v.setRotationY(-TRANSITION_MAX_ROTATION * scrollProgress);
            scale = 1.0f;
            alpha = 1.0f;
            // On the last page, we don't want the page to have any lateral
            // motion.
            translationX = 0;
        } else {
            v.setPivotY(pageHeight / 2.0f);
            v.setPivotX(pageWidth / 2.0f);
            v.setRotationY(0f);
        }

        v.setTranslationX(translationX);
        v.setScaleX(scale);
        v.setScaleY(scale);
        v.setAlpha(alpha);

        // If the view has 0 alpha, we set it to be invisible so as to prevent
        // it from accepting touches
        if (alpha == 0) {
            v.setVisibility(View.VISIBLE);
        } else if (v.getVisibility() != View.VISIBLE) {
            v.setVisibility(View.VISIBLE);
        }
        /* @} */
    }

    @Override
    public Scroller getScroller(Context context) {
        // TODO Auto-generated method stub
        return new Scroller(context);
    }

    @Override
    public int getSnapTime() {
        return 220;
    }
}
