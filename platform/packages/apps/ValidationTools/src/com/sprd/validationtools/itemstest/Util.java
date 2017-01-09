
package com.sprd.validationtools.itemstest;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.content.res.TypedArray;

import com.sprd.validationtools.R;
import com.sprd.validationtools.Const;

public class Util {

    public static Tuple<Integer, Integer> getOptimalSize(
            int sWidth, int sHeight, int width, int height, boolean screen) {
        width = Math.max(width, height);
        height = Math.min(width, height);
        double ratio = (1 / (((double) width) / ((double) height)));
        return getOptimalSize(sWidth, sHeight, ratio, screen);
    }

    public static Tuple<Integer, Integer>
            getOptimalSize(int screenWidth, int screenHeight, double ratio, boolean screen) {
        Tuple<Integer, Integer> result =
                new Tuple<Integer, Integer>(screenWidth, screenHeight);
        int max = -1, min = -1, width = -1, height = -1;
        if (ratio > 1D)
            ratio = (1 / ratio);

        if (screen) {
            max = Math.max(screenWidth, screenHeight);
            min = ((int) (max * ratio));
            if (screenWidth < screenHeight) {
                width = min;
                height = max;
            } else {
                height = min;
                width = max;
            }
        }
        else {
            min = Math.min(screenWidth, screenHeight);
            max = ((int) (min / ratio));
            if (screenWidth > screenHeight) {
                width = max;
                height = min;
            } else {
                width = min;
                height = max;
            }
        }
        if (screenWidth != width || screenHeight != height) {
            result = new Tuple<Integer, Integer>(width, height);
        }
        return result;
    }

    public static String[] getItemStrings(Resources res, int iconsRes) {
        if (iconsRes == 0)
            return null;
        TypedArray array = res.obtainTypedArray(iconsRes);
        int n = array.length();
        String ids[] = new String[n];
        for (int i = 0; i < n; ++i) {
            ids[i] = res.getString(array.getResourceId(i, 0));
        }
        array.recycle();
        return ids;
    }
}
