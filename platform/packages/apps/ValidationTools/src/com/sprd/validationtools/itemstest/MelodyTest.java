
package com.sprd.validationtools.itemstest;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.media.AudioSystem;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.Handler;
import android.os.Message;
import android.os.Vibrator;
import android.view.Gravity;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;

import com.sprd.validationtools.BaseActivity;
import com.sprd.validationtools.R;
import android.media.AudioManager;
import com.sprd.validationtools.Const;

public class MelodyTest extends BaseActivity {

    TextView mContent;
    private int backupMode = 0;
    private List<String> mFilePaths;
    private File mFile;
    MediaPlayer mPlayer = null;
    private Vibrator mVibrator = null;
    private static final long V_TIME = 100000;
    private static final long DIALOG_TIME = 3000;
    private static final String DEFAULT_AUDIO = "Orion.ogg";
    private boolean isSearchFinished = false;

    private boolean isWhaleSupport = Const.isWhale2Support();
    private AudioManager mAudioManager = null;

    private Runnable mR = new Runnable() {
        public void run() {
            if (mPlayer != null) {
                //showResultDialog(getString(R.string.melody_play_info));
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        mAudioManager = (AudioManager) this
                .getSystemService(Context.AUDIO_SERVICE);
        mContent = new TextView(this);
        mContent.setGravity(Gravity.CENTER);
        mContent.setTextSize(25);
        setContentView(mContent);
        setTitle(R.string.melody_test);
        mFilePaths = new ArrayList<String>();
        mPlayer = new MediaPlayer();
        mVibrator = (Vibrator) getSystemService(this.VIBRATOR_SERVICE);

        mContent.setText(R.string.start_searching);
        new Thread() {
            public void run() {
                if (checkSDCard()) {
                    mFile = EnvironmentEx.getExternalStoragePath();
                    toSearchFiles(mFile);

                    if (mFilePaths.size() != 0) {
                        mHandler.sendEmptyMessage(SEARCH_FINISHED);
                        return;
                    }
                }

                File firstAudio = new File("/system/media/audio/ringtones", DEFAULT_AUDIO);
                if (firstAudio.exists()) {
                    mFilePaths.add(firstAudio.getPath());
                } else {
                    mFile = new File("/system/media/audio/ringtones");
                    toSearchFiles(mFile);
                }

                mHandler.sendEmptyMessage(SEARCH_FINISHED);
            }
        }.start();
    }

    private final int SEARCH_FINISHED = 0;
    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case SEARCH_FINISHED:
                    isSearchFinished = true;
                    doPlay();
                    mHandler.postDelayed(mR, DIALOG_TIME);
                    break;
            }
        }
    };

    private void doPlay() {
        int audioNumber = getRandom(mFilePaths.size());
        if (mPlayer == null) {
            return;
        }
        try {
            mPlayer.setDataSource(mFilePaths.get(audioNumber));
            mPlayer.prepare();
        } catch (IllegalArgumentException e) {
            /* SPRD: fix bug350197 setDataSource fail due to crash @{ */
            // mPlayer = null;
            /* @} */
            e.printStackTrace();
        } catch (IllegalStateException e) {
            // mPlayer = null;
            e.printStackTrace();
        } catch (IOException e) {
            // mPlayer = null;
            e.printStackTrace();
        }
        mPlayer.start();
        /*BEGIN BUG559287 2016/05/04 zhijie.yang mmi/ringtones test error */
        if (AudioSystem.DEVICE_STATE_AVAILABLE == AudioSystem.getDeviceConnectionState(
                AudioManager.DEVICE_OUT_EARPIECE, "")) {
            mAudioManager.setMode(AudioManager.MODE_IN_CALL);
        }
        /*END BUG559287 2016/05/04 zhijie.yang mmi/ringtones test error */
        mPlayer.setVolume(mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC));

        mVibrator.vibrate(V_TIME);
        mContent.setText(getResources().getText(R.string.melody_play_tag)
                + mFilePaths.get(audioNumber));
    }

    @Override
    protected void onResume() {
        super.onResume();
        AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
        backupMode = audioManager.getMode();
        if (mPlayer == null) {
            mPlayer = new MediaPlayer();
        }
        if (isSearchFinished) {
            doPlay();
            mHandler.postDelayed(mR, DIALOG_TIME);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mPlayer == null) {
            return;
        }
        mPlayer.stop();
        mPlayer.release();
        mPlayer = null;
        if (mVibrator != null) {
            mVibrator.cancel();
        }
        AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
        audioManager.setMode(backupMode);
    }

    private boolean checkSDCard() {
        boolean hasSDCard = false;
        hasSDCard = Environment.getExternalStorageState().equals(
                Environment.MEDIA_MOUNTED);
        return hasSDCard;
    }

    public void toSearchFiles(File file) {
        File[] files = file.listFiles();
        if (files == null) {
            return;
        }
        for (File tf : files) {
            if (tf.isDirectory()) {
                toSearchFiles(tf);
            } else {
                try {
                    /*  whale2 not support .mp3
                    if (tf.getName().indexOf(".mp3") > -1) {
                        mFilePaths.add(tf.getPath());
                    }*/                    
                    if (tf.getName().indexOf(".ogg") > -1) {
                        mFilePaths.add(tf.getPath());
                    }
                } catch (Exception e) {
                    Toast.makeText(this, "pathError", Toast.LENGTH_SHORT).show();
                }
            }
        }
    }

    private int getRandom(int max) {
        double random = Math.random();
        int result = (int) Math.floor(random * max);
        return result;
    }

    @Override
    public void onDestroy() {
        mHandler.removeCallbacks(mR);
        super.onDestroy();
    }
}
