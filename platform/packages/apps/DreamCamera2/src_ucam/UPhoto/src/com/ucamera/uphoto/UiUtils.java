/*
 * Copyright (C) 2010,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.DisplayMetrics;
import android.view.WindowManager;

public class UiUtils {
    private static final String TAG = "UiUtil";
    private static float sPixelDensity = 1;
    private static int   sPixelHeight  = -1;
    private static int   sPixelWidth   = -1;
    private static int   sDensityDpi   = 1;

    private static int   sEffectItemWidht = -1;
    private static int   sEffectItemHeight = -1;

    public static final int DIRECTION_LEFT = 0;
    public static final int DIRECTION_RIGHT = 1;

    private UiUtils() {}

    public static void initialize(Context context) {
        DisplayMetrics metrics = new DisplayMetrics();
        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        wm.getDefaultDisplay().getMetrics(metrics);
        sPixelDensity = metrics.density;
        sPixelHeight  = metrics.heightPixels;
        sPixelWidth   = metrics.widthPixels;
        sDensityDpi = metrics.densityDpi;
        Drawable drawable = context.getResources().getDrawable(R.drawable.edit_effect_cate_item_normal);
        // avoid null pointer exception --bug 5351
        sEffectItemWidht = drawable != null ? drawable.getIntrinsicWidth() : 92;
        sEffectItemHeight = drawable != null ? drawable.getIntrinsicHeight() : 92;
    }

    public static int dpToPixel(int dp) {
        return Math.round(sPixelDensity * dp);
    }

    public static final int screenWidth() { return sPixelWidth;}
    public static final int screenHeight(){ return sPixelHeight;}
    public static final float screenDensity(){ return sPixelDensity;}
    public static final int screenDensityDPI() {return sDensityDpi;};
    public static final int effectItemWidth() {return sEffectItemWidht;}
    public static final int effectItemHeight() {return sEffectItemHeight;}
}
