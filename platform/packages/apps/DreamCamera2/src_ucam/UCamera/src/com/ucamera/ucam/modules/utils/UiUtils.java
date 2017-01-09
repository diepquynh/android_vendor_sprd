/*
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 *
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.ucamera.ucam.modules.utils;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.DisplayMetrics;
import android.view.WindowManager;
import android.widget.HorizontalScrollView;
import android.content.ContentResolver;
import android.os.ParcelFileDescriptor;
import android.net.Uri;

import com.android.camera2.R;
import com.ucamera.ucam.modules.utils.LogUtils;
import com.ucamera.ucam.modules.utils.Utils;

import android.view.animation.Animation;
import android.view.animation.TranslateAnimation;

/**
 * Collection of utility functions used in this package.
 */
public abstract class UiUtils {
    private static final String TAG = "Util";

    private static float sPixelDensity = 1;
    private static int   sPixelHeight  = -1;
    private static int   sPixelWidth   = -1;
    private static int   sDensityDpi   = 1;
    private static int   sMenuItemWidth = -1;
    private static int   sMenuItemHeight = -1;
    private static int   sEffectItemWidth = -1;
    private static int   sEffectItemHeight = -1;
    private static boolean sHighMemo = false;
    // CID 123780 : UuF: Unused field (FB.UUF_UNUSED_FIELD)
    // private static int mScreenWidth;

    private static boolean mInitialized = false;

    private UiUtils() {}

    public static void initialize(Context context) {
        if (mInitialized) {
            return;
        }
        mInitialized = true;

        DisplayMetrics metrics = new DisplayMetrics();
        WindowManager wm = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        wm.getDefaultDisplay().getMetrics(metrics);
        sPixelDensity = metrics.density;
        sPixelHeight  = metrics.heightPixels;
        sPixelWidth   = metrics.widthPixels;
        if (sPixelHeight < sPixelWidth) {
            int temp = sPixelHeight;
            sPixelHeight = sPixelWidth;
            sPixelWidth = temp;
        }
        sDensityDpi = metrics.densityDpi;
//        int[] itemParam = context.getResources().getIntArray(R.array.gallery_img_layout_params);
//        sMenuItemWidth = Math.round(itemParam[0] * (sPixelDensity < 1.0f ? 1.2f : sPixelDensity));
//        sMenuItemHeight = Math.round(itemParam[1] * (sPixelDensity < 1.0f ? 1.2f : sPixelDensity));

        final Drawable drawable = context.getResources().getDrawable(R.drawable.magiclens_menu_category_item_bk);
        sEffectItemWidth = drawable.getIntrinsicWidth();
        sEffectItemHeight = drawable.getIntrinsicHeight();

        int totalMem = getTotalMemory(context);
        LogUtils.debug(TAG, "init(): totalMem is " + totalMem + "MB");
        if(totalMem >= 512) {
            sHighMemo = true;
        }
        LogUtils.debug(TAG, "screen size is (" + sPixelWidth + " x " + sPixelHeight + "), density is " + sPixelDensity
                + ", sDensityDpi is " + sDensityDpi + ", sMenuItem.WH is (" + sMenuItemWidth  + ", " + sMenuItemHeight + ")");
    }

    public static int dpToPixel(int dp) {
        return Math.round(sPixelDensity * dp);
    }

    public static final int screenWidth() { return sPixelWidth;}
    public static final int screenHeight(){ return sPixelHeight;}
    public static final float screenDensity(){ return sPixelDensity;}
    public static final int screenDensityDPI() {return sDensityDpi;}
    public static final int menuItemWidth() {return sMenuItemWidth;}
    public static final int menuItemHeight() {return sMenuItemHeight;}
    public static final int effectItemWidth() {return sEffectItemWidth;}
    public static final int effectItemHeight() {return sEffectItemHeight;}
    public static final boolean highMemo() {return sHighMemo; }



    public static void scrollToCurrentPosition(HorizontalScrollView scroller, int itemWidth, int currentPosition){

        if(itemWidth > 0 && scroller != null ) {
           if(currentPosition > sPixelWidth/itemWidth) {
               scroller.scrollTo(itemWidth * currentPosition, scroller.getScrollY());
           } else {
               scroller.scrollTo(0, scroller.getScrollY());
           }
        }
    }

    public static Animation createTranslateAnimation(Context context,float x_from,float x_to,float y_from,float y_to, long duration){
        Animation anim = null;
        anim = new TranslateAnimation(x_from, x_to, y_from, y_to);
        anim.setDuration(duration);
        anim.setFillAfter(false);
        return anim;
    }

    private static int getTotalMemory(Context context) {
        String memInfoFile = "/proc/meminfo";
        BufferedReader bufferedReader = null;
        try {
            bufferedReader = new BufferedReader(new FileReader(memInfoFile), 4096);
            String memInfo = bufferedReader.readLine();
            /* SPRD:  CID 108004 : Dereference null return value (NULL_RETURNS) @{ */
            if(memInfo != null){
                String[] splitInfo = memInfo.split("\\s+");
                return (int) Math.ceil(Integer.valueOf(splitInfo[1]).intValue() / 1024f);
            }

            /**
             * String[] splitInfo = memInfo.split("\\s+");
             * return (int) Math.ceil(Integer.valueOf(splitInfo[1]).intValue() / 1024f);
             */
            /* @} */
        } catch (IOException e) {

        } finally {
            Utils.closeSilently(bufferedReader);
        }
        return 0;
    }

    public static boolean isUriValid(Uri uri, ContentResolver resolver) {
        if (uri == null) return false;
        try {
            ParcelFileDescriptor pfd = resolver.openFileDescriptor(uri, "r");
            if (pfd == null) {
                return false;
            }
            pfd.close();
        } catch (IOException ex) {
            return false;
        }
        return true;
    }
}

