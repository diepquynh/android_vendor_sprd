/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Bitmap.Config;
import android.graphics.Paint.Style;
import android.graphics.Path.Direction;
import android.os.Bundle;
import android.os.ConditionVariable;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.GestureDetector;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.widget.SeekBar;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Stack;
import android.graphics.Matrix;

public class MakeupControlView extends ImageViewTouchBase implements
        SeekBar.OnSeekBarChangeListener {
    private static final String TAG = "MakeupControlView";
    // CID 123624 : UrF: Unread field (FB.URF_UNREAD_FIELD)
    // private Activity mActivity;
    private Bitmap mBitmap;
    private Bitmap mEffectBitmap;
    private Handler mHandler;
    private ArrayList<FeatureInfo> mFeatureList;
    private int mMakeupMode = ImageEditConstants.MAKEUP_MODE_NONE;
    private int mBitmapW;
    private int mBitmapH;
    private int mPreviewW;
    private int mPreviewH;
    private int mInitLeft;
    private int mInitTop;
    private int[] mRadiusGroup;
    private int mRadiusIndex = 0;

    private View mMakeupRoot = null;
    private ImageView mSeekBarIcon = null;
    private TextView mSeekBarPercent = null;
    private SeekBar mSeekBarOther = null;
    private SeekBar mSeekBarDeblemish = null;
    private RectF mDisplayRect = null;

    private int[] mFace_num = new int[MakeupEngine.Max_FaceNum];
    private Rect[] mFace_rect = new Rect[MakeupEngine.Max_FaceNum];

    private Point[] mEyePoint = new Point[2];
    private Point[] mMouthPoint = new Point[1];

    private int mIntensity;

    private static final int RADIUS = 128;
    private static final int SIZE = 256;
    private PopupWindow mPopupWindow;
    private MagnifierView mMagnifierView;

    private Rect mSrcRect;
    private Point mDstPoint;

    private Stack<FeatureInfo> mFeatureStack = null;

    private boolean mIsLongPress = false;
    private GestureDetector mGestureDetector;
    private final ConditionVariable mSizeLock = new ConditionVariable();

    private float mScale = 1.0f;
    private int mPreviewLocationY = 0;

    // bitmap with just deblemish effect but not effect like softenface...
    // we'll load this bitmap when we have processed 30 deblemish effects.
    private Bitmap mDeblemishBitmap = null;

    public MakeupControlView(Context context) {
        super(context);
        mGestureDetector = new GestureDetector(context, new LongPressGesture());
        mMagnifierView = new MagnifierView(context);
        try {
            mMagnifierView.setLayerType(View.LAYER_TYPE_SOFTWARE, null);
        } catch(NoSuchMethodError error) {
            Log.w(TAG, "setLayerType method is not exists!!!");
        }
        mPopupWindow = new PopupWindow(mMagnifierView, SIZE, SIZE);
        mPopupWindow.setBackgroundDrawable(context.getResources().getDrawable(R.drawable.ic_deblemish_magnifier));
        mPopupWindow.setAnimationStyle(android.R.style.Animation_Toast);

        mSrcRect = new Rect(0, 0, 2 * RADIUS, 2 * RADIUS);
        mDstPoint = new Point(0, 0);
    }

    public void initControl(Bitmap bitmap, Activity activity, Handler handler, ImageEditDesc imageEditDesc,
            int previewW, int previewH) {

        mDeblemishBitmap = bitmap.copy(Config.ARGB_8888, true);

        // CID 123624 : UrF: Unread field (FB.URF_UNREAD_FIELD)
        // mActivity = activity;
//        mBitmap = bitmap;
        setBitmap(bitmap);
        mHandler = handler;
        setPreviewDimension(previewW, previewH);

        mMakeupRoot = activity.findViewById(R.id.makeup_seekbar_layout);
        mSeekBarIcon = (ImageView) activity.findViewById(R.id.makeup_seekbar_icon);
        mSeekBarPercent = (TextView) activity.findViewById(R.id.makeup_seekbar_percent);
        mSeekBarOther = (SeekBar) activity.findViewById(R.id.makeup_seekbar_other);
        mSeekBarDeblemish = (SeekBar) activity.findViewById(R.id.makeup_seekbar_deblemish);

        mSeekBarOther.setOnSeekBarChangeListener(this);
        mSeekBarDeblemish.setOnSeekBarChangeListener(this);

        mFeatureStack = new Stack<FeatureInfo>();
    }

    // SPRD: marginTop is used to calculate clicked height of bitmap.
    public void initPreviewLocationY(int previewLocationY) {
        mPreviewLocationY = previewLocationY;
    }

    private void setScale() {
        if (mBitmapW == 0 || mPreviewW == 0) return;

        float scaleX = (float) mPreviewW / (float) mBitmapW;
        float scaleY = (float) mPreviewH / (float) mBitmapH;
        mScale = scaleX < scaleY ? scaleX : scaleY;

        // bitmap tiles the preview interface
        mInitLeft = scaleX < scaleY ? 0 : (int) ((mPreviewW - mBitmapW * scaleY) / 2);
        mInitTop = scaleY < scaleX ? 0 : (int) ((mPreviewH - mBitmapH * scaleX) / 2);
    }

    private void setBitmap(Bitmap bitmap) {
        if (bitmap == null) return;
        Config config = bitmap.getConfig();
        if(config != null && config != Config.ARGB_8888) {
            bitmap = bitmap.copy(Config.ARGB_8888, true);
        }
        mBitmap = bitmap;
        mBitmapW = mBitmap.getWidth();
        mBitmapH = mBitmap.getHeight();

        setScale();
    }

    public void setPreviewDimension(int w, int h) {
        if (w > 0 && h >0) {
            mPreviewW = w;
            mPreviewH = h;

            setScale();

            mSizeLock.open();
        } else {
            mSizeLock.close();
            mHandler.sendEmptyMessage(ImageEditConstants.MAKEUP_GET_PREVIEW_SIZE);
        }
    }

    // used to find the clicked bitmapX and bitmapY.
    // there's another way.
    private Matrix mMat = new Matrix();	;

    public void detectFaceRectView(final Bitmap bitmap, final boolean showMsg) {
        new Thread(){
            @Override
            public void run() {
                synchronized (this) {
                    if(showMsg) {
                        mHandler.sendEmptyMessage(ImageEditConstants.HANDLER_MSG_SHOW_DLG);
                    }
                    if ( !(mPreviewW >0 && mPreviewH >0) ) {
                        mSizeLock.block();
                    }

                    setBitmap(bitmap);
                    if(mFeatureStack == null) {
                        mFeatureStack = new Stack<FeatureInfo>();
                    }
                    mDisplayRect = ImageEditOperationUtil.resizeRectF(mBitmap, mPreviewW, mPreviewH);

                    RectF rectCanv = new RectF(0, 0, mBitmap.getWidth(), mBitmap.getHeight());
                    RectF rectView = new RectF(0, 0, mPreviewW, mPreviewH);
                    mMat.setRectToRect(rectCanv, rectView, Matrix.ScaleToFit.CENTER);

                    int left = (mPreviewW - mBitmapW) / 2;
                    int top = (mPreviewH - mBitmapH) / 2;
                    MakeupEngine.LoadImage(mBitmap, mFace_num, mFace_rect, mEyePoint, mMouthPoint);
                    Message msg = new Message();
                    Bundle bundle = new Bundle();
                    if(mFace_num[0] > 0) {
                        mFace_rect[0].offset(left, top);
//                        bundle.putParcelableArray(ImageEditConstants.MAKEUP_EXTRA_FACE_RECT, mFace_rect);
                        bundle.putIntArray(ImageEditConstants.MAKEUP_EXTRA_FACE_RECT, new int[]{
                                mFace_rect[0].left, mFace_rect[0].top, mFace_rect[0].right, mFace_rect[0].bottom
                        });
                        bundle.putIntArray(ImageEditConstants.MAKEUP_EXTRA_EYE_POINTS, new int[] {
                                mEyePoint[0].x, mEyePoint[0].y, mEyePoint[1].x, mEyePoint[1].y
                        });
                        bundle.putIntArray(ImageEditConstants.MAKEUP_EXTRA_MOUTH_POINT,new int[] {mMouthPoint[0].x, mMouthPoint[0].y});
//                        bundle.putParcelableArray(ImageEditConstants.MAKEUP_EXTRA_EYE_POINTS, mEyePoint);
//                        bundle.putParcelableArray(ImageEditConstants.MAKEUP_EXTRA_MOUTH_POINT, mMouthPoint);
                    }
                    bundle.putIntArray(ImageEditConstants.MAKEUP_EXTRA_FACE_NUM, mFace_num);
                    msg.what = ImageEditConstants.MAKEUP_FACE_DETECT;
                    msg.obj = bundle;
                    mHandler.sendMessage(msg);
                }
            }

        }.start();
    }

    public void setMakeupMode(int makeupMode) {
        mMakeupMode = makeupMode;
        FeatureInfo info = getLastMakeupItem();
        if(info == null) {
            return;
        }

        int iconId = 0;
        int visibility = View.GONE;
        int percent = 0;
        switch(makeupMode) {
            case ImageEditConstants.MAKEUP_MODE_NONE:
                visibility = View.GONE;
                break;
            case ImageEditConstants.MAKEUP_MODE_DEBLEMISH:
                visibility = View.VISIBLE;
                mSeekBarDeblemish.setVisibility(View.VISIBLE);
                mSeekBarOther.setVisibility(View.GONE);
                mSeekBarOther.setEnabled(false);
                mSeekBarDeblemish.setEnabled(true);
                mSeekBarDeblemish.setProgress(info.getDeblemish());
                mSeekBarIcon.setImageResource(R.drawable.ic_makeup_deblemish_small);
                mSeekBarPercent.setText(String.format("%d", info.getDeblemish() + 1));
                break;
            case ImageEditConstants.MAKEUP_MODE_WHITENFACE:
                visibility = View.VISIBLE;
                iconId = R.drawable.ic_makeup_whiteface_small;
                percent = info.getWhitenFace();
                break;
            case ImageEditConstants.MAKEUP_MODE_SOFTENFACE:
                visibility = View.VISIBLE;
                iconId = R.drawable.ic_makeup_softenface_small;
                percent = info.getSoftenFace();
                break;
            case ImageEditConstants.MAKEUP_MODE_TRIMFACE:
                visibility = View.VISIBLE;
                iconId = R.drawable.ic_makeup_trimface_small;
                percent = info.getTrimFace();
                break;
            case ImageEditConstants.MAKEUP_MODE_BIGEYE:
                visibility = View.VISIBLE;
                iconId = R.drawable.ic_makeup_bigeye_small;
                percent = info.getBigEye();
                break;
            default:
                break;
        }

        mMakeupRoot.setVisibility(visibility);
        if(iconId != 0) {
            mSeekBarOther.setVisibility(View.VISIBLE);
            mSeekBarDeblemish.setVisibility(View.GONE);
            mSeekBarOther.setEnabled(true);
            mSeekBarDeblemish.setEnabled(false);
            mSeekBarOther.setProgress(percent);
            mSeekBarIcon.setImageResource(iconId);
            mSeekBarPercent.setText(String.format("%d", percent));
        }
    }

    public int SetRadiusIndex(int index) {
        mRadiusIndex = index;
        if(mRadiusGroup != null) {
            return mRadiusGroup[index];
        }
        return 6;
    }

    public void setRadius(int[] radiusGroup) {
        mRadiusGroup = radiusGroup;

        if(mFeatureList == null) {
            initFeatureInfo();
        }
    }

    public void adjustControlVisibility(boolean bVisible) {
        int nVisible = bVisible ? View.VISIBLE : View.GONE;
        mMakeupRoot.setVisibility(nVisible);
        if (!bVisible) {
            mMakeupMode = ImageEditConstants.MAKEUP_MODE_NONE;
        }
    }

    public void setImageBitmap(Bitmap bitmap, Bitmap origBitmap) {
        super.setImageBitmap(bitmap, 0);
        mBitmap = origBitmap;
        /*
         * FIX BUG: 4729
         * BUG COMMENT : recycle old bitmap before set new one
         * DATE : 2013-08-28
         */
        if(mEffectBitmap != null && !mEffectBitmap.isRecycled()) {
            mEffectBitmap.recycle();
            mEffectBitmap = null;
        }
        mEffectBitmap = bitmap;
    }

    public void setImageBitmap(Bitmap bitmap) {
        if (bitmap != null && !bitmap.isRecycled()) {
            super.setImageBitmap(bitmap, 0);
            mBitmap = bitmap;
        }
    }

    public void resetEffectBitmap(Bitmap bitmap) {
        if(mEffectBitmap != null && !mEffectBitmap.isRecycled()) {
            mEffectBitmap.recycle();
            mEffectBitmap = null;
        }
        mEffectBitmap = bitmap;
    }

    public Bitmap getEffectBitmap() {
        return mEffectBitmap;
    }

    // the bitmap is loaded by makeupengine, then makeup starts.
    // it should be the same as original bitmap at the beginning, then may change.
    public Bitmap getEngineLoadBitmap() {
        return mDeblemishBitmap;
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        mIntensity = progress;
        if(mMakeupMode != ImageEditConstants.MAKEUP_MODE_DEBLEMISH) {
            mSeekBarPercent.setText(String.format("%d", mIntensity));
        } else {
            mRadiusIndex = mIntensity;
            mSeekBarPercent.setText(String.format("%d", mIntensity + 1));
        }
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {

    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        processOtherEffect(true, null, mMakeupMode);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if(mMakeupMode != ImageEditConstants.MAKEUP_MODE_DEBLEMISH) {
            return true;
        }
        if(mGestureDetector.onTouchEvent(event)){
            return true;
        }
        if(!mIsLongPress) return true;
        int action = event.getAction();
        if (action == MotionEvent.ACTION_MOVE) {
            int x = (int) (event.getRawX() - mInitLeft);
            int y = (int) (event.getRawY() - mInitTop - mPreviewLocationY);

            int bitmapX = (int) (x / mScale);
            int bitmapY = (int) (y / mScale);

            mSrcRect.offsetTo(bitmapX - RADIUS, bitmapY - RADIUS);
            mDstPoint.set(x - RADIUS, y - 2 * RADIUS);
            /* SPRD: CID 108932 : Logically dead code (DEADCODE) @{
            if (action == MotionEvent.ACTION_DOWN) {
                removeCallbacks(mShowZoom);
                post(mShowZoom);
            } else
            @} */
            if (!mPopupWindow.isShowing()) {
                mShowZoom.run();
            }
            mPopupWindow.update(mInitLeft + mDstPoint.x, mInitTop + mDstPoint.y, -1, -1);
            mMagnifierView.invalidate();
        } else if (action == MotionEvent.ACTION_UP) {
            if(mPopupWindow.isShowing()) {
                mIsLongPress = false;
                removeCallbacks(mShowZoom);
                mPopupWindow.dismiss();

                if(!mDisplayRect.contains(event.getRawX(), event.getRawY())) {
                    return true;
                }
                if (event.getPointerCount() == 1
                        && mMakeupMode == ImageEditConstants.MAKEUP_MODE_DEBLEMISH) {
                    /* this is the another way to calculate bitmapX and bitmapY @{
                    int bitmapX = (int) ((event.getRawX() - mInitLeft) / mScale);
                    int bitmapY = (int) ((event.getRawY() - mInitTop - mPreviewLocationY) / mScale);
                    @} */
                    float[] point = new float[4];
                    point[0] = event.getRawX() - mRadiusGroup[mRadiusIndex]/4.0f;
                    point[1] = (event.getRawY() - mPreviewLocationY) - mRadiusGroup[mRadiusIndex]/4.0f;
                    point[2] = event.getRawX() + mRadiusGroup[mRadiusIndex]/4.0f;
                    point[3] = (event.getRawY() - mPreviewLocationY) + mRadiusGroup[mRadiusIndex]/4.0f;

                    Matrix invert = new Matrix();
                    mMat.invert(invert);
                    invert.mapPoints(point);

                    int bitmapX = (int) ((point[0] + point[2]) / 2);
                    int bitmapY = (int) ((point[1] + point[3]) / 2);

                    processDeblemishEffect(true, null, mMakeupMode, bitmapX, bitmapY);
                }
            }
        }
        invalidate();
        return true;

    }

    private Runnable mShowZoom = new Runnable() {
        public void run() {
            /**
             * BUG : 4694
             * BUGCAUSE:  【crash】Null pointer exception.
             * DATE: 2013-08-13
             */
            if(mPopupWindow != null) {
                mPopupWindow.showAtLocation(mMagnifierView, Gravity.NO_GRAVITY, mInitLeft + mDstPoint.x, mInitTop + mDstPoint.y);
            }
        }
    };

    private class LongPressGesture extends GestureDetector.SimpleOnGestureListener {

        @Override
        public void onLongPress(MotionEvent event) {
            mIsLongPress = true;
            int x = (int) (event.getRawX() - mInitLeft);
            int y = (int) (event.getRawY() - mInitTop - mPreviewLocationY);

            int bitmapX = (int) (x / mScale);
            int bitmapY = (int) (y / mScale);
            mSrcRect.offsetTo(bitmapX - RADIUS, bitmapY - RADIUS);

            mDstPoint.set(x - RADIUS, y - 2 * RADIUS);
            removeCallbacks(mShowZoom);
            post(mShowZoom);
            if (!mPopupWindow.isShowing()) {
                mShowZoom.run();
            }
            mPopupWindow.update(mInitLeft + mDstPoint.x, mInitTop + mDstPoint.y, -1, -1);
            mMagnifierView.invalidate();
            super.onLongPress(event);
        }
    }

    /*
     * SPRD: deleted for two reasons
     * 1. not used
     * 2. conflicted with the current deblemish logic @{
     * /
    public void undoMakeup() {
        FeatureInfo info = getLastMakeupItem();
        if(info != null) {
            synchronized(mFeatureList) {
                mFeatureList.remove(info);
            }
            mFeatureStack.push(info);
            FeatureInfo curInfo = getLastMakeupItem();
            if(curInfo != null) {
                MakeupEngine.SetImageParam(curInfo);
                processEffect(curInfo);
            }
        }
    }

    public void redoMakeup() {
        if(!mFeatureStack.isEmpty()) {
           FeatureInfo info = mFeatureStack.pop();
           synchronized(mFeatureList) {
               mFeatureList.add(info);
           }
            MakeupEngine.SetImageParam(info);
            processEffect(info);
       }
    }
    @} */

    private FeatureInfo getLastMakeupItem() {
        FeatureInfo info = null;
        if(mFeatureList == null || mFeatureList.isEmpty()) {
            info = null;
        } else {
            info = mFeatureList.get(mFeatureList.size() - 1);
        }

        return info;
    }

    public int getMakeupCount() {
        if(mFeatureList == null) {
            return 0;
        }

        return mFeatureList.size() - 1;
    }

    public int getMakeupStackCount() {
        if(mFeatureStack == null) {
            return 0;
        }

        return mFeatureStack.size();
    }

    public boolean hasMakeupItemInStack() {
        if(mFeatureStack == null) {
            return false;
        }

        return !mFeatureStack.isEmpty();
    }

    public void clearMakeupStack() {
        if(mFeatureStack != null) {
            mFeatureStack.clear();
        }
    }

    private void processEffect(FeatureInfo info) {
        if (info != null) {
            int makeupMode = info.GetMod();
            int makeupIntensity = info.GetIntensity();

            setMakeupMode(makeupMode);

            if (makeupMode == ImageEditConstants.MAKEUP_MODE_DEBLEMISH) {
                mRadiusIndex = makeupIntensity;
                processDeblemishEffect(false, info, makeupMode, 0, 0);
            } else {
                processOtherEffect(false, info, makeupMode);
            }
        }
    }

    /**
     * process the deblemish effect;
     * @param isNormal if false redo or undo, else true
     */
    private void processDeblemishEffect(final boolean isNormal, final FeatureInfo info, final int makeupMode, final float pointerX, final float pointerY) {
        if(mFeatureList == null) return;
        new Thread() {
            @Override
            public void run() {

                try {
                    this.sleep(150);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                FeatureInfo item = null;
                Message msg = new Message();
                if(isNormal) {
                    FeatureInfo oldItem = getLastMakeupItem();
                    if(oldItem != null) {
                        item = oldItem.clone();
                    } else {
                        item = new FeatureInfo(ImageEditConstants.MAKEUP_MODE_NONE);
                    }
                    item.setMode(makeupMode);
                    switch(makeupMode) {
                        case ImageEditConstants.MAKEUP_MODE_SOFTENFACE:
                            item.setSoftenFace(mIntensity);
                            break;
                        case ImageEditConstants.MAKEUP_MODE_WHITENFACE:
                            item.setWhitenFace(mIntensity);
                            break;
                        case ImageEditConstants.MAKEUP_MODE_DEBLEMISH:
                            // Convert float to upper int, then add extra one, if not add, the circle
                            // set into libmakeupengine.so will be smaller than the the green circle
                            // in MagnifierView in situation of larger level of deblemish like 4, that
                            // means the current deblemish scope is small.
                            item.SetDeblemishRadius(item.GetDeblemishNum(), mRadiusGroup[mRadiusIndex]);
                            item.setDeblemish(mRadiusIndex);
                            if (item.setArea((int) pointerX, (int) pointerY) == false) {
                                /* we wont give tips, while, reset and rebegin @{
                                msg.what = ImageEditConstants.MAKEUP_PROCESS_RESULT_WARN_1;
                                msg.obj = mActivity.getResources().getString(R.string.text_edit_makeup_too_many_point);
                                mHandler.sendMessage(msg);
                                return ;
                                @} */
                                updateDelemish(item);
                            }
                            break;
                        case ImageEditConstants.MAKEUP_MODE_TRIMFACE:
                            item.setTrimFace(mIntensity);
                            break;
                        case ImageEditConstants.MAKEUP_MODE_BIGEYE:
                            item.setBigEye(mIntensity);
                            break;
                    }
                    synchronized(mFeatureList) {
                        mFeatureList.add(item);
                    }
                    msg.arg1 = ImageEditConstants.MAKEUP_MSG_REDO_ENABLE;
                } else {
                    item = info;
                }
                mHandler.sendEmptyMessage(ImageEditConstants.HANDLER_MSG_SHOW_DLG);
//                Bitmap newBitmap = ImageEditOperationUtil.createBitmapFromConfig(mBitmap, Config.ARGB_8888);
                /**
                 * BUG : 4694
                 * BUGCAUSE:  【crash】Null pointer exception.
                 * DATE: 2013-08-13
                 */
                if(mBitmap != null) {
                    Bitmap newBitmap = null;
                    try{
                        newBitmap = mBitmap.copy(Config.ARGB_8888, true);
                    }catch (OutOfMemoryError e) {
                        System.gc();
                        Log.e(TAG, "processDeblemishEffect : OOM exception");
                        return;
                    }
                    long startTime = System.currentTimeMillis();
                    if (makeupMode == ImageEditConstants.MAKEUP_MODE_DEBLEMISH) {
                        MakeupEngine.ManageImgae2(newBitmap, item);
                        for (int radius = mRadiusGroup[mRadiusIndex] - 1; radius > 0; radius = radius >> 1) {
                            item.SetDeblemishRadius(item.GetDeblemishNum(), radius);
                            item.setDeblemish(mRadiusIndex);
                            if (!item.setArea((int) pointerX, (int) pointerY)) {
                                updateDelemish(item);
                            }
                            MakeupEngine.ManageImgae2(newBitmap, item);
                        }
                    } else {
                        MakeupEngine.ManageImgae2(newBitmap, item);
                    }
                    long endTime = System.currentTimeMillis();
                    Log.i(TAG, "processDeblemishEffect cost: " + (endTime - startTime));

                    msg.what = ImageEditConstants.MAKEUP_PROCESS_RESULT;
                    msg.obj = newBitmap;
                    mHandler.sendMessage(msg);
                    /* SPRD: draw the circle set into libmakeupengine.so @{
                    Canvas canvas = new Canvas(newBitmap);
                    Paint paint = new Paint();
                    paint.setAntiAlias(true);
                    paint.setColor(Color.RED);
                    paint.setStyle(Style.STROKE);
                    int index = mRadiusGroup[mRadiusIndex];
                    paint.setStrokeWidth((int)(3 / mScale / 2));
                    canvas.drawCircle(pointerX, pointerY, index, paint);
                    canvas.save();
                    @} */
                }
            }
        }.start();
    }

    private void updateDelemish(FeatureInfo item) {
        if (item == null || item.GetMod()
                != ImageEditConstants.MAKEUP_MODE_DEBLEMISH || mDeblemishBitmap == null) {
            return;
        }
        Log.d(TAG, "updateDelemish");

        /* use LoadImage() and ManageImgae2(), update mDeblemishBitmap @{ */
        FeatureInfo itemWithJustDeblemish = item.clone();
        itemWithJustDeblemish.ReSetAllButDeblemish();
        MakeupEngine.LoadImage(mDeblemishBitmap, mFace_num, mFace_rect, mEyePoint, mMouthPoint);
        MakeupEngine.ManageImgae2(mDeblemishBitmap, itemWithJustDeblemish);
        /* @} */

        /* mDeblemishBitmap as the new beginning, restore origin effect @{ */
        MakeupEngine.LoadImage(mDeblemishBitmap, mFace_num, mFace_rect, mEyePoint, mMouthPoint);

        Bitmap newBitmap = mBitmap.copy(Config.ARGB_8888, true);
        item.setMode(ImageEditConstants.MAKEUP_MODE_SOFTENFACE);
        MakeupEngine.ManageImgae2(newBitmap, item);

        item.setMode(ImageEditConstants.MAKEUP_MODE_WHITENFACE);
        MakeupEngine.ManageImgae2(newBitmap, item);

        item.setMode(ImageEditConstants.MAKEUP_MODE_TRIMFACE);
        MakeupEngine.ManageImgae2(newBitmap, item);

        item.setMode(ImageEditConstants.MAKEUP_MODE_BIGEYE);
        MakeupEngine.ManageImgae2(newBitmap, item);
        newBitmap.recycle();
        /* @} */

        /* libmakeupengine.so supports just 30 makeup point currently,
         * reset here @{
         */
        item.setMode(ImageEditConstants.MAKEUP_MODE_DEBLEMISH);
        item.ReSetDeblemishNum();
        if(mFeatureList != null) {
            synchronized(mFeatureList) {
                mFeatureList.clear();
            }
            mFeatureList = new ArrayList<FeatureInfo>();
        /* SPRD: CID 123590 : Dereference after null check (FORWARD_NULL) @{ */
            mFeatureList.add(item);
        }
        /**
        }
        mFeatureList.add(item);
        */
        /* @} */
        /* @} */
    }

    /**
     * process the other(whitenface/softenface/trimface/bigeye) effect;
     * @param isNormal if false redo or undo, else true
     */
    private void processOtherEffect(final boolean isNormal, final FeatureInfo info, final int makeupMode) {
        Log.d(TAG, "processOtherEffect(): isNormal is " + isNormal + ", makeupMode is " + makeupMode + ", mFeatureList is " + mFeatureList);
        if(mFeatureList == null) return;
        if (makeupMode != ImageEditConstants.MAKEUP_MODE_DEBLEMISH) {
            new Thread() {
                @Override
                public void run() {
                    FeatureInfo item = null;
                    Message msg = new Message();
                    if(isNormal) {
                        FeatureInfo oldItem = getLastMakeupItem();
                        if(oldItem != null) {
                            item = oldItem.clone();
                        } else {
                            item = new FeatureInfo(ImageEditConstants.MAKEUP_MODE_NONE);
                        }
                        item.setMode(makeupMode);
                        switch(makeupMode) {
                            case ImageEditConstants.MAKEUP_MODE_SOFTENFACE:
                                item.setSoftenFace(mIntensity);
                                break;
                            case ImageEditConstants.MAKEUP_MODE_WHITENFACE:
                                item.setWhitenFace(mIntensity);
                                break;
                            case ImageEditConstants.MAKEUP_MODE_DEBLEMISH:

                                break;
                            case ImageEditConstants.MAKEUP_MODE_TRIMFACE:
                                item.setTrimFace(mIntensity);
                                break;
                            case ImageEditConstants.MAKEUP_MODE_BIGEYE:
                                item.setBigEye(mIntensity);
                                break;
                        }
                        synchronized(mFeatureList) {
                            mFeatureList.add(item);
                        }
                        msg.arg1 = ImageEditConstants.MAKEUP_MSG_REDO_ENABLE;
                    } else {
                        item = info;
                    }
//                    Bitmap newBitmap = ImageEditOperationUtil.createBitmapFromConfig(mBitmap, Config.ARGB_8888);
                    /**
                     * BUG : 4694
                     * BUGCAUSE:  【crash】Null pointer exception.
                     * DATE: 2013-08-13
                     */
                    if(mBitmap != null ) {
                        Bitmap newBitmap = null;
                        try{
                            newBitmap = mBitmap.copy(Config.ARGB_8888, true);
                        }catch (OutOfMemoryError e) {
                            System.gc();
                            Log.e(TAG, "processOtherEffect: OOM exception");
                            return;
                        }
                        MakeupEngine.ManageImgae2(newBitmap, item);
                        msg.what = ImageEditConstants.MAKEUP_PROCESS_RESULT;
                        msg.obj = newBitmap;
                        mHandler.sendMessage(msg);
                    }
                }
            }.start();
        }
    }

    private void initFeatureInfo() {
        if(mFeatureList == null) {
            mFeatureList = new ArrayList<FeatureInfo>();
            FeatureInfo info = new FeatureInfo(ImageEditConstants.MAKEUP_MODE_DEBLEMISH);
            info.setSoftenFace(0);
            info.setWhitenFace(0);
            info.ReSetDeblemishNum();
            info.setIntensity(SetRadiusIndex(mRadiusIndex));
            info.setDeblemish(mRadiusIndex);
            info.setTrimFace(0);
            info.setBigEye(0);

            synchronized(mFeatureList) {
                mFeatureList.add(info);
            }
        }
    }

    public void reset() {
        if(mFeatureList != null) {
            synchronized(mFeatureList) {
                mFeatureList.clear();
            }
            mFeatureList = null;
        }
        mIntensity = 0;
    }

    public void resetPopupWindow() {
        mIsLongPress = false;
        removeCallbacks(mShowZoom);
        if(mPopupWindow != null) {
            mPopupWindow.dismiss();
        }
    }

    public void recycledBitmap() {
        if(mBitmap != null) {
            mBitmap.recycle();
            mBitmap = null;
        }
        if(mEffectBitmap != null) {
            mEffectBitmap.recycle();
            mEffectBitmap = null;
        }

        if (mDeblemishBitmap != null) {
            mDeblemishBitmap.recycle();
            mDeblemishBitmap = null;
        }
    }

    class MagnifierView extends View {
        private Paint mPaint;
        private Paint mCenterPaint;
        private Rect rect;
        private Path clip;

        public MagnifierView(Context context) {
            super(context);
            mPaint = new Paint();
            mPaint.setAntiAlias(true);
            mPaint.setColor(0xffFFFF00);
            mPaint.setStyle(Style.STROKE);

            mCenterPaint = new Paint();
            mCenterPaint.setAntiAlias(true);
            mCenterPaint.setColor(Color.GREEN);
            mCenterPaint.setStyle(Style.STROKE);
            mCenterPaint.setStrokeWidth(2);

            rect = new Rect(0, 0, RADIUS * 4, RADIUS * 4);
            rect.offset(-RADIUS, -RADIUS);
            clip = new Path();
            clip.addCircle(RADIUS, RADIUS, RADIUS - 15, Direction.CW);
            clip.close();
        }

        @Override
        protected void onDraw(Canvas canvas) {
            canvas.save();
            canvas.clipPath(clip);
            // draw popup
            mPaint.setAlpha(255);
            if(mEffectBitmap != null && !mEffectBitmap.isRecycled()) {
                canvas.drawBitmap(mEffectBitmap, mSrcRect, rect, mPaint);
            } else {
                canvas.drawBitmap(mBitmap, mSrcRect, rect, mPaint);
            }
            //draw center circle
            canvas.drawCircle(rect.centerX(), rect.centerY(), mRadiusGroup[mRadiusIndex] * 2, mCenterPaint);
            canvas.restore();
        }
    }
}
