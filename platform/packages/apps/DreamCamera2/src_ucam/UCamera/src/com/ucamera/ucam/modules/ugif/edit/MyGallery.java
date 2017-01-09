/*
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ucam.modules.ugif.edit;

import com.android.camera2.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PixelFormat;
import android.graphics.drawable.BitmapDrawable;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;

/**
 * A view that shows items in a center-locked, and can drag item to anywhere,
 * horizontally scrolling list.
 */
public class MyGallery extends HGallery {
    //current context
    private Context mContext;
    //start to long press position
    private int mStartPosition;
    //start to drag x-coordinate relative parent component(MyGallery)
    private int mDragPointX;
    //start to drag y-coordinate relative parent component(MyGallery)
    private int mDragPointY;
    //offset x-coordinate relative current window
    private int mDragOffsetX;
    //offset y-coordinate relative current window
    private int mDragOffsetY;
    //float and drag imageview
    private ImageView mDragImageView;
    //loading the window about floating images
    private WindowManager mWindowManager;
    //window parameter
    private WindowManager.LayoutParams mWindowParams;
    //item width
    private int mItemWidth;
    //item height
    private int mItemHeight;
    //screen width
    private int mScreenWidth;
    //temp variable, temp storage position of variable
    private int mTempPosition = -1;
    //current item pressed
    private Bitmap mCurrentPressedBitmap;
    private GalleryCallback mCallback = null;
    /**
     * Constructor method
     *
     * @param context context
     *
     * @param attrs attrs
     */
    public MyGallery(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    public void setGalleryCallback(GalleryCallback callback) {
        mCallback = callback;
    }

    public interface GalleryCallback {
        public void gifChangePosition();
        public void itemLongClick();
    }
    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (ev.getAction() == MotionEvent.ACTION_DOWN){
            /* FIX BUG: 400
             * BUG CAUSE: the ev.getX() and getY() changed in onItemLongClick function on ICS version
             * FIX COMMENT: pass the right value of x y rawx rawy to onItemLongClick function
             * DATE: 2012-04-10
             */
            setOnItemLongClickListener((int)ev.getX(), (int)ev.getY(), ev.getRawX(), ev.getRawY());
        }
        return super.onInterceptTouchEvent(ev);
    }

    /**
     * Handles long clicking
     *
     * @param ev motion event
     *
     * @return True there was an assigned OnItemLongClickListener that was called, false
     *         otherwise is returned.
     */
    public void setOnItemLongClickListener(final int x, final int y, final float rawx, final float rawy) {
        setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            public boolean onItemLongClick(AdapterView<?> adapterView, View view, int position, long id) {
                if (mCallback != null) {
                    mCallback.itemLongClick();
                }

                mScreenWidth = MyGallery.this.getWidth();
                mStartPosition = pointToPosition(x, y);
                mTempPosition = mStartPosition;
                if (mStartPosition == AdapterView.INVALID_POSITION) {
                    return false;
                }
                View tempView = MyGallery.this.getChildAt(mStartPosition - getFirstVisiblePosition());
                ImageView itemView = (ImageView)tempView.findViewById(R.id.image_list);
                mDragPointX = x - (((View)itemView.getParent()).getLeft() + getPaddingLeft());
                mDragPointY = y - itemView.getTop();

                mDragOffsetX = (int) (rawx - x);
                mDragOffsetY = (int) (rawy - y);

                mItemWidth = itemView.getWidth();
                mItemHeight = itemView.getHeight();

                BitmapDrawable mDrawable =  (BitmapDrawable) itemView.getDrawable();
                Bitmap bm = mDrawable.getBitmap();
                mCurrentPressedBitmap = BitmapFactory.decodeResource(mContext.getResources(), R.drawable.ugif_edit_drag_ugif_item);
                itemView.setImageBitmap(mCurrentPressedBitmap);
                startDrag(bm, x, y);

                return false;
            };
        });
    }

    /**
     * build floating window add the cache item to it
     *
     * @param bm item cache
     *
     * @param x x-coordinate
     *
     * @param y y-coordinate
     */
    private void startDrag(Bitmap bm, int x, int y) {
        stopDrag();

        mWindowParams = new WindowManager.LayoutParams();
        mWindowParams.gravity = Gravity.TOP | Gravity.LEFT;
        mWindowParams.x = x - mDragPointX + mDragOffsetX;
        mWindowParams.y = y - mDragPointY + mDragOffsetY;
        mWindowParams.height = (int)(mItemHeight * 1.2);
        mWindowParams.width = (int)(mItemWidth * 1.2);
        mWindowParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
                | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN;
        mWindowParams.format = PixelFormat.TRANSLUCENT;
        mWindowParams.windowAnimations = 0;

        ImageView imageView = new ImageView(getContext());
        imageView.setImageBitmap(bm);
        mWindowManager = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
        mWindowManager.addView(imageView, mWindowParams);
        mDragImageView = imageView;
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (mDragImageView != null && mStartPosition != AdapterView.INVALID_POSITION) {
            int x = (int) ev.getX();
            int y = (int) ev.getY();
            switch (ev.getAction()) {
                case MotionEvent.ACTION_MOVE:
                    onDrag(x, y);
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    //ACTION_CANCEL, to avoid triggering the HOME key when drag the item
                    stopDrag();
                    onDrop(x, y);
                    /**
                     * FIX BUG: 1231
                     * BUG CAUSE: As long as moved, and drop the picture, whether it is in the original position regardless,
                     *            the save button will be clicked;
                     * FIX COMMENT: To determine the start position and end positions are the same;
                     * DATE: 2012-07-16
                     */
                    if(mStartPosition != mTempPosition) {
                        if (mCallback != null) {
                            mCallback.gifChangePosition();
                        }
                    }
                    break;
            }
        }
        return super.onTouchEvent(ev);
    }

    /**
     * start dragging the float window.
     *
     * @param x current x-coordinate relative parent component(MyGallery)
     *
     * @param y current y-coordinate relative parent component(MyGallery)
     */
    private void onDrag(int x, int y) {
        if (mDragImageView != null) {
            mWindowParams.alpha = 0.9f;
            mWindowParams.x = x - mDragPointX + mDragOffsetX;
            mWindowParams.y = y - mDragPointY + mDragOffsetY;
            mWindowManager.updateViewLayout(mDragImageView, mWindowParams);
        }

        //drag the item to the edge of the screen, the component will slide item one by one.
        int currentPosition = pointToPosition(x, y);
        if(((x + mItemWidth / 3) > mScreenWidth && currentPosition < mItemCount - 1) || ((x - mItemWidth / 3) < 0 && currentPosition > 0)) {
            scrollToChild(currentPosition - mFirstPosition);
        }

        if(currentPosition != AdapterView.INVALID_POSITION && currentPosition != mTempPosition) {
            dragToExchangeItem(mTempPosition, currentPosition);
            mTempPosition = currentPosition;
        }
    }

    /**
     * refresh items in time
     *
     * @param startPosition start to press position
     *
     * @param endPosition end position
     */
    private void dragToExchangeItem(int startPosition, int endPosition) {
        MyGalleryAdapter adapter = (MyGalleryAdapter) this.getAdapter();
        adapter.refreshAdapter(startPosition, endPosition, false, mCurrentPressedBitmap);
    }

    /**
     * complete dragging, the event is key up.
     *
     * @param x x-coordinate relative parent component(MyGallery)
     *
     * @param y y-coordinate relative parent component(MyGallery)
     */
    private void onDrop(int x, int y) {
        int endPosition = pointToPosition(x, y);
        if (endPosition == AdapterView.INVALID_POSITION) {
            endPosition = mTempPosition;
        }
        MyGalleryAdapter adapter = (MyGalleryAdapter) this.getAdapter();
        adapter.refreshAdapter(mTempPosition, endPosition, true, mCurrentPressedBitmap);
        //recycled the temp bitmap
        if(mCurrentPressedBitmap != null && !mCurrentPressedBitmap.isRecycled()) {
            mCurrentPressedBitmap.recycle();
            mCurrentPressedBitmap = null;
        }
    }

    /**
     * remove the cache item from floating window.
     */
    private void stopDrag() {
        if (mDragImageView != null) {
            mWindowManager.removeView(mDragImageView);
            mDragImageView = null;
        }
    }
}
