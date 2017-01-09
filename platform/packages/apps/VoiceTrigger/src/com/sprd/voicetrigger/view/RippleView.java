
package com.sprd.voicetrigger.view;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.BitmapDrawable;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

import com.sprd.voicetrigger.R;

public class RippleView extends View {
    private static final String TAG = "RippleView";
    private final int DEFALT_TEXT_SIZE = 20;
    private final int DEFAULT_VIEW_WIDTH = 720;
    // the screen width and height
    private int mScreenWidth;
    private int mScreenHeight;
    private Paint mRipplePaint = new Paint();
    private int mViewWeight;
    private int mViewHeight;
    private boolean isStartRipple;
    private int heightPaddingTop;
    private int heightPaddingBottom;
    private int widthPaddingLeft;
    private int widthPaddingRight;
    // the ripple radius
    private int rippleRadius;
    private int rippleFirstRadius;
    private int rippleSecendRadius;
    private int rippleThirdRadius;
    private Paint textPaint = new Paint();
    private String mText;
    private float mTextSize = DEFALT_TEXT_SIZE;

    private boolean enableTouchActionFlag = false;
    // bitmap
    private final BitmapDrawable mBitdra0 = (BitmapDrawable) getContext().getResources()
            .getDrawable(R.drawable.enroll_speach0);
    private final Bitmap mSpeakButtonBitmap0 = mBitdra0.getBitmap();
    private final BitmapDrawable mBitdra1 = (BitmapDrawable) getContext().getResources()
            .getDrawable(R.drawable.enroll_speach1);
    private final Bitmap mSpeakButtonBitmap1 = mBitdra1.getBitmap();
    private final BitmapDrawable mBitdra2 = (BitmapDrawable) getContext().getResources()
            .getDrawable(R.drawable.enroll_speach2);
    private final Bitmap mSpeakButtonBitmap2 = mBitdra2.getBitmap();
    // control witch bitmap will show when start animation
    private int showPic = 0;
    private RippleOnTouchListener mRippleOnTouchListener = null;
    private Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            // TODO Auto-generated method stub
            super.handleMessage(msg);
            invalidate();
            if (isStartRipple) {
                rippleFirstRadius++;
                if (rippleFirstRadius > 100) {
                    rippleFirstRadius = 0;
                }
                rippleSecendRadius++;
                if (rippleSecendRadius > 100) {
                    rippleSecendRadius = 0;
                }
                rippleThirdRadius++;
                if (rippleThirdRadius > 100) {
                    rippleThirdRadius = 0;
                }
                if (showPic > 100) {
                    showPic = 0;
                }
                showPic++;
                sendEmptyMessageDelayed(0, 20);
            }
        }
    };

    public RippleView(Context context) {
        super(context);
        init();
    }

    public RippleView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setAttrs(context, attrs);
        init();
    }

    public RippleView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        setAttrs(context, attrs);
        init();
    }

    private void setAttrs(Context context, AttributeSet attrs) {
        int resourceId = -1;
        TypedArray typedArray = context.obtainStyledAttributes(attrs, R.styleable.RippleView);
        for (int i = 0; i < typedArray.getIndexCount(); i++) {
            int attr = typedArray.getIndex(i);
            switch (attr) {
                case R.styleable.RippleView_text:
                    resourceId = typedArray.getResourceId(R.styleable.RippleView_text, 0);
                    this.setText((String) (resourceId > 0 ? typedArray.getResources().getText(
                            resourceId) : typedArray.getString(R.styleable.RippleView_text)));
                    break;
                case R.styleable.RippleView_text_size:
                    resourceId = typedArray.getResourceId(R.styleable.RippleView_text_size, 0);
                    this.setTextSize(resourceId > 0 ? typedArray.getResources().getDimension(
                            resourceId) : typedArray.getInt(R.styleable.RippleView_text_size,
                            DEFALT_TEXT_SIZE));
                    break;
            }
        }
        typedArray.recycle();
    }

    private void init() {
        Log.d("RippleView", "init");
        rippleFirstRadius = 0;
        rippleSecendRadius = -33;
        rippleThirdRadius = -66;
        // set the paint color with the bitmap's on point color;
        mRipplePaint.setColor(mSpeakButtonBitmap0.getPixel(80, 80));
//        mRipplePaint.setColor(getResources().getColor(R.color.ripple_button_bg));
        mRipplePaint.setAntiAlias(true);
        mRipplePaint.setStyle(Paint.Style.FILL);
        textPaint.setTextSize(mTextSize);
        textPaint.setAntiAlias(true);
        textPaint.setStyle(Paint.Style.FILL);
        textPaint.setColor(Color.WHITE);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        Log.d(TAG, "onMeasure");
        int resultW = 0;

        if (MeasureSpec.getMode(widthMeasureSpec) == MeasureSpec.EXACTLY) {
            resultW = MeasureSpec.getSize(widthMeasureSpec);
        } else {
            if (MeasureSpec.getMode(widthMeasureSpec) == MeasureSpec.AT_MOST) {
                resultW = Math.min(DEFAULT_VIEW_WIDTH, MeasureSpec.getSize(widthMeasureSpec));
            }
        }
        int resultH = 0;
        if (MeasureSpec.getMode(heightMeasureSpec) == MeasureSpec.EXACTLY) {
            resultH = MeasureSpec.getSize(heightMeasureSpec);
        } else {
            if (MeasureSpec.getMode(heightMeasureSpec) == MeasureSpec.AT_MOST) {
                resultH = Math.min(mSpeakButtonBitmap0.getHeight(),
                        MeasureSpec.getSize(heightMeasureSpec));
            }
        }
        mViewWeight = resultW;
        mViewHeight = resultH;
        setMeasuredDimension(resultW, resultH);

        rippleRadius = Math.min(mViewWeight, mViewHeight) * 3 / 10;
        Log.d(TAG, "mBitmapWidth = " + mViewWeight);
        Log.d(TAG, "mBitmapHeight = " + mViewHeight);
        Log.d(TAG, "rippleRadius = " + rippleRadius);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (isStartRipple) {
            float f1 = 2 * Math.min(mViewHeight, mViewWeight) / 1000.0F;
            int firstAlpha = (int) (220.0F - (220.0F - 0.0F) / 100.0F * rippleFirstRadius);
            mRipplePaint.setAlpha(firstAlpha);
            canvas.drawCircle(mViewWeight / 2, mViewHeight / 2, rippleRadius + f1
                    * rippleFirstRadius, mRipplePaint);

            if (rippleSecendRadius >= 0) {
                int secendAlpha = (int) (220.0F - (220.0F - 0.0F) / 100.0F * rippleSecendRadius);
                mRipplePaint.setAlpha(secendAlpha);
                canvas.drawCircle(mViewWeight / 2, mViewHeight / 2, rippleRadius + f1
                        * rippleSecendRadius, mRipplePaint);
            }

            if (rippleThirdRadius >= 0) {
                int thirdAlpha = (int) (220.0F - (220.0F - 0.0F) / 100.0F * rippleThirdRadius);
                mRipplePaint.setAlpha(thirdAlpha);
                canvas.drawCircle(mViewWeight / 2, mViewHeight / 2, rippleRadius + f1
                        * rippleThirdRadius, mRipplePaint);
            }
            // show the bitmap animation
            mRipplePaint.setAlpha(255);
            if (showPic < 33) {
                drawBitmap(canvas, mSpeakButtonBitmap0);
            } else if (showPic >= 33 && showPic < 66) {
                drawBitmap(canvas, mSpeakButtonBitmap1);
            } else {
                drawBitmap(canvas, mSpeakButtonBitmap2);
            }
        } else {
            if (mText != null) {
                float length = textPaint.measureText(mText);
                // why do this mesure ? for more info visit
                // http://blog.csdn.net/linghu_java/article/details/46404081
                float textYCoordinate = textPaint.getFontMetricsInt().bottom;
                canvas.drawText(mText, (mViewWeight - length) / 2, mViewHeight / 2
                        + textYCoordinate, textPaint);
            }
            mRipplePaint.setAlpha(255);
            drawBitmap(canvas, mSpeakButtonBitmap0);
        }
    }

    private void drawBitmap(Canvas canvas, Bitmap bmp) {
        Rect rect = new Rect();
        rect.set(0, 0, bmp.getHeight(), bmp.getWidth());
        RectF rectf = new RectF();
        rectf.set(mViewWeight / 2 - rippleRadius, mViewHeight / 2 - rippleRadius, mViewWeight / 2
                + rippleRadius, mViewHeight / 2 + rippleRadius);
        canvas.drawBitmap(bmp, rect, rectf, textPaint);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        // TODO Auto-generated method stub
        super.onSizeChanged(w, h, oldw, oldh);
        mScreenWidth = w;
        mScreenHeight = h;
        confirmSize();
        invalidate();
    }

    private void confirmSize() {
        int minScreenSize = Math.min(mScreenWidth, mScreenHeight);
        int widthOverSize = mScreenWidth - minScreenSize;
        int heightOverSize = mScreenHeight - minScreenSize;
        heightPaddingTop = (getPaddingTop() + heightOverSize / 2);
        heightPaddingBottom = (getPaddingBottom() + heightOverSize / 2);
        widthPaddingLeft = (getPaddingLeft() + widthOverSize / 2);
        widthPaddingRight = (getPaddingRight() + widthOverSize / 2);

        int width = getWidth();
        int height = getHeight();

        new RectF(widthPaddingLeft, heightPaddingTop, width - widthPaddingRight, height * 2
                - heightPaddingBottom);
    }

    public void setTouchActionEnable(boolean enable) {
        this.enableTouchActionFlag = enable;
    }

    public void stratRipple() {
        if(!isStartRipple){
            isStartRipple = true;
            handler.sendEmptyMessage(0);
        }
    }

    public void stopRipple() {
        if (isStartRipple){
            isStartRipple = false;
            showPic = 0;
            rippleFirstRadius = 0;
            rippleSecendRadius = -33;
            rippleThirdRadius = -66;
            invalidate();
            handler.removeMessages(0);
        }
    }

    /**
     * Title: setRippleOnTouchListener
     * Description:  if you need listen the button status, you can set a listener use this
     * method
     *
     * @param mOnTouchListener
     * @see com.sprd.voicetrigger.view.RippleOnTouchListener
     */
    public void setRippleOnTouchListener(RippleOnTouchListener mOnTouchListener) {
        this.mRippleOnTouchListener = mOnTouchListener;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int mAction = event.getAction();
        float x = event.getX();
        float y = event.getY();

        if (mAction == MotionEvent.ACTION_DOWN) {
            if (x > (mViewWeight - rippleRadius) / 2 && x < (mViewWeight + rippleRadius) / 2) {
                Log.d("RippleView", "in the circle" + x + ":" + y);

                if (enableTouchActionFlag && !isStartRipple && mRippleOnTouchListener != null) {
                    stratRipple();
                    mRippleOnTouchListener.onTouchEvent(this, event);
                    mRippleOnTouchListener.onStart();
                }
                return true;
            } else {
                Log.d("RippleView", "out of circle" + x + ":" + y);
                return false;
            }
        } else if (mAction == MotionEvent.ACTION_UP) {
            if (enableTouchActionFlag && mRippleOnTouchListener != null) {
                // stop animation when up finger
                // stopRipple();
                mRippleOnTouchListener.onActionUP();
            }
            return true;
        }
        return false;
    }

    /**
     * set the view text in button center
     *
     * @param mText the text to display
     */
    public void setText(String mText) {
        this.mText = mText;
    }

    /**
     * set the view text size
     *
     * @param mTextSize the int value for text size
     */
    public void setTextSize(float mTextSize) {
        this.mTextSize = mTextSize;
        textPaint.setTextSize(mTextSize);
    }
}
