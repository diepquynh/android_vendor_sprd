package com.sprd.soundrecorder;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.android.soundrecorder.Recorder;
import com.android.soundrecorder.R;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup.LayoutParams;

public class RecordWavesView extends View {
    private String TAG = "RecordWavesView";
    private float mSpace;
    public float mWavesInterval;
    private Paint mPaintDate, mPaintLine, mPaintTag, mPaintText;
    private String mDateColor = "#c6c7d1";
    private String mTagColor = "#f24a09";
    public List<Float> mWaveDataList;
    public List<Integer> mTagList;
    public HashMap<Integer, Integer> mTagHashMap;
    public boolean mIsRecordPlay;
    private Bitmap mBitmap;
    public int mPosition;
    public int mCount = 0;
    public int mTagNumber = 0;
    public int mVisibleLines = 0;
    public float mWaveBaseValue;
    public int mLastSize = 0;

    public RecordWavesView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
    }

    public RecordWavesView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public RecordWavesView(Context context) {
        super(context);
        init();
    }

    private void init() {
        Resources res = getResources();
        mSpace = res.getDimension(R.dimen.panit_line_space);
        mWavesInterval = mSpace / 3;

        mPaintDate = new Paint();
        mPaintDate.setStrokeWidth(mSpace * 0.2f);
        mPaintDate.setColor(Color.parseColor(mDateColor));

        mPaintLine = new Paint();
        mPaintLine.setStrokeWidth(mSpace * 0.2f);
        mPaintLine.setColor(Color.parseColor(mTagColor));

        mPaintTag = new Paint();
        mPaintTag.setStrokeWidth(mSpace * 0.2f);
        mPaintTag.setColor(Color.parseColor(mTagColor));
        mPaintTag.setAlpha(100);

        mPaintText = new Paint();
        mPaintText.setTextSize(mSpace * 0.8f);
        mPaintText.setColor(Color.WHITE);

        mWaveDataList = new ArrayList<Float>();
        mTagList = new ArrayList<Integer>();
        mTagHashMap = new HashMap<Integer, Integer>();

        mBitmap = BitmapFactory.decodeResource(res, R.drawable.tag);
        invalidate();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        canvas.drawLine(0, getHeight() / 2, getWidth(), getHeight() / 2,
                mPaintDate);
        if (mWaveDataList == null || mWaveDataList.size() == 0)
            return;

        int start = 0;
        int end = 0;
        mWaveBaseValue = (float) getHeight() / (2 * 100);

        if (mIsRecordPlay) {
            mVisibleLines = (int) (getWidth() / mWavesInterval);
            start = mPosition > mVisibleLines / 2 ? (mPosition - mVisibleLines / 2)
                    : 0;
            end = mWaveDataList.size() > (start + mVisibleLines) ? (start + mVisibleLines)
                    : mWaveDataList.size();
            if (mPosition == 0 || mCount != mPosition) {
                mCount = 0;
            } else {
                mCount = 1;
            }
            for (int i = start; i < end; i++) {
                canvas.drawLine(
                        getWidth() / 2 + (i - mPosition) * mWavesInterval
                                - (mCount) * mWavesInterval / 2,
                        getHeight() / 2 - mWaveBaseValue * mWaveDataList.get(i),
                        getWidth() / 2 + (i - mPosition) * mWavesInterval
                                - (mCount) * mWavesInterval / 2, getHeight()
                                / 2 + mWaveBaseValue * mWaveDataList.get(i),
                        mPaintDate);

                if (mTagHashMap.containsKey(i)) {
                    canvas.drawBitmap(mBitmap, getWidth() / 2 + (i - mPosition)
                            * mWavesInterval - (mCount) * mWavesInterval / 2
                            - mSpace * 0.1f, getHeight() / 2 - mWaveBaseValue
                            * 70, mPaintTag);

                    canvas.drawLine(
                            getWidth() / 2 + (i - mPosition) * mWavesInterval
                                    - (mCount) * mWavesInterval / 2,
                            getHeight() / 2 - mWaveBaseValue * 70
                                    + mBitmap.getHeight(), getWidth() / 2
                                    + (i - mPosition) * mWavesInterval
                                    - (mCount) * mWavesInterval / 2,
                            getHeight() / 2 + mWaveBaseValue * 70, mPaintTag);
                    mTagNumber = mTagHashMap.get(i);
                    if (mTagNumber < 10) {
                        canvas.drawText(
                                "" + mTagNumber,
                                getWidth() / 2 + (i - mPosition)
                                        * mWavesInterval - (mCount)
                                        * mWavesInterval / 2 + mSpace * 0.5f,
                                getHeight() / 2 - mWaveBaseValue * 70
                                        + mBitmap.getHeight() * 2 / 3,
                                mPaintText);
                    } else {
                        canvas.drawText(
                                "" + mTagNumber,
                                getWidth() / 2 + (i - mPosition)
                                        * mWavesInterval - (mCount)
                                        * mWavesInterval / 2 + mSpace * 0.3f,
                                getHeight() / 2 - mWaveBaseValue * 70
                                        + mBitmap.getHeight() * 2 / 3,
                                mPaintText);
                    }
                }
            }
            mCount = mPosition;
        } else {
            if (mWaveDataList.size() != mLastSize) {
                mLastSize = mWaveDataList.size();
                mCount = 0;
                Log.d(TAG,"onDraw set mCount = 0");
            } else {
                mCount = 1;
                Log.d(TAG,"onDraw mCount = "+mCount);
            }
            mVisibleLines = (int) (getWidth() / (2 * mWavesInterval));
            start = mWaveDataList.size() > mVisibleLines ? (mWaveDataList
                    .size() - mVisibleLines) : 0;
            for (int i = start; i < mWaveDataList.size(); i++) {
                canvas.drawLine(
                        getWidth() / 2 - (mWaveDataList.size() - i - 1)
                                * mWavesInterval - mCount * mWavesInterval / 2,
                        getHeight() / 2 - mWaveBaseValue * mWaveDataList.get(i),
                        getWidth() / 2 - (mWaveDataList.size() - i - 1)
                                * mWavesInterval - mCount * mWavesInterval / 2,
                        getHeight() / 2 + mWaveBaseValue * mWaveDataList.get(i),
                        mPaintDate);

                if (mTagHashMap.containsKey(i)) {
                    canvas.drawBitmap(mBitmap,
                            getWidth() / 2 - (mWaveDataList.size() - i - 1)
                                    * mWavesInterval - mCount * mWavesInterval
                                    / 2 - mSpace * 0.1f, getHeight() / 2
                                    - mWaveBaseValue * 70, mPaintTag);

                    canvas.drawLine(getWidth() / 2
                            - (mWaveDataList.size() - i - 1) * mWavesInterval
                            - mCount * mWavesInterval / 2, getHeight() / 2
                            - mWaveBaseValue * 70 + mBitmap.getHeight(),
                            getWidth() / 2 - (mWaveDataList.size() - i - 1)
                                    * mWavesInterval - mCount * mWavesInterval
                                    / 2, getHeight() / 2 + mWaveBaseValue * 70,
                            mPaintTag);
                    mTagNumber = mTagHashMap.get(i);
                    if (mTagNumber < 10) {
                        canvas.drawText("" + mTagNumber, getWidth() / 2
                                - (mWaveDataList.size() - i - 1)
                                * mWavesInterval - mCount * mWavesInterval / 2
                                + mSpace * 0.5f, getHeight() / 2
                                - mWaveBaseValue * 70 + mBitmap.getHeight() * 2
                                / 3, mPaintText);
                    } else {
                        canvas.drawText("" + mTagNumber, getWidth() / 2
                                - (mWaveDataList.size() - i - 1)
                                * mWavesInterval - mCount * mWavesInterval / 2
                                + mSpace * 0.3f, getHeight() / 2
                                - mWaveBaseValue * 70 + mBitmap.getHeight() * 2
                                / 3, mPaintText);
                    }
                }
            }
        }
        canvas.drawLine(getWidth() / 2, getHeight() / 2 - mWaveBaseValue * 70,
                getWidth() / 2, getHeight() / 2 + mWaveBaseValue * 70,
                mPaintLine);
    }
}
