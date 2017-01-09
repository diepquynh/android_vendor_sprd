
package com.android.camera.ui;

import android.view.MotionEvent;
import com.android.camera2.R;
import android.animation.ValueAnimator;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.ImageView;

public class VoiceImageView extends ImageView {
    private static final int CIRCLE_DURATION = 10 * 1000;
    private static final float START_DEGREE = 270.0f;
    private Drawable mCircleDrawable;
    private ValueAnimator mValueAnimator;
    private float mDegrees = 0;

    public VoiceImageView(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
        init(context);

    }

    public VoiceImageView(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
        init(context);
    }

    public VoiceImageView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        // TODO Auto-generated constructor stub
        init(context);
    }

    public VoiceImageView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        // TODO Auto-generated constructor stub
        init(context);
    }


    public void init(Context context) {
        mCircleDrawable = context.getDrawable(R.drawable.ic_voice_outline_playing);
        mValueAnimator = ValueAnimator.ofFloat(0.0f, 360.0f);
        mValueAnimator.setDuration(CIRCLE_DURATION);
    }

    public void setDuration(long duration) {
        mValueAnimator.setDuration(duration);
    }

    public void startCircle() {
        mDegrees = 0;
        mValueAnimator.removeAllUpdateListeners();
        mValueAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator arg0) {
                // TODO Auto-generated method stub
                mDegrees = (Float) arg0.getAnimatedValue();
                invalidate();
            }
        });
        mValueAnimator.start();
    }

    public void stopCircle() {
        mValueAnimator.removeAllUpdateListeners();
        mValueAnimator.cancel();
        mDegrees = 0;
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        // TODO Auto-generated method stub
        super.onDraw(canvas);
        canvas.save();
        mCircleDrawable.setBounds(0, 0, getMeasuredWidth(), getMeasuredHeight());
        canvas.clipPath(getPath());
        mCircleDrawable.draw(canvas);
        canvas.restore();
    }

    private Path getPath() {
        Path path = new Path();
        path.addArc(0.0f, 0.0f, (float) getMeasuredWidth(), (float) getMeasuredHeight(),
                START_DEGREE,
                mDegrees);
        path.close();
        return path;
    }

}
