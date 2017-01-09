/** Created by Spreadst */
package com.android.gallery3d.app;

import android.annotation.AttrRes;
import android.annotation.NonNull;
import android.annotation.Nullable;
import android.annotation.StyleRes;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.*;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

/**
 * Used to display the progress of sound playback.
 */
public class PhotoVoiceProgress extends TextView {

    private static final String TAG = "PhotoVoiceProgress";

    private static final int MSG_UPDATE_TIME = 1;
    private static final int UPDATE_TIME_INTERVAL = 1000;
    private static final int TEXT_SIZE = 20;

    private final int mDefaultTotalTime = 10;
    private int mCurrentTotalTime = mDefaultTotalTime;
    private int mCurrentTime = 0;

    private Handler mHandler = new Handler(){

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (mCurrentTime <= mCurrentTotalTime){
                setTime(String.valueOf(mCurrentTime));
                mHandler.sendEmptyMessageDelayed(MSG_UPDATE_TIME, UPDATE_TIME_INTERVAL);
                mCurrentTime++;
            }
        }
    };

    public PhotoVoiceProgress(@NonNull Context context) {
        super(context);
        setTextSize(TypedValue.COMPLEX_UNIT_DIP, TEXT_SIZE);
    }

    public PhotoVoiceProgress(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        setTextSize(TypedValue.COMPLEX_UNIT_DIP, TEXT_SIZE);
    }

    public PhotoVoiceProgress(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        setTextSize(TypedValue.COMPLEX_UNIT_DIP, TEXT_SIZE);
    }

    public PhotoVoiceProgress(@NonNull Context context, @Nullable AttributeSet attrs, @AttrRes int defStyleAttr, @StyleRes int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        setTextSize(TypedValue.COMPLEX_UNIT_DIP, TEXT_SIZE);
    }

    public void startShowTime(){
        mCurrentTime = 0;
        setText("0");
        setVisibility(View.VISIBLE);
        mHandler.sendEmptyMessage(MSG_UPDATE_TIME);
    }

    private void setTime(String text) {
        Log.d(TAG, "setTime: " + text);
        String time = "00:";
        if (text.equals("10")){
            time += text + "/00:";
        }else {
            time += "0" + text + "/00:";
        }

        if (mCurrentTotalTime == 10){
            time += "10";
        }else {
            time += "0" + mCurrentTotalTime;
        }
        setText(time);
    }

    public void stopShowTime(){
        Log.d(TAG, "stopShowTime");
        mCurrentTime = 0;
        setVisibility(View.GONE);
        mHandler.removeMessages(MSG_UPDATE_TIME);
    }

    public void setTotalTime(int totalTime){
        Log.d(TAG, "setTotalTime: " + totalTime);
        int time = totalTime / 1000;
        if (time == 9) time = 10;
        if (time <= mDefaultTotalTime && time > 0){
            mCurrentTotalTime = time;
        }else {
            mCurrentTotalTime = mDefaultTotalTime;
        }
    }


}
