/** Created by Spreadtrum */
package com.sprd.sprdlauncher2.effect;

import android.content.Context;
import android.graphics.Camera;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Transformation;
import android.widget.Scroller;

public abstract class EffectInfo {
    public final int id;
    public boolean flag;

    public EffectInfo(int id) {
        this.id = id;
    }

    public EffectInfo(int id, boolean flag) {
        this.id = id;
        this.flag = flag;

    }
    public abstract boolean getCellLayoutChildStaticTransformation(ViewGroup viewGroup, View viewiew, Transformation transformation, Camera camera, float offset);

    public abstract boolean getWorkspaceChildStaticTransformation(ViewGroup viewGroup, View viewiew, Transformation transformation, Camera camera, float offset);
    public abstract Scroller getScroller(Context context);
    public abstract int getSnapTime();

    /* SPRD: Fix bug258437 @{*/
    public abstract void getTransformationMatrix(View view ,float offset, int pageWidth, int pageHeight,float distance, boolean overScroll, boolean overScrollLeft);
    /* @} */
    public float getWorkspaceOvershootInterpolatorTension(){
        return 0f;// we set 0 as default value
    }
}
