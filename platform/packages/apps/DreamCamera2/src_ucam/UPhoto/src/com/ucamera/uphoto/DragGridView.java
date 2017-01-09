/*
 * Copyright (C) 2010,2012 Thundersoft Corporation
 * All rights Reserved
 */

package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.GestureDetector;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.GridView;
import android.widget.ImageView;

public class DragGridView extends GridView implements GestureDetector.OnGestureListener{
    private static final String TAG = "DragGridView";
    private int dragPosition;
    private int dropPosition;
    private int dragPointX; //item x
    private int dragPointY; //item y
    private int dragOffsetX;
    private int dragOffsetY;
    private ImageView dragImageView;// item preview
    private View itemView;// item view
    private boolean isLongTouch;

    private WindowManager windowManager;
    private WindowManager.LayoutParams windowParams;

    private int mItemHeight;

    private GestureDetector gstDtct;
    public DragGridView(Context context) {
        super(context);
        init(context);
    }
    public DragGridView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    public DragGridView(Context context, AttributeSet attrs, int style) {
        super(context, attrs, style);
        init(context);
    }

    private void init(Context context) {
        dragImageView = null;
        itemView = null;
        isLongTouch = false;

        windowManager = (WindowManager)getContext().getSystemService(Context.WINDOW_SERVICE);//"window"
        windowParams = new WindowManager.LayoutParams();

        gstDtct = new GestureDetector(context, this);
    }

    private void startDrag(Bitmap bm, int x, int y) {
        stopDrag();

        windowParams.gravity = Gravity.TOP|Gravity.LEFT;
        windowParams.x = x - dragPointX + dragOffsetX;
        windowParams.y = y - dragPointY + dragOffsetY;
        windowParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
        windowParams.width = WindowManager.LayoutParams.WRAP_CONTENT;
        windowParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                                | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                                | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
                                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;

        windowParams.format = PixelFormat.TRANSLUCENT;
        windowParams.windowAnimations = 0;

        ImageView iv = new ImageView(getContext());
        iv.setImageBitmap(bm);
        windowManager.addView(iv, windowParams);
        dragImageView = iv;
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        gstDtct.onTouchEvent(ev);
        if(dragImageView!=null && dragPosition!=AdapterView.INVALID_POSITION){
            int x = (int)ev.getX();
            int y = (int)ev.getY();
            switch(ev.getAction()){
                case MotionEvent.ACTION_MOVE:
                    onDrag(x,y);
                    break;
                case MotionEvent.ACTION_UP:
                    stopDrag();
                    onDrop(x,y);
                    break;
            }
            return true;
        }
        if(isLongTouch){
            if(MotionEvent.ACTION_UP == ev.getAction()) {
                isLongTouch = false;
                /**
                 * FIX BUG: 5699
                 * FIX COMMENT: remove the black background;
                 * DATE: 2014-01-07
                 */
                ((EffectShowedAdapter)getAdapter()).setPressed(-1);
            }
            return true;
        }
        return super.onTouchEvent(ev);
    }

    private void onDrag(int x, int y) {
        if(dragImageView!=null){
            windowParams.alpha = 0.6f;
            windowParams.x = x - dragPointX + dragOffsetX;
            windowParams.y = y - dragPointY + dragOffsetY;
            windowManager.updateViewLayout(dragImageView, windowParams);
        }
        /**
         * FIX BUG: 4705 4749
         * FIX COMMENT: add scroll feature;
         * DATE: 2013-10-31
         */
        if((y + mItemHeight ) > UiUtils.screenHeight()) {
            smoothScrollToPosition(getLastVisiblePosition() + 1);
        } else if((y - mItemHeight / 2) < 0) {
            smoothScrollToPosition(getFirstVisiblePosition() - 1);
        }
    }

    private void onDrop(int x,int y) {
        int tempPosition = pointToPosition(x, y);
        if(tempPosition!=AdapterView.INVALID_POSITION){
            dropPosition = tempPosition;
        }
        if(dropPosition!=dragPosition){
            DragGridViewAdapter adapter = (DragGridViewAdapter)getAdapter();
            adapter.exchangeItem(dragPosition, dropPosition);
        }
        if(null != itemView){
            itemView.setVisibility(View.VISIBLE);
            itemView = null;
        }
    }

    private void stopDrag() {
        if(dragImageView!=null){
            windowManager.removeView(dragImageView);
            dragImageView = null;
            isLongTouch = false;
            ((EffectShowedAdapter)getAdapter()).setPressed(-1);
        }
    }

    @Override
    public boolean onDown(MotionEvent e) {
        return false;
    }

    @Override
    public void onShowPress(MotionEvent e) {

    }

    @Override
    public boolean onSingleTapUp(MotionEvent e) {
        return false;
    }
    @Override
    public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
        return false;
    }

    private Matrix cachedMatrix;

    @Override
    public void onLongPress(MotionEvent ev) {
        isLongTouch = true;
        int x = (int)ev.getX();
        int y = (int)ev.getY();
        dragPosition = dropPosition = pointToPosition(x, y);
        if(dragPosition==AdapterView.INVALID_POSITION){
            return;
        }
        DragGridViewAdapter adapter = (DragGridViewAdapter)getAdapter();
        if(false == adapter.checkBeforeDragItem(dragPosition)) {
            return;
        }
//        View itemView = (View)getChildAt(dragPosition - getFirstVisiblePosition());
        itemView = (View)getChildAt(dragPosition - getFirstVisiblePosition());
        dragPointX = x - itemView.getLeft();
        dragPointY = y - itemView.getTop();
        mItemHeight = itemView.getMeasuredHeight();

        dragOffsetX = (int)(ev.getRawX() - x);
        dragOffsetY = (int)(ev.getRawY() - y);
        itemView.destroyDrawingCache();
        itemView.setDrawingCacheEnabled(true);
//        Bitmap bm = Bitmap.createBitmap(itemView.getDrawingCache());
        Bitmap cachedBm = itemView.getDrawingCache();
        if(null == cachedMatrix) {
            cachedMatrix = new Matrix();
            cachedMatrix.postScale(1.2f, 1.2f);
        }
        Bitmap bm = Bitmap.createBitmap(cachedBm, 0, 0, cachedBm.getWidth(), cachedBm.getHeight(),
                cachedMatrix, false);
        itemView.setVisibility(View.INVISIBLE);
        ((EffectShowedAdapter)getAdapter()).setPressed(dragPosition);
        startDrag(bm,x,y);
    }

    @Override
    public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
        return false;
    }
}
