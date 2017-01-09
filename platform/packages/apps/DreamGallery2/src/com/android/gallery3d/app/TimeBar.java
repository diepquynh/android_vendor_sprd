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
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.view.MotionEvent;
import android.view.View;

import com.android.gallery3d.R;
import com.android.gallery3d.common.Utils;

/**
 * The time bar view, which includes the current and total time, the progress
 * bar, and the scrubber.
 */
public class TimeBar extends View {

    public interface Listener {
        void onScrubbingStart();

        void onScrubbingMove(int time);

        void onScrubbingEnd(int time, int start, int end);
    }

    // Padding around the scrubber to increase its touch target
    private static final int SCRUBBER_PADDING_IN_DP = 10;

    // The total padding, top plus bottom
    private static final int V_PADDING_IN_DP = 52;

    private static final int TEXT_SIZE_IN_DP = 14;

    protected final Listener mListener;

    // the bars we use for displaying the progress
    protected final Rect mProgressBar;
    protected final Rect mPlayedBar;

    protected final Paint mProgressPaint;
    protected final Paint mPlayedPaint;
    protected final Paint mTimeTextPaint;

    protected final Bitmap mScrubber;
    protected int mScrubberPadding; // adds some touch tolerance around the
                                    // scrubber

    protected int mScrubberLeft;
    protected int mScrubberTop;
    protected int mScrubberCorrection;
    protected boolean mScrubbing;
    protected boolean mShowTimes;
    protected boolean mShowScrubber;

    protected int mTotalTime;
    protected int mCurrentTime;
    /* SPRD:Modify for new feature 568552:UI check @{*/
    private int mCentralLineY ;
    private float mTextSizeInPx;
    /* Bug568552 End @} */

    protected final Rect mTimeBounds;

    protected int mVPaddingInPx;

    /** SPRD:Bug474600 improve video control functions
     * add new parameters @{ */
    private int currentProgress;
    private boolean isTouchEnable = true;

    /**@}*/

    public TimeBar(Context context, Listener listener) {
        super(context);
        mListener = Utils.checkNotNull(listener);

        mShowTimes = true;
        mShowScrubber = true;

        mProgressBar = new Rect();
        mPlayedBar = new Rect();

        mProgressPaint = new Paint();
        mProgressPaint.setColor(0xFF808080);
        mPlayedPaint = new Paint();
        mPlayedPaint.setColor(0xFF00FFFF);//SPRD:modify for new feature bug568552

        DisplayMetrics metrics = context.getResources().getDisplayMetrics();
        float textSizeInPx = metrics.density * TEXT_SIZE_IN_DP;
        mTextSizeInPx = textSizeInPx; // SPRD:Modify for new feature bug568552:UI check
        mTimeTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mTimeTextPaint.setColor(0xFFCECECE);
        mTimeTextPaint.setTextSize(textSizeInPx);
        mTimeTextPaint.setTextAlign(Paint.Align.CENTER);

        mTimeBounds = new Rect();
        /**SPRD:Bug474600 improve video control functions
         * @{*/
        mTimeTextPaint.getTextBounds("00:00:00", 0, 8, mTimeBounds);
        /**@}*/
        mScrubber = BitmapFactory.decodeResource(getResources(), R.drawable.new_scrubber_knob);//SPRD:Modify for new feature bug568552
        mScrubberPadding = (int) (metrics.density * SCRUBBER_PADDING_IN_DP);

        mVPaddingInPx = (int) (metrics.density * V_PADDING_IN_DP);
    }

    private void update() {
        mPlayedBar.set(mProgressBar);

        if (mTotalTime / 1000 > 0) {// SPRD:Bug474600 change mTotalTime to mTotalTime/1000
                                    // here video's duration >= 1s, mTotalTime's unit is ms.
            /**
             * SPRD:Bug474600 improve video control functions
             * remove @{
             * @orig
             *  mPlayedBar.right = mPlayedBar.left + (int) ((mProgressBar.width() * (long)
             *       mCurrentTime) / mTotalTime); @}
             */
            /** SPRD:Bug474600 improve video control functions
             * change the parameters @{ */
            mPlayedBar.right =
                    mPlayedBar.left
                            + (int) ((mProgressBar.width() * (long) currentProgress) / mTotalTime);
            /**   @}*/
        } else {
            mPlayedBar.right = mProgressBar.left;
        }

        if (!mScrubbing) {
            mScrubberLeft = mPlayedBar.right - mScrubber.getWidth() / 2;
        }
        invalidate();
    }

    /**
     * @return the preferred height of this view, including invisible padding
     */
    public int getPreferredHeight() {
        return mTimeBounds.height() + mVPaddingInPx + mScrubberPadding;
    }

    /**
     * @return the height of the time bar, excluding invisible padding
     */
    public int getBarHeight() {
        return mTimeBounds.height() + mVPaddingInPx;
    }

    public void setTime(int currentTime, int totalTime,
            int trimStartTime, int trimEndTime) {
        /**
         * SPRD:Bug474600 improve video control functions
         * improve video control functions
         * remove @{
         * @orig
         *if (mCurrentTime == currentTime && mTotalTime == totalTime) { return; }
         *@}
         */
        mCurrentTime = currentTime;
        /**SPRD:Bug474600 improve video control functions
         * add new parameter @{*/
        currentProgress = currentTime;
        /**@}*/
        mTotalTime = totalTime;
        update();
    }

    private boolean inScrubber(float x, float y) {
        int scrubberRight = mScrubberLeft + mScrubber.getWidth();
        int scrubberBottom = mScrubberTop + mScrubber.getHeight();
        return mScrubberLeft - mScrubberPadding < x && x < scrubberRight + mScrubberPadding
                && mScrubberTop - mScrubberPadding < y && y < scrubberBottom + mScrubberPadding;
    }
    /** SPRD:Bug474600 improve video control functions
     *  add for TouchEvent options @{ **/
    private boolean inProgressbar(float x, float y){
        int scrubberBottom = mScrubberTop + mScrubber.getHeight();
        // SPRD:Delete for new feature 568552 when you touch the bottom of the screen,the slide-to-adjust-progress will reponse
        return mProgressBar.left < x && x < mProgressBar.right && mScrubberTop - mScrubberPadding < y;
    }

    private int getProgressClickTime(float x) {
        return (int) ((long) (x - mProgressBar.left)
            * mTotalTime / mProgressBar.width());
      }
    /** @} **/
    private void clampScrubber() {
        int half = mScrubber.getWidth() / 2;
        int max = mProgressBar.right - half;
        int min = mProgressBar.left - half;
        mScrubberLeft = Math.min(max, Math.max(min, mScrubberLeft));
    }

    private int getScrubberTime() {
        return (int) ((long) (mScrubberLeft + mScrubber.getWidth() / 2 - mProgressBar.left)
                * mTotalTime / mProgressBar.width());
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        int w = r - l;
        int h = b - t;
        if (!mShowTimes && !mShowScrubber) {
            mProgressBar.set(0, 0, w, h);
        } else {
            int margin = mScrubber.getWidth() / 3;
            if (mShowTimes) {
                margin += mTimeBounds.width();
            }
            int progressY = (h + mScrubberPadding) / 2;
            mCentralLineY = progressY;//Modify for new feature 568552:UI check
            mScrubberTop = progressY - mScrubber.getHeight() / 2 + 1;
            mProgressBar.set(
                    getPaddingLeft() + margin, progressY,
                    w - getPaddingRight() - margin, progressY + 4);
        }
        update();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        // draw progress bars
        canvas.drawRect(mProgressBar, mProgressPaint);
        canvas.drawRect(mPlayedBar, mPlayedPaint);

        // draw scrubber and timers
        if (mShowScrubber) {
            canvas.drawBitmap(mScrubber, mScrubberLeft, mScrubberTop, null);
        }
        if (mShowTimes) {
            canvas.drawText(
                    stringForTime(mCurrentTime),
                            mTimeBounds.width() / 2 + getPaddingLeft(),
                            mCentralLineY + mTextSizeInPx/2 - 1, // SPRD:Modify for new feature 568552:UI check
                    mTimeTextPaint);
            canvas.drawText(
                    stringForTime(mTotalTime),
                            getWidth() - getPaddingRight() - mTimeBounds.width() / 2,
                            mCentralLineY + mTextSizeInPx/2 -1, // SPRD:Modify for new feature 568552:UI check
                    mTimeTextPaint);
        }
    }

    public int getTimeTextCenterLine(){
        return getWidth() - getPaddingRight() - mTimeBounds.width() / 2;
    }

    public int getTimebarStartLine(){
        return mProgressBar.left;
    }
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (mShowScrubber && isTouchEnable) {// SPRD:Bug474600 add
            int x = (int) event.getX();
            int y = (int) event.getY();

            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    /**SPRD:Bug474600 improve video control functions
                     * remove@{
                     * @orig
                    {
                    mScrubberCorrection = inScrubber(x, y)
                            ? x - mScrubberLeft
                            : mScrubber.getWidth() / 2;
                    mScrubbing = true;
                    mListener.onScrubbingStart();
                    }
                    *@}
                    */
                    /**SPRD:Bug474600 improve video control functions
                     * add @{ */
                    if (inScrubber(x, y)) {
                        mScrubberCorrection = x - mScrubberLeft;
                        mScrubbing = true;
                        mListener.onScrubbingStart();
                        return true;
                    } else if (inProgressbar(x, y)) {
                        mListener.onScrubbingEnd(getProgressClickTime(x), 0, 0);
                        return true;
                    }
                    /** @}*/
                   break;
                // fall-through
                case MotionEvent.ACTION_MOVE: {
                    mScrubberLeft = x - mScrubberCorrection;
                    clampScrubber();
                    mCurrentTime = getScrubberTime();
                    mListener.onScrubbingMove(mCurrentTime);
                    invalidate();
                    /**SPRD:Bug474600 improve video control functions
                     * add @{ */
                    if(inScrubber(x, y)){
                        mScrubbing = true;
                    }
                    /** @}*/
                    return true;
                }
                case MotionEvent.ACTION_CANCEL:
                case MotionEvent.ACTION_UP: {
                    /**SPRD:Bug474600 improve video control functions
                     * remove @{
                     * @orig
                    mListener.onScrubbingEnd(getScrubberTime(), 0, 0);
                    mScrubbing = false;
                    return true;
                    *@}
                    */
                    /**SPRD:Bug474600 improve video control functions
                     *  add a if and break statement @{*/
                    if(mScrubbing){
                    mListener.onScrubbingEnd(getScrubberTime(), 0, 0);
                    mScrubbing = false;
                    return true;
                    }
                    break;
                    /**  @}*/
                }
            }
        }
        return false;
    }

    protected String stringForTime(long millis) {
        int totalSeconds = (int) millis / 1000;
        int seconds = totalSeconds % 60;
        int minutes = (totalSeconds / 60) % 60;
        int hours = totalSeconds / 3600;
        if (hours > 0) {
            return String.format("%d:%02d:%02d", hours, minutes, seconds).toString();
        } else {
            return String.format("%02d:%02d", minutes, seconds).toString();
        }
    }

    public void setSeekable(boolean canSeek) {
        mShowScrubber = canSeek;
    }
    /** SPRD:Bug474600 improve video control functions
     * add new methods @{ */
    public void setTimeTextOnly(int currentTime, int totalTime,
            int trimStartTime, int trimEndTime) {
        this.mCurrentTime = currentTime;
        this.currentProgress = 0;
        this.mTotalTime = totalTime;
        update();
    }

    public void resetTime() {
        setTime(0, 0, 0, 0);
      }

    @Override
    public void setEnabled(boolean enabled) {
        isTouchEnable = enabled;
        super.setEnabled(enabled);
    }

    public void setscrubbing() {
        mScrubbing = false;
    }
    /** @} */
}
