package com.sprd.soundrecorder;

import java.io.IOException;

import com.sprd.soundrecorder.RecordingFileList;
import com.android.soundrecorder.R;
import android.app.Activity;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer;
import android.net.Uri;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import com.sprd.soundrecorder.MarkSeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.Toast;

public class PreviewPlayer extends MediaPlayer implements OnPreparedListener,
        OnErrorListener, OnCompletionListener {

    public static String TAG = "PreviewPlayer";
    private RecordingFileList mActivity;
    public boolean mIsPrepared = false;
    public MarkSeekBar mSeekBar;
    public int mDuration;
    public boolean mSeeking = false;
    @Override
    public void onPrepared(MediaPlayer mp) {
        mIsPrepared = true;
        mSeekBar.setOnSeekBarChangeListener(mSeekListener);
        mActivity.start();
        mDuration = getDuration();
        addMarkInfo();//bug 613017 Play the file icon will appear on another file or will not appear play
        mActivity.updatePlayPause();
    }
    //bug 613017 Play the file icon will appear on another file or will not appear play
    private void addMarkInfo(){
        if (mDuration != 0) {
            mSeekBar.setMax(mDuration);
            mSeekBar.setDuration(mDuration);
            int tagSize = mActivity.mTagHashMap.size();
            for (int i = 1; i < tagSize; i++) {
                float mark = (float)mActivity.mTagHashMap.get(i)/mActivity.mTagHashMap.get(tagSize);
                mSeekBar.addMark((int)(mDuration*mark));
            }
            mSeekBar.setVisibility(View.VISIBLE);
            if (!mSeeking) {
                mSeekBar.setProgress(getCurrentPosition());
            }
        }
    }

    public void setScrolSeekBar(SeekBar seekBar){
        mSeekBar = (MarkSeekBar)seekBar;
        mSeekBar.setOnSeekBarChangeListener(mSeekListener);
        mDuration = getDuration();
        addMarkInfo();
    }
    //bug 613017 end
    @Override
    public void onCompletion(MediaPlayer mp) {
        mIsPrepared = false;
        /** SPRD:Bug 618322 file play fail the sound recorder crash( @{ */
        if (null != mSeekBar){
            mSeekBar.setProgress(mDuration);
            mSeekBar.clearAllMarks();
        }
        /** @} */
        mActivity.onCompletion();
    }

    @Override
    public boolean onError(MediaPlayer mp, int what, int extra) {
        Toast.makeText(mActivity, R.string.playback_failed, Toast.LENGTH_SHORT).show();
        return false;
    }

    public void setUISeekBar(SeekBar seekBar){
        mSeekBar = (MarkSeekBar)seekBar;
    }

    public void setDuration(int duration){
        mSeekBar.setDuration(duration);
    }

    public void setActivity(Activity activity) {
        mActivity = (RecordingFileList)activity;
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

    private OnSeekBarChangeListener mSeekListener = new OnSeekBarChangeListener() {
        public void onStartTrackingTouch(SeekBar bar) {
            mSeeking = true;
        }
        public void onProgressChanged(SeekBar bar, int progress, boolean fromuser) {
            if (!fromuser || !mIsPrepared) {
                return;
            }
            mActivity.updatePlayedDuration(progress);
            seekTo(progress);
        }
        public void onStopTrackingTouch(SeekBar bar) {
            mSeeking = false;
        }
    };

}
