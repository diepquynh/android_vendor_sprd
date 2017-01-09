/** Created by Spreadtrum */
package com.sprd.sprdlauncher2.effect;

import android.content.Context;
import android.graphics.Camera;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class FlatEffect extends EffectInfo{

    public FlatEffect(int id) {
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
        return false;
    }


    /* SPRD: Fix bug258437 @{*/
    @Override
    public void getTransformationMatrix(View view ,float offset, int pageWidth, int pageHeight,float distance, boolean overScroll, boolean overScrollLeft){
    /* @} */
        float absOffset = Math.abs(offset);
        float mAlpha = 1.0F - absOffset;

        int mViewWidth = pageWidth;
        int mViewHeight = pageHeight;

        int mViewHalfWidth = mViewWidth >> 1;
        int mViewHalfHeight = mViewHeight >> 1;



        float yRotate = 60.0F * (-offset);

        float xPost = mViewHalfWidth * offset;

        view.setTranslationX(xPost);
        view.setPivotY(mViewHalfHeight);
        view.setPivotX(mViewHalfWidth);
        view.setRotationY(yRotate);
        view.setRotationX(0f);
        view.setRotation(0f);
        view.setScaleX(1.0f);
        view.setScaleY(1.0f);

        view.setAlpha(mAlpha);
    }
    @Override
    public Scroller getScroller(Context context) {
        return null;
    }

    public float getWorkspaceOvershootInterpolatorTension(){
        return 1.3f;
    }

    @Override
    public int getSnapTime() {
        return 200;
    }
}
