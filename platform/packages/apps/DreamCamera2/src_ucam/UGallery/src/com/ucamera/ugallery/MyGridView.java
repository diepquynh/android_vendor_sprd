/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;

public class MyGridView extends GridView {

    private WindowManager mWindowManager;
    private WindowManager.LayoutParams mWindowParams;
    private LinearLayout mDragItem;
    private ImageView mDragView;
    private Bitmap mDragBitmap;
    private int mItemOffset_X;
    private int mItemOffset_Y;
    private int mItemHeightNormal = 0;

    public MyGridView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public MyGridView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        mWindowManager = (WindowManager) getContext().getSystemService("window");
    }

    public void upItem(){
        stopDragging();
        unExpandViews();
        mWindowParams = null;
        /*
         * FIX BUG: 6061
         * BUG COMENT: we need to update the current item press state,because the up event has been intercept in onTouch function
         * DATE: 2014-03-05
         */
        if(mDragItem != null && mDragItem.getChildCount() > 0) {
            mDragItem.getChildAt(0).setPressed(false);
        }
    }

    public void moveItem(MotionEvent event){
        if(mDragItem != null){
            dragView((int)event.getX(), (int)event.getY());
            doExpansion();
        }
    }
    public void setSelectedItem(View v){
        mDragItem = (LinearLayout)v;
        mItemOffset_X = (int) (mDragItem.getWidth()*0.5);
        mItemOffset_Y = (int) (mDragItem.getHeight()*0.25);
        mItemHeightNormal = mDragItem.getHeight();
    }

    public void downItem(MotionEvent event) {
        if(mDragItem != null){
            Bitmap bitmap = Bitmap.createBitmap(mDragItem.getDrawingCache());
            startDragging(bitmap, (int)event.getX(),(int)event.getY());
        }
    }

    private void dragView(int x, int y) {
        float alpha = 1.0f;
        if(mDragItem == null){
            return;
        }
        Bitmap bitmap = null;
        if(mWindowParams == null){
            mDragItem.setDrawingCacheEnabled(true);
            bitmap = Bitmap.createBitmap(mDragItem.getDrawingCache());
            startDragging(bitmap, x, y);
        }
        mWindowParams.alpha = alpha;
        mWindowParams.y = y - mItemOffset_Y;
        mWindowParams.x = x - mItemOffset_X;
        mWindowParams.flags = WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;

        if(mDragView == null){
            mDragView = new ImageView(getContext());
            mDragView.setImageBitmap(bitmap);
        }
        /*
         * FIX BUG : 5959
         * BUG COMMENT : avoid null point exception
         * DATE : 2014-02-24
         */
        if(mDragView != null && mWindowManager != null){
            mWindowManager.updateViewLayout(mDragView, mWindowParams);
        }
    }

    private void startDragging(Bitmap bm, int x,int y) {
        stopDragging();

        mWindowParams = new WindowManager.LayoutParams();
        mWindowParams.gravity = Gravity.TOP|Gravity.LEFT;
        mWindowParams.x = x - mItemOffset_X;
        mWindowParams.y = y - mItemOffset_Y;

        mWindowParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
        mWindowParams.width = WindowManager.LayoutParams.WRAP_CONTENT;
        mWindowParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
        mWindowParams.format = PixelFormat.TRANSLUCENT;
        mWindowParams.windowAnimations = 0;

        ImageView v = new ImageView(getContext());
        v.setImageBitmap(bm);
        mDragBitmap = bm;

        mWindowManager.addView(v, mWindowParams);
        mDragView = v;
    }

    private void stopDragging() {
        if (mDragView != null) {
            WindowManager wm = (WindowManager) getContext().getSystemService("window");
            wm.removeView(mDragView);
            /*
             * FIX BUG: 6071
             * BUG COMMENT: fix the selected album can not restore at once in long press states
             * DATE: 2014-03-06
             */
            mDragView.setVisibility(View.GONE);
            mDragView.setImageDrawable(null);
            mDragView = null;
        }
        if (mDragBitmap != null) {
            mDragBitmap.recycle();
            mDragBitmap = null;
        }
    }

    private void doExpansion() {
        for (int i = 0;; i++) {
            View view = getChildAt(i);
            if (view == null) {
                break;
            }
            int height = mItemHeightNormal;
            int visibility = View.VISIBLE;
            if (view.equals(mDragItem)) {
                    visibility = View.INVISIBLE;
            }
            ViewGroup.LayoutParams params = view.getLayoutParams();
            params.height = height;
            view.setLayoutParams(params);
            view.setVisibility(visibility);
        }
    }


    private void unExpandViews() {
        for (int i = 0;; i++) {
            View v = getChildAt(i);
            if (v == null) {
                layoutChildren();
                v = getChildAt(i);
                if (v == null) {
                    break;
                }
            }
            ViewGroup.LayoutParams params = v.getLayoutParams();
            params.height = mItemHeightNormal;
            v.setLayoutParams(params);
            v.setVisibility(View.VISIBLE);
        }
    }
}
