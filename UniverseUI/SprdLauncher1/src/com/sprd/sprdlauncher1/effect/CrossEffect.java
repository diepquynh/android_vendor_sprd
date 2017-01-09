/** Created by Spreadst */
package com.sprd.launcher3.effect;

import android.content.Context;
import android.graphics.Camera;
import android.graphics.Matrix;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class CrossEffect extends EffectInfo{

    public CrossEffect(int id) {
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
        float mViewWidth = viewiew.getMeasuredWidth();
        float mViewHeight = viewiew.getMeasuredHeight();
        Matrix tMatrix = transformation.getMatrix();
        if (offset == 0.0F || offset >= 1.0F || offset <= -1.0F)
            return false;
          float mAlpha = 1.0F - Math.abs(offset);
         transformation.setAlpha(mAlpha);
         // viewiew.setAlpha(mAlpha);
          camera.save();
          camera.translate(mViewWidth * offset, 0.0F, 0.0F);
          camera.rotateY(45.0F * offset);
          camera.getMatrix(tMatrix);
          camera.restore();
          tMatrix.preTranslate(-mViewWidth/2.0F, -mViewHeight/2.0F);
          tMatrix.postTranslate(mViewWidth/2.0F, mViewHeight/2.0F);
          transformation.setTransformationType(Transformation.TYPE_BOTH);
        return true;
    }

    /* SPRD: Fix bug257614 @{ */
    @Override
    public void getTransformationMatrix(View view ,float offset, int pageWidth, int pageHeight, boolean overScroll, boolean overScrollLeft){
        float absOffset = Math.abs(offset);
        float mAlpha = 1.0F - absOffset;

        int mViewWidth = pageWidth;
        int mViewHeight = pageHeight;

        int mViewHalfWidth = mViewWidth >> 1;
        int mViewHalfHeight = mViewHeight >> 1;

        float yRotate = 90.0F * (-offset);

        float xPost = mViewWidth * offset;

        if(offset == -1 || offset == 0 || offset == 1){
            if (overScroll) {
                view.setTranslationX(0f);
            } else {
                view.setTranslationX(xPost);
            }
            view.setRotationY(0);
            view.setAlpha(1f);
            view.setPivotY(mViewHalfHeight);
            view.setPivotX(mViewHalfWidth);
        }else{
            if (overScroll) {
                view.setTranslationX(0f);
            } else {
                view.setTranslationX(xPost);
            }
            view.setPivotY(mViewHalfHeight);
            view.setPivotX(mViewHalfWidth);
            view.setRotationY(yRotate);
            view.setAlpha(mAlpha);
        }
        /* @} */
    }
    @Override
    public Scroller getScroller(Context context) {
        return new Scroller(context);
    }

    @Override
    public int getSnapTime() {
        return 200;
    }

}
