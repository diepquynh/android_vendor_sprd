/*
 * Copyright (C) 2011,2012 Thundersoft Corporation
 * All rights Reserved
 */

/*
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

package com.ucamera.ugallery;

import com.ucamera.ugallery.R;
import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.IImageList;
import com.ucamera.ugallery.gallery.ImageLoader;

import static com.ucamera.ugallery.util.Util.Assert;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.media.AudioManager;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.GestureDetector;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.widget.Scroller;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.TreeMap;

class GridViewSpecial extends View {
    @SuppressWarnings("unused")
    private static final String TAG = "GridViewSpecial";

    private static final float MAX_FLING_VELOCITY = 2500;

    public static int mSelectItem = -1;
    public static interface Listener {
//        void onImageClicked(int index);

        void onImageTapped(int index);

        void onTitleTapped(int row);

        void onLayoutComplete(boolean changed);

        /**
         * Invoked when the <code>GridViewSpecial</code> scrolls.
         *
         * @param scrollPosition the position of the scroller in the range [0,
         *            1], when 0 means on the top and 1 means on the buttom
         */
        void onScroll(float scrollPosition);

        void onImageLongPressed(int index);
    }

    public static interface DrawAdapter {
        void drawImage(Canvas canvas, IImage image, Bitmap b, int xPos, int yPos, int w,
                int h);
        void drawPlayIcon(Canvas canvas, IImage image, Bitmap b, int xPos, int yPos, int w,
                int h);
        void drawDecoration(Canvas canvas, IImage image, int xPos, int yPos, int w, int h);

        void drawTitleDecoration(Canvas canvas,TimeModeItem item, int titleRow, int xPos, int yPos, int w, int h);

        boolean needsDecoration();
    }

    public static final int INDEX_NONE = -1;

    private LayoutSpec[] mCellSizeChoices;

    private void initCellSize() {
        Activity a = (Activity) getContext();
        DisplayMetrics metrics = new DisplayMetrics();
        a.getWindowManager().getDefaultDisplay().getMetrics(metrics);
        mCellSizeChoices = new LayoutSpec[] {
                new LayoutSpec(67, 67, 8, 0, metrics), new LayoutSpec(100, 100, 5, 0, metrics),
        };
    }
    public LayoutSpec getLayoutSpec() {
        return mCellSizeChoices[mSizeChoice];
    }
    // These are set in init().
    private final Handler mHandler = new Handler();

    private GestureDetector mGestureDetector;

    private ImageBlockManager mImageBlockManager;

    // These are set in set*() functions.
    private ImageLoader mLoader;

    private Listener mListener = null;

    private DrawAdapter mDrawAdapter = null;

    //private IImageList mAllImages = ImageManager.makeEmptyImageList();

    private int mSizeChoice = 1; // default is big cell size

    // These are set in onLayout().
    private LayoutSpec mSpec;

    private int mColumns;

    private int mMaxScrollY;

    // We can handle events only if onLayout() is completed.
    private boolean mLayoutComplete = false;
    private boolean mLoadComplete = false;

    // Selection state
    private int mCurrentSelection = INDEX_NONE;

    private int mCurrentPressState = 0;

    private static final int TAPPING_FLAG = 1;

    private static final int CLICKING_FLAG = 2;

    // These are cached derived information.
    private int mCount; // Cache mImageList.getCount();

    private int mRows; // Cache (mCount + mColumns - 1) / mColumns

    private int mBlockHeight; // Cache mSpec.mCellSpacing + mSpec.mCellHeight

    private boolean mRunning = false;

    private Scroller mScroller = null;

    private ArrayList<TimeModeItem> mTimeModeItems;
    private int mDateCount = 0;
//    private int mTitleHeight;
    private int mCurrentMode;
    private TreeMap<Integer, TimeModeItem> mTitleRowMap;
    private TreeMap<Integer, Integer> mTimeRowMap;

    private ImageListSpecial mImageListSpecial;

    public GridViewSpecial(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    private void init(Context context) {
        setVerticalScrollBarEnabled(true);
        /* SPRD: CID 109010 : Resource leak (RESOURCE_LEAK) @{ */
        TypedArray arr = context.obtainStyledAttributes(android.R.styleable.View);
        initializeScrollbars(arr);
        arr.recycle();
        // initializeScrollbars(context.obtainStyledAttributes(android.R.styleable.View));
        /* @} */
        mGestureDetector = new GestureDetector(context, new MyGestureDetector());
        setFocusableInTouchMode(true);
        initCellSize();
    }

    private final Runnable mRedrawCallback = new Runnable() {
        public void run() {
            invalidate();
        }
    };

    public void setLoader(ImageLoader loader) {
        Assert(mRunning == false);
        mLoader = loader;
        mLoader.setImageCount(mCount);
    }
    public void setListener(Listener listener) {
        Assert(mRunning == false);
        mListener = listener;
    }

    public void setDrawAdapter(DrawAdapter adapter) {
        Assert(mRunning == false);
        mDrawAdapter = adapter;
    }

    public void setImageList(ImageListSpecial list) {
        Assert(mRunning == false);
        mImageListSpecial = list;
        mCount = mImageListSpecial.getCount();
    }

    public void setImageList(IImageList list) {
        Assert(mRunning == false);
        mImageListSpecial = new ImageListSpecial(list, false);
        mCount = mImageListSpecial.getCount();
    }

    public void setTimeModeItems(ArrayList<TimeModeItem> timeModeItems) {
        mTimeModeItems = timeModeItems;
    }

    public void resetTimeModeItems() {
        if(mTimeModeItems != null) {
            mTimeModeItems.clear();
            mTimeModeItems = null;
        }
    }

    public void setDisplayMode(int currentMode) {
        mCurrentMode = currentMode;
    }

    private boolean isTimeMode() {
        return mCurrentMode == 1;
    }
    public void setSizeChoice(int choice) {
        /**
         * FIX BUG: 1137
         * FIX COMMENT: mRunning is true, will return;
         * DATE: 2012-07-18
         */
//        Assert(mRunning == false);
        if(mRunning) {
            Log.w(TAG, "setSizeChoice(): mRunning is true, return;");
            return;
        }
        if (mSizeChoice == choice) {
            return;
        }
        mSizeChoice = choice;
    }

    @Override
    public void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);

        if (!mRunning) {
            return;
        }
        if(mLoadComplete) return;
        mSpec = mCellSizeChoices[mSizeChoice];

        int width = right - left;

        // The width is divided into following parts:
        //
        // LeftEdgePadding CellWidth (CellSpacing CellWidth)* RightEdgePadding
        //
        // We determine number of cells (columns) first, then the left and right
        // padding are derived. We make left and right paddings the same size.
        //
        // The height is divided into following parts:
        //
        // CellSpacing (CellHeight CellSpacing)+

        mColumns = 1 + (width - mSpec.mCellWidth) / (mSpec.mCellWidth + mSpec.mCellSpacing);
        mSpec.setColumns(mColumns);
        mSpec.mLeftEdgePadding = (width - ((mColumns - 1) * mSpec.mCellSpacing) - (mColumns * mSpec.mCellWidth)) / 2;
        mBlockHeight = mSpec.mCellSpacing + mSpec.mCellHeight;
        boolean isTimeMode = isTimeMode();

        if(isTimeMode) {
            int tmpRows = 0;
            if(mTimeModeItems != null && mTimeModeItems.size() > 0) {
                mDateCount = mTimeModeItems.size();
                mTitleRowMap = new TreeMap<Integer, TimeModeItem>();
                mTimeRowMap = new TreeMap<Integer, Integer>();
                int rowIndex = 0;
                int titleIndex;
                for(TimeModeItem item : mTimeModeItems) {
                    titleIndex = tmpRows + rowIndex;
                    mTitleRowMap.put(titleIndex, item);
                    int tmpCount = item.getCount();
                    tmpRows +=(tmpCount + mColumns - 1) / mColumns;
                    int quotient = tmpCount / mColumns;
                    int mode = tmpCount % mColumns;
                    for(int xx = (titleIndex + 1); xx <= (titleIndex + quotient); xx++) {
                        mTimeRowMap.put(xx, mColumns);
                    }
                    if(mode > 0 || quotient == 0) {
                        mTimeRowMap.put(titleIndex + quotient + 1, mode);
                    }
                    rowIndex++;
                }
            }
            mRows = tmpRows + mDateCount;
        } else {
            mRows = (mCount + mColumns - 1) / mColumns;
        }

        mMaxScrollY = mSpec.mCellSpacing + (mRows * mBlockHeight) - (bottom - top);
        // Put mScrollY in the valid range. This matters if mMaxScrollY is
        // changed. For example, orientation changed from portrait to landscape.
        mScrollY = Math.max(0, Math.min(mMaxScrollY, mScrollY));
        generateOutlineBitmap();

        if (mImageBlockManager != null) {
            mImageBlockManager.recycle();
        }

        mImageBlockManager = new ImageBlockManager(mHandler, mRedrawCallback, mImageListSpecial.getFilterImageList(), mLoader, mDrawAdapter,
                mSpec, mColumns, mRows, width, mOutlines[OUTLINE_EMPTY], isTimeMode, mTitleRowMap, mTimeRowMap);

        mListener.onLayoutComplete(changed);

        moveDataWindow();

        mLayoutComplete = true;
        mLoadComplete=true;
    }
    @Override
    protected int computeVerticalScrollRange() {
        return mMaxScrollY + getHeight();
    }

    public void resetPosition(){
        mScrollY = 0;
    }

      @Override
    protected void onConfigurationChanged(Configuration newConfig) {
          mLoadComplete=false;
     super.onConfigurationChanged(newConfig);
   }
    public void resetLayoutCompleteStatus(){
          mLoadComplete=false;
     }
  // We cache the three outlines from NinePatch to Bitmap to speed up
    // drawing. The cache must be updated if the cell size is changed.
    public static final int OUTLINE_EMPTY = 0;

    public static final int OUTLINE_PRESSED = 1;

    public static final int OUTLINE_SELECTED = 2;

    public Bitmap mOutlines[] = new Bitmap[3];

    private void generateOutlineBitmap() {
        int w = mSpec.mCellWidth;
        int h = mSpec.mCellHeight;

        releaseBitmaps();
        for (int i = 0; i < mOutlines.length; i++) {
            /*
             * FIX BUG: 4494
             * BUG COMMENT:catch OOM error and return, if this error has come,the item will lose pressed effect.
             * DATE: 2014-02-13
             */
            try{
                mOutlines[i] = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_4444);
            }catch(OutOfMemoryError oom) {
                Log.e(TAG, "generateOutlineBitmap OOM Error : "+oom);
                return;
            }
        }

        Drawable cellOutline;
        cellOutline = GridViewSpecial.this.getResources().getDrawable(
                R.drawable.gallery_bg_photoes_status);
        cellOutline.setBounds(0, 0, w, h);
        Canvas canvas = new Canvas();

        canvas.setBitmap(mOutlines[OUTLINE_EMPTY]);
        cellOutline.setState(EMPTY_STATE_SET);
        cellOutline.draw(canvas);

        canvas.setBitmap(mOutlines[OUTLINE_PRESSED]);
        cellOutline.setState(PRESSED_ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET);
        cellOutline.draw(canvas);

        canvas.setBitmap(mOutlines[OUTLINE_SELECTED]);
        cellOutline.setState(ENABLED_FOCUSED_SELECTED_WINDOW_FOCUSED_STATE_SET);
        cellOutline.draw(canvas);
    }

    private void moveDataWindow() {
        // Calculate visible region according to scroll position.
        int startRow = (mScrollY - mSpec.mCellSpacing) / mBlockHeight;
        int endRow = (mScrollY + getHeight() - mSpec.mCellSpacing - 1) / mBlockHeight + 1;

        // Limit startRow and endRow to the valid range.
        // Make sure we handle the mRows == 0 case right.
        startRow = Math.max(Math.min(startRow, mRows - 1), 0);
        endRow = Math.max(Math.min(endRow, mRows), 0);
        mImageBlockManager.setVisibleRows(startRow, endRow);
    }

    /**
     * In MyGestureDetector we have to check canHandleEvent() because
     * GestureDetector could queue events and fire them later. At that time
     * stop() may have already been called and we can't handle the events.
     *
     */
    private class MyGestureDetector extends SimpleOnGestureListener {
        private AudioManager mAudioManager;

        @Override
        public boolean onDown(MotionEvent e) {
            if (!canHandleEvent()) {
                return false;
            }
            if (mScroller != null && !mScroller.isFinished()) {
                mScroller.forceFinished(true);
                return false;
            }
            int index = computeSelectedIndex(e.getX(), e.getY());
            if (index >= 0 && index < mCount) {
                setSelectedIndex(index);
            } /*else if(isTimeMode() && index >= mCount) {

            }*/ else {
                setSelectedIndex(INDEX_NONE);
            }

            return true;
        }

        @Override
        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
            if (!canHandleEvent()) {
                return false;
            }
            if (velocityY > MAX_FLING_VELOCITY) {
                velocityY = MAX_FLING_VELOCITY;
            } else if (velocityY < -MAX_FLING_VELOCITY) {
                velocityY = -MAX_FLING_VELOCITY;
            }

            setSelectedIndex(INDEX_NONE);
            mScroller = new Scroller(getContext());
            mScroller.fling(0, mScrollY, 0, -(int) velocityY, 0, 0, 0, mMaxScrollY);
            computeScroll();

            return true;
        }

        @Override
        public void onLongPress(MotionEvent e) {
            mLoadComplete=false;
            int index = computeSelectedIndex(e.getX(), e.getY());
            if (!canHandleEvent()) {
                return;
            }
            performLongClick();
            if (index >= 0 && index < mCount) {
                mSelectItem = index;
                mListener.onImageLongPressed(index);
            }
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            if (!canHandleEvent()) {
                return false;
            }
            setSelectedIndex(INDEX_NONE);
            scrollBy(0, (int) distanceY);
            invalidate();
            return true;
        }

        @Override
        public boolean onSingleTapUp(MotionEvent e) {
            if (!canHandleEvent()) {
                return false;
            }
            int index = computeSelectedIndex(e.getX(), e.getY());
            if (index >= 0 && index < mCount) {
                // Play click sound.
                if (mAudioManager == null) {
                    mAudioManager = (AudioManager) getContext().getSystemService(
                            Context.AUDIO_SERVICE);
                }
                mAudioManager.playSoundEffect(AudioManager.FX_KEY_CLICK);

                mListener.onImageTapped(index);
                return true;
            } else if(index >= mCount) {
                //this case computes title row, the index is current row add mCount.
                int row = index - mCount;
                if(isTimeMode() && mTitleRowMap.containsKey(row)) {
                    mListener.onTitleTapped(row);
                }
                return true;
            }
            return false;
        }

        /*@Override
        public boolean onSingleTapConfirmed(MotionEvent e) {
            if (!canHandleEvent()) {
                return false;
            }
            int index = computeSelectedIndex(e.getX(), e.getY());
            if (index >= 0 && index < mCount) {
                // Play click sound.
                if (mAudioManager == null) {
                    mAudioManager = (AudioManager) getContext().getSystemService(
                            Context.AUDIO_SERVICE);
                }
                mAudioManager.playSoundEffect(AudioManager.FX_KEY_CLICK);

                mListener.onImageTapped(index);
                return true;
            }
            return false;
        }*/
    }

    public TreeMap<Integer, TimeModeItem> getTitleRowMap() {
        return mTitleRowMap;
    }

    public TreeMap<Integer, Integer> getImageIndexMap() {
        return mTimeRowMap;
    }

    public int getCurrentSelection() {
        return mCurrentSelection;
    }

    public void invalidateImage(int index) {
        /*
         * FIX BUG : 5964
         * BUG COMMENT : avoid null point exception
         * DATE : 2014-02-24
         */
        if (mImageBlockManager != null && index != INDEX_NONE) {
            mImageBlockManager.invalidateImage(index);
        }
    }

    /**
     * @param index <code>INDEX_NONE</code> (-1) means remove selection.
     */
    public void setSelectedIndex(int index) {
        // A selection box will be shown for the image that being selected,
        // (by finger or by the dpad center key). The selection box can be drawn
        // in two colors. One color (yellow) is used when the the image is
        // still being tapped or clicked (the finger is still on the touch
        // screen or the dpad center key is not released). Another color
        // (orange) is used after the finger leaves touch screen or the dpad
        // center key is released.

        if (mCurrentSelection == index) {
            return;
        }
        // This happens when the last picture is deleted.
        mCurrentSelection = Math.min(index, mCount - 1);

        if (mCurrentSelection != INDEX_NONE) {
            ensureVisible(mCurrentSelection);
        }
        invalidate();
    }

    public void scrollToImage(int index) {
        Rect r = getRectForPosition(index);
        scrollTo(0, r.top);
    }

    public void scrollToVisible(int index) {
        Rect r = getRectForPosition(index);
        int top = getScrollY();
        int bottom = getScrollY() + getHeight();
        if (r.bottom > bottom) {
            scrollTo(0, r.bottom - getHeight());
        } else if (r.top < top) {
            scrollTo(0, r.top);
        }
    }

    private void ensureVisible(int pos) {
        Rect r = getRectForPosition(pos);
        int top = getScrollY();
        int bot = top + getHeight();

        if (r.bottom > bot) {
            mScroller = new Scroller(getContext());
            mScroller.startScroll(mScrollX, mScrollY, 0, r.bottom - getHeight() - mScrollY, 200);
            computeScroll();
        } else if (r.top < top) {
            mScroller = new Scroller(getContext());
            mScroller.startScroll(mScrollX, mScrollY, 0, r.top - mScrollY, 200);
            computeScroll();
        }
    }

    public void start() {
        // These must be set before start().
        Assert(mLoader != null);
        Assert(mListener != null);
        Assert(mDrawAdapter != null);
        mLoadComplete = false;
        mRunning = true;
        requestLayout();
    }

    // If the the underlying data is changed, for example,
    // an image is deleted, or the size choice is changed,
    // The following sequence is needed:
    //
    // mGvs.stop();
    // mGvs.set...(...);
    // mGvs.set...(...);
    // mGvs.start();
    public void stop() {
        // Remove the long press callback from the queue if we are going to
        // stop.
        mHandler.removeCallbacks(mLongPressCallback);
//        mScroller.computeScrollOffset();
        mScroller = null;
        if (mImageBlockManager != null) {
            mImageBlockManager.recycle();
            mImageBlockManager = null;
        }
        mRunning = false;
        mLoadComplete = false;
        mCurrentSelection = INDEX_NONE;
        releaseBitmaps();
    }
     /**
      * FIX BUG:6212
      * BUG COMMENT: Add the function to get the stat of mRunning;
      * DATE: 2014-04-17
      */
    public boolean getRunningStat(){
        return mRunning;
    }

    private void releaseBitmaps() {
        for (int i = 0; i < mOutlines.length; ++i) {
            if (mOutlines[i] != null && !mOutlines[i].isRecycled()) {
                mOutlines[i].recycle();
            }
        }
    }

    @Override
    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (!canHandleEvent()) {
            return;
        }
        /*
         * FIX BUG : 4742
         * BUG COMMENT : avoid null point exception
         * DATE : 2013-08-23
         */
        if(mImageBlockManager != null) {
            mImageBlockManager.doDraw(canvas, getWidth(), getHeight(), mScrollY);
            paintDecoration(canvas);
            paintSelection(canvas);
            moveDataWindow();
        }
    }

    @Override
    public void computeScroll() {
        if (mScroller != null) {
            boolean more = mScroller.computeScrollOffset();
            scrollTo(0, mScroller.getCurrY());
            if (more) {
                invalidate(); // So we draw again
            } else {
                mScroller = null;
            }
        } else {
            super.computeScroll();
        }
    }

    // Return the rectange for the thumbnail in the given position.
    Rect getRectForPosition(int pos) {
        int row = 0;
        int col = 0;
        if(isTimeMode()) {
            /*
             * FIX BUG : 6125
             * BUG COMMENT : avoid null point exception
             * DATE : 2014-03-17
             */
            if(mImageBlockManager != null) {
                row = mImageBlockManager.findRowByPos(pos);
                col = mImageBlockManager.findColumnByPos(row, pos);
            }
        } else {
            row = pos / mColumns;
            col = pos - (row * mColumns);
        }

        int left = mSpec.mLeftEdgePadding + (col * (mSpec.mCellWidth + mSpec.mCellSpacing));
        int top = row * mBlockHeight;

        return new Rect(left, top, left + mSpec.mCellWidth + mSpec.mCellSpacing, top
                + mSpec.mCellHeight + mSpec.mCellSpacing);
    }

    // Inverse of getRectForPosition: from screen coordinate to image position.
    int computeSelectedIndex(float xFloat, float yFloat) {
        int x = (int) xFloat;
        int y = (int) yFloat;

        int spacing = mSpec.mCellSpacing;
        int leftSpacing = mSpec.mLeftEdgePadding;

        int row = (mScrollY + y - spacing) / (mSpec.mCellHeight + spacing);
        int selectedIndex = INDEX_NONE;
        if(isTimeMode()) {
            if(mTitleRowMap != null && mTitleRowMap.containsKey(row)) {
                int left = (mSpec.mCellWidth + mSpec.mCellSpacing) * (mColumns - 1);
                int top = (mSpec.mCellHeight + mSpec.mCellSpacing) * row - mScrollY;
                int offset = mSpec.mCellWidth + mSpec.mCellSpacing;
                Rect rect = new Rect(left+ offset/2, top, left + offset*3/2, top + mSpec.mCellHeight + mSpec.mCellSpacing);
                if(rect.contains(x, y)) {
                    //If current row is title, then the row num is current row add mCount;
                    return row + mCount;
                } else {
                    return INDEX_NONE;
                }
            } else if(row < mRows && mTimeRowMap != null && mTimeRowMap.containsKey(row)){
                if(mImageBlockManager == null || mTimeRowMap == null) {
                    return INDEX_NONE;
                }
                int pos = mImageBlockManager.findPosByRow(row);
                int rowNum = mTimeRowMap.get(row);
                int col = (x - leftSpacing) / (mSpec.mCellWidth + spacing);
                if(col > rowNum - 1) {
                    return INDEX_NONE;
                }
                selectedIndex = pos + col;
            }
        } else {
            int col = Math.min(mColumns - 1, (x - leftSpacing) / (mSpec.mCellWidth + spacing));
            selectedIndex = (row * mColumns) + col;
        }

        return selectedIndex;
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        if (!canHandleEvent()) {
            return false;
        }
        switch (ev.getAction()) {
            case MotionEvent.ACTION_DOWN:
                mCurrentPressState |= TAPPING_FLAG;
                invalidate();
                break;
            case MotionEvent.ACTION_UP:
                mCurrentPressState &= ~TAPPING_FLAG;
                invalidate();
                break;
            default :
                break;
        }
        mGestureDetector.onTouchEvent(ev);
        // Consume all events
        return true;
    }

    @Override
    public void scrollBy(int x, int y) {
        scrollTo(mScrollX + x, mScrollY + y);
    }

    public void scrollTo(float scrollPosition) {
        scrollTo(0, Math.round(scrollPosition * mMaxScrollY));
    }

    @Override
    public void scrollTo(int x, int y) {
        y = Math.max(0, Math.min(mMaxScrollY, y));
        if (mSpec != null) {
            mListener.onScroll((float) mScrollY / mMaxScrollY);
        }
        super.scrollTo(x, y);
    }

    private boolean canHandleEvent() {
        return mRunning && mLayoutComplete;
    }

    private final Runnable mLongPressCallback = new Runnable() {
        public void run() {
            mCurrentPressState &= ~CLICKING_FLAG;
            showContextMenu();
        }
    };

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (!canHandleEvent()) {
            return false;
        }
        int sel = mCurrentSelection;
        if (sel != INDEX_NONE) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    if (sel != mCount - 1 && (sel % mColumns < mColumns - 1)) {
                        sel += 1;
                    }
                    break;
                case KeyEvent.KEYCODE_DPAD_LEFT:
                    if (sel > 0 && (sel % mColumns != 0)) {
                        sel -= 1;
                    }
                    break;
                case KeyEvent.KEYCODE_DPAD_UP:
                    if (sel >= mColumns) {
                        sel -= mColumns;
                    }
                    break;
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    if (sel != (mCount - 1)) {
                        sel = Math.min(mCount - 1, sel + mColumns);
                    } else {
                        return super.onKeyDown(keyCode, event);
                    }
                    break;
                case KeyEvent.KEYCODE_DPAD_CENTER:
                    if (event.getRepeatCount() == 0) {
                        mCurrentPressState |= CLICKING_FLAG;
                        mHandler.postDelayed(mLongPressCallback, ViewConfiguration
                                .getLongPressTimeout());
                    }
                    break;
                default:
                    return super.onKeyDown(keyCode, event);
            }
        } else {
            switch (keyCode) {
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                case KeyEvent.KEYCODE_DPAD_LEFT:
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    int startRow = (mScrollY - mSpec.mCellSpacing) / mBlockHeight;
                    int topPos = startRow * mColumns;
                    Rect r = getRectForPosition(topPos);
                    if (r.top < getScrollY()) {
                        topPos += mColumns;
                    }
                    topPos = Math.min(mCount - 1, topPos);
                    sel = topPos;
                    break;
                default:
                    return super.onKeyDown(keyCode, event);
            }
        }
        setSelectedIndex(sel);
        return true;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (!canHandleEvent()) {
            return false;
        }

        if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER) {
            mCurrentPressState &= ~CLICKING_FLAG;
            invalidate();

            // The keyUp doesn't get called when the longpress menu comes up. We
            // only get here when the user lets go of the center key before the
            // longpress menu comes up.
            mHandler.removeCallbacks(mLongPressCallback);

            // open the photo
//            mListener.onImageClicked(mCurrentSelection);
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    private void paintDecoration(Canvas canvas) {
        if (!mDrawAdapter.needsDecoration()) {
            return;
        }

        // Calculate visible region according to scroll position.
        int startRow = (mScrollY - mSpec.mCellSpacing) / mBlockHeight;
        int endRow = (mScrollY + getHeight() - mSpec.mCellSpacing - 1) / mBlockHeight + 1;

        // Limit startRow and endRow to the valid range.
        // Make sure we handle the mRows == 0 case right.
        startRow = Math.max(Math.min(startRow, mRows - 1), 0);
        endRow = Math.max(Math.min(endRow, mRows), 0);

        if(isTimeMode()) {
            for(; startRow < endRow; startRow++) {
//                int rowNum = mImageBlockManager.numColumns(startRow);
                if(mTimeRowMap.containsKey(startRow)) {
                    int startIndex = mImageBlockManager.findPosByRow(startRow);
                    int rowNum = mImageBlockManager.numColumns(startRow);
                    int xPos = mSpec.mLeftEdgePadding;
                    int yPos = mSpec.mCellSpacing + startRow * mBlockHeight;
                    for(int i = 0; i < rowNum; i++) {
                        IImage image = mImageListSpecial.getImageAtIndex(startIndex + i);

                        mDrawAdapter.drawDecoration(canvas, image, xPos, yPos, mSpec.mCellWidth,
                                mSpec.mCellHeight);
                        // Calculate next position
                        xPos += mSpec.mCellWidth + mSpec.mCellSpacing;
                    }
                } else if(mTitleRowMap.containsKey(startRow)) {
                    int xPos = mSpec.mLeftEdgePadding + (mSpec.mCellWidth + mSpec.mCellSpacing) * (mColumns - 1);
                    int yPos = mSpec.mCellSpacing + startRow * mBlockHeight;
                    TimeModeItem item = mTitleRowMap.get(startRow);
                    mDrawAdapter.drawTitleDecoration(canvas, item, startRow, xPos, yPos, mSpec.mCellWidth, mSpec.mCellHeight);
                }
            }
        } else {
            int startIndex = startRow * mColumns;
            int endIndex = Math.min(endRow * mColumns, mCount);

            int xPos = mSpec.mLeftEdgePadding;
            int yPos = mSpec.mCellSpacing + startRow * mBlockHeight;
            int off = 0;
            for (int i = startIndex; i < endIndex; i++) {
                IImage image = mImageListSpecial.getImageAtIndex(i);

                mDrawAdapter.drawDecoration(canvas, image, xPos, yPos, mSpec.mCellWidth,
                        mSpec.mCellHeight);

                // Calculate next position
                off += 1;
                if (off == mColumns) {
                    xPos = mSpec.mLeftEdgePadding;
                    yPos += mBlockHeight;
                    off = 0;
                } else {
                    xPos += mSpec.mCellWidth + mSpec.mCellSpacing;
                }
            }
        }
    }

    private void paintSelection(Canvas canvas) {
        if (mCurrentSelection == INDEX_NONE) {
            return;
        }

        int row = 0;
        int col = 0;
        if(isTimeMode()) {
            row = mImageBlockManager.findRowByPos(mCurrentSelection);
            col = mImageBlockManager.findColumnByPos(row, mCurrentSelection);
        } else {
            row = mCurrentSelection / mColumns;
            col = mCurrentSelection - (row * mColumns);
        }

        int spacing = mSpec.mCellSpacing;
        int leftSpacing = mSpec.mLeftEdgePadding;
        int xPos = leftSpacing + (col * (mSpec.mCellWidth + spacing));
        int yTop = spacing + (row * mBlockHeight);

        int type = OUTLINE_EMPTY;
        if (mCurrentPressState != 0) {
            type = OUTLINE_PRESSED;
        }
        // Paint p = new Paint();
        // p.setStyle(Paint.Style.FILL);
        // p.setColor(0x00ffffff);
        if(mOutlines[type] != null && !mOutlines[type].isRecycled()) {
            canvas.drawBitmap(mOutlines[type], xPos, yTop, null);
        }
        // canvas.drawBitmap(mOutline[type], xPos, yTop, p);
    }
}

class ImageBlockManager {
    @SuppressWarnings("unused")
    private static final String TAG = "ImageBlockManager";

    // Number of rows we want to cache.
    // Assume there are 6 rows per page, this caches 5 pages.
    private static final int CACHE_ROWS = 30;

    // mCache maps from row number to the ImageBlock.
    private final HashMap<Integer, ImageBlock> mCache;

    // These are parameters set in the constructor.
    private final Handler mHandler;

    private final Runnable mRedrawCallback; // Called after a row is loaded,

    // so GridViewSpecial can draw
    // again using the new images.
    //private final IImageList mImageList;
    private final ArrayList<IImage> mImageList;

    private final ImageLoader mLoader;

    private final GridViewSpecial.DrawAdapter mDrawAdapter;

    private final LayoutSpec mSpec;

    private final int mColumns; // Columns per row.

    private final int mBlockWidth; // The width of an ImageBlock.

    private final Bitmap mOutline; // The outline bitmap put on top of each

    // image.
    private final int mCount; // Cache mImageList.getCount().

    private final int mRows; // Cache (mCount + mColumns - 1) / mColumns

    private final int mBlockHeight; // The height of an ImageBlock.

    // Visible row range: [mStartRow, mEndRow). Set by setVisibleRows().
    private int mStartRow = 0;

    private int mEndRow = 0;

    private boolean isVisibleDone = false;

    private boolean isDebugPrint = false;

    private boolean mIsTimeMode = false;
    private TreeMap<Integer, TimeModeItem> mTitleMap;
    private TreeMap<Integer, Integer> mTimeMap;

    ImageBlockManager(Handler handler, Runnable redrawCallback, ArrayList<IImage> imageList,
            ImageLoader loader, GridViewSpecial.DrawAdapter adapter, LayoutSpec spec,
            int columns, int rows, int blockWidth, Bitmap outline, boolean isTimeMode,
            TreeMap<Integer, TimeModeItem> titleMap, TreeMap<Integer, Integer> timeMap) {
        mHandler = handler;
        mRedrawCallback = redrawCallback;
        mImageList = imageList;
        mLoader = loader;
        mDrawAdapter = adapter;
        mSpec = spec;
        mColumns = columns;
        mBlockWidth = blockWidth;
        mOutline = outline;
        mBlockHeight = mSpec.mCellSpacing + mSpec.mCellHeight;
        mCount = imageList.size();
//        mRows = (mCount + mColumns - 1) / mColumns;
        mRows = rows;
        mCache = new HashMap<Integer, ImageBlock>();
        mPendingRequest = 0;
        mIsTimeMode = isTimeMode;
        mTitleMap = titleMap;
        mTimeMap = timeMap;
        // Performance probe
        // QuIC Performance Profiling Log
        /*if (SystemProperties.getInt("debug.ui", 0) == 1) {
            isDebugPrint = true;
        }*/

        initGraphics();
    }

    // Set the window of visible rows. Once set we will start to load them as
    // soon as possible (if they are not already in cache).
    public void setVisibleRows(int startRow, int endRow) {
        if (startRow != mStartRow || endRow != mEndRow) {
            mStartRow = startRow;
            mEndRow = endRow;
            startLoading();
        }
    }

    int mPendingRequest; // Number of pending requests (sent to ImageLoader).

    // We want to keep enough requests in ImageLoader's queue, but not too
    // many.
    static final int REQUESTS_LOW = 3;

    static final int REQUESTS_HIGH = 6;

    // After clear requests currently in queue, start loading the thumbnails.
    // We need to clear the queue first because the proper order of loading
    // may have changed (because the visible region changed, or some images
    // have been invalidated).
    private void startLoading() {
        isVisibleDone = false;
        clearLoaderQueue();
        continueLoading();
    }

    private void clearLoaderQueue() {
        int[] tags = mLoader.clearQueue();
        for (int pos : tags) {
            int row = pos / mColumns;
            int col = pos - row * mColumns;
            if(mIsTimeMode) {
                row = findRowByPos(pos);
                col = findColumnByPos(row, pos);
            }
            ImageBlock blk = mCache.get(row);
            Assert(blk != null); // We won't reuse the block if it has pending
            // requests. See getEmptyBlock().
            blk.cancelRequest(col);
        }
    }

    public int findRowByPos(int pos) {
        int row = 0;
        int curCount = 0;
        for(Integer rowIndex: mTimeMap.keySet()) {
            curCount += mTimeMap.get(rowIndex);
            if(pos < curCount) {
                row = rowIndex;
                break;
            }
        }
        return row;
    }

    public int findColumnByPos(int row, int pos) {
        int count = 0;
        for(Integer index : mTimeMap.keySet()) {
            if(index == row) {
                break;
            }
            count += mTimeMap.get(index);
        }
        int column = Math.max(pos - count, 0);
        return column;
    }

    public int findPosByRow(int row) {
        int pos = 0;
        for(Integer index : mTimeMap.keySet()) {
            if(index == row) {
                break;
            }
            pos += mTimeMap.get(index);
        }
        return pos;
    }

    // Scan the cache and send requests to ImageLoader if needed.
    private void continueLoading() {
        // Check if we still have enough requests in the queue.
        if (mPendingRequest >= REQUESTS_LOW) {
            return;
        }

        // Scan the visible rows.
        for (int i = mStartRow; i < mEndRow; i++) {
            if (scanOne(i)) {
                return;
            }
        }

        // Performance probe
        // QuIC Performance Profiling Log
        isVisibleDone = true;

        int range = (CACHE_ROWS - (mEndRow - mStartRow)) / 2;
        // Scan other rows.
        // d is the distance between the row and visible region.
        for (int d = 1; d <= range; d++) {
            int after = mEndRow - 1 + d;
            int before = mStartRow - d;
            if (after >= mRows && before < 0) {
                break; // Nothing more the scan.
            }
            if (after < mRows && scanOne(after)) {
                return;
            }
            if (before >= 0 && scanOne(before)) {
                return;
            }
        }
    }

    // Returns true if we can stop scanning.
    private boolean scanOne(int i) {
        mPendingRequest += tryToLoad(i);
        return mPendingRequest >= REQUESTS_HIGH;
    }

    // Returns number of requests we issued for this row.
    private int tryToLoad(int row) {
        Assert(row >= 0 && row < mRows);
        ImageBlock blk = mCache.get(row);
        if (blk == null) {
            // Find an empty block
            try {
                blk = getEmptyBlock();
            } catch (OutOfMemoryError ex) {
                // Out of memory
                return 0;
            /* SPRD: fix bug 552487, when this view's width is zero in monkey tests, it will cause a
                IllegalArgumentException, so catch this exception. About influence, it will make
                PuzzleImagePicker's image view has white background when cause this exception @{ */
            } catch (IllegalArgumentException iae) {
                Log.e(TAG, "tryToLoad: maybe create a bitmap failed", iae);
                return 0;
            /* @} */
            }
            blk.setRow(row);
            blk.invalidate();
            mCache.put(row, blk);
        }
        return blk.loadImages();
    }

    // Get an empty block for the cache.
    private ImageBlock getEmptyBlock() {
        // See if we can allocate a new block.
        if (mCache.size() < CACHE_ROWS) {
            return new ImageBlock();
        }
        // Reclaim the old block with largest distance from the visible region.
        int bestDistance = -1;
        int bestIndex = -1;
        for (int index : mCache.keySet()) {
            // Make sure we don't reclaim a block which still has pending
            // request.
            if (mCache.get(index).hasPendingRequests()) {
                continue;
            }
            int dist = 0;
            if (index >= mEndRow) {
                dist = index - mEndRow + 1;
            } else if (index < mStartRow) {
                dist = mStartRow - index;
            } else {
                // Inside the visible region.
                continue;
            }
            if (dist > bestDistance) {
                bestDistance = dist;
                bestIndex = index;
            }
        }
        mCache.remove(bestIndex);
        return new ImageBlock();
    }

    public void invalidateImage(int index) {
        int row = 0;
        int col = 0;
        if(mIsTimeMode) {
            row = findRowByPos(index);
            col = findColumnByPos(row, index);
        } else {
            row = index / mColumns;
            col = index - (row * mColumns);
        }

        ImageBlock blk = mCache.get(row);
        if (blk == null) {
            return;
        }
        if ((blk.mCompletedMask & (1 << col)) != 0) {
            blk.mCompletedMask &= ~(1 << col);
        }
        startLoading();
    }

    // After calling recycle(), the instance should not be used anymore.
    public void recycle() {
        for (ImageBlock blk : mCache.values()) {
            blk.recycle();
        }
        mCache.clear();
        mEmptyBitmap.recycle();
        if(mIsTimeMode) {
            mTitleBitmap.recycle();
        }
    }

    // Draw the images to the given canvas.
    public void doDraw(Canvas canvas, int thisWidth, int thisHeight, int scrollPos) {
        final int height = mBlockHeight;

        // Note that currentBlock could be negative.
        int currentBlock = (scrollPos < 0) ? ((scrollPos - height + 1) / height)
                : (scrollPos / height);

        while (true) {
            final int yPos = currentBlock * height;
            if (yPos >= scrollPos + thisHeight) {
                break;
            }

            ImageBlock blk = mCache.get(currentBlock);
            if (blk != null) {
                blk.doDraw(canvas, 0, yPos);
            } else {
                drawEmptyBlock(canvas, 0, yPos, currentBlock);
            }

            currentBlock += 1;
        }
    }

    // Return number of columns in the given row. (This could be less than
    // mColumns for the last row).
    public int numColumns(int row) {
        if(mIsTimeMode && mTitleMap != null) {
            int cols = -1;
            if(mTitleMap.containsKey(row)) {
                cols = Math.min(mColumns, 1);
            } else if(mTimeMap.containsKey(row)) {
                cols = Math.min(mColumns, mTimeMap.get(row));
            }
            return cols;
        }
        return Math.min(mColumns, mCount - row * mColumns);
    }

    // Draw a block which has not been loaded.
    private void drawEmptyBlock(Canvas canvas, int xPos, int yPos, int row) {
        // Draw the background.
        canvas.drawRect(xPos, yPos, xPos + mBlockWidth, yPos + mBlockHeight, mBackgroundPaint);

        // Draw the empty images.
        int x = xPos + mSpec.mLeftEdgePadding;
        int y = yPos + mSpec.mCellSpacing;
        int cols = numColumns(row);
        if(mIsTimeMode && mTitleMap != null && mTitleMap.containsKey(row)) {
            x = xPos;
        }

        for (int i = 0; i < cols; i++) {
            /* SPRD: CID 108953 : Dereference after null check (FORWARD_NULL) @{ */
            if(mIsTimeMode && mTitleMap != null && mTitleMap.containsKey(row)) {
            // if(mIsTimeMode && mTitleMap.containsKey(row)) {
            /* @} */
                continue;
            } else {
                canvas.drawBitmap(mEmptyBitmap, x, y, null);
                x += (mSpec.mCellWidth + mSpec.mCellSpacing);
            }
        }
    }

    // mEmptyBitmap is what we draw if we the wanted block hasn't been loaded.
    // (If the user scrolls too fast). It is a gray image with normal outline.
    // mBackgroundPaint is used to draw the (black) background outside
    // mEmptyBitmap.
    Paint mBackgroundPaint;
    Paint mCountPaint;
    Paint mTitlePaint;

    private Bitmap mEmptyBitmap;
    private Bitmap mTitleBitmap;

    private void initGraphics() {
        mBackgroundPaint = new Paint();
        mBackgroundPaint.setStyle(Paint.Style.FILL);
        mBackgroundPaint.setColor(0x00ffffff);
        mEmptyBitmap = Bitmap.createBitmap(mSpec.mCellWidth, mSpec.mCellHeight,
                Bitmap.Config.RGB_565);
        Canvas canvas = new Canvas(mEmptyBitmap);
//        canvas.drawRGB(0xDD, 0xDD, 0xDD);
        canvas.drawRGB(0xe1, 0xe2, 0xe3);
        if(mOutline != null && !mOutline.isRecycled()) {
            canvas.drawBitmap(mOutline, 0, 0, null);
        }
        if(mIsTimeMode) {
            mTitleBitmap = Bitmap.createBitmap(mBlockWidth, mBlockHeight, Bitmap.Config.ARGB_8888);
            Canvas titleCanvas = new Canvas(mTitleBitmap);
//            titleCanvas.drawRGB(0xe1, 0xe2, 0xe3);
            //black background .
//            titleCanvas.drawRGB(0x00, 0x00, 0x00);
            titleCanvas.drawColor(Color.TRANSPARENT);
            mTitlePaint = new Paint();
            mTitlePaint.setTextSize(mSpec.mTitleSize);
            mTitlePaint.setAntiAlias(true);
//            mTitlePaint.setTextAlign(Paint.Align.CENTER);
            mTitlePaint.setTypeface(Typeface.SANS_SERIF);
            mTitlePaint.setColor(Color.WHITE);

            mCountPaint = new Paint();
            mCountPaint.setTextSize(mSpec.mCountSize);
            mCountPaint.setAntiAlias(true);
//            mCountPaint.setTextAlign(Paint.Align.CENTER);
            mCountPaint.setTypeface(Typeface.DEFAULT_BOLD);
            mCountPaint.setColor(Color.WHITE);
        }
    }

    /**
     * ImageBlock stores bitmap for one row. The loaded thumbnail images are
     * drawn to mBitmap. mBitmap is later used in onDraw() of GridViewSpecial.
     *
     *
     */
    private class ImageBlock {
        private Bitmap mBitmap;

        private final Canvas mCanvas;

        // Columns which have been requested to the loader
        private int mRequestedMask;

        // Columns which have been completed from the loader
        private int mCompletedMask;

        // The row number this block represents.
        private int mRow;

        public ImageBlock() {
            mBitmap = Bitmap.createBitmap(mBlockWidth, mBlockHeight, Bitmap.Config.ARGB_8888);
            mCanvas = new Canvas(mBitmap);
            mRow = -1;
        }

        public void setRow(int row) {
            mRow = row;
        }

        public void invalidate() {
            // We do not change mRequestedMask or do cancelAllRequests()
            // because the data coming from pending requests are valid. (We only
            // invalidate data which has been drawn to the bitmap).
            mCompletedMask = 0;
        }

        // After recycle, the ImageBlock instance should not be accessed.
        public void recycle() {
            cancelAllRequests();
            mBitmap.recycle();
            mBitmap = null;
        }

        private boolean isVisible() {
            return mRow >= mStartRow && mRow < mEndRow;
        }

        // Returns number of requests submitted to ImageLoader.
        public int loadImages() {
            Assert(mRow != -1);

            int columns = numColumns(mRow);

            // Calculate what we need.
            int needMask = ((1 << columns) - 1) & ~(mCompletedMask | mRequestedMask);

            if (needMask == 0) {
                return 0;
            }

            int retVal = 0;
            if(mIsTimeMode && mTitleMap.containsKey(mRow)) {
                if ((needMask & (1 << columns)) == 0) {
                    return retVal;
                }
                mRequestedMask |= (1 << columns);
                retVal += 1;
                return retVal;
            }

            int base = 0;
            if(mIsTimeMode) {
                for(int index = 1; index <= mRow; index++) {
                    if(mTimeMap.containsKey(index)) {
                        base += mTimeMap.get(index);
                    }
                }
                base = base - columns;
            } else {
                base = mRow * mColumns;
            }
            for (int col = 0; col < columns; col++) {
                if ((needMask & (1 << col)) == 0) {
                    continue;
                }

                int pos = base + col;
                if(pos >= mCount) {
                    continue;
                }
                final IImage image = mImageList.get(pos);
                if (image != null) {
                    // This callback is passed to ImageLoader. It will invoke
                    // loadImageDone() in the main thread. We limit the callback
                    // thread to be in this very short function. All other
                    // processing is done in the main thread.
                    final int colFinal = col;
                    ImageLoader.LoadedCallback cb = new ImageLoader.LoadedCallback() {
                        public void run(final Bitmap b) {
                            mHandler.post(new Runnable() {
                                public void run() {
                                    loadImageDone(image, b, colFinal);
                                }
                            });
                        }
                    };
                    // Load Image
                    mLoader.getBitmap(image, cb, pos);
                    mRequestedMask |= (1 << col);
                    retVal += 1;
                }
            }

            return retVal;
        }

        // Whether this block has pending requests.
        public boolean hasPendingRequests() {
            return mRequestedMask != 0;
        }

        // Called when an image is loaded.
        private void loadImageDone(IImage image, Bitmap b, int col) {
            if (mBitmap == null) {
                return; // This block has been recycled.
            }

            int spacing = mSpec.mCellSpacing;
            int leftSpacing = mSpec.mLeftEdgePadding;
            final int yPos = spacing;
            final int xPos = leftSpacing + (col * (mSpec.mCellWidth + spacing));
            drawBitmap(image, b, xPos, yPos);

            if (b != null) {
                b.recycle();
            }

            int mask = (1 << col);
            /**
             * SPRD: Replace for bug 559971 @{
            Assert((mCompletedMask & mask) == 0);
            Assert((mRequestedMask & mask) != 0);
             */
            if ((mCompletedMask & mask) != 0 || (mRequestedMask & mask) == 0) return;
            /* @} */
            mRequestedMask &= ~mask;
            mCompletedMask |= mask;
            mPendingRequest--;

            if (isVisible()) {
                mRedrawCallback.run();
                // Performance probe
                // QuIC Performance Profiling Log
                if (isVisibleDone && isDebugPrint) {
                    Log.d(TAG, "One of the last few visible images is redrawn");
                }
            }
            // Kick start next block loading.
            continueLoading();
        }

        // Draw the loaded bitmap to the block bitmap.
        private void drawBitmap(IImage image, Bitmap b, int xPos, int yPos) {
            mDrawAdapter.drawImage(mCanvas, image, b, xPos, yPos, mSpec.mCellWidth,
                    mSpec.mCellHeight);
            if(!BaseImagePicker.mIsLongPressed || image != mImageList.get(GridViewSpecial.mSelectItem)){
                if(mOutline != null && !mOutline.isRecycled()) {
                    mCanvas.drawBitmap(mOutline, xPos, yPos, null);
                }
                mDrawAdapter.drawPlayIcon(mCanvas, image, b, xPos, yPos, mSpec.mCellWidth,mSpec.mCellHeight);
            }
        }

        // Draw the block bitmap to the specified canvas.
        public void doDraw(Canvas canvas, int xPos, int yPos) {
            /*
             * BUG FIX: 1534
             * FIX COMMENT: Null
             * Date: 2012-08-30
             */
            if (mBitmap == null) {
                return;
            }
            int cols = numColumns(mRow);

            if (cols == mColumns) {
                canvas.drawBitmap(mBitmap, xPos, yPos, null);
            } else {
                if(mIsTimeMode && mTitleMap.containsKey(mRow)) {
                    canvas.drawRect(xPos, yPos, xPos + mBlockWidth, yPos + mBlockHeight, mBackgroundPaint);
//                    int w = mSpec.mLeftEdgePadding + cols * (mSpec.mCellWidth + mSpec.mCellSpacing);
                    Rect srcRect = new Rect(0, 0, mBlockWidth, mBlockHeight);
                    Rect dstRect = new Rect(srcRect);
                    dstRect.offset(xPos, yPos);
                    canvas.drawBitmap(mBitmap, srcRect, dstRect, null);
                } else {
                    // This must be the last row -- we draw only part of the block.
                    // Draw the background.
                    canvas.drawRect(xPos, yPos, xPos + mBlockWidth, yPos + mBlockHeight, mBackgroundPaint);
                    // Draw part of the block.
                    int w = mSpec.mLeftEdgePadding + cols * (mSpec.mCellWidth + mSpec.mCellSpacing);
                    Rect srcRect = new Rect(0, 0, w, mBlockHeight);
                    Rect dstRect = new Rect(srcRect);
                    dstRect.offset(xPos, yPos);
                    canvas.drawBitmap(mBitmap, srcRect, dstRect, null);
                }
            }

            // Draw the part which has not been loaded.
            int isEmpty = ((1 << cols) - 1) & ~mCompletedMask;

            if (isEmpty != 0) {
                int x = xPos + mSpec.mLeftEdgePadding;
                int y = yPos + mSpec.mCellSpacing;
                if(mIsTimeMode && mTitleMap.containsKey(mRow)) {
                    x = xPos;
                }

                for (int i = 0; i < cols; i++) {
                    if ((isEmpty & (1 << i)) != 0) {
                        if(mIsTimeMode && mTitleMap.containsKey(mRow)) {
                            canvas.drawBitmap(mTitleBitmap, x, y, null);
                            drawTitle(canvas, mRow, x, y);
                        } else {
                            canvas.drawBitmap(mEmptyBitmap, x, y, null);
                        }
                    }
                    x += (mSpec.mCellWidth + mSpec.mCellSpacing);
                }
            }
        }

        private void drawTitle(Canvas canvas, int row, float x, float y) {
            TimeModeItem item = mTitleMap.get(row);
            String count = String.valueOf(item.getPictureCount());
            String week = item.getWeek();
            String date = item.getDate();
            float countLenDate = mCountPaint.measureText(String.valueOf(date));
            canvas.drawText(count, x + mSpec.mLeftEdgePadding , y + mSpec.mCellSpacing + mBlockHeight / 2, mCountPaint);
            canvas.drawText(date,  x + mSpec.mLeftEdgePadding + mBlockHeight , y + mSpec.mCellSpacing + mBlockHeight / 2, mTitlePaint);
            canvas.drawText(week,  x + mSpec.mLeftEdgePadding + mBlockHeight +  + countLenDate/2, y + mSpec.mCellSpacing + mBlockHeight /2, mTitlePaint);
            Paint paint = new Paint();
            paint.setStyle(Paint.Style.FILL);
            paint.setStrokeWidth(1);
            paint.setAntiAlias(true);
            paint.setColor(Color.parseColor("#636158"));
            canvas.drawLine(x + mSpec.mLeftEdgePadding, y + mSpec.mCellSpacing + mBlockHeight * 2/3 , mSpec.mMetrics.widthPixels -(x + mSpec.mLeftEdgePadding), y + mSpec.mCellSpacing + mBlockHeight * 2/3, paint);
        }

        // Mark a request as cancelled. The request has already been removed
        // from the queue of ImageLoader, so we only need to mark the fact.
        public void cancelRequest(int col) {
            int mask = (1 << col);
            Assert((mRequestedMask & mask) != 0);
            mRequestedMask &= ~mask;
            mPendingRequest--;
        }

        // Try to cancel all pending requests for this block. After this
        // completes there could still be requests not cancelled (because it is
        // already in progress). We deal with that situation by setting mBitmap
        // to null in recycle() and check this in loadImageDone().
        private void cancelAllRequests() {
            if(mIsTimeMode) {
                int columns = numColumns(mRow);
                for(int j = 0; j < columns; j++) {
                    int mask = (1 << j);
                    if ((mRequestedMask & mask) != 0) {
//                        int pos = (mRow * mColumns) + j;
                        int pos = findPosByRow(mRow) + j;
                        if (mLoader.cancel(mImageList.get(pos))) {
                            mRequestedMask &= ~mask;
                            mPendingRequest--;
                        }
                    }
                }
            } else {
                for (int i = 0; i < mColumns; i++) {
                    int mask = (1 << i);
                    if ((mRequestedMask & mask) != 0) {
                        int pos = (mRow * mColumns) + i;
                        if (mLoader.cancel(mImageList.get(pos))) {
                            mRequestedMask &= ~mask;
                            mPendingRequest--;
                        }
                    }
                }
            }

        }
    }
}
