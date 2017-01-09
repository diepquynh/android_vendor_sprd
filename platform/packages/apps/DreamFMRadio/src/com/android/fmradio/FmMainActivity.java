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
import android.app.FragmentManager;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.res.Resources;
import android.database.Cursor;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import android.view.animation.AnimationUtils;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.Toolbar;
/**
 * SPRD:
 *
 * @{
 */
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.pm.PackageManager;
import android.Manifest;
import android.text.Editable;
import android.text.Selection;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.widget.EditText;
import android.content.SharedPreferences;
import android.database.sqlite.SQLiteConstraintException;
import com.android.fmradio.dialogs.FmFavoriteFreqEditDialog;
import com.android.fmradio.FmRecorder;
import com.android.fmradio.FmService.TimeCountListener;
import java.util.List;
import java.util.ArrayList;
import android.os.Environment;
import android.os.EnvironmentEx;
import android.os.SystemProperties;
import android.os.UserManager;
/**
 * @}
 */

import com.android.fmradio.FmStation.Station;
import com.android.fmradio.dialogs.FmFavoriteEditDialog;
import com.android.fmradio.views.FmScroller;
import com.android.fmradio.views.FmSnackBar;
import com.android.fmradio.views.FmScroller.EventListener;

import java.lang.reflect.Field;
import java.util.Locale;
import android.provider.Settings;

/**
 * This class interact with user, provide FM basic function.
 */
/**
 * Bug545627 Add RDS feature for brcm. Original code: public class FmMainActivity extends Activity
 * implements FmFavoriteEditDialog.EditFavoriteListener,
 * FmFavoriteFreqEditDialog.EditFavoriteFreqListener{
 * 
 * @{
 */
public class FmMainActivity extends Activity implements FmFavoriteEditDialog.EditFavoriteListener,
        FmFavoriteFreqEditDialog.EditFavoriteFreqListener, FmBrcmUtil.FMStateChangeListener {
    /**
     * @}
     */
    // Logging
    private static final String TAG = "FmMainActivity";
    // Request code
    private static final int REQUEST_CODE_FAVORITE = 1;

    public static final int REQUEST_CODE_RECORDING = 2;

    /**
     * SPRD: bug474717, input frequency.
     * 
     * @{
     */
    private static final int NO_DIALOG = -1;
    private static final int INPUT_DIALOG = 0;
    // SPRD: bug568587, Regularly power off, AirPlane mode,change DIALOG_COUNT 3 to 5.
    private static final int DIALOG_COUNT = 5;
    private static Dialog[] mDialogs;
    /**
     * @}
     */

    /**
     * SPRD: bug474741, recording format selection.
     * 
     * @{
     */
    private static final int RECORD_FORMAT_DIALOG = 1;
    /**
     * @}
     */

    /**
     * SPRD: bug474747, recording path selection.
     * 
     * @{
     */
    private static final int RECORD_FILE_PATH_DIALOG = 2;
    /**
     * @}
     */
    private static final int AIRPLANE_DIALOG = 4;
    // Extra for result of request REQUEST_CODE_RECORDING
    public static final String EXTRA_RESULT_STRING = "result_string";

    // FM
    private static final String FM = "FM";

    /**
     * SPRD: bug474728, new feature, preset station.
     * 
     * @{
     */
    private static final String USER_PRESET_STATIONS = "fm_stations_user_preset";
    private static final String SET_FLAG = "fm_stations_preset_flag";
    /**
     * @}
     */

    /**
     * SPRD: bug474741, recording format selection.
     * 
     * @{
     */
    private static final String FM_RECORD_FORMAT = "FM_RECORD_FORMAT";
    private static final String DEFAULT_FORMAT = "default_format";
    /**
     * @}
     */

    /**
     * SPRD: bug474747, recording path selection.& bug524632 check external storage status.
     * 
     * @{
     */
    private static final String FM_RECORD_STORAGE = "fm_record_storage";
    private static final String FM_RECORD_DEFAULT_PATH = "default_path";
    private String mRecordInternalStorage;
    private String mRecordExternalStorage;
    private static List mRecordeFilePathList = null;
    /**
     * @}
     */

    private static boolean mIsUseBrcmFmChip = SystemProperties.getBoolean(
            "ro.fm.chip.port.UART.androidm", false);

    // UI views
    private TextView mTextStationName = null;

    private TextView mTextStationValue = null;

    // RDS text view
    private TextView mTextRds = null;

    private TextView mActionBarTitle = null;

    private TextView mNoEarPhoneTxt = null;

    private ImageButton mButtonDecrease = null;

    private ImageButton mButtonPrevStation = null;

    private ImageButton mButtonNextStation = null;

    private ImageButton mButtonIncrease = null;

    private ImageButton mButtonAddToFavorite = null;

    private ImageButton mButtonPlay = null;

    private ImageView mNoHeadsetImgView = null;

    private View mNoHeadsetImgViewWrap = null;

    private float mMiddleShadowSize;

    private LinearLayout mMainLayout = null;

    private RelativeLayout mNoHeadsetLayout = null;

    private LinearLayout mNoEarphoneTextLayout = null;

    private LinearLayout mBtnPlayInnerContainer = null;

    private LinearLayout mBtnPlayContainer = null;

    // Menu
    private Menu mMenu = null;

    // Menu items
    private MenuItem mMenuItemStationlList = null;

    private MenuItem mMenuItemHeadset = null;

    private MenuItem mMenuItemStartRecord = null;

    // SPRD : bug568587, new feature FM new UI
    private MenuItem mMenuItemRecord = null;

    /**
     * SPRD: bug474741, recording format selection.
     * 
     * @{
     */
    private MenuItem mMenuItemRecordFormat = null;
    /**
     * @}
     */

    private MenuItem mMenuItemRecordList = null;

    /**
     * SPRD: bug474747, recording path selection.
     * 
     * @{
     */
    private MenuItem mMenuItemRecordPath = null;
    /**
     * @}
     */
    private MenuItem mMenuItemRDSSwitch = null;
    private MenuItem mMenuItemAFSwitch = null;
    // State variables
    private boolean mIsServiceStarted = false;

    private boolean mIsServiceBinded = false;

    private boolean mIsTune = false;

    private boolean mIsDisablePowerMenu = false;

    private boolean mIsActivityForeground = true;

    private int mCurrentStation = FmUtils.DEFAULT_STATION;
    // Instance variables
    private FmService mService = null;

    private Context mContext = null;

    private Toast mToast = null;

    private FragmentManager mFragmentManager = null;

    private AudioManager mAudioManager = null;

    private UserManager mUserManager;

    private FmScroller mScroller;

    private FmScroller.EventListener mEventListener;
    /**
     * Bug545627 Add RDS feature for brcm.
     * 
     * @{
     */
    private FmBrcmUtil mFmBrcmUtil = FmBrcmUtil.getUtil();
    /**
     * @}
     */

    /** SPRD: bug568587, Regularly power off.@{ */
    private TextView mCustomTime = null;
    private MenuItem mMenuItemRegularlyPoweroff = null;
    private static final String FM_REGULAYLY_POWER_OFF = "fm_regularly_poweroff";
    private static final String DEFAULT_CUSTOM_SETTING = "default_custom_setting";
    private static final int REGULAYLY_POWER_OFF = 3;
    private EditText mCustomEdit = null;
    private LinearLayout mEditContainer = null;
    /** @} */

    // Service listener
    private FmListener mFmRadioListener = new FmListener() {
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

    // Button click listeners on UI
    private final View.OnClickListener mButtonClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            switch (v.getId()) {

                case R.id.button_add_to_favorite:
                    updateFavoriteStation();
                    break;

                case R.id.button_decrease:
                    tuneStation(FmUtils.computeDecreaseStation(mCurrentStation));
                    break;

                case R.id.button_increase:
                    tuneStation(FmUtils.computeIncreaseStation(mCurrentStation));
                    break;

                case R.id.button_prevstation:
                    seekStation(mCurrentStation, false); // false: previous station
                    break;

                case R.id.button_nextstation:
                    seekStation(mCurrentStation, true); // true: previous station
                    break;

                case R.id.play_button:
                    /**
                     * SPRD: bug500046, for monkey test.
                     * 
                     * @{
                     */
                    if (null == mService) {
                        Log.e(TAG, "onClick case playbutton, mService is null");
                        return;
                    }
                    /**
                     * @}
                     */
                    if (mService.getPowerStatus() == FmService.POWER_UP) {
                        if(mService.isMuted()){
                            mService.setMuteAsync(false);
                        }else{
                            powerDownFm();
                        }
                    } else {
                        powerUpFm();
                    }
                    break;
                default:
                    Log.d(TAG, "mButtonClickListener.onClick, invalid view id");
                    break;
            }
        }
    };

    /**
     * SPRD: bug474717, input frequency.
     * 
     * @{
     */
    private final View.OnClickListener mTextViewClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            showDialog(INPUT_DIALOG);
        }
    };

    /**
     * Main thread handler to update UI
     */
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            Log.d(TAG,
                    "mHandler.handleMessage, what = " + msg.what + ",hashcode:"
                            + mHandler.hashCode());
            Bundle bundle;
            switch (msg.what) {
            /**
             * SPRD: bug514631, hands-free (headset) in FM Radio app
             * 
             * @{
             */
                case FmListener.MSGID_FM_HEADSET_NEXT:
                    refreshImageButton(false);
                    refreshActionMenuItem(false);
                    refreshPopupMenuItem(false);
                    refreshPlayButton(false);
                    break;
                /**
                 * @}
                 */
                case FmListener.MSGID_POWERUP_FINISHED:
                    bundle = msg.getData();
                    boolean isPowerup = (mService.getPowerStatus() == FmService.POWER_UP);
                    int station = bundle.getInt(FmListener.KEY_TUNE_TO_STATION);
                    mCurrentStation = station;
                    refreshStationUI(station);
                    if (isPowerup) {
                        removeDialog(AIRPLANE_DIALOG);
                        refreshImageButton(true);
                        refreshPopupMenuItem(true);
                        refreshActionMenuItem(true);
                        /**
                         * SPRD: bug513374,when set speaker mode,the icon is headset
                         * 
                         * @{
                         */
                        if (mMenuItemHeadset != null) {
                            mMenuItemHeadset.setIcon(mService.isSpeakerUsed()
                                    ? R.drawable.btn_fm_speaker_selector
                                    : R.drawable.btn_fm_headset_selector);
                        }
                        /**
                         * @}
                         */
                    } else {
                        showToast(getString(R.string.not_available));
                    }
                    // if not powerup success, refresh power to enable.
                    refreshPlayButton(true);
                    // SPRD: Modify for 652192,Fix problems about the scheme through earphone's button control fm.
                    if (mMenu != null) {
                        mMenu.close();
                    }
                    break;
                    /* SPRD:Bug 607554 FM continue to play after stopped with hook key and press DUT volume key. @{ */
                case FmListener.MSGID_SET_MUTE_FINISHED:
                    boolean ismute = mService.isMuted();
                    if(mService.getPowerStatus() == FmService.POWER_UP){
                        Log.d(TAG,"set mute:ismute =" +ismute);
                    }
                    if (ismute) {
                        refreshImageButton(false);
                        refreshActionMenuItem(false);
                        refreshPopupMenuItem(false);
                    }else{
                        refreshImageButton(true);
                        refreshActionMenuItem(true);
                        refreshPopupMenuItem(true);
                    }
                    mButtonPlay.setEnabled(!mService.isScanning());
                    mButtonPlay.setImageResource(((!ismute)
                                 ? R.drawable.btn_fm_stop_selector
                                 : R.drawable.btn_fm_start_selector));
                    mScroller.refreshPlayIndicator(mCurrentStation, !ismute);
                    // SPRD: Modify for 652192,Fix problems about the scheme through earphone's button control fm.
                    if (mMenu != null) {
                        mMenu.close();
                    }
                    break;
                    /* Bug 607554 End@} */

                case FmListener.MSGID_SWITCH_ANTENNA:
                    bundle = msg.getData();
                    boolean hasAntenna = bundle.getBoolean(FmListener.KEY_IS_SWITCH_ANTENNA);
                    // if receive headset plug out, need set headset mode on ui
                    /* SPRD : bug568587, new feature FM new UI @{ */
                    if (mDialogs[REGULAYLY_POWER_OFF] != null
                            && mDialogs[REGULAYLY_POWER_OFF].isShowing()) {
                        mDialogs[REGULAYLY_POWER_OFF].dismiss();
                    }
                    /* @} */
                    if (hasAntenna) {
                        if (mIsActivityForeground) {
                            cancelNoHeadsetAnimation();
                            playMainAnimation();
                        } else {
                            changeToMainLayout();
                        }
                        setMenuVisibility(true);
                    } else {
                        if (mMenu != null) {
                            mMenu.close();
                        }
                        /**
                         * SPRD: bug515225, use mMenuItemHeadset before onCreateOptionMenu, case
                         * null-pointer.
                         * 
                         * @{
                         */
                        if (mMenuItemHeadset != null) {
                            mMenuItemHeadset.setIcon(R.drawable.btn_fm_headset_selector);
                        }
                        /**
                         * @}
                         */
                        if (mIsActivityForeground) {
                            cancelMainAnimation();
                            playNoHeadsetAnimation();
                        } else {
                            changeToNoHeadsetLayout();
                        }
                        mScroller.dismissPopupMenu();
                        setMenuVisibility(false);
                    }
                    break;

                case FmListener.MSGID_POWERDOWN_FINISHED:
                    bundle = msg.getData();
                    refreshImageButton(false);
                    refreshActionMenuItem(false);
                    refreshPopupMenuItem(false);
                    refreshPlayButton(true);
                    // SPRD: Bug602573 FM haven't stop timer power down after power down fm.
                    if (mService != null) {
                        mService.stopTimer();
                    }
                    // SPRD: Modify for 652192,Fix problems about the scheme through earphone's button control fm.
                    if (mMenu != null) {
                        mMenu.close();
                    }
                    break;

                case FmListener.MSGID_TUNE_FINISHED:
                    bundle = msg.getData();
                    boolean isTune = bundle.getBoolean(FmListener.KEY_IS_TUNE);
                    boolean isPowerUp = (mService.getPowerStatus() == FmService.POWER_UP);

                    // tune finished, should make power enable
                    mIsDisablePowerMenu = false;
                    float frequency = bundle.getFloat(FmListener.KEY_TUNE_TO_STATION);
                    mCurrentStation = FmUtils.computeStation(frequency);
                    // After tune to station finished, refresh favorite button and
                    // other button status.
                    refreshStationUI(mCurrentStation);
                    // tune fail,should resume button status
                    if (!isTune) {
                        Log.d(TAG, "mHandler.tune: " + isTune);
                        refreshActionMenuItem(isPowerUp);
                        refreshImageButton(isPowerUp);
                        refreshPopupMenuItem(isPowerUp);
                        refreshPlayButton(true);
                        return;
                    }
                    refreshImageButton(true);
                    refreshActionMenuItem(true);
                    refreshPopupMenuItem(true);
                    refreshPlayButton(true);
                    break;

                case FmListener.MSGID_FM_EXIT:
                    finish();
                    break;

                case FmListener.LISTEN_RDSSTATION_CHANGED:
                    bundle = msg.getData();
                    int rdsStation = bundle.getInt(FmListener.KEY_RDS_STATION);
                    refreshStationUI(rdsStation);
                    break;

                case FmListener.LISTEN_PS_CHANGED:
                    String stationName = FmStation.getStationName(mContext, mCurrentStation);
                    mTextStationName.setText(stationName);
                    mScroller.notifyAdatperChange();
                    break;

                case FmListener.LISTEN_RT_CHANGED:
                    bundle = msg.getData();
                    String rtString = bundle.getString(FmListener.KEY_RT_INFO);
                    mTextRds.setText(rtString);
                    break;

                case FmListener.LISTEN_SPEAKER_MODE_CHANGED:
                    bundle = msg.getData();
                    boolean isSpeakerMode = bundle.getBoolean(FmListener.KEY_IS_SPEAKER_MODE);
                    break;

                case FmListener.LISTEN_RECORDSTATE_CHANGED:
                    if (mService != null) {
                        mService.updatePlayingNotification();
                    }
                    break;
                /**
                 * SPRD:Bug545627 Add RDS feature for brcm.
                 * 
                 * @{
                 */
                case FmListener.MSG_RDS_DATA_UPDATE:
                    Bundle data = msg.getData();
                    removeMessages(FmListener.MSG_RDS_DATA_UPDATE);
                    if (mService != null && (mService.getPowerStatus() == FmService.POWER_UP)) {
                        String rdsData = data.getString("rdsdata");
                        int rdsDataType = data.getInt("rdsdatatype");
                        Log.d(TAG, "MSG_RDS_DATA_UPDATE rdsdata = " + rdsData
                                + " rdsPty =" + rdsDataType);
                        ContentValues values = null;
                        switch (rdsDataType) {
                            case FmBrcmUtil.FM_RDS_DATA_PTY_UPDATE:
                                break;
                            case FmBrcmUtil.FM_RDS_DATA_PS_UPDATE:
                                /*
                                 * SPRD: fix bug 552726 PS data disappear when back from FM station
                                 * list. @{
                                 */
                                if (FmStation.isStationExist(mContext, mCurrentStation)) {
                                    values = new ContentValues(1);
                                    values.put(Station.PROGRAM_SERVICE, rdsData);
                                    FmStation.updateStationToDb(mContext, mCurrentStation, values);
                                } else {
                                    values = new ContentValues(2);
                                    values.put(Station.FREQUENCY, mCurrentStation);
                                    values.put(Station.PROGRAM_SERVICE, rdsData);
                                    FmStation.insertStationToDb(mContext, values);
                                }
                                /* @} */
                                // update PS data display
                                mTextStationName.setText(rdsData);
                                mScroller.notifyAdatperChange();
                                break;
                            case FmBrcmUtil.FM_RDS_DATA_PTYN_UPDATE:
                                break;
                            case FmBrcmUtil.FM_RDS_DATA_RT_UPDATE:
                                /*
                                 * SPRD: fix bug 552726 PS data disappear when back from FM station
                                 * list. @{
                                 */
                                if (FmStation.isStationExist(mContext, mCurrentStation)) {
                                    values = new ContentValues(1);
                                    values.put(Station.RADIO_TEXT, rdsData);
                                    FmStation.updateStationToDb(mContext, mCurrentStation, values);
                                } else {
                                    values = new ContentValues(2);
                                    values.put(Station.FREQUENCY, mCurrentStation);
                                    values.put(Station.RADIO_TEXT, rdsData);
                                    FmStation.insertStationToDb(mContext, values);
                                }
                                /* @} */
                                // update RT data display
                                mTextRds.setText(rdsData);
                                break;
                            default:
                                break;
                        }
                    } else {
                        Message message = mHandler.obtainMessage(FmListener.MSG_CLEAR_RDS_DATA);
                        mHandler.sendMessage(message);
                    }
                    break;
                case FmListener.MSG_CLEAR_RDS_DATA:
                    removeMessages(FmListener.MSG_CLEAR_RDS_DATA);
                    mTextRds.setText("");
                    mTextStationName.setText("");
                    break;
                /**
                 * @}
                 */
                /* SPRD: fix bug 573755 FM shouldn't show rds data after close rds. @{ */
                case FmListener.MSG_CLOSE_RDS:
                    removeMessages(FmListener.MSG_CLOSE_RDS);
                    Cursor cursor = null;
                    ContentValues values = null;
                    try {
                        cursor = mContext.getContentResolver().query(Station.CONTENT_URI,
                                FmStation.COLUMNS, null, null, FmStation.Station.FREQUENCY);
                        if (cursor != null && cursor.moveToFirst()) {
                            do {
                                int stationFreq = cursor.getInt(cursor
                                        .getColumnIndex(FmStation.Station.FREQUENCY));
                                String ps = cursor.getString(cursor
                                        .getColumnIndex(FmStation.Station.PROGRAM_SERVICE));
                                String rt = cursor.getString(cursor
                                        .getColumnIndex(FmStation.Station.RADIO_TEXT));
                                if (!TextUtils.isEmpty(ps) || !TextUtils.isEmpty(rt)) {
                                    Log.d(TAG, "close rds, ps = " + ps + ", rt = " + rt
                                            + ", stationFreq = " + stationFreq);
                                    values = new ContentValues(2);
                                    values.put(Station.PROGRAM_SERVICE, "");
                                    values.put(Station.RADIO_TEXT, "");
                                    FmStation.updateStationToDb(mContext, stationFreq, values);
                                }
                            } while (cursor.moveToNext());
                        } else {
                            Log.d(TAG, "close rds, cursor is null");
                        }
                    } finally {
                        if (cursor != null) {
                            cursor.close();
                        }
                    }
                    Log.d(TAG,
                            "close rds, mTextStationName = "
                                    + FmStation.getStationName(mContext, mCurrentStation));
                    mTextStationName.setText(FmStation.getStationName(mContext, mCurrentStation));
                    mTextRds.setText("");
                    if (mService != null) {
                        mService.clearPsAndRt();
                    }
                    break;
                /* @} */
                /* SPRD: Fix for bug 589228 forbid turn on FM in airplane mode. @{ */
                case FmListener.MSGID_AIRPLANE_STATE:
                    Bundle airdata = msg.getData();
                    boolean airplaneState = airdata.getBoolean("airplane");
                    if(airplaneState){
                        changeToAirPlaneLayout();
                    } else {
                        removeDialog(AIRPLANE_DIALOG);
                        if (isAntennaAvailable()) {
                            changeToMainLayout();
                        } else {
                            changeToNoHeadsetLayout();
                        }
                    }
                    break;
                /* Bug 589228 End@} */
                case FmListener.MSGID_SCAN_FINISHED:
                    Message message = mHandler.obtainMessage(FmListener.MSGID_SET_MUTE_FINISHED);
                    mHandler.sendMessage(message);
                    updateMenuStatus();
                    break;
                default:
                    break;
            }
        }
    };

    // When call bind service, it will call service connect. register call back
    // listener and initial device
    private final ServiceConnection mServiceConnection = new ServiceConnection() {

        /**
         * called by system when bind service
         * 
         * @param className component name
         * @param service service binder
         */
        @Override
        public void onServiceConnected(ComponentName className, IBinder service) {
            mService = ((FmService.ServiceBinder) service).getService();
            if (null == mService) {
                Log.e(TAG, "onServiceConnected, mService is null");
                finish();
                return;
            }
            Log.d(TAG, "onServiceConnected, mService is not null");
            refreshRDSVisiable();
            mService.registerFmRadioListener(mFmRadioListener);
            mService.setFmMainActivityForeground(mIsActivityForeground);
            // SPRD : bug568587, new feature FM new UI
            mService.setTimerListenr(new MyTimeCountListener());
            if (FmRecorder.STATE_RECORDING != mService.getRecorderState()) {
                // SPRD: fix bug 572273 Fail to show FM notification in lock screen
                if (mIsActivityForeground) {
                    mService.removeNotification();
                }
            }
            if (!mService.isServiceInited()) {
                mService.initService(mCurrentStation);
                powerUpFm();
            } else {
                if (mService.isDeviceOpen()) {
                    // tune to station during changing language,we need to tune
                    // again when service bind success
                    if (mIsTune) {
                        tuneStation(mCurrentStation);
                        mIsTune = false;
                    }
                    updateCurrentStation();
                    updateMenuStatus();
                } else {
                    // Normal case will not come here
                    // Need to exit FM for this case
                    exitService();
                    finish();
                }
            }
        }

        /**
         * When unbind service will call this method
         * 
         * @param className The component name
         */
        @Override
        public void onServiceDisconnected(ComponentName className) {
        }
    };

    private class NoHeadsetAlpaOutListener implements AnimationListener {

        @Override
        public void onAnimationEnd(Animation animation) {
            if (!isAntennaAvailable()) {
                return;
            }
            changeToMainLayout();
            cancelMainAnimation();
            Animation anim = AnimationUtils.loadAnimation(mContext,
                    R.anim.main_alpha_in);
            mMainLayout.startAnimation(anim);
            anim = AnimationUtils.loadAnimation(mContext, R.anim.floatbtn_alpha_in);

            mBtnPlayContainer.startAnimation(anim);
        }

        @Override
        public void onAnimationRepeat(Animation animation) {
        }

        @Override
        public void onAnimationStart(Animation animation) {
            mNoHeadsetImgViewWrap.setElevation(0);
        }
    }

    private class NoHeadsetAlpaInListener implements AnimationListener {

        @Override
        public void onAnimationEnd(Animation animation) {
            if (isAntennaAvailable()) {
                return;
            }
            changeToNoHeadsetLayout();
            cancelNoHeadsetAnimation();
            Animation anim = AnimationUtils.loadAnimation(mContext,
                    R.anim.noeaphone_alpha_in);
            mNoHeadsetLayout.startAnimation(anim);
        }

        @Override
        public void onAnimationRepeat(Animation animation) {
        }

        @Override
        public void onAnimationStart(Animation animation) {
            mNoHeadsetImgViewWrap.setElevation(mMiddleShadowSize);
        }

    }

    /**
     * Update the favorite UI state
     */
    private void updateFavoriteStation() {
        // Judge the current output and switch between the devices.
        if (FmStation.isFavoriteStation(mContext, mCurrentStation)) {
            FmStation.removeFromFavorite(mContext, mCurrentStation);
            mButtonAddToFavorite.setImageResource(R.drawable.btn_fm_favorite_off_selector);
            // Notify scroller
            mScroller.onRemoveFavorite();
            mTextStationName.setText(FmStation.getStationName(mContext, mCurrentStation));
        } else {
            // Add the station to favorite
            if (FmStation.isStationExist(mContext, mCurrentStation)) {
                FmStation.addToFavorite(mContext, mCurrentStation);
            } else {
                ContentValues values = new ContentValues(2);
                values.put(Station.FREQUENCY, mCurrentStation);
                values.put(Station.IS_FAVORITE, true);
                FmStation.insertStationToDb(mContext, values);
            }
            mButtonAddToFavorite.setImageResource(R.drawable.btn_fm_favorite_on_selector);
            // Notify scroller
            mScroller.onAddFavorite();
        }
    }

    /**
     * SPRD: bug474728, new feature, preset station.
     */
    private void addPresetStations() {
        Resources res = getResources();
        String[] stationName = res.getStringArray(R.array.presetstationname);
        String[] presetFrequency = res.getStringArray(R.array.presetstationfreq);
        if (stationName.length != presetFrequency.length || presetFrequency.length == 0) {
            Log.w(TAG, "preset station and station name is not equal");
            return;
        }
        Float fFrq = (float) 0;
        int iFrq = 0;
        ContentValues values = new ContentValues(2);
        for (int i = 0; i < presetFrequency.length; i++) {
            fFrq = Float.parseFloat(presetFrequency[i]);
            iFrq = (int) (fFrq * 10);
            values.put(Station.FREQUENCY, iFrq);
            values.put(Station.STATION_NAME, stationName[i]);
            FmStation.insertStationToDb(mContext, values);
            values.clear();
        }
    }

    /**
     * Called when the activity is first created, initial variables
     * 
     * @param savedInstanceState The saved bundle in onSaveInstanceState
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setVolumeControlStream(AudioManager.STREAM_MUSIC);
        setContentView(R.layout.main);
        try {
            ViewConfiguration config = ViewConfiguration.get(this);
            Field menuKeyField = ViewConfiguration.class.getDeclaredField("sHasPermanentMenuKey");
            if (menuKeyField != null) {
                menuKeyField.setAccessible(true);
                menuKeyField.setBoolean(config, false);
            }
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }

        mFragmentManager = getFragmentManager();
        mContext = getApplicationContext();

        initUiComponent();
        /**
         * SPRD: bug474717, input frequency.
         * 
         * @{
         */
        mDialogs = new Dialog[DIALOG_COUNT];
        mTextStationValue.setOnClickListener(mTextViewClickListener);
        /**
         * @}
         */
        registerButtonClickListener();
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);

        mScroller = (FmScroller) findViewById(R.id.multiscroller);
        mScroller.initialize();
        mEventListener = new EventListener() {
            @Override
            public void onRename(int frequency) {
                showRenameDialog(frequency);
            }

            @Override
            public void onRemoveFavorite(int frequency) {
                // TODO it's on UI thread, change to sub thread
                if (FmStation.isFavoriteStation(mContext, frequency)) {
                    FmStation.removeFromFavorite(mContext, frequency);
                    if (mCurrentStation == frequency) {
                        mTextStationName.setText(FmStation.getStationName(mContext, frequency));
                        mButtonAddToFavorite.setImageResource(R.drawable.btn_fm_favorite_off_selector);
                    }
                    // Notify scroller
                    mScroller.onRemoveFavorite();
                }
            }

            @Override
            public void onPlay(int frequency) {
                /**
                 * SPRD: bug513467 Fix NullPointerException in monkey test.
                 * 
                 * @{
                 */
                if (null == mService) {
                    Log.e(TAG, "onPlay, mService is null");
                    return;
                }
                /**
                 * @}
                 */
                if (frequency != 0 && (mService.getPowerStatus() == FmService.POWER_UP)) {
                    tuneStation(frequency);
                }
            }

            /**
             * SPRD: bug496334, change station frequency.
             * 
             * @{
             */
            @Override
            public void onSetFreq(int frequency) {
                showChangeFreqDialog(frequency);
            }
            /**
             * @}
             */
        };
        mScroller.registerListener(mEventListener);
        /**
         * SPRD: bug474728, new feature, preset station.
         * 
         * @{
         */
        SharedPreferences mysp = this.getSharedPreferences(USER_PRESET_STATIONS,
                Context.MODE_PRIVATE);
        boolean hasSet = mysp.getBoolean(SET_FLAG, false);
        if (!hasSet) {
            Log.i(TAG, "preset user stations");
            SharedPreferences.Editor edit = mysp.edit();
            edit.putBoolean(SET_FLAG, true);
            edit.commit();
            addPresetStations();
        }
        /**
         * @}
         */

        /**
         * SPRD: bug474741, recording format selection.
         * 
         * @{
         */
        SharedPreferences formatSP = this.getSharedPreferences(FM_RECORD_FORMAT,
                Context.MODE_PRIVATE);
        FmRecorder.GLOBAL_RECORD_FORMAT_FLAG = formatSP.getInt(DEFAULT_FORMAT, 0);
        /**
         * @}
         */

        /**
         * SPRD: bug474747, recording path selection.& bug524632 check external storage status.
         * 
         * @{
         */
        SharedPreferences storageSP = this.getSharedPreferences(FM_RECORD_STORAGE,
                Context.MODE_PRIVATE);
        FmUtils.FM_RECORD_STORAGE_PATH = storageSP.getInt(FM_RECORD_DEFAULT_PATH, 0);
        mRecordInternalStorage = String.valueOf(getResources().getString(R.string.storage_phone));
        mRecordExternalStorage = String.valueOf(getResources().getString(R.string.storage_sd));
        mRecordeFilePathList = new ArrayList();
        /**
         * @}
         */

        /** SPRD: bug568587, Regularly power off.@{ */
        SharedPreferences customSP = this.getSharedPreferences(FM_REGULAYLY_POWER_OFF,
                Context.MODE_PRIVATE);
        FmUtils.GLOBAL_CUSTOM_TIME = customSP.getFloat(DEFAULT_CUSTOM_SETTING, (float) 15.0);
        /** @} */

        mUserManager = (UserManager) this.getSystemService(Context.USER_SERVICE);
        /**
         * Bug545627 Add RDS feature for brcm.
         * 
         * @{
         */
        mFmBrcmUtil.registerfmStateChangeListener(this);
        /**
         * @}
         */
    }

    @Override
    public void editFavorite(int stationFreq, String name) {
        int updateResult =
                FmStation.updateStationToDb(mContext, stationFreq, name);
        if (updateResult == 0) {
            return;
        }
        if (mCurrentStation == stationFreq) {
            String stationName = FmStation.getStationName(mContext, mCurrentStation);
            mTextStationName.setText(stationName);
        }
        mScroller.notifyAdatperChange();
        String title = getString(R.string.toast_station_renamed);
        FmSnackBar.make(FmMainActivity.this, title, null, null,
                FmSnackBar.DEFAULT_DURATION).show();
    }

    /**
     * Display the rename dialog
     * 
     * @param frequency The display frequency
     */
    public void showRenameDialog(int frequency) {
        if (mService != null) {
            String name = FmStation.getStationName(mContext, frequency);
            FmFavoriteEditDialog newFragment = FmFavoriteEditDialog.newInstance(name, frequency);
            newFragment.show(mFragmentManager, "TAG_EDIT_FAVORITE");
            mFragmentManager.executePendingTransactions();
        }
    }

    /**
     * Go to station list activity
     */
    private void enterStationList() {
        if (mService != null) {
            // AMS change the design for background start
            // activity. need check app is background in app code
            if (mService.isActivityForeground()) {
                Intent intent = new Intent();
                intent.setClass(FmMainActivity.this, FmFavoriteActivity.class);
                // SPRD : bug568587, new feature FM new UI
                intent.putExtra(FmFavoriteActivity.ACTIVITY_RESULT, mCurrentStation);
                startActivityForResult(intent, REQUEST_CODE_FAVORITE);
            }
        }
    }

    /**
     * Refresh the favorite button with the given station, if the station is favorite station, show
     * favorite icon, else show non-favorite icon.
     * 
     * @param station The station frequency
     */
    private void refreshStationUI(int station) {
        if (FmUtils.isFirstTimePlayFm(mContext)) {
            Log.d(TAG, "refreshStationUI, set station value null when it is first time ");
            return;
        }
        // TODO it's on UI thread, change to sub thread
        // Change the station frequency displayed.
        mTextStationValue.setText(FmUtils.formatStation(station));
        // Show or hide the favorite icon
        if (FmStation.isFavoriteStation(mContext, station)) {
            mButtonAddToFavorite.setImageResource(R.drawable.btn_fm_favorite_on_selector);
        } else {
            mButtonAddToFavorite.setImageResource(R.drawable.btn_fm_favorite_off_selector);
        }

        String stationName = "";
        String radioText = "";
        ContentResolver resolver = mContext.getContentResolver();
        Cursor cursor = null;
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
                // If the station name is not exist, show program service(PS) instead
                stationName = cursor.getString(cursor.getColumnIndex(Station.STATION_NAME));
                if (TextUtils.isEmpty(stationName)) {
                    stationName = cursor.getString(cursor.getColumnIndex(Station.PROGRAM_SERVICE));
                }
                radioText = cursor.getString(cursor.getColumnIndex(Station.RADIO_TEXT));

            } else {
                Log.d(TAG, "showPlayingNotification, cursor is null");
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        mTextStationName.setText(stationName);
        mTextRds.setText(radioText);
    }

    /**
     * Start and bind service, reduction variable values if configuration changed
     */
    @Override
    public void onStart() {
        super.onStart();
        // check layout onstart
        if(FmUtils.isAirplane(this)){
            changeToAirPlaneLayout();
        }else if (isAntennaAvailable()) {
            changeToMainLayout();
        } else {
            changeToNoHeadsetLayout();
        }

        // Should start FM service first.
        if (null == startService(new Intent(FmMainActivity.this, FmService.class))) {
            Log.e(TAG, "onStart, cannot start FM service");
            return;
        }

        mIsServiceStarted = true;
        mIsServiceBinded = bindService(new Intent(FmMainActivity.this, FmService.class),
                mServiceConnection, Context.BIND_AUTO_CREATE);

        if (!mIsServiceBinded) {
            Log.e(TAG, "onStart, cannot bind FM service");
            finish();
            return;
        }
    }

    /**
     * Refresh UI, when stop search, dismiss search dialog, pop up recording dialog if FM stopped
     * when recording in background
     */
    @Override
    public void onResume() {
        super.onResume();
        mIsActivityForeground = true;
        mScroller.onResume();
        if (null == mService) {
            Log.d(TAG, "onResume, mService is null");
            return;
        }

        mService.setFmMainActivityForeground(mIsActivityForeground);
        if (FmRecorder.STATE_RECORDING != mService.getRecorderState()) {
            mService.removeNotification();
        }
        updateMenuStatus();
        // SPRD : bug568587, new feature FM new UI
        timerTextUpdate();
    }

    /**
     * When activity is paused call this method, indicate activity enter background if press exit,
     * power down FM
     */
    @Override
    public void onPause() {
        mIsActivityForeground = false;
        if (null != mService) {
            mService.setFmMainActivityForeground(mIsActivityForeground);
        }
        /**
         * SPRD: bug474741, recording format selection.
         * 
         * @{
         */
        SharedPreferences formatSP = this.getSharedPreferences(FM_RECORD_FORMAT,
                Context.MODE_PRIVATE);
        SharedPreferences.Editor edit = formatSP.edit();
        edit.putInt(DEFAULT_FORMAT, FmRecorder.GLOBAL_RECORD_FORMAT_FLAG);
        edit.commit();
        /**
         * @}
         */
        mScroller.onPause();
        super.onPause();
    }

    /**
     * Called when activity enter stopped state, unbind service, if exit pressed, stop service
     */
    @Override
    public void onStop() {
        if (null != mService) {
            mService.setNotificationClsName(FmMainActivity.class.getName());
            mService.updatePlayingNotification();
        }
        if (mIsServiceBinded) {
            unbindService(mServiceConnection);
            mIsServiceBinded = false;
        }
        super.onStop();
    }

    /**
     * W activity destroy, unregister broadcast receiver and remove handler message
     */
    @Override
    public void onDestroy() {
        // need to call this function because if doesn't do this,after
        // configuration change will have many instance and recording time
        // or playing time will not refresh
        // Remove all the handle message
        mHandler.removeCallbacksAndMessages(null);
        if (mService != null) {
            mService.unregisterFmRadioListener(mFmRadioListener);
        }
        mFmRadioListener = null;
        mScroller.closeAdapterCursor();
        mScroller.unregisterListener(mEventListener);
        super.onDestroy();
        /**
         * Bug545627 Add RDS feature for brcm.
         * 
         * @{
         */
        mFmBrcmUtil.unregisterfmStateChangeListener(this);
        /**
         * @}
         */
    }

    /**
     * Create options menu
     * 
     * @param menu The option menu
     * @return true or false indicate need to handle other menu item
     */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.fm_action_bar, menu);
        mMenu = menu;
        mMenuItemStationlList = menu.findItem(R.id.fm_station_list);
        mMenuItemHeadset = menu.findItem(R.id.fm_headset);
        /* SPRD : bug568587, new feature FM new UI */
        // mMenuItemStartRecord = menu.findItem(R.id.fm_start_record);
        mMenuItemRecord = menu.findItem(R.id.item_fm_record);
        mMenuItemRecordList = menu.findItem(R.id.fm_record_list);
        mMenuItemRegularlyPoweroff = menu.findItem(R.id.fm_regularly_poweroff);
        timerTextUpdate();
        /* @} */
        mMenuItemRDSSwitch = menu.findItem(R.id.fm_rds_switch);
        mMenuItemAFSwitch = menu.findItem(R.id.fm_rds_af_switch);
        refreshRDSVisiable();
        return true;
    }

    /**
     * SPRD: bug474717 input frequency & bug474741 recording format selection & bug474747 recording
     * path selection & bug490888 Seek station with 50kHz step. & bug524632 check external storage
     * status.
     * 
     * @{
     */
    protected Dialog onCreateDialog(int id) {
        switch (id) {
            case INPUT_DIALOG:
                LayoutInflater inflater = LayoutInflater.from(this);
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                View view = inflater.inflate(R.layout.input_freq_alert_dialog, null);
                TextView title = (TextView) view.findViewById(R.id.title);
                title.setText(R.string.change_frequency);
                final EditText freq = (EditText) view.findViewById(R.id.edit_freq);
                Log.d(TAG, "INPUT_DIALOG:mCurrentStation=" + FmUtils.formatStation(mCurrentStation));
                freq.setText(FmUtils.formatStation(mCurrentStation));

                builder.setPositiveButton(R.string.button_ok,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                String strFreq = freq.getText().toString();
                                if (strFreq.length() == 0) {
                                    showToast(getString(R.string.freq_input_info));
                                } else {
                                    if (strFreq.charAt(0) == '.') {
                                        strFreq = '0' + strFreq;
                                    }
                                    Float fFreq = Float.parseFloat(strFreq);
                                    int iFreq = (int) (fFreq * FmUtils.CONVERT_RATE);
                                    if (FmUtils.isValidStation(iFreq)) {
                                        tuneStation(iFreq);
                                    } else {
                                        showToast(getString(R.string.range_error));
                                    }
                                }
                            }
                        }
                        );

                builder.setNegativeButton(R.string.button_cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.cancel();
                            }
                        }
                        );
                mDialogs[INPUT_DIALOG] = builder.setView(view).create();
                mDialogs[INPUT_DIALOG].setCanceledOnTouchOutside(false);
                return mDialogs[INPUT_DIALOG];
            case RECORD_FORMAT_DIALOG:
                mDialogs[RECORD_FORMAT_DIALOG] = new AlertDialog.Builder(this)
                        .setTitle(R.string.select_file_type)
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
                mDialogs[RECORD_FORMAT_DIALOG].setCanceledOnTouchOutside(false);
                return mDialogs[RECORD_FORMAT_DIALOG];
            case RECORD_FILE_PATH_DIALOG:
                if (!Environment.MEDIA_MOUNTED.equals(EnvironmentEx.getExternalStoragePathState())) {
                    mRecordeFilePathList.add(mRecordInternalStorage);
                    FmUtils.FM_RECORD_STORAGE_PATH = 0;
                    saveRecordDefaultPath();
                } else {
                    mRecordeFilePathList.add(mRecordInternalStorage);
                    if (mUserManager.isSystemUser()) {
                        mRecordeFilePathList.add(mRecordExternalStorage);
                        Log.i(TAG, "is  SystemUser");
                    } else {
                        FmUtils.FM_RECORD_STORAGE_PATH = 0;
                        saveRecordDefaultPath();
                        Log.i(TAG, "is not SystemUser");
                    }
                }
                mDialogs[RECORD_FILE_PATH_DIALOG] = new AlertDialog.Builder(this)
                        .setTitle(R.string.select_file_path)
                        .setSingleChoiceItems(
                                (String[]) (mRecordeFilePathList.toArray(new String[mRecordeFilePathList
                                        .size()]))
                                , FmUtils.FM_RECORD_STORAGE_PATH,
                                new DialogInterface.OnClickListener() {
                                    public void onClick(DialogInterface dialog, int which) {
                                        switch (which) {
                                            case 0:
                                                FmUtils.FM_RECORD_STORAGE_PATH = which;
                                                break;
                                            case 1:
                                                FmUtils.FM_RECORD_STORAGE_PATH = which;
                                                break;
                                            default:
                                        }
                                        saveRecordDefaultPath();
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
                mDialogs[RECORD_FILE_PATH_DIALOG].setCanceledOnTouchOutside(false);
                mRecordeFilePathList.clear();
                return mDialogs[RECORD_FILE_PATH_DIALOG];
                /* SPRD: bug568587, Regularly power off.@{ */
            case REGULAYLY_POWER_OFF:
                LayoutInflater inflater2 = LayoutInflater.from(this);
                AlertDialog.Builder builder2 = new AlertDialog.Builder(this);
                View view2 = inflater2.inflate(R.layout.regularly_poweroff_dialog, null);
                final RadioGroup radioGroup = (RadioGroup) view2.findViewById(R.id.radioGroup);
                radioGroup.setOnCheckedChangeListener(new RadioGroupListener());
                mEditContainer = (LinearLayout) view2.findViewById(R.id.edit_container);
                mCustomEdit = (EditText) view2.findViewById(R.id.edit_custom_setting);
                mCustomEdit.setEnabled(false);
                RadioButton time_10min = (RadioButton) view2.findViewById(R.id.time_10min);
                RadioButton time_20min = (RadioButton) view2.findViewById(R.id.time_20min);
                RadioButton time_30min = (RadioButton) view2.findViewById(R.id.time_30min);
                RadioButton time_60min = (RadioButton) view2.findViewById(R.id.time_60min);
                RadioButton time_custom = (RadioButton) view2.findViewById(R.id.time_custom);
                if (FmUtils.GLOBAL_CUSTOM_TIME == 15.0) {
                    time_10min.setChecked(true);
                } else if (FmUtils.GLOBAL_CUSTOM_TIME == 30.0) {
                    time_20min.setChecked(true);
                } else if (FmUtils.GLOBAL_CUSTOM_TIME == 60.0) {
                    time_30min.setChecked(true);
                } else if (FmUtils.GLOBAL_CUSTOM_TIME == 90.0) {
                    time_60min.setChecked(true);
                } else {
                    time_custom.setChecked(true);
                    mCustomEdit.setEnabled(true);
                    mCustomEdit.setText(String.valueOf((int) FmUtils.GLOBAL_CUSTOM_TIME));
                    mCustomEdit.requestFocus();
                }
                builder2.setPositiveButton(R.string.button_ok,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                if (mCustomEdit.isEnabled()) {
                                    int powerOffTime = 0;
                                    String strCustomTime = mCustomEdit.getText().toString();
                                    if (!"".equals(strCustomTime)) {
                                        powerOffTime = Integer.parseInt(strCustomTime);
                                    }
                                    if (powerOffTime <= 0) {
                                        showToast(getString(R.string.input_error));
                                        return;
                                    } else {
                                        if (strCustomTime.charAt(0) == '.') {
                                            strCustomTime = '0' + strCustomTime;
                                        }
                                        FmUtils.GLOBAL_CUSTOM_TIME = Float
                                                .parseFloat(strCustomTime);
                                    }
                                }
                                saveDefaulCustomTime();
                                long millisInFuture = (long) (FmUtils.GLOBAL_CUSTOM_TIME * 60 * 1000);
                                mService.startTimer(millisInFuture, 1000);
                            }
                        }
                        );
                builder2.setNegativeButton(R.string.button_cancel,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                dialog.cancel();
                            }
                        }
                        );
                mDialogs[REGULAYLY_POWER_OFF] = builder2.setView(view2).create();
                mDialogs[REGULAYLY_POWER_OFF].setCanceledOnTouchOutside(false);
                return mDialogs[REGULAYLY_POWER_OFF];
                /* @} */
            /* SPRD: Fix for bug 589228 forbid turn on FM in airplane mode. @{ */
            case AIRPLANE_DIALOG:
                mDialogs[AIRPLANE_DIALOG] = new AlertDialog.Builder(this)
                .setTitle(R.string.airplane_title).setMessage(R.string.airplane_message)
                .setPositiveButton(R.string.button_ok,
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                finish();
                                dialog.cancel();
                            }
                        }
                ).create();
                mDialogs[AIRPLANE_DIALOG].setCanceledOnTouchOutside(false);
                return mDialogs[AIRPLANE_DIALOG];
            /* Bug 589228 End@} */
            default:
                return null;
        }
    }

    private void saveRecordDefaultPath() {
        SharedPreferences storageSP = mContext.getSharedPreferences(FM_RECORD_STORAGE,
                Context.MODE_PRIVATE);
        SharedPreferences.Editor storageEdit = storageSP.edit();
        storageEdit.putInt(FM_RECORD_DEFAULT_PATH, FmUtils.FM_RECORD_STORAGE_PATH);
        storageEdit.commit();
    }

    /**
     * SPRD: bug474717, input frequency.
     * 
     * @{
     */
    protected void onPrepareDialog(int id, Dialog dialog) {
        switch (id) {
            case INPUT_DIALOG:
                EditText inputFreq = (EditText) dialog.findViewById(R.id.edit_freq);
                inputFreq.setText(FmUtils.formatStation(mCurrentStation));
                Editable lastEdit = inputFreq.getText();
                Selection.setSelection(lastEdit, lastEdit.length());
                break;
            default:
                break;
        }
    }

    /**
     * Prepare options menu
     * 
     * @param menu The option menu
     * @return true or false indicate need to handle other menu item
     */
    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        if (null == mService) {
            Log.d(TAG, "onPrepareOptionsMenu, mService is null");
            return true;
        }
        int powerStatus = mService.getPowerStatus();
        boolean isPowerUp = (powerStatus == FmService.POWER_UP);
        boolean isPowerdown = (powerStatus == FmService.POWER_DOWN);
        boolean isSeeking = mService.isSeeking();
        boolean isSpeakerUsed = mService.isSpeakerUsed();
        // if fm power down by other app, should enable power menu, make it to
        // powerup.
        refreshActionMenuItem(isSeeking ? false : (isPowerUp && !mService.isMuted()));
        refreshPlayButton(isSeeking ? false
                : (isPowerUp || (isPowerdown && !mIsDisablePowerMenu)));
        mMenuItemHeadset.setIcon(isSpeakerUsed ? R.drawable.btn_fm_speaker_selector
                : R.drawable.btn_fm_headset_selector);
        boolean isRDSOpenstate = FmUtils.isRDSOpen(mContext);
        boolean isAFOpenstate = FmUtils.isAFOpen(mContext);
        mMenuItemRDSSwitch.setTitle(isRDSOpenstate ? R.string.rds_switch_off
                : R.string.rds_switch_on);
        mMenuItemAFSwitch.setTitle(isAFOpenstate ? R.string.rds_af_switch_off
                : R.string.rds_af_switch_on);
        mMenuItemAFSwitch.setEnabled(isRDSOpenstate);
        if (isAntennaAvailable()) {
            setMenuVisibility(true);
        } else {
            setMenuVisibility(false);
        }
        if (!mService.isRdsSupported()) {
            mMenuItemRDSSwitch.setVisible(false);
        }
        return true;
    }

    /**
     * Handle event when option item selected
     * 
     * @param item The clicked item
     * @return true or false indicate need to handle other menu item or not
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                onBackPressed();
                break;

            case R.id.fm_station_list:
                refreshImageButton(false);
                refreshActionMenuItem(false);
                refreshPopupMenuItem(false);
                refreshPlayButton(false);
                // Show favorite activity.
                enterStationList();
                break;

            case R.id.earphone_menu:
                //Modify for 645950,Switch audio device to quondam device,a sound deadlock happen.
                if (mService.isSpeakerUsed()) {
                    setSpeakerPhoneOn(false);
                    mMenuItemHeadset.setIcon(R.drawable.btn_fm_headset_selector);
                    invalidateOptionsMenu();
                }
                break;

            case R.id.speaker_menu:
                //Modify for 645950,Switch audio device to quondam device,a sound deadlock happen.
                if (!mService.isSpeakerUsed()) {
                    setSpeakerPhoneOn(true);
                    mMenuItemHeadset.setIcon(R.drawable.btn_fm_speaker_selector);
                    invalidateOptionsMenu();
                }
                break;

            /*
             * SPRD : bug568587, new feature FM new UI @{ case R.id.fm_start_record: //SPRD:
             * bug547849 fail to record FM recording and stuck at recording page and have no
             * response... if (mAudioManager.isAudioRecording()) { new AlertDialog.Builder(this)
             * .setTitle(R.string.app_name) .setMessage(R.string.same_application_running)
             * .setPositiveButton(R.string.button_ok, null) .setCancelable(false) .show(); } else{
             * Intent recordIntent = new Intent(this, FmRecordActivity.class);
             * recordIntent.putExtra(FmStation.CURRENT_STATION, mCurrentStation);
             * startActivityForResult(recordIntent, REQUEST_CODE_RECORDING); } break;
             */

            case R.id.item_fm_record:
                if (mService.isAudioRecording()) {
                    new AlertDialog.Builder(this)
                            .setTitle(R.string.app_name)
                            .setMessage(R.string.same_application_running)
                            .setPositiveButton(R.string.button_ok, null)
                            .setCancelable(false)
                            .show();
                } else {
                    Intent recordIntent = new Intent(this, FmRecordActivity.class);
                    recordIntent.putExtra(FmStation.CURRENT_STATION, mCurrentStation);
                    startActivityForResult(recordIntent, REQUEST_CODE_RECORDING);
                }
                break;
            /* @} */

            case R.id.fm_record_list:
                /* new feature,FM new UI @{ */
                refreshImageButton(false);
                refreshActionMenuItem(false);
                refreshPopupMenuItem(false);
                refreshPlayButton(false);
                // Show RecordList activity.
                enterRecordList();
                /* @} */
                break;
            /* SPRD: bug568587, Regularly power off.@{ */
            case R.id.fm_regularly_poweroff:
                if (mService.mIsStartTimeCount) {
                    mService.stopTimer();
                } else {
                    showDialog(REGULAYLY_POWER_OFF);
                }
                break;
            /* @} */
            case R.id.fm_rds_switch:
                if (null != mMenuItemRDSSwitch) {
                    boolean preRDSOpenstate = FmUtils.isRDSOpen(mContext);
                    mMenuItemRDSSwitch.setTitle(!preRDSOpenstate ? R.string.rds_switch_off
                            : R.string.rds_switch_on);
                    FmUtils.setRDSState(mContext, !preRDSOpenstate);
                    if (!preRDSOpenstate) {
                        boolean isAFOpenstate = FmUtils.isAFOpen(mContext);
                        mMenuItemAFSwitch.setTitle(isAFOpenstate ? R.string.rds_af_switch_off
                                : R.string.rds_af_switch_on);
                    }
                    mMenuItemAFSwitch.setEnabled(!preRDSOpenstate);
                    if (mService != null) {
                        mService.setRdsAsync(!preRDSOpenstate);
                    }
                    /* SPRD: fix bug 573755 FM shouldn't show rds data after close rds. @{ */
                    if (!FmUtils.isRDSOpen(mContext)) {
                        Message msg = mHandler.obtainMessage(FmListener.MSG_CLOSE_RDS);
                        mHandler.sendMessage(msg);
                    }
                    /* @} */
                }
                break;
            case R.id.fm_rds_af_switch:
                if (null != mMenuItemAFSwitch) {
                    boolean isAFOpenstate = FmUtils.isAFOpen(mContext);
                    mMenuItemAFSwitch.setTitle(isAFOpenstate ? R.string.rds_af_switch_off
                            : R.string.rds_af_switch_on);
                    FmUtils.setAFState(mContext, !isAFOpenstate);
                    if (mService != null) {
                        mService.setAFAsync(!isAFOpenstate);
                    }
                }
                break;
            default:
                Log.e(TAG, "onOptionsItemSelected, invalid options menu item.");
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    /**
     * Check whether antenna is available
     * 
     * @return true or false indicate antenna available or not
     */
    private boolean isAntennaAvailable() {
        return mAudioManager.isWiredHeadsetOn();
    }

    /**
     * When on activity result, tune to station which is from station list
     * 
     * @param requestCode The request code
     * @param resultCode The result code
     * @param data The intent from station list
     */
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (RESULT_OK == resultCode) {
            if (REQUEST_CODE_RECORDING == requestCode) {
                final Uri playUri = data.getData();
                boolean isSaved = playUri != null;
                String title = data.getStringExtra(EXTRA_RESULT_STRING);
                String action = null;
                FmSnackBar.OnActionTriggerListener listener = null;
                final String extensionString = (1 == FmRecorder.GLOBAL_RECORD_FORMAT_FLAG ? "audio/3gpp"
                        : "audio/amr");
                if (isSaved) {
                    action = FmMainActivity.this.getString(R.string.toast_listen);
                    listener = new FmSnackBar.OnActionTriggerListener() {
                        @Override
                        public void onActionTriggered() {
                            Intent playMusicIntent = new Intent(Intent.ACTION_VIEW);
                            try {
                                playMusicIntent.setClassName("com.google.android.music",
                                        "com.google.android.music.AudioPreview");
                                playMusicIntent.setDataAndType(playUri, extensionString);
                                startActivity(playMusicIntent);
                            } catch (ActivityNotFoundException e1) {
                                try {
                                    /*
                                     * SPRD : bug568587, new feature FM new UI @{ playMusicIntent =
                                     * new Intent(Intent.ACTION_VIEW);
                                     * playMusicIntent.setDataAndType(playUri, extensionString);
                                     * startActivity(playMusicIntent);
                                     */
                                    Intent playRecordIntent = new Intent(FmMainActivity.this,
                                            FmRecordListActivity.class);
                                    playRecordIntent.setData(playUri);
                                    startActivity(playRecordIntent);
                                    /* @} */
                                } catch (ActivityNotFoundException e2) {
                                    // No activity respond
                                    Log.d(TAG, "onActivityResult, no activity "
                                            + "respond play record file intent");
                                }
                            }
                        }
                    };
                }
                FmSnackBar.make(FmMainActivity.this, title, action, listener,
                        FmSnackBar.DEFAULT_DURATION).show();
            } else if (REQUEST_CODE_FAVORITE == requestCode) {
                int iStation =
                        data.getIntExtra(FmFavoriteActivity.ACTIVITY_RESULT, mCurrentStation);
                // Tune to this station.
                mCurrentStation = iStation;
                // if tune from station list, we should disable power menu,
                // especially for power down state
                mIsDisablePowerMenu = true;
                Log.d(TAG, "onActivityForReult:" + mIsDisablePowerMenu);
                if (null == mService) {
                    Log.d(TAG, "onActivityResult, mService is null");
                    mIsTune = true;
                    return;
                }
                tuneStation(iStation);
            } else {
                Log.e(TAG, "onActivityResult, invalid requestcode.");
                return;
            }
        }

        // TODO it's on UI thread, change to sub thread
        if (FmStation.isFavoriteStation(mContext, mCurrentStation)) {
            mButtonAddToFavorite.setImageResource(R.drawable.btn_fm_favorite_on_selector);
        } else {
            mButtonAddToFavorite.setImageResource(R.drawable.btn_fm_favorite_off_selector);
        }
        mTextStationName.setText(FmStation.getStationName(mContext, mCurrentStation));
    }

    /**
     * Power up FM
     */
    private void powerUpFm() {
        if(FmUtils.isAirplane(this)){
            changeToAirPlaneLayout();
        }else{
            refreshImageButton(false);
            refreshActionMenuItem(false);
            refreshPopupMenuItem(false);
            refreshPlayButton(false);
            mService.powerUpAsync(FmUtils.computeFrequency(mCurrentStation));
        }
    }

    /**
     * Power down FM
     */
    private void powerDownFm() {
        refreshImageButton(false);
        refreshActionMenuItem(false);
        refreshPopupMenuItem(false);
        refreshPlayButton(false);
        mService.powerDownAsync();
    }

    private void setSpeakerPhoneOn(boolean isSpeaker) {
        if (isSpeaker) {
            mService.setSpeakerPhoneOn(true);
        } else {
            mService.setSpeakerPhoneOn(false);
        }
    }

    /**
     * Tune a station
     * 
     * @param station The tune station
     */
    private void tuneStation(final int station) {
        /**
         * SPRD: bug500046, for monkey test.
         * 
         * @{
         */
        if (null == mService) {
            Log.e(TAG, "tuneStation, mService is null");
            return;
        }
        /**
         * @}
         */
        refreshImageButton(false);
        refreshActionMenuItem(false);
        refreshPopupMenuItem(false);
        refreshPlayButton(false);
        mService.tuneStationAsync(FmUtils.computeFrequency(station));
    }

    /**
     * Seek station according current frequency and direction
     * 
     * @param station The seek start station
     * @param direction The seek direction
     */
    private void seekStation(final int station, boolean direction) {
        // If the seek AsyncTask has been executed and not canceled, cancel it
        // before start new.
        /**
         * SPRD: bug500046, for monkey test.
         * 
         * @{
         */
        if (null == mService) {
            Log.e(TAG, "seekStation, mService is null");
            return;
        }
        /**
         * @}
         */
        refreshImageButton(false);
        refreshActionMenuItem(false);
        refreshPopupMenuItem(false);
        refreshPlayButton(false);
        mService.seekStationAsync(FmUtils.computeFrequency(station), direction);
    }

    private void refreshRDSVisiable() {
        if (null == mMenuItemRDSSwitch)
            return;
        boolean isVisiable = (null == mService ? false : mService.isRdsSupported());
        mMenuItemRDSSwitch.setVisible(isVisiable);
        mMenuItemAFSwitch.setVisible(isVisiable);
        mMenuItemAFSwitch.setVisible(false);
    }

    private void refreshImageButton(boolean enabled) {
        mButtonDecrease.setEnabled(enabled);
        mButtonPrevStation.setEnabled(enabled);
        mButtonNextStation.setEnabled(enabled);
        mButtonIncrease.setEnabled(enabled);
        mButtonAddToFavorite.setEnabled(enabled);
    }

    // Refresh action menu except power menu
    private void refreshActionMenuItem(boolean enabled) {
        // action menu
        if (null != mMenuItemStationlList) {
            mMenuItemStationlList.setEnabled(enabled);
            mMenuItemRecord.setEnabled(enabled);
            mMenuItemHeadset.setEnabled(enabled);
            // SPRD: bug568587, Regularly power off.
            mMenuItemRegularlyPoweroff.setEnabled(enabled);
            mMenuItemRDSSwitch.setEnabled(enabled);
        }
    }

    // Refresh play/stop float button
    private void refreshPlayButton(boolean enabled) {
        // action menu
        /**
         * SPRD: bug500046, for monkey test. Original Android code: boolean isPowerUp =
         * (mService.getPowerStatus() == FmService.POWER_UP);
         * 
         * @{
         */
        boolean isPowerUp = ((mService != null) ? (mService.getPowerStatus() == FmService.POWER_UP && !mService.isMuted())
                : false);
        /**
         * @}
         */
        mButtonPlay.setEnabled(enabled);
        mButtonPlay.setImageResource((isPowerUp
                ? R.drawable.btn_fm_stop_selector
                : R.drawable.btn_fm_start_selector));
        Resources r = getResources();
        /**
         * SPRD: Bug507770 Add setContentDescription for play_button.
         * 
         * @{
         */
        mButtonPlay.setContentDescription((isPowerUp ? r.getString(R.string.fm_stop) : r
                .getString(R.string.fm_play)));
        /**
         * @}
         */
        mBtnPlayInnerContainer.setBackground(r.getDrawable(R.drawable.fb_red));
        mScroller.refreshPlayIndicator(mCurrentStation, isPowerUp);
    }

    private void refreshPopupMenuItem(boolean enabled) {
        if (null != mMenuItemStationlList) {
            /* SPRD: bug568587, Regularly power off, FM new UI @{ */
            // mMenuItemStartRecord.setEnabled(enabled);
            mMenuItemRecord.setEnabled(enabled);
            mMenuItemRegularlyPoweroff.setEnabled(enabled);
            /* @} */
        }
    }

    /**
     * SPRD: bug513989 Fix IllegalStateException in monkey test.
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

    /**
     * Called when back pressed
     */
    @Override
    public void onBackPressed() {
        // exit fm, disable all button
        if ((null != mService) && (mService.getPowerStatus() == FmService.POWER_DOWN)) {
            refreshImageButton(false);
            refreshActionMenuItem(false);
            refreshPopupMenuItem(false);
            refreshPlayButton(false);
            exitService();
            return;
        }
        super.onBackPressed();
    }

    private void showToast(CharSequence text) {
        if (null == mToast) {
            mToast = Toast.makeText(mContext, text, Toast.LENGTH_SHORT);
        }
        mToast.setText(text);
        mToast.show();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
    }

    /**
     * Exit FM service
     */
    private void exitService() {
        if (mIsServiceBinded) {
            unbindService(mServiceConnection);
            mIsServiceBinded = false;
        }

        if (mIsServiceStarted) {
            stopService(new Intent(FmMainActivity.this, FmService.class));
            mIsServiceStarted = false;
        }
    }

    /**
     * Update current station according service station
     */
    private void updateCurrentStation() {
        // get the frequency from service, set frequency in activity, UI,
        // database
        // same as the frequency in service
        int freq = mService.getFrequency();
        if (FmUtils.isValidStation(freq)) {
            if (mCurrentStation != freq) {
                mCurrentStation = freq;
                FmStation.setCurrentStation(mContext, mCurrentStation);
                refreshStationUI(mCurrentStation);
            }
        }
    }

    /**
     * Update menu status, and animation
     */
    private void updateMenuStatus() {
        int powerStatus = mService.getPowerStatus();
        boolean isPowerUp = (powerStatus == FmService.POWER_UP);
        boolean isDuringPowerup = (powerStatus == FmService.DURING_POWER_UP);
        boolean isSeeking = mService.isSeeking();
        boolean isPowerdown = (powerStatus == FmService.POWER_DOWN);
        boolean isSpeakerUsed = mService.isSpeakerUsed();
        boolean fmStatus = (isSeeking || isDuringPowerup);
        // when seeking, all button should disabled,
        // else should update as origin status
        refreshImageButton(fmStatus ? false : (isPowerUp && !mService.isMuted()));
        refreshPopupMenuItem(fmStatus ? false : (isPowerUp && !mService.isMuted()));
        refreshActionMenuItem(fmStatus ? false : (isPowerUp && !mService.isMuted()));
        // if fm power down by other app, should enable power button
        // to powerup.
        Log.d(TAG, "updateMenuStatus.mIsDisablePowerMenu: " + mIsDisablePowerMenu);
        refreshPlayButton(fmStatus ? false
                : (isPowerUp || (isPowerdown && !mIsDisablePowerMenu)));
        if (null != mMenuItemHeadset) {
            mMenuItemHeadset.setIcon(isSpeakerUsed ? R.drawable.btn_fm_speaker_selector
                    : R.drawable.btn_fm_headset_selector);
        }
        if (null != mMenuItemRDSSwitch) {
            boolean isRDSOpenstate = FmUtils.isRDSOpen(mContext);
            boolean isAFOpenstate = FmUtils.isAFOpen(mContext);
            mMenuItemRDSSwitch.setTitle(isRDSOpenstate ? R.string.rds_switch_off
                    : R.string.rds_switch_on);
            mMenuItemAFSwitch.setTitle(isAFOpenstate ? R.string.rds_af_switch_off
                    : R.string.rds_af_switch_on);
        }
    }

    private void initUiComponent() {
        mTextRds = (TextView) findViewById(R.id.station_rds);
        mTextStationValue = (TextView) findViewById(R.id.station_value);
        mButtonAddToFavorite = (ImageButton) findViewById(R.id.button_add_to_favorite);
        mTextStationName = (TextView) findViewById(R.id.station_name);
        mButtonDecrease = (ImageButton) findViewById(R.id.button_decrease);
        mButtonIncrease = (ImageButton) findViewById(R.id.button_increase);
        mButtonPrevStation = (ImageButton) findViewById(R.id.button_prevstation);
        mButtonNextStation = (ImageButton) findViewById(R.id.button_nextstation);

        // put favorite button here since it might be used very early in
        // changing recording mode
        mCurrentStation = FmStation.getCurrentStation(mContext);
        refreshStationUI(mCurrentStation);

        // l new
        mMainLayout = (LinearLayout) findViewById(R.id.main_view);
        mNoHeadsetLayout = (RelativeLayout) findViewById(R.id.no_headset);
        mNoEarphoneTextLayout = (LinearLayout) findViewById(R.id.no_bottom);
        mBtnPlayContainer = (LinearLayout) findViewById(R.id.play_button_container);
        mBtnPlayInnerContainer = (LinearLayout) findViewById(R.id.play_button_inner_container);
        mButtonPlay = (ImageButton) findViewById(R.id.play_button);
        mNoEarPhoneTxt = (TextView) findViewById(R.id.no_eaphone_text);
        mNoHeadsetImgView = (ImageView) findViewById(R.id.no_headset_img);
        mNoHeadsetImgViewWrap = findViewById(R.id.no_middle);
        mMiddleShadowSize = getResources().getDimension(R.dimen.fm_middle_shadow);
        // main ui layout params
        final Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setActionBar(toolbar);
        getActionBar().setTitle("");
        // SPRD: bug568587, Regularly power off.
        mCustomTime = (TextView) findViewById(R.id.custom_time);
    }

    private void registerButtonClickListener() {
        mButtonAddToFavorite.setOnClickListener(mButtonClickListener);
        mButtonDecrease.setOnClickListener(mButtonClickListener);
        mButtonIncrease.setOnClickListener(mButtonClickListener);
        mButtonPrevStation.setOnClickListener(mButtonClickListener);
        mButtonNextStation.setOnClickListener(mButtonClickListener);
        mButtonPlay.setOnClickListener(mButtonClickListener);
    }

    /**
     * play main animation
     */
    private void playMainAnimation() {
        if (null == mService) {
            Log.e(TAG, "playMainAnimation, mService is null");
            return;
        }
        if (mMainLayout.isShown()) {
            Log.w(TAG, "playMainAnimation, main layout has already shown");
            return;
        }
        Animation animation = AnimationUtils.loadAnimation(mContext,
                R.anim.noeaphone_alpha_out);
        mNoEarPhoneTxt.startAnimation(animation);
        mNoHeadsetImgView.startAnimation(animation);

        animation = AnimationUtils.loadAnimation(mContext,
                R.anim.noeaphone_translate_out);
        animation.setAnimationListener(new NoHeadsetAlpaOutListener());
        mNoEarphoneTextLayout.startAnimation(animation);
    }

    /**
     * clear main layout animation
     */
    private void cancelMainAnimation() {
        mNoEarPhoneTxt.clearAnimation();
        mNoHeadsetImgView.clearAnimation();
        mNoEarphoneTextLayout.clearAnimation();
    }

    /**
     * play change to no headset layout animation
     */
    private void playNoHeadsetAnimation() {
        if (null == mService) {
            Log.e(TAG, "playNoHeadsetAnimation, mService is null");
            return;
        }
        if (mNoHeadsetLayout.isShown()) {
            Log.w(TAG, "playNoHeadsetAnimation, no headset layout has already shown");
            return;
        }
        Animation animation = AnimationUtils.loadAnimation(mContext, R.anim.main_alpha_out);
        mMainLayout.startAnimation(animation);
        animation.setAnimationListener(new NoHeadsetAlpaInListener());
        mBtnPlayContainer.startAnimation(animation);
    }

    /**
     * clear no headset layout animation
     */
    private void cancelNoHeadsetAnimation() {
        mMainLayout.clearAnimation();
        mBtnPlayContainer.clearAnimation();
    }

    /**
     * change to main layout
     */
    private void changeToMainLayout() {
        removeDialog(AIRPLANE_DIALOG);
        mNoEarphoneTextLayout.setVisibility(View.GONE);
        mNoHeadsetImgView.setVisibility(View.GONE);
        mNoHeadsetImgViewWrap.setVisibility(View.GONE);
        mNoHeadsetLayout.setVisibility(View.GONE);
        // change to main layout
        mMainLayout.setVisibility(View.VISIBLE);
        mBtnPlayContainer.setVisibility(View.VISIBLE);
    }

    /**
     * change to no headset layout
     */
    private void changeToNoHeadsetLayout() {
        removeDialog(AIRPLANE_DIALOG);
        mMainLayout.setVisibility(View.GONE);
        mBtnPlayContainer.setVisibility(View.GONE);
        mNoEarphoneTextLayout.setVisibility(View.VISIBLE);
        mNoHeadsetImgView.setVisibility(View.VISIBLE);
        mNoHeadsetImgViewWrap.setVisibility(View.VISIBLE);
        mNoHeadsetLayout.setVisibility(View.VISIBLE);
        mNoHeadsetImgViewWrap.setElevation(mMiddleShadowSize);
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
                    Intent playMusicIntent = new Intent(Intent.ACTION_VIEW);
                    int playlistId = FmRecorder.getPlaylistId(mContext);
                    Bundle extras = new Bundle();
                    extras.putInt("playlist", playlistId);
                    try {
                        playMusicIntent.putExtras(extras);
                        playMusicIntent.setClassName("com.google.android.music",
                                "com.google.android.music.ui.TrackContainerActivity");
                        playMusicIntent.setType("vnd.android.cursor.dir/playlist");
                        startActivity(playMusicIntent);
                    } catch (ActivityNotFoundException e1) {
                        try {
                            playMusicIntent = new Intent(Intent.ACTION_VIEW);
                            /**
                             * SPRD: SPRD: bug490967, remove FM in recent, re-enter the radio does
                             * not open. Original Android code: playMusicIntent.putExtras(extras);
                             * 
                             * @{
                             */
                            Bundle musicExtras = new Bundle();
                            musicExtras.putString("playlist", "" + playlistId);
                            playMusicIntent.putExtras(musicExtras);
                            /**
                             * @}
                             */
                            playMusicIntent.setType("vnd.android.cursor.dir/playlist");
                            startActivity(playMusicIntent);
                        } catch (ActivityNotFoundException e2) {
                            // No activity respond
                            Log.d(TAG,
                                    "onOptionsItemSelected, No activity respond playlist view intent");
                        }
                    }

                } else {
                    // Toast.makeText(this, getString(R.string.error_permissions),
                    // Toast.LENGTH_LONG).show();
                }
            }
        }
    }

    /**
     * SPRD: Display the change freq dialog
     * 
     * @param frequency The display frequency
     */
    public void showChangeFreqDialog(int frequency) {
        if (mService != null) {
            String name = FmStation.getStationName(mContext, frequency);
            FmFavoriteFreqEditDialog newFragment = FmFavoriteFreqEditDialog.newInstance(frequency);
            newFragment.show(mFragmentManager, "TAG_EDIT_FAVORITE_FREQ");
            mFragmentManager.executePendingTransactions();
        }
    }

    @Override
    public void editFavoriteFreq(int oldFreq, int changeFreq) {
        Log.d(TAG, "oldFreq=" + oldFreq + ", changeFreq=" + changeFreq);
        if (!FmUtils.isValidStation(changeFreq)) {
            showToast(getString(R.string.range_error));
            return;
        }
        String title = getString(R.string.toast_station_freq_changed);
        if (oldFreq == changeFreq) {
            FmSnackBar.make(FmMainActivity.this, title, null, null, FmSnackBar.DEFAULT_DURATION)
                    .show();
            return;
        }
        Cursor cursor = null;
        // remove dup freq from db
        FmStation.deleteStationInDb(mContext, changeFreq);

        // build new station infor from old.
        ContentValues newStation = new ContentValues();
        try {
            cursor = mContext.getContentResolver().query(Station.CONTENT_URI, null,
                    Station.FREQUENCY + "=?"
                    , new String[] {
                        String.valueOf(oldFreq)
                    }, null);
            if (cursor != null && cursor.moveToFirst()) {
                if (cursor.getCount() >= 1) {
                    newStation.put(Station.FREQUENCY, changeFreq);
                    newStation.put(Station.IS_FAVORITE,
                            cursor.getString(cursor.getColumnIndex(Station.IS_FAVORITE)));
                    newStation.put(Station.STATION_NAME,
                            cursor.getString(cursor.getColumnIndex(Station.STATION_NAME)));
                    /*
                     * SPRD: bug569373 Non-RDS station show RDS information after edit favorite rds
                     * station newStation.put(Station.RADIO_TEXT,
                     * cursor.getString(cursor.getColumnIndex(Station.RADIO_TEXT)));
                     * newStation.put(Station.PROGRAM_SERVICE,
                     * cursor.getString(cursor.getColumnIndex(Station.PROGRAM_SERVICE)));
                     */
                }
            }
        } catch (Exception e) {

        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        // update FAVORITE flag for old.
        ContentValues updateContentValues = new ContentValues();
        updateContentValues.put(Station.IS_FAVORITE, false);
        FmStation.updateStationToDb(mContext, oldFreq, updateContentValues);
        FmStation.insertStationToDb(mContext, newStation);
        mScroller.notifyAdatperChange();

        // update staion UI
        if (oldFreq == mCurrentStation) {
            mButtonAddToFavorite.setImageResource(R.drawable.btn_fm_favorite_off_selector);
        } else if (changeFreq == mCurrentStation) {
            mTextStationName.setText(newStation.getAsString(Station.STATION_NAME));
            mTextRds.setText(newStation.getAsString(Station.RADIO_TEXT));
        }
        FmSnackBar.make(FmMainActivity.this, title, null, null, FmSnackBar.DEFAULT_DURATION).show();
    }

    /**
     * SPRD:Bug545627 Add RDS feature for brcm.
     * 
     * @{
     */
    public int onRdsDataUpdate(int type, String data, String to) {
        Log.d(TAG, "onRdsDataUpdate type: " + type + ",data: " + data + ",to: " + to);
        Message msg = mHandler.obtainMessage(FmListener.MSG_RDS_DATA_UPDATE);
        Bundle rdsdata = new Bundle();
        rdsdata.putString("rdsdata", data);
        rdsdata.putInt("rdsdatatype", type);
        msg.setData(rdsdata);
        mHandler.sendMessage(msg);
        return 0;
    }

    @Override
    public int onError(int state, String info, String to) {
        Log.d(TAG, "onError info: " + info + ",to: " + to);
        return 0;
    }

    public int onAfJumpChange(int freq, String data, String to) {
        Log.d(TAG, "onAfJumpChange freq: " + freq + ",data: " + data + ",to: " + to);
        return 0;
    }

    public int onRdsAFStateUpdate(int rdsType, String afState, String to) {
        Log.d(TAG, "onRdsAFStateUpdate rdsType: " + rdsType + ",afState: " + afState + ",to: "
                + to);
        return 0;
    }

    @Override
    public int onStateChange(int state, String info, String to) {
        Log.d(TAG, "onStateChange state: " + state + ",info: " + info + ",to: " + to);
        switch (state) {
            case FmBrcmUtil.FM_STATE_CHANGE_FREQ:
                Message msg = mHandler.obtainMessage(FmListener.MSG_CLEAR_RDS_DATA);
                mHandler.sendMessage(msg);
                break;
            default:
                break;
        }
        return 0;
    }

    /**
     * @}
     */

    /** SPRD: bug568587, Regularly power off. @{ */

    class RadioGroupListener implements OnCheckedChangeListener {

        @Override
        public void onCheckedChanged(RadioGroup group, int checkedId) {
            switch (checkedId) {
                case R.id.time_10min:
                    FmUtils.GLOBAL_CUSTOM_TIME = 15;
                    mCustomEdit.setText("");
                    mCustomEdit.setEnabled(false);
                    mEditContainer.setVisibility(View.GONE);
                    break;
                case R.id.time_20min:
                    FmUtils.GLOBAL_CUSTOM_TIME = 30;
                    mCustomEdit.setText("");
                    mCustomEdit.setEnabled(false);
                    mEditContainer.setVisibility(View.GONE);
                    break;
                case R.id.time_30min:
                    FmUtils.GLOBAL_CUSTOM_TIME = 60;
                    mCustomEdit.setText("");
                    mCustomEdit.setEnabled(false);
                    mEditContainer.setVisibility(View.GONE);
                    break;
                case R.id.time_60min:
                    FmUtils.GLOBAL_CUSTOM_TIME = 90;
                    mCustomEdit.setText("");
                    mCustomEdit.setEnabled(false);
                    mEditContainer.setVisibility(View.GONE);
                    break;
                case R.id.time_custom:
                    mCustomEdit.setEnabled(true);
                    mEditContainer.setVisibility(View.VISIBLE);
                    break;
                default:
                    return;
            }
        }
    }

    private void saveDefaulCustomTime() {
        SharedPreferences storageSP = mContext.getSharedPreferences(FM_REGULAYLY_POWER_OFF,
                Context.MODE_PRIVATE);
        SharedPreferences.Editor storageEdit = storageSP.edit();
        storageEdit.putFloat(DEFAULT_CUSTOM_SETTING, FmUtils.GLOBAL_CUSTOM_TIME);
        storageEdit.commit();
    }

    class MyTimeCountListener implements TimeCountListener {

        @Override
        public void onTimerTick(long millisUntilFinished) {
            String str = makeTimeString4MillSec(millisUntilFinished);
            if (mCustomTime != null) {
                mCustomTime.setText(str);
            }
        }

        @Override
        public void onTimerFinish() {
            if (mCustomTime != null) {
                mCustomTime.setText("");
            }
            if (null == mService) {
                Log.e(TAG, "TimeCount onFinish, mService is null");
                return;
            }
            if (mService.getPowerStatus() == FmService.POWER_UP) {
                powerDownFm();
            }
        }

        @Override
        public void onUpdateTimeString() {
            timerTextUpdate();
        }
    }

    private void timerTextUpdate() {
        if (mService != null && mMenuItemRegularlyPoweroff != null) {
            if (mService.mIsStartTimeCount) {
                mMenuItemRegularlyPoweroff.setTitle(R.string.cancel_regularly_poweroff);
            } else {
                mCustomTime.setText("");
                mMenuItemRegularlyPoweroff.setTitle(R.string.fm_regularly_poweroff);
            }
        }
    }

    private String makeTimeString4MillSec(long millSec) {
        String str = "";
        int hour = 0;
        int minute = 0;
        int second = 0;
        second = Math.round(millSec / 1000);
        if (second > 59) {
            minute = second / 60;
            second = second % 60;
        }
        if (minute > 59) {
            hour = minute / 60;
            minute = minute % 60;
        }
        if (hour != 0) {
            str = (hour < 10 ? "0" + hour : hour) + ":"
                    + (minute < 10 ? "0" + minute : minute) + ":"
                    + (second < 10 ? "0" + second : second);
        } else {
            str = (minute < 10 ? "0" + minute : minute) + ":"
                    + (second < 10 ? "0" + second : second);
        }
        if (hour == 0 && minute == 0 && second == 0) {
            str = "00:00";
        }
        return str;
    }

    /** @} */
    /* SPRD: bug568587 FM new UI @{ */
    private void enterRecordList() {
        if (mService != null) {
            if (mService.isActivityForeground()) {
                Intent intent = new Intent();
                intent.setClass(FmMainActivity.this, FmRecordListActivity.class);
                startActivity(intent);
            }
        }
    }
    /* @} */

    /* SPRD: Fix for bug 589228 forbid turn on FM in airplane mode. @{ */
    private void changeToAirPlaneLayout() {
        mNoEarphoneTextLayout.setVisibility(View.GONE);
        mNoHeadsetImgView.setVisibility(View.GONE);
        mNoHeadsetImgViewWrap.setVisibility(View.GONE);
        mNoHeadsetLayout.setVisibility(View.GONE);
        mMainLayout.setVisibility(View.VISIBLE);
        mBtnPlayContainer.setVisibility(View.VISIBLE);
        refreshImageButton(false);
        refreshActionMenuItem(false);
        refreshPopupMenuItem(false);
        refreshPlayButton(true);
        removeDialog(AIRPLANE_DIALOG);
        showDialog(AIRPLANE_DIALOG);
    }
    /* Bug 589228 End@} */
    private void setMenuVisibility(boolean isVisible){
        if (null != mMenuItemRecordList) {
          mMenuItemRecordList.setVisible(isVisible);
        }
        if (null != mMenuItemRegularlyPoweroff) {
          mMenuItemRegularlyPoweroff.setVisible(isVisible);
        }
        if (null != mMenuItemRDSSwitch) {
          mMenuItemRDSSwitch.setVisible(isVisible);
        }
   }
}
