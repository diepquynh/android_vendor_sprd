
package com.dream.camera.ui;

import java.util.HashMap;

import android.view.MotionEvent;
import android.view.View;

import com.android.camera.ui.VoiceImageView;
import com.android.camera2.R;
import android.content.Context;
import android.util.AttributeSet;

public class DreamVoiceImageView extends VoiceImageView {
    private DreamVoiceImageOnclickListener mDreamVoiceImageOnclickListener;

    public DreamVoiceImageView(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
        init(context);

    }

    public DreamVoiceImageView(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
        init(context);
    }

    public DreamVoiceImageView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        // TODO Auto-generated constructor stub
        init(context);
    }

    public DreamVoiceImageView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        // TODO Auto-generated constructor stub
        init(context);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        mDreamVoiceImageOnclickListener.onVoicePlayClicked();
        return false;
    }

    public interface DreamVoiceImageOnclickListener {
        public void onVoicePlayClicked();
    }

    public void addListener(DreamVoiceImageOnclickListener listener) {
        mDreamVoiceImageOnclickListener = listener;
    }
}
