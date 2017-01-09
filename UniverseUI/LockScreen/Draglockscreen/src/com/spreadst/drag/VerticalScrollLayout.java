/** Create by Spreadst */

package com.spreadst.drag;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Scroller;

public class VerticalScrollLayout extends ViewGroup {

    private static final String TAG = "ScrollLayout";
    private VelocityTracker mVelocityTracker;
    private static final int SNAP_VELOCITY = 600;
    private Scroller mScroller;
    private int mCurScreen;
    private int mDefaultScreen = 1;
    private float mLastMotionY;
    private OnViewChangeListener mOnViewChangeListener;

    public VerticalScrollLayout(Context context) {
        super(context);
    }

    public VerticalScrollLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public VerticalScrollLayout(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
    }

    public void init(Context context, int curScreen) {
        mCurScreen = mDefaultScreen;
        mScroller = new Scroller(context);

    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        if (changed) {
            int childTop = 0;
            final int childCount = getChildCount();
            for (int i = 0; i < childCount; i++) {
                final View childView = getChildAt(i);
                if (childView.getVisibility() != View.GONE) {
                    final int childHeight = childView.getMeasuredHeight();
                    childView.layout(0, childTop,
                            childView.getMeasuredHeight(), childTop
                                    + childHeight);
                    childTop += childHeight;
                }
            }
        }

    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        final int height = MeasureSpec.getSize(heightMeasureSpec);
        final int count = getChildCount();
        for (int i = 0; i < count; i++) {
            getChildAt(i).measure(widthMeasureSpec, heightMeasureSpec);
        }
        scrollTo(0, mCurScreen * height);
    }

    public void snapToDestination() {
        final int screenHeight = getHeight();
        final int destScreen = (getScrollY() + screenHeight / 2) / screenHeight;
        snapToScreen(destScreen);
    }

    public void snapToScreen(int whichScreen) {
        // get the valid layout page
        whichScreen = Math.max(0, Math.min(whichScreen, getChildCount() - 1));
        if (getScrollY() != (whichScreen * getHeight())) {
            final int delta = whichScreen * getHeight() - getScrollY();
            mScroller.startScroll(0, getScrollY(), 0, delta,
                    Math.abs(delta) * 2);

            mCurScreen = whichScreen;
            invalidate(); // Redraw the layout
            if (mOnViewChangeListener != null) {
                mOnViewChangeListener.OnViewChange(mCurScreen);
            }
        }
    }

    @Override
    public void computeScroll() {
        if (mScroller.computeScrollOffset()) {
            scrollTo(mScroller.getCurrX(), mScroller.getCurrY());
            postInvalidate();
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        final int action = event.getAction();
        final float y = event.getY();

        switch (action) {
            case MotionEvent.ACTION_DOWN:
                Log.i("", "onTouchEvent  ACTION_DOWN");
                if (mVelocityTracker == null) {
                    mVelocityTracker = VelocityTracker.obtain();
                    mVelocityTracker.addMovement(event);
                }
                if (!mScroller.isFinished()) {
                    mScroller.abortAnimation();
                }
                mLastMotionY = y;
                break;

            case MotionEvent.ACTION_MOVE:
                int deltaY = (int) (mLastMotionY - y);
                if (IsCanMove(deltaY)) {
                    if (mVelocityTracker != null) {
                        mVelocityTracker.addMovement(event);
                    }
                    Log.d("", getScrollY() + ", " + mDefaultScreen
                            * getHeight() + ", " + deltaY);
                    if (getScrollY() >= mDefaultScreen * getHeight()
                            && deltaY > 0) {

                        return false;
                    }
                    mLastMotionY = y;
                    scrollBy(0, deltaY);
                }

                break;
            case MotionEvent.ACTION_UP:
                if (mVelocityTracker != null) {
                    mVelocityTracker.addMovement(event);
                    mVelocityTracker.computeCurrentVelocity(1000);
                }
                scrollTo(0, mDefaultScreen * getHeight());

                if (mVelocityTracker != null) {
                    mVelocityTracker.recycle();
                    mVelocityTracker = null;
                }
                break;
        }
        return true;
    }

    private boolean IsCanMove(int deltaY) {
        if (getScrollY() <= 0 && deltaY < 0) {
            return false;
        }
        if (getScrollY() >= (getChildCount() - 1) * getHeight() && deltaY > 0) {
            return false;
        }
        return true;
    }

    public void SetOnViewChangeListener(OnViewChangeListener listener) {
        mOnViewChangeListener = listener;
    }

}
