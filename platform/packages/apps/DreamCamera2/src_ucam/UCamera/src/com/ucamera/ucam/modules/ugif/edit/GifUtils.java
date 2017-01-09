/*
 *   Copyright (C) 2010,2013 Thundersoft Corporation
 *   All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit;

import com.ucamera.ucam.modules.utils.Utils;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.util.Log;

public class GifUtils {
    /** create the scaled bitmaps from **.gif
     * @param bmps the bitmaps needed to scale
     * @return the scaled bitmaps.
     */
    public static Bitmap[] scaleBitmaps(Bitmap[] bmps) {
        int n = bmps.length;
        int picSize = getFitGifSize(bmps);
        for (int i = 0; i < n; i++) {
            int x = 0;
            int y = 0;
            int widthCut;
            if (bmps[i].getWidth() > bmps[i].getHeight()) {
                widthCut = bmps[i].getHeight();
                x = (bmps[i].getWidth() - widthCut) / 2;
            } else {
                widthCut = bmps[i].getWidth();
                y = (bmps[i].getHeight() - widthCut) / 2;
            }
            Matrix matrix = new Matrix();
            matrix.postScale((float) picSize / widthCut, (float) picSize
                    / widthCut);
            try {
                Bitmap tmp = Bitmap.createBitmap(bmps[i], x, y, widthCut, widthCut,
                        matrix, true);
                if (tmp != bmps[i] && tmp != null){
                    Utils.recycleBitmap(bmps[i]);
                    bmps[i] = tmp;
                }
            } catch (OutOfMemoryError ex) {
                /** FIX BUG: 6567
                 * BUG CAUSE:gif is too big to decoder
                 * FIX COMMENT:catch the exception and recyle the bitmap created.
                 * Date: 2011-12-15
                 */
                Log.e("not enough memory in gif edit", ""+ex.getMessage());
                Utils.recycleBitmaps(bmps);
                return null;
            }

        }
        return bmps;
    }

    /**
     * get the smallest picture size of bmps
     * @param bmps the bitmap list need to scaled
     * @return the width to scale.
     */
    private static int getFitGifSize(Bitmap[] bmps){
        int n = bmps.length;
        int bmpSize = 0;
        int gifMaxSize = 260;
        int fitSize = 0;
        for (int i = 0; i < n; i++) {
            if (bmps[i].getWidth() > bmps[i].getHeight()) {
                bmpSize = bmps[i].getHeight();
            } else {
                bmpSize = bmps[i].getWidth();
            }
            if (fitSize == 0) {
                fitSize = bmpSize;
            }
            if (bmpSize < fitSize) {
                fitSize = bmpSize;
            }
        }
        if (fitSize > gifMaxSize) {
           return gifMaxSize;
        }else {
           return fitSize;
        }
    }
}
