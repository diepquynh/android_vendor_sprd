/** Created by Spreadst */
package com.sprd.launcher3.effect;

import android.content.Context;
import android.graphics.Camera;
import android.graphics.Matrix;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class PageEffect extends EffectInfo{

    public PageEffect(int id) {
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
        float mViewWidth = viewiew.getMeasuredWidth();
        float mViewHalfWidth = mViewWidth / 2.0F;
        float mViewHalfHeight = viewiew.getMeasuredHeight() / 2.0F;
        float absOffset = Math.abs(offset);
        if (offset == 0.0F || offset >= 1.0F || offset <= -1.0F)
            return false;
         transformation.setAlpha(1.0F - absOffset);
        //viewiew.setAlpha(1.0F - absOffset);
        camera.save();
        float xTranslate = (-mViewHalfWidth) * absOffset / 3.0F;
        float zTranslate = -mViewHalfWidth * offset;
        camera.translate(xTranslate, mViewHalfHeight, zTranslate);
        float yRotate = 30.0F * (-offset);
        camera.rotateY(yRotate);
        camera.getMatrix(tMatrix);
        camera.restore();
        float xPost = mViewWidth * offset;
        tMatrix.postTranslate(xPost, mViewHalfHeight);
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

        if (overScroll) {
            view.setTranslationX(0f);
        } else {
            view.setTranslationX(xPost);
        }
        /* @} */

        view.setPivotY(mViewHalfHeight);
        view.setPivotX(0);
        view.setRotationY(yRotate);
        view.setRotationX(0f);
        view.setRotation(0f);
        view.setScaleX(1.0f);
        view.setScaleY(1.0f);

        view.setAlpha(mAlpha);
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
