package com.sprd.camera.voice;

import java.io.File;

import android.media.MediaPlayer;
import android.util.Log;
import android.widget.Toast;

import com.android.camera.app.AppController;
import com.android.camera.data.FilmstripItem;
import com.android.camera2.R;

import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;

public class PhotoVoicePlayer {
    private static final String TAG = "PhotoVoicePlayer";
    private String mLastPhotoVoice;
    private MediaPlayer mMediaPlayer;
    private AppController mAppController;

    /**
     * Get a new global PhotoVoiceRecorder.
     */
    public PhotoVoicePlayer(AppController app) {
        mAppController = app;
    }

    private void initlizeMediaPlayer() {
        mMediaPlayer = new MediaPlayer();
                    mMediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
                @Override
                public void onCompletion(MediaPlayer arg0) {
                    // TODO Auto-generated method stub
                    abandonAudioFocus();
                    mAppController.getCameraAppUI().stopCircle();
                }
            });
            mMediaPlayer.setOnErrorListener(new MediaPlayer.OnErrorListener() {
                @Override
                public boolean onError(MediaPlayer arg0, int arg1, int arg2) {
                    // TODO Auto-generated method stub
                    abandonAudioFocus();
                    mAppController.getCameraAppUI().stopCircle();
                    return false;
                }
            });
    }
    public void updateVoicePlayButtonByData(FilmstripItem currentData){
        mLastPhotoVoice = currentData.getVoice();
        playPhotoVoice(null);
        mAppController.getCameraAppUI().setVoicePlayButtonVisible(currentData.getVoice() != null);
    }

    public void releasePlayer() {
        if (mMediaPlayer != null) {
            Log.i(TAG, "releasePlayer");
            mMediaPlayer.reset();
            mMediaPlayer.release();
            mMediaPlayer = null;
            abandonAudioFocus();
        }
    }

    public void playPhotoVoice() {
        playPhotoVoice(mLastPhotoVoice);
    }

    public void playPhotoVoice(String path) {
        if (null == mMediaPlayer) {
            initlizeMediaPlayer();
        }
        if (path != null && mMediaPlayer != null) {
            if(mMediaPlayer.isPlaying()){
                abandonAudioFocus();
                mMediaPlayer.reset();
                mAppController.getCameraAppUI().stopCircle();
                Log.i(TAG, "playPhotoVoice isPlaying , reset stop play");
                return;
            }
            File voiceFile = new File(path);
            if (!voiceFile.exists()) {
                Log.i(TAG, "playPhotoVoice path = " + path + " does not exist!");
                return;
            }
            try {
                if (requestAudioFocus() == AudioManager.AUDIOFOCUS_REQUEST_FAILED) {
                    Toast.makeText(
                            mAppController.getAndroidContext(),
                            mAppController.getAndroidContext().getResources()
                                    .getString(R.string.camera_play_voice_failed),
                            Toast.LENGTH_LONG).show();
                    return;
                }
                mMediaPlayer.reset();
                mMediaPlayer.setDataSource(path);
                mMediaPlayer.prepare();
                mAppController.getCameraAppUI().setCircleDuration(mMediaPlayer.getDuration());
                mAppController.getCameraAppUI().startCircle();
                mMediaPlayer.start();
            } catch (Exception e) {
                Log.i(TAG, "playPhotoVoice Exception e = " + e.toString());
            }
            Log.i(TAG, "playPhotoVoice play path = " + path);
        } else {
            if (mMediaPlayer != null) {
                abandonAudioFocus();
                mMediaPlayer.reset();
                mAppController.getCameraAppUI().stopCircle();
            }
        }
    }

    OnAudioFocusChangeListener afChangeListener = new OnAudioFocusChangeListener() {

        @Override
        public void onAudioFocusChange(int focusChange) {
            // TODO Auto-generated method stub
            switch (focusChange) {
            case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
//                break;
            case AudioManager.AUDIOFOCUS_LOSS:
                if (mMediaPlayer != null) {
                    mMediaPlayer.reset();
                    mAppController.getCameraAppUI().stopCircle();
                }
                break;
            }
        }
    };

    private int requestAudioFocus() {
        return ((AudioManager) mAppController.getAndroidContext()
                .getSystemService(
                        mAppController.getAndroidContext().AUDIO_SERVICE))
                .requestAudioFocus(afChangeListener, AudioManager.STREAM_MUSIC,
                        AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
    }

    private void abandonAudioFocus() {
        ((AudioManager) mAppController.getAndroidContext().getSystemService(
                mAppController.getAndroidContext().AUDIO_SERVICE))
                .abandonAudioFocus(afChangeListener);
    }
}
