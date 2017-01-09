/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.fmradio;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.FragmentManager;
import android.app.Notification;
import android.app.Notification.Builder;
import android.app.PendingIntent;

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.database.ContentObserver;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.UserManager;
import android.os.storage.StorageManager;
import android.os.storage.VolumeInfo;
import android.text.SpannableString;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.android.fmradio.FmStation.Station;
import com.android.fmradio.dialogs.FmSaveDialog;
import com.android.fmradio.views.FmSnackBar;
import com.android.fmradio.views.FmVisualizerView;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/**
 * SPRD:
 *
 * @{
 */
import android.Manifest;
import android.media.AudioManager;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.storage.StorageManager;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.SharedPreferences;

/**
 * @}
 */

/**
 * This class interact with user, FM recording function.
 */
public class FmRecordActivity extends Activity implements
        FmSaveDialog.OnRecordingDialogClickListener {
    private static final String TAG = "FmRecordActivity";

    private static final String FM_STOP_RECORDING = "fmradio.stop.recording";
    private static final String FM_ENTER_RECORD_SCREEN = "fmradio.enter.record.screen";
    private static final String TAG_SAVE_RECORDINGD = "SaveRecording";
    private static final int MSG_UPDATE_NOTIFICATION = 1000;
    private static final int TIME_BASE = 60;
    private Context mContext;
    private TextView mMintues;
    private TextView mSeconds;
    private TextView mFrequency;
    private View mStationInfoLayout;
    private TextView mStationName;
    private TextView mRadioText;
    private Button mStopRecordButton;
    private FmVisualizerView mPlayIndicator;
    private FmService mService = null;
    private FragmentManager mFragmentManager;
    private boolean mIsInBackground = false;
    private int mRecordState = FmRecorder.STATE_INVALID;
    private int mCurrentStation = FmUtils.DEFAULT_STATION;
    private Notification.Builder mNotificationBuilder = null;
    // SPRD:add feature for support OTG.
    private String mSDCardPathName = "";
    private boolean mIsShowDialogLater = false;

    /**
     * SPRD: bug490629, recording length increases jumping seconds.
     * 
     * @{
     */
    private static final int MSGID_REFRESH_DELAY = 1000;
    /**
     * @}
     */

    /**
     * SPRD: bug495859, request runtime permissions
     */
    private boolean mIsSaveDialogShown = false;
    /**
     * @}
     */

    /**
     * SPRD: bug522185, FM can not show save dialog after play video with horizontal mode.
     */
    private boolean mStateFromBundle = false;
    /**
     * @}
     */
    // SPRD: bug526250, re-enter FmRecordActivity and rotate screen,occur inner error
    private boolean mIsPermissionDialogShown = false;
    private boolean mShouldShowSaveDialog = false;
    private boolean mIsChangeRecord = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // SPRD : bug568587, new feature FM new UI
        getActionBar().setDisplayHomeAsUpEnabled(true);
        Log.d(TAG, "onCreate");
        setVolumeControlStream(AudioManager.STREAM_MUSIC);

        /**
         * SPRD: bug495859, request runtime permissions
         */
        if (savedInstanceState != null) {
            mIsSaveDialogShown = savedInstanceState.getBoolean("isSaveDialogShown");
            // SPRD: bug526250, re-enter FmRecordActivity and rotate screen,occur inner erro
            mIsPermissionDialogShown = savedInstanceState.getBoolean("isPermissionDialogShown");
            mShouldShowSaveDialog = savedInstanceState.getBoolean("mShouldShowSaveDialog");
        }
        /**
         * @}
         */

        /**
         * SPRD: bug524253, restore record path and format.
         * 
         * @{
         */
        SharedPreferences formatSP = this.getSharedPreferences(FM_RECORD_FORMAT,
                Context.MODE_PRIVATE);
        FmRecorder.GLOBAL_RECORD_FORMAT_FLAG = formatSP.getInt(DEFAULT_FORMAT, 0);
        mRecordInternalStorage = String.valueOf(getResources().getString(R.string.storage_phone));
        mRecordExternalStorage = String.valueOf(getResources().getString(R.string.storage_sd));
        SharedPreferences storageSP = this.getSharedPreferences(FM_RECORD_STORAGE,
                Context.MODE_PRIVATE);
        FmUtils.FM_RECORD_STORAGE_PATH = storageSP.getInt(FM_RECORD_DEFAULT_PATH, 0);
        FmUtils.FM_RECORD_STORAGE_PATH_NAME = storageSP.getString(FM_RECORD_DEFAULT_PATH_NAME,
                mRecordInternalStorage);
        /**
         * @}
         */

        mContext = getApplicationContext();
        // SPRD:add feature for support OTG.
        mSDCardPathName = String.valueOf(getResources().getString(R.string.storage_sd));
        mFragmentManager = getFragmentManager();
        setContentView(R.layout.fm_record_activity);

        mMintues = (TextView) findViewById(R.id.minutes);
        mSeconds = (TextView) findViewById(R.id.seconds);

        mFrequency = (TextView) findViewById(R.id.frequency);
        mStationInfoLayout = findViewById(R.id.station_name_rt);
        mStationName = (TextView) findViewById(R.id.station_name);
        mRadioText = (TextView) findViewById(R.id.radio_text);

        mStopRecordButton = (Button) findViewById(R.id.btn_stop_record);
        mStopRecordButton.setEnabled(false);
        mStopRecordButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                /**
                 * SPRD: bug582477, for monkey test.
                 * 
                 * @{
                 */
                if (null == mService) {
                    Log.e(TAG, "onClick, mService is null");
                    return;
                }
                /**
                 * @}
                 */
                /* SPRD : bug568587, new feature FM new UI @{ */
                // Stop or start recording
                mIsChangeRecord = true;
                if (!isStartRecording()) {
                    if (mService.isAudioRecording()) {
                        new AlertDialog.Builder(FmRecordActivity.this)
                        .setTitle(setSpString(getResources().getString(R.string.app_name)))
                        .setMessage(setSpString(getResources().getString(R.string.same_application_running)))
                        .setPositiveButton(R.string.button_ok, null)
                        .setCancelable(false)
                        .show();
                        return;
                }
                    mStopRecordButton.setEnabled(false);
                    mService.startRecordingAsync();
                    mShouldShowSaveDialog = true;
                } else {
                    mService.stopRecordingAsync();
                    mShouldShowSaveDialog = false;
                }
                /* @} */
            }
        });

        mPlayIndicator = (FmVisualizerView) findViewById(R.id.fm_play_indicator);

        if (savedInstanceState != null) {
            mCurrentStation = savedInstanceState.getInt(FmStation.CURRENT_STATION);
            mRecordState = savedInstanceState.getInt("last_record_state");
            /**
             * SPRD: bug522185, FM can not show save dialog after play video with horizontal mode.
             */
            mStateFromBundle = true;
            /**
             * @}
             */
        } else {
            Intent intent = getIntent();
            mCurrentStation = intent.getIntExtra(FmStation.CURRENT_STATION,
                    FmUtils.DEFAULT_STATION);
            mRecordState = intent.getIntExtra("last_record_state", FmRecorder.STATE_INVALID);
        }
        bindService(new Intent(this, FmService.class), mServiceConnection,
                Context.BIND_AUTO_CREATE);
        int id = updateUi();
        mContext.getContentResolver().registerContentObserver(
                ContentUris.withAppendedId(Station.CONTENT_URI, id), false, mContentObserver);
        registerFmBroadcastReceiver();
        /* SPRD : bug568587, new feature FM new UI @{ */
        mUserManager = (UserManager) this.getSystemService(Context.USER_SERVICE);
        mSettingDialogs = new Dialog[SETTING_DIALOG_COUNT];
        mStartString = String.valueOf(getResources().getString(R.string.record_start));
        mStopString = String.valueOf(getResources().getString(R.string.stop_record));
        mRecordeFilePathList = new ArrayList();
        /* @} */
    }

    private int updateUi() {
        // TODO it's on UI thread, change to sub thread
        ContentResolver resolver = mContext.getContentResolver();
        mFrequency.setText("FM " + FmUtils.formatStation(mCurrentStation));
        Cursor cursor = null;
        int id = 0;
        try {
            cursor = resolver.query(
                    Station.CONTENT_URI,
                    FmStation.COLUMNS,
                    Station.FREQUENCY + "=?",
                    new String[] {
                        String.valueOf(mCurrentStation)
                    },
                    null);
            if (cursor != null && cursor.moveToFirst()) {
                // If the station name does not exist, show program service(PS) instead
                String stationName = cursor.getString(cursor.getColumnIndex(Station.STATION_NAME));
                if (TextUtils.isEmpty(stationName)) {
                    stationName = cursor.getString(cursor.getColumnIndex(Station.PROGRAM_SERVICE));
                }
                String radioText = cursor.getString(cursor.getColumnIndex(Station.RADIO_TEXT));
                mStationName.setText(stationName);
                mRadioText.setText(radioText);
                id = cursor.getInt(cursor.getColumnIndex(Station._ID));
                // If no station name and no radio text, hide the view
                if ((!TextUtils.isEmpty(stationName))
                        || (!TextUtils.isEmpty(radioText))) {
                    mStationInfoLayout.setVisibility(View.VISIBLE);
                } else {
                    mStationInfoLayout.setVisibility(View.GONE);
                }
                Log.d(TAG, "updateUi, frequency = " + mCurrentStation + ", stationName = "
                        + stationName + ", radioText = " + radioText);
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        return id;
    }

    private void updateRecordingNotification(long recordTime) {
        if (mNotificationBuilder == null) {
            Intent intent = new Intent(FM_STOP_RECORDING);
            intent.setClass(mContext, FmRecordActivity.class);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            PendingIntent pendingIntent = PendingIntent.getActivity(mContext, 0, intent,
                    PendingIntent.FLAG_UPDATE_CURRENT);

            Bitmap largeIcon = FmUtils.createNotificationLargeIcon(mContext,
                    FmUtils.formatStation(mCurrentStation));
            mNotificationBuilder = new Builder(this)
                    .setContentText(getText(R.string.record_notification_message))
                    .setShowWhen(false)
                    .setAutoCancel(true)
                    .setSmallIcon(R.drawable.status_fm_record)
                    .setLargeIcon(largeIcon)
                    .addAction(R.drawable.btn_fm_rec_stop_enabled, getText(R.string.stop_record),
                            pendingIntent);

            Intent cIntent = new Intent(FM_ENTER_RECORD_SCREEN);
            cIntent.setClass(mContext, FmRecordActivity.class);
            cIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            PendingIntent contentPendingIntent = PendingIntent.getActivity(mContext, 0, cIntent,
                    PendingIntent.FLAG_UPDATE_CURRENT);
            mNotificationBuilder.setContentIntent(contentPendingIntent);
        }
        // Format record time to show on title
        /**
         * SPRD: bug515084, FM records over one hour, status bar's time will reset. Original Android
         * code: Date date = new Date(recordTime); SimpleDateFormat simpleDateFormat = new
         * SimpleDateFormat("mm:ss", Locale.ENGLISH); String time = simpleDateFormat.format(date);
         * 
         * @{
         */
        long TimeInSec = Math.round((double) recordTime / 1000D);
        String time = addPaddingForString(TimeInSec / TIME_BASE) + ":"
                + addPaddingForString(TimeInSec % TIME_BASE);
        /**
         * @}
         */

        mNotificationBuilder.setContentTitle(time);
        if (mService != null) {
            mService.showRecordingNotification(mNotificationBuilder.build());
        }
    }

    @Override
    public void onNewIntent(Intent intent) {
        if (intent != null && intent.getAction() != null) {
            String action = intent.getAction();
            if (FM_STOP_RECORDING.equals(action)) {
                // If click stop button in notification, need to stop recording
                if (mService != null && !isStopRecording()) {
                    mService.stopRecordingAsync();
                }
            } else if (FM_ENTER_RECORD_SCREEN.equals(action)) {
                // Just enter record screen, do nothing
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        mIsInBackground = false;
        if (null != mService) {
            mService.setFmRecordActivityForeground(true);
        }
        // Show save dialog if record has stopped and never show it before.
        /*
         * SPRD : bug568587, new feature FM new UI @{ if (isStopRecording() && !isSaveDialogShown())
         * { showSaveDialog(); }
         * @ }
         */
        /*SPRD:add for bug596258 Interrupted by other applications, recording without prompt save@{*/
        if (mIsShowDialogLater && isStopRecording() && !isSaveDialogShown()) {
            showSaveDialog();
        }
        /*@} */
        // Trigger to refreshing timer text if still in record
        if (isStartRecording()) {
            mHandler.removeMessages(FmListener.MSGID_REFRESH);
            mHandler.sendEmptyMessage(FmListener.MSGID_REFRESH);
        }
        // Clear notification, it only need show when in background
        removeNotification();

    }

    @Override
    protected void onPause() {
        super.onPause();
        mIsInBackground = true;
        if (null != mService) {
            mService.setFmRecordActivityForeground(false);
        }
        // Stop refreshing timer text
        mHandler.removeMessages(FmListener.MSGID_REFRESH);
        // Show notification when switch to background
        showNotification();
    }

    private void showNotification() {
        // If have stopped recording, need not show notification
        if (!isStopRecording()) {
            mHandler.sendEmptyMessage(MSG_UPDATE_NOTIFICATION);
        } else {
            // Only when save dialog is shown and FM radio is back to background,
            // it is necessary to update playing notification.
            // Otherwise, FmMainActivity will update playing notification.
            if (mService == null) {
                return;
            }
            mService.updatePlayingNotification();
        }
    }

    private void removeNotification() {
        mHandler.removeMessages(MSG_UPDATE_NOTIFICATION);
        if (mService != null) {
            mService.removeNotification();
            mService.updatePlayingNotification();
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        outState.putInt(FmStation.CURRENT_STATION, mCurrentStation);
        outState.putInt("last_record_state", mRecordState);
        outState.putBoolean("isSaveDialogShown", mIsSaveDialogShown);
        // SPRD: bug526250, re-enter FmRecordActivity and rotate screen,occur inner erro
        outState.putBoolean("isPermissionDialogShown", mIsPermissionDialogShown);
        outState.putBoolean("mShouldShowSaveDialog", mShouldShowSaveDialog);
        super.onSaveInstanceState(outState);
    }

    @Override
    protected void onDestroy() {
        removeNotification();
        mHandler.removeCallbacksAndMessages(null);
        if (mService != null) {
            mService.unregisterFmRadioListener(mFmListener);
        }
        unbindService(mServiceConnection);
        unregisterFmBroadcastReceiver();
        mContext.getContentResolver().unregisterContentObserver(mContentObserver);
        super.onDestroy();
    }

    /**
     * Recording dialog click
     * 
     * @param recordingName The new recording name
     */
    @Override
    public void onRecordingDialogClick(
            String recordingName) {
        // Happen when activity recreate, such as switch language
        if (mIsInBackground) {
            return;
        }

        mIsSaveDialogShown = false;
        if (FmRecorder.FM_RECORDING_TIME_KEY.equals(recordingName)) {
            showListenToast(true, null, getString(R.string.toast_record_not_saved_fortime));
            // SPRD: Modify for 652192,Fix problems about the scheme through earphone's button control fm.
            if (!isHeadset || mService.getPowerStatus() == mService.POWER_DOWN || mService.isMuted()) {
                finish();
            }
            return;
        }

        if (recordingName != null && mService != null) {
            boolean tmpfileExistBeforeSave = mService.isRecordingTmpFileExist();
            /* SPRD : bug568587, new feature FM new UI @{ */
            mService.saveRecordingAsync(recordingName);
            // returnResult(recordingName, getString(R.string.toast_record_saved));
            if (isHeadset) {
                if (tmpfileExistBeforeSave) {
                    showListenToast(true, recordingName, getString(R.string.toast_record_saved));
                } else {
                    showListenToast(false, recordingName, getString(R.string.toast_record_not_saved_tmpfile_deleted));
                }
            }
        } else {
            // returnResult(null, getString(R.string.toast_record_not_saved));
            if (isHeadset) {
                showListenToast(true, recordingName, getString(R.string.toast_record_not_saved));
            }
        }
        if (!isHeadset || mService.getPowerStatus() == mService.POWER_DOWN || mService.isMuted()) {
            finish();
        }
        /* @} */
    }

    /**
     * SPRD: bug519384 Fix IllegalStateException in monkey test.
     * 
     * @{
     */
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (!this.isResumed()) {
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    /**
     * @}
     */
    @Override
    public void onBackPressed() {
        if (mIsChangeRecord) return;
        if (mService != null & isStartRecording()) {
            // Stop recording and wait service notify stop record state to show dialog
            mService.stopRecordingAsync();
            return;
        }
        super.onBackPressed();
    }

    private final ServiceConnection mServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, android.os.IBinder service) {
            mService = ((FmService.ServiceBinder) service).getService();
            /**
             * SPRD: bug582477, for monkey test.
             * 
             * @{
             */
            if (null == mService) {
                Log.e(TAG, "onServiceConnected, mService is null");
                finish();
                return;
            }
            /**
             * @}
             */
            mService.registerFmRadioListener(mFmListener);
            mService.setFmRecordActivityForeground(!mIsInBackground);

            /**
             * SPRD: bug522185, FM can not show save dialog after play video with horizontal mode.
             */
            if (mStateFromBundle) {
                mStateFromBundle = false;
                // SPRD: bug526250, re-enter FmRecordActivity and rotate screen,occur inner error
                if (FmRecorder.STATE_RECORDING == mRecordState) {
                    mRecordState = mService.getRecorderState();
                }
            }
            /**
             * @}
             */
            // 1. If have stopped recording, we need check whether need show save dialog again.
            // Because when stop recording in background, we need show it when switch to foreground.
            if (isStopRecording() && mShouldShowSaveDialog) {
                if (!isSaveDialogShown()) {
                    showSaveDialog();
                }
                return;
            }
            // 2. If not start recording, start it directly, this case happen when start this
            // activity from main fm activity.
            if (!isStartRecording()) {
                /**
                 * SPRD: bug495859, request runtime permissions Original Android code:
                 * mService.startRecordingAsync();
                 * 
                 * @{
                 */
                /* SPRD: fix bug 526250 @{ */
                if (mIsPermissionDialogShown) {
                    Log.d(TAG, "already show PermissionDialog.");
                    return;
                }
                /* @} */
                if (!FmUtils.checkBuildRecordingPermission(FmRecordActivity.this)) {
                    // SPRD: bug526250, re-enter FmRecordActivity and rotate screen,occur inner
                    // error
                    mIsPermissionDialogShown = true;
                    return;
                } else {
                    /* SPRD : bug568587, new feature FM new UI @{ */
                    // mService.startRecordingAsync();
                    mStopRecordButton.setText(mStartString);
                    /* @} */
                }
                /**
                 * @}
                 */
            }
            /* SPRD : bug568587, new feature FM new UI @{ */
            // mPlayIndicator.startAnimation();
            mStopRecordButton.setEnabled(true);
            // mHandler.removeMessages(FmListener.MSGID_REFRESH);
            // mHandler.sendEmptyMessage(FmListener.MSGID_REFRESH);
            /* @} */
        };

        @Override
        public void onServiceDisconnected(android.content.ComponentName name) {
            mService = null;
        };
    };

    private String addPaddingForString(long time) {
        StringBuilder builder = new StringBuilder();
        if (time >= 0 && time < 10) {
            builder.append("0");
        }
        return builder.append(time).toString();
    }

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG,"mHandler.handleMessage, what = " + msg.what + ",hashcode:"
                            + mHandler.hashCode());
            switch (msg.what) {
                case FmListener.MSGID_REFRESH:
                    if (mService != null) {
                        long recordTimeInMillis = mService.getRecordTime();
                        long recordTimeInSec = Math.round((double) recordTimeInMillis / 1000D);
                        mMintues.setText(addPaddingForString(recordTimeInSec / TIME_BASE));
                        mSeconds.setText(addPaddingForString(recordTimeInSec % TIME_BASE));
                        checkStorageSpaceAndStop();
                    }
                    /**
                     * SPRD: bug490629, recording length increases jumping seconds. Original Android
                     * code: mHandler.sendEmptyMessageDelayed(FmListener.MSGID_REFRESH, 1000);
                     * 
                     * @{
                     */
                    mHandler.sendEmptyMessageDelayed(FmListener.MSGID_REFRESH, MSGID_REFRESH_DELAY);
                    /**
                     * @}
                     */
                    break;
                /* SPRD : bug568587, new feature FM new UI @{ */
                case MSG_CLEAR_TIMER_TEXT:
                    mMintues.setText(addPaddingForString(0));
                    mSeconds.setText(addPaddingForString(0));
                    /* @} */
                case MSG_UPDATE_NOTIFICATION:
                    if (mService != null) {
                        if (mIsInBackground && isStartRecording()) {
                            updateRecordingNotification(mService.getRecordTime());
                        }
                        mService.updatePlayingNotification();
                        checkStorageSpaceAndStop();
                    }
                    /**
                     * SPRD: bug490629, recording length increases jumping seconds. Original Android
                     * code: mHandler.sendEmptyMessageDelayed(MSG_UPDATE_NOTIFICATION, 1000);
                     * 
                     * @{
                     */
                    if (isStartRecording()) {
                        mHandler.sendEmptyMessageDelayed(MSG_UPDATE_NOTIFICATION,
                                MSGID_REFRESH_DELAY);
                    }
                    /**
                     * @}
                     */
                    break;

                case FmListener.LISTEN_RECORDSTATE_CHANGED:
                    // State change from STATE_INVALID to STATE_RECORDING mean begin recording
                    // State change from STATE_RECORDING to STATE_IDLE mean stop recording
                    /* SPRD : bug568587, new feature FM new UI @{ */
                    /**
                     * SPRD: bug582477, for monkey test.
                     * 
                     * @{
                     */
                    if (null == mService) {
                        Log.e(TAG, "handleMessage, mService is null");
                        return;
                    }
                    /**
                     * @}
                     */
                    mIsChangeRecord = false;
                    int newState = mService.getRecorderState();
                    updateSettingMenu(newState);
                    Log.d(TAG, "handleMessage, record state changed: newState = " + newState
                            + ", mRecordState = " + mRecordState);
                    if ((mRecordState == FmRecorder.STATE_INVALID
                            && newState == FmRecorder.STATE_RECORDING)
                            || (mRecordState == FmRecorder.STATE_IDLE
                            && newState == FmRecorder.STATE_RECORDING)) {
                        mPlayIndicator.startAnimation();
                        mStopRecordButton.setText(mStopString);
                        mStopRecordButton.setEnabled(true);
                        mRecordState = FmRecorder.STATE_RECORDING;
                        mHandler.sendEmptyMessage(FmListener.MSGID_REFRESH);
                    } else if (mRecordState == FmRecorder.STATE_RECORDING
                            && newState == FmRecorder.STATE_IDLE) {
                        mRecordState = FmRecorder.STATE_IDLE;
                        mHandler.removeMessages(FmListener.MSGID_REFRESH);
                        mHandler.sendEmptyMessage(MSG_CLEAR_TIMER_TEXT);
                        mPlayIndicator.stopAnimation();
                        mStopRecordButton.setText(mStartString);
                        showSaveDialog();
                    }
                    /* @ } */
                    break;

                case FmListener.LISTEN_RECORDERROR:
                    Bundle bundle = msg.getData();
                    int errorType = bundle.getInt(FmListener.KEY_RECORDING_ERROR_TYPE);
                    handleRecordError(errorType);
                    break;

                /* SPRD : bug568587, new feature FM new UI @{ */
                case FmListener.MSGID_SWITCH_ANTENNA:
                    bundle = msg.getData();
                    isHeadset = bundle.getBoolean(FmListener.KEY_IS_SWITCH_ANTENNA);
                    // if receive headset plugout, need set headset mode on ui
                    if (!isHeadset) {
                        if (mService != null & isStartRecording()) {
                            // Stop recording and wait service notify stop record state to show dialog
                            mService.stopRecordingAsync();
                        }
                    }
                    break;
                /* @} */
                /* SPRD: Fix for bug 589228 forbid turn on FM in airplane mode. @{ */
                case FmListener.MSGID_POWERDOWN_FINISHED:
                    //Modify for 642451,Start record fm after make fm close after 1 minute,the time in recrod activity is wrong.
                    if (!mIsSaveDialogShown && !mIsShowDialogLater) {
                        finish();
                    }
                    break;
                /* Bug 589228 End@} */
                /* SPRD: Fix for bug 596494 Remove and reconnect the OTG storage, the recording file save path still display OTG storage. @{ */
                case FmListener.MSGID_STORAGR_CHANGED:
                    if (mIsRecordFilePathDialogShown) {
                        removeDialog(RECORD_FILE_PATH_DIALOG);
                        showDialog(RECORD_FILE_PATH_DIALOG);
                    }
                    break;
                /* Bug 596494 End@} */
                case FmListener.MSGID_SET_MUTE_FINISHED:
                    // SPRD: Modify for 652192,Fix problems about the scheme through earphone's button control fm.
                    if (null != mService && isStartRecording()) {
                        mService.stopRecordingAsync();
                    } else if(mService.isMuted() && !mIsSaveDialogShown) {
                        finish();
                    }
                    break;
                default:
                    break;
            }
        };
    };

    private void checkStorageSpaceAndStop() {
        long recordTimeInMillis = mService.getRecordTime();
        long recordTimeInSec = recordTimeInMillis / 1000L;
        // Check storage free space
        String recordingSdcard = FmUtils.getDefaultStoragePath();
        /**
         * SPRD: bug474747, recording path selection.
         * 
         * @{
         */
        if (recordingSdcard == null
                || recordingSdcard.isEmpty()
                || (mSDCardPathName == FmUtils.FM_RECORD_STORAGE_PATH_NAME && !Environment.MEDIA_MOUNTED
                        .equals(EnvironmentEx.getExternalStoragePathState()))) {
            // Need to record more than 1s.
            // Avoid calling MediaRecorder.stop() before native record starts.
            if (recordTimeInSec >= 1) {
                // Insufficient storage
                mService.stopRecordingAsync();
                Toast.makeText(FmRecordActivity.this,
                        R.string.toast_storage_device_missing,
                        Toast.LENGTH_SHORT).show();
            }
            return;
        }
        /**
         * @}
         */

        if (!FmUtils.hasEnoughSpace(recordingSdcard)) {
            // Need to record more than 1s.
            // Avoid calling MediaRecorder.stop() before native record starts.
            if (recordTimeInSec >= 1) {
                // Insufficient storage
                Log.d(TAG, "hasNoEnoughSpace");
                mService.stopRecordingAsync();
                Toast.makeText(FmRecordActivity.this,
                        R.string.toast_sdcard_insufficient_space,
                        Toast.LENGTH_SHORT).show();
            }
        }
    }

    private void handleRecordError(int errorType) {
        Log.d(TAG, "handleRecordError, errorType = " + errorType);
        String showString = null;
        switch (errorType) {
            case FmRecorder.ERROR_SDCARD_NOT_PRESENT:
                showString = getString(R.string.toast_storage_device_missing);
                returnResult(null, showString);
                finish();
                break;

            case FmRecorder.ERROR_SDCARD_INSUFFICIENT_SPACE:
                showString = getString(R.string.toast_sdcard_insufficient_space);
                returnResult(null, showString);
                finish();
                break;

            case FmRecorder.ERROR_RECORDER_INTERNAL:
                showString = getString(R.string.toast_recorder_internal_error);
                Toast.makeText(mContext, showString, Toast.LENGTH_SHORT).show();
                break;

            case FmRecorder.ERROR_SDCARD_WRITE_FAILED:
                showString = getString(R.string.toast_recorder_internal_error);
                returnResult(null, showString);
                finish();
                break;

            /* SPRD: fix bug 539563 FM shouldn't record external sound. @{ */
            case FmRecorder.ERROR_RECORD_FAILED:
                showString = getString(R.string.toast_play_fm);
                returnResult(null, showString);
                finish();
                break;
            /* @} */
            default:
                Log.w(TAG, "handleRecordError, invalid record error");
                break;
        }
    }

    private void returnResult(String recordName, String resultString) {
        Intent intent = new Intent();
        intent.putExtra(FmMainActivity.EXTRA_RESULT_STRING, resultString);
        if (recordName != null) {
            intent.setData(Uri.parse("file://"
                    + FmService.getRecordingSdcard()
                    + File.separator
                    + FmRecorder.FM_RECORD_FOLDER
                    + File.separator
                    + Uri.encode(recordName)
                    + (1 == FmRecorder.GLOBAL_RECORD_FORMAT_FLAG ? FmRecorder.RECORDING_FILE_EXTENSION
                            : FmRecorder.RECORDING_FILE_EXTENSION_AMR)));
        }
        setResult(RESULT_OK, intent);
    }

    private final ContentObserver mContentObserver = new ContentObserver(new Handler()) {
        public void onChange(boolean selfChange) {
            updateUi();
        };
    };

    // Service listener
    private final FmListener mFmListener = new FmListener() {
        @Override
        public void onCallBack(Bundle bundle) {
            int flag = bundle.getInt(FmListener.CALLBACK_FLAG);
            if (flag == FmListener.MSGID_FM_EXIT) {
                mHandler.removeCallbacksAndMessages(null);
            }

            // remove tag message first, avoid too many same messages in queue.
            Message msg = mHandler.obtainMessage(flag);
            msg.setData(bundle);
            mHandler.removeMessages(flag);
            mHandler.sendMessage(msg);
        }
    };

    /**
     * Show save record dialog
     */
    public void showSaveDialog() {
        /*
         * SPRD : bug568587, new feature FM new UI @{ if (mIsSaveDialogShown) { return; }
         * @}
         */
        removeNotification();
        if (mIsInBackground) {
            mIsShowDialogLater = true;
            Log.d(TAG, "showSaveDialog, activity is in background, show it later");
            return;
        }
        mIsShowDialogLater = false;
        /**
         * SPRD: bug582477, for monkey test.
         * 
         * @{
         */
        if (mService == null) {
            Log.e(TAG, "showSaveDialog, service is null");
            return;
        }
        /**
         * @}
         */
        String sdcard = FmService.getRecordingSdcard();
        String recordingName = mService.getRecordingName();
        String saveName = null;
        if (TextUtils.isEmpty(mStationName.getText())) {
            saveName = FmRecorder.RECORDING_FILE_PREFIX + "_" + recordingName;
        } else {
            saveName = FmRecorder.RECORDING_FILE_PREFIX + "_" + mStationName.getText() + "_"
                    + recordingName;
        }
        FmSaveDialog newFragment = new FmSaveDialog(sdcard, recordingName, saveName);
        newFragment.show(mFragmentManager, TAG_SAVE_RECORDINGD);
        /**
         * SPRD: bug509670, recording time is too short, causing music player anr.
         * 
         * @{
         */
        long recordingTime = mService.getRecordTime();
        Log.d(TAG, "showSaveDialog: recordingTime=" + recordingTime);
        newFragment.setCancelable(false);
        Bundle args = new Bundle();
        args.putLong(FmRecorder.FM_RECORDING_TIME_KEY, recordingTime);
        newFragment.setArguments(args);
        /**
         * @}
         */
        mFragmentManager.executePendingTransactions();
        mIsSaveDialogShown = true;
        mHandler.removeMessages(FmListener.MSGID_REFRESH);
    }

    private boolean isStartRecording() {
        return mRecordState == FmRecorder.STATE_RECORDING;
    }

    private boolean isStopRecording() {
        return mRecordState == FmRecorder.STATE_IDLE;
    }

    private boolean isSaveDialogShown() {
        FmSaveDialog saveDialog = (FmSaveDialog)
                mFragmentManager.findFragmentByTag(TAG_SAVE_RECORDINGD);
        return saveDialog != null;
    }

    /**
     * SPRD: bug516973, High-power radio recording
     * 
     * @{
     */
    private FmRcorderBroadcastReceiver mBroadcastReceiver = null;

    private void unregisterFmBroadcastReceiver() {
        if (null != mBroadcastReceiver) {
            unregisterReceiver(mBroadcastReceiver);
            mBroadcastReceiver = null;
        }
    }

    private void registerFmBroadcastReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        mBroadcastReceiver = new FmRcorderBroadcastReceiver();
        registerReceiver(mBroadcastReceiver, filter);
    }

    private class FmRcorderBroadcastReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context arg0, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_SCREEN_ON.equals(action)) {
                Log.d(TAG, "ACTION_SCREEN_ON");
                if (!isStopRecording()) {
                    mHandler.sendEmptyMessage(MSG_UPDATE_NOTIFICATION);
                }
            } else if (Intent.ACTION_SCREEN_OFF.equals(action)) {
                Log.d(TAG, "ACTION_SCREEN_OFF");
                mHandler.removeMessages(MSG_UPDATE_NOTIFICATION);
            }
        }

    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
            String permissions[], int[] grantResults) {
        Log.d(TAG, "onRequestPermissionsResult");
        switch (requestCode) {
            case FmUtils.FM_PERMISSIONS_REQUEST_CODE: {
                boolean resultsAllGranted = true;
                if (grantResults.length > 0) {
                    for (int result : grantResults) {
                        if (PackageManager.PERMISSION_GRANTED != result) {
                            resultsAllGranted = false;
                        }
                    }
                } else {
                    resultsAllGranted = false;
                }
                if (resultsAllGranted) {
                    // SPRD: bug539563, FM shouldn't record external sound.
                    if (null != mService) {
                        /* SPRD: fix bug 545340 FM shouldn't show prompt when FM is playing. @{ */
                        if (mService.mIsAudioFocusHeld) {
                            /* SPRD : bug568587, new feature FM new UI @{ */
                            // mService.startRecordingAsync();
                            // mPlayIndicator.startAnimation();
                            mStopRecordButton.setEnabled(true);
                            mStopRecordButton.setText(mStartString);
                            mHandler.removeMessages(FmListener.MSGID_REFRESH);
                            // mHandler.sendEmptyMessage(FmListener.MSGID_REFRESH);
                            mHandler.sendEmptyMessage(MSG_CLEAR_TIMER_TEXT);
                            /* @} */
                        } else {
                            if (mService.isMuted()) {
                                handleRecordError(FmRecorder.ERROR_RECORD_FAILED);
                            } else {
                                finish();
                            }
                        }
                        /* @} */
                    }
                } else {
                    handleRecordError(FmRecorder.ERROR_SDCARD_WRITE_FAILED);
                }
            }
        }
        // SPRD: bug526250, re-enter FmRecordActivity and rotate screen,occur inner error
        mIsPermissionDialogShown = false;
    }

    /* SPRD: bug568587 FM new UI */
    private MenuItem mMenuItemRecordFormat = null;
    private MenuItem mMenuItemRecordPath = null;
    private boolean isHeadset = true;
    private static final int SETTING_DIALOG_COUNT = 2;
    private static Dialog[] mSettingDialogs;
    private static final int RECORD_FORMAT_DIALOG = 0;
    private static final int RECORD_FILE_PATH_DIALOG = 1;
    private static final int MSG_CLEAR_TIMER_TEXT = 1001;
    private static final String FM_RECORD_FORMAT = "FM_RECORD_FORMAT";
    private static final String DEFAULT_FORMAT = "default_format";
    private static final String FM_RECORD_STORAGE = "fm_record_storage";
    private static final String FM_RECORD_DEFAULT_PATH = "default_path";
    // SPRD:add feature for support OTG.
    private static final String FM_RECORD_DEFAULT_PATH_NAME = "default_path_name";
    private String mRecordInternalStorage;
    private String mRecordExternalStorage;
    private String mStartString;
    private String mStopString;
    private static List mRecordeFilePathList = null;
    private UserManager mUserManager;
    private boolean mIsRecordFilePathDialogShown = false;

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.fm_record_set_menu, menu);
        mMenuItemRecordFormat = menu.findItem(R.id.fm_record_format);
        mMenuItemRecordPath = menu.findItem(R.id.fm_record_path);
        resetMenuTitleColor(true);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                onBackPressed();
                break;
            case R.id.fm_record_format:
                showDialog(RECORD_FORMAT_DIALOG);
                break;
            case R.id.fm_record_path:
                removeDialog(RECORD_FILE_PATH_DIALOG);
                showDialog(RECORD_FILE_PATH_DIALOG);
                mIsRecordFilePathDialogShown = true;
                break;
            default:
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    private void resetMenuTitleColor(boolean enabled) {
        mMenuItemRecordFormat.setTitle(setSpString(mMenuItemRecordFormat, enabled));
        mMenuItemRecordPath.setTitle(setSpString(mMenuItemRecordPath, enabled));
    }

    private SpannableString setSpString(MenuItem item, boolean enabled) {
        SpannableString itemTitle = new SpannableString(item.getTitle());
        int titleColor = enabled ? getResources().getColor(R.color.actionbar_overflow_title_color)
                : getResources().getColor(R.color.actionbar_overflow_title_disabled_color);
        itemTitle.setSpan(new ForegroundColorSpan(titleColor), 0, itemTitle.length(), 0);
        return itemTitle;
    }

    private SpannableString setSpString(String string) {
        SpannableString title = new SpannableString(string);
        int titleColor = getResources().getColor(R.color.actionbar_overflow_title_color);
        title.setSpan(new ForegroundColorSpan(titleColor), 0, title.length(), 0);
        return title;
    }

    private void saveRecordDefaultPath() {
        SharedPreferences storageSP = mContext.getSharedPreferences(FM_RECORD_STORAGE,
                Context.MODE_PRIVATE);
        SharedPreferences.Editor storageEdit = storageSP.edit();
        storageEdit.putInt(FM_RECORD_DEFAULT_PATH, FmUtils.FM_RECORD_STORAGE_PATH);
        // SPRD:add feature for support OTG.
        storageEdit.putString(FM_RECORD_DEFAULT_PATH_NAME, FmUtils.FM_RECORD_STORAGE_PATH_NAME);
        storageEdit.commit();
    }

    private void updateSettingMenu(int newState) {
        if (newState == FmRecorder.STATE_IDLE) {
            mMenuItemRecordFormat.setEnabled(true);
            mMenuItemRecordPath.setEnabled(true);
            resetMenuTitleColor(true);
        } else {
            mMenuItemRecordFormat.setEnabled(false);
            mMenuItemRecordPath.setEnabled(false);
            resetMenuTitleColor(false);
        }
    }

    protected Dialog onCreateDialog(int id) {
        switch (id) {
            case RECORD_FORMAT_DIALOG:
                mSettingDialogs[RECORD_FORMAT_DIALOG] = new AlertDialog.Builder(this)
                        .setTitle(setSpString(getResources().getString(R.string.select_file_type)))
                        .setSingleChoiceItems(new String[] {
                                String.valueOf(getResources().getString(R.string.record_amr)),
                                String.valueOf(getResources().getString(R.string.record_3gpp))
                        }, FmRecorder.GLOBAL_RECORD_FORMAT_FLAG,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog, int which) {
                                        switch (which) {
                                            case 0:
                                                FmRecorder.GLOBAL_RECORD_FORMAT_FLAG = which;
                                                break;
                                            case 1:
                                                FmRecorder.GLOBAL_RECORD_FORMAT_FLAG = which;
                                                break;
                                            default:
                                        }
                                        SharedPreferences formatSP = mContext.getSharedPreferences(
                                                FM_RECORD_FORMAT,
                                                Context.MODE_PRIVATE);
                                        SharedPreferences.Editor edit = formatSP.edit();
                                        edit.putInt(DEFAULT_FORMAT,
                                                FmRecorder.GLOBAL_RECORD_FORMAT_FLAG);
                                        edit.commit();
                                        dialog.cancel();
                                    }
                                })
                        .setNegativeButton(R.string.button_cancel,
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        dialog.cancel();
                                    }
                                }).create();
                mSettingDialogs[RECORD_FORMAT_DIALOG].setCanceledOnTouchOutside(false);
                return mSettingDialogs[RECORD_FORMAT_DIALOG];
            case RECORD_FILE_PATH_DIALOG:
                if (!Environment.MEDIA_MOUNTED.equals(EnvironmentEx.getExternalStoragePathState())) {
                    mRecordeFilePathList.add(mRecordInternalStorage);
                } else {
                    mRecordeFilePathList.add(mRecordInternalStorage);
                    if (mUserManager.isSystemUser()) {
                        mRecordeFilePathList.add(mRecordExternalStorage);
                        Log.i(TAG, "is  SystemUser");
                    } else {
                        Log.i(TAG, "is not SystemUser");
                    }
                }
                /* SPRD:add feature for support OTG. @{ */
                /* get USB devices list */
                StorageManager storageManager = FmRecordActivity.this
                        .getSystemService(StorageManager.class);
                List<VolumeInfo> volumes = storageManager.getVolumes();
                Collections.sort(volumes, VolumeInfo.getDescriptionComparator());
                String savedPathName = getSavedRecordPathName();
                boolean hasSavedUsbDevice = false;
                for (VolumeInfo vol : volumes) {
                    File file = vol.getPath();
                    /* SPRD: fix bug 591500 device cann't detecte OTG storage when SD card is not inserted in device. @{ */
                    if (file != null && Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(file))) {
                        String usbDeviceName ="";
                        if (vol.disk != null && vol.disk.isUsb()) {
                            usbDeviceName = storageManager.getBestVolumeDescription(vol);
                            mRecordeFilePathList.add(usbDeviceName);
                        }
                   /* SPRD:bug 591500 end */
                        if (usbDeviceName.equals(savedPathName)) {
                            hasSavedUsbDevice = true;
                            FmUtils.FM_RECORD_STORAGE_PATH = mRecordeFilePathList.size() - 1;
                            FmUtils.FM_RECORD_STORAGE_PATH_NAME = usbDeviceName;
                            saveRecordDefaultPath();
                        }
                    }
                }

                if (!hasSavedUsbDevice) {
                    if (!Environment.MEDIA_MOUNTED.equals(EnvironmentEx
                            .getExternalStoragePathState())) {
                        FmUtils.FM_RECORD_STORAGE_PATH = FmUtils.STORAGE_PATH_INTERNAL_CATEGORY;
                        FmUtils.FM_RECORD_STORAGE_PATH_NAME = mRecordInternalStorage;
                        saveRecordDefaultPath();
                    } else {
                        if (!mUserManager.isSystemUser()) {
                            FmUtils.FM_RECORD_STORAGE_PATH = FmUtils.STORAGE_PATH_INTERNAL_CATEGORY;
                            FmUtils.FM_RECORD_STORAGE_PATH_NAME = mRecordInternalStorage;
                            saveRecordDefaultPath();
                            Log.i(TAG, "is not SystemUser..");
                        } else {
                            int path = getSavedRecordPath();
                            /*
                             * last time save USB as default storage,but this time can not find that
                             * USB device
                             */
                            if (path == FmUtils.STORAGE_PATH_USB_CATEGORY) {
                                FmUtils.FM_RECORD_STORAGE_PATH = FmUtils.STORAGE_PATH_EXTERNAL_CATEGORY;
                                FmUtils.FM_RECORD_STORAGE_PATH_NAME = mRecordExternalStorage;
                                saveRecordDefaultPath();
                            }
                        }
                    }
                }
                /* @} */
                final ArrayList<String> list = new ArrayList<String>(mRecordeFilePathList);
                mSettingDialogs[RECORD_FILE_PATH_DIALOG] = new AlertDialog.Builder(this)
                        .setTitle(setSpString(getResources().getString(R.string.select_file_path)))
                        .setSingleChoiceItems(
                                (String[]) (mRecordeFilePathList.toArray(new String[mRecordeFilePathList
                                        .size()]))
                                , FmUtils.FM_RECORD_STORAGE_PATH,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog, int which) {
                                        FmUtils.FM_RECORD_STORAGE_PATH = which;
                                        FmUtils.FM_RECORD_STORAGE_PATH_NAME = (String) list
                                                .get(which);
                                        saveRecordDefaultPath();
                                        dialog.cancel();
                                        mIsRecordFilePathDialogShown = false;
                                    }
                                })
                        .setNegativeButton(R.string.button_cancel,
                                new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        dialog.cancel();
                                        mIsRecordFilePathDialogShown = false;
                                    }
                                }).create();
                mSettingDialogs[RECORD_FILE_PATH_DIALOG].setCanceledOnTouchOutside(false);
                mRecordeFilePathList.clear();
                return mSettingDialogs[RECORD_FILE_PATH_DIALOG];
            default:
                return null;
        }
    }

    private void showListenToast(boolean recordFileExist, String recordName, String resultString) {
        Uri uri = null;
        if (recordName != null) {
            uri = Uri
                    .parse("file://"
                            + FmService.getRecordingSdcard()
                            + File.separator
                            + FmRecorder.FM_RECORD_FOLDER
                            + File.separator
                            + Uri.encode(recordName)
                            + (1 == FmRecorder.GLOBAL_RECORD_FORMAT_FLAG ? FmRecorder.RECORDING_FILE_EXTENSION
                                    : FmRecorder.RECORDING_FILE_EXTENSION_AMR));
        }
        final Uri playUri = uri;
        boolean isSaved = (playUri != null && recordFileExist);
        String title = resultString;
        String action = null;
        FmSnackBar.OnActionTriggerListener listener = null;
        if (isSaved) {
            action = FmRecordActivity.this.getString(R.string.toast_listen);
            listener = new FmSnackBar.OnActionTriggerListener() {
                @Override
                public void onActionTriggered() {
                    try {
                        Intent playRecordIntent = new Intent(FmRecordActivity.this,
                                FmRecordListActivity.class);
                        playRecordIntent.setData(playUri);
                        startActivity(playRecordIntent);
                        finish();
                    } catch (ActivityNotFoundException e2) {
                        Log.d(TAG, "onActivityResult, no activity "
                                + "respond play record file intent");
                    }
                }
            };
        }
        FmSnackBar.make(FmRecordActivity.this, title, action, listener,
                FmSnackBar.DEFAULT_DURATION).show();
    }

    /* SPRD:add feature for support OTG. @{ */
    private String getSavedRecordPathName() {
        SharedPreferences storageSP = this.getSharedPreferences(FM_RECORD_STORAGE,
                Context.MODE_PRIVATE);
        String pathName = storageSP.getString(FM_RECORD_DEFAULT_PATH_NAME, mRecordInternalStorage);
        return pathName;
    }

    private int getSavedRecordPath() {
        SharedPreferences storageSP = this.getSharedPreferences(FM_RECORD_STORAGE,
                Context.MODE_PRIVATE);
        int path = storageSP.getInt(FM_RECORD_DEFAULT_PATH, 0);
        return path;
    }
    /* @} */
    /* SPRD: bug568587 FM new UI */
}
