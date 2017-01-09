
package com.dream.camera.ui;

import com.android.camera.debug.Log;
import com.android.camera.util.CameraUtil;
import com.android.camera2.R;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

public class PhotoVoicePlayProgress extends FrameLayout {
    public int mLimit = 10;// secs
    private static final int START_TIME = 1;
    private TextView mPlayTime;
    private TextView mTotalTime;
    private View mProgress;
    private int mSec;

    private Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            // TODO Auto-generated method stub
            switch (msg.what) {
                case START_TIME:
                    mSec++;
                    mPlayTime.setText("00:0"+mSec);
                    if (mSec == 10) {
                        mPlayTime.setText("00:10");
                    } else if (mSec >= mLimit) {
                        break;
                    }
                    mHandler.sendEmptyMessageDelayed(START_TIME, 1000);
                    break;
                default:
                    break;
            }
        }

    };

    public PhotoVoicePlayProgress(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
    }

    public PhotoVoicePlayProgress(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    public PhotoVoicePlayProgress(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        // TODO Auto-generated constructor stub
    }

    public PhotoVoicePlayProgress(Context context, AttributeSet attrs, int defStyleAttr,
            int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        // TODO Auto-generated constructor stub
    }

    @Override
    protected void onFinishInflate() {
        // TODO Auto-generated method stub
        super.onFinishInflate();
        mPlayTime = (TextView) findViewById(R.id.play_time);
        mTotalTime = (TextView) findViewById(R.id.total_time);
        mTotalTime.setText("/00:00");
    }

    public void playVoice() {
        mSec = 0;
        mHandler.sendEmptyMessageDelayed(START_TIME, 1000);
        this.setVisibility(View.VISIBLE);
    }

    public void stopVoice() {
        mHandler.removeMessages(START_TIME);
        mPlayTime.setText("00:00");
        this.setVisibility(View.INVISIBLE);
    }

    public void setDuration(long time) {
        Log.i(new Log.Tag("zf"), "setDuration"+mLimit);
        if ((int)Math.ceil(time/1000) < 9) {
            mTotalTime.setText("/00:0"+(int)Math.ceil(time/1000));
        } else {
            mTotalTime.setText("/00:10");
        }
        mLimit = (int)Math.ceil(time/1000);
    }
}
