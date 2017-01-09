/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.soundrecorder;

import android.Manifest;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Map;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Intent;
import android.content.Context;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.database.Cursor;
import android.media.AudioManager;
import android.media.AudioSystem;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.PowerManager;
import android.os.StatFs;
import android.os.PowerManager.WakeLock;
import android.provider.MediaStore;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.app.Service;
import android.content.ComponentName;
import android.content.ContentUris;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.database.sqlite.SQLiteDiskIOException;
import android.os.Debug;
import android.os.IBinder;
import android.os.Message;
import android.os.SystemProperties;
import android.telephony.TelephonyManager;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.Window;
import android.widget.Toast;
import android.os.SystemClock;
import android.widget.RelativeLayout;
import android.graphics.Typeface;

import com.sprd.soundrecorder.PathSelect;
import com.sprd.soundrecorder.RecordingFileList;
import com.sprd.soundrecorder.SoundPicker;
import com.sprd.soundrecorder.StopWatch;
import com.sprd.soundrecorder.StorageInfos;
import com.sprd.soundrecorder.RecorderSnackBar;
import com.sprd.soundrecorder.RecordDatabaseHelper;
import com.sprd.soundrecorder.RecordSetting;
import com.sprd.soundrecorder.RecordWavesView;

/**
 * Calculates remaining recording time based on available disk space and
 * optionally a maximum recording file size.
 * 
 * The reason why this is not trivial is that the file grows in blocks
 * every few seconds or so, while we want a smooth countdown.
 */

class RemainingTimeCalculator {
    public static final int UNKNOWN_LIMIT = 0;
    public static final int FILE_SIZE_LIMIT = 1;
    public static final int DISK_SPACE_LIMIT = 2;

    // which of the two limits we will hit (or have fit) first
    private int mCurrentLowerLimit = UNKNOWN_LIMIT;

    private File mSDCardDirectory;

     // State for tracking file size of recording.
    private File mRecordingFile;
    private long mMaxBytes;

    // Rate at which the file grows
    private int mBytesPerSecond;

    // time at which number of free blocks last changed
    private long mBlocksChangedTime;
    // number of available blocks at that time
    private long mLastBlocks;

    // time at which the size of the file has last changed
    private long mFileSizeChangedTime;
    // size of the file at that time
    private long mLastFileSize;

    /* SPRD: remove @{
    public RemainingTimeCalculator() {
        mSDCardDirectory = Environment.getExternalStorageDirectory();
    }*/

    /* SPRD: update for storage path @{ */
    public RemainingTimeCalculator(String path) {
        mSDCardDirectory = new File(path);
    }

    public void setStoragePath(File path) {
        mSDCardDirectory = path;
    }

    public void setStoragePath(String path) {
        setStoragePath(new File(path));
    }

    /**
     * If called, the calculator will return the minimum of two estimates:
     * how long until we run out of disk space and how long until the file
     * reaches the specified size.
     *
     * @param file the file to watch
     * @param maxBytes the limit
     */

    public void setFileSizeLimit(File file, long maxBytes) {
        mRecordingFile = file;
        mMaxBytes = maxBytes;
    }

    /**
     * Resets the interpolation.
     */
    public void reset() {
        mCurrentLowerLimit = UNKNOWN_LIMIT;
        mBlocksChangedTime = -1;
        mFileSizeChangedTime = -1;
    }

    /**
     * Returns how long (in seconds) we can continue recording.
     */
    public long timeRemaining() {
        // Calculate how long we can record based on free disk space
        //StatFs fs = new StatFs(mSDCardDirectory.getAbsolutePath());
        StatFs fs =null;
        try {
            fs = new StatFs(mSDCardDirectory.getAbsolutePath());
        } catch (Exception e) {
            return 0;
        }
        long blocks = fs.getAvailableBlocks();
        long blockSize = fs.getBlockSize();
        long now = System.currentTimeMillis();

        if (mBlocksChangedTime == -1 || blocks != mLastBlocks) {
            mBlocksChangedTime = now;
            mLastBlocks = blocks;
        }

        /* The calculation below always leaves one free block, since free space
           in the block we're currently writing to is not added. This
           last block might get nibbled when we close and flush the file, but
           we won't run out of disk. */

        // at mBlocksChangedTime we had this much time
        if(mBytesPerSecond == 0){
            if (SoundRecorder.AUDIO_AMR.equals(RecordService.mRequestedType)) {
                setBitRate(SoundRecorder.BIT_PER_SEC_FOR_AMR);
            } else if (SoundRecorder.AUDIO_3GPP.equals(RecordService.mRequestedType)) {
                setBitRate(SoundRecorder.BITRATE_3GPP);
            }
        }

        long result = mLastBlocks*blockSize/mBytesPerSecond;
        // so now we have this much time
        result -= (now - mBlocksChangedTime)/1000;

        if (mRecordingFile == null) {
            mCurrentLowerLimit = DISK_SPACE_LIMIT;
            return result;
        }

        // If we have a recording file set, we calculate a second estimate
        // based on how long it will take us to reach mMaxBytes.

        mRecordingFile = new File(mRecordingFile.getAbsolutePath());
        long fileSize = mRecordingFile.length();
        if (mFileSizeChangedTime == -1 || fileSize != mLastFileSize) {
            mFileSizeChangedTime = now;
            mLastFileSize = fileSize;
        }

        long result2 = (mMaxBytes - fileSize)/mBytesPerSecond;
        result2 -= (now - mFileSizeChangedTime)/1000;
        result2 -= 1; // just for safety

        mCurrentLowerLimit = result < result2
            ? DISK_SPACE_LIMIT : FILE_SIZE_LIMIT;

        return Math.min(result, result2);
    }

    /**
     * Indicates which limit we will hit (or have hit) first, by returning one
     * of FILE_SIZE_LIMIT or DISK_SPACE_LIMIT or UNKNOWN_LIMIT. We need this to
     * display the correct message to the user when we hit one of the limits.
     */
    public int currentLowerLimit() {
        return mCurrentLowerLimit;
    }

    /**
     * Is there any point of trying to start recording?
     */
    public boolean diskSpaceAvailable() {
        // SPRD： add
        if (mSDCardDirectory != null) {
            StatFs fs = new StatFs(mSDCardDirectory.getAbsolutePath());
            // keep one free block
            return fs.getAvailableBlocks() > 1;
        }
        return false;
    }

    /**
     * Sets the bit rate used in the interpolation.
     *
     * @param bitRate the bit rate to set in bits/sec.
     */
    public void setBitRate(int bitRate) {
        mBytesPerSecond = bitRate/8;
    }
}

public class SoundRecorder extends Activity
        //implements Button.OnClickListener, Recorder.OnStateChangedListener {
        implements Button.OnClickListener, RecordService.RecorderListener,
        RecordService.OnUpdateTimeViewListener , RecordService.SetResultListener {
    static final String TAG = "SoundRecorder";
    public static final String COMPOSER = "FMSoundRecorder";  // SPRD： add
    static final String STATE_FILE_NAME = "soundrecorder.state";
    static final String RECORDER_STATE_KEY = "recorder_state";
    static final String SAMPLE_INTERRUPTED_KEY = "sample_interrupted";
    static final String MAX_FILE_SIZE_KEY = "max_file_size";

    private static final String ACTION_SOUNDRECORDER_PAUSE = "com.android.soundercorder.soundercorder.pause"; // SPRD： add

    public static final String AUDIO_3GPP = "audio/3gpp";
    public static final String AUDIO_AMR = "audio/amr";
    public static final String AUDIO_ANY = "audio/*";
    public static final String ANY_ANY = "*/*";
    public static final String AUDIO_MP4 = "audio/mp4";

    /* SPRD: add @{ */
    //static final int BITRATE_AMR =  5900; // bits/sec
    //static final int BITRATE_3GPP = 5900;
    public static final int BITRATE_AMR_WB =  12650; // bits/sec
    public static final int BITRATE_AMR_NB =  5900;  // add by  zl  for bug 130427, 2013/2/27
    public static final int BITRATE_3GPP = 32000;

    //the value get by  maxSize/ (bit_per_sec_for_anr/8) -1 = actaulTime
    //for example, maxSize is x and the actual record time is y, then can get the value
    public static final int BIT_PER_SEC_FOR_AMR = 6415;

    private static final int START_RECORDING_DIALOG_SHOW = 1;

    private static final int PATHSELECT_RESULT_CODE = 1;
    private static final int SOUNDPICKER_RESULT_CODE = 2;

    private static int sampleTime = 0;// add for bug 141797 20130327

    private Dialog mdialog;
    private Dialog mScannerDialog;
    private AlertDialog mAlertDialog;
    private Dialog mSaveDialog = null;
    private AlertDialog mOnErrrDialog;
    private AlertDialog addMediaErrorDialog;
    private AlertDialog mErrorPermissionsDialog;
/* @} */
    public static boolean flag = false;
    private boolean isScanning = false;  // SPRD： add

    WakeLock mWakeLock;
    private volatile boolean disableKeyguarFlag = false;   // SPRD： remove
    //String mRequestedType = AUDIO_ANY;  // SPRD： remove
    String mRequestedType = AUDIO_AMR;  // SPRD： add
    Recorder mRecorder;
    boolean mSampleInterrupted = false;    
    String mErrorUiMessage = null; // Some error messages are displayed in the UI, 
                                   // not a dialog. This happens when a recording
                                   // is interrupted for some reason.
    public boolean mIsPaused = false;  // SPRD： add
    public Object mEmptyLock = new Object();  // SPRD： add
    long mMaxFileSize = -1;        // can be specified in the intent
    RemainingTimeCalculator mRemainingTimeCalculator;
    
    String mTimerFormat;
    private boolean hasUpdateTime = false;  // SPRD： add
    public static final boolean UNIVERSEUI_SUPPORT = true;
    private RelativeLayout mRelativeLayout;
    //Runnable mUpdateTimer = new Runnable() {
    //    public void run() { updateTimerView(); }
    //};
    public boolean fromMMS = false; // SPRD： add
    public boolean getFromMMs(){
        return fromMMS;
    }
    boolean isRequestType = false;  // SPRD： add

    ImageButton mRecordButton;
    ImageButton mTagButton;
    ImageButton mStopButton;
    
    //ImageView mStateLED;
    TextView mStateMessage1;
    TextView mStateMessage2;
    private String mFileNameShow = "";
    ProgressBar mStateProgressBar;
    TextView mTimerView;
    Typeface mTypeface;
    
    LinearLayout mExitButtons;
    Button mAcceptButton;
    Button mDiscardButton;
    VUMeter mVUMeter;
    private RecordWavesView mWavesView;
    private MenuItem mMenuItemRecordList = null;
    private MenuItem mMenuItemSetting = null;
    private BroadcastReceiver mSDCardMountEventReceiver = null;

    private static final int SOUNDRECORD_LIST = Menu.FIRST;
    private static final int STORAGEPATH_SET = Menu.FIRST+1;

    public static final int AMR = MediaRecorder.OutputFormat.AMR_NB;
    public static final int THREE_3GPP = MediaRecorder.OutputFormat.THREE_GPP;
    private int mNowState = RecordService.STATE_IDLE;
    //private boolean mHasBreak = false;
    public static boolean mShowDialog = false;
    private long mRemainingTime = -100;
    private boolean isBind = false;
    public static boolean mCanCreateDir = false;
    /* SPRD: add @{ */
    //private static final boolean DEBUG = Debug.isDebug();
    private static final boolean DEBUG = true;
    private RecordService mService = null;
    long mSampleStart = 0;
    private RecordDatabaseHelper mDBHelper;
    private boolean mIsShowWaveView = false;
    private ServiceConnection mServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName arg0, IBinder arg1) {
            Log.e(TAG, "<onServiceConnected> Service connected");
            mService = ((RecordService.SoundRecorderBinder) arg1).getService();
            initService();
        }

        @Override
        public void onServiceDisconnected(ComponentName arg0) {
            Log.e(TAG, "<onServiceDisconnected> Service dis connected");
            isBind = false;
            mService = null;
        }
    };

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)) {
                String reason = intent.getStringExtra("reason");
                if (reason != null && (reason.equals("homekey") || reason.equals("recentapps"))) {
                    Log.i(TAG, "homekey action = Intent.ACTION_CLOSE_SYSTEM_DIALOGS "+reason);
                    if (fromMMS) {
                        if (mNowState == Recorder.RECORDING_STATE || mNowState == Recorder.SUSPENDED_STATE) {
                            mIsPaused = true;
                            mService.stopRecord();
                        } else {
                            if (mSaveDialog != null && mSaveDialog.isShowing()) {
                                mService.saveSample(true);
                                mRecorder.resetSample();
                            }
                            if(isResumed()) {
                                if(isBind){
                                    unbindService(mServiceConnection);
                                    isBind = false;
                                }
                                stopService(new Intent(SoundRecorder.this , RecordService.class));
                            }
                        }
                    }
                }
            }
        }
     };
     /* @} */

    @Override
    public void onCreate(Bundle icycle) {
        if (DEBUG) Log.d(TAG,"onCreate");
        super.onCreate(icycle);

        mNeedRequestPermissions = checkAndBuildPermissions();

        Intent i = getIntent();
        if (i != null) {
            String s = i.getType();
            /* SPRD: update @{ */
            if (s != null) {
                isRequestType = true;
            }
            if (AUDIO_AMR.equals(s) || AUDIO_3GPP.equals(s) /*|| AUDIO_ANY.equals(s)
                    || ANY_ANY.equals(s)*/) {
                mRequestedType = s;
            } else if (AUDIO_ANY.equals(s) || ANY_ANY.equals(s)) {
                if (DEBUG) Log.d(TAG,"Intent type is:" + s + ", Set mRequestedType is AUDIO_AMR！");
                mRequestedType = AUDIO_AMR;
                /* @} */
            } else if (s != null) {
                // we only support amr and 3gpp formats right now
                setResult(RESULT_CANCELED);
                finish();
                return;
            }

            final String EXTRA_MAX_BYTES
                = android.provider.MediaStore.Audio.Media.EXTRA_MAX_BYTES;
            mMaxFileSize = i.getLongExtra(EXTRA_MAX_BYTES, -1);
            /* SPRD: add @{ */
            String action = i.getAction();
            if (Intent.ACTION_GET_CONTENT.equals(action)
                    || MediaStore.Audio.Media.RECORD_SOUND_ACTION.equals(action)) {
                fromMMS = true;
                // 3 minutes limited, 38 is head length, 1seconds more than 180
                // is for safety
//                mMaxFileSize = 1600 * 182 + 37;
                mMaxFileSize = i.getLongExtra("android.provider.MediaStore.extra.MAX_BYTES",0);
            }
            AbortApplication.getInstance().addActivity(fromMMS , SoundRecorder.this);
            /* @} */
        }

        /* SPRD: remove @{
        if (AUDIO_ANY.equals(mRequestedType) || ANY_ANY.equals(mRequestedType)) {
            mRequestedType = AUDIO_3GPP;
        }

        setContentView(R.layout.main);

        mRecorder = new Recorder();
        mRecorder.setOnStateChangedListener(this);
        mRemainingTimeCalculator = new RemainingTimeCalculator();
        @} */

        /* SPRD: update @{ */
        if (UNIVERSEUI_SUPPORT) {
            //this.requestWindowFeature(Window.FEATURE_NO_TITLE);
            final com.sprd.soundrecorder.StopWatch stopWatch = StopWatch.start("SoundRecorder");
            stopWatch.lap("setContentView");
            setContentView(R.layout.main_overlay);
            stopWatch.lap("setContentViewEnd");
            stopWatch.stopAndLog(TAG, 5);
        } else {
            final com.sprd.soundrecorder.StopWatch stopWatch = StopWatch.start("SoundRecorder");
            stopWatch.lap("setContentView");
            setContentView(R.layout.main);
            stopWatch.lap("setContentViewEnd");
            stopWatch.stopAndLog(TAG, 5);
        }
        /* @} */

        /* SPRD: add @{ */
        /* SPRD: removed for Bug 301423 - SoundRecorder does not have the entry for pathselect @{ */
        //SELECTED_PATH = getStorePath();
        /* @} */
        mdialog = new AlertDialog.Builder(this)
                      .setNegativeButton(getResources().getString(R.string.button_cancel), null)
                      .setMessage(getResources().getString(R.string.storage_is_not_enough))
                      .setCancelable(true)
                      .create();
        mScannerDialog = new AlertDialog.Builder(this)
                      .setNegativeButton(getResources().getString(R.string.button_cancel), null)
                      .setMessage(getResources().getString(R.string.is_scanning))
                      .setCancelable(false)
        .create();
        mScannerDialog.setCanceledOnTouchOutside(false);
        /* @} */

        PowerManager pm
            = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK,
                                    "SoundRecorder");

        initResourceRefs();

        setResult(RESULT_CANCELED);
        registerExternalStorageListener();
        if (icycle != null) {
            Bundle recorderState = icycle.getBundle(RECORDER_STATE_KEY);
            if (recorderState != null) {
                //mRecorder.restoreState(recorderState);
                mService.getRecorder().restoreState(recorderState);
                mSampleInterrupted = recorderState.getBoolean(SAMPLE_INTERRUPTED_KEY, false);
                mMaxFileSize = recorderState.getLong(MAX_FILE_SIZE_KEY, -1);
            }
        }

        //updateUi();
        restoreRecordStateAndData();
        /* SPRD: add @{ */
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        registerReceiver(mReceiver, intentFilter);
        /* @} */
        mDBHelper = new RecordDatabaseHelper(this);
    }

    /* SPRD: add @{ */
    @Override
    protected void onStart() {
        if (DEBUG) Log.d(TAG,"onStart");
        super.onStart();
        if (mNeedRequestPermissions) {
            Log.d(TAG, "need request permissions before start RecordService!");
            return;
        }
        /* @} */
        Log.e(TAG, "<onStart> bind service");
        startService(new Intent(SoundRecorder.this, RecordService.class));
        if (!(isBind = bindService(new Intent(SoundRecorder.this, RecordService.class),
                mServiceConnection, BIND_AUTO_CREATE))) {
            Log.e(TAG, "<onStart> fail to bind service");
            finish();
            return;
        }
    }

    private void initService(){
        Log.i(TAG , "initService()");
        if(mService == null){
            return;
        }
        mService.setStateChangedListener(SoundRecorder.this);
        mService.setUpdateTimeViewListener(SoundRecorder.this);
        mRecorder = mService.getRecorder();
        mNowState = mService.getCurrentState();
        mService.setMaxFileSize(mMaxFileSize);
        mService.setRecordingType(mRequestedType);
        //mVUMeter.setRecorder(mRecorder);
        Log.i(TAG, "fromMMS && !mService.getRecroderComeFrom()"+(fromMMS && !mService.getRecroderComeFrom()));
        mService.setResultListener(SoundRecorder.this);
        if (fromMMS) {
            //mService.setResultListener(SoundRecorder.this);
            if(!mService.getRecroderComeFrom()) {
                if(mNowState == Recorder.RECORDING_STATE || mNowState == Recorder.SUSPENDED_STATE){
                    Log.i(TAG, "<initService> stop record when run from other ap");
                    mService.stopRecord();
                } else {
                    SoundRecorder.flag = true;
                }
                synchronized (RecordService.mStopSyncLock) {
                    try {
                        if(!SoundRecorder.flag) {
                            Log.i(TAG, "wait()");
                            RecordService.mStopSyncLock.wait();
                        }
                        mService.reset();
                        mService.fileListener.stopWatching();
                        mService.saveSample(false);
                    } catch (Exception e) {
                        Log.e(TAG,"Exception:", e);
                    }
                }
                saveTost();
                mRecorder.resetSample();
                if(mService.misStatusbarExist) {
                    mService.startNotification();
                }
                Log.i(TAG, "etInstance().exit()");
            }
        }
        AbortApplication.getInstance().exit();
        updateUi();
        SoundRecorder.flag = false;
        mService.setRecroderComeFrom(fromMMS);
        /* SPRD: add new feature @{ */
        if (!mIsShowWaveView) {
            mWavesView.mWaveDataList = mService.mWaveDataList;
            mWavesView.mTagHashMap = mService.mTagHashMap;
            mWavesView.mTagNumber = mService.mTagNumber;
            Log.d(TAG, "initService updateWavesView");
            updateWavesView();
        }
        /* @} */
    }
    private void openDisableKeyGuard() {
        if (!disableKeyguarFlag) {
            disableKeyguarFlag = true;
            //getWindow().addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);
        }
    }

    private void cancleDisableKeyGuard() {
        if (disableKeyguarFlag) {
            disableKeyguarFlag = false;
            //getWindow().clearFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);
        }
    }

    @Override
    protected void onResume() {
        if (DEBUG) Log.d(TAG,"onResume");
        super.onResume();
        if (mNeedRequestPermissions) {
            Log.d(TAG, "need request permissions before onResume");
            return;
        }
        setActivityState(false);
        sdCardCheck();
        hasUpdateTime = false;
        if(UNIVERSEUI_SUPPORT && (mNowState == Recorder.RECORDING_STATE || mNowState == Recorder.SUSPENDED_STATE)){
            mStopButton.setImageResource(R.drawable.stop);
        }
        String actualPath = "";
        if(mService == null) {
            actualPath = RecordService.getSavePath();
        } else {
            actualPath = RecordSetting.getStorePath(getApplicationContext());
        }
        if ("".equals(actualPath)) {
        } else if (!StorageInfos.haveEnoughStorage(actualPath)) {
            if (mdialog != null) {
                mdialog.show();
            }
        } else if (!StorageInfos.isPathExistAndCanWrite(actualPath)) {
            /* SPRD:fix bug 545385 ,sound record error after delete the storage folder @{*/
            RecordSetting.changeStorePath(getApplicationContext());
            Toast.makeText(this, R.string.use_default_path, Toast.LENGTH_SHORT).show();
            /* SPRD:fix bug 545385 ,sound record error after delete the storage folder @}*/
        }
        if (isScanning) {
            if (mScannerDialog != null) {
                mScannerDialog.show();
            }
        }
        if(mSaveDialog != null && mSaveDialog.isShowing()) {
            if(mService.mIsDelFile){
                mRecorder.resetSample();
                mSaveDialog.dismiss();
            }
            updateUi();
        }
        /** SPRD:Bug 619108 Recorder power consumption optimization ( @{ */
        if(mService!=null){
            mService.RecorderPostUpdateTimer();
        }
        /** @} */
        /*if(mRecorder().mPausedAuto && mRecorder().state() == Recorder.SUSPENDED_STATE) {
            mRecorder().resumeRecording();
            mRecorder().mPausedAuto = false;
        }*/
    }

    private void setActivityState(Boolean bool) {
        synchronized (mEmptyLock) {
            mIsPaused = bool;
        }
    }

    public boolean getActivityState() {
        return mIsPaused;
    }

    private void sdCardCheck() {
        if (!(StorageInfos.isInternalStorageMounted() || StorageInfos.isExternalStorageMounted())) {
            mErrorUiMessage = getResources().getString(R.string.insert_sd_card);
        } else {
            mErrorUiMessage = null;
        }
        updateUi();
    }
    /* @} */

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        /* SPRD bug 260892 @{
        setContentView(R.layout.main);
        initResourceRefs();
        updateUi();
        @} */
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        Log.e(TAG, "<onSaveInstanceState> start");
        if(mService != null){
            mService.saveRecordStateAndSetting();
        }
        /*SPRD: delete for fix bug 266670 @{
        if (mRecorder.sampleLength() == 0)
            return;

        Bundle recorderState = new Bundle();

        mRecorder.saveState(recorderState);
        recorderState.putBoolean(SAMPLE_INTERRUPTED_KEY, mSampleInterrupted);
        recorderState.putLong(MAX_FILE_SIZE_KEY, mMaxFileSize);

        outState.putBundle(RECORDER_STATE_KEY, recorderState);@} */
    }

    /*
     * Whenever the UI is re-created (due f.ex. to orientation change) we have
     * to reinitialize references to the views.
     */
    private void initResourceRefs() {
        mRecordButton = (ImageButton) findViewById(R.id.recordButton);
        mTagButton = (ImageButton) findViewById(R.id.tagButton);
        mStopButton = (ImageButton) findViewById(R.id.stopButton);

        /* SPRD: add for uui @{ */
        mRecordButton.setImageResource(R.drawable.record);
        /* @} */
        mRecordButton.setContentDescription(getResources().getString(R.string.start_record));

        //mStateLED = (ImageView) findViewById(R.id.stateLED);
        mStateMessage1 = (TextView) findViewById(R.id.stateMessage1);
        mStateMessage2 = (TextView) findViewById(R.id.stateMessage2);
        //mStateProgressBar = (ProgressBar) findViewById(R.id.stateProgressBar);
        mTimerView = (TextView) findViewById(R.id.timerView);
        /* SPRD: add new feature @{ */
        mTypeface = Typeface.createFromAsset(getAssets(),"fonts/msyi.ttf");
        mTimerView.setTypeface(mTypeface);

        mRelativeLayout=(RelativeLayout)findViewById(R.id.wavesLayout);
        mWavesView = new RecordWavesView(this);
        mRelativeLayout.addView(mWavesView);
        /* @} */

        /* SPRD: update for uui @{ */
        if (!UNIVERSEUI_SUPPORT) {
            mExitButtons = (LinearLayout) findViewById(R.id.exitButtons);
            mAcceptButton = (Button) findViewById(R.id.acceptButton);
            mDiscardButton = (Button) findViewById(R.id.discardButton);
            //mVUMeter = (VUMeter) findViewById(R.id.uvMeter);
            mDiscardButton.setOnClickListener(this);
            mAcceptButton.setOnClickListener(this);
        }
        /* @} */
        mTagButton.setOnClickListener(this);
        mRecordButton.setOnClickListener(this);
        mStopButton.setOnClickListener(this);

        mTimerFormat = getResources().getString(R.string.timer_format);
        
        //mVUMeter.setRecorder(mRecorder);
    }

    /*
     * Make sure we're not recording music playing in the background, ask
     * the MediaPlaybackService to pause playback.
     */
    private void stopAudioPlayback() {
        //AudioManager am = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        //am.requestAudioFocus(null, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS
        // Shamelessly copied from MediaPlaybackService.java, which
        // should be public, but isn't.
        Intent i = new Intent("com.android.music.musicservicecommand");
        i.putExtra("command", "pause");

        sendBroadcast(i);
    }

    private void startRecord(){
        // mm04 bug 2844
        Intent intent = new Intent();
        intent.setAction(ACTION_SOUNDRECORDER_PAUSE);
        sendBroadcast(intent);
        //mRemainingTimeCalculator.reset();

        TelephonyManager pm = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        TelephonyManager pm1 = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE
                + "1");
        AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
        if (AudioSystem.isSourceActive(MediaRecorder.AudioSource.MIC) ||
                AudioSystem.isSourceActive(MediaRecorder.AudioSource.CAMCORDER) ||
                AudioSystem.isSourceActive(MediaRecorder.AudioSource.VOICE_RECOGNITION)) {
            new AlertDialog.Builder(this)
            .setTitle(R.string.app_name)
            .setMessage(R.string.same_application_running)
            .setPositiveButton(R.string.button_ok, null)
            .setCancelable(false)
            .show();
            updateUi();
        } else if ((pm != null && (pm.getCallState() == TelephonyManager.CALL_STATE_OFFHOOK ||
                pm.getCallState() == TelephonyManager.CALL_STATE_RINGING))
                || (pm1 != null && (pm1.getCallState() == TelephonyManager.CALL_STATE_OFFHOOK ||
                pm1.getCallState() == TelephonyManager.CALL_STATE_RINGING))
                || audioManager.getMode() == AudioManager.MODE_IN_COMMUNICATION) {
            // mErrorUiMessage =
            // getResources().getString(R.string.phone_message);
            Toast.makeText(getApplicationContext(), R.string.phone_message,
                    Toast.LENGTH_SHORT).show();
            updateUi();
        } else if (!(StorageInfos.isInternalStorageMounted() || StorageInfos.isExternalStorageMounted())) {
            mSampleInterrupted = true;
            mErrorUiMessage = getResources().getString(R.string.insert_sd_card);
            updateUi();
            if(mNowState == Recorder.IDLE_STATE){
                mRecordButton.setImageResource(R.drawable.record);
            }
        } /*else if (!mRemainingTimeCalculator.diskSpaceAvailable()) {
            mSampleInterrupted = true;
            mErrorUiMessage = getResources()
                    .getString(R.string.storage_is_full);
            updateUi();
        } */else {
//            stopAudioPlayback();
            //add by linying bug112673 begin
            //If the stored path is in the external storage and this storage state isn't mount, prompt:"path error!"
            if (!StorageInfos.isPathExistAndCanWrite(RecordService.getSavePath())) {
                Toast.makeText(this, R.string.error_path, 1000).show();
                mRecorder.setState(Recorder.IDLE_STATE);
                updateUi();
                return;
            }
            if(AUDIO_AMR.equals(mRequestedType) || AUDIO_3GPP.equals(mRequestedType)){
                mService.startRecording(mRequestedType);
                openDisableKeyGuard();
            }else{
                throw new IllegalArgumentException(
                        "Invalid output file type requested");
            }
        }
    }

    private Toast mToast = null;
     private void doRecord(){
         printLog(TAG,"isRequestType " + isRequestType);

         //modify by linying bug112720 begin
         //if(!checkScannerStatus()){
         if (isScanning) {
         //modify by linying bug112720 end
             if(null != mScannerDialog){
                 mScannerDialog.show();
             }
             return;
         }
         if(mNowState == Recorder.IDLE_STATE) {
             Map<String, String> map = StorageInfos.getStorageInfo(RecordService.getSavePath());
             if(map != null){
                 if(Boolean.parseBoolean(map.get("isEnough"))){
                     long size = Long.parseLong(map.get("availableBlocks"));
                     if(mRecorder == null) {
                         return;
                     } else {
                         mRecorder.setRecordMaxSize(size);
                     }
                 }else{
                     if(mdialog != null){
                         mdialog.show();
                         return;
                     }
                 }
             }

             if (!StorageInfos.isPathExistAndCanWrite(RecordService.getSavePath())) {
                 if(!fromMMS){
                     if(mToast !=  null) {
                         mToast.cancel();
                     }
                     mToast = Toast.makeText(this, R.string.error_path, 1000);
                     mToast.show();
                     return;
                 }else{
                     RecordSetting.changeStorePath(getApplicationContext());
                 }
             }

             /* SPRD: fix bug 608091 SoundRecorder crash when start record and phone storage is full. @{ */
             if (!StorageInfos.haveEnoughStorage(StorageInfos.getInternalStorageDirectory().getPath())) {
                 Toast.makeText(this, R.string.phone_storage_not_enough, Toast.LENGTH_SHORT).show();
                 return;
             }
             /* @} */

             if (!isRequestType){
                 SharedPreferences recordSavePreferences = this.getSharedPreferences(RecordingFileList.SOUNDREOCRD_TYPE_AND_DTA, MODE_PRIVATE);
                 mRequestedType = recordSavePreferences.getString(RecordingFileList.SAVE_RECORD_TYPE, AUDIO_AMR);
             }
             /* SPRD: fix bug 521431 @{ */
             long startTime= System.currentTimeMillis();
             if (((startTime - mSampleStart) > 1000) || ((startTime - mSampleStart) < 0)) {
                 Log.d(TAG, "doRecord startRecord.");
                 startRecord();
                 mSampleStart = System.currentTimeMillis();
             } else {
                 Log.d(TAG, "doRecord startRecord failed!");
                 // SPRD: fix bug 558034 Record icon show wrong when FM is recording
                 updateUi();
             }
             /* @} */
             Log.i(TAG,"mRequestedType is:" + mRequestedType);
             return;
         } else if (mNowState == Recorder.RECORDING_STATE&&mRecorder.sampleFile() != null) {//bug 651092 com.android.soundrecorder happens NativeCrash
             mService.pauseRecord();
         } else if (mNowState == Recorder.SUSPENDED_STATE&&mRecorder.sampleFile() != null) {//bug 651092 com.android.soundrecorder happens NativeCrash
             mService.resumeRecord();
         }
     }
     /* @} */

    /*
     * Handle the buttons.
     */
     /* SPRD: remove @{
    public void onClick(View button) {
        if (!button.isEnabled())
            return;

        switch (button.getId()) {
            case R.id.recordButton:
                mRemainingTimeCalculator.reset();
                if (!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
                    mSampleInterrupted = true;
                    mErrorUiMessage = getResources().getString(R.string.insert_sd_card);
                    updateUi();
                } else if (!mRemainingTimeCalculator.diskSpaceAvailable()) {
                    mSampleInterrupted = true;
                    mErrorUiMessage = getResources().getString(R.string.storage_is_full);
                    updateUi();
                } else {
                    stopAudioPlayback();

                    if (AUDIO_AMR.equals(mRequestedType)) {
                        mRemainingTimeCalculator.setBitRate(BITRATE_AMR);
                        mRecorder.startRecording(MediaRecorder.OutputFormat.AMR_NB, ".amr", this);
                    } else if (AUDIO_3GPP.equals(mRequestedType)) {
                        mRemainingTimeCalculator.setBitRate(BITRATE_3GPP);
                        mRecorder.startRecording(MediaRecorder.OutputFormat.THREE_GPP, ".3gpp",
                                this);
                    } else {
                        throw new IllegalArgumentException("Invalid output file type requested");
                    }
                    
                    if (mMaxFileSize != -1) {
                        mRemainingTimeCalculator.setFileSizeLimit(
                                mRecorder.sampleFile(), mMaxFileSize);
                    }
                }
                break;
            case R.id.playButton:
                mRecorder.startPlayback();
                break;
            case R.id.stopButton:
                mRecorder.stop();
                break;
            case R.id.acceptButton:
                mRecorder.stop();
                saveSample();
                finish();
                break;
            case R.id.discardButton:
                mRecorder.delete();
                finish();
                break;
        }
    }
    @} */

     /* SPRD: add @{ */
     public void onClick(View button) {
         if (!button.isEnabled())
             return;
         switch (button.getId()) {
             case R.id.recordButton:
                 if(!UNIVERSEUI_SUPPORT){
                     invalidateOptionsMenu();
                 }
                 /* SPRD: fix bug 523373 @{ */
                 mNeedRequestPermissions = checkAndBuildPermissions();
                 if (!mNeedRequestPermissions) {
                     doRecord();
                 }
                 /* @} */
                 break;
             case R.id.tagButton:
                 // mRecorder.startPlayback(this);
                 /* SPRD: add new feature @{ */
                 if (mWavesView.mTagHashMap.size() >= 20){
                     runOnUiThread(new Runnable() {
                         @Override
                         public void run() {
                             String title = SoundRecorder.this.getString(R.string.tag_limit);
                             RecorderSnackBar.make(SoundRecorder.this, title, null, null, RecorderSnackBar.DEFAULT_DURATION).show();
                         }
                     });
                     return;
                 }
                 mService.addRecordTag();
                 /* @} */
                 break;
             case R.id.stopButton:
                 /*if (UNIVERSEUI_SUPPORT) {
                     mStopButton.setImageResource(R.drawable.menu_sprd);
                     if(mNowState == Recorder.IDLE_STATE) {
                         if(mRecorder == null) {
                             return;
                         } else {
                             if(mRecorder.sampleFile() == null) {
                                 Intent intent = null;
                                 if(fromMMS){
                                     intent = new Intent(SoundRecorder.this, SoundPicker.class);
                                     startActivityForResult(intent,SOUNDPICKER_RESULT_CODE);
                                 }else{
                                     intent = new Intent(SoundRecorder.this, RecordingFileList.class);
                                     startActivity(intent);
                                 }
                                 return;
                             }
                         }
                     }
                 }*/
                 if(!UNIVERSEUI_SUPPORT){
                     invalidateOptionsMenu();
                 }
                 mRecorder.stop();
                 //mRecordButton.setImageResource(R.drawable.record);
                 break;
             case R.id.acceptButton:
                 mRecorder.stop();
                 mService.saveSample(true);
                 mRecorder.resetSample();
                 updateUi();
                 if(fromMMS){
                     finish();
                     break;
                 }//else                 delete by zhangrongbing 20130315 bug 134959 begin
//                     this.saveTost();  delete by zhangrongbing 20130315 bug 134959 end
//                 finish();
                 break;
             case R.id.discardButton:
                 mRecorder.delete();
                 noSaveTost();
                 updateUi();
//                 finish();
                 break;
         }
     }
     /* @} */

 /* SPRD: add @{ */
     private void saveTost() {
         if (mRecorder.sampleLengthSec() != 0) {
         /** SPRD:Bug 611642 DUT shows concatenated string in snack bar while save voice recorded file.( @{ */
             String title = mRecorder.mFileName +" "+ this.getString(R.string.recording_save);
         /** @} */
             RecorderSnackBar.make(this, title, null, null,
                     RecorderSnackBar.DEFAULT_DURATION).show();
         }
     }

     private void noSaveTost() {
         Toast.makeText(getApplicationContext(), R.string.recording_nosave, Toast.LENGTH_SHORT)
                 .show();
     }
 /* @} */

    /*
     * Handle the "back" hardware key.
     */
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            //switch (mRecorder.state()) {
            switch (mNowState) {
                case Recorder.IDLE_STATE:
                    /* SPRD: update @{ 
                    if (mRecorder.sampleLength() > 0)
                        saveSample();
                    finish();
                    */
                    if(mRecorder != null && mRecorder.sampleFile() != null) {
                        /** SPRD:Bug 615194  com.android.soundrecorder happens JavaCrash,log:java.util.concurrent.TimeoutException ( @{ */
                        mRecorder.stop();
                        /** @} */
                        noSaveTost();
                        updateUi();
                    } else {
                        finish();
                        if(mNowState == Recorder.IDLE_STATE || mNowState == Recorder.STOP_STATE){
                            stopService(new Intent(this , RecordService.class));
                        }
                    }
                    break;
                case Recorder.PLAYING_STATE:
                    mRecorder.stop();
                    //saveSample();
                    break;
            /* SPRD: update @{ */
                case Recorder.RECORDING_STATE:
                    //mRecorder.clear();
                    //break;
                case Recorder.SUSPENDED_STATE:
                    Log.e(TAG, "onKeyDown , Recorder.RECORDING_STATE:");
                    if (fromMMS) {
                        mRecorder.stop();
                    } else {
                        //return super.onKeyDown(keyCode, event);
                        moveTaskToBack(true);
                    }
                    break;
        }
            return true;
        } if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK) {
            if(event.getRepeatCount() != 0){
                return true;
             }
            doRecord();
            return true;
            /* @} */
        }
        if(keyCode == KeyEvent.KEYCODE_MENU){
            if(UNIVERSEUI_SUPPORT){
                return true;
            }
            return false;
        }else {
            return super.onKeyDown(keyCode, event);
        }
    }

    @Override
    public void onStop() {
        /* SPRD: update @{ */
        if (DEBUG) Log.d(TAG,"onStop");
        //mRecorder.stop();
        super.onStop();
        // SPRD: bug594052 SoundRecorder happens JavaCrash.
        if (mService != null) {
            mService.setResultListener(null);
        }
        if(isBind){
            unbindService(mServiceConnection);
            isBind = false;
        }
        if (fromMMS) {
            if (mSaveDialog != null && mSaveDialog.isShowing()) {
                mService.saveSample(true);
                mRecorder.resetSample();
                finish();
                updateUi();
                stopService(new Intent(SoundRecorder.this , RecordService.class));
            }
        }
        /* @} */
    }

    @Override
    protected void onPause() {
        /* SPRD: update @{ */
        //mSampleInterrupted = mRecorder.state() == Recorder.RECORDING_STATE;
        //mRecorder.stop();

        //super.onPause();
        if (DEBUG) Log.d(TAG,"onPause");
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        if (mNowState == Recorder.IDLE_STATE && !pm.isScreenOn()) {
            if(mRecorder != null){
                mRecorder.stop();
                }
        }
        mIsPaused = true;
        /* @} */
        /** SPRD:Bug 619108 Recorder power consumption optimization ( @{ */
        if(mService!=null){
            mService.RecorderRemoveUpdateTimer();
        }
        /** @} */
        super.onPause();
    }


    /*
     * If we have just recorded a smaple, this adds it to the media data base
     * and sets the result to the sample's URI.
     */
    /* SPRD: remove @{
    private void saveSample() {
        if (mRecorder.sampleLength() == 0)
            return;
        Uri uri = null;
        try {
            uri = this.addToMediaDB(mRecorder.sampleFile());
        } catch(UnsupportedOperationException ex) {  // Database manipulation failure
            return;
        }
        if (uri == null) {
            return;
        }
        setResult(RESULT_OK, new Intent().setData(uri)
                                         .setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION));
    }    */

    /*
     * Called on destroy to unregister the SD card mount event receiver.
     */
    @Override
    public void onDestroy() {
        if (DEBUG) Log.d(TAG,"onDestroy");
        if (mSDCardMountEventReceiver != null) {
            unregisterReceiver(mSDCardMountEventReceiver);
            mSDCardMountEventReceiver = null;
        }
        if (mService != null){
            mService.fileListener.stopWatching();
            if(mShowDialog && mService.getSampleFile() != null && mSaveDialog != null){
                mService.saveSample(true);
                mRecorder.resetSample();
                mService.startNotification();
            }
        }
        /* SPRD: add @{ */
        if(mdialog != null){
            mdialog.dismiss();
        }
        if(mSaveDialog != null){
            mSaveDialog.dismiss();
        }
        if(mScannerDialog != null){
            mScannerDialog.dismiss();
        }
        /* SPRD: add @{ */
        if(mOnErrrDialog != null){
            mOnErrrDialog.dismiss();
        }
        /* @} */
        if(null != mReceiver){
            try {
                unregisterReceiver(mReceiver);
                mReceiver = null;
             } catch (IllegalArgumentException e) {
                 Log.e(TAG , "mReceiver not registered. ");
             }
        }
        if(addMediaErrorDialog != null){
            addMediaErrorDialog.dismiss();
        }
        if (fromMMS
                && (mNowState == Recorder.RECORDING_STATE || mNowState == Recorder.SUSPENDED_STATE)) {
            Log.i(TAG, "isFinishing()= " + isFinishing() + " stop record");
            if (mService != null){
                mService.stopRecord();
            }
        }
        mCanCreateDir = false;
        mShowDialog = false;
        mHandler.removeCallbacks(mUpdateWavesView);
        super.onDestroy();
    }

    /*
     * Registers an intent to listen for ACTION_MEDIA_EJECT/ACTION_MEDIA_MOUNTED
     * notifications.
     */
    private void registerExternalStorageListener() {
        if (mSDCardMountEventReceiver == null) {
            mSDCardMountEventReceiver = new BroadcastReceiver() {
                @Override
                public void onReceive(Context context, Intent intent) {
                    if(intent == null){
                        return;
                    }
                    String action = intent.getAction();
                    if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
                        /* SPRD: update for path @{ */
                        String path = intent.getData().getPath();
                        if(RecordService.getSavePath().startsWith(path)) {
                            int state = mRecorder.state();
                            if(state == Recorder.RECORDING_STATE || state == Recorder.SUSPENDED_STATE || (state == Recorder.IDLE_STATE && mSaveDialog != null && mSaveDialog.isShowing())) {
                                    mRecorder.stop();
                                    mRecorder.delete();
                                    mRecorder.resetSample();
                                    if( mSaveDialog != null && mSaveDialog.isShowing()){
                                    mSaveDialog.dismiss();
                                    }
                                Toast.makeText(SoundRecorder.this,R.string.path_miss_nosave, 1000).show();
                            }
                        }
                        /* @} */
                    } else if (action.equals(Intent.ACTION_MEDIA_MOUNTED)) {
                        mSampleInterrupted = false;
                        // SPRD： add for no error
                        // SPRD: Bug632813  Record save in internal storage after insert sdcard.
                        notifyServicePathChanged(context, StorageInfos.getExternalStorageDirectory().getPath());
                        mErrorUiMessage = null;
                        updateUi();
                    }
                }
            };
            IntentFilter iFilter = new IntentFilter();
            iFilter.addAction(Intent.ACTION_MEDIA_EJECT);
            iFilter.addAction(Intent.ACTION_MEDIA_MOUNTED);
            iFilter.addDataScheme("file");
            registerReceiver(mSDCardMountEventReceiver, iFilter);
        }
    }
    // SPRD: Bug632813  Record save in internal storage after insert sdcard begin
    private  void notifyServicePathChanged(Context context, String path) {
        Intent intent = new Intent();
        intent.setAction(RecordService.PATHSELECT_BROADCAST);
        intent.putExtra("newPath", path + Recorder.DEFAULT_STORE_SUBDIR);
        context.sendBroadcast(intent);
    }
    // SPRD: Bug632813 end
    /*
     * A simple utility to do a query into the databases.
     */
    private Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        try {
            ContentResolver resolver = getContentResolver();
            if (resolver == null) {
                return null;
            }
            return resolver.query(uri, projection, selection, selectionArgs, sortOrder);
         } catch (UnsupportedOperationException ex) {
            return null;
        }
    }


    /*
     * Add the given audioId to the playlist with the given playlistId; and maintain the
     * play_order in the playlist.
     */
    /* SPRD: remove @{
    private void addToPlaylist(ContentResolver resolver, int audioId, long playlistId) {
        String[] cols = new String[] {
                "count(*)"
        };
        Uri uri = MediaStore.Audio.Playlists.Members.getContentUri("external", playlistId);
        Cursor cur = resolver.query(uri, cols, null, null, null);
        cur.moveToFirst();
        final int base = cur.getInt(0);
        cur.close();
        ContentValues values = new ContentValues();
        values.put(MediaStore.Audio.Playlists.Members.PLAY_ORDER, Integer.valueOf(base + audioId));
        values.put(MediaStore.Audio.Playlists.Members.AUDIO_ID, audioId);
        resolver.insert(uri, values);
    }*/

    /*
     * Obtain the id for the default play list from the audio_playlists table.
     */
    /* SPRD: remove @{
    private int getPlaylistId(Resources res) {
        Uri uri = MediaStore.Audio.Playlists.getContentUri("external");
        final String[] ids = new String[] { MediaStore.Audio.Playlists._ID };
        final String where = MediaStore.Audio.Playlists.NAME + "=?";
        final String[] args = new String[] { res.getString(R.string.audio_db_playlist_name) };
        Cursor cursor = query(uri, ids, where, args, null);
        if (cursor == null) {
            Log.v(TAG, "query returns null");
        }
        int id = -1;
        if (cursor != null) {
            cursor.moveToFirst();
            if (!cursor.isAfterLast()) {
                id = cursor.getInt(0);
            }
        }
        cursor.close();
        return id;
    }*/

    /*
     * Create a playlist with the given default playlist name, if no such playlist exists.
     */
    /* SPRD: remove @{
    private Uri createPlaylist(Resources res, ContentResolver resolver) {
        ContentValues cv = new ContentValues();
        cv.put(MediaStore.Audio.Playlists.NAME, res.getString(R.string.audio_db_playlist_name));
        Uri uri = resolver.insert(MediaStore.Audio.Playlists.getContentUri("external"), cv);
        if (uri == null) {
            new AlertDialog.Builder(this)
                .setTitle(R.string.app_name)
                .setMessage(R.string.error_mediadb_new_record)
                .setPositiveButton(R.string.button_ok, null)
                .setCancelable(false)
                .show();
        }
        return uri;
    }*/

    /*
     * Adds file and returns content uri.
     */
    /* SPRD: remove @{
    private Uri addToMediaDB(File file) {
        Resources res = getResources();
        ContentValues cv = new ContentValues();
        long current = System.currentTimeMillis();
        long modDate = file.lastModified();
        Date date = new Date(current);
        SimpleDateFormat formatter = new SimpleDateFormat(
                res.getString(R.string.audio_db_title_format));
        String title = formatter.format(date);
        long sampleLengthMillis = mRecorder.sampleLength() * 1000L;

        // Lets label the recorded audio file as NON-MUSIC so that the file
        // won't be displayed automatically, except for in the playlist.
        cv.put(MediaStore.Audio.Media.IS_MUSIC, "0");

        cv.put(MediaStore.Audio.Media.TITLE, title);
        cv.put(MediaStore.Audio.Media.DATA, file.getAbsolutePath());
        cv.put(MediaStore.Audio.Media.DATE_ADDED, (int) (current / 1000));
        cv.put(MediaStore.Audio.Media.DATE_MODIFIED, (int) (modDate / 1000));
        cv.put(MediaStore.Audio.Media.DURATION, sampleLengthMillis);
        cv.put(MediaStore.Audio.Media.MIME_TYPE, mRequestedType);
        cv.put(MediaStore.Audio.Media.ARTIST,
                res.getString(R.string.audio_db_artist_name));
        cv.put(MediaStore.Audio.Media.ALBUM,
                res.getString(R.string.audio_db_album_name));
        Log.d(TAG, "Inserting audio record: " + cv.toString());
        ContentResolver resolver = getContentResolver();
        Uri base = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
        Log.d(TAG, "ContentURI: " + base);
        Uri result = resolver.insert(base, cv);
        if (result == null) {
            new AlertDialog.Builder(this)
                .setTitle(R.string.app_name)
                .setMessage(R.string.error_mediadb_new_record)
                .setPositiveButton(R.string.button_ok, null)
                .setCancelable(false)
                .show();
            return null;
        }
        if (getPlaylistId(res) == -1) {
            createPlaylist(res, resolver);
        }
        int audioId = Integer.valueOf(result.getLastPathSegment());
        addToPlaylist(resolver, audioId, getPlaylistId(res));

        // Notify those applications such as Music listening to the
        // scanner events that a recorded audio file just created.
        sendBroadcast(new Intent(Intent.ACTION_MEDIA_SCANNER_SCAN_FILE, result));
        return result;
    }*/

    /**
     * Update the big MM:SS timer. If we are in playback, also update the
     * progress bar.
     */
    /* SPRD: remove @{
    private void updateTimerView() {
        Resources res = getResources();
        int state = mRecorder.state();

        boolean ongoing = state == Recorder.RECORDING_STATE || state == Recorder.PLAYING_STATE;

        long time = ongoing ? mRecorder.progress() : mRecorder.sampleLength();
        String timeStr = String.format(mTimerFormat, time/60, time%60);
        mTimerView.setText(timeStr);

        if (state == Recorder.PLAYING_STATE) {
            mStateProgressBar.setProgress((int)(100*time/mRecorder.sampleLength()));
        } else if (state == Recorder.RECORDING_STATE) {
            updateTimeRemaining();
        }

        if (ongoing)
            mHandler.postDelayed(mUpdateTimer, 1000);
    }
    @} */

    @Override
    public void updateTimerView(boolean initial) {
        long time = 0;
        int state = mNowState;
        if (!initial) {
            if (null != mService) {
                boolean ongoing = state == Recorder.RECORDING_STATE || state == Recorder.PLAYING_STATE
                        || state == Recorder.SUSPENDED_STATE;
                int lenSec = mRecorder.sampleLengthSec();
                time = ongoing ? (mRecorder.progress()) : lenSec;

                if (mRecorder.progress() > lenSec && state == Recorder.PLAYING_STATE) {
                    time = lenSec;
                }
              }
            }
        long hour = 0;
        long minute = 0;
        long second = time;
        if (second > 59) {
            minute = second / 60;
            second = second % 60;
        }
        if (minute > 59) {
            hour = minute / 60;
            minute = minute % 60;
        }
        String timeStr = String.format(mTimerFormat, hour, minute, second);
        mTimerView.setText(timeStr);

        if (state == Recorder.RECORDING_STATE && mService != null) {
            updateTimeRemaining();
        }

    }

    /*
     * Called when we're in recording state. Find out how much longer we can
     * go on recording. If it's under 5 minutes, we display a count-down in
     * the UI. If we've run out of time, stop the recording.
     */
    private void updateTimeRemaining() {
        //long t = mRemainingTimeCalculator.timeRemaining();
        long t = mService.getTimeRemaining();
        if (t <= 0) {
            mSampleInterrupted = true;

            //int limit = mRemainingTimeCalculator.currentLowerLimit();
            int limit = mService.getCurrentLowerLimit();
            switch (limit) {
                case RemainingTimeCalculator.DISK_SPACE_LIMIT:
                    mErrorUiMessage
                        = getResources().getString(R.string.storage_is_full);
                    break;
                case RemainingTimeCalculator.FILE_SIZE_LIMIT:
                    mErrorUiMessage
                        = getResources().getString(R.string.max_length_reached);
                    break;
                default:
                    mErrorUiMessage = null;
                    break;
            }

            mRecorder.stop();
            return;
        }

        Resources res = getResources();
        String timeStr = "";
        if (fromMMS) {
            if (t < 60)
                timeStr = String.format(res.getString(R.string.sec_available), t);
            /*else if (t < 540)
                timeStr = String.format(res.getString(R.string.min_available), t/60 + 1);*/
            else if(t >= 60){
                int sec = 0;
                int min = 0;
                while(t >= 60) {  // because subtraction is more efficiency than division.
                    t  -= 60;
                    min++;
                }
                sec = (int)t;
                if(sec == 0) {
                    timeStr = String.format(res.getString(R.string.min_available), min);
                } else {
                    timeStr = String.format(res.getString(R.string.min_and_time_available), min, sec);
                }
            }
            // fix bug 250340 mErrorUiMessage is not appear in the center
            mStateMessage1.setText(timeStr);
        } else {
            if (t < 540) {
                //fix bug 250340 mErrorUiMessage is not appear in the center
                mStateMessage1.setText(getResources().getString(R.string.low_memory));
                mStateMessage2.setVisibility(View.GONE);
            }
        }
        /* @} */
    }

    /**
     * Shows/hides the appropriate child views for the new state.
     */
    private void updateUi() {
        Resources res = getResources();
        //switch (mRecorder.state()) {
        switch (mNowState) {
            case Recorder.IDLE_STATE:
                mHandler.removeCallbacks(mUpdateTimer);
                mTimerView.setVisibility(View.VISIBLE);
                mTagButton.setVisibility(View.INVISIBLE);
                mStopButton.setVisibility(View.INVISIBLE);
                if (mMenuItemRecordList != null && mMenuItemSetting != null) {
                    refreshActionMenuItem(true);
                }
                //if (mRecorder.sampleLength() == 0) {
                if (mService == null || mService.getSampleFile() == null || mService.getSampleLengthMillsec() == 0) {
                    mRecordButton.setEnabled(true);
                    mRecordButton.setFocusable(true);
                    if (!UNIVERSEUI_SUPPORT) {
                        mTagButton.setEnabled(false);
                        mTagButton.setFocusable(false);
                        mExitButtons.setVisibility(View.INVISIBLE);
                        mStopButton.setEnabled(false);
                        mStopButton.setFocusable(false);
                    }
                    mRecordButton.requestFocus();

                    mStateMessage1.setVisibility(View.INVISIBLE);
                    //mStateLED.setVisibility(View.INVISIBLE);
                    //mStateLED.setImageResource(R.drawable.idle_led);
                    mStateMessage2.setVisibility(View.INVISIBLE);
                    mStateMessage2.setText(res.getString(R.string.press_record));

                    //mVUMeter.setVisibility(View.VISIBLE);

                    /*if(mStateProgressBar != null){
                        mStateProgressBar.setVisibility(View.INVISIBLE);
                    }*/

                    setTitle(res.getString(R.string.app_name));
                    if(mSaveDialog != null && mSaveDialog.isShowing()) {
                        mSaveDialog.dismiss();
                    }
                } else {
//                    mRecordButton.setEnabled(true);
//                    mRecordButton.setFocusable(true);
                    mRecordButton.setEnabled(false);
                    mRecordButton.setFocusable(false);
                    if (!UNIVERSEUI_SUPPORT) {
                        mTagButton.setEnabled(true);
                        mTagButton.setFocusable(true);
                        if(RecordService.mIsUui_Support){
                          mExitButtons.setVisibility(View.VISIBLE);
                          //mVUMeter.setVisibility(View.INVISIBLE);
                        }else{
                          mExitButtons.setVisibility(View.INVISIBLE);
                          mRecordButton.setEnabled(true);
                          mRecordButton.setFocusable(true);
                          mTagButton.setEnabled(false);
                          mTagButton.setFocusable(false);
                          //mVUMeter.setVisibility(View.VISIBLE);
                        }
                        mStopButton.setEnabled(false);
                        mStopButton.setFocusable(false);
                    }
                    mStateMessage1.setVisibility(View.INVISIBLE);
                    //mStateLED.setVisibility(View.INVISIBLE);
                    mStateMessage2.setVisibility(View.INVISIBLE);

                    //mStateProgressBar.setVisibility(View.INVISIBLE);

                    setTitle(res.getString(R.string.app_name));
                }
                mRecordButton.setImageResource(R.drawable.record);
                mRecordButton.setContentDescription(res.getString(R.string.start_record));
//              if (mSampleInterrupted) {
//                  mStateMessage2.setVisibility(View.VISIBLE);
//                  mStateMessage2.setText(res.getString(R.string.recording_stopped));
//                  mStateLED.setImageResource(R.drawable.idle_led);
//                  mStateLED.setVisibility(View.VISIBLE);
//              }

                if (mErrorUiMessage != null) {
                    /* SPRD: fix bug 250340 mErrorUiMessage is not appear in the center@{ */
                    mStateMessage2.setVisibility(View.GONE);
                    //mStateLED.setVisibility(View.GONE);
                    /* @} */
                    mStateMessage1.setText(mErrorUiMessage);
                    mStateMessage1.setVisibility(View.VISIBLE);
                    mErrorUiMessage = null;
                } else {
                    mStateMessage1.setText("");
                }
                break;
            case Recorder.RECORDING_STATE:
                mHandler.removeCallbacks(mUpdateTimer);
                mTimerView.setVisibility(View.VISIBLE);
                mRecordButton.setEnabled(true);
                mRecordButton.setFocusable(true);
                mRecordButton.setImageResource(R.drawable.suspended);
                mRecordButton.setContentDescription(res.getString(R.string.pause));
                mTagButton.setVisibility(View.VISIBLE);
                mTagButton.setEnabled(true);
                mTagButton.setImageResource(R.drawable.tag_default);
                mStopButton.setVisibility(View.VISIBLE);
                mStopButton.setEnabled(true);
                mStopButton.setImageResource(R.drawable.stop);
                if (mMenuItemRecordList != null && mMenuItemSetting != null) {
                    refreshActionMenuItem(false);
                }

                if (!UNIVERSEUI_SUPPORT) {
                    mTagButton.setEnabled(false);
                    mTagButton.setFocusable(false);
                    mExitButtons.setVisibility(View.INVISIBLE);
                    mStopButton.setEnabled(true);
                    mStopButton.setFocusable(true);
                }
                mStateMessage1.setVisibility(View.VISIBLE);
                //mStateLED.setVisibility(View.VISIBLE);
                //mStateLED.setImageResource(R.drawable.recording_led);
                if (mRecorder != null)
                    mFileNameShow = mRecorder.mFileName;
                mStateMessage2.setVisibility(View.VISIBLE);
                mStateMessage2.setText(mFileNameShow);

                //mVUMeter.setVisibility(View.VISIBLE);

                /*if(mStateProgressBar != null){
                mStateProgressBar.setVisibility(View.INVISIBLE);
                }*/

                setTitle(res.getString(R.string.app_name));

                break;

            case Recorder.PLAYING_STATE:
                //mRecordButton.setEnabled(true);
                //mRecordButton.setFocusable(true);
                mRecordButton.setEnabled(false);
                mRecordButton.setFocusable(false);
                if (!UNIVERSEUI_SUPPORT) {
                    mTagButton.setEnabled(false);
                    mTagButton.setFocusable(false);
                    mExitButtons.setVisibility(View.VISIBLE);
                    //mVUMeter.setVisibility(View.INVISIBLE);
                    mStopButton.setEnabled(true);
                    mStopButton.setFocusable(true);
                }
                mStateMessage1.setVisibility(View.INVISIBLE);
                //mStateLED.setVisibility(View.INVISIBLE);
                mStateMessage2.setVisibility(View.INVISIBLE);

                //mStateProgressBar.setVisibility(View.VISIBLE);

                setTitle(res.getString(R.string.app_name));
                break;
                /* @} */
            /* SPRD: add @{ */
            case Recorder.SUSPENDED_STATE:
                mHandler.removeCallbacks(mUpdateTimer);
                mHandler.post(mUpdateTimer);
                mRecordButton.setImageResource(R.drawable.play);
                mRecordButton.setContentDescription(res.getString(R.string.resume_record));
                mRecordButton.setEnabled(true);
                mRecordButton.setFocusable(false);
                mTagButton.setVisibility(View.VISIBLE);
                mTagButton.setEnabled(false);
                mTagButton.setImageResource(R.drawable.tag_disabled);
                mStopButton.setVisibility(View.VISIBLE);
                mStopButton.setEnabled(true);
                mStopButton.setImageResource(R.drawable.stop);
                if (!UNIVERSEUI_SUPPORT) {
                    mTagButton.setEnabled(false);
                    mTagButton.setFocusable(false);
                    mExitButtons.setVisibility(View.INVISIBLE);
                    mStopButton.setEnabled(true);
                    mStopButton.setFocusable(true);
                }
                mStateMessage1.setVisibility(View.VISIBLE);
                //mStateLED.setVisibility(View.VISIBLE);
                //mStateLED.setImageResource(R.drawable.idle_led);
                if (mRecorder != null)
                    mFileNameShow = mRecorder.mFileName;
                mStateMessage2.setVisibility(View.VISIBLE);
                mStateMessage2.setText(mFileNameShow);
                if (!"".equals(mStateMessage1.getText().toString())) {
                    mStateMessage2.setVisibility(View.GONE);
                }

                //mVUMeter.setVisibility(View.VISIBLE);

                //mStateProgressBar.setVisibility(View.INVISIBLE);

                setTitle(res.getString(R.string.app_name));
                break;
        /* @} */
        }
        updateTimerView(false);
        //mVUMeter.invalidate();
    }

    /*
     * Called when Recorder changed it's state.
     */
    /* SPRD: remove @{
    public void onStateChanged(int state) {
        if (state == Recorder.PLAYING_STATE || state == Recorder.RECORDING_STATE) {
            mSampleInterrupted = false;
            mErrorUiMessage = null;
            mWakeLock.acquire(); // we don't want to go to sleep while recording or playing
        } else {
            if (mWakeLock.isHeld())
                mWakeLock.release();
        }

        updateUi();
    }
    @} */

    public void acquireWakeLock() {
        mSampleInterrupted = false;
        mErrorUiMessage = null;
        if(!mWakeLock.isHeld()) {
            mWakeLock.acquire();  // we don't want to go to sleep while recording or playing
        }
    }

    public void releaseWakeLock() {
        if (mWakeLock.isHeld()){
            mWakeLock.release();
        }
    }
    /* @} */

    /*
     * Called when MediaPlayer encounters an error.
     */
    public void onError(int error) {
        if(isFinishing()){
            return;
        }
        Resources res = getResources();

        String message = null;
        switch (error) {
            case Recorder.SDCARD_ACCESS_ERROR:
                message = res.getString(R.string.error_sdcard_access);
                break;
            case Recorder.IN_CALL_RECORD_ERROR:
                // TODO: update error message to reflect that the recording could not be
                //       performed during a call.
            case Recorder.INTERNAL_ERROR:
                message = res.getString(R.string.error_app_internal);
                break;
            case Recorder.PATH_NOT_EXIST:
                message = res.getString(R.string.path_miss);
                updateUi();
                break;
            case Recorder.RECORDING_ERROR:
                /* SPRD: modify */
                if (mRecorder.sampleLengthMillsec() <= 1000) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            /** SPRD:Bug 614337 Less than 1 second recording is interrupted in other interface without saving prompt( @{ */
                            if(mIsPaused){
                                Toast.makeText(SoundRecorder.this,R.string.recording_time_short, Toast.LENGTH_SHORT).show();
                            }else{
                                String title = SoundRecorder.this.getString(R.string.recording_time_short);
                                RecorderSnackBar.make(SoundRecorder.this, title, null, null, RecorderSnackBar.DEFAULT_DURATION).show();
                            }
                            /** @} */
                        }
                    });
                    Log.w(TAG, "the recodering time is short");
                } else {
                    message = res.getString(R.string.record_error);
                }
                /* @} */
                break;
            default :
                break;
        }
        if (message != null && mOnErrrDialog == null) {
            /* SPRD: add @{ */
            mOnErrrDialog = new AlertDialog.Builder(this)
                .setTitle(R.string.app_name)
                .setMessage(message)
                //.setPositiveButton(R.string.button_ok, null)
                .setPositiveButton(R.string.button_ok, new OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if (mOnErrrDialog != null) {
                            mOnErrrDialog.dismiss();
                            mOnErrrDialog = null;
                        }
                    }
                 })
                .setCancelable(false)
                .show();
             /* @} */
        }
    }

    /* SPRD: add @{ */
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if(PATHSELECT_RESULT_CODE == requestCode){
            Bundle bundle = null;
            if(data!=null&&(bundle=data.getExtras())!=null){
                String selectPath = bundle.getString("file");
                mService.setStoragePath(selectPath);
            }
        }
        if(SOUNDPICKER_RESULT_CODE == requestCode){
            if(data != null){
                if( data.getData() != null){
                    setResult(RESULT_OK, new Intent().setData(data.getData()));
                }
                this.finish();
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu){
        /*if (!fromMMS) {
            super.onCreateOptionsMenu(menu);
            MenuInflater inflater = getMenuInflater();
            if (UNIVERSEUI_SUPPORT) {
                inflater.inflate(R.menu.options_menu_overlay, menu);
            } else {
                Resources res = getResources();
                menu.add(Menu.NONE, SOUNDRECORD_LIST, 1, res.getText(R.string.menu_recording_file_list));
                menu.add(Menu.NONE + 1, STORAGEPATH_SET, 2, res.getText(R.string.menu_set_save_path));
            }
        }*/
        super.onCreateOptionsMenu(menu);
        getMenuInflater().inflate(R.menu.setting_menu, menu);
        mMenuItemRecordList = menu.findItem(R.id.item_recordlist);
        mMenuItemSetting = menu.findItem(R.id.item_setting);
        return true;
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        if (!UNIVERSEUI_SUPPORT && !fromMMS) {
            if (Recorder.RECORDING_STATE == mNowState
                    || Recorder.SUSPENDED_STATE == mNowState) {
                menu.findItem(SOUNDRECORD_LIST).setEnabled(false);
                menu.findItem(STORAGEPATH_SET).setEnabled(false);
            } else {
                menu.findItem(SOUNDRECORD_LIST).setEnabled(true);
                menu.findItem(STORAGEPATH_SET).setEnabled(true);
            }
        }
        if (Recorder.RECORDING_STATE == mNowState
                || Recorder.SUSPENDED_STATE == mNowState) {
            refreshActionMenuItem(false);
        } else {
            refreshActionMenuItem(true);
        }
        return true;
    }

    /* SPRD: add new feature @{ */
    private void refreshActionMenuItem(boolean enabled) {
        mMenuItemRecordList.setEnabled(enabled);
        mMenuItemSetting.setEnabled(enabled);
        if (enabled) {
            mMenuItemSetting.setIcon(R.drawable.ic_setting_holo_light);
            mMenuItemRecordList.setIcon(R.drawable.ic_menu_holo_light);
        }else{
            mMenuItemSetting.setIcon(R.drawable.ic_setting_holo_dark);
            mMenuItemRecordList.setIcon(R.drawable.ic_menu_holo_dark);
        }
    }
    /* @} */
    @Override
    public boolean onOptionsItemSelected(MenuItem item){
        /*Intent intent = null;
        if(UNIVERSEUI_SUPPORT){
            switch (item.getItemId()) {
                case R.id.menu_recording_file_list:
                    intent = new Intent(SoundRecorder.this, RecordingFileList.class);
                    break;
                case R.id.menu_set_save_path:
                    if(!StorageInfos.isExternalStorageMounted() && !StorageInfos.isInternalStorageMounted()){
                        Toast.makeText(this, R.string.stroage_not_mounted, Toast.LENGTH_LONG).show();
                    }else{
                        intent = new Intent(SoundRecorder.this, PathSelect.class);
                    }
                    break;
                default:
                    break;
            }
        }
        else{
            switch (item.getItemId()) {
                case SOUNDRECORD_LIST:
                    intent = new Intent(SoundRecorder.this, RecordingFileList.class);
                    break;
                case STORAGEPATH_SET:
                    if(!StorageInfos.isExternalStorageMounted() && !StorageInfos.isInternalStorageMounted()){
                        Toast.makeText(this, R.string.stroage_not_mounted, Toast.LENGTH_LONG).show();
                    }else{
                        intent = new Intent(SoundRecorder.this, PathSelect.class);
                    }
                    break;
                default:
                    break;
            }
            if (intent != null) {
                startActivityForResult(intent, PATHSELECT_RESULT_CODE);
                return true;
            }
        }*/

        /* SPRD: add new feature @{ */
        int id = item.getItemId();
        if (id == R.id.item_setting) {
            Intent intent = new Intent(SoundRecorder.this, RecordSetting.class);
            startActivity(intent);
            return true;
        }else if(id == R.id.item_recordlist){
            if (UNIVERSEUI_SUPPORT) {
                if(mNowState == Recorder.IDLE_STATE) {
                    if(mRecorder == null) {
                        return true;
                    } else {
                        if(mRecorder.sampleFile() == null) {
                            Intent intent = null;
                            if(fromMMS){
                                intent = new Intent(SoundRecorder.this, SoundPicker.class);
                                startActivityForResult(intent,SOUNDPICKER_RESULT_CODE);
                            }else{
                                intent = new Intent(SoundRecorder.this, RecordingFileList.class);
                                startActivity(intent);
                            }
                            return true;
                        }
                    }
                }
            }
        }
        /* @} */
        return super.onOptionsItemSelected(item);
    }
    protected void dialog() {
        AlertDialog.Builder builder = new Builder(SoundRecorder.this);
        builder.setTitle(R.string.dialog_title);
        builder.setMessage(R.string.dialog_message);
        builder.setPositiveButton(R.string.button_ok, new OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
                mShowDialog = false;
                /* SPRD: fix bug 519001 @{ */
                mNeedRequestPermissions = checkAndBuildPermissions();
                if (!mNeedRequestPermissions) {
                    mService.saveSample(true);
                }
                /* @} */
                mRecorder.resetSample();
                //SPRD: fix bug511425 remove record notification when stop record.
                //mService.startNotification();
                updateUi();
                if (fromMMS) {
                    finish();
                    if(mNowState == Recorder.IDLE_STATE || mNowState == Recorder.STOP_STATE){
                        stopService(new Intent(SoundRecorder.this , RecordService.class));
                    }
                }
            }
        });
        builder.setNegativeButton(R.string.button_cancel, new OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
                mShowDialog = false;
                mRecorder.delete();
                noSaveTost();
               updateUi();
            }
        });
        builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                dialog.dismiss();
                mShowDialog = false;
                mRecorder.delete();
                noSaveTost();
                updateUi();
            }
        });
        builder.setCancelable(true);
        mSaveDialog = builder.create();
        if(mSaveDialog != null && !isFinishing()){
            mSaveDialog.setCanceledOnTouchOutside(false);
            mSaveDialog.show();
            mShowDialog = true;
        }
    }

    private void printLog(String tag, String msg) {
        if (DEBUG) {
            android.util.Log.d(tag, msg);
        }
    }
/* @} */
    private void changeStorePath(){
        String path = StorageInfos.getInternalStorageDirectory().getAbsolutePath().toString();
        if(mService != null){
            mService.initRemainingTimeCalculator(path);
        }
        SharedPreferences fileSavePathShare = this.getSharedPreferences(PathSelect.DATA_BASE, MODE_PRIVATE);
        SharedPreferences.Editor edit = fileSavePathShare.edit();
        edit.putString(PathSelect.SAVE_PATH, path);
        edit.commit();
        if(mSaveDialog != null && mSaveDialog.isShowing()){
            mSaveDialog.dismiss();
        }
    }

    @Override
    public void onStateChanged(int stateCode) {
        Log.e(TAG , "onStateChanged stateCode "+stateCode);
        mNowState = stateCode;
        switch (stateCode) {
            case Recorder.STOP_STATE:
                if(fromMMS && (mIsPaused || mService.mIsStopFromStatus || mService.mHasBreak)){
                    mService.saveSample(true);
                    Log.e(TAG , "stopService");
                    finish();
                    if(isBind){
                        unbindService(mServiceConnection);
                        isBind = false;
                    }
                    stopService(new Intent(SoundRecorder.this, RecordService.class));
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mRecorder.resetSample();
                        }
                    });
                }
                if(mService.mHasBreak||mRecorder.mIsAudioFocus_Loss || mService.mIsStopFromStatus) {
                    mService.saveSample(true);
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mRecorder.resetSample();
                            if (fromMMS) {
                               finish();
                               if(isBind) {
                                   unbindService(mServiceConnection);
                                   isBind = false;
                               }
                               stopService(new Intent(SoundRecorder.this , RecordService.class));
                            }
                        }
                    });
                } else {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            File file = mRecorder.sampleFile();
                            if(file == null || !file.exists()) {
                                //noSaveTost();
                                mRecorder.delete();
                                mRecorder.resetSample();
                                updateUi();
                                return;
                            } else if(mRecorder.sampleLengthMillsec() <= 1000) {
                                /** SPRD:Bug 614337 Less than 1 second recording is interrupted in other interface without saving prompt( @{ */
                                //Toast.makeText(SoundRecorder.this, R.string.recording_time_short, 1000).show();
                                /** @} */
                                mRecorder.delete();
                                updateUi();
                                return;
                            } else {
                                if(SoundRecorder.UNIVERSEUI_SUPPORT) {
                                    //dialog();
                                    saveRecord();
                                    SoundRecorder.flag = false;
                                }
                            }
                        }
                    });
                }
                break;

            case Recorder.SUSPENDED_STATE:
                //mService.resetRemainingTimeCalculator();
                break;
            case Recorder.PLAYING_STATE:
            case Recorder.RECORDING_STATE:
                //acquireWakeLock();
                openDisableKeyGuard();
                break;
            case Recorder.IDLE_STATE:
                mService.mHasBreak = false;
                mIsPaused = false;
                break;
            default:
                break;
        }
        mHandler.sendEmptyMessage(mNowState);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                updateUi();
            }
        });
    }
    private void restoreRecordStateAndData(){
        SharedPreferences recordSavePreferences = this.getSharedPreferences(RecordService.SOUNDREOCRD_STATE_AND_DTA, MODE_PRIVATE);
        mNowState = recordSavePreferences.getInt(RecordService.SAVE_RECORD_STATE, Recorder.IDLE_STATE);
        Log.e(TAG , "restoreRecordStateAndData mNowState"+mNowState);
    }

    @Override
    public void addMediaError() {
        addMediaErrorDialog = new AlertDialog.Builder(this)
        .setTitle(R.string.app_name)
        .setMessage(R.string.error_mediadb_new_record)
        .setPositiveButton(R.string.button_ok, null)
        .setCancelable(false)
        .show();
    }

    @Override
    public void setResultRequest(Uri uri) {
        Log.i(TAG , "setResultRequest(Uri uri) "+uri);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                saveTost();
            }
        });
        if (fromMMS)
            setResult(RESULT_OK, new Intent().setData(uri).setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION));
    }

    private static final int RECORD_PERMISSIONS_REQUEST_CODE = 200;
    private boolean mNeedRequestPermissions = false;
    /**
     * @}
     */
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
            mCanCreateDir = true;
            return false;
        }
        String[] permissionsToRequest = new String[numPermissionsToRequest];
        int permissionsRequestIndex = 0;
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

    @Override
    public void onRequestPermissionsResult(int requestCode,
            String permissions[], int[] grantResults) {
        switch (requestCode) {
            case RECORD_PERMISSIONS_REQUEST_CODE: {
                boolean resultsAllGranted = true;
                if (grantResults.length > 0) {
                    for (int result : grantResults) {
                        if (PackageManager.PERMISSION_GRANTED != result) {
                            resultsAllGranted = false;
                            mCanCreateDir = false;
                        }
                    }
                } else {
                    resultsAllGranted = false;
                    mCanCreateDir = false;
                }
                /* SPRD: fix bug 522124 @{ */
                if (mErrorPermissionsDialog != null) {
                    mErrorPermissionsDialog.dismiss();
                }
                /* @} */
                if (resultsAllGranted) {
                    mNeedRequestPermissions = false;
                    mCanCreateDir = true;
                    // Should start recordService  first.

                    Log.d(TAG, "<onRequestPermissionsResult> bind service");
                    startService(new Intent(SoundRecorder.this, RecordService.class));
                    if (!(isBind = bindService(new Intent(SoundRecorder.this, RecordService.class),
                            mServiceConnection, BIND_AUTO_CREATE))) {
                        Log.e(TAG, "<onStart> fail to bind service");
                        finish();
                        return;
                    }
                } else {
                    mErrorPermissionsDialog = new AlertDialog.Builder(this)
                            .setTitle(
                                    getResources()
                                            .getString(R.string.error_app_internal))
                            .setMessage(getResources().getString(R.string.error_permissions))
                            .setCancelable(false)
                            .setOnKeyListener(new Dialog.OnKeyListener() {
                                @Override
                                public boolean onKey(DialogInterface dialog, int keyCode,
                                        KeyEvent event) {
                                    if (keyCode == KeyEvent.KEYCODE_BACK) {
                                        finish();
                                    }
                                    return true;
                                }
                            })
                            .setPositiveButton(getResources().getString(R.string.dialog_dismiss),
                                    new DialogInterface.OnClickListener() {
                                        @Override
                                        public void onClick(DialogInterface dialog, int which) {
                                            finish();
                                        }
                                    })
                            .show();
                }
                return;
            }
        }
    }

    /* SPRD: add new feature @{ */
    final Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG, "mHandler handleMessage msg.what = "+msg.what);
            super.handleMessage(msg);
            switch (msg.what) {
                case Recorder.RECORDING_STATE:
                    updateWavesView();
                    break;
                case Recorder.STOP_STATE:
                    mHandler.removeCallbacks(mUpdateWavesView);
                    mService.addTagToDB(mRecorder.mFileName, mWavesView.mTagNumber+1, mWavesView.mWaveDataList.size());
                    break;
                case Recorder.IDLE_STATE:
                    mService.mWaveDataList.clear();
                    mService.mTagHashMap.clear();
                    mService.mTagNumber = 0;

                    mWavesView.mWaveDataList.clear();
                    mWavesView.mTagHashMap.clear();
                    mWavesView.mTagNumber = 0;
                    mWavesView.mCount = 0;
                    mWavesView.invalidate();
                    mWavesView.mLastSize = 0;
                    break;
            }
        }
    };

    private final Runnable mUpdateWavesView = new Runnable() {
        public void run() {
            updateWavesView();
        }
    };

    public void updateWavesView() {
        if (mNowState == Recorder.RECORDING_STATE) {
            mIsShowWaveView = true;
            mWavesView.invalidate();
            mHandler.removeCallbacks(mUpdateWavesView);
            mHandler.postDelayed(mUpdateWavesView, 50);
        } else if (mNowState == Recorder.SUSPENDED_STATE) {
            Log.d(TAG, "updateWavesView: SUSPENDED_STATE mIsShowWaveView = "+mIsShowWaveView);
            if (!mIsShowWaveView) {
                mWavesView.invalidate();
            }
        }
    }

    private final Runnable mUpdateTimer = new Runnable() {
        public void run() {
            if (mTimerView.getVisibility() == View.VISIBLE) {
                mTimerView.setVisibility(View.INVISIBLE);
            } else {
                mTimerView.setVisibility(View.VISIBLE);
            }
            mHandler.postDelayed(mUpdateTimer, 500);
       }
    };

    protected void saveRecord(){
        mShowDialog = false;
        mNeedRequestPermissions = checkAndBuildPermissions();
        if (!mNeedRequestPermissions) {
            mService.saveSample(true);
        }
        mRecorder.resetSample();
        updateUi();
        if (fromMMS) {
            finish();
            if(mNowState == Recorder.IDLE_STATE || mNowState == Recorder.STOP_STATE){
                stopService(new Intent(SoundRecorder.this , RecordService.class));
            }
        }
    }
    /* @} */
}
