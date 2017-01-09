/** Created by Spreadtrum */
package com.sprd.sprdlauncher2.effect;

import android.content.Context;
import android.graphics.Camera;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Transformation;
import android.widget.Scroller;

public class CuboidEffect extends EffectInfo{

    public CuboidEffect(int id) {
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
    /* @ */
    }

}