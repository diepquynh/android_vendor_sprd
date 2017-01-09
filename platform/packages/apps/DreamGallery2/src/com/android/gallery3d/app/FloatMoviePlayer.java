
package com.android.gallery3d.app;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.Service;
import android.app.KeyguardManager;
import android.content.AsyncQueryHandler;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.database.Cursor;
import android.database.CursorIndexOutOfBoundsException;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.media.AudioManager;
import android.media.Metadata;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.media.MediaPlayer;
import android.media.MediaPlayerEx;
import android.media.MediaPlayer.OnBufferingUpdateListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.provider.MediaStore;
import android.provider.OpenableColumns;
import android.telephony.TelephonyManager;
import android.util.Log;

import android.view.Display;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnTouchListener;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import com.android.gallery3d.R;
import com.android.gallery3d.common.Utils;
import com.android.gallery3d.ui.FloatFrameLayout;
import com.android.gallery3d.ui.FloatFrameLayout.OnConfigurationChangedListener;
import com.android.gallery3d.util.GalleryUtils;

import com.sprd.gallery3d.app.NewVideoActivity;
import com.sprd.gallery3d.app.VideoItems;
import com.sprd.gallery3d.app.VideoUtil;

import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;

public class FloatMoviePlayer implements OnBufferingUpdateListener,
        OnCompletionListener, MediaPlayer.OnPreparedListener, OnTouchListener,
        SurfaceHolder.Callback, OnClickListener,MediaPlayer.OnErrorListener {
    public static final String TAG = "FloatMoviePlayer";
    public static final int STATE_ERROR = -1;
    public static final int STATE_IDLE = 0;
    public static final int STATE_PREPARING = 1;
    public static final int STATE_PREPARED = 2;
    public static final int STATE_PLAYING = 3;
    public static final int STATE_PAUSED = 4;
    public static final int STATE_PLAYBACK_COMPLETED = 5;

    //SPRD: fix the bug 531300
    /**
     * the minimum value with player window's width in portrait orientation;
     */
    private static final int MIN_WIDTH_IN_PORTRAIT = 216;
    /**
     * the maximum value with player window's height in landscape orientation;
     */
    private static final int MIN_HEIGHT_IN_LANDSCAPE = 280;
    private static final float MIN_SCALE = 0.4f;
    private static final float MAX_SCALE = 1.0f;

    /** modify by old Bug493063 @{ */
    private int currentPlayBackMode = 0;
    public final int SINGLE_PLAYBACK = 1;
    public final int ALL_PLAYBACK = 2;
    private Dialog alertDialog;//SPRD:modify by old Bug493063
    /**@}*/

    private int mVideoWidth;
    private int mVideoHeight;
    private int mSurfaceWidth;
    private int mSurfaceHeight;
    private int mWindowWidth;
    private int mWindowHeight;
    private MediaPlayerEx mMediaPlayer;
    private SurfaceHolder mSurfaceHolder;
    private SeekBar skbProgress;

    private float mTouchStartX;
    private float mTouchStartY;
    private float mLastX;
    private float mLastY;
    private float mX;
    private float mY;

    private WindowManager mWm;
    private WindowManager.LayoutParams mWmParams;
    private Context mContext;
    private int mStatusBarHeight;
    private long mLastTouchDownTime;
    private long mCurTouchDownTime;
    public FloatFrameLayout mFloatLayout;
    private ImageView mPausePlayBtn;
    private ImageView mBackToNormalBtn;
    private ImageView mCloseWindowBtn;
    private TextView mVideoLoadingTextView;
    private boolean mHideTitle;

    private Uri mUri;
    private ScaleGestureDetector mScaleGestureDetector;
    private int mPosition;
    private int mSeekPositionWhenPrepared;
    private int mCurrentState;
    private int mTargetState;
    private int mState;

    private final Handler mHandler;
    private final Runnable startHidingRunnable;
    private AudioManager mAm;

    private Display mDisplay;

    private Intent mDataIntent;
    private LinkedList<MovieInfo> mPlayList;
    private int curVideoIndex;
    private Uri videoListUri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
    private SurfaceView mSurfaceView;
    private String mTitle = "";
    private boolean mIsTitleShort = false;// bug 355537
    private boolean mCanSeekBack;
    private boolean mCanSeekForward;
    private float mScale = (float) 2 / 3;
    private boolean isPhoneCallActivityTop = false;
    private String scheme;//SPRD:modify by old Bug515022
    private static final String FLAG_GALLERY="startByGallery";
    private String mAlbum;
    private boolean mIsStartByGallery;
    private boolean mIsScreenOff;// SPRD:Add for bug575931 FloatMoviePlayer start playing when the screen is off
    /**SPRD:Bug571356 the float window is not pause, when change to an other video @{*/
    private TelephonyManager mTelephonyManager;
    private SubscriptionManager mSubscriptionManager;
    /**@}*/
    private boolean mIsLayoutAdded = true; // SPRD: Modify for bug578015 System crashed at com.android.gallery3d
    public String mFragmentName;
    private String mOtgDevicePath;
    private static final String ALL_VIDEOS_FRAGMENT = "AllVideosFragment";
    private static final String LOCAL_VIDEOS_FRAGMENT = "LocalVideosFragment";
    private static final String FILMED_VIDEOS_FRAGMENT = "FilmedVideosFragment";
    private static final String HISTORY_VIDEOS_FRAGMENT = "HistoryVideosFragment";
    private static final String OTG_VIDEOS_FRAGMENT = "OtgVideosFragment";
    private boolean mIsAbandonAudiofocus = false;
    private boolean mFirstError = false;
    private int mErrorPosition = -1;
    private int mErrorTimes = 0;

    OnAudioFocusChangeListener afChangeListener = new OnAudioFocusChangeListener() {
        public void onAudioFocusChange(int focusChange) {
            Log.d(TAG, "focusChange:" + focusChange);
            switch (focusChange) {
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                    break;
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                case AudioManager.AUDIOFOCUS_LOSS:
                    /* SPRD:Add for bug613664 When changing from float window to normal play,the musicplayer will play @{ */
                    if (mIsAbandonAudiofocus) {
                        ((Service) mContext).stopSelf();
                    }
                    /* Bug613664 end @} */
                    mState = mCurrentState;
                    if (mCurrentState == STATE_PLAYING) {
                        pause();
                    }
                    break;
                case AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK:
                case AudioManager.AUDIOFOCUS_GAIN_TRANSIENT:
                case AudioManager.AUDIOFOCUS_GAIN:
                    KeyguardManager keyguardManager = (KeyguardManager) mContext
                            .getSystemService(Service.KEYGUARD_SERVICE);
                    // SPRD:Add for bug575931 616302 The line blow about audiofocus
                    if (!keyguardManager.isKeyguardLocked() && !mIsScreenOff) {
                        if (mCurrentState != STATE_PLAYING && mState == STATE_PLAYING && mMediaPlayer != null) {
                            start();
                            hiddenBtn();
                            mState = 0;
                        }
                    }
                    break;

                default:
                    break;
            }
        }
    };

    public void setState(int state){
        mState = state;
    }
    public FloatMoviePlayer(Context context) {
        mContext = context;
        mWm = (WindowManager) context.getApplicationContext()
                .getSystemService(Context.WINDOW_SERVICE);
        mStatusBarHeight = getStatusBarHeight();

        mDisplay = ((WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE))
                .getDefaultDisplay();

        mScaleGestureDetector = new ScaleGestureDetector(context, new ScaleGestureListener());

        mHandler = new Handler() {
            public void handleMessage(Message msg) {

                int position = mMediaPlayer.getCurrentPosition();
                int duration = mMediaPlayer.getDuration();

                if (duration > 0) {
                    long pos = skbProgress.getMax() * position / duration;
                    skbProgress.setProgress((int) pos);
                }
            };
        };
        startHidingRunnable = new Runnable() {
            public void run() {
                hiddenBtn();
            }
        };
        mMediaPlayer = new MediaPlayerEx();
        mMediaPlayer.setOnErrorListener(this);
        mMediaPlayer.setWakeMode(mContext, PowerManager.FULL_WAKE_LOCK);
        Log.d(TAG, "new FloatMoviePlayer");
    }
    /** modify by old Bug493063 @{ */
    public void errorDialogCheckAndDismiss() {
        if (alertDialog != null && alertDialog.isShowing()) {
            Log.d(TAG, "oNError alertDialog");
            alertDialog.dismiss();
        }
    }
    public void alertVideoError(int messageId) {
        Log.d(TAG, "messageId=" + messageId);
        if (currentPlayBackMode == ALL_PLAYBACK) {
            alertDialog = new AlertDialog.Builder(mContext)
                    .setMessage(messageId)
                    .setPositiveButton(com.android.internal.R.string.VideoView_error_button,
                            new android.content.DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    nextVideo();
                                }
                            }).setCancelable(false)
                    .create();
            alertDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            if (alertDialog.isShowing()) {
                alertDialog.dismiss();
            }
            alertDialog.show();
        } else {
            alertDialog = new AlertDialog.Builder(mContext)
                    .setMessage(messageId)
                    .setPositiveButton(com.android.internal.R.string.VideoView_error_button,
                            new android.content.DialogInterface.OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    closeWindow();
                                }
                            }).setCancelable(false)
                    .create();
            alertDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
            if (alertDialog.isShowing()) {
                alertDialog.dismiss();
            }
            alertDialog.show();
        }
    }
    private void allPlayBack() {
        /** modify by old Bug503434 @{ */
       // nextVideo();
        Log.d(TAG, "mPlayList.size() "+mPlayList.size());
        // SPRD: Bug568552, temp modify for AndroidN porting @{
        // /**SPRD:Bug474646 Add Drm feature @{ @{ */
        // if (VideoDrmUtils.getInstance().isDrmFile()) {
        //    mMediaPlayer.setNeedToConsume(true);
        // }
        // /**@}*/
        // @}
        int n = mPlayList.size();
        if (n == 0) {
            return;
        }
        if (++curVideoIndex >= n) {
            curVideoIndex = 0;
        }
        Log.d(TAG,"curVideoIndex "+curVideoIndex);
        isFileExistCheck("allPlayBack");//SPRD:Bug527349 add
        changeVideo();
        /**@}*/
    }
    private void singlePlayBack() {
        /** modify by old Bug503434 @{ */
       // start();
        // SPRD: Bug568552, temp modify for AndroidN porting @{
        // /**SPRD:Bug474646 Add Drm feature @{ @{ */
        // if (VideoDrmUtils.getInstance().isDrmFile()) {
        //     mMediaPlayer.setNeedToConsume(true);
        // }
        // /**@}*/
        // @}
        int n = mPlayList.size();
        if (n == 0) {
            return;
        }
        /** SPRD:Bug527349 the gallery is crashed then click the next video @{ */
        boolean exist=checkVideoExitOrNot();
        if(!exist){
            closeWindow();
            Toast.makeText(mContext, R.string.video_file_does_not_exist, Toast.LENGTH_SHORT).show( );
        }
        /**@}*/
        changeVideo();
        /**@}*/
    }
    /**@}*/
    public void seekTo(int position) {
        if (isInPlaybackState()) {
            mMediaPlayer.seekTo(position);
            mSeekPositionWhenPrepared = 0;
        } else {
            mSeekPositionWhenPrepared = position;
        }
        mPosition = position;
    }

    private boolean isInPlaybackState() {
        Log.d(TAG, "isInPlaybackState mCurrentState " + mCurrentState);
        return (mMediaPlayer != null &&
                mCurrentState != STATE_ERROR &&
                mCurrentState != STATE_IDLE && mCurrentState != STATE_PREPARING);
    }

    public void setPhoneState(boolean phoneState) {
        isPhoneCallActivityTop = phoneState;
    }

    public boolean getPhoneState(){
        return isPhoneCallActivityTop;
    }

    public void start() {
        Log.d(TAG, "start");
        /* SPRD: Modify for bug578015 System crashed at com.android.gallery3d @{ */
        if (!mIsLayoutAdded) {
            return;
        }
        /* @} */
        /** SPRD:Bug531341 click the play button,the player is not work again @{ */
        if (mMediaPlayer == null) {
            openVideoIfNeed();
        }
        /**@}*/
        if (isInPlaybackState()) {
            mMediaPlayer.start();
            setCurrentState(STATE_PLAYING);
            /*SPRD:bug614698 When play all back ,the dialog is show all the time @{*/
            if (mMediaPlayer.isPlaying()) {
                mFirstError = false;
                mErrorPosition = -1;
                mErrorTimes = 0;
            }
            /*end @}*/
            updateBtnState();
        }
        showBtn();
        mTargetState = STATE_PLAYING;
        if (!mAm.isAudioFocusExclusive()) {
            mAm.requestAudioFocus(
                    afChangeListener, AudioManager.STREAM_MUSIC,
                    AudioManager.AUDIOFOCUS_GAIN);
        }
    }

    /**SPRD:Bug531341 click the play button,the player is not work again @{ */
    public void openVideoIfNeed() {
        if (mUri == null) {
            Log.w(TAG, "openVideo mUri or mSurfaceHolder is null");
            return;
        }
        try {
            if (mMediaPlayer == null) {
                mMediaPlayer = new MediaPlayerEx();
                mMediaPlayer.setDisplay(mSurfaceHolder);
                mMediaPlayer.setOnErrorListener(this);
                mMediaPlayer.setWakeMode(mContext, PowerManager.FULL_WAKE_LOCK);
            }
            mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mMediaPlayer.setOnBufferingUpdateListener(this);
            mMediaPlayer.setOnPreparedListener(this);
            mMediaPlayer.setOnCompletionListener(this);
            mMediaPlayer.reset();
            mMediaPlayer.setDataSource(mContext, mUri);
            mMediaPlayer.prepareAsync();
            setCurrentState(STATE_PREPARING);
            mSeekPositionWhenPrepared = 0;
            Log.d(TAG, "setVideoUri end");
            String uriString = mUri.toString();
            if (uriString.startsWith("https://") || uriString.startsWith("http://")
                    || uriString.startsWith("rtsp://")) {
                // mSurfaceView.destroyDrawingCache();
                mSurfaceView.refreshDrawableState();
                mPausePlayBtn.setVisibility(View.INVISIBLE);
                mVideoLoadingTextView.setVisibility(View.VISIBLE);
            }
        } catch (Exception e) {
            Log.e(TAG, "error", e);
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            /*
             * SPRD:add for bug554216
             * when playing the video attachment in float window in single play mode,clear the recent task,the float window cannot play normally @{
            */
            onError(mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNKNOWN, 0);
            /*@}*/
        }
    }
    /**@}*/

    public void initTitle() {
        // if(mPlayList == null){
        // return;
        // }
        // MovieInfo movieInfo = mPlayList.get(curVideoIndex);
        // String path = movieInfo.mPath;
        // sprd:change for bug 361907
        getVideoName(mDataIntent);
        String uriString = mUri.toString();
        if (uriString.startsWith("http://") || uriString.startsWith("https://")
                || uriString.startsWith("rtsp://")) {
            mTitle = "";
        }
        Log.e(TAG, "mike  initTitle  mTitle = " + mTitle);
    }

    public void setVideoUri(Uri videoUri) {
        try {
            mUri = videoUri;
            openVideo();
            if (mMediaPlayer != null) {
                mMediaPlayer.reset();
                mMediaPlayer.setDataSource(mContext, videoUri);
                mMediaPlayer.prepareAsync();
                setCurrentState(STATE_PREPARING);
                mSeekPositionWhenPrepared = 0;
                Log.d(TAG, "setVideoUri end");
                String uriString = mUri.toString();
                if (uriString.startsWith("https://") || uriString.startsWith("http://")
                        || uriString.startsWith("rtsp://")) {
                    // mSurfaceView.destroyDrawingCache();
                    mSurfaceView.refreshDrawableState();
                    mPausePlayBtn.setVisibility(View.INVISIBLE);
                    mVideoLoadingTextView.setVisibility(View.VISIBLE);
                }
            }
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (IllegalStateException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
       //add for bug532214  java.lang.NullPointerException
        } catch(Exception ex){
            Log.w(TAG, "Unable to open the video" + mUri, ex);
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            /*SPRD:add for bug554216
             * when playing the video attachment in float window in single play mode,clear the recent task,the float window cannot play normally @{
            */
            onError(mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNKNOWN, 0);
            /*@}*/
        }
    }

    public void pause() {
        if (isInPlaybackState()) {
            mPosition = mMediaPlayer.getCurrentPosition();
            mMediaPlayer.pause();
            setCurrentState(STATE_PAUSED);
            updateBtnState();
            showBtn();
        }
        mTargetState = STATE_PAUSED;
    }

    public void stop() {
        if (mMediaPlayer != null) {
            mMediaPlayer.stop();
            mMediaPlayer.release();
            mMediaPlayer = null;
            setCurrentState(STATE_IDLE);
            mTargetState = STATE_IDLE;
        }
    }

    public void openVideo() {
        Log.d(TAG, "openVideo()");
        if (mUri == null) {
            Log.w(TAG, "openVideo mUri or mSurfaceHolder is null");
            return;
        }
        try {
            if (mMediaPlayer == null) {
                mMediaPlayer = new MediaPlayerEx();
                /**SPRD:Bug531341 click the play button,the player is not work again @{ */
                mMediaPlayer.setDisplay(mSurfaceHolder);
                mMediaPlayer.setOnErrorListener(this);
                mMediaPlayer.setWakeMode(mContext, PowerManager.FULL_WAKE_LOCK);
                /**@}*/
            }
            // mMediaPlayer.setDisplay(mSurfaceHolder);
            mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mMediaPlayer.setOnBufferingUpdateListener(this);
            mMediaPlayer.setOnPreparedListener(this);
            mMediaPlayer.setOnCompletionListener(this);
        } catch (Exception e) {
            Log.e(TAG, "error", e);
        }

    }

    public void release() {
        MediaPlayer tempPlayer;
        tempPlayer = mMediaPlayer;
        mMediaPlayer = null;
        if (tempPlayer != null) {
            tempPlayer.reset();
            tempPlayer.release();
            tempPlayer = null;
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder arg0, int arg1, int width, int height) {
        Log.d(TAG, "surface changed" + arg0 + " " + arg1 + " width=" + width + " height=" + height);
        mSurfaceWidth = width;
        mSurfaceHeight = height;
        if (mSeekPositionWhenPrepared > 0) {
            seekTo(mSeekPositionWhenPrepared);
        }
        if (mTargetState == STATE_PLAYING && mCurrentState != STATE_PLAYING) {
            start();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "surface created");
        mSurfaceHolder = holder;
        // openVideo();
        if (mMediaPlayer != null) {
            mMediaPlayer.setDisplay(mSurfaceHolder);
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder arg0) {
        Log.d(TAG, "surface destroyed");
        release();
    }

    @Override
    public void onPrepared(MediaPlayer player) {
        setCurrentState(STATE_PREPARED);
        Metadata data = player.getMetadata(MediaPlayer.METADATA_ALL,
                MediaPlayer.BYPASS_METADATA_FILTER);
        if (data != null) {
            mCanSeekBack = !data.has(Metadata.SEEK_BACKWARD_AVAILABLE)
                    || data.getBoolean(Metadata.SEEK_BACKWARD_AVAILABLE);
            mCanSeekForward = !data.has(Metadata.SEEK_FORWARD_AVAILABLE)
                    || data.getBoolean(Metadata.SEEK_FORWARD_AVAILABLE);
        }
        mVideoWidth = mMediaPlayer.getVideoWidth();
        mVideoHeight = mMediaPlayer.getVideoHeight();
        Log.d(TAG, "onPrepared   mUri = " + mUri);
        String uriString = mUri.toString();
        // if(uriString.startsWith("http://") || uriString.startsWith("https://") ||
        // uriString.startsWith("rtsp://")){
        if (mVideoHeight == 0 || mVideoWidth == 0) {
            mSurfaceView.setBackgroundColor(Color.BLACK);
            mVideoWidth = 480;
            mVideoHeight = 320;
        } else {
            mSurfaceView.setBackgroundColor(Color.TRANSPARENT);
        }
        // }
        if (mVideoHeight != 0 && mVideoWidth != 0) {
            if (mSeekPositionWhenPrepared > 0) {
                seekTo(mSeekPositionWhenPrepared);
            }
            Log.d(TAG, "onPrepared mTargetState=" + mTargetState + " mCurrentState="
                    + mCurrentState);
            if (mTargetState == STATE_PLAYING && mCurrentState != STATE_PLAYING) {
                start();
            }
        }
        // preparedLocation();
        reSize();
        // bug 357447 end
        Log.d(TAG, "onPrepared mVideoWidth=" + mVideoWidth + " mVideoHeight=" + mVideoHeight);
        if (uriString.startsWith("https://") || uriString.startsWith("http://")
                || uriString.startsWith("rtsp://")) {
            mPausePlayBtn.setVisibility(View.VISIBLE);
            mVideoLoadingTextView.setVisibility(View.INVISIBLE);
        }
        /**SPRD:Bug571356 the float window is not pause, when change to an other video @{*/
        if (isCallingState()) {
            pause();
        }
        /**@}*/
    }

    /* SPRD: add uselessness method
    // bug 357447 begin
    private void preparedLocation() {
        mWindowWidth = mDisplay.getWidth();
        mWindowHeight = mDisplay.getHeight();
        int targetWidth = mVideoWidth;
        int targetHeight = mVideoHeight;
        float widRate = (float) mWindowWidth / mVideoWidth;
        float heiRate = (float) mWindowHeight / mVideoHeight;
        float videoRate = (float) mVideoHeight / mVideoWidth;
        if (widRate > heiRate) {
            targetHeight = (int) (mWindowHeight * mScale);
            if (targetHeight < 216) {
                targetHeight = 216;
            } else if (targetHeight > mWindowHeight) {
                targetHeight = mWindowHeight;
            }
            targetWidth = (int) (targetHeight / videoRate);
        } else if (widRate <= heiRate) {
            targetWidth = (int) (mWindowWidth * mScale);
            if (targetWidth < 216) {
                targetWidth = 216;
            } else if (targetWidth > mWindowWidth) {
                targetWidth = mWindowWidth;
            }
            targetHeight = (int) (targetWidth * videoRate);
        }
        mWmParams.x = (int) ((mWindowWidth - targetWidth) / 2);
        mWmParams.y = (int) ((mWindowHeight - targetHeight) / 2);
    }

    // bug 357447 end   */
    @Override
    public void onCompletion(MediaPlayer arg0) {
        /** modify by old Bug515022 @{ */
        Log.d(TAG, "mUri=" + mUri);
        scheme = ( (mUri == null)? null : mUri.getScheme() );
        if (mPlayList != null){
            if (currentPlayBackMode == SINGLE_PLAYBACK) {
                Log.d(TAG, "single playback mode");
                singlePlayBack();
            }else if(currentPlayBackMode == ALL_PLAYBACK){
                Log.d(TAG, "all playback mode");
                allPlayBack();
            }else{
             // the video is over normally
                onCompletion();
            }
        }else if("file".equalsIgnoreCase(scheme) ||
                "content".equalsIgnoreCase(scheme)){
            if(currentPlayBackMode == SINGLE_PLAYBACK || currentPlayBackMode == ALL_PLAYBACK){
                setVideoUri(mUri);
                seekTo(0);
                start();
            }else{
             // the video is over normally
                onCompletion();
            }
        }else{
            // the video is over normally
            onCompletion();
        }
        /**  @}{ */
    }
    /** modify by old Bug515022 @{ */
    public void onCompletion(){
        Log.d(TAG, "onCompletion");
        // SPRD: Bug568552, temp modify for AndroidN porting @{
        // /**SPRD:Bug474646 Add Drm feature @{ @{ */
        // if (VideoDrmUtils.getInstance().isDrmFile()) {
        //     mMediaPlayer.setNeedToConsume(true);
        // }
        // /**@}*/
        // @}
        release();//SPRD:Bug531341 add
        setCurrentState(STATE_PLAYBACK_COMPLETED);
        mTargetState = STATE_PLAYBACK_COMPLETED;
        updateBtnState();
        mPosition = -1;
    }
    /**  @}{ */
    @Override
    public void onBufferingUpdate(MediaPlayer arg0, int bufferingProgress) {
        if (skbProgress == null) {
            return;
        }
        skbProgress.setSecondaryProgress(bufferingProgress);
        int currentProgress = skbProgress.getMax() * mMediaPlayer.getCurrentPosition()
                / mMediaPlayer.getDuration();
        Log.d(currentProgress + "% play", bufferingProgress + "% buffer");

    }

    public int getCurrentState() {
        return mCurrentState;
    }

    public void setCurrentState(int state) {
        Log.d(TAG, "setCurrentState " + state);
        mCurrentState = state;
    }

    public void updateBtnState() {
        if (mCurrentState == STATE_PLAYING) {
            mPausePlayBtn.setImageResource(R.drawable.new_float_pause);
        } else {
            mPausePlayBtn.setImageResource(R.drawable.new_float_play);
        }
    }

    public void addToWindow() {
        Log.d(TAG, "addToWindow");
        mWm = (WindowManager) mContext.getApplicationContext().getSystemService(
                Context.WINDOW_SERVICE);
        mWmParams = new WindowManager.LayoutParams();

        mWmParams.type = LayoutParams.TYPE_PHONE; // set window type
        mWmParams.format = PixelFormat.RGBA_8888;

        // set Window flag
        mWmParams.flags = LayoutParams.FLAG_NOT_TOUCH_MODAL
                | LayoutParams.FLAG_NOT_FOCUSABLE;

        mWmParams.gravity = Gravity.LEFT | Gravity.TOP;

        mWmParams.x = 0;
        mWmParams.y = 0;

        // set window size
        /**SPRD:modify by old Bug515672 @{*/
        //mWindowWidth = mDisplay.getWidth();
        //mWindowHeight = mDisplay.getHeight();
        mWindowWidth=0;
        mWindowHeight=0;
        /**@}*/
        mWmParams.width = mWindowWidth;
        mWmParams.height = mWindowHeight;

        LayoutInflater inflater = LayoutInflater.from(mContext);
        mFloatLayout = (FloatFrameLayout) inflater.inflate(R.layout.float_movie_view, null);

        mSurfaceView = (SurfaceView) mFloatLayout.findViewById(R.id.float_surfaceview);
        mSurfaceHolder = mSurfaceView.getHolder();
        mSurfaceHolder.addCallback(this);
        mSurfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        mFloatLayout.setOnTouchListener(this);
        mFloatLayout.setOnConfigurationChangedListener(new OnConfigurationChangedListener() {
            @Override
            public boolean onConfigurationChanged(Configuration newConfig) {
                Log.d(TAG,"onConfigurationChanged");
                // preparedLocation();
                reSize();
                return false;
            }
        });

        mPausePlayBtn = (ImageView) mFloatLayout.findViewById(R.id.btn_pause);
        mPausePlayBtn.setOnClickListener(this);
        mBackToNormalBtn = (ImageView) mFloatLayout.findViewById(R.id.btn_back_to_normal);
        mBackToNormalBtn.setOnClickListener(this);
        mCloseWindowBtn = (ImageView) mFloatLayout.findViewById(R.id.btn_close_win);
        mCloseWindowBtn.setOnClickListener(this);
        mVideoLoadingTextView = (TextView) mFloatLayout.findViewById(R.id.video_load);
        /* SPRD: Modify for bug578015 System crashed at com.android.gallery3d @{ */
        mAm = ((AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE));
        mAm.requestAudioFocus(
                afChangeListener, AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN);
        try {
            mWm.addView(mFloatLayout, mWmParams);
        } catch (SecurityException e) {
            mIsLayoutAdded = false;
            ((Service) mContext).stopSelf();
            return;
        }
        /* Bug578015 End @} */
    }

    public void removeFromWindow() {
        Log.d(TAG, "removeFromWindow");
        if (mFloatLayout != null) {
            // SPRD: Modify for bug578015 System crashed at com.android.gallery3d
            if (mIsLayoutAdded) {
                // SPRD: Fix bug 615151 Using the removeViewImmediate method, we call View.onDetachWindow () to remove the view
                mWm.removeViewImmediate(mFloatLayout);
            }
            mFloatLayout = null;
        }
        /* SPRD:Add for bug613664 When changing from float window to normal play,the musicplayer will play @{ */
        if (mIsAbandonAudiofocus) {
            mAm.abandonAudioFocus(afChangeListener);
        }
        /* Bug613664 end @} */
    }

    public void showBtn() {
        Log.d(TAG, "showBtn");
        Boolean state = mPlayList == null ? true : false;
        Log.d(TAG, "mPlayList  is " + state);
        int visibility = mVideoLoadingTextView.getVisibility();
        if (visibility == View.INVISIBLE || visibility == View.GONE) {
            mPausePlayBtn.setVisibility(View.VISIBLE);
        } else {
            mPausePlayBtn.setVisibility(View.INVISIBLE);
        }
        mBackToNormalBtn.setVisibility(View.VISIBLE);
        mCloseWindowBtn.setVisibility(View.VISIBLE);
        String uriString = mUri.toString();
        if (uriString.startsWith("https://") || uriString.startsWith("http://")
                || uriString.startsWith("rtsp://")) {
        }
        if (mCurrentState == STATE_PLAYBACK_COMPLETED && mFloatLayout != null) {
            mWm.updateViewLayout(mFloatLayout, mWmParams);
        }
        mHandler.removeCallbacks(startHidingRunnable);
        if (mCurrentState == STATE_PLAYING || mCurrentState == STATE_PREPARED) {
            mHandler.postDelayed(startHidingRunnable, 7000);
        }
    }

    public void hiddenBtn() {
        Log.d(TAG, "hiddenBtn");
        mPausePlayBtn.setVisibility(View.INVISIBLE);
        mBackToNormalBtn.setVisibility(View.INVISIBLE);
        mCloseWindowBtn.setVisibility(View.INVISIBLE);
    }

    public void toggleBtn() {
        if (mPausePlayBtn.getVisibility() == View.VISIBLE) {
            hiddenBtn();
        } else {
            showBtn();
        }
    }

    public boolean handleTouchEvent(MotionEvent event) {
        mLastX = mX;
        mLastY = mY;
        // get coordinate on window
        mX = event.getRawX();
        mY = event.getRawY() - mStatusBarHeight;

        // Log.d(TAG, "x=" + x + " y=" + y);
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                // get coordinate on view
                mTouchStartX = event.getX();
                mTouchStartY = event.getY();

                mLastTouchDownTime = mCurTouchDownTime;
                mCurTouchDownTime = System.currentTimeMillis();
                if (mCurTouchDownTime - mLastTouchDownTime < 400) {
                    // DoubleTap
                    // goToNormalPlay();
                    mCurTouchDownTime = 0;
                }
                break;
            case MotionEvent.ACTION_MOVE:
                if (mTouchStartX != 0) {
                    if (Math.abs(mLastY - mY) > 10 || Math.abs(mLastX - mX) > 10) {
                        updateViewPosition();
                        showBtn();
                    }
                }
                break;

            case MotionEvent.ACTION_UP:
                mTouchStartX = mTouchStartY = 0;

                if (mCurTouchDownTime - mLastTouchDownTime > 400
                        && System.currentTimeMillis() - mCurTouchDownTime < 500) {
                    toggleBtn();
                }
                break;
        }
        return true;
    }

    private void updateViewPosition() {
        if (mFloatLayout != null) {
            mWmParams.x = (int) (mX - mTouchStartX);
            mWmParams.y = (int) (mY - mTouchStartY);
            mWm.updateViewLayout(mFloatLayout, mWmParams);
        }
    }

    private void goToNormalPlay() {
        int state = mCurrentState == STATE_PLAYING ? STATE_PLAYING : STATE_PAUSED;
        if (mCurrentState == STATE_PLAYING) {
            mPosition = mMediaPlayer.getCurrentPosition();
        }
        removeFromWindow();
        Intent intent;
        if (mCurrentState == STATE_PLAYBACK_COMPLETED) {
            intent = new Intent(Intent.ACTION_MAIN);
            intent.setClass(mContext, NewVideoActivity.class);
            intent.addCategory(Intent.CATEGORY_LAUNCHER);
        } else {
            intent = new Intent(Intent.ACTION_VIEW);
            intent.putExtras(mDataIntent);
            intent.setClassName("com.android.gallery3d", "com.sprd.gallery3d.app.Video");
            intent.setData(mUri);
            if (mCanSeekBack && mCanSeekForward) {
                intent.putExtra("position", mPosition);
            } else {
                intent.putExtra("position", 0);
            }
            intent.putExtra("currentstate", state);
            intent.putExtra("clearDialog", true);
            intent.putExtra("isConsumed", true);
            intent.putExtra(FLAG_GALLERY,mIsStartByGallery);
            intent.putExtra(Intent.EXTRA_TITLE, mTitle);
        }
        /**SPRD:532462
         * SPRD:add Intent.FLAG_GRANT_READ_URI_PERMISSION to grant read EmlAttachmentProvider permission
         */
        intent.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_ACTIVITY_NEW_TASK);
        /* SPRD:Add for bug607543 When changing from float window to normal play,the musicplayer will play @{ */
        mIsAbandonAudiofocus = true;
        intent.putExtra("waitToClose", true);
        /* Bug607543 end @} */
        mContext.startActivity(intent);
        /*SPRD:Bug617084 When play a video completed in float window and then go to normal window,click a item
         of video can't play in normal window @{*/
        if (mCurrentState == STATE_PLAYBACK_COMPLETED) {
            ((Service) mContext).stopSelf();
        }
        /*end @}*/
    }

    public void closeWindow() {
        removeFromWindow();
        ((Service) mContext).stopSelf();
    }

    public void setDataIntent(Intent intent) {
        mDataIntent = intent;
        mFragmentName = mDataIntent.getStringExtra("mFragmentName");
        mOtgDevicePath =mDataIntent.getStringExtra("mOtgDevicePath");
        mIsStartByGallery=mDataIntent.getBooleanExtra(FLAG_GALLERY, false);
        currentPlayBackMode = intent.getIntExtra("currentPlaybackMode", 0);//SPRD:modify by old bug496063
        initPlayList(mIsStartByGallery);
    }

    private void initPlayList(Boolean isStartByGallery) {
        mPlayList = null;
        if (!isStartByGallery) {
            return;
        }
        Uri uri = mDataIntent.getData();
        if (uri == null) {
            return;
        }
        Log.d(TAG, "initPlayList");
        String UriID = uri.toString().substring(
                uri.toString().lastIndexOf("/") + 1, uri.toString().length());
        /* SPRD:Add for Bug606479 The mPlaylist of the float window and the normal play is different @{ */
        ArrayList<VideoItems> mVideoList = new ArrayList<VideoItems>();
        if (mFragmentName == null) {
            return;
        }
        LinkedList<MovieInfo> playList2 = new LinkedList<MovieInfo>();
        switch (mFragmentName) {
            case ALL_VIDEOS_FRAGMENT:
                mVideoList = VideoUtil.getVideoList(mContext);
                break;

            case LOCAL_VIDEOS_FRAGMENT:
                mVideoList = VideoUtil.getLocalVideos(VideoUtil.getVideoList(mContext));
                break;

            case FILMED_VIDEOS_FRAGMENT:
                mVideoList = VideoUtil.getFilmedVideos(VideoUtil.getVideoList(mContext));
                break;

            case OTG_VIDEOS_FRAGMENT:
                mVideoList = VideoUtil.getOtgVideos(VideoUtil.getVideoList(mContext), mOtgDevicePath);
                break;

            case HISTORY_VIDEOS_FRAGMENT:
                MovieInfo mInfo = new MovieInfo();
                mInfo.mID = VideoUtil.getIdFromUri(uri);
                mInfo.mPath = uri.getLastPathSegment();
                mInfo.mAlbum = getFilePathFromUri(uri);
                playList2.add(mInfo);
                mPlayList = playList2;
                break;
            default:
                break;
        }
        if (!mFragmentName.equals(HISTORY_VIDEOS_FRAGMENT)) {
            for (int i = 0; i < mVideoList.size(); i++) {
                MovieInfo mInfo = new MovieInfo();
                mInfo.mAlbum = mVideoList.get(i).getUrl();
                mInfo.mID = String.valueOf(mVideoList.get(i).getId());
                mInfo.mPath = mVideoList.get(i).getDisplayName();
                playList2.add(mInfo);
            }
            mPlayList = playList2;
        }
        /* Bug606479 end @} */
        String scheme = uri.getScheme();
        if ("content".equalsIgnoreCase(scheme)) {
            if (mPlayList != null) {
                for (int i = 0; i < mPlayList.size(); i++) {
                    if (UriID.equalsIgnoreCase(mPlayList.get(i).mID)) {
                        curVideoIndex = i;
                        break;
                    }
                }
            }
        }
    }

    class MovieInfo {
        String mAlbum;
        String mID;
        String mPath;
    }

    /**
     * get state bar height
     *
     * @return
     */
    public int getStatusBarHeight() {
        Class<?> c = null;
        Object obj = null;
        java.lang.reflect.Field field = null;
        int x = 0;
        int statusBarHeight = 0;
        try {
            c = Class.forName("com.android.internal.R$dimen");
            obj = c.newInstance();
            field = c.getField("status_bar_height");
            x = Integer.parseInt(field.get(obj).toString());
            statusBarHeight = mContext.getResources().getDimensionPixelSize(x);
            return statusBarHeight;
        } catch (Exception e) {
            e.printStackTrace();
        }
        return statusBarHeight;
    }

    @Override
    public boolean onTouch(View v, MotionEvent event) {
        if (event.getPointerCount() > 1) {
            mScaleGestureDetector.onTouchEvent(event);
            mTouchStartX = mTouchStartY = 0;
            return true;
        }
        handleTouchEvent(event);
        return false;
    }

    public int getPosition() {
        if (mMediaPlayer != null
                && (mCurrentState == STATE_PAUSED || mCurrentState == STATE_PLAYING)) {
            mPosition = mMediaPlayer.getCurrentPosition();
            return mPosition;
        } else if (mMediaPlayer == null) {
            return mPosition;
        } else {
            return 0;
        }
    }

    public void reSize() {
        if (GalleryUtils.isMonkey()) {
            Log.d(TAG, "<reSize> start");
        }
        /**SPRD:Bug556237 remove mMediaPlayer== null in the judgment statement @{*/
        if (mFloatLayout == null) {
        /**@}*/
            return;
        }
        Point point = new Point();
        mDisplay.getSize(point);
        mWindowWidth = point.x;
        mWindowHeight = point.y;
        int stateHeight = mFloatLayout.getStateHeight();
        int statusBarHeight = getStatusBarHeight();
        int targetWidth = mVideoWidth;
        int targetHeight = mVideoHeight;
        float widRate = (float) mWindowWidth / mVideoWidth;
        float heiRate = (float) mWindowHeight / mVideoHeight;
        // the ratio of height to width;
        float videoAspectRatio = (float) mVideoHeight / mVideoWidth;
        // bug 355537 begin
        // targetWidth = (int) (mSurfaceWidth * scale);
        // if (targetWidth < 216) {
        // targetWidth = 216;
        // } else if (targetWidth > mWindowWidth) {
        // targetWidth = mWindowWidth;
        // }
        Log.e(TAG, "mScale= " + mScale);
        if (widRate > heiRate) {
            targetHeight = (int) (mWindowHeight * mScale);
            // SPRD: add fix the bug 531300
            if (targetHeight < MIN_HEIGHT_IN_LANDSCAPE) {
                targetHeight = MIN_HEIGHT_IN_LANDSCAPE;
            } else if (targetHeight > mWindowHeight - statusBarHeight) {

                targetHeight = mWindowHeight - statusBarHeight;
            }
            targetWidth = (int) (targetHeight / videoAspectRatio);
        } else if (widRate <= heiRate) {
            targetWidth = (int) (mWindowWidth * mScale);
            if (targetWidth < MIN_WIDTH_IN_PORTRAIT) {
                targetWidth = MIN_WIDTH_IN_PORTRAIT;
            } else if (targetWidth > mWindowWidth) {
                targetWidth = mWindowWidth;
            }
            targetHeight = (int) (targetWidth * videoAspectRatio);
            if (targetHeight > mWindowHeight - statusBarHeight) {
                targetHeight = mWindowHeight - statusBarHeight;
                targetWidth = (int) (targetHeight / videoAspectRatio);
            }
        }
        // bug 355537 end

        Log.d(TAG, "reSize  targetWidth=" + targetWidth + " targetHeight=" + targetHeight);
        if (Math.abs(mWmParams.width - targetWidth) > 5
                || Math.abs(mWmParams.height - targetHeight) > 5) {
            mWmParams.width = targetWidth;
            mWmParams.height = targetHeight;
            mWm.updateViewLayout(mFloatLayout, mWmParams);
        }

        if (GalleryUtils.isMonkey()) {
            Log.d(TAG, "<reSize> end");
        }
    }

    public class ScaleGestureListener implements ScaleGestureDetector.OnScaleGestureListener {

        @Override
        public boolean onScale(ScaleGestureDetector detector) {

            float scale = detector.getScaleFactor();
            /* SPRD: add for modify the value of mScale out of range float possibility @{ */
            if ( MIN_SCALE <= mScale && mScale <= MAX_SCALE) {
                mScale = mScale * scale;
                reSize();
            } else if ( mScale <= MIN_SCALE) {
                // modify value
                mScale = MIN_SCALE;
            } else if ( mScale >= MAX_SCALE ) {
                // modify value
                mScale = MAX_SCALE;
            }
            /* @} */

            return true;
        }

        @Override
        public boolean onScaleBegin(ScaleGestureDetector detector) {
            // TODO Auto-generated method stub
            return true;
        }

        @Override
        public void onScaleEnd(ScaleGestureDetector detector) {
            // TODO Auto-generated method stub
        }
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.btn_pause:
                if (!isPhoneCallActivityTop) {
                    if (mCurrentState == STATE_PLAYING) {
                        pause();
                    } else {
                        start();
                    }
                }
                break;
            case R.id.btn_close_win:
                mIsAbandonAudiofocus = true;
                closeWindow();
                break;
            case R.id.btn_back_to_normal:
                if (!isPhoneCallActivityTop) {
                    goToNormalPlay();
                }
                break;
            default:
                break;
        }
    }

    private void nextVideo() {
        Log.d(TAG, "nextVideo");
        // SPRD: Bug568552, temp modify for AndroidN porting @{
        // /**SPRD:Bug474646 Add Drm feature @{ @{ */
        // if (VideoDrmUtils.getInstance().isDrmFile()) {
        //     mMediaPlayer.setNeedToConsume(true);
        // }
        // /**@}*/
        // @}
        if (mPlayList != null) {
            if (curVideoIndex == mPlayList.size() - 1) {
                curVideoIndex = 0;
            } else {
                ++curVideoIndex;
            }
            isFileExistCheck("nextVideo");
            changeVideo();
        }
    }

    private void preVideo() {
        // SPRD: Bug568552, temp modify for AndroidN porting @{
        // /**SPRD:Bug474646 Add Drm feature @{ @{ */
        // if (VideoDrmUtils.getInstance().isDrmFile()) {
        //     mMediaPlayer.setNeedToConsume(true);
        // }
        // /**@}*/
        // @}
        if (mPlayList != null) {
            if (curVideoIndex <= 0) {
                curVideoIndex = mPlayList.size() - 1;
            } else {
                --curVideoIndex;
            }
            isFileExistCheck("preVideo");
            changeVideo();
        }
    }

    private void isFileExistCheck(String options) {
        MovieInfo movieInfo = mPlayList.get(curVideoIndex);
        String moviePath = movieInfo.mAlbum;
        File file = new File(moviePath);
        int count=0;//SPRD:Bug527349 add
        while (file == null || !file.exists()) {
            if (options.equals("preVideo")) {
                if (curVideoIndex <= 0) {
                    curVideoIndex = mPlayList.size() - 1;
                } else {
                    --curVideoIndex;
                }
            } else {
                if (curVideoIndex == mPlayList.size() - 1) {
                    curVideoIndex = 0;
                } else {
                    ++curVideoIndex;
                }
            }
            movieInfo = mPlayList.get(curVideoIndex);
            moviePath = movieInfo.mAlbum;
            file = new File(moviePath);
            /** SPRD:Bug527349 the gallery is crashed then click the next video @{ */
            count++;
            if(mPlayList.size() == count){
                closeWindow();
                Toast.makeText(mContext, R.string.video_file_does_not_exist, Toast.LENGTH_SHORT).show( );
                break;
            }
            /**@}*/
        }
    }

    /** SPRD:Bug527349 the gallery is crashed then click the next video @{ */
    public boolean checkVideoExitOrNot() {
        MovieInfo movieInfo = mPlayList.get(curVideoIndex);
        String moviePath = movieInfo.mAlbum;
        File file = new File(moviePath);
        if (!file.exists()) {
            return false;
        }
        return true;
    }
    /**@}*/

    private void changeVideo() {
        MovieInfo movieInfo = mPlayList.get(curVideoIndex);
        String id = movieInfo.mID;
        Uri uri = videoListUri.buildUpon().appendPath(id).build();
        Log.d(TAG, "changeVideo   uri = " + uri);
        //Sprd:Bug613723 VideoPlayer in all_play_back mode,goto floatwindow and play nextthen goto normal window,the title is wrong
        mTitle = movieInfo.mPath;
        // changedVideoTitle(movieInfo.mPath);// SPRD:Modify for new feature 568552 simplify the float window
        setVideoUri(uri);
        seekTo(0);
        errorDialogCheckAndDismiss();//SPRD:Bug515022
        start();
    }

    private void getVideoName(Intent intent) {
        if (mAlbum != null && mDataIntent.getData() != null && mPlayList != null) {
            MovieInfo movieInfo = mPlayList.get(curVideoIndex);
            mTitle = movieInfo.mPath;
        } else {
            mTitle = mDataIntent.getStringExtra(Intent.EXTRA_TITLE);
        }
        if (mTitle != null) {
            return;
        } else if ("file".equals(mUri.getScheme())) {
            mTitle = mUri.getLastPathSegment();
        } else if ("http".equalsIgnoreCase(mUri.getScheme())
                || "rtsp".equalsIgnoreCase(mUri.getScheme())) {
            mTitle = mUri.getLastPathSegment();
        } else {
            AsyncQueryHandler queryHandler =
                    new AsyncQueryHandler(mContext.getContentResolver()) {
                        @Override
                        protected void onQueryComplete(int token, Object cookie,
                                Cursor cursor) {
                            try {
                                if ((cursor != null) && cursor.moveToFirst()) {
                                    String displayName = cursor.getString(0);
                                    mTitle = displayName;
                                }
                            } finally {
                                Utils.closeSilently(cursor);
                            }
                        }
                    };
            queryHandler.startQuery(0, null, mUri,
                    new String[] {
                        OpenableColumns.DISPLAY_NAME
                    }, null, null,
                    null);
        }
    }

    @Override
    public boolean onError(MediaPlayer mp, int framework_err, int impl_err) {
        /*SPRD:bug614698 When play all back ,the dialog is show all the time@{*/
        if (currentPlayBackMode == ALL_PLAYBACK && mPlayList != null) {
            if (!mFirstError) {
                mErrorPosition = curVideoIndex;
                if (mErrorPosition < 0) {
                    mErrorPosition = mPlayList.size() - 1;
                }
            }
            mErrorTimes++;
            mFirstError = true;
            if (curVideoIndex == mErrorPosition && mErrorTimes >= 2) {
                currentPlayBackMode = 0;
                mFirstError = false;
                mErrorPosition = -1;
                mErrorTimes = 0;
            }
        }
        /*end @}*/
        errorDialogCheckAndDismiss();// SPRD:modify by old Bug493063
        int messageId;
        if (framework_err == MediaPlayer.MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK) {
            messageId = com.android.internal.R.string.VideoView_error_text_invalid_progressive_playback;
        } else {
            messageId = com.android.internal.R.string.VideoView_error_text_unknown;
        }
        alertVideoError(messageId);
        return true;
    }

   /**SPRD:Bug571356 the float window is not pause, when change to an other video @{*/
    public void setTelePhoneManager(TelephonyManager manager) {
        mTelephonyManager = manager;
    }

    public void setSubscriptionManager(SubscriptionManager manager) {
        mSubscriptionManager = manager;
    }
    public boolean isCallingState() {
        if(mTelephonyManager == null
                ||mSubscriptionManager ==null || mContext == null)return false;
        if(!GalleryUtils.checkReadPhonePermissions(mContext)){
           return false;
        }
        boolean CallState = false;
        List<SubscriptionInfo> subInfoRecords = mSubscriptionManager
                .getActiveSubscriptionInfoList();
        if (subInfoRecords != null) {
            for (int i = 0; i < subInfoRecords.size(); i++) {
                SubscriptionInfo sir = subInfoRecords.get(i);
                CallState = mTelephonyManager.getCallState(sir.getSubscriptionId()) != TelephonyManager.CALL_STATE_IDLE;
                if (CallState == true) {
                    break;
                }
            }
        }
        Log.e("TAG", "isCallingState()" + CallState);
        return CallState;
    }
    /**@}*/
    /* SPRD:Add for bug575931 FloatMoviePlayer start playing when the screen is off @{*/
    public void setScreenOff(boolean isScreenOff) {
        mIsScreenOff = isScreenOff;
    }
    /* @} */

    public void setIsAbandonAudiofocus(boolean isAbandonAudiofocus) {
        mIsAbandonAudiofocus = isAbandonAudiofocus;
    }
    /* SPRD:Add for Bug606479 The mPlaylist of the float window and the normal play is different @{ */
    public String getFilePathFromUri(Uri uri) {
        Cursor cursor = null;
        String path = null;
        if (uri.getScheme().toString().compareTo("file") == 0) {
            path = uri.toString().replace("file://", "");
        } else {
            try {
                ContentResolver cr = mContext.getContentResolver();
                cursor = cr.query(uri, null, null, null, null);
                if (cursor != null) {
                    if (cursor.getCount() == 0) {
                        path = null;
                    } else {
                        cursor.moveToFirst();
                        path = cursor.getString(1);
                    }
                } else {
                    path = "uselessUri";
                }
            } catch (IllegalArgumentException e) {
                e.printStackTrace();
            } catch (NullPointerException e) {
                e.printStackTrace();
            } catch (CursorIndexOutOfBoundsException e) {
                e.printStackTrace();
            } catch (SecurityException e) {
                e.printStackTrace();
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
        }
        return path;
    }
    /* Bug606479 end @} */
}
