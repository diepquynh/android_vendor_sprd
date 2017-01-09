/*
 * Copyright (C) 2014,2015 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.ugallery;


import java.util.HashMap;

import com.ucamera.ugallery.gallery.IImage;
import com.ucamera.ugallery.gallery.IImageList;
import com.ucamera.ugallery.gallery.ImageLoader;
import com.ucamera.ugallery.util.UiUtils;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Bitmap.Config;
import android.graphics.Paint.Style;
import android.graphics.Rect;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.view.GestureDetector.SimpleOnGestureListener;
import android.view.GestureDetector;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Scroller;

public class ThumbnailGridView extends View {
    private IImageList mImageList;
    private ImageLoader mLoader;
    private int WIDTH = 0;
    private int HEIGHT = 0;
    private int VIEW_TOP = 0;
    private int PADDING_LEFT = 0;
    private static final float MAX_FLING_VELOCITX = 2500;
    public static int SELECT_NONE = -1;
    private Scroller mScroller = null;
    private int mCurrentSelection = SELECT_NONE;
    private Paint mPaint;
    public static interface DrawAdapter {
        void drawImage(Canvas canvas, IImage image, Bitmap b, int xPos, int yPos, int w, int h);
    }
    public static interface Listener {
        void onImageTapped(int index);
        void onLayoutComplete(boolean changed, int width);
        void onScroll(float scrollPosition);
    }
    private Listener mListener = null;
    private DrawAdapter mDrawAdapter = null;
    private ThumbnailBlockManager mBlockManager;
    private Handler mHandler = new Handler();
    private int mCount;
    private int mColumns;
    private GestureDetector mGesture;
    private int mMaxScrollX;
    private boolean mLayoutComplete = false;
    private boolean mRunning = false;
    private boolean mLoadComplete = false;
    private LayoutSpec mSpec;

    public ThumbnailGridView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }
    private void init(Context context) {
        setHorizontalScrollBarEnabled(true);
        setFocusableInTouchMode(true);
        mGesture = new GestureDetector(context, new ThumbnailGustureListener());
        initCellSize();
        initPaint();
    }
    private void initPaint() {
        mPaint = new Paint();
        mPaint.setStyle(Style.STROKE);
        int width = UiUtils.dpToPixel(3);
        mPaint.setStrokeWidth(width);
        mPaint.setColor(0xff00aced);
    }
    private void initCellSize() {
        Activity a = (Activity) getContext();
        DisplayMetrics metrics = new DisplayMetrics();
        a.getWindowManager().getDefaultDisplay().getMetrics(metrics);
        mSpec = new LayoutSpec(70, 70, 0, 0, metrics);
        WIDTH = mSpec.getImageWidth();
        HEIGHT = mSpec.getImageHeight();
        VIEW_TOP = 0;
    }
    public void initCellTop(int barheight) {
        VIEW_TOP = 0;
    }
    public void setImageList(IImageList list) {
        mImageList = list;
        mCount = mImageList.getCount();
    }
    public void setImageLoader(ImageLoader loader) {
        mLoader = loader;
        mLoader.setImageCount(mCount);
    }
    public void setListener(Listener listener) {
        mListener = listener;
    }
    public void setDrawAdapter(DrawAdapter adapter) {
        mDrawAdapter = adapter;
    }
    public void onStart() {
        mRunning = true;
        mLoadComplete = false;
        requestLayout();
    }
    public void onStop() {
        mRunning = false;
        mLoadComplete = false;
        if(mBlockManager != null) {
            mBlockManager.recycle();
            mBlockManager = null;
        }
        SELECT_NONE = -1;
        PADDING_LEFT = 0;
    }
    private final Runnable mRedrawCallback = new Runnable() {
        public void run() {
            invalidate();
        }
    };
    public void setSelectIndex(int index) {
        if(mCurrentSelection == index) {
            return;
        }
        mCurrentSelection = index;
        if(mCurrentSelection != SELECT_NONE) {
            ensureVisible(index);
        }
        invalidate();
    }
    private void ensureVisible(int pos) {
        Rect r = getRectForPosition(pos);
        int left = getScrollX();
        int right = left + getWidth();
        if(r.left < left) {
            mScroller = new Scroller(getContext());
            mScroller.startScroll(getScrollX(), getScrollY(), r.left - left, 0, 200);
            computeScroll();
        }
        if(r.right > right) {
            mScroller = new Scroller(this.getContext());
            mScroller.startScroll(getScrollX(), getScrollY(), r.right - right, 0, 200);
            computeScroll();
        }
    }
    private Rect getRectForPosition(int pos) {
        int left = pos * WIDTH;
        int right = (pos + 1) * WIDTH;
        int top = VIEW_TOP;
        int bottom = VIEW_TOP + HEIGHT;
        return new Rect(left, top, right, bottom);
    }
    private int computeSelectedIndex(float posX, float posY) {
        int x = (int)posX - PADDING_LEFT;
        if(x < 0) return -1;
        return (getScrollX() + x) / WIDTH;
    }

    public void updateSelectedItem() {
        /*
         * FIX BUG: 6025
         * BUG CAUSE:  Null pointer exception;
         * DATE: 2013-03-05
         */
        if(mBlockManager != null) {
            mBlockManager.rotateThumbnail(mCurrentSelection);
        }
//        mBlockManager.scanOne(mCurrentSelection);
    }

    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);
        if(!mRunning) {
            return;
        }
        if(mLoadComplete) {
            return;
        }
        mColumns = mCount;
        mMaxScrollX = WIDTH * mColumns - (right - left);
        if((right - left) - mColumns * WIDTH > 0) {
            PADDING_LEFT = ((right - left) - mColumns * WIDTH) / 2;
        }
        if(mBlockManager != null) {
            mBlockManager.recycle();
        }
        mBlockManager = new ThumbnailBlockManager(mHandler, mRedrawCallback, mLoader, mImageList);
        moveVisibleWindow();
        mLayoutComplete = true;
        mLoadComplete = true;
        if(mListener != null) {
            mListener.onLayoutComplete(changed, right - left);
        }
    }
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        setMeasuredDimension(UiUtils.screenWidth(), HEIGHT);
    }
    @Override
    protected int computeHorizontalScrollRange() {
        return mMaxScrollX + getWidth();
    }
    public void computeScroll() {
        if (mScroller != null) {
            boolean more = mScroller.computeScrollOffset();
            scrollTo(mScroller.getCurrX(), 0);
            if (more) {
                invalidate();  // So we draw again
            } else {
                mScroller = null;
            }
        } else {
            super.computeScroll();
        }
    }
    public boolean onTouchEvent(MotionEvent event) {
        if(!canHandleEvent()) return false;
        switch (event.getAction()) {
        case MotionEvent.ACTION_DOWN:
            invalidate();
            break;
        case MotionEvent.ACTION_UP:
            invalidate();
            break;
        }
        mGesture.onTouchEvent(event);
        return true;
    }
    class ThumbnailGustureListener extends SimpleOnGestureListener {
        public boolean onSingleTapUp(MotionEvent e) {
            if(!canHandleEvent()) return false;
            int index = computeSelectedIndex(e.getX(), e.getY());
            if(index >= 0 && index < mColumns) {
                mListener.onImageTapped(index);
                return true;
            } else {
                return false;
            }
        }

        public boolean onScroll(MotionEvent e1, MotionEvent e2, float distanceX, float distanceY) {
            if(!canHandleEvent()) return false;
            scrollBy((int)distanceX, 0);
            invalidate();
            return true;
        }

        public boolean onFling(MotionEvent e1, MotionEvent e2, float velocityX, float velocityY) {
            if(!canHandleEvent()) return false;
            if (velocityY > MAX_FLING_VELOCITX) {
                velocityY = MAX_FLING_VELOCITX;
            } else if (velocityY < -MAX_FLING_VELOCITX) {
                velocityY = -MAX_FLING_VELOCITX;
            }
            mScroller = new Scroller(getContext());
            mScroller.fling(getScrollX(), 0, -(int) velocityX, 0, 0, mMaxScrollX, 0,0);
            computeScroll();
            return true;
        }

        public boolean onDown(MotionEvent e) {
            if(!canHandleEvent()) return false;
            if(mScroller != null && !mScroller.isFinished()) {
                mScroller.forceFinished(true);
                return false;
            }
            return super.onDown(e);
        }
    }
    private boolean canHandleEvent() {
        return mRunning && mLayoutComplete;
    }
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if(!canHandleEvent()) return;
        if(mBlockManager != null) {
            mBlockManager.doDraw(canvas, getWidth(), getHeight(), getScrollX());
            drawSelection(canvas);
            moveVisibleWindow();
        }
    }
    private void drawSelection(Canvas canvas) {
        if(mCurrentSelection != SELECT_NONE) {
            Rect r = new Rect(mCurrentSelection * WIDTH + PADDING_LEFT, VIEW_TOP, mCurrentSelection * WIDTH + WIDTH + PADDING_LEFT, VIEW_TOP + HEIGHT);
            int insert = UiUtils.dpToPixel(1);
            r.inset(insert, insert);
            canvas.drawRect(r, mPaint);
        }
    }
    public void scrollTo(float x) {
        scrollTo(0, Math.round(x * mMaxScrollX));
    }
    public void scrollToVisible(int index) {
        Rect r = getRectForPosition(index);
        int left = getScrollX();
        int right = getScrollX() + getWidth();
        if (r.right > right) {
            scrollTo(r.right - getWidth(), 0);
        } else if (r.left < left) {
            scrollTo(r.left, 0);
        }
    }
    public void scrollToImage() {}
    public void scrollTo(int x, int y) {
        x = Math.max(0, Math.min(x, mMaxScrollX));
        mListener.onScroll((float)x / mMaxScrollX);
        super.scrollTo(x, y);
    }
    public void scrollBy(int x, int y) {
        super.scrollBy(x, y);
    }
    private void moveVisibleWindow() {
        int startColumn = getScrollX() / WIDTH;
        int endColumn = (getScrollX() + getWidth()) / WIDTH + 1;
        startColumn = Math.max(0, Math.min(mColumns -1, startColumn));
        endColumn = Math.max(0, Math.min(mColumns, endColumn));
        mBlockManager.setVisibleColumns(startColumn, endColumn);
    }
    class ThumbnailBlockManager {
        private static final int COLUMN_CACHE = 100;
        private static final int REQUEST_LOW = 30;
        private static final int REQUEST_HIGH = 40;
        private Handler mHandler;
        private int mPendingRequest;
        private int mStartColumn = 0;
        private int mEndColumn = 0;
        private Bitmap mEmptyBitmap;
        private Runnable mRedrawCallback;
        private HashMap<Integer, ThumbnailBlock> mCache;
        private ImageLoader mLoader;
        private IImageList mImageList;
        public ThumbnailBlockManager(Handler handler, Runnable mRedrawCallback, ImageLoader loader, IImageList list) {
            mHandler = handler;
            this.mRedrawCallback = mRedrawCallback;
            mLoader = loader;
            mImageList = list;
            mPendingRequest = 0;
            mCache = new HashMap<Integer, ThumbnailGridView.ThumbnailBlockManager.ThumbnailBlock>();
            initCell();
        }
        private void initCell() {
            mEmptyBitmap = Bitmap.createBitmap(WIDTH, HEIGHT, Bitmap.Config.RGB_565);
            Canvas canvas = new Canvas(mEmptyBitmap);
            canvas.drawRGB(0xff, 0xff, 0xff);
        }
        public void recycle() {
            for(ThumbnailBlock block : mCache.values()) {
                block.recycle();
            }
            mCache.clear();
            if(mEmptyBitmap != null && !mEmptyBitmap.isRecycled()) {
                mEmptyBitmap.recycle();
                mEmptyBitmap = null;
            }
        }
        private void clearLoaderQueue() {
            if(mLoader == null) return;
            int[] tags = mLoader.clearQueue();
            for (int pos : tags) {
                ThumbnailBlock blk = mCache.get(pos);
//                Assert(blk != null); // We won't reuse the block if it has pending
                // requests. See getEmptyBlock().
                if(blk != null) {
                    blk.recycleRequest();
                }else {
                    mCache.remove(pos);
                }
            }
        }
        public void setVisibleColumns(int start, int end) {
            if(mStartColumn != start || mEndColumn != end) {
                mStartColumn = start;
                mEndColumn = end;
                startLoading();
            }
        }
        private void startLoading() {
            clearLoaderQueue();
            continueLoading();
        }
        private void continueLoading() {
            if(mPendingRequest >= REQUEST_LOW) {
                return;
            }
            for(int i = mStartColumn; i < mEndColumn; i++) {
                if(scanOne(i)) {
                    return;
                }
            }
            int range = (COLUMN_CACHE - (mEndColumn - mStartColumn)) / 2;
            for(int i = 1; i <= range; i++) {
                int left = mStartColumn - i;
                int right = mEndColumn + i - 1;
                if(left < 0 && right >= mColumns) {
                    break;
                }
                if(left >= 0 && scanOne(left)) {
                    return;
                }
                if(right < mColumns && scanOne(right)) {
                    return;
                }
            }
        }
        public void rotateThumbnail(int column) {
            ThumbnailBlock thumb = mCache.get(column);
            if(thumb != null) {
                thumb.reset();
                mPendingRequest += thumb.loadImage();
            }
        }
        private boolean scanOne(int column) {
            mPendingRequest += tryToLoad(column);
            return mPendingRequest >= REQUEST_HIGH;
        }

        private int tryToLoad(int column) {
            ThumbnailBlock thumb = mCache.get(column);
            if(thumb == null) {
                try{
                    thumb = getEmptyBlock();
                }catch(OutOfMemoryError error) {
                    return 0;
                }
                thumb.setColumn(column);
                mCache.put(column, thumb);
                thumb.reset();
            }
            return thumb.loadImage();
        }
        private ThumbnailBlock getEmptyBlock() {
            if(mCache.size() < COLUMN_CACHE) {
                return new ThumbnailBlock();
            }
            int bestDistance = -1;
            int bestIndex = -1;
            for(int index : mCache.keySet()) {
                if(mCache.get(index).hasPendingRequest()) {
                    continue;
                }
                int dst = 0;
                if(index >= mEndColumn) {
                    dst = index + 1 - mEndColumn;
                }else if(index < mStartColumn) {
                    dst = mStartColumn - index;
                } else {
                    continue;
                }
                if(dst > bestDistance) {
                    bestDistance = dst;
                    bestIndex = index;
                }
            }
            if(bestIndex != -1) {
                mCache.remove(bestIndex);
            }
//            if(bestIndex != -1) {
//                return mCache.remove(bestIndex);
//            }else {
                return new ThumbnailBlock();
//            }
//            mCache.remove(bestIndex);
//            return new ThumbnailBlock();
        }
        public void doDraw(Canvas canvas, int viewWidth, int viewHeight, int scrollX) {
            int width = WIDTH;
            int currentBlock = scrollX < 0 ? ((scrollX - width + 1) / width) : (scrollX / width);
            while(true) {
                int pos = currentBlock * width + PADDING_LEFT;
                if(pos >= scrollX + viewWidth) {
                    return;
                }
                if(currentBlock >= mColumns) {
                    return;
                }
                ThumbnailBlock block = null;
                /*
                 * FIX BUG:5864
                 * FIX COMMETN: avoid null point exception
                 * DATE: 2014-01-21
                 */
                if(mCache != null) {
                    block = mCache.get(currentBlock);
                }
                if(block != null) {
                    block.doDraw(canvas, pos, VIEW_TOP);
                }else {
                    drawEmptyBlock(canvas, pos, VIEW_TOP);
                }
                currentBlock++;
            }
        }
        private void drawEmptyBlock(Canvas canvas, int x, int y) {
            canvas.drawBitmap(mEmptyBitmap, x, y, null);
        }

        class ThumbnailBlock {
            private Bitmap mBitmap;
            private int mColumn;
            private boolean mRequestCompleted = false;
            private Canvas mCanvas;
            public ThumbnailBlock() {
                mBitmap = Bitmap.createBitmap(WIDTH, HEIGHT, Config.ARGB_8888);
                mCanvas = new Canvas(mBitmap);
                mColumn = -1;
            }
            public void setColumn(int column) {
                mColumn = column;
            }
            public boolean isVisible() {
                return mColumn >= mStartColumn && mColumn < mEndColumn;
            }
            public int loadImage() {
                if(mRequestCompleted) {
                    return 0;
                }
                final IImage image = mImageList.getImageAt(mColumn);
                int count = 0;
                if(image != null) {
                    ImageLoader.LoadedCallback cb = new ImageLoader.LoadedCallback() {
                        public void run(final Bitmap b) {
                            mHandler.post(new Runnable() {
                                public void run() {
                                    loadImageDone(image, b);
                                }
                            });
                        }
                    };
                    mLoader.getBitmap(image, cb, mColumn);
                    count++;
                }
                return count;
            }
            public void recycle() {
                cancelRequest();
                if(mBitmap != null && !mBitmap.isRecycled()) {
                    mBitmap.recycle();
                }
                mBitmap = null;
            }
            public void recycleRequest() {
                mPendingRequest--;
            }
            public void cancelRequest() {
                if(mLoader.cancel(mImageList.getImageAt(mColumn))) {
                    mPendingRequest--;
                }
            }
            public void reset() {
                mRequestCompleted = false;
            }
            private void loadImageDone(IImage image, Bitmap bitmap) {
                if(mBitmap == null) {
                    return;
                }
                int xPos = mColumn * WIDTH;
                mDrawAdapter.drawImage(mCanvas, image, bitmap, xPos, VIEW_TOP, WIDTH, HEIGHT);
                if(bitmap != null) {
                    bitmap.recycle();
                }
                mRequestCompleted = true;
                mPendingRequest--;
                if (isVisible()) {
                    mRedrawCallback.run();
                }
//                continueLoading();
            }
            public boolean hasPendingRequest() {
                return !mRequestCompleted;
            }
            public void doDraw(Canvas canvas, int posX, int posY) {
                if(mBitmap == null) {
                    return;
                }
                canvas.drawBitmap(mBitmap, posX, posY, null);
            }
        }
    }
}
