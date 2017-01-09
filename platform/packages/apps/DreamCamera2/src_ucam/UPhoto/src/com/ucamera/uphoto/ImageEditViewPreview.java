/**
 * Copyright (C) 2010,2011 Thundersoft Corporation
 * All rights Reserved
 */
package com.ucamera.uphoto;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Message;
import android.os.Handler.Callback;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.Scroller;
/**
 *  The view is to preview the image for editing.
 */
public class ImageEditViewPreview extends ImageViewTouchBase {
    private final static String TAG = "ImageEditViewPreview";
    protected ScrollController mScroller;   // mainly used to control the image' translation for its being zoom
    protected Bitmap bm;
    private Context mContext;
    private boolean initView = true; // flag to reconstruct hierarchy of view's parent
    protected ImageEditViewInputProcessor mInputProcessor;  // mainly to be in charge of touch event
    private boolean performRefresh;
    private boolean isTouchable;
    private boolean isCertainTouchable;
    private int mRotation;

    public ImageEditViewPreview(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        mInputProcessor = new ImageEditViewInputProcessor(this, context);
        setCertainTouchable(true);
        setTouchable(true);
    }


    public ImageEditViewPreview(Context context) {
        super(context);
        mContext = context;
        mInputProcessor = new ImageEditViewInputProcessor(this, context);
        setCertainTouchable(true);
        setTouchable(true);
    }

    public int postTranslateCenter(float dx, float dy) {
        super.postTranslate(dx, dy);
        return center(true, true);
    }

    protected ScrollController getScrollHandler() {
        return mScroller;
    }

    public boolean isCertainTouchable() {
        return isCertainTouchable;
    }

    public void setMinZoom(boolean bSetMinZoom, float fMinZoom) {
        mInputProcessor.setMinZoom(bSetMinZoom, fMinZoom);
    }

    public void setCertainTouchable(boolean isCertainTouchable) {
        this.isCertainTouchable = isCertainTouchable;
    }

    public boolean isTouchable() {
        return isTouchable;
    }

    public void setTouchable(boolean isTouchable) {
        this.isTouchable = isTouchable;
    }

    public Matrix getSuppMatrix(){
        return mSuppMatrix;
    }

    public float getScale() {
        return super.getScale();
    }

    public Bitmap getBitmap(){
        return bm;
    }

    @Override
    public void setImageBitmap(Bitmap bitmap) {
        Log.d(TAG, "setImageBitmap(p1): bitmap = " + bitmap + ", bm = " + bm);
        bm = bitmap;
        super.setImageBitmap(bitmap);
    }

    @Override
    public void setImageBitmap(Bitmap bitmap, int rotation) {
        Log.d(TAG, "setImageBitmap(p1, p2): rotation = " + rotation + ", bitmap = " + bitmap + ", bm = " + bm);
        bm = bitmap;
        mRotation = rotation;
        super.setImageBitmap(bitmap, rotation);
    }


    @Override
    protected void onAttachedToWindow() {
        if(initView){
            initView = false;
            ViewGroup parent = (ViewGroup) getParent();
            int index = parent.indexOfChild(this);
            parent.removeView(this);
            mScroller = new ScrollController(mContext);
            mScroller.addView(this);
            parent.addView(mScroller, index);
        }
        super.onAttachedToWindow();
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if(isCertainTouchable){
            mInputProcessor.dispatchTouchEvent(event);
        }
        return super.dispatchTouchEvent(event);
    }

    boolean performFrontOperator;
    @Override
    protected void onDraw(Canvas canvas) {
        if(getBitmap() == null || getBitmap().isRecycled()){
            return;
        }
        try {
            super.onDraw(canvas);
            if(performRefresh){
                invalidate();
            }
        }
        catch (Exception e){
            e.printStackTrace();
        }
    }

    @Override
    public void setImageDrawable(Drawable drawable) {
        super.setImageDrawable(drawable);
    }

    public boolean onTouchEvent(MotionEvent event) {
        mInputProcessor.onTouchEvent(event);
        return isTouchable;
    }

    public ScrollController getScroller() {
        return mScroller;
    }

    public void setBitmap() {
        if(bm != null && !bm.isRecycled()) {
            setImageRotateBitmapResetBase(new RotateBitmap(bm, mRotation), false);
        }
    }

    public Matrix getImageViewMatrix(){
        return super.getImageViewMatrix();
    }

    public int center(boolean horizontal, boolean vertical) {
        return super.center(horizontal, vertical);
    }

    public void populateMoveEvent(int deltaX) {
        if(getScale() > 1.0f) {
            mScroller.scrollTo(getLeft() + deltaX, 0);
        }
    }

    protected void scrollX(int deltaX) {
        mScroller.scrollBy(deltaX, 0);
    }

    protected int getScrollOffset() {
        return mScroller.getScrollX();
    }

    public void populateScale(){
        if(getScale() > 1F) {
            zoomTo(1f);
        } else {
            zoomTo(3f);
        }
    }

    public void resetScale(){
        if(getScale() > 1F) {
            zoomTo(1f);
        }
    }
    private float getBaseScale() {
        float viewWidth = getWidth();
        float viewHeight = getHeight();

        float w = bm.getWidth();
        float h = bm.getHeight();
        float widthScale = viewWidth / w;
        float heightScale = viewHeight / h;
        return Math.min(Math.min(widthScale, heightScale), 3);
    }

    public void pupulateScale(float centerX, float centerY, float durationMs){
        if(getScale() > 1F){
            zoomTo(1f, centerX, centerY, durationMs);
        }else{
//            zoomTo(3f, centerX, centerY, durationMs);
            // avoid null pointer exception
            if(bm != null) {
                zoomTo(getBaseScale(), centerX, centerY, durationMs);
            }
        }
    }

    public class ScrollController extends LinearLayout {
        private Scroller mScroller;
        private int mWidth = -1;
        static final int baseScrollDuration = 1000;

        static final int sPadding = 20;

        public ScrollController(Context context) {
            super(context);
            mScroller = new Scroller(context);
        }

        public ScrollController(Context context, AttributeSet attrs) {
            super(context, attrs);
            mScroller = new Scroller(context);
        }

        public void startScrollTo(int newX, int newY) {
            int oldX = getScrollX();
            int oldY = getScrollY();

            int deltaX = newX - oldX;
            int deltaY = newY - oldY;

            if (mWidth == -1) {
                mWidth = getWidth();
            }

            int width = mWidth;

            int duration = width > 0 ? baseScrollDuration * Math.abs(deltaX) / width : 0;
            mScroller.startScroll(oldX, oldY, deltaX, deltaY, duration);
            invalidate();
        }

        @Override
        public void computeScroll() {
            if (mScroller.computeScrollOffset()) {
                scrollTo(mScroller.getCurrX(), mScroller.getCurrY());
                postInvalidate();
            }
        }

        @Override
        protected void onLayout(boolean hasChanged, int left, int top, int right, int bottom) {
            ImageEditViewPreview imageView = ImageEditViewPreview.this;
            imageView.layout(left, top, right, bottom);
//            final float bitmapWidth = bm.getWidth();
//            final float bitmapHeight = bm.getHeight();
//            final float layoutWidth = right - left;
//            final float layoutHeight = bottom - top;
//            if(bitmapWidth < layoutWidth && bitmapHeight < layoutHeight){
//                final Matrix m = new Matrix();
//                float scale;
//                if((layoutHeight/bitmapHeight) > (layoutWidth/bitmapWidth)){
//                    scale = layoutWidth/bitmapWidth; //fixed the bug30855 bug30887
//                }else{
//                    scale = layoutHeight/bitmapHeight;
//                }
////                final Bitmap oldOne = bm;
//                m.setScale(scale, scale);
//                Bitmap newOne = null;
//                //fixed the bug31201
//                do {
//                    try {
//                        newOne = Bitmap.createBitmap(bm, 0, 0, bm.getWidth(), bm.getHeight(), m, false);
//                    } catch(OutOfMemoryError oom) {
//                        Log.w(TAG, "handleTextBubble(): oom, newOne = " + newOne);
//                    }
//                    if(newOne == null) {
//                        //index is zero, means that any operation can not be Completed
//                        if(mImageEditDesc.reorganizeQueue() < 2) {
//                            return;
//                        }
//                    }
//                } while(newOne == null);
//
//                bm = newOne;
//            }
            if (hasChanged) {
                setBitmap();
            }
            super.onLayout(hasChanged, left, top, right, bottom);
        }
    }

    protected void lastInvalidateView(long updateMillis) {
        performRefresh = true;
        performFrontOperator = true;
        invalidate();
        if(!handler.hasMessages(ACTION_INVALIDATE_CANCEL)){
            handler.sendEmptyMessageDelayed(ACTION_INVALIDATE_CANCEL, (long) (updateMillis * 1.5));
        }
    }

    private final static int ACTION_INVALIDATE_CANCEL = -1;
    private Handler handler = new Handler(new Callback() {

        public boolean handleMessage(Message msg) {
            switch (msg.what) {
            case ACTION_INVALIDATE_CANCEL:
                performRefresh = false;
                performFrontOperator = false;
                handler.removeMessages(ACTION_INVALIDATE_CANCEL);
                break;
            default:
                break;
            }
            return true;
        }
    });

}
