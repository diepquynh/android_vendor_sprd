/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.gallery3d.app;

import android.annotation.TargetApi;
import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.KeyguardManager;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.database.Cursor;
import android.media.AudioManager;
import android.media.MediaMetadataRetriever;
import android.media.MediaScannerConnection;
import android.media.MediaPlayer;
import android.media.audiofx.AudioEffect;
import android.media.audiofx.Virtualizer;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.EnvironmentEx;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore;
import android.provider.Settings.SettingNotFoundException;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.OrientationEventListener;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Toast;

import com.android.gallery3d.R;
import com.android.gallery3d.common.ApiHelper;
import com.android.gallery3d.common.BlobCache;
import com.android.gallery3d.util.CacheManager;
import com.android.gallery3d.util.GalleryUtils;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.LinkedList;

import com.sprd.gallery3d.app.VideoUtil;
import com.sprd.gallery3d.cmcc.VideoCmccUtils;
import com.sprd.gallery3d.drm.VideoDrmUtils;

import com.android.gallery3d.app.CommonControllerOverlay.State;
import com.android.gallery3d.app.MovieActivity.MovieInfo;
import com.sprd.gallery3d.app.MoviePlayerVideoView;
import com.sprd.gallery3d.app.NewControllerOverlay;

import android.provider.Settings;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.hardware.Sensor;
import android.hardware.SprdSensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;


public class MoviePlayer  implements
        MediaPlayer.OnErrorListener, MediaPlayer.OnCompletionListener,
        MediaPlayer.OnPreparedListener, MediaPlayer.OnSeekCompleteListener,
        MediaPlayer.OnInfoListener, MediaPlayer.OnVideoSizeChangedListener,
        ControllerOverlay.Listener,NewControllerOverlay.NewListener {

    @SuppressWarnings("unused")
    private static final String TAG = "MoviePlayer";

    private static final String KEY_VIDEO_POSITION = "video-position";
    private static final String KEY_RESUMEABLE_TIME = "resumeable-timeout";
    private static final String KEY_PLAY_PAUST_STATE = "playpause_state";
    private static final String KEY_DRM_VIDEO_CONSUMED = "drm_consumed";

    public static final String AUDIO_SERVICE = "audio";
    // These are constants in KeyEvent, appearing on API level 11.
    private static final int KEYCODE_MEDIA_PLAY = 126;
    private static final int KEYCODE_MEDIA_PAUSE = 127;
    private static final String HISTORY_VIDEOS_FRAGMENT = "HistoryVideosFragment";

    /**
     * SPRD:Bug502125 FM can't stop normally, when play the 3gpp format recording by the video
     * player
     * 
     * @orig modify by old bug379538
     * @{ // Copied from MediaPlaybackService in the Music Player app. private static final String
     *    SERVICECMD = "com.android.music.musicservicecommand"; private static final String CMDNAME
     *    = "command"; private static final String CMDPAUSE = "pause"; @}
     */

    private static final String VIRTUALIZE_EXTRA = "virtualize";
    private static final long BLACK_TIMEOUT = 500;

    // If we resume the acitivty with in RESUMEABLE_TIMEOUT, we will keep playing.
    // Otherwise, we pause the player.
    private static final long RESUMEABLE_TIMEOUT = 3 * 60 * 1000; // 3 mins

    private Context mContext;
    private final View mRootView;
    private final Bookmarker mBookmarker;
    /**
     * SPRD:Bug474600 improve video control functions remove @{
     * @orig private final Uri mUri; private final VideoView mVideoView; @} / /**SPRD:Bug474600
     *       improve video control functions remove @{
     * @orig private final Handler mHandler = new Handler(); @}
     */
    private final AudioBecomingNoisyReceiver mAudioBecomingNoisyReceiver;
    private final MovieControllerOverlay mController;

    private long mResumeableTime = Long.MAX_VALUE;
    private int mVideoPosition = 0;

    //Volume protection value
    private static final int VOLUME_SAFE_VALUE = 10;
    //Maximum percent protection
    private static final float VOLUME_SAFE_PERCENTAGE = 0.68f;

    private boolean mHasPaused = false;
    private int mLastSystemUiVis = 0;
    private String mFilePath;
    public boolean mComplete = false;
    private static final int CMD_STOP_BEGIN = 1;
    private static final int CMD_STOP_END = 0;
    public int curVideoIndex = 0;
    private String scheme;
    private Uri mBookMarkPath;
    public LinkedList<MovieInfo> mPlayList;
    private Bundle msavedInstance = null;
    private MovieActivity mMovieActivity = null;
    private int mVideoDuration = 0;
    private boolean isControlPause = false;
    private Uri mUri;
    /**
     * SPRD:Bug474600 improve video control functions remove @{
     * 
     * @orig private VideoView mVideoView=null; @}
     */
    private MoviePlayerVideoView mVideoView = null;
    private boolean mIsChanged = false;
    private boolean isStop = false;
    private boolean mIsError;
    public boolean misLiveStreamUri = false;
    private String mAlbum;
    private final ActionBar mActionBar;
    private long mClickTime = 0;
    private boolean mIsCanSeek = true;
    private int mBeforeSeekPosition = 0;
    /**  @}*/
    public boolean mIsBookmark = true;
    /**
     * SPRD:Bug474600 improve video control functions user the Custom Handler class to replace the
     * original Handler class @{
     */
    private final MyHandler mHandler = new MyHandler();
    /** @} */
    /** SPRD:Bug 474639 add phone call reaction @{ */
    public int mMaxVolume = 0;
    private AudioManager mAudioManager = null;
    private boolean isBookmark = false;
    private boolean mIsPhonePause;
    private long currentTime;
    private AlertDialog mTimeOutDialog;
    /** @} */
    /** SPRD:Bug 511468 add new sensor feature for MoviePlayer @{ */
    private SensorManager mSensorManager = null;
    private Sensor mSensor=null;
    private float mZaxis;
    private KeyguardManager mKeyguardManager;
    private boolean mIsSettingOpen=false;
    private boolean mFirstVist = true;
    private boolean mPreState;
    private boolean mStateIsChange = false;
    private boolean mIfPlayExcute = false;
    private boolean mIfPauseExcute = false;
    private boolean mHasChanged = false;
    /** @ } */
    // If the time bar is being dragged.
    private boolean mDragging;

    // If the time bar is visible.
    private boolean mShowing;

    private Virtualizer mVirtualizer;
    /** SPRD:bug 474614: porting float play @{ */
    private Intent mServiceIntent; // old bug info:339523
    /** @} */
    /**
     * SPRD:Bug474615 Playback loop mode add new parameters @{
     */
    private int currentPlayBackMode = 0;
    public final int SINGLE_PLAYBACK = 1;
    public final int ALL_PLAYBACK = 2;
    /** @} */
    /** SPRD:Bug474609 video playback resume after interruption @{ */
    private AlertDialog mResumeDialog;
    private boolean isResumeDialogShow = false;
    /** @} */
    /** SPRD:Bug474635 exit the video player interface*/
    private String mId;
    /** @} */
    /** SPRD:Bug474618 Channel volume settings
     * add new parameters @{ */
    private float mLeftVolume = 1, mRightVolume = 1;
    /**
     * SPRD:Bug 474631 add headset Control old bug info: add for bug 327112 when long press
     * KeyEvent.KEYCODE_HEADSETHOOK or KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE the MoviePlayer do nothing @{
     */
    private long mKeyDownEventTime = 0;
    /** @} */
    /*SPRD:Add VideoPlayer new UI design 568552 @{ */
    private boolean isLock = false;
    private boolean isOrientate = false;
    private int orientation;
    private OrientationEventListener orintationListener;
    private static final int SCREEN_PORTRAIT = 1;
    private static final int SCREEN_LANDSCAPE = 0;
    private int mVolume = -1;
    private float mCurrentLightness = -1.0f;
    private int mCurrentPosition = -1;
    private float mLightness = -1.0f;
    private static final int MSG_CAPTURE_SUCCESS = 1;
    private static final int MSG_CAPTURE_FAILED = 0;
    private static final float MAX_LIGHTNESS = 255.0f;
    private boolean mIsAudioFocusOwner = false;
    private boolean mFirstError = false;
    private int mErrorPosition = -1;
    private int mErrorTimes = 0;
    private int mPreErroPosition = -1;
    /*@}*/
    /* SPRD:Bug 605744  Capture screen quickly and more than once , then landscape screen,the video  surface is wrong @{*/
    private Handler mCaptureHandler = new Handler() {
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch(msg.what){
                case MSG_CAPTURE_SUCCESS:
                    String targetPath = (String)msg.obj;
                    Toast.makeText(mMovieActivity, String.format(mContext.getResources().getString(R.string.save_picture),targetPath),
                            Toast.LENGTH_SHORT).show();
                    break;
                case MSG_CAPTURE_FAILED:
                    Toast.makeText(mMovieActivity,R.string.screenshot_failed,Toast.LENGTH_SHORT).show();
                    break;
                default:
                    break;
            }
        }
    };
    /*Bug605744 end@}*/
    private final Runnable mPlayingChecker = new Runnable() {
        @Override
        public void run() {
            if (mVideoView != null && mVideoView.isPlaying()) {// SRPD:Bug272737 add
                                                               // mVideoView!=null to check null
                                                               // pointer
                /**
                 * SPRD:Bug474600 improve video control functions add to check the video is playing@{
                 */
                Log.d(TAG, "mPlayingChecker isPlaying");
                if (misLiveStreamUri) {
                    mController.setLiveMode();
                }
                /** @} */
                showControllerPlaying();
                /**
                 * SPRD:Bug474600 improve video control functions add to check whether the video
                 * player is paused or not @{
                 */
            } else if (mHasPaused) {
                if (misLiveStreamUri) {
                    mController.setLiveMode();
                }
                showControllerPaused();
                mController.showPaused();
                /** @} */
            } else {
                /**
                 * SPRD:Bug474600 improve video control functions add to show loading@{
                 */
                Log.d(TAG, "mPlayingChecker isLoading");
                mController.showLoading();
                /** @} */
                mHandler.postDelayed(mPlayingChecker, 250);
            }
        }
    };

    private final Runnable mProgressChecker = new Runnable() {
        @Override
        public void run() {
            int pos = setProgress();
            mHandler.postDelayed(mProgressChecker, 1000 - (pos % 1000));
        }
    };

    /**
     * SPRD:Bug474600 improve video control functions add handler @{
     */
    private class MyHandler extends Handler {

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case CMD_STOP_BEGIN:
                    mController.showLoading();
                    break;

                case CMD_STOP_END:
                    mController.showPaused();
                    break;

                default:
                    break;
            }
            super.handleMessage(msg);
        }
    }

    /** @} */
    /**
     * SPRD:Bug474600 improve video control functions add new parameter "playList"
     */
    public MoviePlayer(View rootView, final MovieActivity movieActivity,
            Uri videoUri, LinkedList<MovieInfo> playList, Bundle savedInstance, boolean canReplay) {

        /**
         * SPRD:Bug474600 improve video control functions add new value @{
         */
        mMovieActivity = movieActivity;
        mActionBar = movieActivity.getActionBar();
        /** @} */
        mContext = movieActivity.getApplicationContext();
        mRootView = rootView;
        /**
         * SPRD:Bug474600 improve video control functions remove this value, because mVideoView is
         * initialized in initVideoView() method @{
         * 
         * @orig mVideoView = (VideoView) rootView.findViewById(R.id.surface_view); @}
         */
        mBookmarker = new Bookmarker(movieActivity);
        mUri = videoUri;
        /** SPRD:bug 474614: porting float play @{ */
        // old bug info:339523 begin
        int position = mMovieActivity.getIntent().getIntExtra("position", 0);
        if (position >= 0) {
            mVideoPosition = position;
        }
        // bug 339523 end
        /** @} */
        /** SPRD:Bug474600 improve video control functions
         * add new values @{ */
      /*
       * SPRD:Bug540206 remove @{
        scheme = mUri.getScheme();
        if ("content".equalsIgnoreCase(scheme)) {
            if (playList != null) {
                String UriID = mUri.toString().substring(
                        mUri.toString().lastIndexOf("/") + 1,
                        mUri.toString().length());
                for (int i = 0; i < playList.size(); i++) {
                    if (UriID.equalsIgnoreCase(playList.get(i).mID)) {
                        mBookMarkPath = Uri.parse(playList.get(i).mPath);
                        Log.d(TAG, "mBookMarkPath " + mBookMarkPath);
                        curVideoIndex = i;
                        break;
                    }
                }
            }
        }
        *@}
        */
        mPlayList = playList;
        changeBookMarkerPathUri(mUri);//SPRD:Bug540206 Add
        Log.d(TAG,"curVideoIndex is "+curVideoIndex);
        /** @} */
        /** SPRD:Bug 474639 add phone call reaction @{ */
        mAudioManager = (AudioManager) movieActivity
                .getSystemService(Context.AUDIO_SERVICE);
        mMaxVolume = mAudioManager
                .getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        msavedInstance = savedInstance;
        /** @} */
        mController = new MovieControllerOverlay(mContext);
        ((ViewGroup) rootView).addView(mController.getView());
        mController.setListener(this);
        mController.setNewListener(this);
        mController.setCanReplay(canReplay);
        /**
         * SPRD:Bug474600 improve video control functions add new values @{
         */
        if (mPlayList == null) {
            Log.d(TAG, "mPlayList is null");
        }
        if (mPlayList == null || mPlayList.size() <= 1) {
            mController.showNextPrevBtn(false);
        } else {
            mController.showNextPrevBtn(true);
        }
        /** @} */
        if (mVirtualizer != null) {
            mVirtualizer.release();
            mVirtualizer = null;
        }
        mAudioBecomingNoisyReceiver = new AudioBecomingNoisyReceiver();
        mAudioBecomingNoisyReceiver.register();
        /** SPRD:Bug 511468 add new sensor feature for MoviePlayer @{ */
        mKeyguardManager = (KeyguardManager) mContext.getSystemService(Service.KEYGUARD_SERVICE);
        mSensorManager = (SensorManager) mContext.getSystemService(Context.SENSOR_SERVICE);
        mSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        mSensor = mSensorManager.getDefaultSensor(SprdSensor.TYPE_SPRDHUB_FACE_UP_DOWN);
        /** @ } */
    }

    /**
     * SPRD:Bug474600 improve video control functions add new method @{
     */
    private void initVideoView() {
        mVideoView = (MoviePlayerVideoView) mRootView.findViewById(R.id.surface_view);
        mVideoView.setOnErrorListener(this);
        mVideoView.setOnCompletionListener(this);
        mVideoView.setOnPreparedListener(this);
        mVideoView.setOnInfoListener(this);
        mVideoView.setOnSeekCompleteListener(this);
        /** SPRD:474621 screen size setting @{ */
        mVideoView.setOnVideoSizeChangedListener(this);
        /**@}*/
        mVideoView.resize(mMovieActivity.getIsFullScreen());
        mVideoView.setVideoURI(mUri);
        if (!misLiveStreamUri) {
            mVideoView.setLastInterruptPosition(Integer.valueOf(mVideoPosition).longValue());
        }
        // When the user touches the screen or uses some hard key, the framework
        // will change system ui visibility from invisible to visible. We show
        // the media control at this point.
        mVideoView.postDelayed(new Runnable() {
            @Override
            public void run() {
                // SPRD: Fix bug 625943 Phone status does not go this way
                if (mVideoView != null && mMovieActivity != null && !mMovieActivity.isCallingState()) {
                    mVideoView.setVisibility(View.VISIBLE);
                }
            }
        }, BLACK_TIMEOUT);
        Intent ai = mMovieActivity.getIntent();
        boolean virtualize = ai.getBooleanExtra(VIRTUALIZE_EXTRA, false);
        if (virtualize) {
            int session = mVideoView.getAudioSessionId();
            if (session != 0) {
                Virtualizer virt = new Virtualizer(0, session);
                AudioEffect.Descriptor descriptor = virt.getDescriptor();
                String uuid = descriptor.uuid.toString();
                if (uuid.equals("36103c52-8514-11e2-9e96-0800200c9a66")
                        || uuid.equals("36103c50-8514-11e2-9e96-0800200c9a66")) {
                    mVirtualizer = virt;
                    mVirtualizer.setEnabled(true);
                } else {
                    // This is not the audio virtualizer we're looking for
                    virt.release();
                }
            } else {
                Log.w(TAG, "no session");
            }
        }
        mVideoView.setOnTouchListener(new View.OnTouchListener() {
            public boolean onTouch(View v, MotionEvent event) {
                /** SPRD:Add VideoPlayer operate function @{ */
                //mController.show();
                mController.onTouchEvent(event);
                /** @} */
                return true;
            }
        });
    }

    /** @}*/

    /*
     * SPRD:delete for new feature 568552
     * when click the screen the first time,it intercept the touch event @{
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    private void setOnSystemUiVisibilityChangeListener() {
        if (!ApiHelper.HAS_VIEW_SYSTEM_UI_FLAG_HIDE_NAVIGATION)
            return;

         //When the user touches the screen or uses some hard key, the framework
         //will change system ui visibility from invisible to visible. We show
         //the media control and enable system UI (e.g. ActionBar) to be visible at this point
        mVideoView.setOnSystemUiVisibilityChangeListener(
                new View.OnSystemUiVisibilityChangeListener() {
                    @Override
                    public void onSystemUiVisibilityChange(int visibility) {
                        int diff = mLastSystemUiVis ^ visibility;
                        mLastSystemUiVis = visibility;
                        if ((diff & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) != 0
                                && (visibility & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0) {
                            mController.show();
                        }
                    }
                });
    }
     */
    /*@}*/

    @SuppressWarnings("deprecation")
    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    private void showSystemUi(boolean visible) {
        if (!ApiHelper.HAS_VIEW_SYSTEM_UI_FLAG_LAYOUT_STABLE)
            return;

        int flag = View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
        if (!visible) {
            // We used the deprecated "STATUS_BAR_HIDDEN" for unbundling
            flag |= View.STATUS_BAR_HIDDEN | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_SURFACE;
        }
        if (mVideoView != null) {
            mVideoView.setSystemUiVisibility(flag);
        }
    }

    public void onSaveInstanceState(Bundle outState) {
        outState.putInt(KEY_VIDEO_POSITION, mVideoPosition);
        outState.putLong(KEY_RESUMEABLE_TIME, mResumeableTime);
        /** SPRD:bug 506989 Add Drm feature @{ */
        if (VideoDrmUtils.getInstance().isDrmFile()) {
            outState.putBoolean(KEY_DRM_VIDEO_CONSUMED, VideoDrmUtils.getInstance().isConsumed());
        }
        /** @} */
        outState.putBoolean(KEY_PLAY_PAUST_STATE, isControlPause);
    }

    public void onRestoreInstanceState(Bundle outState) {
        mVideoPosition = outState.getInt(KEY_VIDEO_POSITION, 0);
        mResumeableTime = outState.getLong(KEY_RESUMEABLE_TIME, Long.MAX_VALUE);
        isControlPause = outState.getBoolean(KEY_PLAY_PAUST_STATE, false);
        Log.i(TAG, "onRestoreInstanceState:" + mVideoPosition);
    }

    private void showResumeDialog(Context context, final int bookmark) {
        /** SPRD:Bug474609 video playback resume after interruption @{ */
        if (isResumeDialogShow)
            return; // SPRD : add

        if (mResumeDialog == null) {
            mResumeDialog = new AlertDialog.Builder(context)
                    .setTitle(R.string.resume_playing_title)
                    .setMessage(
                            String.format(
                                    context.getString(R.string.resume_playing_message),
                                    GalleryUtils.formatDuration(context,
                                            bookmark / 1000)))
                    .setOnCancelListener(new OnCancelListener() {
                        @Override
                        public void onCancel(DialogInterface dialog) {
                            isResumeDialogShow = false; // SPRD : add by old bug379259
                            onCompletion();
                        }
                    })
                    .setPositiveButton(R.string.resume_playing_resume,
                            new OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    mVideoView.seekTo(bookmark);
                                    mVideoView.setVisibility(View.VISIBLE);
                                    /* SPRD:Add for bug609816 Wrong logic of the audiofocus control @{ */
                                    if (!requestAudioFocus()) {
                                        return;
                                    }
                                    /* Bug609816 end @} */
                                    startVideo();
                                    isControlPause = false;
                                    isResumeDialogShow = false;
                                    isBookmark = false;

                                }
                            })
                    .setNegativeButton(R.string.resume_playing_restart,
                            new OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog,
                                        int which) {
                                    mVideoView.seekTo(0); // SPRD: added for bug
                                                          // 278584
                                    mVideoView.setVisibility(View.VISIBLE);
                                    /* SPRD:Add for bug609816 Wrong logic of the audiofocus control @{ */
                                    if (!requestAudioFocus()) {
                                        return;
                                    }
                                    /* Bug609816 end @} */
                                    startVideo();
                                    mVideoPosition = 0;
                                    isControlPause = false;
                                    isResumeDialogShow = false;
                                    isBookmark = false;
                                }
                            }).create();
        }
        mResumeDialog.show();
        mResumeDialog.setCanceledOnTouchOutside(false);
        isResumeDialogShow = true; // SPRD : add
        /** @} */
    }

    /** SPRD:bug 474614: porting float play @{ */
    // old bug info:339523 begin
    public void setControlPause(boolean pause) {
        isControlPause = pause;
    }
    // bug 339523 end
    /** @ } */

    public void onPause() {
        // mHasPaused = true;
        //SPRD:Add for new feature 568552 :when back out the video player,the screen lightness is adjusted to previous lightness
        mHandler.removeCallbacksAndMessages(null);
        mDragging = false;
        mController.setscrubbing();
        /** SPRD:Bug 474639 add phone call reaction @{ */
        if (mMovieActivity.isCallingState()) {
            mIsPhonePause = true;
            currentTime = System.currentTimeMillis();
            showControllerPaused();
        }
        if (!misLiveStreamUri && mVideoView != null && mVideoView.isPlay() && !isBookmark) {
            mVideoPosition = mVideoView.getCurrentPosition();
        }
        /** @ } */
        /**
         * SPRD:Bug500699 After waiting for the play after the manual drive,click Start,Continue to
         * play video tips modify by old bug429923 @{
         */
        if (mComplete) {
            mVideoPosition = 0;
        }
        /** @ } */
        /**
         * SPRD:Bug474600 improve video control functions remove @{
         *
         * @orig mVideoPosition = mVideoView.getCurrentPosition(); @}
         */
        mIsCanSeek = mVideoView.canSeekBackward() && mVideoView.canSeekForward();
        if (!isStreamUri() && mIsCanSeek) {
            mBookmarker.setBookmark(mBookMarkPath, mVideoPosition, mVideoDuration);
        }
        mResumeableTime = System.currentTimeMillis() + RESUMEABLE_TIMEOUT;
        mHandler.removeCallbacks(mProgressChecker);
        mSeekState = ESeekState.SEEKFORWARD;
        if (mMovieActivity.mPhoneState) {
            mVideoView.pause();
        } else {
            executeSuspend();
        }
        /** @ } */
        /** SRPD:Bug474600 add @{ */
        if (misLiveStreamUri || !mIsCanSeek) {
            mController.resetTime();
            mVideoPosition = 0;
        }
        /** @ } */
        /** SPRD:Bug474609 video playback resume after interruption @{ */
        if (mResumeDialog != null && mResumeDialog.isShowing()) {
            isResumeDialogShow = false;
            mResumeDialog.dismiss();
        }
        /** @ } */
        /** SPRD:Bug 511468 add new sensor feature for MoviePlayer @{ */
        unregisterSensorEventListener();
        /** @ } */
    }

    public void onResume() {
        // SPRD:Add for bug597621 Recovery the lightness when back out the videoplayer
        recoveryLightness();
        /** SPRD:Bug 474639 add phone call reaction @{ */
        boolean isResumeFromPhone = !mIsError && isStreamUri() && mIsPhonePause
                && (mVideoPosition > 0);
        Log.d(TAG, "!mIsError is  " + !mIsError + "  isStreamUri() is  " + isStreamUri()
                + "  mIsPhonePause  is  "
                + mIsPhonePause + "  mVideoPosition > 0  is  " + (mVideoPosition > 0));
        // boolean isTimeOut = System.currentTimeMillis() - currentTime > 60 * 1000;
        boolean isTimeOut = VideoCmccUtils.getInstance().ifIsPhoneTimeout(currentTime);
        /** @ } */
        /**
         * SPRD:Bug474600 improve video control functions add new values @{
         */
                Log.d(TAG, "mPlayer's onResume method is work!");
        if (mVideoView == null) {
            initVideoView();
            /** SPRD:Bug 474639 add phone call reaction @{ */
        } else if (!isResumeFromPhone || !isTimeOut) {
            if (mTimeOutDialog != null && mTimeOutDialog.isShowing()) {
                return;
            }
            /* SPRD: add for bug533817
             * when playing drm videos and click the share button,when the dialog appears ,cancel it .
             * This process consumes the rights two times @{
             */
            if (VideoDrmUtils.getInstance().isDrmFile()) {
                mVideoView.setNeedToConsume(!VideoDrmUtils.getInstance().isConsumed());
             }
            /*@}*/
            /* SPRD:Add for bug607235 When lock and unlock the screen,the video can't play normally
            mVideoView.resume(); */
            mVideoView.setVideoURI(mUri);
            /* Bug607235 end @}*/
        }
        /** old bug info : BUG 261460 stream timeout when incall @{ */
        if (isResumeFromPhone) {
            mIsPhonePause = false;
            if (isTimeOut) {
                showTimeOutDialog();
            } else {
                mHandler.postDelayed(mResumeCallback, 1500);
                if (msavedInstance != null) { // this is a resumed activity
                    mVideoPosition = msavedInstance.getInt(KEY_VIDEO_POSITION, 0);
                    mResumeableTime = msavedInstance.getLong(KEY_RESUMEABLE_TIME, Long.MAX_VALUE);
                    msavedInstance = null;
                }
            }
        } else {
            /** SPRD :@} */
            if (isStreamUri()) {
                postResume();
            } else {
                /** SPRD add code to check show resume dialog @{ */
                Log.d(TAG, "isBookmark="+isBookmark);
                if (isBookmark) {
                    final Integer bookmark = mBookmarker.getBookmark(mBookMarkPath);
                    if (bookmark != null) {
                        mVideoPosition = bookmark;
                        Log.d(TAG, "mVideoView.pause();");
                        mVideoView.pause();
                        showResumeDialog(mMovieActivity, bookmark);
                    } else {
                        postResume();
                    }
                } else {
                    postResume();
                }
                /** @} */
            }
            /** @ } */
            /** @ } */
            if (msavedInstance != null) { // this is a resumed activity
                mVideoPosition = msavedInstance.getInt(KEY_VIDEO_POSITION, 0);
                mResumeableTime = msavedInstance.getLong(KEY_RESUMEABLE_TIME,
                        Long.MAX_VALUE);
                /**
                 * SPRD: remove @{ mVideoView.start(); mVideoView.suspend(); mHasPaused = true; @ }
                 */
                /** SPRD:Bug474609 video playback resume after interruption @{ */
                isControlPause = msavedInstance.getBoolean(KEY_PLAY_PAUST_STATE, false);
                /** @ } */
                /** SPRD:Bug474646 Add Drm feature modify by old bug 506989 @{ */
                if (VideoDrmUtils.getInstance().isDrmFile()) {
                    VideoDrmUtils.getInstance().needToConsume(
                            !msavedInstance.getBoolean(KEY_DRM_VIDEO_CONSUMED, false));
                }
                /** @ } */
            } else {
                final Integer bookmark = mBookmarker.getBookmark(mBookMarkPath);
                Log.d(TAG, "mIsBookmark="+mIsBookmark);
                if (mMovieActivity.mShowResumeDialog && bookmark != null && mIsBookmark) { // SPRD:Bug 474639 add
                    // phone call reaction
                    /** SPRD: remove @{ */
                    mVideoPosition = bookmark;
                    isBookmark = true;
                    mVideoView.pause();
                    /** @ } */
                    showResumeDialog(mMovieActivity, bookmark);
                }
            }
            /** @}*/
            /** SPRD:Bug474646 Add Drm feature modify by old bug 506989@{ */
            if (VideoDrmUtils.getInstance().isDrmFile()) {
                mVideoView.setNeedToConsume(!VideoDrmUtils.getInstance().isConsumed());
                // mVideoView.setNeedToConsume(false);
            }
            /** @} */
        }
        /** SPRD:Bug 511468 add new sensor feature for MoviePlayer @{ */
        registerSensorEventListener();
        /** @} */
        /** SPRD:Add VideoPlayer operate function @{ */
        orientation = mMovieActivity.getResources().getConfiguration().orientation;
        if(orientation != SCREEN_PORTRAIT){
            orientation = SCREEN_LANDSCAPE;
        }
        if(orintationListener == null){
            orintationListener = new OrientationEventListener(mMovieActivity){
                @Override
                public void onOrientationChanged(int rotation) {
                    int portrait;
                    if (((rotation >= 0) && (rotation <= 60)) || (rotation >= 300)) {
                        portrait = SCREEN_PORTRAIT;
                    }else{
                        portrait = SCREEN_LANDSCAPE;
                    }
                    if(orientation != portrait){
                        orientation = portrait;
                        if(!isOrientate){
                            mMovieActivity.setOrientation(ActivityInfo.SCREEN_ORIENTATION_USER);//Bug593857 modify the orientation parameter
                        }
                        mController.lockUnlockOrintation(isOrientate);
                    }
                }
            };
            orintationListener.enable();
        }
        /** @} */
    }

    /** SPRD:Bug 474639 add phone call reaction @{ */
    private void showTimeOutDialog() {
        if (mTimeOutDialog == null) {
            mTimeOutDialog = new AlertDialog.Builder(mMovieActivity).setTitle(
                    R.string.time_out).setMessage(R.string.time_out_message).setPositiveButton(
                    R.string.resume_playing_resume, new OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            // begin bug 201610
                            if (!misLiveStreamUri) {
                                mVideoView.setLastInterruptPosition(Integer.valueOf(mVideoPosition)
                                        .longValue());
                            }
                            // end bug 201610
                            mVideoView.seekTo(mVideoPosition);
                            isStop = false;
                            mVideoView.setVisibility(View.VISIBLE);
                            showControllerPlaying();
                            mVideoView.resume();
                            startVideo();
                            setProgress();
                        }
                    }).setNegativeButton(R.string.resume_playing_restart, new OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    mVideoPosition = 0;
                    // begin bug 201610
                    if (!misLiveStreamUri) {
                        mVideoView.setLastInterruptPosition(Integer.valueOf(mVideoPosition)
                                .longValue());
                    }
                    // end bug 201610
                    mVideoView.seekTo(mVideoPosition);
                    isStop = false;
                    mVideoView.setVisibility(View.VISIBLE);
                    showControllerPlaying();
                    mVideoView.resume();
                    startVideo();
                    setProgress();
                }
            }).create();
        }
        mTimeOutDialog.show();
    }

    /** @}*/
    public void onDestroy() {
        if (mVirtualizer != null) {
            mVirtualizer.release();
            mVirtualizer = null;
        }
        /**
         * SPRD:Bug474600 improve video control functions remove@{
         *
         * @orig mVideoView.stopPlayback();
         * @/ /** SPRD:bug 506989 Add Drm feature @{
         */
        if (VideoDrmUtils.getInstance().isDrmFile()) {
            VideoDrmUtils.getInstance().needToConsume(true);
        }
        /** @ } */
        /**
         * SPRD:Bug474600 improve video control functions old Bug 256898 add @{
         */
        if (mVideoView != null) {
            mVideoView.stopPlayback();
            mVideoView = null;
        }
        /** @} */
        /** SPRD:Bug 474600 improve video control functions@{ */
        mIsError = false;
        /** @} */
        if (mAudioBecomingNoisyReceiver != null) {
            mAudioBecomingNoisyReceiver.unregister();
        }
        /** SPRD:Add VideoPlayer operate function @{ */
        if(orintationListener != null){
            orintationListener.disable();
            orintationListener = null;
        }
        /** @} */
    }

    /** SPRD:bug 474614: porting float play @{ */
    public void releaseView(View rootView) {
        if (rootView != null && mController != null) {
            ((ViewGroup) rootView).removeView(mController.getView());
        }
    }

    /** @} */

    // This updates the time bar display (if necessary). It is called every
    // second by mProgressChecker and also from places where the time bar needs
    // to be updated immediately.
    private int setProgress() {
        if (mDragging || !mShowing || mVideoView == null) {// SPRD:Bug474600 add mVideoView Check
            return 0;
        }
        /**
         * SPRD:Bug474600 remove @{
         *
         * @Orig int position = mVideoView.getCurrentPosition(); @}
         */
        /*SPRD:bug614698 When play all back ,the dialog is show all the time@{*/
        if (mVideoView.isPlaying()) {
            mFirstError = false;
            mErrorPosition = -1;
            mErrorTimes = 0;
            mPreErroPosition = -1;
        }
        /*end @}*/
        int position = 0;
        if (mVideoView.isPlay()) {
            position = mVideoView.getCurrentPosition();
        } else {
            position = mVideoPosition;
        }
        int duration = mVideoView.getDuration();
        /** SPRD:Bug474600 add @{ */
        if (duration == -1)
            duration = mVideoDuration;
        if (isControlPause) {
            position = mVideoPosition;
        } else {
            if (!misLiveStreamUri) {
                if (isStreamUri()) {
                    switch (mSeekState) {
                        case SEEKFORWARD:
                            if (mVideoView.isPlay() && position > mBeforeSeekPosition) {
                                mSeekState = ESeekState.NOSEEK;
                            }
                            if (position < mVideoPosition) {
                                position = mVideoPosition;
                            }
                            break;
                        case SEEKBACK:
                            Log.d(TAG, "mBeforeSeekPosition " + mBeforeSeekPosition);
                            if (mVideoView.isPlay() && position < mBeforeSeekPosition) {
                                mSeekState = ESeekState.NOSEEK;
                            }
                            if (position > mVideoPosition) {
                                position = mVideoPosition;
                            }
                            break;
                        case NOSEEK:
                            break;

                        default:
                            break;
                    }
                } else {
                    if (mVideoPosition > 0 && position == 0)
                        position = mVideoPosition;
                }
            }
        }
        /** @} */
        /*Sprd:Bug626490 The duration of a 5 seconds video is different in VideoPlayer and image @{*/
        position = (position+500)/1000*1000;// unit is ms
        duration = (duration+500)/1000*1000;// unit is ms
        if(duration > 0 && position > duration) {
            position = duration;
        }
        /*end @}*/
        mController.setTimes(position, duration, 0, 0);
        mVideoPosition = position;
        return position;
    }

    private void startVideo() {
        // SPRD:Add for bug615788 output adjustment
        backToPreOutputDevice();
        Log.e(TAG, "startVideo()");
        // For streams that we expect to be slow to start up, show a
        // progress spinner until playback starts.
        /**
         * SPRD:Bug500699 After waiting for the play after the manual drive,click Start,Continue to
         * play video tips modify by old bug429923 @{
         */
        mComplete = false;
        /** @ } */
        String scheme = mUri.getScheme();
        if ("http".equalsIgnoreCase(scheme) || "rtsp".equalsIgnoreCase(scheme)) {
            mController.showLoading();
            mHandler.removeCallbacks(mPlayingChecker);
            mHandler.postDelayed(mPlayingChecker, 250);
        } else {
            /* SPRD:Add for bug589696 Lock the video view and re-enter it from recent app list, it displays as locked but user can control the video playing @{ */
            if (!mController.mIsLock) {
                mController.showPlaying();
            }
            /* Bug589696 end @} */
            // mController.hide();
            // SPRD：Add for bug601586 The playbutton can't control the video playing
            mController.setState(State.PLAYING);
        }
        if (mIsChanged) {
            if (!mMovieActivity.isFirst || isStop) {
                mVideoView.setVisibility(View.VISIBLE);
                mIsChanged = false;
            }
        }
        mVideoView.start();
        mVideoView.setVisibility(View.VISIBLE);
        setProgress();
        /** SPRD:Bug474600 add @{ */
        mHandler.post(mProgressChecker);
        isStop = false;
        /**@}*/
    }

    private void playVideo() {
        // SPRD:Add for bug615788 output adjustment
        backToPreOutputDevice();
        isControlPause = false;
        /*SPRD:add for bug555549
         * CRASH: com.android.gallery3d（Short Msg: java.lang.NullPointerException）@{
         */
        if (mVideoView == null) {
            return;
        }
        /*@}*/
        mVideoView.start();
        mController.showPlaying();
        mHasPaused = false;// SPRD:modify by old Bug468361
        setProgress();
        isStop = false;
        mVideoView.setVisibility(View.VISIBLE);
        EnableControlButton();
        mHandler.post(mProgressChecker);
    }

    private void pauseVideo() {
        /**
         * SRPD:Bug474600 improve video control functions remove @{
         *
         * @orig mVideoView.pause(); mController.showPaused(); @}
         */
        Log.d(TAG, "pauseVideo()");
        /**
         * SPRD:Bug474600 improve video control functions add for remove progress check @{
         */
        if (!misLiveStreamUri) {
            isControlPause = true;
            Log.d(TAG, "mVideoView.isPlay() " + mVideoView.isPlay());
            if (mVideoView.isPlay() && mVideoView.canSeekForward() && mVideoView.canSeekBackward()) {
                mVideoPosition = mVideoView.getCurrentPosition();
            }
            mHandler.removeCallbacksAndMessages(null);
            mHandler.removeCallbacks(mProgressChecker);
            mVideoView.pause();
        }
        mController.showPaused();
        showControllerPaused();
        mHasPaused = true;// SPRD:modify by old Bug468361
        /** @} */
        //Sprd:Bug626490 The duration of a 5 seconds video is different in VideoPlayer and image
        setProgress();//when pasue video, update the progress
    }

    public void pause() {
        if (mVideoView != null) {
            mVideoView.pause();
            showControllerPaused();
        }
    }

    // Below are notifications from VideoView
    @Override
    public boolean onError(MediaPlayer player, int arg1, int arg2) {
        mHandler.removeCallbacksAndMessages(null);
        // VideoView will show an error dialog if we return false,do nothing here
        mIsError = true;
        /*SPRD:bug614698 When play all back ,the dialog is show all the time@{*/
        if (currentPlayBackMode == ALL_PLAYBACK && mPlayList != null) {
            if (!mFirstError) {
                mErrorPosition = curVideoIndex -1;
                if(mErrorPosition < 0) {
                    mErrorPosition = mPlayList.size() -1;
                }
            }
            if (mPreErroPosition != curVideoIndex) {
                mErrorTimes++;
                mPreErroPosition = curVideoIndex;
            }
            mFirstError = true;
            if (curVideoIndex == mErrorPosition && mErrorTimes == mPlayList.size()) {
                currentPlayBackMode = 0;
                mFirstError = false;
                mErrorPosition = -1;
                mErrorTimes = 0;
                mPreErroPosition = -1;
            }

        }
        /*end @}*/
        if (currentPlayBackMode == SINGLE_PLAYBACK) {
            currentPlayBackMode = 0;
        }
        mController.showErrorMessage("");
        return false;
    }

    @Override
    public void onCompletion(MediaPlayer mp) {
        /* SPRD: Add for bug608328 progress bar can not reach the end of point when the video playing finished @{ */
        if (!mDragging && mShowing && mVideoView != null) {
            mController.setTimes(mVideoView.getDuration(), mVideoView.getDuration(), 0, 0);
         }
        /* Bug608328 end @} */
        /**
         * SPRD: Bug474615 Playback loop mode change for PlayBackMode @{
         */
        Log.d(TAG, "currentPlayBackMode " + currentPlayBackMode);
        scheme = ((mUri == null) ? null : mUri.getScheme()); // SPRD: Defensive code
        mComplete = true;
        if (mPlayList != null) {
            if (currentPlayBackMode == SINGLE_PLAYBACK) {
                singlePlayBack();
            } else if (currentPlayBackMode == ALL_PLAYBACK) {
                allPlayBack();
            } else {
                //Sprd:Bug626490 The duration of a 5 seconds video is different in VideoPlayer and image
                setProgress();
                mController.showEnded();
                onCompletion();
            }

        } else if ("file".equalsIgnoreCase(scheme)
                || "content".equalsIgnoreCase(scheme) ||(mMovieActivity.mFragmentName != null &&  (mMovieActivity.mFragmentName).equals(HISTORY_VIDEOS_FRAGMENT))) {// SPRD:add 255423;avoid null pointer for
            if (currentPlayBackMode == SINGLE_PLAYBACK || currentPlayBackMode == ALL_PLAYBACK) {
                mVideoView.setVideoURI(mUri);
                mVideoPosition = 0;
                mVideoDuration = 0;
                isControlPause = false;
                startVideo();
                showControllerPlaying();
                EnableControlButton();
            } else {
                //Sprd:Bug626490 The duration of a 5 seconds video is different in VideoPlayer and image
                setProgress();
                mController.showEnded();
                onCompletion();
            }
        } else {
            //Sprd:Bug626490 The duration of a 5 seconds video is different in VideoPlayer and image
            setProgress();
            mController.showEnded();
            onCompletion();
        }
        /** @} */
    }

    public void onCompletion() {
        /**
         * SPRD:Bug474600 improve video control functions reset value @{
         */
        mVideoDuration = 0;
        mVideoPosition = 0;
        /** @ } */
        /** SPRD:bug 506989 Add Drm feature @{ */
        if (VideoDrmUtils.getInstance().isDrmFile()) {
            VideoDrmUtils.getInstance().needToConsume(true);
        }
        /** @ } */
    }

    /** SPRD:Bug521702 the music player and the video player can play at the same time @{ */
    public void onPauseVideo() {
        if (mMovieActivity.isCallingState()) {
            return;
        }
        if (mVideoView.isPlaying()) {
            if (misLiveStreamUri) {
                stopPlaybackInRunnable();
                mVideoView.setVisibility(View.INVISIBLE);
                mVideoPosition = 0;
                isStop = true;
                mController.resetTime();
                mController.timeBarEnable(false);
            } else {
                pauseVideo();
                mHasPaused = true;
            }
        }
    }

    /* SPRD:Bug531413 modify by old bug521106 for video replay @{ */
    public void onPlayVideo() {
        // SPRD:Add for bug609816 When we call the method only when we have audiofocus
        if (VideoDrmUtils.getInstance().isDrmFile()) {
            Log.d(TAG, "VideoDrmUtils.getInstance().isDrmFile() return true");
            VideoDrmUtils.getInstance().checkRightBeforePlay(mMovieActivity, this);
        } else {
            // If not a drm file, just follow oringinal logic
            playVideo();
            mHasPaused = false;
        }
    }

    /**@}*/

    public boolean isPlaying() {
        if (mVideoView != null) {
            return mVideoView.isPlaying();
        } else {
            return false;
        }
    }

    /*SPRD:Bug620570 When have a call,the button of playpause maybe show play @{*/
    public boolean haveResumeDialog(){
        if (mResumeDialog != null) {
            return true;
        }
        return false;
    }
    /*Bug620570 end@}*/
    /** @ } */
    // Below are notifications from ControllerOverlay
    @Override
    public void onPlayPause() {
        /** SPRD:Bug 474639 add phone call reaction @{ */
        if (mMovieActivity.isCallingState()) {
            return;
        }
        /** @ } */
        /**
         * SPRD:Bug474600 improve video control functions add to check video playing
         */
        if (mVideoView.isPlaying()) {
            if (misLiveStreamUri) {
                stopPlaybackInRunnable();
                mVideoView.setVisibility(View.INVISIBLE);
                mVideoPosition = 0;
                isStop = true;
                mController.resetTime();
                mController.timeBarEnable(false);
                /**
                 * SPRD:Bug474600 improve video control functions remove @{ pauseVideo(); @}
                 */
            } else {
                pauseVideo();
                mHasPaused = true;
            }
        } else {
            /* SPRD:Add for bug609816 Wrong logic of the audiofocus control @{ */
            if (!requestAudioFocus()) {
                return;
            }
            /* Bug609816 end @} */
            /** SPRD:Bug474646 Add Drm feature @{ @{ */
            if (VideoDrmUtils.getInstance().isDrmFile()) {
                Log.d(TAG, "VideoDrmUtils.getInstance().isDrmFile() return true");
                VideoDrmUtils.getInstance().checkRightBeforePlay(mMovieActivity, this);
                // SPRD:Add for bug613675 The rights consume for drm videos is not normal
                VideoDrmUtils.getInstance().setStopState(false);
            } else {
                // If not a drm file, just follow oringinal logic
                playVideo();
                mHasPaused = false;
            }
            /** @ } */
        }
        mController.maybeStartHiding();//SPRD:add for new feature 568552 hide the bar whether the video player is paused or playing
    }

    @Override
    public void onSeekStart() {
        mDragging = true;
    }

    @Override
    public void onSeekMove(int time) {
        /**
         * SPRD:Bug520393 there are some noise when seek the video modify by old Bug376820
         *
         * @orig remove @{ mVideoView.seekTo(time); @}
         */
    }

    @Override
    public void onSeekEnd(int time, int start, int end) {
        mDragging = false;
        /**
         * SPRD:Bug474600 improve video control functions remove @{
         *
         * @orig mVideoView.seekTo(time);
         */
        /** SPRD:Bug474600 add @{ */
        if (mVideoView != null) {
            mBeforeSeekPosition = mVideoView.getCurrentPosition();
            Log.d(TAG, "onSeekEnd mBeforeSeekPosition " + mBeforeSeekPosition);
            mVideoPosition = time;
            seek(time);
            /** @} */
            setProgress();
        }
    }

    @Override
    public void onShown() {
        mShowing = true;
        setProgress();
        // SPRD:Modify for new feature 568552
         showSystemUi(true);
        /**SRPD:Bug474600 improve video control functions
         * show the ActionBar @{*/
        mActionBar.show();
        /** @}*/
    }

    @Override
    public void onHidden() {
        mShowing = false;
        // SPRD:Modify for new feature 568552
        showSystemUi(false);
        mActionBar.hide();
    }

    @Override
    public void onReplay() {
        startVideo();
    }

    /** SPRD:bug 474614: porting float play @{ */
    // old bug info:339523 begin
    @Override
    public void onStartFloatPlay() {
        //SPRD:Bug612818 When goto floatwindow is not smooth
        showStatusBar();
        /* SPRD: Add for bug590286 com.android.gallery3d (pid 22867) by old bug567647 @{ */
        if (mVideoView == null) {
            return;
        }
        /* Bug590286 end @} */
        // add for bug502230,play videos,call in,start the float player
        // the float video playing and the calling run at the same time
        if (mMovieActivity.isCallingState()) {
            return;
        }
        /*SPRD:Bug 614653 Starting floatPlayer notices "Unfortunately, Gallyery has stopped"@{*/
        if (Settings.canDrawOverlays(mMovieActivity) == false) {
            Toast.makeText(mMovieActivity, R.string.gallery_premission_error,
                    Toast.LENGTH_LONG).show();
            Intent permission_intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION);
            permission_intent.setData(Uri.parse("package:" + mMovieActivity.getPackageName()));
            mMovieActivity.startActivity(permission_intent);
            return;
        }
        /*Bug 614653 end@}*/
        MediaPlayer mediaPlayer = null;
        try {
            mediaPlayer = new MediaPlayer();// check whether the file is exist
                                            // or not
            mediaPlayer.setDataSource(mContext, mUri);
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (SecurityException e) {
            e.printStackTrace();
        } catch (IllegalStateException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
            Toast.makeText(mContext, R.string.video_file_does_not_exist,
                    Toast.LENGTH_SHORT).show();
            return;
        } finally {
            if (mediaPlayer != null) {
                mediaPlayer.reset();
                mediaPlayer.release();
            }
        }
        int position = mVideoView.getCurrentPosition();
        int state = mVideoView.isPlaying() ? FloatMoviePlayer.STATE_PLAYING
                : FloatMoviePlayer.STATE_PAUSED;
        Log.d(TAG, "onStartFloatPlay position=" + position + " state=" + state);
        mVideoView.stopPlayback();

        mServiceIntent = new Intent(mMovieActivity, FloatPlayerService.class);
        mServiceIntent.putExtras(mMovieActivity.getIntent());
        mServiceIntent.setData(mUri);
        mServiceIntent.putExtra("position", position);
        mServiceIntent.putExtra("currentstate", state);
        mServiceIntent.putExtra("currentPlaybackMode", currentPlayBackMode);
        mServiceIntent.putExtra("FullScreen", mMovieActivity.getIsFullScreen());
        mServiceIntent.putExtra("left", mLeftVolume);
        mServiceIntent.putExtra("right", mRightVolume);
        mServiceIntent.putExtra("LoudSpeakerOn", mMovieActivity.getIsLoudSpeakerOn());
        mServiceIntent.putExtra("optionsSelectedLoudSpeakerOn", mMovieActivity.
                getOptionsSelectedLoudSpeakerOn());
        //Sprd:Bug613723 VideoPlayer in all_play_back mode,goto floatwindow and play nextthen goto normal window,the title is wrong
        mServiceIntent.putExtra(Intent.EXTRA_TITLE,mActionBar.getTitle());
        /**
         * SPRD:532462 SPRD:add Intent.FLAG_GRANT_READ_URI_PERMISSION to grant read
         * EmlAttachmentProvider permission
         */
        mServiceIntent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        mMovieActivity.startService(mServiceIntent);
        mMovieActivity.finish();
    }
    // bug 339523 end
    /** @} */
    // Below are key events passed from MovieActivity.
    public boolean onKeyDown(int keyCode, KeyEvent event) {

        // Some headsets will fire off 7-10 events on a single click
        if (event.getRepeatCount() > 0) {
            return isMediaKey(keyCode);
        }

        switch (keyCode) {
            case KeyEvent.KEYCODE_HEADSETHOOK:
            case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
                /**
                 * SPRD:Bug 474631 add headset Control old bug info: add for bug 327112 when long
                 * press KeyEvent.KEYCODE_HEADSETHOOK or KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE the
                 * MoviePlayer do nothing ori: if (mVideoView.isPlaying()) { pauseVideo(); } else {
                 * playVideo(); }
                 *
                 * @{
                 */
                mKeyDownEventTime = event.getEventTime();
                /** @} */
                return true;
            case KEYCODE_MEDIA_PAUSE:
                if (mVideoView != null && mVideoView.isPlaying()) {
                    /**
                     * SPRD:531424 press KEYCODE_MEDIA_PAUSE, the Live stream has been stopped
                     */
                    if (misLiveStreamUri) {
                        onPauseVideo();
                    } else {
                        pauseVideo();
                    }
                }
                return true;
            case KEYCODE_MEDIA_PLAY:
                if (mVideoView != null && !mVideoView.isPlaying() && requestAudioFocus()) {
                    playVideo();
                }
                return true;
            case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
                /* SPRD:Add for Bug608362 The videoplayer can't be controlled by headset @{ */
                if (mPlayList != null && mPlayList .size() > 0) {
                    prevVideo();
                }
                return true;
            case KeyEvent.KEYCODE_MEDIA_NEXT:
                if (mPlayList != null && mPlayList .size() > 0){
                    nextVideo();
                }
                /* Bug608362 end @} */
                return true;
        }
        return false;
    }

    public boolean onKeyUp(int keyCode, KeyEvent event) {
        /**
         * SPRD:Bug 474631 add headset Control old bug info: add for bug 327112 when long press
         * KeyEvent.KEYCODE_HEADSETHOOK or KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE the MoviePlayer do
         * nothing @{
         */
        if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK || keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE) {
            if (event.getEventTime() - mKeyDownEventTime <= 500) {
                if (mVideoView != null && mVideoView.isPlaying()) {
                    /**
                     * SPRD:531424 press KEYCODE_MEDIA_PAUSE, the Live stream has been stopped
                     */
                    if (misLiveStreamUri) {
                        Log.d(TAG, "KEYCODE_MEDIA_PLAY_PAUSE Live Stream onPauseVideo()");
                        onPauseVideo();
                    } else {
                        Log.d(TAG, "KEYCODE_MEDIA_PLAY_PAUSE pauseVideo()");
                        pauseVideo();
                    }
                } else {
                    /* SPRD:Add for bug609816 Wrong logic of the audiofocus control @{ */
                    if (requestAudioFocus()) {
                        playVideo();
                    }
                    /* Bug609816 end @} */
                }
            }
            mKeyDownEventTime = 0;
            return true;
        }
        /** @} */
        return isMediaKey(keyCode);
    }

    private static boolean isMediaKey(int keyCode) {
        return keyCode == KeyEvent.KEYCODE_HEADSETHOOK
                || keyCode == KeyEvent.KEYCODE_MEDIA_PREVIOUS
                || keyCode == KeyEvent.KEYCODE_MEDIA_NEXT
                || keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE
                || keyCode == KeyEvent.KEYCODE_MEDIA_PLAY
                || keyCode == KeyEvent.KEYCODE_MEDIA_PAUSE;
    }

    // We want to pause when the headset is unplugged.
    private class AudioBecomingNoisyReceiver extends BroadcastReceiver {

        public void register() {
            mContext.registerReceiver(this,
                    new IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY));
        }

        public void unregister() {
            mContext.unregisterReceiver(this);
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            /**SPRD:231424,bug537283
             * SPRD:when receive broadcast of ACTION_AUDIO_BECOMING_NOISY, the live stream does not response
             */
            if (mVideoView != null && mVideoView.isPlaying() && !misLiveStreamUri)
                pauseVideo();
            }
    }

    /**
     * SPRD:bug 474646 Add Drm feature @{ SPRD: Add these ugly public wrappers so that private
     * methods can be invoked in drm plugin
     */
    public void executeSuspend() {
        /** SPRD:Bug474646 Add Drm feature modify by old bug 506989@{ */
        if (VideoDrmUtils.getInstance().isDrmFile() && !VideoDrmUtils.getInstance().isConsumed()) {
            VideoDrmUtils.getInstance().needToConsume(false);
        }
        /** @} */
        // SPRD: Add for Drm feature
        if (mVideoView == null) {
            return;
        }
        mVideoView.suspend();
    }

    public void playVideoWrapper() {
        /* SPRD:Add for bug609816 Wrong logic of the audiofocus control @{ */
        if (!requestAudioFocus()) {
            return;
        }
        /* Bug609816 end @} */
        playVideo();
        mHasPaused = false;
    }

    public void changeVideoWrapper() {
        /* SPRD:Add for bug609816 Wrong logic of the audiofocus control @{ */
        if (!requestAudioFocus()) {
            return;
        }
        /* Bug609816 end @} */
        _changeVideo();
    }

    /**@}*/
    /**
     * SPRD:Bug474600 improve video control functions add new private methods @{
     */
    public boolean isStreamUri() {
        return ("http".equalsIgnoreCase(scheme) || "rtsp"
                .equalsIgnoreCase(scheme));
    }

    @Override
    public boolean isNewStreamUri() {
        return isStreamUri();
    }

    private void nextVideo() {
        Log.d(TAG, "nextVideo()");
        /* SPRD:Add for bug609816 Wrong logic of the audiofocus control @{ */
        if (!requestAudioFocus()) {
            return;
        }
        /* Bug609816 end @} */
        /** SPRD:Bug474646 Add Drm feature modify by old bug 506989@{ */
        if (VideoDrmUtils.getInstance().isDrmFile()) {
            VideoDrmUtils.getInstance().needToConsume(true);
        }
        /** @} */
        if (mPlayList != null) {
            if (curVideoIndex == mPlayList.size() - 1) {
                curVideoIndex = 0;
            } else {
                ++curVideoIndex;
            }
            /** SPRD:Bug609881 This method is not used when the MoviePlayerVideoView is null@{ */
            if (mVideoView != null) {
                changeVideo();
            }
            /** @} */
        } else {
            if (mVideoView != null) {
                mVideoView.stopPlayback();
                mVideoView.setVideoURI(mUri);
                mVideoPosition = 0;
                mVideoDuration = 0;
                isControlPause = false;
                startVideo();
                showControllerPlaying();
                EnableControlButton();
            }
        }
    }

    private void prevVideo() {
        Log.d(TAG, "prevVideo()");
        /* SPRD:Add for bug609816 Wrong logic of the audiofocus control @{ */
        if (!requestAudioFocus()) {
            return;
        }
        /* Bug609816 end @} */
        /** SPRD:Bug474646 Add Drm feature modify by old bug 506989@{ */
        if (VideoDrmUtils.getInstance().isDrmFile()) {
            VideoDrmUtils.getInstance().needToConsume(true);
        }
        /** @} */
        if (mPlayList != null) {
            if (curVideoIndex <= 0) {
                curVideoIndex = mPlayList.size() - 1;
            } else {
                --curVideoIndex;
            }
            /** SPRD:Bug609881 This method is not used when the MoviePlayerVideoView is null@{ */
            if (mVideoView != null) {
                changeVideo();
            }
            /** @} */
        } else {
            if (mVideoView != null) {
                mVideoView.stopPlayback();
                mVideoView.setVideoURI(mUri);
                mVideoPosition = 0;
                mVideoDuration = 0;
                isControlPause = false;
                startVideo();
                showControllerPlaying();
                EnableControlButton();
            }
        }
    }

    private void changeVideo() {
        Log.d(TAG, "changeVideo()");
        misLiveStreamUri = false;
        mController.setNotLiveMode();
        if (isStreamUri()) {
            stopPlaybackInRunnable();
            changeVideoInRunable();
        } else {
            if (mVideoView != null && !isStreamUri() && mVideoView.canSeekBackward()
                    && mVideoView.canSeekForward()) {
                mBookmarker.setBookmark(mBookMarkPath, mVideoPosition,
                        mVideoDuration);
            }
            mController.showLoading();
            mVideoView.stopPlayback();
            mVideoView.setVisibility(View.INVISIBLE);
            // _changeVideo();
            // Before check right, file path must be updated
            MovieInfo item = mPlayList.get(curVideoIndex);
            String id = item.mID;
            mUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI.buildUpon().appendPath(id).build();
            VideoDrmUtils.getInstance().getFilePathByUri(mUri, mMovieActivity);
            if (VideoDrmUtils.getInstance().isDrmFile()) {
                VideoDrmUtils.getInstance().checkRightBeforeChange(mMovieActivity, this);
            } else {
                _changeVideo();
            }
        }
    }

    private void _changeVideo() {
        Log.d(TAG, "_changeVideo() ");
        mIsChanged = true;
        mMovieActivity.mShowResumeDialog = true;// SPRD: bug 526178 just show resume dialog once.
        MovieInfo item = mPlayList.get(curVideoIndex);
        /**
         * SPRD:Bug474605 video sharing change by bug old 276956 @{
         */
        /**
         * SPRD:Bug474605 video sharing change by old Bug339523 @{
         */
        // mUri = Uri.fromFile(new File(item.mAlbum));
        String id = item.mID;
        mUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI.buildUpon().appendPath(id).build();
        /**@}*/
        mVideoView.setVideoURI(mUri);
        /**@}*/
        changeBookMarkerPathUri(mUri);//SPRD:Bug540206 add
        /**
         * SPRD:Bug474605 video sharing add by old Bug304358
         */
        Uri nUri = Uri.parse("content://media/external/video/media");
        Uri mVideoUri = null;
        String picpath = item.mPath;
        Cursor cursor = null;
        try {
            cursor = mMovieActivity.getContentResolver().query(
                    MediaStore.Video.Media.EXTERNAL_CONTENT_URI, null, null, null,
                    MediaStore.Video.Media.DEFAULT_SORT_ORDER);
            if (cursor != null) {
                Log.d(TAG, "cursor: " + cursor);
                cursor.moveToNext();
                while (!cursor.isAfterLast()) {
                    String data = cursor.getString(cursor
                            .getColumnIndex(MediaStore.MediaColumns.DATA));
                    if (picpath.equals(data)) {
                        int RTID = cursor.getInt(cursor
                                .getColumnIndex(MediaStore.MediaColumns._ID));
                        mVideoUri = Uri.withAppendedPath(nUri, "" + RTID);
                        break;
                    }
                    cursor.moveToNext();
                }
            }
            mMovieActivity.onVideoUriChange(mVideoUri);
            mAlbum = item.mAlbum;
            mActionBar.setTitle(mAlbum);
            /** SPRD:Bug474635 exit the video player interface @{ */
            mId = item.mID;
            /** @}*/
            mVideoPosition = 0;
            mVideoDuration = 0;
            isControlPause = false;
            if (mVideoView != null) {
                mVideoView.errorDialogCheckAndDismiss();
            }
            startVideo();
            showControllerPlaying();
            EnableControlButton();
        } catch (Exception e) {
            Log.i(TAG, "Exception" + e);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        /**@}*/
    }

    /** SPRD:Bug474635 exit the video player @{ */
    public String getCurrentId() {
        return mId;
    }
    /**@}*/

    private void stopPlaybackInRunnable() {
        isStop = true;
        Message msgBegin = mHandler.obtainMessage(CMD_STOP_BEGIN);
        mHandler.removeMessages(CMD_STOP_BEGIN);
        mHandler.sendMessage(msgBegin);

        new Thread(new Runnable() {

            @Override
            public void run() {
                try {
                    if (mVideoView != null) {
                        mVideoView.stopPlayback();
                    }
                    Message msgEnd = mHandler.obtainMessage(CMD_STOP_END);
                    mHandler.removeMessages(CMD_STOP_END);
                    mHandler.sendMessage(msgEnd);
                } catch (NullPointerException e) {

                }
            }
        }).start();

        mController.timeBarEnable(false);
    }

    private void changeVideoInRunable() {
        if (!mVideoView.isStopPlaybackCompleted()) {
            mHandler.postDelayed(new Runnable() {

                @Override
                public void run() {
                    changeVideoInRunable();
                }
            }, 100);
        } else {
            _changeVideo();
        }
    }

    private void singlePlayBack() {
        int n = mPlayList.size();
        if (n == 0) {
            return;
        }
        changeVideo();
    }

    private void allPlayBack() {
        Log.d(TAG, "mPlayList.size() " + mPlayList.size());
        /** SPRD:Bug474646 Add Drm feature modify by old bug 506989@{ */
        if (VideoDrmUtils.getInstance().isDrmFile()) {
            VideoDrmUtils.getInstance().needToConsume(true);
        }
        /** @} */
        int n = mPlayList.size();
        if (n == 0) {
            return;
        }
        if (++curVideoIndex >= n) {
            curVideoIndex = 0;
        }
        Log.d(TAG, "curVideoIndex " + curVideoIndex);
        changeVideo();
    }

    private enum ESeekState {
        SEEKFORWARD, NOSEEK, SEEKBACK
    }

    ESeekState mSeekState = ESeekState.SEEKFORWARD;

    private void seek(int time) {
        if (mVideoView != null) {
            if (mVideoView.getCurrentPosition() > time) {
                mSeekState = ESeekState.SEEKBACK;
            } else if (mVideoView.getCurrentPosition() < time) {
                mSeekState = ESeekState.SEEKFORWARD;
            } else {
                mSeekState = ESeekState.NOSEEK;
            }
            mVideoPosition = time;
            mVideoView.seekTo(time);
            if (isStreamUri()) {
                mController.showLoading();
                /* SPRD:Add for bug605798 when seeking the video,the progressbar cant update normally @{
                mHandler.removeCallbacks(mPlayingChecker);
                mHandler.postDelayed(mPlayingChecker, 250);
                */
                /* Bug605798 end @} */
            }
            setProgress();
        }
    }

    private void showControllerPlaying() {
        if (!mController.mIsLock) {
            if (!misLiveStreamUri) {
                mController.showPlaying();
            } else {
                mController.clearPlayState();
            }
        }
    }

    private void EnableControlButton() {
        if (isStop) {
            mController.timeBarEnable(false);
        } else {
            mController.timeBarEnable(mVideoView.canSeekBackward());
        }
        mController.setControlButtonEnableForStop(true);
        if (misLiveStreamUri) {
            mController.setLiveMode();
        }
    }

    private void showControllerPaused() {
        if (!mController.mIsLock) {
            if (!misLiveStreamUri) {
                mController.showPaused();
            } else {
                mController.clearPlayState();
            }
        }
    }

    private void postResume() {
        if (!mIsCanSeek && !misLiveStreamUri) {
            Toast.makeText(mMovieActivity, R.string.special_video_restart_playing,
                    Toast.LENGTH_LONG)
                    .show();
        }
        /* SPRD:Bug 630851 VideoPlayer play a damaged video ,when it can't play press Home button and open it again ,it restart play @{ */
        if (mIsError) {
            pauseVideo();
            return;
        /* @} */
        } else if (isControlPause) {
            mVideoView.seekTo(mVideoPosition);
            /** SPRD new code for bug 255338 @{ */
            if (!isStreamUri()) {
                mVideoView.setVisibility(View.VISIBLE);
            }
            /** @} */
            Log.d(TAG, "postResume() pauseVideo()");
            pauseVideo();
        } else if (!isStop && !isResumeDialogShow) {// SPRD:Bug474609 video playback resume after
            // interruption add
            mVideoView.seekTo(mVideoPosition);
            isStop = false;
            showControllerPlaying();
            startVideo();
        }
    }

    private Runnable mResumeCallback = new Runnable() {

        @Override
        public void run() {
            postResume();
        }
    };

    public void onConfigurationChanged(Configuration newConfig) {
        mDragging = false;
        mController.setscrubbing();
        //SPRD:Bug605721 Turn screen and then click pause button ,the controllerOvlay is disappear
        mController.removeHandler();
        if (isStreamUri()) {
            if (!isStop) {
                mController.showLoading();
                mHandler.removeCallbacks(mPlayingChecker);
                mHandler.postDelayed(mPlayingChecker, 250);
            }
        } else {
            /** SPRD new code @{ */
            if (mVideoView != null) {
                if (mVideoView.isPlaying()) {
                    showControllerPlaying();
                } else if (mHasPaused || isStop) { // SPRD:add for 251812
                    showControllerPaused();
                }
            }
            /** @} */
        }
    }

    /** @}*/
    /**
     * SPRD:Bug474600 improve video control functions add new public methods @{
     */
    @Override
    public void onStopVideo() {
        long currentTime = System.currentTimeMillis();
        if (currentTime - mClickTime < 350)
            return;
        mClickTime = currentTime;
        if (isStop || mVideoView.isStopPlaybackCompleted()) {
            return;
        }
        mController.timeBarEnable(false);
        stopPlaybackInRunnable();
        mVideoView.setVisibility(View.INVISIBLE);
        mVideoPosition = 0;
        setProgress();
        isStop = true;
        // SPRD:Add for bug613675 The rights consume for drm videos is not normal
        VideoDrmUtils.getInstance().setStopState(false);
    }

    @Override
    public void onNext() {
        nextVideo();
    }

    @Override
    public void onPrev() {
        prevVideo();
    }

    @Override
    public void onSeekComplete(MediaPlayer arg0) {
        /** SPRD: new code @{ */
        Log.d(TAG, "onSeekComplete");
        if (isControlPause) {
            showControllerPaused();
        }
        /** @} */
    }

    @Override
    public void onPrepared(MediaPlayer arg0) {
        Log.d(TAG, "onPrepared isplaying=" + mVideoView.isPlaying());
        Log.d("CanseekForward", "mVideoView.canSeekForward is " + mVideoView.canSeekForward());
        mVideoDuration = mVideoView.getDuration();
        /** SPRD: new code @{ */
        if (!misLiveStreamUri && !isStop && !mController.mIsLock) {
            mController.timeBarEnable(mVideoView.canSeekBackward());
        } else if (isStop) {
            mController.timeBarEnable(false);
        }
        // SPRD：Add for bug621147 The audio focus is retrieved when playback begins
        requestAudioFocus();
        // SPRD：Add for bug609816 When we start playing,request audiofocus first.
        if (isStreamUri() && requestAudioFocus()) {
            Log.d(TAG, "startVideo() http rtsp");
            mHandler.removeCallbacks(mProgressChecker);
            mHandler.post(mProgressChecker);
            EnableControlButton();
            mHandler.removeCallbacks(mPlayingChecker);
            mHandler.postDelayed(mPlayingChecker, 250);
            mVideoView.start();
            // SPRD:Add for bug615788 output adjustment
            backToPreOutputDevice();
        }
    }

    public void setVideoViewInvisible() {
        if (mVideoView != null) {
            mVideoView.setVisibility(View.INVISIBLE);
        }
    }

    /* SPRD: Fix bug 620554 Hide the pre/next button @{ */
    public void setNextPrevBtnInvisible() {
        if (mController != null) {
            mController.showNextPrevBtn(false);
        }
    }
    /* @} */

    public void hideToShow() {
        this.mController.hideToShow();
    }

    @Override
    public boolean onInfo(MediaPlayer mp, int what, int extra) {
        /**
         * SPRD:Bug474600 improve video control functions add media buffering start and end process@{
         */
        Log.d(TAG, "onInfo " + what);
        if (what == MediaPlayer.MEDIA_INFO_NOT_SEEKABLE) {
            /** SPRD:modify by old bug515315 */
            if (isStreamUri()) {
                misLiveStreamUri = true;
            }
        }
        /**@}*/
        if (isStreamUri()) {
            if (what == MediaPlayer.MEDIA_INFO_BUFFERING_START) {
                // SPRD:Add for bug605798 when seeking the video,the progress bar cant update normally
                mVideoView.setIsPlay(false);
                mController.showLoading();
            } else if (what == MediaPlayer.MEDIA_INFO_BUFFERING_END) {
                if (!mVideoView.isPlay() && !isControlPause) {
                    mVideoView.start();
                }
                if (mVideoView.isPlaying()) {
                    mController.showPlaying();
                }
            }
        }
        return false;
    }

    /** @} */
    /**
     * SPRD:Bug474615 Playback loop mode new method @{
     */
    public void setPlayBackMode(int mode) {
        currentPlayBackMode = mode;
    }

    /** @} */
    public void setChannel(float left, float right) {
        mLeftVolume = left;
        mRightVolume = right;
    }

    /**
     * SPRD:Bug474621 screen size setting add new public method @{
     */
    public void resize(boolean isFit) {
        if (mVideoView != null) {
            mVideoView.resize(isFit);
        }
    }

    @Override
    public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
        setProgress();
        mHandler.removeCallbacks(mProgressChecker);
        mHandler.post(mProgressChecker);
        /** SPRD:Bug474618 Channel volume settings @{ */
        Log.d(TAG, "mLeftVolume=" + mLeftVolume + "mRightVolume" + mRightVolume);
        setChannelVolume(mLeftVolume, mRightVolume);
        /**@}*/
        if (!mController.mIsLock) {
            EnableControlButton();
        }
        if (isStreamUri()) {
            Log.d(TAG, "startVideo() http rtsp");
            mHandler.removeCallbacks(mPlayingChecker);
            mHandler.postDelayed(mPlayingChecker, 250);
        } else {
            if (isControlPause) {
                showControllerPaused();
            } else {
                showControllerPlaying();
            }
        }
    }

    /** @} */
    /**
     * SPRD:Bug474618 Channel volume settings new method @{
     */
    public void setChannelVolume(float left, float right) {
        mLeftVolume = left;
        mRightVolume = right;
        if (mVideoView != null) {
            mVideoView.setChannelVolume(left, right);
        }
    }

    /** @} */

    public void updateVolume(int volume) {
        Log.d(TAG, "updateVolume volume " + volume);
        if (volume > mMaxVolume || volume < 0) {
            return;
        }
        /* SPRD: Fix bug613744 Plug in the headset Slide the screen to adjust the volume, to achieve the volume protection value after the pop-up system prompt box @{ */
        if (mAudioManager != null && mAudioManager.isWiredHeadsetOn() && mAudioManager.getSafeMediaVolumeEnabled()
                && volume > VOLUME_SAFE_VALUE) {
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, VOLUME_SAFE_VALUE + 1, AudioManager.FLAG_SHOW_UI);
        } else {
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, volume, 0);
        }
        /* @} */
    }

    /** SPRD:Bug 511468 add new sensor feature for MoviePlayer @{ */
    private SensorEventListener mSensorListener = new SensorEventListener() {
        public void onSensorChanged(SensorEvent event) {
            if (mKeyguardManager.isKeyguardLocked()) {
                return;
            }
            mZaxis=(int)event.values[SensorManager.DATA_Z];
            Log.d(TAG, "event:" + mZaxis);
            if (mZaxis == 1) {
                if (mHasPaused) {
                    /* SPRD:Add for bug609816 Wrong logic of the audiofocus control @{ */
                    if (!requestAudioFocus()) {
                        return;
                    }
                    /* Bug609816 end @} */
                    playVideo();
                    mHasPaused = false;
                }
            } else if (mZaxis == 2) {
                if (mVideoView.isPlaying()) {
                    pauseVideo();
                    mHasPaused = true;
                }
            }
        }

        public void onAccuracyChanged(Sensor sensor, int accuracy) {
        }
    };

    public void registerSensorEventListener() {
        mKeyguardManager = (KeyguardManager)mContext.getSystemService(Service.KEYGUARD_SERVICE);
        mSensorManager = (SensorManager) mContext.getSystemService(Context.SENSOR_SERVICE);
        mSensor = mSensorManager.getDefaultSensor(SprdSensor.TYPE_SPRDHUB_FLIP);
        mIsSettingOpen=Settings.Global.getInt(mContext.getContentResolver(), Settings.Global.PLAY_CONTROL, 0) != 0;
        Log.d(TAG, "mIsSettingOpen "+mIsSettingOpen);
        if (!mIsSettingOpen) {
            return;
        }
        Log.d(TAG, "mSensorManager  "+(mSensorManager != null)+"   mSensor  "+(mSensor!= null));
        if (mSensorManager != null && mSensor != null) {
        Log.d(TAG, "mSensor is registered");
            mSensorManager.registerListener(mSensorListener, mSensor,
                    SensorManager.SENSOR_DELAY_NORMAL);
        }
    }

    public void unregisterSensorEventListener() {
        if (mSensorManager != null) {
            mSensorManager.unregisterListener(mSensorListener);
            mSensorManager = null;
        }
    }
    /**@}*/

    /** SPRD:Bug538278 when you unplug the SD card, a error dialog always shows @{ */
    public void disableMediaPlayer() {
        if (mMovieActivity != null) {
            mMovieActivity.finish();
        }
    }
    /**@}*/
    /** SPRD:Bug540206 the Video can't resume normally, when press the next and pre button  @{ */
    public void changeBookMarkerPathUri(Uri uri){
        if(uri == null)return;
        scheme = uri.getScheme();
        if ("content".equalsIgnoreCase(scheme)) {
            if (mPlayList != null) {
                String UriID = uri.toString().substring(
                        uri.toString().lastIndexOf("/") + 1,
                        uri.toString().length());
                for (int i = 0; i < mPlayList.size(); i++) {
                    if (UriID.equalsIgnoreCase(mPlayList.get(i).mID)) {
                        mBookMarkPath = Uri.parse(mPlayList.get(i).mPath);
                        curVideoIndex = i;
                        Log.d(TAG, "mBookMarkPath in changePath method: " + mBookMarkPath);
                        break;
                    }
                }
            }
        }
    }
    /**@}*/

    @Override
    public int getVideoWidth(){
        if(mVideoView != null){
            return mVideoView.getMovieWidth();
        }
        return 0;
    }

    /*
     * Add for new feature 568552 when double click the screen,the controller won't be shown when it was not shown before it. @{
     */
    @Override
    public void onVideoSliding(float percent) {
        //SPRD:bug615376 The videoplayer may happens crash when excute sliding
        if(mVideoView != null) {
            if (mCurrentPosition == -1) {
                mCurrentPosition = mVideoView.getCurrentPosition();
            }
            int duration = mVideoView.getDuration();
            float targetPosition = (float) mCurrentPosition + (float) 120000 * percent;
            if (targetPosition > duration) {
                targetPosition = duration;
            } else if (targetPosition < 0) {
                targetPosition = 0;
            }
            int progressSpeed = (int) ((float) (float) 120000 * percent);
            if (progressSpeed > 0) {
                if (targetPosition < duration) {
                    mController.showSpeed(progressSpeed);
                }
            } else if (progressSpeed < 0) {
                if (targetPosition > 0) {
                    mController.showBackward(Math.abs(progressSpeed));
                } else if (targetPosition == 0) {
                    mController.showBackward(0);
                }
            }
            mVideoPosition = (int) targetPosition;
            mVideoView.seekTo(mVideoPosition);
            setProgress();
            if (isControlPause) {
                mController.setState(State.PAUSED);
            } else {
                mController.setState(State.PLAYING);
            }
        }
    }

    @Override
    public void adjustVolume(float percent) {
        if (mVolume == -1) {
            mVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
        }
        float targetVolume = (float) mVolume + (float) mMaxVolume * percent;
        float targetPercent = targetVolume / mMaxVolume;
        /* SPRD: Fix bug612108 Volume value is less than 1 and greater than 0, all as a volume of 1 to deal with @{ */
        if (targetVolume > 0f && targetVolume < 1f) {
            targetVolume = 1f;
        } else if (targetVolume > 15f) {
            targetVolume = 15f;
        }

        /* SPRD: Fix bug613744 Plug in the headset Slide the screen to adjust the volume, to achieve the volume protection value after the pop-up system prompt box @{ */
        if (mAudioManager != null && targetPercent > VOLUME_SAFE_PERCENTAGE && mAudioManager.getSafeMediaVolumeEnabled() && mAudioManager.isWiredHeadsetOn()) {
            targetPercent = VOLUME_SAFE_PERCENTAGE;
        }
        /*@}*/

        if (targetPercent > 1f) {
            targetPercent = 1f;
        } else if (targetPercent < 0f) {
            targetPercent = 0f;
        }
        String actual_volume_per = String.valueOf(Math.round(targetPercent * 100));
        updateVolume((int) targetVolume);
        mController.showVolume(actual_volume_per);
        /*
         *SPRD:add for new feature 568552 change the state when finishing adjust volume @{
         */
        if (isControlPause) {
            mController.setState(State.PAUSED);
        } else {
            mController.setState(State.PLAYING);
        }
        /* Bug568552 end @} */
    }

    @Override
    public void adjustLightness(float percent) {
        /* SPRD:Add for bug597621 The lightness cannot be changed normally @{ */
        WindowManager.LayoutParams layoutParams = mMovieActivity.getWindow().getAttributes();
        getDefaultLightness(true);
        float targetLightness = mCurrentLightness + MAX_LIGHTNESS * percent;
        float targetPercent = targetLightness / MAX_LIGHTNESS;
        if (targetLightness > MAX_LIGHTNESS) {
            targetLightness = MAX_LIGHTNESS;
        } else if (targetLightness < 0) {
            targetLightness = 0;
        }
        if (targetPercent > 1.0f) {
            targetPercent = 1.0f;
        } else if (targetPercent < 0) {
            targetPercent = 0;
        }
        String lightnessPercent = String.valueOf(Math.round(targetPercent * 100));
        layoutParams.screenBrightness = targetLightness / MAX_LIGHTNESS;
        mLightness = targetLightness;
        mMovieActivity.getWindow().setAttributes(layoutParams);
        mController.showLightness(lightnessPercent);
        /* Bug597621 end @} */
        if (isControlPause) {
            mController.setState(State.PAUSED);
        } else {
            mController.setState(State.PLAYING);
        }
    }
    /*@}*/

    public void onSlideComplete() {
        Log.d(TAG, "onSlideComplete");
        // SPRD:Add for bug597621 Remember the lightness when back out the videoplayer
        rememberLightness();
        mVolume = -1;
        mCurrentLightness = -1.0f;
        mCurrentPosition = -1;
    }

    @Override
    public void endSliding() {
        onSlideComplete();
    }

    @Override
    public void lockControl() {
        if (!isLock) {
            isLock = true;
        } else {
            isLock = false;
        }
        mController.lockUnlockControl(isLock);
    }

    @Override
    public void lockOrintation() {
        if (!isOrientate) {
            isOrientate = true;
            /* SPRD: Modify for bug601532 In the interface of Video player, trun screen to right landscape ,Click the button of Orientation Lock,the screen rotate 180°@{
            mMovieActivity.setOrientation(orientation); @{ */
            mMovieActivity.setOrientation(ActivityInfo.SCREEN_ORIENTATION_LOCKED);
            /*Bug 601532 end@}*/
        } else {
            isOrientate = false;
            mMovieActivity.setOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR);
        }
        mController.lockUnlockOrintation(isOrientate);
    }

    @Override
    public void printScreen() {
        maybeStartHiding();
        /* SPRD:Bug 605744  Capture screen quickly and more than once , then landscape screen,the video  surface is wrong @{*/
        /* SPRD:Add for bug610475 CRASH: com.android.gallery3d @{ */
            new Thread(new Runnable() {
                @Override
                public void run() {
                    // TODO Auto-generated method stub
                    //SPRD: Bug 608430 when shoot screen maybe cause videoplayer crash@{
                    if (mMovieActivity != null && mUri != null && mVideoView != null) {
                        MediaMetadataRetriever rev = new MediaMetadataRetriever();
                        /*SPRD:Bug605710 When play a video in usb disk and  remove usb disk then click printscreen button,Gallery stoped@{*/
                        try {
                            rev.setDataSource(mMovieActivity, mUri);
                        }catch (IllegalArgumentException e) {
                            e.printStackTrace();
                            Message msg = new Message();
                            msg.what = MSG_CAPTURE_FAILED;
                            mCaptureHandler.sendMessage(msg);
                            return;
                        }
                        /*Bug 605710 end @}*/
                        int capTime = -1;
                        try {
                            capTime = mVideoView.getCurrentPosition() * 1000;
                        } catch (Exception e) {
                            e.printStackTrace();
                            Message msg = new Message();
                            msg.what = MSG_CAPTURE_FAILED;
                            mCaptureHandler.sendMessage(msg);
                            return;
                        }
                        Bitmap bitmap = rev.getFrameAtTime(capTime,
                                MediaMetadataRetriever.OPTION_CLOSEST_SYNC);
                        /*Bug 610475 end @} */
                        savePicture(bitmap);
                        Log.e(TAG, "bitmap:" + bitmap);
                    } else {
                        Message msg = new Message();
                        msg.what = MSG_CAPTURE_FAILED;
                        mCaptureHandler.sendMessage(msg);
                    }
                    //Bug 608430 end@}
                }
            }).start();
      /*Bug 605744 end @} */
    }

    public void savePicture(Bitmap bmp) {
        /* SPRD:Add for bug596396 Do screenshot when playing videos with no images,the gallery will crash @{ */
        Message msg = new Message();
        if (bmp == null) {
            /* SPRD:Bug 605744  Capture screen quickly and more than once , then landscape screen,the video  surface is wrong @{*/
           msg.what = MSG_CAPTURE_FAILED;
           mCaptureHandler.sendMessage(msg);
           /*Bug 605744 end@}*/
            return;
        }
        /* Bug596396 end @} */
        SimpleDateFormat simpleDateFormat = new SimpleDateFormat("yyyyMMdd_HHmmSS");
        String targetPath = "";
        String internalPath = EnvironmentEx.getInternalStoragePath().getAbsolutePath();
        Log.d(TAG, "internalPath=" + internalPath);
        targetPath = internalPath + "/DCIM/VideoShot";
        File file = new File(targetPath);
        try {
            if (!file.exists()) {
                file.mkdirs();
            }
            String fileName = targetPath + "/" + simpleDateFormat.format(new Date()) + ".png";
            FileOutputStream out = new FileOutputStream(fileName);
            bmp.compress(CompressFormat.PNG, 100, out);
            /* SPRD:Bug 605744  Capture screen quickly and more than once , then landscape screen,the video  surface is wrong @{*/
            if (null == msg) {
                msg = new Message();
            }
            msg.obj = targetPath;
            msg.what = MSG_CAPTURE_SUCCESS;
            mCaptureHandler.sendMessage(msg);
            /*Bug 605744 end@}*/
            /* SPRD: Add for bug589689 @{ */
            ArrayList<String> fileList = new ArrayList<String>();
            fileList.add(fileName);
            MediaScannerConnection.scanFile(mContext, (String[]) fileList.toArray(new String[] {}), null, null);
            /* Bug589689 end @} */
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void doubleClickPlayPause() {
        if (mMovieActivity.isCallingState()) {
            return;
        }
        if (mVideoView.isPlaying()) {
            if (misLiveStreamUri) {
                stopPlaybackInRunnable();
                mVideoView.setVisibility(View.INVISIBLE);
                mVideoPosition = 0;
                isStop = true;
                mController.resetTime();
                mController.timeBarEnable(false);
            } else {
                Log.d(TAG, "pauseVideo()");
                if (!misLiveStreamUri) {
                    isControlPause = true;
                    Log.d(TAG, "mVideoView.isPlay() " + mVideoView.isPlay());
                    if (mVideoView.isPlay() && mVideoView.canSeekForward() && mVideoView.canSeekBackward()) {
                        mVideoPosition = mVideoView.getCurrentPosition();
                    }
                    mHandler.removeCallbacksAndMessages(null);
                    mHandler.removeCallbacks(mProgressChecker);
                    mVideoView.pause();
                }
                mHasPaused = true;//SPRD:modify by old Bug468361
                mController.setState(State.PAUSED);
            }
        } else {
            /* SPRD:Add for bug609816 Wrong logic of the audiofocus control @{ */
            if (!requestAudioFocus()) {
                return;
            }
            /* Bug609816 end @} */
            if (VideoDrmUtils.getInstance().isDrmFile()) {
                Log.d(TAG, "VideoDrmUtils.getInstance().isDrmFile() return true");
                VideoDrmUtils.getInstance().checkRightBeforePlay(mMovieActivity, this);
            } else {
                 /*SPRD:Bug646879 Play a stream video and stop it ,when play it again ,it can't play @{*/
                if (misLiveStreamUri) {
                    mVideoView.setVideoURI(mUri);
                    mVideoView.setVisibility(View.VISIBLE);
                    isStop = false;
                    mController.showLoading();
                    mHandler.removeCallbacks(mPlayingChecker);
                    mHandler.postDelayed(mPlayingChecker, 250);
                } else {
                    isControlPause = false;
                    mVideoView.start();
                    // SPRD:Add for bug615788 output adjustment
                    backToPreOutputDevice();
                    mController.setState(State.PLAYING);
                    mHasPaused = false;//SPRD:modify by old Bug468361
                    setProgress();
                    isStop = false;
                    mVideoView.setVisibility(View.VISIBLE);
                    mHandler.post(mProgressChecker);
                    mHasPaused = false;
                }
                /*SPRD:Bug646879 end @}*/
            }
            /** @ } */
        }
    }

    @Override
    public void adjustVideoSize() {
        mController.maybeStartHiding();
        mMovieActivity.updateScreenState();
        boolean isFullScreen = mMovieActivity.getIsFullScreen();
        resize(isFullScreen);
        mController.setScaleButtonImage(isFullScreen);
    }

    public void setIfBookmark(boolean isBookmark) {
        mIsBookmark = isBookmark;
    }

    @Override
    public void setSystemUiVisibility(boolean isUiVisible) {
        showSystemUi(isUiVisible);
    }
    /* @} */

    /* SPRD:Add for bug601922 The scalebutton can't change normally @{ */
    public void setScaleButtonIcon(boolean isFullScreen){
        mController.setScaleButtonImage(isFullScreen);
    }
    /* Bug601922 end @} */

    /* SRPD:Add for Bug611215 Show notice when playing streaming videos or in camera @{ */
    @Override
    public boolean isDisableFloatWindow() {
        boolean isFloatWindowDisabled = mMovieActivity.getIntent().getBooleanExtra("disable-float-window", false);
        return isFloatWindowDisabled;
    }
    /* Bug611215 end @} */

    /* SPRD:Add for bug597621 The lightness cannot be changed normally @{ */
    private void rememberLightness() {
        SharedPreferences sharedPreferences = mContext.getSharedPreferences("Lightness", Activity.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedPreferences.edit();
        editor.putFloat("lightness", mLightness);
        editor.commit();
    }

    private void recoveryLightness() {
        getDefaultLightness(false);
        WindowManager.LayoutParams layoutParams = mMovieActivity.getWindow().getAttributes();
        layoutParams.screenBrightness = mCurrentLightness / MAX_LIGHTNESS;
        mMovieActivity.getWindow().setAttributes(layoutParams);
    }

    private void getDefaultLightness(boolean isInAdjusting) {
        try {
            if (isInAdjusting) {
                if (mCurrentLightness == -1.0f) {
                    WindowManager.LayoutParams layoutParams = mMovieActivity.getWindow().getAttributes();
                    if (layoutParams.screenBrightness == -1.0f) {
                        mCurrentLightness = Settings.System.getInt(mContext.getContentResolver(),
                                Settings.System.SCREEN_BRIGHTNESS);
                    } else {
                        mCurrentLightness = layoutParams.screenBrightness * MAX_LIGHTNESS;
                    }
                }
            } else {
                SharedPreferences sharedPreferences = mContext.getSharedPreferences("Lightness",
                        Activity.MODE_PRIVATE);
                mCurrentLightness = sharedPreferences.getFloat("lightness", -1.0f);
                if (mCurrentLightness == -1.0f) {
                    mCurrentLightness = Settings.System.getInt(mContext.getContentResolver(),
                            Settings.System.SCREEN_BRIGHTNESS);
                }
            }
        } catch (SettingNotFoundException e) {
            e.printStackTrace();
        }
    }
    /* Bug597621 end @} */

    /*SPRD:Bug612810  when goto vedio parameter page,it happens blink@{*/
    public void cancelHiding() {
        mController.cancelHiding();
    }

    public void maybeStartHiding() {
        mController.maybeStartHiding();
    }
    /*Bug612810 end @}*/

    /* SPRD:Add for bug609816 Wrong logic of the audiofocus control @{ */
    private boolean requestAudioFocus() {
        if (mContext != null && !mIsAudioFocusOwner) {
            mIsAudioFocusOwner = ((AudioManager) mContext.getSystemService(AUDIO_SERVICE))
                    .requestAudioFocus(mMovieActivity.afChangeListener, AudioManager.STREAM_MUSIC,
                            AudioManager.AUDIOFOCUS_GAIN_TRANSIENT) == AudioManager.AUDIOFOCUS_REQUEST_GRANTED;
        }
        return mIsAudioFocusOwner;
    }

    public void abandonAudioFocus() {
        if (mContext != null && mIsAudioFocusOwner) {
            ((AudioManager) mContext.getSystemService(AUDIO_SERVICE))
                    .abandonAudioFocus(mMovieActivity.afChangeListener);
            mIsAudioFocusOwner = false;
        }
    }

    public void setIsAudioFocusOwner(boolean isAudioFocusOwner) {
        mIsAudioFocusOwner = isAudioFocusOwner;
    }
    /* Bug609816 end @} */
    /*SPRD:Bug612810  when goto vedio parameter page,it happens blink@{*/
    protected void showStatusBar() {
        if (mMovieActivity != null) {
            mMovieActivity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN,
                    WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN);
            WindowManager.LayoutParams attr = mMovieActivity.getWindow().getAttributes();
            attr.flags &= (~WindowManager.LayoutParams.FLAG_FULLSCREEN);
            mMovieActivity.getWindow().setAttributes(attr);
            mMovieActivity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS, WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);
        }
    }
    /*Bug612810 end @}*/

    /* SPRD：Add for bug615788 When playing videos in speaker mode,the music player does the same @{ */
    private void backToPreOutputDevice() {
        if (mMovieActivity != null) {
            mMovieActivity.backToPreOutputDevice();
        }
    }
    /* Bug615788 end @} */
}

class Bookmarker {
    private static final String TAG = "Bookmarker";

    private static final String BOOKMARK_CACHE_FILE = "bookmark";
    private static final int BOOKMARK_CACHE_MAX_ENTRIES = 100;
    private static final int BOOKMARK_CACHE_MAX_BYTES = 10 * 1024;
    private static final int BOOKMARK_CACHE_VERSION = 1;
    private static final String DATABASE_TABLE = "content://com.sprd.gallery3d.app.VideoBookmarkProvider/bookmarks";

    private static final int HALF_MINUTE = 30 * 1000;
    private static final int TWO_MINUTES = 4 * HALF_MINUTE;

    private final Context mContext;

    public Bookmarker(Context context) {
        mContext = context;
    }

    public void setBookmark(Uri uri, int bookmark, int duration) {
        try {
            BlobCache cache = CacheManager.getCache(mContext,
                    BOOKMARK_CACHE_FILE, BOOKMARK_CACHE_MAX_ENTRIES,
                    BOOKMARK_CACHE_MAX_BYTES, BOOKMARK_CACHE_VERSION);
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            DataOutputStream dos = new DataOutputStream(bos);
            dos.writeUTF(uri.toString());
            dos.writeInt(bookmark);
            dos.writeInt(duration);
            dos.flush();
            cache.insert(uri.hashCode(), bos.toByteArray());
            /*
             * SPRD:add for new feature 568552 save video play records in database @{
             */
            ContentValues values = new ContentValues();
            if (uri.getLastPathSegment() != null) {
                values.put("title", uri.getLastPathSegment());
            } else {
                values.put("title", "unkown video");
            }
            /* SPRD:Add for bug605100 There are too many same records in videoplayer @{ */
            values.put("bookmark", String.valueOf(bookmark));
            values.put("uri", uri.toString());
            values.put("duration", String.valueOf(duration));
            values.put("date", String.valueOf(new Date()));
            if (VideoUtil.isUriExistedInDB(uri,mContext)) {
                int count = mContext.getContentResolver().delete(Uri.parse(
                        DATABASE_TABLE),"uri"+"=?",new String[]{uri.toString()});
                Log.d(TAG,"Delete "+count+" rows sucessfully!");
            }
            Uri _uri = mContext.getContentResolver().insert(Uri.parse(
                        DATABASE_TABLE), values);
            Log.d(TAG, "_uri=" + _uri);
            /* Bug605100 end @} */
            Log.d(TAG, "save successfully！");
        } catch (Throwable t) {
            Log.w(TAG, "setBookmark failed", t);
        }
        /* Bug568552 @} */
    }

    public Integer getBookmark(Uri uri) {
        try {
            BlobCache cache = CacheManager.getCache(mContext,
                    BOOKMARK_CACHE_FILE, BOOKMARK_CACHE_MAX_ENTRIES,
                    BOOKMARK_CACHE_MAX_BYTES, BOOKMARK_CACHE_VERSION);

            byte[] data = cache.lookup(uri.hashCode());
            if (data == null)
                return null;

            DataInputStream dis = new DataInputStream(
                    new ByteArrayInputStream(data));

            String uriString = DataInputStream.readUTF(dis);
            int bookmark = dis.readInt();
            int duration = dis.readInt();
            if (!uriString.equals(uri.toString())) {
                return null;
            }
            Log.d(TAG, "bookamrk is " + bookmark / 1000);
            Log.d(TAG, "duration is  " + duration / 1000);
            if ((bookmark < HALF_MINUTE) || (duration < TWO_MINUTES)
                    || (bookmark > (duration - HALF_MINUTE))) {
                return null;
            }
            return Integer.valueOf(bookmark);
        } catch (Throwable t) {
            Log.w(TAG, "getBookmark failed", t);
        }
        return null;
    }
}
