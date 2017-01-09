/*
 * Copyright (C) 2012 The Android Open Source Project
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
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.os.Handler;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.animation.Animation.AnimationListener;
import android.view.WindowManager;
import android.view.Display;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.android.gallery3d.R;
import com.sprd.gallery3d.app.NewControllerOverlay;

/**
 * The common playback controller for the Movie Player or Video Trimming.
 */
public abstract class CommonControllerOverlay extends FrameLayout implements
        ControllerOverlay,
        OnClickListener,
        TimeBar.Listener,
        AnimationListener,
        NewControllerOverlay { // SPRD:Add for new feature 568552

    public  enum State {
        PLAYING,
        PAUSED,
        ENDED,
        ERROR,
        LOADING,
        /** SPRD:Add VideoPlayer operate function @{ */
        SPEED,
        BACKWARD,
        VOLUME,
        MUTE,
        LIGHT
        /** @} */
    }

    private static final float ERROR_MESSAGE_RELATIVE_PADDING = 1.0f / 6;
    protected Listener mListener;

    protected final View mBackground;
    protected TimeBar mTimeBar;

    protected View mMainView;
    protected final LinearLayout mLoadingView;
    protected final TextView mErrorView;
    protected final ImageView mPlayPauseReplayView;

    protected State mState;

    protected boolean mCanReplay = true;
    /** SPRD:bug 474614: porting float play @{ */
    // old bug info 339523 begin
    protected ImageView floatPlayButton;

    // bug 339523 end
    /** @} */
    public void setSeekable(boolean canSeek) {
        mTimeBar.setSeekable(canSeek);
    }

    /**
     * SPRD:Bug474600 improve video control functions add new image parameters for Play @{
     */
    protected LinearLayout playControllerView;
    protected ImageView mNextVideoView;
    protected ImageView mPrevVideoView;
    protected boolean isLiveMode = false;
    private boolean hidden;
    boolean mIsFloatWindowDisabledByTrim = false;
    /** @} */
    /**
     * SPRD:Bug474600 improve video control functions add new parameters @{
     */
    private final String TAG = "CommonControllerOverlay";

    /**   @}*/
    /*
     * SPRD:add for new feature 568552 @{
     */
    protected  ImageView mSlideView;
    protected  TextView mSlideText;
    protected  ImageView mLockControlView;
    protected  ImageView mLockOrintationView;
    protected  ImageView mPrintScreenView;
    protected  ImageView mStartFloatPlayView;
    protected  ImageView mScaleScreen;
    protected boolean hasNext = false;
    protected boolean mIsLock = false;
    protected WindowManager mWindowManager;
    protected NewListener mNewListener;
    /*@}*/
    private Handler mHander;
    private Runnable startHidingRunnable;
    private long mPrintScreenDuration = 0;
    private boolean mIsFirstPrintScreen = true;
    /** @} */
    public CommonControllerOverlay(Context context) {
        super(context);
        mWindowManager = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);// SPRD:Add for new feature 568552
        mState = State.LOADING;
        // TODO: Move the following layout code into xml file.
        LayoutParams wrapContent =
                new LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
        LayoutParams matchParent =
                new LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
        /* SPRD: bug 540192,let the layout adapt to the screen @} */
        LayoutParams buttonParam = new LayoutParams(
                getIntFromDimens(R.dimen.movie_button_size),
                getIntFromDimens(R.dimen.movie_button_size));
        /* @} */

        mBackground = new View(context);
        mBackground.setBackgroundColor(context.getResources().getColor(R.color.darker_transparent));
        addView(mBackground, matchParent);

        mLoadingView = new LinearLayout(context);
        mLoadingView.setOrientation(LinearLayout.VERTICAL);
        mLoadingView.setGravity(Gravity.CENTER_HORIZONTAL);
        ProgressBar spinner = new ProgressBar(context);
        spinner.setIndeterminate(true);
        mLoadingView.addView(spinner, wrapContent);
        TextView loadingText = createOverlayTextView(context);
        loadingText.setText(R.string.loading_video);
        mLoadingView.addView(loadingText, wrapContent);
        addView(mLoadingView, wrapContent);
        /** SPRD:Bug474600 improve video control functions @{ */
        playControllerView = new LinearLayout(context);
        playControllerView.setOrientation(LinearLayout.HORIZONTAL);
        playControllerView.setGravity(Gravity.CENTER_HORIZONTAL);
        playControllerView.setLayoutParams(wrapContent);
        playControllerView.setClickable(true);
        /** @} */
        /**SPRD:Bug545844 modify by old bug 539690,in Arabic system,the video player back & forward arrows are reversed */
        playControllerView.setLayoutDirection(LAYOUT_DIRECTION_LTR);
        /** @} */

        /** SPRD:Add VideoPlayer operate function @{ */
        mPlayPauseReplayView = new ImageView(context);
        mPlayPauseReplayView.setImageResource(R.drawable.control_play);
        mPlayPauseReplayView.setContentDescription(
                context.getResources().getString(R.string.accessibility_play_video));
        mPlayPauseReplayView.setScaleType(ScaleType.FIT_CENTER);
        mPlayPauseReplayView.setLayoutParams(buttonParam);
        mPlayPauseReplayView.setFocusable(true);
        mPlayPauseReplayView.setClickable(true);
        mPlayPauseReplayView.setOnClickListener(this);

        mNextVideoView = new ImageView(context);
        // SPRD:modify for new feature 568552
        mNextVideoView.setImageResource(R.drawable.new_next_video);
        /* SPRD: bug 540192,let the layout adapt to the screen @{ */
        mNextVideoView.setScaleType(ScaleType.FIT_CENTER);
        mNextVideoView.setLayoutParams(buttonParam);
        /* @} */
        mNextVideoView.setFocusable(true);
        mNextVideoView.setClickable(true);
        mNextVideoView.setOnClickListener(nextListener);

        mPrevVideoView = new ImageView(context);
        // SPRD:modify for new feature 568552
        mPrevVideoView.setImageResource(R.drawable.new_last_video);
        /* SPRD: bug 540192,let the layout adapt to the screen @{ */
        mPrevVideoView.setScaleType(ScaleType.FIT_CENTER);
        mPrevVideoView.setLayoutParams(buttonParam);
        /* @} */
        mPrevVideoView.setFocusable(true);
        mPrevVideoView.setClickable(true);
        mPrevVideoView.setOnClickListener(prevListener);

        mLockControlView = new ImageView(context);
        mLockControlView.setImageResource(R.drawable.new_lock_control_close);
        mLockControlView.setBackgroundResource(R.drawable.new_lock_control_bg);
        mLockControlView.setScaleType(ScaleType.CENTER);
        mLockControlView.setVisibility(View.INVISIBLE);
        mLockControlView.setClickable(true);
        mLockControlView.setOnClickListener(mLockControlViewListener);
        addView(mLockControlView, wrapContent);

        mLockOrintationView = new ImageView(context);
        mLockOrintationView.setImageResource(R.drawable.new_unlock_orientation);
        mLockOrintationView.setBackgroundResource(R.drawable.new_lock_control_bg);
        mLockOrintationView.setScaleType(ScaleType.CENTER);
        mLockOrintationView.setVisibility(View.INVISIBLE);
        mLockOrintationView.setClickable(false);
        mLockOrintationView.setOnClickListener(mLockOrintationViewListener);
        addView(mLockOrintationView, wrapContent);

        mPrintScreenView = new ImageView(context);
        mPrintScreenView.setImageResource(R.drawable.new_printscreen);
        mPrintScreenView.setBackgroundResource(R.drawable.new_printscreen_bg);
        mPrintScreenView.setScaleType(ScaleType.CENTER);
        mPrintScreenView.setVisibility(View.VISIBLE);
        mPrintScreenView.setClickable(true);
        mPrintScreenView.setOnClickListener(mPrintScreenViewListener);
        addView(mPrintScreenView, wrapContent);

        mSlideView = new ImageView(context);
        mSlideView.setImageResource(R.drawable.ic_vidcontrol_speed);
        mSlideView.setBackgroundResource(R.drawable.bg_slidecontrol);
        mSlideView.setScaleType(ScaleType.CENTER);
        mSlideView.setVisibility(View.INVISIBLE);
        mSlideText = createOverlayTextView(context);
        mSlideText.setVisibility(View.INVISIBLE);
        addView(mSlideView, wrapContent);
        addView(mSlideText, wrapContent);

        /* SPRD:Add for Bug609458 Change the location of the float window button @{ */
        mStartFloatPlayView = new ImageView(context);
        mStartFloatPlayView.setImageResource(R.drawable.new_float_window);
        mStartFloatPlayView.setScaleType(ScaleType.FIT_CENTER);
        mStartFloatPlayView.setLayoutParams(buttonParam);
        mStartFloatPlayView.setFocusable(true);
        mStartFloatPlayView.setClickable(true);
        mStartFloatPlayView.setOnClickListener(mFloatPlayViewListener);
        /* Bug609458 end @} */

        mScaleScreen = new ImageView(context);
        mScaleScreen.setImageResource(R.drawable.new_scale_to_larger);
        mScaleScreen.setScaleType(ScaleType.FIT_CENTER);
        mScaleScreen.setLayoutParams(buttonParam);
        mScaleScreen.setFocusable(true);
        mScaleScreen.setClickable(true);
        mScaleScreen.setOnClickListener(mScaleScreenListener);
        /*@}*/

        /** SPRD:Add VideoPlayer operate function @{ */
        //SPRD:Modify for new feature bug:568552 609458
        ImageView[] btns = new ImageView[]{mStartFloatPlayView, mPrevVideoView, mPlayPauseReplayView, mNextVideoView,mScaleScreen};
        /** @} */
        for(int i=0; i<btns.length; i++) {
            addView(btns[i], wrapContent);
        }
        /** @} */
        // Depending on the usage, the timeBar can show a single scrubber, or
        // multiple ones for trimming.
        createTimeBar(context);
        addView(mTimeBar, wrapContent);
        mTimeBar.setContentDescription(
                context.getResources().getString(R.string.accessibility_time_bar));
        mErrorView = createOverlayTextView(context);
        addView(mErrorView, matchParent);

        RelativeLayout.LayoutParams params =
                new RelativeLayout.LayoutParams(
                        LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT);
        setLayoutParams(params);
        hide();
        /** SPRD:Add VideoPlayer operate function @{ */
        mHander = new Handler();
        startHidingRunnable = new Runnable() {
            @Override
            public void run() {
                startHiding();
            }
        };
        /** @} */

    }

    /* @} */
    abstract protected void createTimeBar(Context context);

    private TextView createOverlayTextView(Context context) {
        TextView view = new TextView(context);
        view.setGravity(Gravity.CENTER);
        view.setTextColor(0xFFFFFFFF);
        view.setPadding(0, 15, 0, 15);
        return view;
    }

    @Override
    public void setListener(Listener listener) {
        this.mListener = listener;
    }

    @Override
    public void setCanReplay(boolean canReplay) {
        this.mCanReplay = canReplay;
    }

    @Override
    public View getView() {
        return this;
    }

    /*SPRD:Bug605721 Turn screen and then click pause button ,the controllerOvlay is disappear@{*/
    public void removeHandler() {
        if(mHander != null) {
            mHander.removeCallbacks(startHidingRunnable);
        }
    }
    /*Bug605721 end@}*/

    @Override
    public void showPlaying() {
        Log.d(TAG, "showPlaying");
        mState = State.PLAYING;
        /* SPRD:Add for new feature 568552 @{ */
        showMainView(null);
        /* @} */
    }

    @Override
    public void showPaused() {
        Log.d(TAG, "showPaused");
        mState = State.PAUSED;
        /* SPRD:Add for new feature 568552 @{ */
        showMainView(null);
        /* @} */
    }

    @Override
    public void showEnded() {
        Log.d(TAG, "showEnded()");
        mState = State.ENDED;
        /* SPRD:Add for new feature 568552 @{ */
        if (mCanReplay) showMainView(null);
        /* @} */
    }

    @Override
    public void showLoading() {
        Log.d(TAG, "showLoading");
        mState = State.LOADING;
        showMainView(mLoadingView);
    }

    @Override
    public void showErrorMessage(String message) {
        mState = State.ERROR;
        int padding = (int) (getMeasuredWidth() * ERROR_MESSAGE_RELATIVE_PADDING);
        mErrorView.setPadding(
                padding, mErrorView.getPaddingTop(), padding, mErrorView.getPaddingBottom());
        mErrorView.setText(message);
        showMainView(mErrorView);
    }

    @Override
    public void setTimes(int currentTime, int totalTime,
            int trimStartTime, int trimEndTime) {
        /**
         * SPRD:Bug474600 improve video control functions remove @{ mTimeBar.setTime(currentTime,
         * totalTime, trimStartTime, trimEndTime); @}
         */
        /** SPRD:Bug474600 improve video control functions @{ */
        if (isLiveMode && totalTime == -1) {
            mTimeBar.setTimeTextOnly(currentTime, totalTime, trimStartTime, trimEndTime);
        } else {
            mTimeBar.setTime(currentTime, totalTime, trimStartTime, trimEndTime);
        }
        /** @} */
    }

    public void hide() {
        Log.d(TAG, "common hide is work");
        /** SPRD:Bug474600 improve video control functions @{ */
        boolean wasHidden = hidden;
        hidden = true;
        /** @} */
        /** SPRD:Bug474600 improve video control functions
         * add @{ */
        playControllerView.setVisibility(View.INVISIBLE);
        /** @} */
        mLoadingView.setVisibility(View.INVISIBLE);
        mBackground.setVisibility(View.INVISIBLE);
        mTimeBar.setVisibility(View.INVISIBLE);
        mPrintScreenView.setVisibility(View.INVISIBLE);
        if (!mIsLock) {
            mLockControlView.setVisibility(View.INVISIBLE);
        }
        mLockOrintationView.setVisibility(View.INVISIBLE);
        /* SPRD:add for new feature 568552 609548
         * new UI  @{
         * setVisibility(View.INVISIBLE);
         */
        mStartFloatPlayView.setVisibility(View.INVISIBLE);
        mScaleScreen.setVisibility(View.INVISIBLE);
        mPrevVideoView.setVisibility(View.INVISIBLE);
        mNextVideoView.setVisibility(View.INVISIBLE);
        mPlayPauseReplayView.setVisibility(View.INVISIBLE);
        /* @} */
        setFocusable(true);
        requestFocus();
        /** SPRD:Bug474600 improve video control functions @{ */
        if (mNewListener != null && wasHidden != hidden) {
            mNewListener.onHidden();
        }
        /** @} */
    }

    private void showMainView(View view) {
        mMainView = view;
        mErrorView.setVisibility(mMainView == mErrorView ? View.VISIBLE : View.INVISIBLE);
        mLoadingView.setVisibility(mMainView == mLoadingView ? View.VISIBLE : View.INVISIBLE);
            mPlayPauseReplayView.setVisibility(
                    mMainView == mPlayPauseReplayView ? View.VISIBLE : View.INVISIBLE);
        show();
    }

    @Override
    public void show() {
        Log.d(TAG, "common show is work!");
        /** SPRD:Bug474600 improve video control functions @{ */
        boolean wasHidden = hidden;
        hidden = false;
        /** @} */
        updateViews();
        setVisibility(View.VISIBLE);
        setFocusable(false);
        /** SPRD:Bug474600 improve video control functions @{ */
        if (mNewListener != null && wasHidden != hidden) {
            mNewListener.onShown();
        }
        /** @} */
    }

    @Override
    public void onClick(View view) {
        if (mListener != null) {
            if (view == mPlayPauseReplayView) {
                if (mState == State.ENDED) {
                    if (mCanReplay) {
                        mListener.onReplay();
                    }
                } else if (mState == State.PAUSED || mState == State.PLAYING) {
                    mListener.onPlayPause();
                }
            }
        }
    }

    /**
     * SPRD:Bug474600 improve video control functions add new Listeners, such as:nextListener,
     * mStopListener,mFfwdListener,prevListener, mFfwdListener,mRewListener
     * @{
     */
    private View.OnClickListener nextListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            mNewListener.onNext();
        }
    };
    private View.OnClickListener mStopListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            mNewListener.onStopVideo();
        }
    };
    private View.OnClickListener prevListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            mNewListener.onPrev();
        }
    };

    /** @} */
    /** SPRD:bug 474614: porting float play @{ */
    // old bug info:339523 begin 423
    private View.OnClickListener floatPlayListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            Context context = getContext();
            try {
                Context packageContext = context.createPackageContext(
                        "com.sprd.videoswallpapers",
                        Context.CONTEXT_IGNORE_SECURITY);
                SharedPreferences sp = packageContext.getSharedPreferences(
                        "wallpaper", Context.MODE_WORLD_READABLE
                                | Context.MODE_MULTI_PROCESS);
                boolean isVideoWallpaper = sp.getBoolean("videowallpaper",
                        false);
                Log.d(TAG, "onClick isVideoWallpaper = " + isVideoWallpaper
                        + " ss = ");
                if (isVideoWallpaper) {
                    Log.d(TAG, "onClick isVideoWallpaper true");
                    Toast.makeText(context, R.string.can_not_open_float_video,
                            Toast.LENGTH_LONG).show();
                } else {
                    mNewListener.onStartFloatPlay();
                }
            } catch (NameNotFoundException e) {
                Log.d(TAG, "onClick videoswallpapers NameNotFound");
                mNewListener.onStartFloatPlay();
            }
        }
    };

    // bug 339523 end
    /** @} */

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        Log.d(TAG,"CommonControllerOverlay++onTouchEvent");
        if (super.onTouchEvent(event)) {
            return true;
        }
        return false;
    }

    // The paddings of 4 sides which covered by system components. E.g.
    // +-----------------+\
    // | Action Bar | insets.top
    // +-----------------+/
    // | |
    // | Content Area | insets.right = insets.left = 0
    // | |
    // +-----------------+\
    // | Navigation Bar | insets.bottom
    // +-----------------+/
    // Please see View.fitSystemWindows() for more details.
    private final Rect mWindowInsets = new Rect();

    @Override
    protected boolean fitSystemWindows(Rect insets) {
        // We don't set the paddings of this View, otherwise,
        // the content will get cropped outside window
        mWindowInsets.set(insets);
        return true;
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        Rect insets = mWindowInsets;
        int pl = insets.left; // the left paddings
        int pr = insets.right;
        int pt = insets.top;
        int pb = insets.bottom;
        /** SPRD:Bug474600 improve video control functions@{ */
        int bw;
        int bh;
        bw = mTimeBar.getBarHeight();
        bh = bw;
        /** @} */

        int h = bottom - top;
        int w = right - left;
        boolean error = mErrorView.getVisibility() == View.VISIBLE;
        /*
         * SPRD:Modify for new feature 568552 609998
         * UI widgets location change according to the UI/UE design @{
         */
        int screenHeight;
        Display display = mWindowManager.getDefaultDisplay();
        screenHeight = display.getHeight();//The screen height;

        int btnWidth = getIntFromDimens(R.dimen.sprd_controlbtn_width);//The width of the button
        int timebarTopLocation = getIntFromDimens(R.dimen.sprd_timebar_top_location);//The y-direction coordinate of the timebar
        int neededValues = getIntFromDimens(R.dimen.sprd_needed_values);//We calculate the value according to the design
        int backgroundHeight = getIntFromDimens(R.dimen.sprd_background_height);//The height of the background at the bottom of the MovieActivity
        int timeBarPreferredHeight = mTimeBar.getPreferredHeight();//The whole height of the timebar including paddings
        mTimeBar.layout(left, screenHeight - pb - bw, w - pr, screenHeight
                - pb);
        mBackground.layout(0, h - pb - backgroundHeight, w, h
                - pb);
        int cx = left + (w - pr) / 2; // center x
        int screenCenterline = top + (h - pb) / 2;

        //Calculate the width and the height of the button view
        int sw = mSlideView.getMeasuredWidth();
        int sh = mSlideView.getMeasuredHeight();
        int stw = mSlideText.getMeasuredWidth();
        int sth = mSlideText.getMeasuredHeight();
        int lobd_portrait = getIntFromDimens(R.dimen.width_height_portrait);//diameter of the lockcontrolbtn and printbtn
        int lobd_landscape = getIntFromDimens(R.dimen.width_height_landscape);//diameter of the orientationbtn
        int controllerTopLine = h - pb - timebarTopLocation;//The top line of the five buttons
        int controllerBottomLine = h - pb - neededValues - timeBarPreferredHeight / 4;//The bottom line of the five buttons

        mSlideView.layout(cx - sw / 2, screenCenterline - sh
                / 2, cx + sw / 2, screenCenterline + sh / 2);
        int sp = screenCenterline + sh / 2;
        mSlideText.layout(cx - stw / 2, sp - sth, cx + stw / 2, sp + sth);
        //parameters in portrait screen mode
        int printScreenViewLeft = 2 * cx - lobd_portrait * 4 / 3;
        int printScreenViewTop = screenCenterline - lobd_portrait / 2;
        int lockControlViewLeft = 0 + lobd_portrait / 3;
        int lockControlViewTop = screenCenterline - lobd_portrait / 2;
        int lockOrientationViewLeft = cx - lobd_landscape / 2;
        int lockOrientationViewTop = screenCenterline - lobd_landscape / 2;
        int spbX_left = 0, spbX_right = 0, pvbX_left = 0, pvbX_right = 0, pprbX_left = 0, pprbX_right = 0, nvbX_left = 0, nvbX_right = 0, ssbX_left = 0, ssbX_right = 0;

        //The x-axis and y-axis location of the pre-video-button
        pvbX_left = w * 3 / 10 - btnWidth / 2;
        pvbX_right = w * 3 / 10 + btnWidth / 2;
        //The x-axis and y-axis location of the play-pause-replay-button
        pprbX_left = w / 2 - btnWidth / 2;
        pprbX_right = w / 2 + btnWidth / 2;
        //The x-axis and y-axis location of the next-video-button
        nvbX_left = w * 7 / 10 - btnWidth / 2;
        nvbX_right = w * 7 / 10 + btnWidth / 2;

        if (mContext.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {
            //The x-axis and y-axis location of the screen-projection-button
            spbX_left = w - mTimeBar.getTimeTextCenterLine() - btnWidth / 2;
            spbX_right = w - mTimeBar.getTimeTextCenterLine() + btnWidth / 2;
            //The x-axis and y-axis location of the scale-screen-button
            ssbX_left = mTimeBar.getTimeTextCenterLine() - btnWidth / 2;
            ssbX_right = mTimeBar.getTimeTextCenterLine() + btnWidth / 2;
        } else if (mContext.getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
            //parameters in landscape screen mode
            spbX_left = mTimeBar.getTimebarStartLine();
            spbX_right = mTimeBar.getTimebarStartLine() + btnWidth;
            ssbX_left = w - mTimeBar.getTimebarStartLine() - btnWidth;
            ssbX_right = w - mTimeBar.getTimebarStartLine();
        }
        // SPRD:Add for Bug609458 Change the location of the float window button
        mStartFloatPlayView.layout(spbX_left, controllerBottomLine, spbX_right, controllerTopLine);
        mPrevVideoView.layout(pvbX_left, controllerBottomLine, pvbX_right, controllerTopLine);
        mPlayPauseReplayView.layout(pprbX_left, controllerBottomLine, pprbX_right, controllerTopLine);
        mNextVideoView.layout(nvbX_left, controllerBottomLine, nvbX_right, controllerTopLine);
        mScaleScreen.layout(ssbX_left, controllerBottomLine, ssbX_right, controllerTopLine);

        mPrintScreenView.layout(printScreenViewLeft, printScreenViewTop,
                printScreenViewLeft + lobd_portrait, printScreenViewTop + lobd_portrait);
        mLockControlView.layout(lockControlViewLeft, lockControlViewTop,
                lockControlViewLeft + lobd_portrait, lockControlViewTop + lobd_portrait);
        mLockOrintationView.layout(lockOrientationViewLeft, lockOrientationViewTop,
                lockOrientationViewLeft + lobd_landscape, lockOrientationViewTop + lobd_landscape);
        /* Bug568552 609998 End @} */
        if (mMainView != null) {
            layoutCenteredView(mMainView, left, top, right, bottom);
        }
    }

    private void layoutCenteredView(View view, int l, int t, int r, int b) {
        int cw = view.getMeasuredWidth();
        int ch = view.getMeasuredHeight();
        int cl = (r - l - cw) / 2;
        int ct = (b - t - ch) / 2;
        view.layout(cl, ct, cl + cw, ct + ch);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        measureChildren(widthMeasureSpec, heightMeasureSpec);
    }

    protected void updateViews() {
        Log.d(TAG, "updateViews is work!");
        if (hidden || mIsLock) {//SPRD:Bug595223 video view displays as locked but you can control the video playing
            return;
        }
        mBackground.setVisibility(View.VISIBLE);
        mTimeBar.setVisibility(View.VISIBLE);
        /* 
         * SPRD: Modify for new feature 568552 @{
         */
        if (!mIsFloatWindowDisabledByTrim) {
            if (hasNext) {
                mPrevVideoView.setVisibility(View.VISIBLE);
                mNextVideoView.setVisibility(View.VISIBLE);
            }
            mLockControlView.setVisibility(View.VISIBLE);
            mPrintScreenView.setVisibility(View.VISIBLE);
            // SPRD:Add for Bug609458 Change the location of the float window button
            mStartFloatPlayView.setVisibility(View.VISIBLE);
            mScaleScreen.setVisibility(View.VISIBLE);
        }
        /** SPRD:Bug474600 improve video control functions @{ */
        if (isLiveMode) {
            mPlayPauseReplayView
                    .setImageResource(mState == State.PAUSED ? R.drawable.control_play
                            : R.drawable.ic_vidcontrol_stop);
        } else {
            mPlayPauseReplayView
                    .setImageResource(mState == State.PAUSED ? R.drawable.control_play
                            : R.drawable.control_pause);
        }
        if (!mIsFloatWindowDisabledByTrim) {
            mPlayPauseReplayView.setVisibility(View.VISIBLE);
        }
        /*@} */

        requestLayout();
    }

    /** SPRD:Bug474600 improve video control functions @{ */
    public void showNextPrevBtn(boolean show) {
        if (!show) {
            mNextVideoView.setVisibility(View.GONE);
            mPrevVideoView.setVisibility(View.GONE);
        } else {
            /** SPRD:Bug474600 improve video control functions @{ */
            hasNext = true;
            /** @} */
            mNextVideoView.setVisibility(View.VISIBLE);
            mPrevVideoView.setVisibility(View.VISIBLE);
        }
    }

    public void timeBarEnable(boolean enable){
        mTimeBar.setEnabled(enable);
    }

    public void resetTime() {
        mTimeBar.resetTime();
    }

    // TimeBar listener

    @Override
    public void onScrubbingStart() {
        mListener.onSeekStart();
    }

    @Override
    public void onScrubbingMove(int time) {
        mListener.onSeekMove(time);
    }

    @Override
    public void onScrubbingEnd(int time, int trimStartTime, int trimEndTime) {
        mListener.onSeekEnd(time, trimStartTime, trimEndTime);
    }

    private void startHiding() {
        if(mLockOrintationView.getVisibility() == View.VISIBLE){
            mLockOrintationView.setVisibility(View.INVISIBLE);
            /* SPRD:Add for showing the controller @{ */
            if (MovieActivity.getMoviePlayer() != null && !MovieActivity.mMenuVisibility) {
                MovieActivity.getMoviePlayer().setSystemUiVisibility(false);
            }
            /* End @} */
        }
    }

    /*
     * SPRD: add for new feature 568552 click to adjust video size @{
     */
    @Override
    public void setNewListener(NewListener listener) {
        mNewListener = listener;
    }

    @Override
    public void showSpeed(int time) {
        mState = State.SPEED;
        showSlideUI(stringForTime(time));
    }

    @Override
    public void showBackward(int time) {
        mState = State.BACKWARD;
        showSlideUI(stringForTime(time));
    }

    @Override
    public void showVolume(String vol) {
        if (vol.equals("0")) {
            mState = State.MUTE;
        } else {
            mState = State.VOLUME;
        }
        showSlideUI(vol);
    }

    @Override
    public void showLightness(String light) {
        mState = State.LIGHT;
        showSlideUI(light);
    }

    public void showSlideUI(String time) {
        if (!isLiveMode) {
            mSlideView
                    .setImageResource(mState == State.SPEED ? R.drawable.ic_vidcontrol_speed
                            : mState == State.BACKWARD ? R.drawable.ic_vidcontrol_back
                                    : mState == State.VOLUME ?
                                            R.drawable.ic_adjust_volume
                                            : mState == State.MUTE ? R.drawable.ic_adjust_mute :
                                                    R.drawable.ic_adjust_lightness);
            mSlideView.setVisibility(View.VISIBLE);
            mSlideText.setText(mState == State.SPEED ? "+" + time
                    : mState == State.BACKWARD ? "-" + time : mState == State.MUTE ? getResources()
                            .getString(R.string.mute) : time
                            + getResources().getString(R.string.percent));
            mSlideText.setVisibility(View.VISIBLE);
            setVisibility(View.VISIBLE);
            /* SPRD:Add for showing the controller @{ */
            if (MovieActivity.getMoviePlayer() != null) {
                MovieActivity.getMoviePlayer().setSystemUiVisibility(true);
            }
            /* End @}*/
        }
    }

    public void hideSlideView() {
        mHander.postDelayed(new Runnable() {
            @Override
            public void run() {
                mSlideView.setVisibility(View.INVISIBLE);
                mSlideText.setVisibility(View.INVISIBLE);
            }
        }, 1000);
    }

    private View.OnClickListener mLockControlViewListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            mNewListener.lockControl();
        }
    };

    private View.OnClickListener mLockOrintationViewListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            mNewListener.lockOrintation();
        }
    };

    private View.OnClickListener mPrintScreenViewListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            /*SPRD Bug644526 PrintScreen more time maybe happens ANR @{*/
            long time = System.currentTimeMillis();
            int duration = (int) (time - mPrintScreenDuration);
            if (mIsFirstPrintScreen || duration >= 1000) {
            /* SPRD:Add for bug611219 When playing streaming videos,the print button is not shown @{ */
                if (mNewListener.isNewStreamUri()) {
                    Toast.makeText(mContext, R.string.stream_screenshot_not_supported, Toast.LENGTH_SHORT).show();
                } else {
                    mNewListener.printScreen();
                    mIsFirstPrintScreen = false;
                }
            /* Bug611219 end @}*/
                mPrintScreenDuration = time;
            /*Bug644526 end @}*/
            }
        }
    };

    /* SPRD:Add for Bug609458 Change the location of the float window button @{ */
    private View.OnClickListener mFloatPlayViewListener = new View.OnClickListener(){
        @Override
        public void onClick(View view) {
            /* SRPD:Add for Bug611215 Show notice when playing streaming videos or in camera @{ */
            Boolean float_window = android.os.SystemProperties.getBoolean(
                    "persist.sys.floating_window", true);
            if (!float_window) {
                Toast.makeText(mContext,R.string.floatwindow_unsupported_device,Toast.LENGTH_SHORT).show();
                return;
            } else if (mNewListener.isDisableFloatWindow()) {
                Toast.makeText(mContext,R.string.floatwindow_unsupported_camera,Toast.LENGTH_SHORT).show();
                return;
            } else if (mNewListener.isNewStreamUri()) {
                Toast.makeText(mContext,R.string.floatwindow_unsupported_streaming,Toast.LENGTH_SHORT).show();
                return;
            } else {
                mNewListener.onStartFloatPlay();
            }
            /* Bug611215 end @} */
        }
    };
    /* Bug609458 end @} */

    private View.OnClickListener mScaleScreenListener = new View.OnClickListener() {

        @Override
        public void onClick(View v) {
            mNewListener.adjustVideoSize();
        }
    };

    public void setScaleButtonImage(boolean isFullScreen) {
        if (!isFullScreen) {
            mScaleScreen.setImageResource(R.drawable.new_scale_to_smaller);
        } else {
            mScaleScreen.setImageResource(R.drawable.new_scale_to_larger);
        }
    }

    public void lockUnlockControl(boolean lock) {
        mIsLock = lock;
        if (lock) {
            mLockControlView.setImageResource(R.drawable.new_lock_control_start);
            hide();
        } else {
            mLockControlView.setImageResource(R.drawable.new_lock_control_close);
            show();
        }
    }

    public void lockUnlockOrintation(boolean lock) {
        if (lock) {
            mLockOrintationView.setImageResource(R.drawable.new_lock_orientation);
        } else {
            mLockOrintationView.setImageResource(R.drawable.new_unlock_orientation);
        }
        if (!mIsLock) {
            mLockOrintationView.setClickable(true);
            mLockOrintationView.setVisibility(View.VISIBLE);
            /* SPRD:Add for showing the controller @{ */
            if (MovieActivity.getMoviePlayer() != null) {
                MovieActivity.getMoviePlayer().setSystemUiVisibility(true);
            }
            /* End @} */
        } else {
            mLockOrintationView.setClickable(false);
        }
        maybestartHiding();
    }

    private String stringForTime(int millis) {
        int totalSeconds = millis / 1000;
        int seconds = totalSeconds % 60;
        int minutes = (totalSeconds / 60) % 60;
        int hours = totalSeconds / 3600;
        if (hours > 0) {
            return String.format("%d:%02d:%02d", hours, minutes, seconds).toString();
        } else {
            return String.format("%02d:%02d", minutes, seconds).toString();
        }
    }

    private void maybestartHiding(){
        mHander.removeCallbacks(startHidingRunnable);
        mHander.postDelayed(startHidingRunnable, 2500);
    }

    public int getIntFromDimens(int index) {
        int result = this.getResources().getDimensionPixelSize(index);
        return result;
    }

    public void setFloatWindowInvisible(boolean isFloatWindowInvisible) {
        mIsFloatWindowDisabledByTrim = isFloatWindowInvisible;
    }
    /* @} */
}
