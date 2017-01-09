/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.gallery3d.app;

import android.content.Context;
import android.os.Handler;
import android.util.Log;
import android.view.Display;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.AnimationUtils;

import com.android.gallery3d.R;

/**
 * The playback controller for the Movie Player.
 */
public class MovieControllerOverlay extends CommonControllerOverlay implements
        AnimationListener {

    private boolean hidden;

    private final Handler handler;
    private final Runnable startHidingRunnable;
    private final Animation hideAnimation;
    private static String TAG="MovieControllerOverlay";
    /* SPRD:Add for VideoPlayer new Ui feature 568552 @{ */
    private GestureDetector mGestureDetector;
    private static final int ADJUST_PROGRESS = 1;
    private static final int ADJUST_VOLUME = 2;
    private static final int ADJUST_LIGHTNESS = 3;
    /* @} */
    public MovieControllerOverlay(Context context) {
        super(context);

        handler = new Handler();
        startHidingRunnable = new Runnable() {
                @Override
            public void run() {
                startHiding();
            }
        };

        hideAnimation = AnimationUtils.loadAnimation(context, R.anim.player_out);
        hideAnimation.setAnimationListener(this);

        hide();
        /** SPRD:Add VideoPlayer operate function @{ */
        mGestureDetector = new GestureDetector(context, new MyGestureDetectorListener());
        /** @} */
    }

    @Override
    protected void createTimeBar(Context context) {
        mTimeBar = new TimeBar(context, this);
    }

    @Override
    public void hide() {
        Log.d(TAG, "Hide is work!");
        boolean wasHidden = hidden;
        hidden = true;
        super.hide();
        if (mListener != null && wasHidden != hidden) {
            mNewListener.onHidden();
        }
        Log.d(TAG, "Hide is work end!");
    }

    @Override
    public void show() {
        Log.d(TAG, "show() is work start!");
        boolean wasHidden = hidden;
        hidden = false;
        super.show();
        /** SPRD:Bug474600  improve video control functions @{ */
        updateViews();
        setVisibility(View.VISIBLE);
        setFocusable(false);
        /** @} */
        if (mListener != null && wasHidden != hidden) {
            mNewListener.onShown();
        }
        maybeStartHiding();
        Log.d(TAG, "show() is work end!");
    }

    public void maybeStartHiding() {
        Log.d(TAG, "maybeStartHiding() is work start!");
        cancelHiding();
        /*
         * SPRD:Modify for new feature 568552
         * when the video is paused ,the controller can also be hidden@{
         */
        handler.postDelayed(startHidingRunnable, 2500);
        /* @} */
        Log.d(TAG, "maybeStartHiding() is work end!");
    }

    private void startHiding() {
        Log.d(TAG, "startHiding() is work start!");
        /** SPRD:Add VideoPlayer operate function @{ */
        hide();
        startHideAnimation(mBackground);
        startHideAnimation(mTimeBar);
        startHideAnimation(mPlayPauseReplayView);
        startHideAnimation(mPrevVideoView);
        startHideAnimation(mNextVideoView);
        startHideAnimation(mLockControlView);
        startHideAnimation(mPrintScreenView);
        /** @} */
        /** SPRD:bug 474614: porting float play @{ */
        // old bug info:339523
        if (floatPlayButton != null) {
            startHideAnimation(floatPlayButton);
        }
        /** @} */
        hidden = true;
        Log.d(TAG, "startHiding() is work end!");
    }

    private void startHideAnimation(View view) {
        if (view.getVisibility() == View.VISIBLE) {
            /** SPRD:bug 474614: porting float play @{ */
            //view.startAnimation(hideAnimation);
            view.setVisibility(View.INVISIBLE);
            /** @} */
        }
    }

    protected void cancelHiding() {
        Log.d(TAG, "cancelHiding() is work start!");
        handler.removeCallbacks(startHidingRunnable);
        mBackground.setAnimation(null);
        mTimeBar.setAnimation(null);
        if (floatPlayButton != null) {
            floatPlayButton.setAnimation(null);
        }
        if(mPrintScreenView != null)
            mPrintScreenView.setAnimation(null);
        Log.d(TAG, "cancelHiding() is work start end!");
    }

    @Override
    public void onAnimationStart(Animation animation) {
        // Do nothing.
    }

    @Override
    public void onAnimationRepeat(Animation animation) {
        // Do nothing.
    }

    @Override
    public void onAnimationEnd(Animation animation) {
        hide();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(TAG,"onKeyDown()");
        if (hidden) {
            show();
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (mGestureDetector.onTouchEvent(event)) {
            return true;
        }
        // process touch event over
        switch (event.getAction() & MotionEvent.ACTION_MASK) {
        case MotionEvent.ACTION_UP:
            mNewListener.endSliding();
            hideSlideView();
            break;
        default:
            break;
        }
        return true;//modify for new feature bug568552,if returns false here,we cannot get the whole process of the touch event
    }

    @Override
    protected void updateViews() {
        if (hidden) {
            return;
        }
        super.updateViews();
    }

    // TimeBar listener

    @Override
    public void onScrubbingStart() {
        cancelHiding();
        super.onScrubbingStart();
    }

    @Override
    public void onScrubbingMove(int time) {
        cancelHiding();
        super.onScrubbingMove(time);
    }

    @Override
    public void onScrubbingEnd(int time, int trimStartTime, int trimEndTime) {
        maybeStartHiding();
        super.onScrubbingEnd(time, trimStartTime, trimEndTime);
    }
   /**SPRD:Bug474600 improve video control functions@{*/
    @Override
    public void setControlButtonEnableForStop(boolean enable) {
        mNextVideoView.setEnabled(enable);
        mPrevVideoView.setEnabled(enable);
    }

    @Override
    public void clearPlayState() {
        mLoadingView.setVisibility(View.INVISIBLE);
        show();
    }

    @Override
    public void setLiveMode() {
        mTimeBar.setEnabled(false);
        isLiveMode = true;
    }
    public void setNotLiveMode() {
        isLiveMode = false;
    }

    /** SPRD:bug 474614: porting float play @{ */
    // old bug info:339523 begin
    public void showFloatPlayerButton(boolean show) {
        if (floatPlayButton == null)
            return;
        if (show) {
            floatPlayButton.setVisibility(View.VISIBLE);
        } else {
            floatPlayButton.setVisibility(View.INVISIBLE);
        }
    }
    // bug 339523 end
    /** @} */

    public void setscrubbing() {
        mTimeBar.setscrubbing();
    }

    public void hideToShow() {
        maybeStartHiding();
    }
    /** @} */

    /*
     * SPRD: add for new feature 568552
     * Add VideoPlayer operate function @{
     */
    public void showLock() {
        if(mLockControlView.getVisibility() == View.INVISIBLE){
            setVisibility(View.VISIBLE);
            requestLayout();
            maybeStartHiding();
        }
    }

    public class MyGestureDetectorListener extends GestureDetector.SimpleOnGestureListener{
        boolean firstScroll = false;
        int flagSlide = 0;

        @Override
        public boolean onDown(MotionEvent e) {
            Log.d(TAG, "onDown()");
            if(!mIsLock){
                firstScroll = true;
            }else{
                showLock();
            }
            return super.onDown(e);
        }

        public boolean onDoubleTap(MotionEvent event){
            Log.d(TAG,"onDoubleTap()");
            if (!mIsLock) {
                maybeStartHiding();
                if (mState == State.PLAYING || mState == State.PAUSED) {
                    mNewListener.doubleClickPlayPause();
                    return true;
                }
            }
            return super.onDoubleTap(event);
        }

        public boolean onSingleTapUp(MotionEvent e) {
            Log.d(TAG,"onSingleTapUp()");
            flagSlide = 0;
            return super.onSingleTapUp(e);
        }

        @Override
        public boolean onSingleTapConfirmed(MotionEvent e) {
            Log.d(TAG, "onSingleTapConfirmed()");
            if (!mIsLock) {
                if (hidden) {
                    show();
                } else {
                    hide();
                }
            } else {
                if (hidden) {
                    /* SPRD:Add for showing the controller @{ */
                    /* SPRD:Add for bug597099 When clear the cache,enter the videoplayer and lock control,the controller will disappear @{
                    if (MovieActivity.getMoviePlayer() != null) {
                        MovieActivity.getMoviePlayer().setSystemUiVisibility(true);
                    }*/
                    mLockControlView.setVisibility(View.VISIBLE);
                    mNewListener.setSystemUiVisibility(true);
                    /* Bug597099 end @} */
                    maybeStartHiding();
                    /* End @}*/
                    hidden = false;
                } else {
                    mLockControlView.setVisibility(View.INVISIBLE);
                    hidden = true;
                }
            }
            return super.onSingleTapConfirmed(e);
        }

        @Override
        public boolean onScroll(MotionEvent e1, MotionEvent e2,
                float vX, float vY) {
            float oldY = e1.getRawY();
            float y = e2.getRawY();
            float oldX = e1.getRawX();
            float x = e2.getRawX();
            Display disp = ((WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
            int windowHeight = disp.getHeight();
            int windowWidth = disp.getWidth();
            maybeStartHiding();
            if (!mIsLock) {
                if (firstScroll) {
                    if (Math.abs(vX) >= Math.abs(vY)) {
                        // adjust video progress
                        flagSlide = ADJUST_PROGRESS;
                    } else {
                        int winWidth = mNewListener.getVideoWidth();
                        if (e1.getX() < winWidth * 0.5) {
                            // adjust volume
                            flagSlide = ADJUST_LIGHTNESS;
                        } else {
                            // adjust lightness
                            flagSlide = ADJUST_VOLUME;
                        }
                    }
                }
                if (flagSlide == ADJUST_PROGRESS && !isLiveMode) {
                    mNewListener.onVideoSliding((x - oldX)/windowWidth);
                } else if (flagSlide == ADJUST_VOLUME) {
                    mNewListener.adjustVolume((oldY - y)/windowHeight);
                } else if (flagSlide == ADJUST_LIGHTNESS) {
                    mNewListener.adjustLightness((oldY - y)/windowHeight);
                }
                firstScroll = false;
            }
            return super.onScroll(e1, e2, vX, vY);
        }
    }

    public void setState(State state) {
        mState = state;
        updateViews();
    }
    /* @} */
}
