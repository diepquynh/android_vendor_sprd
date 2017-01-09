/**
 *  Created By Spreadst
 */

package com.android.fmradio;

import java.io.IOException;

import android.app.Activity;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.net.Uri;
import android.util.Log;
import android.view.View;
import android.widget.SeekBar;
import android.widget.Toast;
import android.widget.SeekBar.OnSeekBarChangeListener;

public class FmMediaPalyer extends MediaPlayer implements OnPreparedListener, OnErrorListener,
        OnCompletionListener {

    public static String TAG = "PreviewPlayer";
    private FmRecordListActivity mActivity;
    public boolean mIsPrepared = false;
    public SeekBar mSeekBar;
    public int mDuration;
    public boolean mSeeking = false;
    private OnSeekBarChangeListener mSeekListener = new OnSeekBarChangeListener() {
        public void onStartTrackingTouch(SeekBar bar) {
            mSeeking = true;
        }

        public void onProgressChanged(SeekBar bar, int progress, boolean fromuser) {
            if (!fromuser || !mIsPrepared) {
                return;
            }
            // SPRD : Modify for 651838,Drag the seekbar to the end,the thumb will kick back.
            seekTo(progress);
        }

        public void onStopTrackingTouch(SeekBar bar) {
            mSeeking = false;
        }
    };

    public void setActivity(Activity activity) {
        mActivity = (FmRecordListActivity) activity;
        setOnErrorListener(this);
        setOnPreparedListener(this);
        setOnCompletionListener(this);

    }

    @Override
    public void onPrepared(MediaPlayer mp) {
        mIsPrepared = true;
        mSeekBar.setOnSeekBarChangeListener(mSeekListener);
        mDuration = getDuration();
        mActivity.start();
        if (mDuration != 0) {
            mSeekBar.setMax(mDuration);
            mSeekBar.setVisibility(View.VISIBLE);
            if (!mSeeking) {
                mSeekBar.setProgress(getCurrentPosition());
            }
        }
        mActivity.updatePlayPause();
    }

    @Override
    public void onCompletion(MediaPlayer mp) {
        // SPRD : Modify for 645885,Drag the seekbar from side to side,FM crash sometimes.
        mIsPrepared = false;
        if (null != mSeekBar){
            mSeekBar.setProgress(mDuration);
        }
        mActivity.onCompletion();
    }

    @Override
    public boolean onError(MediaPlayer mp, int what, int extra) {
        Toast.makeText(mActivity, R.string.play_error, Toast.LENGTH_SHORT).show();
        return false;
    }

    public void setDataSourceAndPrepare(Uri uri)
            throws IllegalArgumentException, SecurityException,
            IllegalStateException, IOException {
        setDataSource(mActivity, uri);
        prepareAsync();
    }

    public void setUISeekBar(SeekBar seekBar) {
        mSeekBar = seekBar;
    }
    /**
     * SPRD: Add for Bug638498
     * @{
     */
    public void setProgressListener(){
        mSeekBar.setOnSeekBarChangeListener(mSeekListener);
    }
    /* SPRD: Add for Bug638498  @} */

    //Modify for 662375,Click recording file alternately and quickly,play error will occur high frequently.
    public boolean isPrepared() {
        return mIsPrepared;
    }
}
