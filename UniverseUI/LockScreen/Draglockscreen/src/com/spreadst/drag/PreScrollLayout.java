/** Create by Spreadst */

package com.spreadst.drag;

import android.app.ActivityManagerNative;
import android.app.KeyguardManager;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Scroller;

public class PreScrollLayout extends ViewGroup {

    public static final int ACTION_TO_UNLOCK = 1;
    public static final int ACTION_TO_MMS = 2;
    public static final int ACTION_TO_PHONE = 3;

    public static final int MESSAGE_TO_DEFAUT_SCREEN = 10;

    private static final String TAG = "preScrollLayout";
    private VelocityTracker mVelocityTracker;
    private static final int SNAP_VELOCITY = 600;
    private Scroller mScroller;
    private int mCurScreen;
    private int mDefaultScreen = 0;
    private float mLastMotionX;
    private float mLastMotionY;
    private float mLastInterceptMotionX;
    private boolean mIsInUnlockArea = false;

    // 0 do not move, 1 up or down, 2 left or right
    private static final int MOVE_DEFAULT = 0;
    private static final int MOVE_UP_DOWN = 1;
    private static final int MOVE_LEFT_RIGHT = 2;
    private int mOperatorMode = MOVE_DEFAULT;

    private int mUnlockAreaHeight = 0;
    private int mUnlockblockHeight = 0;

    private int mCurrenToAction = 0;

    private float mDown_x = 0;

    private OnViewChangeListener mOnViewChangeListener;

    private Handler mScrollToDefaltScreenHandler;

    private static final int INTERCEPT_MOTION_X_VALUE = 20;
    // SPRD: Modify 20130910 Spreadst of Bug 212843 unlock UUI lockscreen sound play two times
    boolean isUnlock = false;

    private KeyguardManager mKeyguardManager;

    public PreScrollLayout(Context context) {
        super(context);
        // TODO Auto-generated constructor stub

    }

    public PreScrollLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub

    }

    public PreScrollLayout(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        // TODO Auto-generated constructor stub

    }

    public void init(Context context, int curScreen) {
        mCurScreen = curScreen < 0 ? mDefaultScreen : curScreen;
        mScroller = new Scroller(context);
        mScrollToDefaltScreenHandler = new ScrollToDefaltScreenHandler();
        mKeyguardManager = (KeyguardManager) mContext.getSystemService(Context.KEYGUARD_SERVICE);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        // TODO Auto-generated method stub
        int childLeft = 0;
        final int childCount = getChildCount();
        for (int i = 0; i < childCount; i++) {
            final View childView = getChildAt(i);
            if (childView.getVisibility() != View.GONE) {
                final int childWidth = childView.getMeasuredWidth();
                final int childHeight = childView.getMeasuredHeight();
                if (i == 3) {
                    childView.layout(childWidth, childHeight, 2 * childWidth,
                            2 * childHeight);
                } else {
                    childView.layout(childLeft, childHeight, childLeft
                            + childWidth, childHeight * 2);
                    childLeft += childWidth;
                }
            }
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        final int width = MeasureSpec.getSize(widthMeasureSpec);
        final int height = MeasureSpec.getSize(heightMeasureSpec);
        final int count = getChildCount();
        for (int i = 0; i < count; i++) {
            getChildAt(i).measure(widthMeasureSpec, heightMeasureSpec);
        }
        mUnlockAreaHeight = height; // init drag unlock view's height
        scrollTo(mCurScreen * width, mUnlockAreaHeight);
    }

    public void snapToDestination() {
        final int screenWidth = getWidth();
        final int destScreen = (getScrollX() + screenWidth / 2) / screenWidth;
        snapToScreen(destScreen);
    }

    public void snapToScreen(int whichScreen) {
        // get the valid layout page
        whichScreen = Math.max(0, Math.min(whichScreen, getChildCount() - 2));
        if (getScrollX() != (whichScreen * getWidth())) {
            final int delta = whichScreen * getWidth() - getScrollX();
            mScroller.startScroll(getScrollX(), getScrollY(), delta, 0,
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
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        Log.d(TAG, "onInterceptTouchEvent....." + mLastInterceptMotionX + ", "
                + ev.getX());
        if (ev.getAction() == MotionEvent.ACTION_DOWN) {
            mDown_x = ev.getX();
            mOnViewChangeListener.pokeWakelock();
            if (mScrollToDefaltScreenHandler
                    .hasMessages(MESSAGE_TO_DEFAUT_SCREEN)) {
                mScrollToDefaltScreenHandler
                        .removeMessages(MESSAGE_TO_DEFAUT_SCREEN);
            }
        }
        if (mLastInterceptMotionX == 0) {
            mLastInterceptMotionX = ev.getX();
        }
        if (Math.abs(mLastInterceptMotionX - ev.getX()) > INTERCEPT_MOTION_X_VALUE) {
            Log.d(TAG, "onInterceptTouchEvent.....true");
            mLastMotionX = mLastInterceptMotionX;
            return true;
        }
        if (ev.getAction() == MotionEvent.ACTION_CANCEL
                || ev.getAction() == MotionEvent.ACTION_UP) {
            Log.d(TAG, "onInterceptTouchEvent.....up");
            mLastInterceptMotionX = 0;
            if (mScrollToDefaltScreenHandler
                    .hasMessages(MESSAGE_TO_DEFAUT_SCREEN)) {
                mScrollToDefaltScreenHandler
                        .removeMessages(MESSAGE_TO_DEFAUT_SCREEN);
            }
            mScrollToDefaltScreenHandler.sendEmptyMessageDelayed(
                    MESSAGE_TO_DEFAUT_SCREEN, 5000);
        }

        Log.d(TAG, "onInterceptTouchEvent.....false");
        return false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        mLastInterceptMotionX = 0;
        final int action = event.getAction();
        final float x = event.getX();
        final float y = event.getY();
        Log.d(TAG, "onTouchEvent action : " + action + "|x = " + x);
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                mOperatorMode = 0;
                mDown_x = x;
                if (mVelocityTracker == null) {
                    mVelocityTracker = VelocityTracker.obtain();
                    mVelocityTracker.addMovement(event);
                }
                if (!mScroller.isFinished()) {
                    mScroller.abortAnimation();
                }
                mLastMotionX = x;
                mLastMotionY = y;
                if (y > getHeight() - mUnlockblockHeight) {
                    mIsInUnlockArea = true;
                }
                break;

            case MotionEvent.ACTION_MOVE:
                int deltaX = (int) (mLastMotionX - x);
                int deltaY = (int) (mLastMotionY - y);

                if (mOperatorMode == MOVE_DEFAULT) {
                    if (deltaX == 0 && deltaY == 0) {
                        break;
                    }
                    int dx = Math.abs(deltaX);
                    int dy = Math.abs(deltaY);
                    mOperatorMode = dx >= dy ? MOVE_LEFT_RIGHT
                            : MOVE_UP_DOWN;
                }

                if (mOperatorMode == MOVE_UP_DOWN) {
                    // start to move up or down
                    if (mIsInUnlockArea) {
                        if (mCurrenToAction > 0 && getScrollX() == getWidth()) {
                            if (IsYCanMove(deltaY)) {
                                if (mVelocityTracker != null) {
                                    mVelocityTracker.addMovement(event);
                                }
                                mLastMotionY = y;
                                if (getScrollY() + deltaY > mUnlockAreaHeight) {
                                    deltaY = mUnlockAreaHeight - getScrollY();
                                }
                                scrollBy(0, deltaY);
                            }
                        }
                    } else {
                        mOperatorMode = MOVE_LEFT_RIGHT;
                    }
                }
                if (mOperatorMode == MOVE_LEFT_RIGHT) {
                    // start to move left or right
                    if (IsCanMove(deltaX)) {
                        if (mVelocityTracker != null) {
                            mVelocityTracker.addMovement(event);
                        }
                        mLastMotionX = x;
                        if (getScrollX() + deltaX < 0) {
                            deltaX = 0 - getScrollX();
                        } else if (getScrollX() + deltaX > (getChildCount() - 2)
                                * getWidth()) {
                            deltaX = (getChildCount() - 2) * getWidth()
                                    - getScrollX();
                        }
                        scrollBy(deltaX, 0);
                    }
                }

                /* SPRD: Modify 20130925 Spreast of bug 202974 previous screen will be flashed when unlock to camera @{ */
                if (isEnableToUnlock()) {
                    if (mCurrenToAction == ACTION_TO_MMS) {
                        mOnViewChangeListener.goToSMS();
                        if (mKeyguardManager.isKeyguardSecure()) {
                            isUnlock = true;
                        }
                        try {
                            ActivityManagerNative.getDefault().dismissKeyguardOnNextActivity();
                        } catch (RemoteException e) {
                            Log.w(TAG, "can't dismiss keyguard on launch mms");
                        }
                    } else if (mCurrenToAction == ACTION_TO_PHONE) {
                        mOnViewChangeListener.goToCall();
                        if (mKeyguardManager.isKeyguardSecure()) {
                            isUnlock = true;
                        }
                        try {
                            ActivityManagerNative.getDefault().dismissKeyguardOnNextActivity();
                        } catch (RemoteException e) {
                            Log.w(TAG, "can't dismiss keyguard on launch dial");
                        }
                    }
                    /* SPRD: Modify 20130910 Spreadst of Bug 212843 unlock UUI lockscreen sound play two times @{ */
                    if (!isUnlock && (mCurrenToAction == ACTION_TO_UNLOCK)) {
                        isUnlock = true;
                    }
                } else {
                    isUnlock = false;
                }
                /* @} */
                /*SPRD: Modify 20130925 Spreast of bug 202974 end @} */


                break;
            case MotionEvent.ACTION_CANCEL:
            case MotionEvent.ACTION_UP:

                if (mOperatorMode == MOVE_LEFT_RIGHT) {
                    int velocityX = 0;
                    if (mVelocityTracker != null) {
                        mVelocityTracker.addMovement(event);
                        mVelocityTracker.computeCurrentVelocity(1000);
                        velocityX = (int) mVelocityTracker.getXVelocity();
                    }
                    float x_diff = mDown_x - event.getX();
                    Log.d(TAG, " velocityX " + velocityX + " x_diff " + x_diff);
                    if (velocityX > SNAP_VELOCITY && mCurScreen > 0 && x_diff < 0) {
                        // Fling enough to move left
                        Log.d(TAG, "snap left");
                        snapToScreen(mCurScreen - 1);
                    } else if (velocityX < -SNAP_VELOCITY
                            && mCurScreen < getChildCount() - 2 && x_diff > 0) {
                        // Fling enough to move right
                        Log.d(TAG, "snap right");
                        snapToScreen(mCurScreen + 1);
                    } else {
                        snapToDestination();
                    }
                }

                if (mVelocityTracker != null) {
                    mVelocityTracker.recycle();
                    mVelocityTracker = null;
                }
                mCurrenToAction = 0;
                mIsInUnlockArea = false;
                if (mScroller.isFinished()) {
                    scrollTo(mCurScreen * getWidth(), mUnlockAreaHeight);
                }
                if (mScrollToDefaltScreenHandler
                        .hasMessages(MESSAGE_TO_DEFAUT_SCREEN)) {
                    mScrollToDefaltScreenHandler
                            .removeMessages(MESSAGE_TO_DEFAUT_SCREEN);
                }
                mScrollToDefaltScreenHandler.sendEmptyMessageDelayed(
                        MESSAGE_TO_DEFAUT_SCREEN, 5000);
                /* SPRD: Modify 20130910 Spreadst of Bug 212843 unlock UUI lockscreen sound play two times @{ */
                if (isUnlock == true && mOnViewChangeListener != null){
                    mOnViewChangeListener.goToUnlockScreen();
                    isUnlock = false;
                }
                /* @} */

                break;
            case MotionEvent.ACTION_POINTER_UP:
                scrollTo(mCurScreen * getWidth(), mUnlockAreaHeight);
                return false;
        }
        return true;
    }

    private boolean IsCanMove(int deltaX) {
        if (getScrollX() <= 0 && deltaX < 0) {
            return false;
        }
        if (getScrollX() >= (getChildCount() - 2) * getWidth() && deltaX > 0) {
            return false;
        }
        return true;
    }

    private boolean IsYCanMove(int deltaY) {
        Log.d(TAG, "deltaY=" + deltaY + ", " + getScrollY() + ", "
                + getHeight());
        if (getScrollY() >= mUnlockAreaHeight && deltaY > 0) {

            return false;
        }

        if (getScrollY() <= 0) {
            return false;
        }
        return true;
    }

    private boolean isEnableToUnlock() {

        if (getScrollY() < getHeight() - (mUnlockblockHeight / 2)
                && mIsInUnlockArea) {

            return true;
        }

        return false;
    }

    public void SetOnViewChangeListener(OnViewChangeListener listener) {
        mOnViewChangeListener = listener;
    }

    public void unRegistrationViewChangeListener() {
        if (mOnViewChangeListener != null) {
            mOnViewChangeListener = null;
        }
    }

    public void setToAction(int action) {

        mCurrenToAction = action;
    }

    public void setUnlockblockHeight(int UnlockblockHeight) {

        mUnlockblockHeight = UnlockblockHeight;
    }

    class ScrollToDefaltScreenHandler extends Handler {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {

                case MESSAGE_TO_DEFAUT_SCREEN:
                    Log.d(TAG, "mCurScreen= " + mCurScreen);
                    if (mCurScreen != 1) {
                        snapToScreen(1);
                    }
            }

        }

    }

}
