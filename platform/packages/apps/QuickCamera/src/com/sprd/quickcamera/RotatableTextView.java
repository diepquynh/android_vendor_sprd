package com.sprd.quickcamera;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.graphics.Point;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;


public class RotatableTextView extends TextView {

    private final int TEXTVIEW_PADDING = 40;

    private int mTextViewRotation = 0;
    private int mTranslateX = 0;
    private int mTranslateY = 0;
    private int mLcdWidth = 480;
    private int mLcdHeight = 640;

    public RotatableTextView(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
    }

    public RotatableTextView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        // TODO Auto-generated constructor stub
    }

    public RotatableTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    public void setTextViewRotation(int rotation) {
        Point point = new Point();
        //fix coverity issue 108982
        if (getDisplay() != null) getDisplay().getSize(point);
        mLcdWidth = Math.min(point.x, point.y);
        mLcdHeight = Math.max(point.x, point.y);
        Log.v("RotatableTextView", "lcdWidth = " + mLcdWidth + "; lcdHeight = " + mLcdHeight);

        switch (rotation) {
        case 0:
            mTextViewRotation = 90;
            mTranslateX = mLcdWidth - TEXTVIEW_PADDING;
            mTranslateY = 0;
            break;
        case 90:
            mTextViewRotation = 0;
            mTranslateX = 0;
            mTranslateY = 0;
            break;

        case 180:
            mTextViewRotation = 270;
            mTranslateX = 0;
            mTranslateY = mLcdHeight - TEXTVIEW_PADDING;
            break;
        case 270:
            mTextViewRotation = 180;
            mTranslateX = mLcdWidth - TEXTVIEW_PADDING;
            mTranslateY = mLcdHeight - TEXTVIEW_PADDING;
            break;

        default:
            mTextViewRotation = rotation;
            break;
        }

    }

    /*
     * (non-Javadoc)
     *
     * @see android.widget.TextView#onDraw(android.graphics.Canvas)
     */
    @Override
    protected void onDraw(Canvas canvas) {
        // TODO Auto-generated method stub
        canvas.translate(mTranslateX, mTranslateY);
        canvas.rotate(mTextViewRotation);
        super.onDraw(canvas);
    }

    /*
     * (non-Javadoc)
     *
     * @see android.widget.TextView#onMeasure(int, int)
     */
    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        // TODO Auto-generated method stub
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        setMeasuredDimension(mLcdWidth, mLcdHeight);
    }

}
