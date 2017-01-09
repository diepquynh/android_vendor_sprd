/** Created by Spreadtrum */
package com.sprd.sprdlauncher2.effect;

import android.content.Context;
import android.graphics.Camera;
import android.graphics.Matrix;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class CubeEffect extends EffectInfo {

    private boolean flag;

    public CubeEffect(int id) {
        super(id);
    }

    public CubeEffect(int id, boolean flag) {
        super(id, flag);
        this.flag = flag;

    }

    @Override
    public boolean getCellLayoutChildStaticTransformation(ViewGroup viewGroup, View viewiew,
            Transformation transformation, Camera camera, float offset) {
        return false;
    }

    @Override
    public boolean getWorkspaceChildStaticTransformation(ViewGroup viewGroup, View viewiew,
            Transformation transformation, Camera camera, float offset) {
        Matrix tMatrix = transformation.getMatrix();
        float mViewHalfWidth = viewiew.getMeasuredWidth() / 2.0F;
        float mViewHalfHeight = viewiew.getMeasuredHeight() / 2.0F;
        if (offset == 0.0F || offset >= 1.0F || offset <= -1.0F)
            return false;
        transformation.setAlpha(1.0F - Math.abs(offset));
        // viewiew.setAlpha(1.0F - Math.abs(offset));
        camera.save();
        camera.translate(0.0F, 0.0F, mViewHalfWidth);
        camera.rotateY(-90.0F * offset);
        camera.translate(0.0F, 0.0F, -mViewHalfWidth);
        camera.getMatrix(tMatrix);
        camera.restore();
        tMatrix.preTranslate(-mViewHalfWidth, -mViewHalfHeight);
        float transOffset = (1.0F + 2.0F * offset) * mViewHalfWidth;
        tMatrix.postTranslate(transOffset, mViewHalfHeight);
        transformation.setTransformationType(Transformation.TYPE_BOTH);
        return true;
    }

    /* SPRD: Fix bug258437 @{*/
    @Override
    public void getTransformationMatrix(View view, float offset, int pageWidth, int pageHeight,
            float distance, boolean overScroll, boolean overScrollLeft) {

        float rotation = (flag ? 90.0f : -90.0f) * offset;
        float alpha = 1 - Math.abs(offset) * 0.4f;
        if (flag) {
            view.setCameraDistance(distance);
        }

        float xPost = 0f;
        if (overScroll) {
            if (overScrollLeft) {
                xPost = pageWidth * Math.abs(offset);
            } else {
                xPost = pageWidth * Math.abs(offset) * -1f;
            }
        }
        view.setTranslationX(xPost);
        /* @} */

        view.setPivotX(offset < 0 ? 0 : pageWidth);
        view.setPivotY(pageHeight * 0.5f);
        view.setRotationY(rotation);
        view.setAlpha(alpha);
        view.invalidate();

    }

    @Override
    public Scroller getScroller(Context context) {
        return new Scroller(context, new DecelerateInterpolator());
    }

    @Override
    public int getSnapTime() {
        return 180;
    }

}
