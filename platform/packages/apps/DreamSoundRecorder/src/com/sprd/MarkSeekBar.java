package com.sprd.soundrecorder;

import java.util.ArrayList;

import com.android.soundrecorder.R;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.graphics.Paint.FontMetrics;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.SeekBar;

public class MarkSeekBar extends SeekBar {

    private ArrayList<Mark> mMarks = new ArrayList<Mark>();
    private Paint mShapePaint;
    private Resources res;
    private int progressMax;
    private Rect mBarRect = new Rect();
    private int mDuration;
    private Bitmap bitmap;
    private int bitmapHeight;
    private String playedTagColor = "#57575c";
    private String tagColor = "#acacac";

    public MarkSeekBar(Context context) {
        super(context);
        init();
    }

    public MarkSeekBar(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public MarkSeekBar(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
    }

    private void init() {
        res = getResources();
        initDraw();
        progressMax = getMax();
    }

    private void initDraw() {
        mShapePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        mBarRect.left = getPaddingLeft() ;
        mBarRect.right = getWidth() - getPaddingRight();
        final boolean isLayoutRtl = isLayoutRtl();//bug 632174 Playback Marked recording, the mark position is opposite to the actual position
        float cx;
        float cy = getPaddingTop()+getHeight()/2;
        float barRectwidth = mBarRect.width();
        float radius;
        int markPostion;
        if (mMarks.size() > 0) {
            for (int i = 0; i < mMarks.size(); i++) {
                markPostion = mMarks.get(i).postion;
                if (getProgress() > markPostion) {
                    mShapePaint.setColor(Color.parseColor(playedTagColor));
                } else {
                    mShapePaint.setColor(Color.parseColor(tagColor));
                }
                //bug 632174 Playback Marked recording, the mark position is opposite to the actual position begin
                if (isLayoutRtl){
                    cx = mBarRect.right - barRectwidth
                        * (mMarks.get(i).postion / (float) mDuration);
                }else{
                    cx = mBarRect.left + barRectwidth
                        * (mMarks.get(i).postion / (float) mDuration);
                }
                //bug 632174 end
                radius = getResources().getDimension(R.dimen.panit_line_space)/5;
                canvas.drawCircle(cx, cy, radius, mShapePaint);
            }
        }
        super.onDraw(canvas);
    }

    public void setMyPadding(int left, int top, int right, int bottom) {
        setPadding(left, top, right, bottom);
    }

    public void addMark(int postion) {
        mMarks.add(new Mark(postion));
    }

    public void clearAllMarks() {
        mMarks.clear();
    }

    public void setDuration(int duration) {
        mDuration = duration;
    }

    public class Mark extends RectF {
        public Mark(int postion) {
            this.postion = postion;
        }

        public int postion;

        @Override
        public String toString() {
            return "Mark [postion=" + postion + ", left=" + left + ", top="
                    + top + ", right=" + right + ", bottom=" + bottom + "]";
        }
    }
}
