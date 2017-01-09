/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.sprdlauncher1;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;
import android.graphics.Bitmap;
import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.ColorMatrix;
import android.graphics.ColorMatrixColorFilter;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PaintFlagsDrawFilter;
import android.graphics.Rect;
import android.graphics.Bitmap.Config;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.PaintDrawable;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Random;

/* SPRD: bug333853 2014-07-16 use MediaFile to get mime type from file path. @{ */
import android.media.MediaFile;
import android.media.MediaFile.MediaFileType;
/* SPRD: bug333853 2014-07-16 use MediaFile to get mime type from file path. @} */

/**
 * Various utilities shared amongst the Launcher's classes.
 */
final class Utilities {
    private static final String TAG = "Launcher.Utilities";

    private static int sIconWidth = -1;
    private static int sIconHeight = -1;
    public static int sIconTextureWidth = -1;
    public static int sIconTextureHeight = -1;

    private static final Paint sBlurPaint = new Paint();
    private static final Paint sGlowColorPressedPaint = new Paint();
    private static final Paint sGlowColorFocusedPaint = new Paint();
    private static final Paint sDisabledPaint = new Paint();
    private static final Rect sOldBounds = new Rect();
    private static final Canvas sCanvas = new Canvas();
    /* SPRD: Feature 253522, Remove the application drawer view @{ */
    private static final int[] icons_back = new int[]{
        R.drawable.icon_back,
    };
    private static final int icons_pre = R.drawable.icon_pre;
    /* @} */

    /* SPRD: Fix bug288618, @{ */
    private static final int COMPOSE_SHOULD_INIT = -1;
    private static final int DONT_COMPOSE_ICON = 0;
    private static final int NEED_COMPOSE_ICON = 1;
    private static int sNeedComposeIcon = COMPOSE_SHOULD_INIT;
    /* @} */

    static {
        sCanvas.setDrawFilter(new PaintFlagsDrawFilter(Paint.DITHER_FLAG,
                Paint.FILTER_BITMAP_FLAG));
    }
    static int sColors[] = { 0xffff0000, 0xff00ff00, 0xff0000ff };
    static int sColorIndex = 0;

    /**
     * Returns a FastBitmapDrawable with the icon, accurately sized.
     */
    static Drawable createIconDrawable(Bitmap icon) {
        FastBitmapDrawable d = new FastBitmapDrawable(icon);
        d.setFilterBitmap(true);
        resizeIconDrawable(d);
        return d;
    }

    /**
     * Resizes an icon drawable to the correct icon size.
     */
    static void resizeIconDrawable(Drawable icon) {
        icon.setBounds(0, 0, sIconTextureWidth, sIconTextureHeight);
    }

    /**
     * Returns a bitmap suitable for the all apps view. Used to convert pre-ICS
     * icon bitmaps that are stored in the database (which were 74x74 pixels at hdpi size)
     * to the proper size (48dp)
     */
    static Bitmap createIconBitmap(Bitmap icon, Context context) {
        int textureWidth = sIconTextureWidth;
        int textureHeight = sIconTextureHeight;
        int sourceWidth = icon.getWidth();
        int sourceHeight = icon.getHeight();
        if (sourceWidth > textureWidth && sourceHeight > textureHeight) {
            // Icon is bigger than it should be; clip it (solves the GB->ICS migration case)
            return Bitmap.createBitmap(icon,
                    (sourceWidth - textureWidth) / 2,
                    (sourceHeight - textureHeight) / 2,
                    textureWidth, textureHeight);
        } else if (sourceWidth == textureWidth && sourceHeight == textureHeight) {
            // Icon is the right size, no need to change it
            return icon;
        } else {
            // Icon is too small, render to a larger bitmap
            final Resources resources = context.getResources();
            return createIconBitmap(new BitmapDrawable(resources, icon), context);
        }
    }

    /**
     * Returns a bitmap suitable for the all apps view.
     */
    static Bitmap createIconBitmap(Drawable icon, Context context) {
        synchronized (sCanvas) { // we share the statics :-(
            if (sIconWidth == -1) {
                initStatics(context);
            }

            int width = sIconWidth;
            int height = sIconHeight;

            if (icon instanceof PaintDrawable) {
                PaintDrawable painter = (PaintDrawable) icon;
                painter.setIntrinsicWidth(width);
                painter.setIntrinsicHeight(height);
            } else if (icon instanceof BitmapDrawable) {
                // Ensure the bitmap has a density.
                BitmapDrawable bitmapDrawable = (BitmapDrawable) icon;
                Bitmap bitmap = bitmapDrawable.getBitmap();
                if (bitmap.getDensity() == Bitmap.DENSITY_NONE) {
                    bitmapDrawable.setTargetDensity(context.getResources().getDisplayMetrics());
                }
            }
            int sourceWidth = icon.getIntrinsicWidth();
            int sourceHeight = icon.getIntrinsicHeight();
            if (sourceWidth > 0 && sourceHeight > 0) {
                // Scale the icon proportionally to the icon dimensions
                final float ratio = (float) sourceWidth / sourceHeight;
                if (sourceWidth > sourceHeight) {
                    height = (int) (width / ratio);
                } else if (sourceHeight > sourceWidth) {
                    width = (int) (height * ratio);
                }
            }

            // no intrinsic size --> use default size
            int textureWidth = sIconTextureWidth;
            int textureHeight = sIconTextureHeight;

            final Bitmap bitmap = Bitmap.createBitmap(textureWidth, textureHeight,
                    Bitmap.Config.ARGB_8888);
            final Canvas canvas = sCanvas;
            canvas.setBitmap(bitmap);

            final int left = (textureWidth-width) / 2;
            final int top = (textureHeight-height) / 2;

            @SuppressWarnings("all") // suppress dead code warning
            final boolean debug = false;
            if (debug) {
                // draw a big box for the icon for debugging
                canvas.drawColor(sColors[sColorIndex]);
                if (++sColorIndex >= sColors.length) sColorIndex = 0;
                Paint debugPaint = new Paint();
                debugPaint.setColor(0xffcccc00);
                canvas.drawRect(left, top, left+width, top+height, debugPaint);
            }

            sOldBounds.set(icon.getBounds());
            icon.setBounds(left, top, left+width, top+height);
            icon.draw(canvas);
            icon.setBounds(sOldBounds);
            canvas.setBitmap(null);

            return bitmap;
        }
    }

    /**
     * Returns a Bitmap representing the thumbnail of the specified Bitmap.
     *
     * @param bitmap The bitmap to get a thumbnail of.
     * @param context The application's context.
     *
     * @return A thumbnail for the specified bitmap or the bitmap itself if the
     *         thumbnail could not be created.
     */
    static Bitmap resampleIconBitmap(Bitmap bitmap, Context context) {
        synchronized (sCanvas) { // we share the statics :-(
            if (sIconWidth == -1) {
                initStatics(context);
            }

            if (bitmap.getWidth() == sIconWidth && bitmap.getHeight() == sIconHeight) {
                return bitmap;
            } else {
                final Resources resources = context.getResources();
                return createIconBitmap(new BitmapDrawable(resources, bitmap), context);
            }
        }
    }

    /**
     * Given a coordinate relative to the descendant, find the coordinate in a parent view's
     * coordinates.
     *
     * @param descendant The descendant to which the passed coordinate is relative.
     * @param root The root view to make the coordinates relative to.
     * @param coord The coordinate that we want mapped.
     * @param includeRootScroll Whether or not to account for the scroll of the descendant:
     *          sometimes this is relevant as in a child's coordinates within the descendant.
     * @return The factor by which this descendant is scaled relative to this DragLayer. Caution
     *         this scale factor is assumed to be equal in X and Y, and so if at any point this
     *         assumption fails, we will need to return a pair of scale factors.
     */
    public static float getDescendantCoordRelativeToParent(View descendant, View root,
                                                           int[] coord, boolean includeRootScroll) {
        ArrayList<View> ancestorChain = new ArrayList<View>();

        float[] pt = {coord[0], coord[1]};

        View v = descendant;
        while(v != root && v != null) {
            ancestorChain.add(v);
            v = (View) v.getParent();
        }
        ancestorChain.add(root);

        float scale = 1.0f;
        int count = ancestorChain.size();
        for (int i = 0; i < count; i++) {
            View v0 = ancestorChain.get(i);
            // For TextViews, scroll has a meaning which relates to the text position
            // which is very strange... ignore the scroll.
            if (v0 != descendant || includeRootScroll) {
                pt[0] -= v0.getScrollX();
                pt[1] -= v0.getScrollY();
            }

            v0.getMatrix().mapPoints(pt);
            pt[0] += v0.getLeft();
            pt[1] += v0.getTop();
            scale *= v0.getScaleX();
        }

        coord[0] = (int) Math.round(pt[0]);
        coord[1] = (int) Math.round(pt[1]);
        return scale;
    }

    /**
     * Inverse of {@link #getDescendantCoordRelativeToSelf(View, int[])}.
     */
    public static float mapCoordInSelfToDescendent(View descendant, View root,
                                                   int[] coord) {
        ArrayList<View> ancestorChain = new ArrayList<View>();

        float[] pt = {coord[0], coord[1]};

        View v = descendant;
        while(v != root) {
            ancestorChain.add(v);
            v = (View) v.getParent();
        }
        ancestorChain.add(root);

        float scale = 1.0f;
        Matrix inverse = new Matrix();
        int count = ancestorChain.size();
        for (int i = count - 1; i >= 0; i--) {
            View ancestor = ancestorChain.get(i);
            View next = i > 0 ? ancestorChain.get(i-1) : null;

            pt[0] += ancestor.getScrollX();
            pt[1] += ancestor.getScrollY();

            if (next != null) {
                pt[0] -= next.getLeft();
                pt[1] -= next.getTop();
                next.getMatrix().invert(inverse);
                inverse.mapPoints(pt);
                scale *= next.getScaleX();
            }
        }

        coord[0] = (int) Math.round(pt[0]);
        coord[1] = (int) Math.round(pt[1]);
        return scale;
    }

    private static void initStatics(Context context) {
        final Resources resources = context.getResources();
        final DisplayMetrics metrics = resources.getDisplayMetrics();
        final float density = metrics.density;

        /* SPRD: Fix bug 326588,we use the icon_pre's size as standard.
        sIconWidth = sIconHeight = (int) resources.getDimension(R.dimen.app_icon_size);
         @{ */
        int width = ((BitmapDrawable) context.getResources().getDrawable(icons_pre)).getBitmap().getWidth();
        sIconWidth = sIconHeight = width;
        /* @} */
        sIconTextureWidth = sIconTextureHeight = sIconWidth;

        sBlurPaint.setMaskFilter(new BlurMaskFilter(5 * density, BlurMaskFilter.Blur.NORMAL));
        sGlowColorPressedPaint.setColor(0xffffc300);
        sGlowColorFocusedPaint.setColor(0xffff8e00);

        ColorMatrix cm = new ColorMatrix();
        cm.setSaturation(0.2f);
        sDisabledPaint.setColorFilter(new ColorMatrixColorFilter(cm));
        sDisabledPaint.setAlpha(0x88);
    }

    public static void setIconSize(int widthPx) {
        /* SPRD: Fix bug 326588,we use the icon_pre's size as standard.
        sIconWidth = sIconHeight = widthPx;
        sIconTextureWidth = sIconTextureHeight = widthPx;
        */
    }

    public static void scaleRect(Rect r, float scale) {
        if (scale != 1.0f) {
            r.left = (int) (r.left * scale + 0.5f);
            r.top = (int) (r.top * scale + 0.5f);
            r.right = (int) (r.right * scale + 0.5f);
            r.bottom = (int) (r.bottom * scale + 0.5f);
        }
    }

    public static void scaleRectAboutCenter(Rect r, float scale) {
        int cx = r.centerX();
        int cy = r.centerY();
        r.offset(-cx, -cy);
        Utilities.scaleRect(r, scale);
        r.offset(cx, cy);
    }

    public static void startActivityForResultSafely(
            Activity activity, Intent intent, int requestCode) {
        try {
            activity.startActivityForResult(intent, requestCode);
        } catch (ActivityNotFoundException e) {
            Toast.makeText(activity, R.string.activity_not_found, Toast.LENGTH_SHORT).show();
        } catch (SecurityException e) {
            Toast.makeText(activity, R.string.activity_not_found, Toast.LENGTH_SHORT).show();
            Log.e(TAG, "Launcher does not have the permission to launch " + intent +
                    ". Make sure to create a MAIN intent-filter for the corresponding activity " +
                    "or use the exported attribute for this activity.", e);
        }
    }

    /* SPRD: Fix bug288618, @{ */
    static void onConfigurationChanged() {
        sNeedComposeIcon = COMPOSE_SHOULD_INIT;
    }

    private static void initShouldComposeIcon(Context context) {
        Drawable preDrawable  = context.getResources().getDrawable(icons_pre);
        Bitmap preBitmap = ((BitmapDrawable) preDrawable).getBitmap();
        int preWidth = preBitmap.getWidth();
        int preHeight = preBitmap.getHeight();
        int[] preRGB = new int[preWidth * preHeight];
        preBitmap.getPixels(preRGB, 0, preWidth, 0, 0, preWidth, preHeight);
        int length = preRGB.length;
        for(int i = 0; i < length; i++) {
            if (preRGB[i] == 0) {
                sNeedComposeIcon = NEED_COMPOSE_ICON;
                return;
            }
        }
        sNeedComposeIcon = DONT_COMPOSE_ICON;
    }
    /* @} */

    /* SPRD: bug332497 2014-07-16 resize app icon to the same with resource icon_pre. @{ */
    static Bitmap createIconBitmap(Drawable icon, Context context, boolean back){
        synchronized (sCanvas) { // we share the statics :-(
            if (sIconWidth == -1) {
                initStatics(context);
            }
            if (sNeedComposeIcon == COMPOSE_SHOULD_INIT) {
                initShouldComposeIcon(context);
            }
            int width = sIconWidth;
            int height = sIconHeight;
            if (icon instanceof PaintDrawable) {
                PaintDrawable painter = (PaintDrawable) icon;
                painter.setIntrinsicWidth(width);
                painter.setIntrinsicHeight(height);
            } else if (icon instanceof BitmapDrawable) {
                // Ensure the bitmap has a density.
                BitmapDrawable bitmapDrawable = (BitmapDrawable) icon;
                Bitmap bitmap = bitmapDrawable.getBitmap();
                if (bitmap.getDensity() == Bitmap.DENSITY_NONE) {
                    bitmapDrawable.setTargetDensity(context.getResources().getDisplayMetrics());
                }
            }

            // if not support cucc , to draw background
            if (back && sNeedComposeIcon == NEED_COMPOSE_ICON) {
                int drawable_id = icons_back[new Random().nextInt(icons_back.length)];
                Drawable drawable_back  = context.getResources().getDrawable(drawable_id);
                Bitmap bitmap_pre = ((BitmapDrawable) context.getResources().getDrawable(icons_pre)).getBitmap();
                Bitmap bitmap_back = ((BitmapDrawable) drawable_back).getBitmap();
                Bitmap tempBitmap = getStandardBitmap(icon, width, height);
                icon = getCompoundedDrawable(bitmap_back, tempBitmap);
                icon = getDestBitmap(((BitmapDrawable)icon).getBitmap(), bitmap_pre);
            }

            // no intrinsic size --> use default size
            int textureWidth = sIconTextureWidth;
            int textureHeight = sIconTextureHeight;

            final Bitmap bitmap = Bitmap.createBitmap(textureWidth, textureHeight,
                    Bitmap.Config.ARGB_8888);
            final Canvas canvas = sCanvas;
            canvas.setBitmap(bitmap);

            final int left = (textureWidth-width) / 2;
            final int top = (textureHeight-height) / 2;

            sOldBounds.set(icon.getBounds());
            icon.setBounds(left, top, left + width, top + height);
            icon.draw(canvas);
            icon.setBounds(sOldBounds);
            canvas.setBitmap(null);

            return bitmap;
        }
    }
    /* SPRD: bug332497 2014-07-16 resize app icon to the same with resource icon_pre. @} */

    public static Bitmap resizeImage(Bitmap bitmap, int w, int h) {

        // load the origial Bitmap

        Bitmap BitmapOrg = bitmap;

        int width = BitmapOrg.getWidth();

        int height = BitmapOrg.getHeight();
        if (width < w && height < h) {
            return bitmap;
        }

        int newWidth = w;

        int newHeight = h;

        // calculate the scale

        float scaleWidth = ((float) newWidth) / width;

        float scaleHeight = ((float) newHeight) / height;

        // create a matrix for the manipulation

        Matrix matrix = new Matrix();

        matrix.postScale(scaleWidth, scaleHeight);

        Bitmap resizedBitmap = Bitmap.createBitmap(BitmapOrg, 0, 0, width,

        height, matrix, true);

        return resizedBitmap;

    }

    public static Drawable getCompoundedDrawable(Bitmap src1, Bitmap src2) {
        int src1W = src1.getWidth();
        int src1H = src1.getHeight();

        int src2W = src2.getWidth();
        int src2H = src2.getHeight();
        int destW = src2W;
        int destH = src2H;
        float src1X = 0;
        float src1Y = 0;
        float src2X = 0;
        float src2Y = 0;
        if (src2W < src1W) {
            src2X = (src1W - src2W) / 2;
            destW = src1W;
        }
        if (src2H < src1H) {
            src2Y = (src1H - src2H) / 2;
            destH = src1H;
        }

        if (src1W < src2W) {
            src1X = (src2W - src1W) / 2;
        }

        if (src1H < src2H) {
            src1Y = (src2H - src1H) / 2;
        }
        Bitmap mBitmap = Bitmap.createBitmap(destW, destH, Config.ARGB_8888);

        Canvas canvas = new Canvas(mBitmap);
        Paint mPaint = new Paint();
        mPaint.setAntiAlias(true);

        canvas.drawBitmap(src1, src1X, src1Y, mPaint);
        canvas.drawBitmap(src2, src2X, src2Y, mPaint);
        return new BitmapDrawable(mBitmap);
    }

    static public Drawable getDestDrawale(Drawable drawable, int width, int height){

        if(drawable == null){
            return null;
        }
        int sourceWidth = ((BitmapDrawable)drawable).getBitmap().getWidth();
        int sourceHeight = ((BitmapDrawable)drawable).getBitmap().getHeight();
        if(sourceWidth < width  || sourceHeight < height){
            return drawable;
        }
        int destX = (sourceWidth - width) / 2;
        int dextY = (sourceHeight - height) / 2;
        BitmapDrawable bitmapDrawable = (BitmapDrawable) drawable;
        Bitmap bitmap = bitmapDrawable.getBitmap();

        Bitmap destBitmap = Bitmap.createBitmap(bitmap, destX, dextY, width, height);

        return new BitmapDrawable(destBitmap);
    }

    public static Drawable getDestBitmap(Bitmap src, Bitmap pre) {
        if (src == null) {
            return null;
        }
        int srcW = src.getWidth();
        int srcH = src.getHeight();
        int preW = pre.getWidth();
        int preH = pre.getHeight();
        int rgb1[] = new int[srcW * srcH];
        int rgb2[] = new int[preW * preH];
        src.getPixels(rgb1, 0, srcW, 0, 0, srcW, srcH);
        pre.getPixels(rgb2, 0, preW, 0, 0, preW, preH);

        /* SPRD: fix bug 277351 @{ */
        for (int i = 0; i < rgb1.length; i++) {
            if (i < rgb2.length) {
                /* SPRD: fix bug 210598 @{ */
                /* SPRD: Fix bug 326588, we should have use the alpha of standard icon to make the new icon. @{ */
                if (rgb2[i] == 0) {
                    rgb1[i] = 0x00000000;
                } else {
                    int alpha = (rgb2[i] & 0xff000000);
                    if (alpha != 0xff000000 && alpha != 0x00000000) {
                        int back = (rgb1[i] & 0x00ffffff);
                        rgb1[i] = back | alpha;
                    }
                }
                /* @} */
                // rgb2[i] = rgb1[i];
                /* @} */
            } else {
                rgb1[i] = 0x000000;
            }
        }
        Bitmap dest = Bitmap.createBitmap(rgb1, srcW, srcH, Config.ARGB_8888);
        /* @} */
        return new BitmapDrawable(dest);
    }
    /* @} */

    /* SPRD : fix bug280707 @{ */
    public static int getStatusbarHeight(Context context) {
        Class<?> c = null;
        Object obj = null;
        Field field = null;
        int x = 0, sbar = 0;
        try {
            c = Class.forName("com.android.internal.R$dimen");
            obj = c.newInstance();
            field = c.getField("status_bar_height");
            x = Integer.parseInt(field.get(obj).toString());
            sbar = context.getResources().getDimensionPixelSize(x);
        } catch (NumberFormatException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (NotFoundException e) {
            e.printStackTrace();
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        } catch (InstantiationException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        }
        return sbar;
    }
    /* @} */

    /* SPRD: bug333853 2014-07-16 use MediaFile to get mime type from file path. @{ */
    public static String getMimeTypeBaseOnFileExtension(String filePath) {
        MediaFileType mft = MediaFile.getFileType(filePath);
        if (mft != null) {
            return mft.mimeType;
        }
        return "";
    }
    /* SPRD: bug333853 2014-07-16 use MediaFile to get mime type from file path. @} */

    /* SPRD: bug332497 2014-07-16 resize app icon to the same with resource icon_pre. @{ */
    private static Bitmap getStandardBitmap(Drawable icon, int width, int height) {
        if (icon == null || width == 0 || height == 0) {
            return null;
        }

        Bitmap bitmap = Bitmap.createBitmap(width, height,
                Bitmap.Config.ARGB_8888);
        Canvas canvas = sCanvas;
        canvas.setBitmap(bitmap);

        sOldBounds.set(icon.getBounds());
        icon.setBounds(0, 0, width, height);
        icon.draw(canvas);
        icon.setBounds(sOldBounds);
        canvas.setBitmap(null);
        return bitmap;
    }
    /* SPRD: bug332497 2014-07-16 resize app icon to the same with resource icon_pre. @} */
}
