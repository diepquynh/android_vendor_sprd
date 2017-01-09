/** Create by Spreadst */

package com.spreadst.s2lockscreen;

import android.graphics.drawable.AnimationDrawable;

public class Tools {

    public static int initAdDuration(AnimationDrawable ad) {
        int duration = 0;
        for (int i = 0; i < ad.getNumberOfFrames(); i++) {
            duration += ad.getDuration(i);
        }
        return duration;
    }

    public static int getL(double nowL, float maxL, float mixL) {
        float l = (maxL - mixL) / 7;
        nowL = nowL - mixL;
        for (int i = 1; i < 7; i++) {
            if (i * l <= nowL && (i + 1) * l >= nowL) {
                return i;
            }
        }

        return 1;
    }

    public static boolean isout(float x1, float y1, float x2, float y2, float l) {
        return isout(x1, y1, x2, y2) > l;
    }

    public static double isout(float x1, float y1, float x2, float y2) {
        float x = x1 - x2;
        float y = y1 - y2;
        return Math.hypot(x, y);
    }

}
