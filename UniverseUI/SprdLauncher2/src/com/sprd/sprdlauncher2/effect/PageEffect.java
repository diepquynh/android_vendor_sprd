/** Created by Spreadtrum */
package com.sprd.sprdlauncher2.effect;

import android.content.Context;
import android.graphics.Camera;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class PageEffect extends EffectInfo {

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
        return false;
    }

    @Override
    public Scroller getScroller(Context context) {
        return null;
    }

    @Override
    public int getSnapTime() {
        return 0;
    }

    /* SPRD: Fix bug258437 @{*/
    @Override
    public void getTransformationMatrix(View view, float offset, int pageWidth, int pageHeight,
            float distance, boolean overScroll, boolean overScrollLeft) {
        float xpost = pageWidth * offset;
        view.setCameraDistance(distance);
        if (offset > 0 && offset < 1) {
            view.setPivotX(0);
            view.setPivotY(pageHeight >> 1);
            view.setRotationY(offset * -120.f);

        }

        if (offset > -1 && offset < 0) {
            view.setPivotX(view.getMeasuredWidth());
            view.setPivotY(view.getMeasuredHeight() >> 1);
            view.setRotationY(offset * 120.0f);
        }
        if (overScroll) {
            view.setTranslationX(0f);
        } else {
            view.setTranslationX(xpost);
        }
        /* @} */
        view.setScaleX(1.0f);
        view.setScaleY(1.0f);
    }
}
