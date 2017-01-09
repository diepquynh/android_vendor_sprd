/*
 * Copyright (C) 2011,2013 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit.cate;

import com.ucamera.ucam.modules.ugif.edit.BackGroundWorkTask;
import com.ucamera.ucam.modules.ugif.edit.GifEditDatas;
import com.ucamera.ucam.modules.ugif.edit.callback.ProcessCallback;
import com.ucamera.ucam.modules.ugif.edit.cate.GifEditCateAdapter.ViewHolder;
import com.ucamera.ucam.modules.ugif.edit.cate.GifEditTypeAdapter.ListenerId;
import com.android.camera.settings.Keys;
import com.android.camera.settings.SettingsManager;
import com.ucamera.ucam.modules.utils.UiUtils;
import com.ucamera.ucam.modules.utils.Utils;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffXfermode;
import android.graphics.Xfermode;
import android.graphics.Bitmap.Config;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import com.android.camera.app.CameraApp;

import com.android.camera2.R;

import java.lang.reflect.Method;
import com.android.camera.app.CameraServicesImpl;

public class BasicEditCate extends GifBasicCate {
    private final static String TAG = "BasicEditCate";
    private final static int FLIP_VERTICAL = 1;
    private final static int FLIP_HORIZONTAL = -1;
    private final static int ROTATE_CLOCKWISE = -90;
    private final static int ROTATE_UNCLOCKWISE = 90;
    private int mNestRectWidth;
    private int mNestRectHeight;
    private int mNestRectDeltaY;
    private int mNestRectDeltaX;

    protected String[] mTitleRes;

    public BasicEditCate(Context context, ProcessCallback callback) {
        super(context, callback);
        mArrayRes = Utils.getIconIds(context.getResources(), R.array.ugif_edit_basic_edit_icons);
        mTitleRes = Utils.getIconStrings(context.getResources(), R.array.ugif_edit_basic_edit_titles);
        /*
         * FIX BUG: 6192
         * BUG COMMENT: null pointer exception
         * DATE: 2014-04-11
         */

        if(UiUtils.highMemo()) {
            if(GifEditDatas.getOriBitmaps() != null) {
                mNestRectWidth = getBitmapWidth(GifEditDatas.getOriBitmaps()[0]);
                mNestRectHeight = getBitmapHeight(GifEditDatas.getOriBitmaps()[0]);
            } else {
                CameraApp application = (CameraApp) context.getApplicationContext();
                SettingsManager settingsManager = CameraServicesImpl.instance().getSettingsManager();
                String picsize = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,Keys.KEY_GIF_MODE_PIC_SIZE,
                        context.getString(R.string.pref_gif_mode_pic_size_default));
                mNestRectWidth = mNestRectHeight = Integer.valueOf(picsize);
            }
        } else {
            if(GifEditDatas.getResultBitmaps() != null) {
                mNestRectWidth = getBitmapWidth(GifEditDatas.getResultBitmaps()[0]);
                mNestRectHeight = getBitmapHeight(GifEditDatas.getResultBitmaps()[0]);
            } else {
                CameraApp application = (CameraApp) context.getApplicationContext();
                SettingsManager settingsManager = CameraServicesImpl.instance().getSettingsManager();
                String picsize = settingsManager.getString(SettingsManager.SCOPE_GLOBAL,Keys.KEY_GIF_MODE_PIC_SIZE,
                        context.getString(R.string.pref_gif_mode_pic_size_default));
                mNestRectWidth = mNestRectHeight = Integer.valueOf(picsize);
            }
        }
        mNestRectDeltaY = mNestRectHeight / 2;
        mNestRectDeltaX = mNestRectWidth / 2;
        /*
         * FIX BUG: 5363 6158
         * BUG COMMENT: null pointer exception
         * DATE: 2013-11-28 2014-03-26
         */
        if (mHashMap != null) {
            for(Method m: BasicEditCate.this.getClass().getDeclaredMethods()){
                ListenerId li = null;
                if(m != null) {
                    li = m.getAnnotation(ListenerId.class);
                }
                if (li != null){
                    mHashMap.put(li.Id(), m);
                }
            }
        }
    }

    /*
     * FIX BUG: 4987
     * FIX COMMENT: avoid null point exception
     * DATE: 2013-10-11
     */
    private int getBitmapWidth(Bitmap bitmap) {
        if(bitmap != null) {
            return bitmap.getWidth();
        }
        return 0;
    }

    private int getBitmapHeight(Bitmap bitmap) {
        if(bitmap != null) {
            return bitmap.getHeight();
        }
        return 0;
    }

    class ViewHolder {
        ImageView iconView;
        TextView textView;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder = null;
        if(convertView != null) {
            holder = (ViewHolder) convertView.getTag();
        } else {
            holder = new ViewHolder();
            convertView = mInflater.inflate(R.layout.ugif_edit_cate_item, parent, false);
            holder.iconView = (ImageView) convertView.findViewById(R.id.ugif_edit_cate_icon_id);
            holder.textView = (TextView) convertView.findViewById(R.id.ugif_edit_cate_text_id);

            convertView.setTag(holder);
        }
        holder.textView.setText(mTitleRes[position]);
        holder.iconView.setImageResource(mArrayRes.get(position));

        return convertView;
    }



    //    @ListenerId(Id = 1)
    private void handleRotateLeftAction() {
        handleRotateAction(ROTATE_CLOCKWISE);
    }
    private void handleRotateAction(int rotate) {
        final int r = rotate;
        BackGroundWorkTask.processTask(mContext, mContext.getString(R.string.ugif_edit_text_waiting),
                new Runnable() {
                    public void run() {
                        Bitmap[] bitmaps = GifEditDatas.getEditBitmaps();
                        if(bitmaps == null)
                            return;
                        int bitmapNums;
                        if(UiUtils.highMemo()) {
                            bitmapNums = GifEditDatas.getOriBitmaps().length;
                        } else {
                            bitmapNums = GifEditDatas.getResultBitmaps().length;
                        }
                        for (int i = 0; i < bitmapNums; i++) {
                            if (bitmaps[i] != null && !bitmaps[i].isRecycled()) {
                                // Warning: must not use the rotate function in utils
                                bitmaps[i] = rotate(bitmaps[i], r);
                            }
                        }
                        GifEditDatas.updateEditBitmaps(bitmaps);
                    }
                }, null);
    }

    @ListenerId(Id = 1)
    private void handleRotateRightAction() {
        handleRotateAction(ROTATE_UNCLOCKWISE);
    }

    @ListenerId(Id = 2)
    private void handleFlipHorizontalAction() {
        handleFlipAction(FLIP_HORIZONTAL);
    }

    @ListenerId(Id = 3)
    private void handleFlipVerticalAction() {
        handleFlipAction(FLIP_VERTICAL);
    }

    private void handleFlipAction(int flipOrientation) {
        final int f = flipOrientation;
        BackGroundWorkTask.processTask(mContext, mContext.getString(R.string.ugif_edit_text_waiting),
                new Runnable() {
                    public void run() {
                        Bitmap[] bitmaps = GifEditDatas.getEditBitmaps();
                        int bitmapNums;
                        if(UiUtils.highMemo()) {
                            bitmapNums = GifEditDatas.getOriBitmaps().length;
                        } else {
                            bitmapNums = GifEditDatas.getResultBitmaps().length;
                        }
                        for (int i = 0; i < bitmapNums; i++) {
                            if (bitmaps[i] != null && !bitmaps[i].isRecycled()) {
                                bitmaps[i] = operateFlipBitmap(bitmaps[i], f);
                            }
                        }
                        GifEditDatas.updateEditBitmaps(bitmaps);
                    }
                }, null);
    }

    @ListenerId(Id = 4)
    private void handleSymmetryHorizontalLeftAction() {
        handleSymmetryAction(0, 0, mNestRectDeltaX, mNestRectHeight, mNestRectDeltaX, 0,
                FLIP_HORIZONTAL);
    }

    @ListenerId(Id = 5)
    private void handleSymmetryHorizontalRightAction() {
        handleSymmetryAction(mNestRectDeltaX, 0, mNestRectDeltaX, mNestRectHeight, 0, 0,
                FLIP_HORIZONTAL);
    }

    @ListenerId(Id = 6)
    private void handleSymmetryVerticalUpAction() {
        handleSymmetryAction(0, 0, mNestRectWidth, mNestRectDeltaY, 0, mNestRectDeltaY,
                FLIP_VERTICAL);
    }

    @ListenerId(Id = 7)
    private void handleSymmetryVerticalDownAction() {
        handleSymmetryAction(0, mNestRectDeltaY, mNestRectWidth, mNestRectDeltaY, 0, 0,
                FLIP_VERTICAL);
    }

    private void handleSymmetryAction(final int nestRectDeltaX, final int nestRectDeltaY,
            final int nestRectWidth, final int nestRectHeight, final int nestRectWidth2,
            final int nestRectHeight2, final int flipOritation) {
        BackGroundWorkTask.processTask(mContext, mContext.getString(R.string.ugif_edit_text_waiting),
                new Runnable() {
                    public void run() {
                        Bitmap[] bitmaps = GifEditDatas.getEditBitmaps();
                        int bitmapNums;
                        /* SPRD:Fix bug 536776 java.lang.NullPointerException @{ */
                        if (UiUtils.highMemo()) {
                            Bitmap[] oriBitmaps = GifEditDatas.getOriBitmaps();
                            if (oriBitmaps == null) return;
                            bitmapNums = oriBitmaps.length;
                        } else {
                            Bitmap[] resultBitmaps = GifEditDatas.getResultBitmaps();
                            if (resultBitmaps == null) return;
                            bitmapNums = resultBitmaps.length;
                        }
                        /* @} */
                        for (int i = 0; i < bitmapNums; i++) {
                            if (bitmaps[i] != null && !bitmaps[i].isRecycled()) {
                                bitmaps[i] = operateSymmetryBitmap(bitmaps[i], nestRectDeltaX,
                                        nestRectDeltaY, nestRectWidth, nestRectHeight,
                                        nestRectWidth2, nestRectHeight2, flipOritation,
                                        new PorterDuffXfermode(PorterDuff.Mode.SRC_OVER));
                            }
                        }
                        GifEditDatas.updateEditBitmaps(bitmaps);
                    }
                }, null);
    }

    public static Bitmap operateSymmetryBitmap(Bitmap bitmap, int clipX, int clipY, int clipWidth,
            int clipHeight, int dstX, int dstY, float scale, Xfermode xfermode) {
        Bitmap bottomLayer = null;
        Bitmap src = bitmap;
        try {
            final Matrix m = new Matrix();
            final Paint paint = new Paint();
            m.setScale(scale, -scale);
            Bitmap nestedBitmap = Bitmap.createBitmap(src, clipX, clipY, clipWidth, clipHeight, m,
                    false);
            Config config = src.getConfig();
            bottomLayer = Bitmap.createBitmap(src.getWidth(), src.getHeight(),
                    (config != null ? config : Config.ARGB_8888));
            m.reset();
            Canvas canvas = new Canvas(bottomLayer);
            canvas.drawBitmap(src, m, paint);
            paint.reset();
            paint.setXfermode(xfermode);
            canvas.drawBitmap(nestedBitmap, dstX, dstY, paint);
            Utils.recycleBitmap(nestedBitmap);
        } catch (OutOfMemoryError ex) {
            // We have no memory to rotate. Return the original bitmap.
            Log.w(TAG, "operateSymmetryBitmap(): code has a memory leak is detected...");
            System.gc();
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.d(TAG, "operateSymmetryBitmap(): bottomLayer = " + bottomLayer);
        return bottomLayer;
    }

    public static Bitmap operateFlipBitmap(Bitmap src, float scale) {
        Bitmap b2 = null;
        final Matrix m = new Matrix();
        m.setScale(scale, -scale);
        try {
            b2 = Bitmap.createBitmap(src, 0, 0, src.getWidth(), src.getHeight(), m, true);
        } catch (OutOfMemoryError ex) {
            // We have no memory to rotate. Return the original bitmap.
            b2 = null;
            Log.w(TAG, "operateFlipBitmap(): code has a memory leak is detected...");
            System.gc();
        }
        return b2;
    }

    public static Bitmap rotate(Bitmap b, int degrees) {
        degrees = degrees % 360;
        if (degrees == 0) return b;

        Bitmap b2 = null;
        if (b != null && !b.isRecycled()) {
            Matrix m = new Matrix();
            m.setRotate(degrees, (float) b.getWidth() / 2, (float) b.getHeight() / 2);
            try {
                b2 = Bitmap.createBitmap(b, 0, 0, b.getWidth(), b.getHeight(),m, true);
                Log.d(TAG, "rotate(): b = " + b + ", b2 = " + b2);
            } catch (OutOfMemoryError ex) {
                // We have no memory to rotate. Return the original bitmap.
                b2 = null;
                Log.w(TAG, "rotate(): code has a memory leak is detected...");
                System.gc();
                //rotate(b, degrees);
            }
        }
        return b2;
    }

    @Override
    public void onItemClick(int position) {
        if (mCallback != null) {
            mCallback.beforeProcess();
        }
        try {
            Method action = mHashMap.get(position + 1);
            action.invoke(this);
        } catch (Exception e) {
            Log.e(TAG, "invoke method failed, id: " + position);
            e.printStackTrace();
        }
        if (mCallback != null) {
            mCallback.afterProcess(position);
        }
    }
}
