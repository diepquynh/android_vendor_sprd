/** Created by Spreadst */
package com.sprd.launcher3.effect;

import android.content.Context;
import android.graphics.Camera;
import android.graphics.Matrix;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class RotateEffect extends EffectInfo{

    public RotateEffect(int id) {
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
        float mViewWidth = viewiew.getMeasuredWidth();
        float mViewHeight = viewiew.getMeasuredHeight();
        Matrix tMatrix = transformation.getMatrix();
        float offsetDegree = -offset * 45.0F;
        tMatrix.setRotate(offsetDegree, mViewWidth/2.0F, mViewHeight);
        transformation.setTransformationType(Transformation.TYPE_MATRIX);
        return true;
    }

    /* SPRD: Fix bug257614 @{ */
    @Override
    public void getTransformationMatrix(View view ,float offset, int pageWidth, int pageHeight, boolean overScroll, boolean overScrollLeft){
        int mViewWidth = pageWidth;
        int mViewHeight = pageHeight;

        float offsetDegree = -offset * 45.0F;
        view.setTranslationX(0f);
        if (overScroll) {
            if (overScrollLeft) {
                view.setPivotX(1.4f * mViewWidth / 2.0f);
                view.setPivotY(mViewHeight * 2.2f);
                view.setRotation(offsetDegree);
            } else {
                view.setPivotX(0.6f * mViewWidth / 2.0f);
                view.setPivotY(mViewHeight * 2.2f);
                view.setRotation(offsetDegree);
            }
        } else {
            view.setPivotY(mViewHeight);
            view.setPivotX(mViewWidth / 2.0f);
            view.setRotation(offsetDegree);
        }
        /* @} */
        view.setRotationX(0f);
        view.setRotationY(0f);
        view.setScaleX(1.0f);
        view.setScaleY(1.0f);
        view.setAlpha(1f);

    }
    @Override
    public Scroller getScroller(Context context) {
        // TODO Auto-generated method stub
        return new Scroller(context);
    }

    @Override
    public int getSnapTime() {
        return 180;
    }
}
