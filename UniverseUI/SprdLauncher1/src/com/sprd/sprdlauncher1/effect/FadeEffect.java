/** Created by Spreadst */
package com.sprd.launcher3.effect;

import android.content.Context;
import android.graphics.Camera;
import android.graphics.Matrix;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class FadeEffect extends EffectInfo{

    public FadeEffect(int id) {
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
        if (offset == 0.0F || offset >= 1.0F || offset <= -1.0F)
            return false;
        Matrix tMatrix = transformation.getMatrix();
        float absOffset = Math.abs(offset);
        float mAlpha = (1.0F - absOffset) * 0.7F + 0.3F;
        transformation.setAlpha(mAlpha);
        tMatrix.setScale(mAlpha, mAlpha, viewiew.getWidth()/2.0f, viewiew.getHeight()/2.0f);
        transformation.setTransformationType(Transformation.TYPE_BOTH);
//        transformation.setTransformationType(Transformation.TYPE_ALPHA);
        return true;
    }

    /* SPRD: Fix bug257614 @{ */
    @Override
    public void getTransformationMatrix(View view ,float offset, int pageWidth, int pageHeight, boolean overScroll, boolean overScrollLeft){
    /* @} */
        float absOffset = Math.abs(offset);
        float mAlpha = 1.0F - absOffset;

        int mViewWidth = pageWidth;
        int mViewHeight = pageHeight;

        int mViewHalfWidth = mViewWidth >> 1;
        int mViewHalfHeight = mViewHeight >> 1;

        view.setTranslationX(0f);
        view.setPivotY(mViewHalfHeight);
        view.setPivotX(mViewHalfWidth);
        view.setRotationX(0f);
        view.setRotation(0f);
        view.setRotationY(0f);
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
        return 240;
    }
}
