package com.sprd.soundrecorder;

import android.app.Activity;
import android.content.Context;
import android.app.ActionBar;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import android.provider.MediaStore;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.android.soundrecorder.R;
import com.android.soundrecorder.Recorder;
import com.android.soundrecorder.SoundRecorder;

import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Button;
import android.widget.ImageButton;
import android.content.ContentValues;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Typeface;
import android.content.Context;
import android.content.res.Resources;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.content.ContentUris;
import android.net.Uri;
import android.widget.Toast;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.KeyEvent;
import android.os.Message;
import android.os.Handler;
import android.graphics.Color;
import android.graphics.drawable.Drawable;

public class RecordingFileClip extends Activity implements
        Button.OnClickListener {
    private String TAG = "RecordingFileClip";
    private TextView mTimerView;
    private Typeface mTypeface;
    private String mTimerFormat;
    private TextView mDuration;
    private String mRecordTitle;
    private Long mRecordID;
    private int mRecordDuration;
    private RelativeLayout mRelativeLayout;
    private ClipWavesView mWavesView;
    private RecordDatabaseHelper mDBHelper;
    private ImageButton mPlayButton;
    private TextView mStartClipTime;
    private TextView mEndClipTime;
    boolean mIsRecordPlaying;
    private RecordingClipPlayer mPlayer = null;
    private AudioManager mAudioManager;
    private boolean mPausedByTransientLossOfFocus;
    private MenuItem mMenuItemClip = null;
    private int spaceTime = 0;

    private final int UPDATE_PLAYED_SHADOW = 1;
    final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
            case UPDATE_PLAYED_SHADOW:
                updateWavesView();
                break;
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().requestFeature(Window.FEATURE_ACTION_BAR);
        setContentView(R.layout.recording_file_clip);
        ActionBar actionBar = getActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        mDBHelper = new RecordDatabaseHelper(this);
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        initResource();
    }

    private void initResource() {
        mPlayButton = (ImageButton) findViewById(R.id.playButton);
        mStartClipTime = (TextView) findViewById(R.id.startClipTime);
        mEndClipTime = (TextView) findViewById(R.id.endClipTime);
        mPlayButton.setImageResource(R.drawable.play);
        mPlayButton.setOnClickListener(this);
        mTimerView = (TextView) findViewById(R.id.timerView);
        mTypeface = Typeface.createFromAsset(getAssets(), "fonts/msyi.ttf");
        mTimerView.setTypeface(mTypeface);

        mTimerFormat = getResources().getString(R.string.timer_format);
        String timeStr = String.format(mTimerFormat, 0, 0, 0);
        mTimerView.setText(timeStr);

        mDuration = (TextView) findViewById(R.id.record_duration);
        mDuration.setText(timeStr);
        mStartClipTime.setText(timeStr);
        mEndClipTime.setText(timeStr);

        Intent intent = getIntent();
        mRecordTitle = intent.getStringExtra("title");
        mRecordID = intent.getLongExtra("id", 0);
        mRecordDuration = intent.getIntExtra("duration", 0);
        Log.d(TAG, "mRecordTitle=" + mRecordTitle + ", mRecordID=" + mRecordID
                + ", mRecordDuration=" + mRecordDuration);

        setTitle(mRecordTitle);
        mDuration.setText(Utils.makeTimeString4MillSec(this, mRecordDuration));
        mRelativeLayout = (RelativeLayout) findViewById(R.id.wavesLayout);
        mWavesView = new ClipWavesView(this);
        mWavesView.setActivity(this);
        mRelativeLayout.addView(mWavesView);

        mWavesView.mTagHashMap = queryTag(mRecordTitle);
        mWavesView.mDataSource = queryWave(mRecordTitle);
        mWavesView.setClipTimeView(mTimerView);
        mWavesView.setStartClipTimeView(mStartClipTime);
        mWavesView.setEndClipTimeView(mEndClipTime);
        //mWavesView.updateTimeView(mEndClipTime,mRecordDuration);
        mWavesView.setRecordDuration(mRecordDuration);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        getMenuInflater().inflate(R.menu.clip_menu, menu);
        mMenuItemClip = menu.findItem(R.id.item_clip);
        mMenuItemClip.setIcon(R.drawable.ic_editor_complete);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case android.R.id.home:
            if (mPlayer != null) {
                stopPlayback();
            }
            onBackPressed();
            break;
        case R.id.item_clip:
            Toast.makeText(this, "enter clip activity", Toast.LENGTH_SHORT)
                    .show();

            break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (mPlayer != null) {
                stopPlayback();
            }
            finish();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onBackPressed() {
        if (isResumed()) {
            super.onBackPressed();
        }
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause");
        super.onPause();
    }

    @Override
    public void onStop() {
        Log.d(TAG, "onStop");
        super.onStop();
    }

    private int queryDuration(String title) {
        int duration = 0;
        Cursor cur = null;
        try {
            StringBuilder where = new StringBuilder();
            where.append(MediaStore.Audio.Media.TITLE).append("='")
                    .append(title).append("'");

            cur = getContentResolver().query(
                    MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                    new String[] { MediaStore.Audio.Media.DURATION },
                    where.toString(), null, null);

            // read cursor
            int index = -1;
            for (cur.moveToFirst(); !cur.isAfterLast(); cur.moveToNext()) {
                duration = cur.getInt(0);
            }
        } catch (Exception e) {
            Log.v(TAG, "RecordingFileClip.queryFile failed; E: " + e);
        } finally {
            if (cur != null)
                cur.close();
        }
        return duration;
    }

    private HashMap queryTag(String title) {
        Cursor cursor = mDBHelper.queryTag(title);
        HashMap<Integer, Integer> hashMap = new HashMap<Integer, Integer>();
        while (cursor.moveToNext()) {
            int tag = cursor.getInt(0);
            int location = cursor.getInt(1);
            hashMap.put(location, tag);
        }
        cursor.close();
        return hashMap;
    }

    private ArrayList queryWave(String title) {
        Cursor cursor = mDBHelper.query(title);
        ArrayList<Float> list = new ArrayList<Float>();
        while (cursor.moveToNext()) {
            float wave = cursor.getFloat(0);
            int tag = cursor.getInt(1);
            list.add(wave);
        }
        cursor.close();
        return list;
    }

    public void onClick(View button) {
        if (!button.isEnabled())
            return;
        switch (button.getId()) {
        case R.id.playButton:
            int startClipTime = mWavesView.getStartClipTime();
            if (mPlayer == null) {
                mPlayer = new RecordingClipPlayer();
                mPlayer.setActivity(this);
                if (mWavesView != null) {
                    mWavesView.setPlayer(mPlayer);
                }
                try {
                    Uri uri = ContentUris.withAppendedId(
                            MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                            mRecordID);
                    mPlayer.setDataSourceAndPrepare(uri);
                } catch (Exception ex) {
                    Log.d(TAG, "Failed to open file: " + ex);
                    Toast.makeText(this, R.string.playback_failed,
                            Toast.LENGTH_SHORT).show();
                    return;
                }
            } else {
                if (mPlayer.isPlaying()) {
                    mPlayer.pause();
                } else {
                    start();
                }
                updatePlayPause();
            }
            break;
        }
    }

    public void start() {
        if (!requestAudioFocus()) {
            Toast.makeText(this.getApplicationContext(),
                    R.string.no_allow_play_calling, Toast.LENGTH_SHORT).show();
            return;
        }
        if (mPlayer != null) {
            if (mIsPlayedOver) {
                mPlayer.seekTo(mWavesView.getStartClipTime());
                mIsPlayedOver = false;
            } else {
                mPlayer.seekTo(mWavesView.mPlayedTime);
            }
        }
        mPlayer.start();
    }

    private boolean requestAudioFocus() {
        int audioFocus = mAudioManager.requestAudioFocus(mAudioFocusListener,
                AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
        if (audioFocus == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            return true;
        }
        return false;
    }

    private OnAudioFocusChangeListener mAudioFocusListener = new OnAudioFocusChangeListener() {
        public void onAudioFocusChange(int focusChange) {
            if (mPlayer == null) {
                mAudioManager.abandonAudioFocus(this);
                return;
            }
            switch (focusChange) {
            case AudioManager.AUDIOFOCUS_LOSS:
                mPausedByTransientLossOfFocus = false;
                mPlayer.pause();
                break;
            case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
            case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                if (mPlayer.isPlaying()) {
                    mPausedByTransientLossOfFocus = true;
                    mPlayer.pause();
                }
                break;
            case AudioManager.AUDIOFOCUS_GAIN:
                if (mPausedByTransientLossOfFocus) {
                    mPausedByTransientLossOfFocus = false;
                    start();
                }
                break;
            }
            // updatePlayPause();
        }
    };

    public void updatePlayPause() {
        if (mPlayButton != null && mPlayer != null) {
            if (mPlayer.isPlaying()) {
                spaceTime = mRecordDuration / (2 * mWavesView.mDataSource.size());
                Message msg = new Message();
                msg.what = UPDATE_PLAYED_SHADOW;
                mHandler.sendMessage(msg);
                mPlayButton.setImageResource(R.drawable.suspended);
            } else {
                mHandler.removeCallbacks(mUpdateWavesView);
                mPlayButton.setImageResource(R.drawable.play);
            }
        }
    }

    private void stopPlayback() {
        if (mPlayer != null) {
            mPlayer.release();
            mPlayer = null;
            mAudioManager.abandonAudioFocus(mAudioFocusListener);
        }
    }

    public void onCompletion() {
        updatePlayPause();
        updateWavesView();
        mIsPlayedOver = true;
    }

    private final Runnable mUpdateWavesView = new Runnable() {
        public void run() {
            updateWavesView();
        }
    };

    public boolean mIsPlayedOver = true;

    public void updateWavesView() {
        if (mPlayer != null && mPlayer.isPlaying()) {
            mWavesView.mPlayedTime = mPlayer.getCurrentPosition();
            int endClipTime = mWavesView.getEndClipTime();
            if (Math.abs(endClipTime - mWavesView.mPlayedTime) < 100) {
                mPlayer.pause();
                mPlayer.seekTo(mWavesView.getStartClipTime());
                mWavesView.mPlayedTime = mWavesView.getStartClipTime();
                mWavesView.invalidate();
                mHandler.removeCallbacks(mUpdateWavesView);
                mPlayButton.setImageResource(R.drawable.play);
                mIsPlayedOver = true;
            } else {
                mWavesView.invalidate();
                updateButton();
                mHandler.postDelayed(mUpdateWavesView, spaceTime);
            }
        } else {
            mWavesView.mPlayedTime = 0;
            mWavesView.invalidate();
        }
    }

    public void updateButton() {
        Drawable drawable;
        if (mWavesView.mTagHashMap.size() > 0 && mWavesView.position == 0) {
            mEndClipTime.setEnabled(true);
            mEndClipTime.setTextColor(Color.parseColor("#4b4c57"));
            drawable = this.getResources().getDrawable(
                    R.drawable.next_tag_default);
            drawable.setBounds(0, 0, drawable.getMinimumWidth(),
                    drawable.getMinimumHeight());
            mEndClipTime.setCompoundDrawables(null, null, drawable, null);
        }

        if (mWavesView.mTagHashMap.containsKey(mWavesView.position)) {
            if (mWavesView.mTagHashMap.get(mWavesView.position) > 0) {
                mStartClipTime.setEnabled(true);
                mStartClipTime.setTextColor(Color.parseColor("#4b4c57"));
                drawable = this.getResources().getDrawable(
                        R.drawable.before_tag_default);
                drawable.setBounds(0, 0, drawable.getMinimumWidth(),
                        drawable.getMinimumHeight());
                mStartClipTime.setCompoundDrawables(drawable, null, null, null);
            } else {
                mStartClipTime.setEnabled(false);
                mStartClipTime.setTextColor(Color.parseColor("#d3d3d3"));
                drawable = this.getResources().getDrawable(
                        R.drawable.before_tag_disabled);
                drawable.setBounds(0, 0, drawable.getMinimumWidth(),
                        drawable.getMinimumHeight());
                mStartClipTime.setCompoundDrawables(drawable, null, null, null);
            }
            if (mWavesView.mTagHashMap.get(mWavesView.position) == (mWavesView.mTagHashMap
                    .size() - 1)) {
                mEndClipTime.setEnabled(false);
                mEndClipTime.setTextColor(Color.parseColor("#d3d3d3"));
                drawable = this.getResources().getDrawable(
                        R.drawable.next_tag_disabled);
                drawable.setBounds(0, 0, drawable.getMinimumWidth(),
                        drawable.getMinimumHeight());
                mEndClipTime.setCompoundDrawables(null, null, drawable, null);
            } else {
                mEndClipTime.setEnabled(true);
                mEndClipTime.setTextColor(Color.parseColor("#4b4c57"));
                drawable = this.getResources().getDrawable(
                        R.drawable.next_tag_default);
                drawable.setBounds(0, 0, drawable.getMinimumWidth(),
                        drawable.getMinimumHeight());
                mEndClipTime.setCompoundDrawables(null, null, drawable, null);
            }
        }
    }

    public class RecordingClipPlayer extends MediaPlayer implements
            OnPreparedListener, OnErrorListener, OnCompletionListener {

        public String TAG = "RecordingClipPlayer";
        private RecordingFileClip mActivity;
        public boolean mIsPrepared = false;
        public int mDuration;
        public boolean mSeeking = false;

        @Override
        public void onPrepared(MediaPlayer mp) {
            mIsPrepared = true;
            mDuration = getDuration();
            mActivity.start();
            mActivity.updatePlayPause();
        }

        @Override
        public void onCompletion(MediaPlayer mp) {
            mIsPrepared = false;
            mActivity.onCompletion();
        }

        @Override
        public boolean onError(MediaPlayer mp, int what, int extra) {
            Toast.makeText(mActivity, R.string.playback_failed,
                    Toast.LENGTH_SHORT).show();
            return false;
        }

        public void seekProgress(int progress) {
            seekTo(progress);
        }

        public void setActivity(Activity activity) {
            mActivity = (RecordingFileClip) activity;
            setOnPreparedListener(this);
            setOnErrorListener(this);
            setOnCompletionListener(this);
        }

        boolean isPrepared() {
            return mIsPrepared;
        }

        public void setDataSourceAndPrepare(Uri uri)
                throws IllegalArgumentException, SecurityException,
                IllegalStateException, IOException {
            setDataSource(mActivity, uri);
            prepareAsync();
        }
    }

}
