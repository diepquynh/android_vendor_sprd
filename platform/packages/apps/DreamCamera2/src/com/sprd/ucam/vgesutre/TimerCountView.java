package com.sprd.ucam.vgesutre;
import android.content.Context;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.ProgressBar;
import com.sprd.hz.selfportrait.MSG;
import com.sprd.hz.selfportrait.detect.DetectEngine;
import com.sprd.hz.selfportrait.util.SoundUtil;
import com.sprd.hz.selfportrait.util.VibrateHelper;
import com.android.camera2.R;
public class TimerCountView extends FrameLayout{
    private int mInterval = 1000;
    private int mCount = 3;
    private ImageView mCountImage = null;
    private ProgressBar bar = null;
    private int mTick = 0;
    private boolean mIsCounting = false;
    private SoundUtil soundUtil;
    private Handler mHandler = null;
    private int[] timeRes = { R.drawable.sound_time_gesture, R.drawable.sound_time_gesture,
            R.drawable.sound_time_gesture };
    private Runnable mTask = new Runnable(){
        @Override
        public void run() {
            if(mTick<mCount)
            {
                //if (isTimeStyle)
                if(true) // ³Â¼Ì ¾õµÃ×ªÈ¦ºÜ³¶µ­
                {
                    //SoundUtil.getInstance().playSound(SoundUtil.SOUND_TIMER);
                    VibrateHelper.Vibrate(mContext, 100);
                    mCountImage.setImageResource(timeRes[mCount - mTick
                            - 1]);
                    soundUtil.playSound(SoundUtil.SOUND_TIMER);
                }
                else
                {
                    bar.setVisibility(View.VISIBLE);
                }
                mHandler.postDelayed(mTask, mInterval);
                mTick ++;
            } else {
                stopTimer();
                mHandler.sendEmptyMessage(MSG.CAMERA_FOCUS_CAPTURE);
            }
        }
    };
    private Context mContext = null;
    public TimerCountView(Context context) {
        super(context);
        mContext = context;
        soundUtil = SoundUtil.getInstance();
        initControls();
    }
    public TimerCountView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        initControls();
    }
    private void initControls() {
        inflate(getContext(), R.layout.camera_panel_timer, this);
        mCountImage = (ImageView)findViewById(R.id.camera_timer_image);
        soundUtil = SoundUtil.getInstance();
        inflate(getContext(), R.layout.camera_panel_progress, this);
        bar=(ProgressBar) findViewById(R.id.time_progressbar);
        setVisibility(View.GONE);
    }
    public void setHandler(Handler h) {
        mHandler = h;
    }
    private void startTimer(int count, int interval)
    {
        mTick = 0;
        mInterval = interval;
        mCount = count;
        //if (isTimeStyle)
        if(true)
        {
            mCountImage.setVisibility(View.VISIBLE);
            mCountImage.setImageResource(timeRes[mCount-1]);
            soundUtil.playSound(SoundUtil.SOUND_TIMER);
            bar.setVisibility(View.GONE);
        }
        else
        {
            mCountImage.setVisibility(View.GONE);
            bar.setVisibility(View.VISIBLE);
        }
        setVisibility(View.VISIBLE);
        mHandler.postDelayed(mTask, 0);
        mIsCounting = true;
    }
    public void startTimer() {
//        timeRes[2] = R.drawable.sound_time_3;
        startTimer(3, 1000);
    }
    public void startTimerFromDetector(int detectMode) {
        switch(detectMode) {
        case DetectEngine.DETECT_TYPE_GESTURE:
            timeRes[2] = R.drawable.sound_time_gesture;
            break;
        }
        startTimer(3, 700);
    }
    public void stopTimer() {
        mHandler.removeCallbacks(mTask);
        setVisibility(View.GONE);
        mIsCounting = false;
    }
    public boolean isTimerCounting() {
        return mIsCounting;
    }
}
