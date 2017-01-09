/** Created by Spreadst */
package com.sprd.launcher3.effect;

import android.content.Context;
import android.graphics.Camera;
import android.graphics.Matrix;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class CubeEffect extends EffectInfo{

    public CubeEffect(int id) {
        super(id);
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
          transformation.setAlpha(1.0F -  Math.abs(offset));
         // viewiew.setAlpha(1.0F -  Math.abs(offset));
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

    /* SPRD: Fix bug257614 @{ */
    @Override
    public void getTransformationMatrix(View view ,float offset, int pageWidth, int pageHeight, boolean overScroll, boolean overScrollLeft){
        final int mViewHalfWidth = pageWidth >> 1;
        final int mViewHalfHeight = pageHeight >> 1;
        final int mViewWidth = pageWidth;
        float offsetDegree = 0f;
        float xPost = 0f;
        if (overScroll) {
            if (overScrollLeft) {
                xPost = mViewHalfWidth * Math.abs(offset);
            } else {
                xPost = mViewHalfWidth * Math.abs(offset) * -1f;
            }
        } else {
            xPost = mViewHalfWidth* offset;
        }
        view.setTranslationX(xPost);
        // SPRD: fix bug271059 if set it more than 90 ,it will cover the next page
        view.setRotationY(-90 * offset);
        view.setAlpha(1.0F -  Math.abs(offset));
        view.setPivotY(mViewHalfHeight);
        view.setPivotX(mViewHalfWidth);
        view.setRotationX(0f);
        view.setRotation(offsetDegree);
        view.setScaleX(1.0f);
        view.setScaleY(1.0f);
        /* @} */
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
