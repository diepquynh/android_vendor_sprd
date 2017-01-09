package com.sprd.soundrecorder;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import com.sprd.soundrecorder.RecordingFileClip;
import com.sprd.soundrecorder.Utils;
import com.android.soundrecorder.Recorder;
import com.android.soundrecorder.R;

import android.content.Context;
import android.content.res.Resources;
import android.drm.DrmStore.RightsStatus;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.media.MediaPlayer;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.TextView;

public class ClipWavesView extends View {
    private String TAG = "ClipWavesView";
    private float space;
    private float interval_left_right;
    private float baseValue = 150;
    private float SHADOW_HALF_HEIGHT = 140;
    private Paint paintData, paintLine, paintTag, paintText, paintPlayShadow;
    private float textHei = 0;
    private String dateColor = "#c6c7d1";
    private String tagColor = "#f24a09";
    public List<Float> mDataSource;
    public List<Integer> mTagList;
    public HashMap<Integer, Integer> mTagHashMap;
    private Bitmap bitmap;
    private Path path;
    public boolean mIsClip;
    public int position;
    private final int CLIPLINE_DOTRADIUS = 5;
    private float leftClipLineX = 0;
    private float rightClipLineX = 0;
    private final int IN_LEFTCLIPRECT = 1;
    private final int IN_RIGHTCLIPRECT = 2;
    private final int IN_BLANKPRECT = 0;
    private int startClipTime = 0;
    private int endClipTime = 0;
    private TextView tv_clipTime = null;
    private TextView tv_start_clipTime = null;
    private TextView tv_end_clipTime = null;
    private Context context = null;
    private int mRecordDuration = 0;
    public int mPlayedTime = 0;
    private float playedShadowEndX = 0;
    private MediaPlayer mPlayer = null;
    private RecordingFileClip mActivity = null;

    public ClipWavesView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        init();
        this.context = context;
    }

    public ClipWavesView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
        this.context = context;
    }

    public ClipWavesView(Context context) {
        super(context);
        init();
        this.context = context;
    }

    private void init() {
        Resources res = getResources();
        space = res.getDimension(R.dimen.panit_line_space);
        interval_left_right = space / 3;

        paintData = new Paint();
        paintDataWidth = space * 0.2f;
        paintData.setStrokeWidth(paintDataWidth);

        paintData.setColor(Color.parseColor(dateColor));

        paintLine = new Paint();
        //paintLine.setAntiAlias(true);
        paintLine.setStrokeWidth(space * 0.2f);
        paintLine.setColor(Color.parseColor(tagColor));

        paintTag = new Paint();
        paintTag.setStrokeWidth(space * 0.2f);
        paintTag.setColor(Color.parseColor(tagColor));
        paintTag.setAlpha(100);

        paintPlayShadow = new Paint(paintTag);
        paintPlayShadow.setAlpha(30);

        paintText = new Paint();
        textHei = space * 0.8f;
        paintText.setTextSize(textHei);
        paintText.setColor(Color.WHITE);

        mDataSource = new ArrayList<Float>();
        mTagList = new ArrayList<Integer>();
        mTagHashMap = new HashMap<Integer, Integer>();

        leftClipLineX = CLIPLINE_DOTRADIUS;
        bitmap = BitmapFactory.decodeResource(res, R.drawable.tag);
        invalidate();
    }

    public void setClipTimeView(TextView view) {
        this.tv_clipTime = view;
    }

    public void setStartClipTimeView(TextView view) {
        this.tv_start_clipTime = view;
    }

    public void setEndClipTimeView(TextView view) {
        this.tv_end_clipTime = view;
    }

    public void setPlayer(MediaPlayer player) {
        this.mPlayer = player;
    }

    public void setActivity(RecordingFileClip activity) {
        this.mActivity = activity;
    }

    public void setRecordDuration(int duration) {
        this.mRecordDuration = duration;
        this.startClipTime = 0;
        this.endClipTime = duration;
        upDateTimeViews();
    }

    private void upDateTimeViews() {
        updateTimeView(tv_start_clipTime, startClipTime);
        updateTimeView(tv_end_clipTime, endClipTime);
        updateTimeView(tv_clipTime, (endClipTime - startClipTime));
    }

    public void updateDate(float db) {
        mDataSource.add(db);
    }

    public void updateTimeView(TextView view, int time) {
        if (time >= 500) {
            view.setText(Utils.makeTimeString4MillSec(this.context, time));
        } else {
            String timeStr = String.format(
                    getResources().getString(R.string.timer_format), 0, 0, 0);
            view.setText(timeStr);
        }
    }

    public int getStartClipTime() {
        return this.startClipTime;
    }

    public int getEndClipTime() {
        return this.endClipTime;
    }

    private int getTouchSection(MotionEvent event) {
        int X = (int) event.getX();
        float leftClipRect_left = leftClipLineX - CLIPLINE_RECT_WIDTH;
        float leftClipRect_right = leftClipLineX + CLIPLINE_RECT_WIDTH;
        if (leftClipRect_left <= 0) {
            leftClipRect_left = 0;
        }
        float rightClipRect_left = rightClipLineX - CLIPLINE_RECT_WIDTH;
        float rightClipRect_right = rightClipLineX + CLIPLINE_RECT_WIDTH;
        if (rightClipRect_right >= getWidth()) {
            rightClipRect_right = getWidth();
        }
        if (leftClipRect_left <= X && X <= leftClipRect_right) {
            return IN_LEFTCLIPRECT;
        } else if (rightClipRect_left <= X && X <= rightClipRect_right) {
            return IN_RIGHTCLIPRECT;
        } else {
            return IN_BLANKPRECT;
        }
    }

    int endX = 0;
    int lastEndX = 0;
    int whichRect = IN_BLANKPRECT;
    int CLIPLINE_RECT_WIDTH = 10;
    boolean isFirstState = true;
    float playDelta = 0;

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getAction()) {
        case MotionEvent.ACTION_DOWN:
            isFirstState = false;
            lastEndX = (int) event.getX();
            whichRect = getTouchSection(event);
            /*if (whichRect == IN_BLANKPRECT) {
                return false;
            }*/
            if (whichRect == IN_LEFTCLIPRECT) {
                playDelta = playedShadowEndX - leftClipLineX;
            }
            break;
        case MotionEvent.ACTION_MOVE:
            endX = (int) event.getX();
            int deltaX = endX - lastEndX;
            if (deltaX != 0 && whichRect == IN_LEFTCLIPRECT) {
                leftClipLineX += deltaX;
                if (leftClipLineX < CLIPLINE_DOTRADIUS) {
                    leftClipLineX = CLIPLINE_DOTRADIUS;
                }
                if (leftClipLineX > rightClipLineX - CLIPLINE_RECT_WIDTH) {
                    leftClipLineX -= deltaX;
                    break;
                }
                playedShadowEndX = playDelta + leftClipLineX;
                if (playedShadowEndX < rightClipLineX) {
                    mPlayedTime = (int) (mRecordDuration * playedShadowEndX / ((float) (getWidth() - CLIPLINE_DOTRADIUS * 2)));
                    if (this.mPlayer != null) {
                        this.mPlayer.seekTo(mPlayedTime);
                    }
                } else {
                    playedShadowEndX = rightClipLineX;
                    mPlayedTime = startClipTime;
                    if (this.mPlayer != null) {
                        if (mPlayer.isPlaying()) {
                            mPlayer.pause();
                        }
                        this.mActivity.mIsPlayedOver = true;
                        this.mActivity.updatePlayPause();
                        //mPlayedTime = startClipTime;
                    }
                }
                invalidate();
            } else if (deltaX != 0 && whichRect == IN_RIGHTCLIPRECT) {
                rightClipLineX += deltaX;
                if (rightClipLineX > getWidth() - CLIPLINE_DOTRADIUS) {
                    rightClipLineX = getWidth() - CLIPLINE_DOTRADIUS;
                }
                if (rightClipLineX < leftClipLineX + CLIPLINE_RECT_WIDTH
                        || playedShadowEndX > rightClipLineX) {
                    rightClipLineX -= deltaX;
                    if (mPlayer.isPlaying()) {
                        mPlayer.pause();

                    }
                    this.mActivity.mIsPlayedOver = true;
                    this.mActivity.updatePlayPause();
                    mPlayedTime = startClipTime;
                    //break;
                }
                invalidate();
            } else if (deltaX != 0 && whichRect == IN_BLANKPRECT) {
                if (endX < playedShadowEndX) {
                    playedShadowEndX += deltaX;
                    if (playedShadowEndX < leftClipLineX + CLIPLINE_RECT_WIDTH
                            || playedShadowEndX > rightClipLineX) {
                        if (mPlayer.isPlaying()) {
                            playedShadowEndX -= deltaX;
                            mPlayer.pause();
                            this.mActivity.mIsPlayedOver = true;
                            this.mActivity.updatePlayPause();
                            mPlayedTime = startClipTime;
                        } else {
                            if (playedShadowEndX > rightClipLineX) {
                                mPlayedTime = startClipTime;
                            }
                        }
                    } else {
                        if (this.mPlayer != null) {
                            mPlayedTime = (int) (mRecordDuration
                                    * playedShadowEndX / ((float) (getWidth() - CLIPLINE_DOTRADIUS * 2)));
                            this.mPlayer.seekTo(mPlayedTime);
                        }
                    }
                    invalidate();
                }
            }
            lastEndX = endX;
            break;
        case MotionEvent.ACTION_UP:
            switch (whichRect) {
            case IN_LEFTCLIPRECT:
                startClipTime = (int) (mRecordDuration * leftClipLineX / ((float) (getWidth() - CLIPLINE_DOTRADIUS * 2)));
                upDateTimeViews();
                break;
            case IN_RIGHTCLIPRECT:
                endClipTime = (int) (mRecordDuration * rightClipLineX / ((float) (getWidth() - CLIPLINE_DOTRADIUS * 2)));
                upDateTimeViews();
                break;
            case IN_BLANKPRECT:
                mPlayedTime = (int) (mRecordDuration * playedShadowEndX / ((float) (getWidth() - CLIPLINE_DOTRADIUS * 2)));
                break;
            default:
                break;
            }
            break;
        }
        return true;
    }

    int count = 0;
    int tagNumber = 0;
    int visibleLines = 0;
    float paintDataWidth = 0;

    @Override
    protected void onDraw(Canvas canvas) {
        // draw wave base line in horization orientation
        if (isFirstState) {
            rightClipLineX = getWidth() - CLIPLINE_DOTRADIUS;
        }
        canvas.drawLine(CLIPLINE_DOTRADIUS, getHeight() - baseValue, getWidth()
                - CLIPLINE_DOTRADIUS * 2, getHeight() - baseValue, paintData);
        if (mDataSource == null || mDataSource.size() == 0)
            return;

        int start = 0;
        int end = 0;
        int maxvisibleLines = getWidth() - CLIPLINE_DOTRADIUS * 2;
        int normalvisibleLines = (int) ((getWidth() - CLIPLINE_DOTRADIUS * 2) / interval_left_right);
        // draw waves
        if (mDataSource.size() < normalvisibleLines) {
            visibleLines = mDataSource.size();
            for (int i = 0; i < visibleLines; i++) {
                canvas.drawLine(CLIPLINE_DOTRADIUS + i * interval_left_right,
                        getHeight() - baseValue - mDataSource.get(i),
                        CLIPLINE_DOTRADIUS + i * interval_left_right,
                        getHeight() - baseValue + mDataSource.get(i), paintData);
            }
        } else if (normalvisibleLines <= mDataSource.size()
                && mDataSource.size() < maxvisibleLines) {
            visibleLines = mDataSource.size();
            float drawSpace = (float) (getWidth() - CLIPLINE_DOTRADIUS * 2)
                    / visibleLines;
            paintDataWidth = 2;
            paintData.setStrokeWidth(paintDataWidth);
            for (int i = 0; i < visibleLines; i++) {
                canvas.drawLine(CLIPLINE_DOTRADIUS + i * (drawSpace),
                        getHeight() - baseValue - mDataSource.get(i),
                        CLIPLINE_DOTRADIUS + i * (drawSpace), getHeight()
                                - baseValue + mDataSource.get(i), paintData);
            }
        } else {
            visibleLines = maxvisibleLines;
            float drawSpace = (float) (getWidth() - CLIPLINE_DOTRADIUS * 2)
                    / visibleLines;
            int dataInterval = (int) (mDataSource.size() / visibleLines);
            int dataIndex = 0;
            int nextDataIndex = 0;
            paintDataWidth = 1;
            paintData.setStrokeWidth(paintDataWidth);
            for (int i = 0; i < visibleLines; i++) {
                dataIndex = dataInterval * i;
                nextDataIndex = dataInterval * (i + 1);
                if (nextDataIndex > mDataSource.size()) {
                    nextDataIndex = mDataSource.size() - 1;
                }
                float partMaxValue = mDataSource.get(dataIndex);
                for (int j = dataIndex; j < nextDataIndex; j++) {
                    if (partMaxValue < mDataSource.get(j)) {
                        partMaxValue = mDataSource.get(j);
                    }
                }
                canvas.drawLine(CLIPLINE_DOTRADIUS + i * (drawSpace),
                        getHeight() - baseValue - partMaxValue,
                        CLIPLINE_DOTRADIUS + i * (drawSpace), getHeight()
                                - baseValue + partMaxValue, paintData);
            }
        }

        // draw tags
        Iterator<HashMap.Entry<Integer, Integer>> Iterator = mTagHashMap
                .entrySet().iterator();
        float textPosY = getHeight() - baseValue - 120 + bitmap.getHeight() / 2
                + textHei / 2;
        float posX = CLIPLINE_DOTRADIUS;
        while (Iterator.hasNext()) {
            Map.Entry<Integer, Integer> entry = Iterator.next();
            Integer key = entry.getKey();
            Integer value = entry.getValue();
            if (value == mTagHashMap.size()) {
                continue;
            }
            if (mDataSource.size() <= normalvisibleLines) {
                posX = interval_left_right * key;
            } else {
                posX = getWidth() * key / mDataSource.size();
            }
            canvas.drawBitmap(bitmap, posX, getHeight() - baseValue - 120,
                    paintTag);
            paintTag.setStrokeWidth(paintDataWidth);
            canvas.drawLine(posX + paintDataWidth / 2, getHeight() - baseValue
                    - 120 + bitmap.getHeight(), posX + paintDataWidth / 2,
                    getHeight() - baseValue + 120, paintTag);
            tagNumber = value;
            if (tagNumber < 10) {
                canvas.drawText("" + tagNumber, posX + bitmap.getWidth() / 2
                        - textHei / 2, textPosY, paintText);
            } else {
                canvas.drawText("" + tagNumber, posX + bitmap.getWidth() / 2
                        - textHei, textPosY, paintText);
            }
        }

        // draw left clip line
        canvas.drawLine(leftClipLineX, getHeight() - baseValue
                - SHADOW_HALF_HEIGHT - 5, leftClipLineX, getHeight()
                - baseValue + SHADOW_HALF_HEIGHT + 5, paintLine);
        canvas.drawCircle(leftClipLineX, getHeight() - baseValue
                + SHADOW_HALF_HEIGHT + 5, CLIPLINE_DOTRADIUS, paintLine);
        // draw right clip line
        canvas.drawLine(rightClipLineX, getHeight() - baseValue
                - SHADOW_HALF_HEIGHT - 5, rightClipLineX, getHeight()
                - baseValue + SHADOW_HALF_HEIGHT + 5, paintLine);
        canvas.drawCircle(rightClipLineX, getHeight() - baseValue
                - SHADOW_HALF_HEIGHT - 5, CLIPLINE_DOTRADIUS, paintLine);
        // draw select shadow
        paintPlayShadow.setAlpha(30);
        canvas.drawRect(leftClipLineX, getHeight() - baseValue
                - SHADOW_HALF_HEIGHT, rightClipLineX, getHeight() - baseValue
                + SHADOW_HALF_HEIGHT, paintPlayShadow);
        // draw played shadow
        playedShadowEndX = ((float) mPlayedTime / mRecordDuration)
                * (getWidth() - CLIPLINE_DOTRADIUS * 2);
        paintPlayShadow.setAlpha(70);
        canvas.drawRect(leftClipLineX, getHeight() - baseValue
                - SHADOW_HALF_HEIGHT, playedShadowEndX, getHeight() - baseValue
                + SHADOW_HALF_HEIGHT, paintPlayShadow);
    }
}
