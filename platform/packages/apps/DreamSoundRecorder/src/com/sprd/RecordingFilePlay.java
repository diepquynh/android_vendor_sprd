package com.sprd.soundrecorder;

import android.app.Activity;
import android.content.Context;
import android.app.ActionBar;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import android.Manifest;
import android.content.pm.PackageManager;
import android.provider.MediaStore;
import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.Iterator;
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
import android.media.AudioManager.OnAudioFocusChangeListener;
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
import android.view.MotionEvent;
import android.view.View.OnTouchListener;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;

public class RecordingFilePlay extends Activity implements
        Button.OnClickListener {
    private String TAG = "RecordingFilePlay";
    private TextView mTimerView;
    private Typeface mTypeface;
    private String mTimerFormat;
    private TextView mDuration;
    private String mRecordTitle;
    private Long mRecordID;
    private int mRecordDuration;
    private RelativeLayout mRelativeLayout;
    private RecordWavesView mWavesView;
    private RecordDatabaseHelper mDBHelper;
    ImageButton mPlayButton;
    Button mPreviousButton;
    Button mNextButton;
    boolean mIsRecordPlaying;
    private RecordingPlayer mPlayer = null;
    private AudioManager mAudioManager;
    private boolean mPausedByTransientLossOfFocus = false;
    private boolean mPlayFromFocusCanDuck = false;
    private MenuItem mMenuItemClip = null;
    private int spaceTime = 0;
    private Iterator mIterator;
    private int mLocation = 0;
    private int mPosition = 0;
    private int mLastPosition = 0;
    private int mFirstTagPosition = -1;
    private int mFinalTagPosition;
    int startX = 0;
    int moveX = 0;

    private static final int UPDATE_WAVE = 1;
    private static final int FADEDOWN = 2;
    private static final int FADEUP = 3;
    private boolean mNeedRequestPermissions = false;//bug 629393 reset application preferencesonce again into the play is error
    private static final int RECORD_PERMISSIONS_REQUEST_CODE = 200;//bug 629393 reset application preferencesonce again into the play is error
    final Handler mHandler = new Handler(){
        float mCurrentVolume = 1.0f;
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            Log.d(TAG, "handleMessage msg.what = "+msg.what);
            switch (msg.what) {
                case UPDATE_WAVE:
                    updateWavesView();
                    break;
                case FADEDOWN:
                    mCurrentVolume -= .05f;
                    if (mCurrentVolume > .2f) {
                        mHandler.sendEmptyMessageDelayed(FADEDOWN, 10);
                    } else {
                        mCurrentVolume = .2f;
                    }
                    mPlayer.setVolume(mCurrentVolume);
                    break;
                case FADEUP:
                    mCurrentVolume += .01f;
                    if (mCurrentVolume < 1.0f) {
                        mHandler.sendEmptyMessageDelayed(FADEUP, 10);
                    } else {
                        mCurrentVolume = 1.0f;
                    }
                    mPlayer.setVolume(mCurrentVolume);
                    break;
            }
        }
    };

    /* SPRD: bug598309 Record should stop play when switch language. @{ */
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "onReceive  intent.getAction(): "+intent.getAction());
            if(intent.getAction().equals(Intent.ACTION_LOCALE_CHANGED)){
                stopPlayback();
            }
        }
    };
    /* @} */

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().requestFeature(Window.FEATURE_ACTION_BAR);
        setContentView(R.layout.recording_file_play);
        ActionBar actionBar = getActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);

        mDBHelper = new RecordDatabaseHelper(this);
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        initResource();
        // SPRD: bug598309 Record should stop play when switch language.
        IntentFilter iFilter = new IntentFilter();
        iFilter.addAction(Intent.ACTION_LOCALE_CHANGED);
        registerReceiver(mReceiver, iFilter);
    }

    private void initResource() {
        mPlayButton = (ImageButton) findViewById(R.id.playButton);
        mPreviousButton = (Button) findViewById(R.id.previousButton);
        mNextButton = (Button) findViewById(R.id.nextButton);
        mPlayButton.setImageResource(R.drawable.play);
        mPlayButton.setContentDescription(getResources().getString(R.string.start_play));
        mPreviousButton.setEnabled(false);
        mNextButton.setEnabled(false);

        mTimerView = (TextView) findViewById(R.id.timerView);
        mTypeface = Typeface.createFromAsset(getAssets(), "fonts/msyi.ttf");
        mTimerView.setTypeface(mTypeface);

        mTimerFormat = getResources().getString(R.string.timer_format);
        String timeStr = String.format(mTimerFormat, 0, 0, 0);
        mTimerView.setText(timeStr);

        mDuration = (TextView) findViewById(R.id.record_duration);
        mDuration.setText(timeStr);

        mRelativeLayout = (RelativeLayout) findViewById(R.id.wavesLayout);
        mWavesView = new RecordWavesView(this);
        mRelativeLayout.addView(mWavesView);
        mWavesView.mIsRecordPlay = true;

        Intent intent = getIntent();
        mRecordTitle = intent.getStringExtra("title");
        mRecordID = intent.getLongExtra("id", 0);
        mRecordDuration = intent.getIntExtra("duration", 0);
        Log.d(TAG, "mRecordTitle=" + mRecordTitle + ", mRecordID="
                + mRecordID + ", mRecordDuration=" + mRecordDuration);

        mDuration.setText(Utils.makeTimeString4MillSec(RecordingFilePlay.this,
                mRecordDuration));

        mWavesView.mTagHashMap = queryTag(mRecordTitle);

        mWavesView.mWaveDataList = queryWave(mRecordTitle);

        mPlayButton.setOnClickListener(this);
        mPreviousButton.setOnClickListener(this);
        mNextButton.setOnClickListener(this);

        mIterator = mWavesView.mTagHashMap.entrySet().iterator();
        while (mIterator.hasNext()){
            Map.Entry<Integer, Integer> entry = (Map.Entry) mIterator.next();
            if (entry.getValue() == 1) {
                mFirstTagPosition = entry.getKey();
                Log.d(TAG, "mFirstTagPosition = " +mFirstTagPosition);
            }
            if (entry.getValue() == mWavesView.mTagHashMap.size()-1) {
                mFinalTagPosition = entry.getKey();
            }
        }

        mWavesView.setOnTouchListener(new OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (mWavesView.mWaveDataList.size() == 0) return true;
                switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    startX = (int)event.getX();
                    mLocation = mWavesView.mPosition;
                    break;
                case MotionEvent.ACTION_MOVE:
                    moveX = (int)event.getX();
                    mWavesView.mPosition = mLocation-(moveX - startX)/(int)mWavesView.mWavesInterval+1;
                    if (mWavesView.mPosition < 0) {
                        mWavesView.mPosition = 0;
                    } else if (mWavesView.mPosition >= mWavesView.mWaveDataList.size()){
                        mWavesView.mPosition = mWavesView.mWaveDataList.size() - 1;
                    }
                    mWavesView.invalidate();
                    mPosition = (int) (mRecordDuration * (float)(mWavesView.mPosition+1)/mWavesView.mWaveDataList.size());
                    if (mPlayer != null && mPlayer.isPlaying()) {
                        mPlayer.seekTo(mPosition);
                    }
                    updateTimeView(mPosition);
                    break;
                case MotionEvent.ACTION_UP:
                    break;
                }
                return true;
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        super.onCreateOptionsMenu(menu);
        /*getMenuInflater().inflate(R.menu.clip_menu, menu);
        mMenuItemClip = menu.findItem(R.id.item_clip);*/
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
        /*case R.id.item_clip:
            Intent intent = new Intent(this, RecordingFileClip.class);
            intent.putExtra("title", mRecordTitle);
            intent.putExtra("id", mRecordID);
            intent.putExtra("duration", mRecordDuration);
            startActivity(intent);
            break;*/
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
        Log.d(TAG,"onPause");
        super.onPause();
    }

    @Override
    public void onStop() {
        Log.d(TAG,"onStop");
        super.onStop();
    }

    /* SPRD: bug598309 Record should stop play when switch language. @{ */
    @Override
    public void onDestroy(){
        Log.d(TAG,"onDestroy");
        unregisterReceiver(mReceiver);
        super.onDestroy();
    }
    /* @} */

    private int queryDuration(String title) {
        int duration = 0;
        Cursor cur = null;
        try {
            StringBuilder where = new StringBuilder();
            where.append(MediaStore.Audio.Media.TITLE).append("='")
                    .append(title).append("'");

            cur = RecordingFilePlay.this.getContentResolver().query(
                    MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                    new String[] { MediaStore.Audio.Media.DURATION },
                    where.toString(), null, null);

            // read cursor
            int index = -1;
            for (cur.moveToFirst(); !cur.isAfterLast(); cur.moveToNext()) {
                duration = cur.getInt(0);
            }
        } catch (Exception e) {
            Log.v(TAG, "RecordingFilePlay.queryFile failed; E: " + e);
        } finally {
            if (cur != null)
                cur.close();
        }
        return duration;
    }

    private LinkedHashMap queryTag(String title) {
        Cursor cursor = mDBHelper.queryTag(title);
        LinkedHashMap<Integer, Integer> hashMap = new LinkedHashMap<Integer, Integer>();
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
            if (mPlayer == null) {
                //bug 629393 reset application preferencesonce again into the play is error begin
                mNeedRequestPermissions = checkAndBuildPermissions();
                if (mNeedRequestPermissions) {
                    return;
                }
                //bug 629393 end
                mPlayer = new RecordingPlayer();
                mPlayer.setActivity(RecordingFilePlay.this);
                try {
                    Uri uri = ContentUris.withAppendedId(
                            MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                            mRecordID);
                    mPlayer.setDataSourceAndPrepare(uri);
                } catch (Exception ex) {
                    Log.d(TAG, "Failed to open file: " + ex);
                    Toast.makeText(RecordingFilePlay.this,
                            R.string.playback_failed, Toast.LENGTH_SHORT)
                            .show();
                    return;
                }
            } else {
                if (mPlayer.isPlaying()) {
                    mPlayer.pause();
                    mLastPosition = mWavesView.mPosition;
                } else {
                    start();
                }
                updatePlayPause();
            }
            break;
        case R.id.previousButton:
            if (mWavesView.mWaveDataList.size() == 0) return;
            mIterator = mWavesView.mTagHashMap.entrySet().iterator();
            while (mIterator.hasNext()){
                Map.Entry<Integer, Integer> entry = (Map.Entry) mIterator.next();
                if (mWavesView.mPosition >= entry.getKey()) {
                    mLocation = entry.getKey()-2;
                } else {
                    if (mPlayer != null) {
                        mPosition = (int) (mPlayer.mDuration * (float)mLocation/mWavesView.mWaveDataList.size());
                        mPlayer.seekTo(mPosition);
                    }
                    return;
                }
            }
            break;
        case R.id.nextButton:
            if (mWavesView.mWaveDataList.size() == 0) return;
            mIterator = mWavesView.mTagHashMap.entrySet().iterator();
            while (mIterator.hasNext()){
                Map.Entry<Integer, Integer> entry = (Map.Entry) mIterator.next();
                if (mWavesView.mPosition < entry.getKey()) {
                    mLocation = entry.getKey()-2;
                    if (mPlayer != null) {
                        mPosition = (int) (mPlayer.mDuration * (float)mLocation/mWavesView.mWaveDataList.size());
                        mPlayer.seekTo(mPosition);
                    }
                    return;
                }
            }
            break;
        }
    }
    //bug 629393 reset application preferencesonce again into the play is error begin
    private boolean checkAndBuildPermissions() {
        int numPermissionsToRequest = 0;

        boolean requestMicrophonePermission = false;
        boolean requestStoragePermission = false;
        boolean requestPhoneStatePermission = false;
        if (checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
            requestMicrophonePermission = true;
            numPermissionsToRequest++;
        }

        if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestStoragePermission = true;
            numPermissionsToRequest++;
        }

        if (checkSelfPermission(Manifest.permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED) {
            requestPhoneStatePermission = true;
            numPermissionsToRequest++;
        }
        if (!requestMicrophonePermission && !requestStoragePermission
                && !requestPhoneStatePermission) {
            return false;
        }
        String[] permissionsToRequest = new String[numPermissionsToRequest];
        int permissionsRequestIndex = 0;
        Log.d(TAG,"sprdstart requestMicrophonePermission="+requestMicrophonePermission+" "+"requestStoragePermission ="+requestStoragePermission+" "+"requestPhoneStatePermission ="+requestPhoneStatePermission);
        if (requestMicrophonePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.RECORD_AUDIO;
            permissionsRequestIndex++;
        }
        if (requestStoragePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.WRITE_EXTERNAL_STORAGE;
            permissionsRequestIndex++;
        }

        if (requestPhoneStatePermission) {
            permissionsToRequest[permissionsRequestIndex] = Manifest.permission.READ_PHONE_STATE;
        }
        requestPermissions(permissionsToRequest, RECORD_PERMISSIONS_REQUEST_CODE);
        return true;
    }
    //bug 629393 end
    public void start() {
        if (!requestAudioFocus()) {
            Toast.makeText(this.getApplicationContext(),
                    R.string.no_allow_play_calling, Toast.LENGTH_SHORT).show();
            return;
        }
        if (mPlayer != null && mLastPosition != mWavesView.mPosition) {
            mPosition = (int) (mPlayer.mDuration * (float)(mWavesView.mPosition+1)/mWavesView.mWaveDataList.size());
            mPlayer.seekTo(mPosition);
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
            Log.d(TAG, "onAudioFocusChange focusChange = "+focusChange);
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
                if (mPlayer.isPlaying()) {
                    mPausedByTransientLossOfFocus = true;
                    mPlayer.pause();
                }
                break;
            case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                mPlayFromFocusCanDuck = true;
                mHandler.removeMessages(FADEUP);
                mHandler.sendEmptyMessage(FADEDOWN);
                break;
            case AudioManager.AUDIOFOCUS_GAIN:
                if (mPausedByTransientLossOfFocus) {
                    mPausedByTransientLossOfFocus = false;
                    start();
                }
                if (mPlayFromFocusCanDuck) {
                    mPlayFromFocusCanDuck = false;
                    mHandler.removeMessages(FADEDOWN);
                    mHandler.sendEmptyMessage(FADEUP);
                }
                break;
            }
            updatePlayPause();
        }
    };

    public void updatePlayPause() {
        if (mPlayButton != null && mPlayer != null) {
            if (mPlayer.isPlaying()) {
                if (mWavesView.mWaveDataList.size() != 0) {
                    spaceTime = mPlayer.mDuration/(2*mWavesView.mWaveDataList.size()) - 2;
                    Message msg = new Message();
                    msg.what = UPDATE_WAVE;
                    mHandler.sendMessage(msg);
                }

                mPlayButton.setImageResource(R.drawable.suspended);
                mPlayButton.setContentDescription(getResources().getString(R.string.pause));
                updateTimeView(mPlayer.getCurrentPosition());
            } else {
                mHandler.removeCallbacks(mUpdateWavesView);
                mHandler.removeCallbacks(mUpdateTimeView);
                mPlayButton.setImageResource(R.drawable.play);
                if (mPlayer.mIsPrepared) {
                    mPlayButton.setContentDescription(getResources().getString(R.string.resume_play));
                } else {
                    mPlayButton.setContentDescription(getResources().getString(R.string.start_play));
                }

                setPreviousButtonEnabled(false);
                setNextButtonEnabled(false);
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
        updateTimeView(0);
    }

    private final Runnable mUpdateWavesView = new Runnable() {
        public void run() {
            updateWavesView();
        }
    };

    private final Runnable mUpdateTimeView = new Runnable() {
        public void run() {
            if (mPlayer != null && mPlayer.isPlaying()) {
                updateTimeView(mPlayer.getCurrentPosition());
            }
        }
    };

    public void updateWavesView() {
        if (mPlayer != null && mPlayer.isPlaying()) {
            if (mWavesView.mWaveDataList.size() != 0) {
                mWavesView.mPosition = (int) (mWavesView.mWaveDataList.size() * (float)mPlayer.getCurrentPosition()/mPlayer.mDuration);
                mWavesView.invalidate();
                updateButton();
                mHandler.removeCallbacks(mUpdateWavesView);
                mHandler.postDelayed(mUpdateWavesView, spaceTime);
            }
        } else {
            mWavesView.mPosition = 0;
            mWavesView.invalidate();
        }
    }

    public void updateButton() {
        Log.d(TAG, "updateButton mWavesView.mPosition = "+mWavesView.mPosition+", mFirstTagPosition = " +mFirstTagPosition);
        if (mWavesView.mPosition <= mFirstTagPosition || mFirstTagPosition==-1) {
            setPreviousButtonEnabled(false);
        } else {
            setPreviousButtonEnabled(true);
        }

        if (mWavesView.mPosition >= mFinalTagPosition) {
            setNextButtonEnabled(false);
        } else {
            setNextButtonEnabled(true);
        }
    }

    private void setPreviousButtonEnabled(boolean enable) {
        Drawable drawable;
        mPreviousButton.setEnabled(enable);
        if (enable) {
            mPreviousButton.setTextColor(Color.parseColor("#4b4c57"));
            drawable= this.getResources().getDrawable(R.drawable.before_tag_default);
        } else {
            mPreviousButton.setTextColor(Color.parseColor("#d3d3d3"));
            drawable= this.getResources().getDrawable(R.drawable.before_tag_disabled);
        }
        drawable.setBounds(0, 0, drawable.getMinimumWidth(), drawable.getMinimumHeight());
        mPreviousButton.setCompoundDrawables(drawable,null,null,null);
    }

    private void setNextButtonEnabled(boolean enable) {
        Drawable drawable;
        mNextButton.setEnabled(enable);
        if (enable) {
            mNextButton.setTextColor(Color.parseColor("#4b4c57"));
            drawable= this.getResources().getDrawable(R.drawable.next_tag_default);
        } else {
            mNextButton.setTextColor(Color.parseColor("#d3d3d3"));
            drawable= this.getResources().getDrawable(R.drawable.next_tag_disabled);
        }
        drawable.setBounds(0, 0, drawable.getMinimumWidth(), drawable.getMinimumHeight());
        mNextButton.setCompoundDrawables(null,null,drawable,null);
    }

    private void updateTimeView(int time) {
        if (time >= 500) {
            mTimerView.setText(Utils.makeTimeString4MillSec(RecordingFilePlay.this, time));
        } else {
            String timeStr = String.format(mTimerFormat, 0, 0, 0);
            mTimerView.setText(timeStr);
        }
        if (mPlayer != null && mPlayer.isPlaying()) {
            mHandler.postDelayed(mUpdateTimeView, 300);
        }
    }
}
