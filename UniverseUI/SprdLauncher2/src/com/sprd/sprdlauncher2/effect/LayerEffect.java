/** Created by Spreadtrum */
package com.sprd.sprdlauncher2.effect;

import android.content.Context;
import android.graphics.Camera;
import android.graphics.Matrix;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class LayerEffect extends EffectInfo{

    public LayerEffect(int id) {
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
        float mViewHeight = viewiew.getMeasuredHeight();
        if (offset == 0.00F || offset >= 0.99F || offset <= -0.99F)
            return false;
        transformation.setAlpha(1.0F - offset);
        //viewiew.setAlpha(1.0F - offset);
        float level = 0.4F * (1.0F - offset);
        float scale = 0.6F + level;
        tMatrix.setScale(scale, scale);
        float xPost = 0.4F * offset* mViewWidth * 3.0F;
        float yPost = 0.4F * offset* mViewHeight * 0.5F;
        tMatrix.postTranslate(xPost, yPost);
        transformation.setTransformationType(Transformation.TYPE_BOTH);
        return true;
    }


    /* SPRD: Fix bug258437 @{*/
    @Override
    public void getTransformationMatrix(View view ,float offset, int pageWidth, int pageHeight,float distance, boolean overScroll, boolean overScrollLeft){
    /* @} */
        float absOffset = Math.abs(offset);
        float mAlpha = 1.0F - absOffset;

        int mViewWidth = pageWidth;
        int mViewHeight = pageHeight;

        int mViewHalfWidth = mViewWidth >>  1;
        int mViewHalfHeight = mViewHeight >> 1;


        float level = 0.4F * (1.0F - offset);
        float scale = 0.6F + level;



        float xPost = 0.4F * offset* mViewWidth * 3.0F;
        float yPost = 0.4F * offset* mViewHeight * 0.5F;

        view.setScaleX(scale);
        view.setScaleY(scale);
        view.setTranslationX(xPost);
        view.setTranslationY(0);
        view.setPivotY(mViewHalfHeight);
        view.setPivotX(mViewHalfWidth);
        view.setRotationY(0f);
        view.setRotationX(0f);
        view.setRotation(0f);

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
