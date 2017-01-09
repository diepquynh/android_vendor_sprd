
package com.android.camera.ui;

import com.android.camera.util.CameraUtil;
import com.android.camera2.R;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.TextView;

public class PhotoVoiceRecordProgress extends FrameLayout {
    public static final int LIMIT_TIME = 10;// secs
    private static final int START_TIME = 1;
    private View mTip;
    private View mProgress;
    private TextView mTimeVoices;
    private int mSec = 1;
    private String mSeconds;

    private Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            // TODO Auto-generated method stub
            switch (msg.what) {
                case START_TIME:
                    mSec++;
                    mTimeVoices.setText("(" + mSec + "/" + LIMIT_TIME + ")" + mSeconds);
                    if (mSec > LIMIT_TIME) {
                        break;
                    }
                    mHandler.sendEmptyMessageDelayed(START_TIME, 1000);
                    break;
                default:
                    break;
            }
        }

    };

    public PhotoVoiceRecordProgress(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
    }

    public PhotoVoiceRecordProgress(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    public PhotoVoiceRecordProgress(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        // TODO Auto-generated constructor stub
    }

    public PhotoVoiceRecordProgress(Context context, AttributeSet attrs, int defStyleAttr,
            int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        // TODO Auto-generated constructor stub
    }

    @Override
    protected void onFinishInflate() {
        // TODO Auto-generated method stub
        super.onFinishInflate();
        mTip = findViewById(R.id.tip);
        mProgress = findViewById(R.id.progress);
        int top = getResources().getDimensionPixelSize(R.dimen.photo_voice_record_progress_top);
        mProgress.setPadding(mProgress.getPaddingLeft(), top, mProgress.getPaddingRight(), mProgress.getPaddingBottom());
        mTip.setPadding(mTip.getPaddingLeft(), top, mTip.getPaddingRight(), mTip.getPaddingBottom());
        mTimeVoices = (TextView) findViewById(R.id.time_voices);
        mSec = 1;
        mSeconds = getResources().getString(R.string.camera_record_voice_seconds);
    }

    public void startVoiceRecord() {
        // Bug596356 The Audio Length is one second less than Camera Display
        mSec = 0;
        mTimeVoices.setText("(" + mSec + "/" + LIMIT_TIME + ")" + mSeconds);
        mHandler.sendEmptyMessageDelayed(START_TIME, 1000);
        mProgress.setVisibility(View.VISIBLE);
        mTip.setVisibility(View.INVISIBLE);
    }

    public void stopVoiceRecord() {
        mHandler.removeMessages(START_TIME);
        mProgress.setVisibility(View.INVISIBLE);
        mTip.setVisibility(View.VISIBLE);
    }

    public void showTip() {
        mTip.setVisibility(View.VISIBLE);
    }

    public void hideTip() {
        mTip.setVisibility(View.INVISIBLE);
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);

        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) mTip.getLayoutParams();
        switch (CameraUtil.getDisplayRotation()) {
            case 0:
            case 90:
            case 270:
                if ((params.gravity == (Gravity.TOP | Gravity.CENTER)) && params.topMargin == 10)
                    return;
                params.gravity = Gravity.TOP | Gravity.CENTER;
                params.topMargin = 10;
                break;
            case 180:
                if ((params.gravity == (Gravity.BOTTOM | Gravity.CENTER)) && params.bottomMargin == 10)
                    return;
                params.gravity = Gravity.BOTTOM | Gravity.CENTER;
                params.bottomMargin = 10;
                break;
        }
        mTip.setLayoutParams(params);
    }

}
