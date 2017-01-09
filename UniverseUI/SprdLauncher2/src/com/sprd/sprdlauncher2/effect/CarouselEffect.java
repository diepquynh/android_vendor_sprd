/** Created by Spreadtrum */
package com.sprd.sprdlauncher2.effect;

import android.content.Context;
import android.graphics.Camera;
import android.graphics.Matrix;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class CarouselEffect extends EffectInfo {

    private boolean flag;

    public CarouselEffect(int id) {
        super(id);
    }

    public CarouselEffect(int id, boolean flag) {
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
        float mViewWidth = viewiew.getMeasuredWidth();
        float mViewHalfWidth = mViewWidth / 2.0F;
        float mViewHalfHeight = viewiew.getMeasuredHeight() / 2.0F;
        float absOffset = Math.abs(offset);
        if (offset == 0.0F || offset >= 1.0F || offset <= -1.0F)
            return false;
        transformation.setAlpha(1.0F - absOffset);
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

    /* SPRD: Fix bug258437 @{*/
    @Override
    public void getTransformationMatrix(View view, float offset, int pageWidth, int pageHeight,
            float distance, boolean overScroll, boolean overScrollLeft) {

        float absOffset = Math.abs(offset);
        float mAlpha = 1.0F - absOffset * 0.4f;

        int mViewWidth = pageWidth;
        int mViewHeight = pageHeight;

        int mViewHalfHeight = mViewHeight >> 1;

        view.setCameraDistance(distance);
        float yRotate = 90.0F * (-offset);
        float xPost = mViewWidth * offset;
        if (overScroll) {
            xPost = 0f;
        }
    /* @} */
        view.setTranslationX(xPost);
        view.setPivotY(mViewHalfHeight);
        view.setPivotX(flag ? 0.0f : mViewWidth);
        view.setRotationY(yRotate);
        view.setRotationX(0f);
        view.setRotation(0f);
        view.setScaleX(1.0f);
        view.setScaleY(1.0f);
        view.setAlpha(mAlpha);
    }

    @Override
    public Scroller getScroller(Context context) {
        return new Scroller(context);
    }

    @Override
    public int getSnapTime() {
        return 220;
    }
}
