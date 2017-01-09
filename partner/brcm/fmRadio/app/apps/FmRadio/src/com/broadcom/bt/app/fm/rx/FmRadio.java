/** Copyright 2009-2013 Broadcom Corporation
 **
 ** This program is the proprietary software of Broadcom Corporation and/or its
 ** licensors, and may only be used, duplicated, modified or distributed
 ** pursuant to the terms and conditions of a separate, written license
 ** agreement executed between you and Broadcom (an "Authorized License").
 ** Except as set forth in an Authorized License, Broadcom grants no license
 ** (express or implied), right to use, or waiver of any kind with respect to
 ** the Software, and Broadcom expressly reserves all rights in and to the
 ** Software and all intellectual property rights therein.
 ** IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 ** SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE 
 ** ALL USE OF THE SOFTWARE.  
 **
 ** Except as expressly set forth in the Authorized License,
 ** 
 ** 1.     This program, including its structure, sequence and organization, 
 **        constitutes the valuable trade secrets of Broadcom, and you shall 
 **        use all reasonable efforts to protect the confidentiality thereof, 
 **        and to use this information only in connection with your use of 
 **        Broadcom integrated circuit products.
 ** 
 ** 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED 
 **        "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, 
 **        REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, 
 **        OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY 
 **        DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, 
 **        NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, 
 **        ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR 
 **        CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 **        OF USE OR PERFORMANCE OF THE SOFTWARE.
 **
 ** 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 **        ITS LICENSORS BE LIABLE FOR 
 **        (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY 
 **              DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO 
 **              YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM 
 **              HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR 
 **        (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE 
 **              SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE 
 **              LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF 
 **              ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 */
package com.broadcom.bt.app.fm.rx;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.Window;
import android.view.ContextMenu.ContextMenuInfo;
import android.widget.TextView;
import android.widget.Toast;

import com.broadcom.bt.app.fm.FmConstants;
import com.broadcom.bt.app.fm.R;
import com.broadcom.fm.fmreceiver.FmProxy;
import com.broadcom.fm.fmreceiver.IFmReceiverEventHandler;
import com.broadcom.fm.fmreceiver.IFmProxyCallback;

import com.broadcom.bt.app.fm.FmSnrThresholdBar;

/**
 * An example FM Receiver application that supports the following features: <li>
 * Access to the FM Receiver Service. <li>Preset station management. <li>RDS
 * capability
 */
public class FmRadio extends Activity implements IRadioViewRxTouchEventHandler,
        OnSharedPreferenceChangeListener, IFmProxyCallback {

    private static final String TAG = "FmRxRadio";
    private static final boolean V = true;
    public static final String PLAYSTATE_CHANGED = "com.android.music.playstatechanged";

    /* CONSTANT BLOCK */

    /* GUI message codes. */
    private static final int GUI_UPDATE_MSG_SIGNAL_STATUS = 1;
    private static final int GUI_UPDATE_MSG_MUTE_STATUS = 2;
    private static final int GUI_UPDATE_MSG_FREQ_STATUS = 3;
    private static final int GUI_UPDATE_MSG_WORLD_STATUS = 4;
    private static final int GUI_UPDATE_MSG_RDS_STATUS = 5;
    private static final int GUI_UPDATE_MSG_AF_STATUS = 6;
    private static final int GUI_UPDATE_MSG_RDS_DATA = 7;
    private static final int SIGNAL_CHECK_PENDING_EVENTS = 20;
    private static final int NFL_TIMER_EVENT = 21;

    private static final int MENU_CH_SET = 1;
    private static final int MENU_CH_CLEAR = 2;
    private static final int MENU_CH_CANCEL = 3;

    /* Default frequency. */
    private static final int DEFAULT_FREQUENCY = 8810;

    /* VARIABLE BLOCK */

    /* Object instant references. */
    private FmReceiveView mView;
    private FmReceiverEventHandler mFmReceiverEventHandler;
    private FmProxy mFmReceiver;
    private SharedPreferences mSharedPrefs;
    private final static int NUM_OF_CHANNELS = 10;
    private int mChannels[] = new int[NUM_OF_CHANNELS];
    private final static String freqPreferenceKey = "channel";
    private final static String lastFreqPreferenceKey = "last";

    /* Local GUI status variables. */
    private int mWorldRegion = FmProxy.FUNC_REGION_DEFAULT;

    private int mFrequency = DEFAULT_FREQUENCY;
    private int mFrequencyStep = 10; // Step in increments of 100 Hz
    private int mMinFreq, mMaxFreq; // updated with mPendingRegion
    private int mNfl;

    private boolean mSeekInProgress = false;
    boolean mPowerOffRadio = false;

    /* Pending values. (To be requested) (Startup values specified) */
    private int mPendingRegion = FmProxy.FUNC_REGION_DEFAULT;
    private int mPendingDeemphasis = FmProxy.DEEMPHASIS_75U;
    private int mPendingAudioMode = FmProxy.AUDIO_MODE_AUTO;
    private int mPendingAudioPath = FmProxy.AUDIO_PATH_WIRE_HEADSET;
    private int mPendingFrequency = DEFAULT_FREQUENCY;
    private boolean mPendingMute = false;
    private int mPendingScanStep = FmProxy.FREQ_STEP_100KHZ; // Step in
                                                             // increments of
                                                             // 100 Hz
    private int mPendingScanMethod = FmProxy.SCAN_MODE_FAST;
    private int mPendingRdsMode = FmProxy.RDS_MODE_OFF;
    private int mPendingRdsType = FmProxy.RDS_COND_NONE;
    private int mPendingAfMode = -1; /* force update to be sent at power on */
    private int mPendingNflEstimate = FmProxy.NFL_MED;
    private int mPendingSearchDirection = FmProxy.SCAN_MODE_NORMAL;
    private boolean mPendingLivePoll = false;
    private int mPendingSnrThreshold = FmProxy.FM_MIN_SNR_THRESHOLD;
    private int mPendingLivePollinterval = 2000; // 2 second polling of rssi

    /* Pending updates. */
    // sprivate boolean enabledUpdatePending = false;
    private boolean shutdownPending = false;
    private boolean worldRegionUpdatePending = false;
    private boolean audioModeUpdatePending = false;
    private boolean audioPathUpdatePending = false;
    private boolean frequencyUpdatePending = false;
    private boolean muteUpdatePending = false;
    private boolean scanStepUpdatePending = false;
    private boolean rdsModeUpdatePending = false;
    private boolean nflEstimateUpdatePending = false;
    private boolean stationSearchUpdatePending = false;
    private boolean livePollingUpdatePending = false;
    private boolean fmVolumeUpdatepending = false;
    private boolean fmSetSnrThresholdPending = false;
    private boolean mFinish = false;
    private boolean bFinishCalled = false;
    private boolean mRadioIsOn = false;

    /* Array of RDS program type names (English). */
    String mRdsProgramTypes[];

    NotificationManager mNotificationManager;
    TelephonyManager mTelephonyManager;
    AudioManager mAudioManager;

    private HeadsetPlugUnplugBroadcastReceiver mHeadsetPlugUnplugBroadcastReceiver;
    private MyPhoneStateListener mPhoneStateListener;

    private boolean mInCall = false;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate");

        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);

        mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        mTelephonyManager = (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);

        mSharedPrefs = PreferenceManager.getDefaultSharedPreferences(this);
        mSharedPrefs.registerOnSharedPreferenceChangeListener(this);

        if (mAudioManager.isMusicActive()) {
            Toast.makeText(getApplicationContext(), "Stop audio from other app and relaunch",
                    Toast.LENGTH_LONG).show();
            finish();
        }

        for (int i = 0; i < NUM_OF_CHANNELS; i++)
            mChannels[i] = mSharedPrefs.getInt(freqPreferenceKey + i, 0);

        /* Retrieve array of program types from arrays.xml. */
        mRdsProgramTypes = getResources().getStringArray(R.array.fm_rds_pty_names);
        mView = (FmReceiveView) (View.inflate(this, R.layout.radio_rx, null));
        mView.init(this);
        setContentView(mView);

        updateMinMaxFrequencies();
        mView.setFrequencyStep(mFrequencyStep);

        for (int i = FmConstants.BUTTON_CH_1; i <= FmConstants.BUTTON_CH_10; i++) {
            View v = mView.findViewById(i);
            registerForContextMenu(v);
        }
        if (Settings.System.getInt(getApplicationContext().getContentResolver(),
                Settings.System.AIRPLANE_MODE_ON, 0) != 0) {
            Toast.makeText(this, "Cannot open FMRadio in Airplane mode", Toast.LENGTH_SHORT).show();
            bFinishCalled = true;
            finish();
        }
        mPhoneStateListener = new MyPhoneStateListener();
        mTelephonyManager.listen(mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);
        registerReceiver(mMediaStateReceiver, new IntentFilter(PLAYSTATE_CHANGED));
        registerReceiver(mAirplaneModeReceiver, new IntentFilter(
                Intent.ACTION_AIRPLANE_MODE_CHANGED));
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    protected void onResume() {
        if (V) {
            Log.d(TAG, "onResume");
        }

        if (mAudioManager.isMusicActive()) {
            finish();
        }

        if (mFmReceiver == null && !bFinishCalled) { // Avoid calling getProxy
                                                     // is finish has been
                                                     // called already
            if (V) {
                Log.d(TAG, "Getting FmProxy proxy...");
            }
            FmProxy.getProxy(this, this);
        }

        setVolumeControlStream(AudioManager.STREAM_MUSIC);
        if (mHeadsetPlugUnplugBroadcastReceiver == null)
            mHeadsetPlugUnplugBroadcastReceiver = new HeadsetPlugUnplugBroadcastReceiver();
        // sticky intent - use last value if exists, otherwise no headset
        Intent intent = registerReceiver(mHeadsetPlugUnplugBroadcastReceiver, new IntentFilter(
                Intent.ACTION_HEADSET_PLUG));
        if (intent != null)
            mHeadsetPlugUnplugBroadcastReceiver.onReceive(this, intent);
        else
            wiredHeadsetIsOn(false);
        super.onResume();
    }

    protected void onPause() {
        if (V) {
            Log.d(TAG, "onPause");
        }
        this.unregisterReceiver(mHeadsetPlugUnplugBroadcastReceiver);

        if (mPendingFrequency != 0) { // save last frequency
            Editor e = mSharedPrefs.edit();
            e.putInt(lastFreqPreferenceKey, mPendingFrequency);
            e.apply();
            if (!shutdownPending) {
                showNotification();
            } else {
                mNotificationManager.cancelAll();
            }
        }
        /*
         * if (mFmReceiver != null) { if (V) { Log.d(TAG,
         * "Finishing FmProxy proxy..."); }
         * mFmReceiver.unregisterEventHandler(); mFmReceiver.finish();
         * mFmReceiver = null; }
         */
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        Log.d(TAG, "Calling onDestroy()");
        unregisterReceiver(mMediaStateReceiver);
        unregisterReceiver(mAirplaneModeReceiver);
        if (mFmReceiver != null) {
            if (V) {
                Log.d(TAG, "Finishing FmProxy proxy...");
            }
            mFmReceiver.unregisterEventHandler();
            mFmReceiver.finish();
            mFmReceiver = null;
        }
        mTelephonyManager.listen(mPhoneStateListener, 0);

    }

    @Override
    public void onBackPressed() {
        moveTaskToBack(true);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            moveTaskToBack(true);
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    private BroadcastReceiver mMediaStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(PLAYSTATE_CHANGED)) {
                Log.d(TAG, intent.toString());
                boolean isPlaying = false;
                // Media service sends playing info in two different "extra"
                // So check for both "playing" and "playstate"
                // to know whether Media is playing.
                if (intent.hasExtra("playing")) {
                    isPlaying = intent.getBooleanExtra("playing", false);
                } else if (intent.hasExtra("playstate")) {
                    // This "playstate" gives either media is actively playing
                    // or paused,
                    String playState = intent.getStringExtra("playstate");
                    if (0 == playState.compareTo("playing")) {
                        isPlaying = true;
                    }
                }
                Log.d(TAG, "received: " + action + ", playing: " + isPlaying);
                if (!isPlaying) {
                    // This will send the Audio Path during
                    // retryPendingCommands() properly
                    audioPathUpdatePending = true;
                    return;
                }
                Log.d(TAG, "Media player started !!.. Shutting down the fm Radio..");
                if (mFmReceiver != null)
                    mFmReceiver.setAudioPath(FmProxy.AUDIO_PATH_NONE);
                bFinishCalled = true;
                if (null != mFmReceiver && mFmReceiver.getRadioIsOn() == true) {
                    mPowerOffRadio = true;
                    powerDownSequence();
                    shutdownPending = true;
                    mFinish = true;
                } else {
                    finish();
                }
                mNotificationManager.cancelAll();
            }
        }
    };

    private BroadcastReceiver mAirplaneModeReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.d(TAG, "mAirplaneModeReceiver action: " + action);
            if (action.equals(Intent.ACTION_AIRPLANE_MODE_CHANGED)) {
                boolean isModeOn = intent.getBooleanExtra("state", false);
                Log.d(TAG, "received: " + action + ", playing: " + isModeOn);
                if (!isModeOn) {
                    audioPathUpdatePending = true; // This will send the Audio
                                                   // Path during
                                                   // retryPendingCommands()
                                                   // properly
                    return;
                }
                Log.d(TAG, "Airplane Mode ON !!.. Shutting down the fm Radio..");
                Toast.makeText(getApplicationContext(), "Shutting down FMRadio in Airplane mode",
                        Toast.LENGTH_SHORT).show();
                if (mFmReceiver != null)
                    mFmReceiver.setAudioPath(FmProxy.AUDIO_PATH_NONE);
                bFinishCalled = true;
                if (null != mFmReceiver && mFmReceiver.getRadioIsOn() == true) {
                    mPowerOffRadio = true;
                    powerDownSequence();
                    shutdownPending = true;
                    mFinish = true;
                } else {
                    finish();
                }
                mNotificationManager.cancelAll();
            }
        }
    };

    /**
     * This function is called to initialize pending operations and activate the
     * FM Radio HW on startup.
     */
    private void powerUpSequence() {
        if (V) {
            Log.d(TAG, "powerUpSequence()");
        }
        int status;

        /* Set pending updates to trigger on response from startup. */
        // enabledUpdatePending = false;
        shutdownPending = false;
        audioModeUpdatePending = true;
        audioPathUpdatePending = true;
        nflEstimateUpdatePending = false;
        frequencyUpdatePending = true;
        scanStepUpdatePending = true;
        rdsModeUpdatePending = true;
        fmVolumeUpdatepending = true;
        livePollingUpdatePending = false;
        stationSearchUpdatePending = false;
        mSeekInProgress = false;
        fmSetSnrThresholdPending = false;

        /* Initialize the radio. This can give us GUI initialization info. */
        Log.d(TAG, "Turning on radio... mFmReceiver = " + mFmReceiver + " ; Softmute state:"
                + FmConstants.FM_SOFTMUTE_FEATURE_ENABLED);
        if (mFmReceiver == null) {
            Log.e(TAG, "Invalid FM Receiver Proxy!!!!");
            return;
        }
        if (FmConstants.FM_SOFTMUTE_FEATURE_ENABLED)
            status = mFmReceiver.turnOnRadio(FmProxy.FUNC_REGION_NA | FmProxy.FUNC_RBDS
                    | FmProxy.FUNC_AF | FmProxy.FUNC_SOFTMUTE, getPackageName());
        else
            status = mFmReceiver.turnOnRadio(FmProxy.FUNC_REGION_NA | FmProxy.FUNC_RBDS
                    | FmProxy.FUNC_AF, getPackageName());
        Log.d(TAG, "Turn on radio status = " + status);
        if (status == FmProxy.STATUS_OK) {
            // powerupComplete();
            // As turnOnRadio an asynchronous call, the callbacks will
            // initialize the frequencies accordingly.
            // Update only the GUI
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_FREQ_STATUS;
            msg.arg1 = mPendingFrequency;
            msg.arg2 = 1;
            viewUpdateHandler.sendMessage(msg);
        } else {
            /* Add recovery code here if startup fails. */
            String error = getString(R.string.error_failed_powerup) + "\nStatus = " + status;
            Log.e(TAG, error);
            displayErrorMessageAndExit(error);
        }
    }

    /**
     * This function is called to initialize pending operations and activate the
     * FM Radio HW on startup.
     */
    private boolean powerDownSequence() {
        if (V) {
            Log.d(TAG, "powerDownSequence()");
        }

        /* Set pending updates to trigger on response from startup. */
        // enabledUpdatePending = false;
        shutdownPending = true;
        audioModeUpdatePending = false;
        nflEstimateUpdatePending = false;
        frequencyUpdatePending = false;
        scanStepUpdatePending = false;
        rdsModeUpdatePending = false;
        livePollingUpdatePending = false;
        stationSearchUpdatePending = false;
        fmVolumeUpdatepending = false;
        fmSetSnrThresholdPending = false;

        /* Initialize the radio. This can give us GUI initialization info. */
        if (mFmReceiver != null) {
            mFmReceiver.setAudioPath(mFmReceiver.AUDIO_PATH_NONE);
            int status = mFmReceiver.turnOffRadio();
            if (status != FmProxy.STATUS_OK) {
                shutdownPending = false;
                AlertDialog alertDialog = new AlertDialog.Builder(this).create();
                alertDialog.setMessage(getString(R.string.error_failed_shutdown) + "\nStatus = "
                        + status);
                alertDialog.setButton(getString(android.R.string.ok),
                        (DialogInterface.OnClickListener) null);
                alertDialog.show();
                return false; // failure
            }
        }
        if (mPendingFrequency != 0) { // save last frequency
            Editor e = mSharedPrefs.edit();
            e.putInt(lastFreqPreferenceKey, mPendingFrequency);
            e.apply();
        }
        return true; // success
    }

    private void powerupComplete() {
        Log.d(TAG, "powerupcomplete");
        mPendingFrequency = mSharedPrefs.getInt(lastFreqPreferenceKey, DEFAULT_FREQUENCY);
        updateFrequency(mPendingFrequency);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        int action = event.getAction();
        int keyCode = event.getKeyCode();
        if (action == KeyEvent.ACTION_UP) {
            if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
                int newVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
                mFmReceiver.setFMVolume(FmProxy.FM_VOLUME_MAX * newVolume
                        / mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC));
            } else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
                int newVolume = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
                mFmReceiver.setFMVolume(FmProxy.FM_VOLUME_MAX * newVolume
                        / mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC));
            }
        }
        return super.dispatchKeyEvent(event);
    }

    /**
     * Request new region operation and pend if necessary.
     * 
     * @param sp
     *            the shared preferences reference to get world region from.
     */
    private void updateWorldRegion(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateWorldRegion()");
        }
        /* Extract preferences and request these settings. */
        mPendingRegion = Integer.parseInt(sp.getString(FmRadioSettings.FM_PREF_WORLD_REGION, "0"));
        updateMinMaxFrequencies();
        mPendingDeemphasis = Integer
                .parseInt(sp.getString(FmRadioSettings.FM_PREF_DEEMPHASIS, "0"));
        if (mPendingRegion == FmProxy.FUNC_REGION_DEFAULT)
            mPendingDeemphasis = FmProxy.DEEMPHASIS_75U;
        else if (mPendingRegion == FmProxy.FUNC_REGION_EUR
                || mPendingRegion == FmProxy.FUNC_REGION_JP
                || mPendingRegion == FmProxy.FUNC_REGION_JP_II)
            mPendingDeemphasis = FmProxy.DEEMPHASIS_50U;
        if (V) {
            Log.d(TAG, "Sending region (" + Integer.toString(mPendingRegion) + ")");
            Log.d(TAG, "Sending deemphasis (" + Integer.toString(mPendingDeemphasis) + ")");
        }
        if (null != mFmReceiver) {
            worldRegionUpdatePending = (FmProxy.STATUS_OK != mFmReceiver.setWorldRegion(
                    mPendingRegion, mPendingDeemphasis));
        }
    }

    /**
     * Request new scan step operation and pend if necessary.
     * 
     * @param sp
     *            the shared preferences reference to get scan step from.
     */
    private void updateScanStep(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateScanStep()");
        }
        /* Extract preferences and request these settings. */
        mPendingScanStep = Integer.parseInt(mSharedPrefs.getString(
                FmRadioSettings.FM_PREF_SCAN_STEP, "0"));
        if (V) {
            Log.d(TAG, "Sending scan step (" + Integer.toString(mPendingScanStep) + ")");
        }

        if (null != mFmReceiver) {
            scanStepUpdatePending = (FmProxy.STATUS_OK != mFmReceiver.setStepSize(mPendingScanStep));
            /*
             * If this succeeds, start using that scan step in manual step
             * updates.
             */
            if (!scanStepUpdatePending) {
                mFrequencyStep = (0 == mPendingScanStep) ? 10 : 5;
                mView.setFrequencyStep(mFrequencyStep);
            }
        }
    }

    /**
     * Request new FM volume operation and pend if necessary.
     * 
     * @param sp
     *            the shared preferences reference to FM volume text editor
     *            from.
     */
    private void updateFMVolume(int volume) {
        // CK - Send the command only of the Service is not busy
        Log.d(TAG, "updateFMVolume()");
        if (null != mFmReceiver) {
            fmVolumeUpdatepending = (FmProxy.STATUS_OK != mFmReceiver.setFMVolume(volume));
        }
    }

    /**
     * Request new RDS mode if necessary.
     * 
     * @param sp
     *            the shared preferences reference to get rds mode from.
     */
    private void updateRdsMode(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateRdsMode()");
        }
        /* Extract preferences and request these settings. */
        int newRdsMode = Integer.parseInt(mSharedPrefs.getString(
                FmRadioSettings.FM_PREF_RDS_ENABLED, "0"));
        int newAfMode = (mSharedPrefs.getBoolean(FmRadioSettings.FM_PREF_AF_ENABLED, false)) ? 1 : 0;

        if ((mPendingRdsMode == newRdsMode) && (mPendingAfMode == newAfMode)) {
            if (V) {
                Log.d(TAG, "RDS mode unchanged(" + Integer.toString(newRdsMode) + ")");
                Log.d(TAG, "AF mode unchanged(" + Integer.toString(newAfMode) + ")");
            }
            return;
        }
        mPendingRdsMode = newRdsMode;
        mPendingAfMode = newAfMode;

        if (V) {
            Log.d(TAG, "Sending RDS mode (" + Integer.toString(mPendingRdsMode) + ")");
            Log.d(TAG, "Sending AF mode (" + Integer.toString(mPendingAfMode) + ")");
        }
        if (null != mFmReceiver) {
            rdsModeUpdatePending = (FmProxy.STATUS_OK != mFmReceiver.setRdsMode(mPendingRdsMode,
                    FmProxy.RDS_FEATURE_PS | FmProxy.RDS_FEATURE_RT | FmProxy.RDS_FEATURE_TP
                            | FmProxy.RDS_FEATURE_PTY | FmProxy.RDS_FEATURE_PTYN, mPendingAfMode,
                    mNfl));
        }
    }

    /**
     * Request new audio mode operation and pend if necessary.
     * 
     * @param sp
     *            the shared preferences reference to get audio mode from.
     */
    private void updateAudioMode(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateAudioMode()");
        }
        /* Extract preferences and request these settings. */
        mPendingAudioMode = Integer.parseInt(mSharedPrefs.getString(
                FmRadioSettings.FM_PREF_AUDIO_MODE, "0"));
        if (V) {
            Log.d(TAG, "Sending audio mode (" + Integer.toString(mPendingAudioMode) + ")");
        }
        if (null != mFmReceiver) {
            audioModeUpdatePending = (FmProxy.STATUS_OK != mFmReceiver
                    .setAudioMode(mPendingAudioMode));
        }
    }

    /**
     * Request new audio path operation and pend if necessary.
     * 
     * @param sp
     *            the shared preferences reference to get audio path from.
     */
    private void updateAudioPath(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateAudioPath()");
        }
        /* Extract preferences and request these settings. */
        mPendingAudioPath = Integer
                .parseInt(mSharedPrefs.getString(FmRadioSettings.FM_PREF_AUDIO_PATH,
                        String.valueOf(FmProxy.AUDIO_PATH_WIRE_HEADSET)));
        if (V) {
            Log.d(TAG, "Sending audio path (" + Integer.toString(mPendingAudioPath) + ")");
        }
        if (null != mFmReceiver) {
            audioPathUpdatePending = (FmProxy.STATUS_OK != mFmReceiver
                    .setAudioPath(mPendingAudioPath));
            Log.v(TAG, "audioPathUpdatePending == " + audioPathUpdatePending);
        }
    }

    /**
     * Request frequency tuning operation and pend if necessary.
     * 
     * @param freq
     *            the frequency to tune to.
     */
    private void updateFrequency(int freq) {
        if (V) {
            Log.d(TAG, "updateFrequency() :" + freq);
        }
        /* Extract pending data and request these settings. */
        mPendingFrequency = freq;
        mView.setFrequencyGraphics(freq);
        mView.setSeekStatus(mSeekInProgress, (mPendingFrequency != mFrequency));
        mView.resetRdsText();

        if (V) {
            Log.d(TAG, "Sending frequency (" + Integer.toString(mPendingFrequency) + ")");
        }
        if (null != mFmReceiver) {
            frequencyUpdatePending = (FmProxy.STATUS_OK != mFmReceiver.tuneRadio(mPendingFrequency));
        }
    }

    /**
     * Request mute operation and pend if necessary.
     * 
     * @param muted
     *            true if muting requested, false otherwise.
     */
    private void updateMuted(boolean muted) {
        if (V) {
            Log.d(TAG, "updateMuted() :" + muted);
        }
        /* Extract pending data and request these settings. */
        mPendingMute = muted;
        if (V) {
            Log.d(TAG, "Sending muted (" + (mPendingMute ? "TRUE" : "FALSE") + ")");
        }
        if (mFmReceiver != null) {
            muteUpdatePending = (mFmReceiver.muteAudio(mPendingMute) != FmProxy.STATUS_OK);
        }
    }

    /**
     * Request NFL estimate operation and pend if necessary.
     * 
     * @param sp
     *            the shared preferences reference to get NFL mode from.
     */
    private void updateNflEstimate(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateNflEstimate()");
        }

        /* Extract preferences and request these settings. */
        mPendingNflEstimate = Integer.parseInt(mSharedPrefs.getString(
                FmRadioSettings.FM_PREF_NFL_MODE, "1"));
        if (V) {
            Log.d(TAG, "Sending NFL mode (" + Integer.toString(mPendingNflEstimate) + ")");
        }
        if (null != mFmReceiver) {
            nflEstimateUpdatePending = (FmProxy.STATUS_OK != mFmReceiver
                    .estimateNoiseFloorLevel(mPendingNflEstimate));
        }
    }

    /**
     * Request station search operation and pend if necessary.
     * 
     * @param direction
     *            the search direction can be up or down.
     */
    private void updateStationSearch(int direction) {
        int endFrequency = mPendingFrequency;

        if (V) {
            Log.d(TAG, "updateStationSearch()");
        }
        /* Extract pending data and request these settings. */
        mPendingSearchDirection = direction;
        if (V) {
            Log.d(TAG, "Sending search direction (" + Integer.toString(mPendingSearchDirection)
                    + ")");
        }

        if (mFmReceiver != null) {
            if (FmConstants.FM_COMBO_SEARCH_ENABLED) {
                if ((mPendingSearchDirection & FmProxy.SCAN_MODE_UP) == FmProxy.SCAN_MODE_UP) {
                    /* Increase the current listening frequency by one step. */
                    if ((mMinFreq <= mPendingFrequency) && (mPendingFrequency <= mMaxFreq)) {
                        mPendingFrequency = (int) (mPendingFrequency + mFrequencyStep);
                        if (mPendingFrequency > mMaxFreq)
                            mPendingFrequency = mMinFreq;
                    } else {
                        mPendingFrequency = mMinFreq;
                        endFrequency = mMaxFreq;
                    }
                } else {
                    /* Decrease the current listening frequency by one step. */
                    if ((mMinFreq <= mPendingFrequency) && (mPendingFrequency <= mMaxFreq)) {
                        mPendingFrequency = (int) (mPendingFrequency - mFrequencyStep);
                        if (mPendingFrequency < mMinFreq)
                            mPendingFrequency = mMaxFreq;
                    } else {
                        mPendingFrequency = mMaxFreq;
                        endFrequency = mMinFreq;
                    }
                }

                stationSearchUpdatePending = mSeekInProgress = (FmProxy.STATUS_OK != mFmReceiver
                        .seekStationCombo(mPendingFrequency, endFrequency,
                                FmProxy.MIN_SIGNAL_STRENGTH_DEFAULT, mPendingSearchDirection,
                                mPendingScanMethod, FmConstants.COMBO_SEARCH_MULTI_CHANNEL_DEFAULT,
                                mPendingRdsType, FmProxy.RDS_COND_PTY_VAL));
            } else {
                stationSearchUpdatePending = mSeekInProgress = (FmProxy.STATUS_OK != mFmReceiver
                        .seekStation(mPendingSearchDirection, FmProxy.MIN_SIGNAL_STRENGTH_DEFAULT));
            }
            if (mSeekInProgress)
                mView.setSeekStatus(true, false); // no new frequency, just turn
                                                  // on the indicator
        }
    }

    /**
     * Update the live polling settings from preferences and pend if necessary.
     *
     * @param sp
     *            the shared preferences reference to get settings from.
     */
    private void updateLivePolling(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateLivePolling()");
        }

        /* Extract preferences and request these settings. */
        mPendingLivePoll = sp.getBoolean(FmRadioSettings.FM_PREF_LIVE_POLLING, false);
        mPendingLivePollinterval = Integer.parseInt(sp.getString(
                FmRadioSettings.FM_PREF_LIVE_POLL_INT, "2000"));
        if (V) {
            Log.d(TAG, "Sending live poll (" + (mPendingLivePoll ? "TRUE" : "FALSE") + ")");
            Log.d(TAG, "Sending live poll interval (" + Integer.toString(mPendingLivePollinterval)
                    + ")");
        }

        if (null != mFmReceiver) {
            livePollingUpdatePending = (FmProxy.STATUS_OK != mFmReceiver.setLiveAudioPolling(
                    mPendingLivePoll, mPendingLivePollinterval));
        }
    }

    /**
     * Update the SNR Threshold from preferences and pend if necessary.
     *
     * @param sp
     *            the shared preferences reference to get settings from.
     */
    private void updateSetSnrThreshold(SharedPreferences sp) {
        if (V) {
            Log.d(TAG, "updateSetSnrThreshold()");
        }

        /* Extract preferences and request these settings. */
        mPendingSnrThreshold = Integer.parseInt(sp.getString(FmRadioSettings.FM_PREF_SNR_THRESHOLD,
                String.valueOf(FmProxy.FM_MIN_SNR_THRESHOLD)));
        if (V) {
            Log.d(TAG, "Setting SNR Threshold(" + mPendingSnrThreshold + ")");
        }

        if (null != mFmReceiver) {
            fmSetSnrThresholdPending = (FmProxy.STATUS_OK != mFmReceiver
                    .setSnrThreshold(mPendingSnrThreshold));
        }
    }

    private void updateMinMaxFrequencies() {
        if ((mPendingRegion == FmProxy.FUNC_REGION_EUR)
                || (mPendingRegion == FmProxy.FUNC_REGION_NA)) {
            mMinFreq = FmConstants.MIN_FREQUENCY_US_EUROPE;
            mMaxFreq = FmConstants.MAX_FREQUENCY_US_EUROPE;
        } else if (mPendingRegion == FmProxy.FUNC_REGION_JP) {
            mMinFreq = FmConstants.MIN_FREQUENCY_JAPAN;
            mMaxFreq = FmConstants.MAX_FREQUENCY_JAPAN;
        } else if (mPendingRegion == FmProxy.FUNC_REGION_JP_II) {
            mMinFreq = FmConstants.MIN_FREQUENCY_JAPAN_II;
            mMaxFreq = FmConstants.MAX_FREQUENCY_JAPAN_II;
        } else {
            // where are we?
            return;
        }
        mView.setMinMaxFrequencies(mMinFreq, mMaxFreq);
    }

    /**
     * Execute any pending commands. Only the latest command will be stored.
     */
    private void retryPendingCommands() {
        if (V) {
            Log.d(TAG, "retryPendingCommands()");
        }
        /* Update event chain. */
        if (nflEstimateUpdatePending) {
            updateNflEstimate(mSharedPrefs);
        } else if (audioModeUpdatePending) {
            updateAudioMode(mSharedPrefs);
        } else if (audioPathUpdatePending) {
            updateAudioPath(mSharedPrefs);
        } else if (fmVolumeUpdatepending) {
            updateFMVolume(FmProxy.FM_VOLUME_MAX
                    * mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC)
                    / mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC));
        } else if (frequencyUpdatePending) {
            updateFrequency(mPendingFrequency);
        } else if (muteUpdatePending) {
            updateMuted(mPendingMute);
        } else if (stationSearchUpdatePending) {
            updateStationSearch(mPendingSearchDirection);
        } else if (rdsModeUpdatePending) {
            updateRdsMode(mSharedPrefs);
        } else if (fmSetSnrThresholdPending) {
            updateSetSnrThreshold(mSharedPrefs);
        } else if (scanStepUpdatePending) {
                updateScanStep(mSharedPrefs);
        } else {
            if (livePollingUpdatePending) {
                updateLivePolling(mSharedPrefs);
            }
            if (worldRegionUpdatePending) {
                updateWorldRegion(mSharedPrefs);
            }
        }
    }

    public void onProxyAvailable(Object ProxyObject) {
        Log.d(TAG, "onProxyAvailable bFinishCalled:" + bFinishCalled);
        if (mFmReceiver == null)
            mFmReceiver = (FmProxy) ProxyObject;
        if (mFmReceiver == null) {
            String error = getString(R.string.error_unable_to_get_proxy);
            Log.e(TAG, error);
            displayErrorMessageAndExit(error);
            return;
        }

        /* Initiate audio startup procedure. */
        if (null != mFmReceiver && mFmReceiverEventHandler == null) {
            mFmReceiverEventHandler = new FmReceiverEventHandler();
            mFmReceiver.registerEventHandler(mFmReceiverEventHandler);
        }
        /* make sure we update frequency display and volume etc upon resume */
        if (!mFmReceiver.getRadioIsOn()) {
            if (mFinish || bFinishCalled) {
                Log.d(TAG, "Finish already initiated here. Hence exiting");
                return;
            }
            mPendingFrequency = mSharedPrefs.getInt(lastFreqPreferenceKey, DEFAULT_FREQUENCY);
            powerUpSequence();
        } else {
            powerupComplete();
            mFmReceiver.getStatus(); // is this even needed?
        }
    }

    /**
     * proxy is Unavailable. mostly likely because FM service crashed in this
     * case we need to force the call to onDestroy() to avoid sending any Fm
     * commands to the proxy.
     */
    public void onProxyUnAvailable() {
        Log.w(TAG, "onProxyUnAvailable() bFinishCalled:"
                + bFinishCalled);
        /* TODO: a nice restart might be better */
        displayErrorMessageAndExit("Unexpected FM radio proxy close. FmRadio is closed");
    }

    private void displayErrorMessageAndExit(String errorMessage) {
        AlertDialog alertDialog = new AlertDialog.Builder(this).create();
        alertDialog.setMessage(errorMessage);
        alertDialog.setButton(getString(android.R.string.ok),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        finish();
                        return;
                    }
                });
        alertDialog.show();
    }

    // @Override
    // public boolean onCreateOptionsMenu(Menu menu) {
    // MenuInflater inflater = getMenuInflater();
    // inflater.inflate(R.menu.fm_rx_menu, menu);
    // return true;
    // }
    //
    // @Override
    // public boolean onOptionsItemSelected(MenuItem item) {
    // // Handle item selection
    // switch (item.getItemId()) {
    // case R.id.menu_item_settings:
    // Intent intent = new Intent();
    // intent.setClass(FmRadio.this, FmRadioSettings.class);
    // startActivity(intent);
    // return true;
    // case R.id.menu_item_exit:
    // /* Shutdown system. */
    // if (null != mFmReceiver) {
    // powerDownSequence();
    // }
    // finish();
    // return true;
    // default:
    // return super.onOptionsItemSelected(item);
    // }
    // }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        Intent intent = new Intent();
        intent.setClass(FmRadio.this, FmRadioSettings.class);
        startActivity(intent);

        return false;
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        super.onCreateContextMenu(menu, v, menuInfo);
        mLongPressChannel = v.getId() - FmConstants.BUTTON_CH_1; // 0 - 9
        if (mLongPressChannel < 0 || mLongPressChannel > 9)
            return;
        menu.clear();
        menu.add(
                Menu.NONE,
                MENU_CH_SET,
                Menu.NONE,
                getResources().getString(R.string.menu_ch_set, (mLongPressChannel + 1),
                        ((double) mPendingFrequency) / 100));
        if (mChannels[mLongPressChannel] != 0)
            menu.add(Menu.NONE, MENU_CH_CLEAR, Menu.NONE,
                    getResources().getString(R.string.menu_ch_clear, (mLongPressChannel + 1)));
        menu.add(Menu.NONE, MENU_CH_CANCEL, Menu.NONE,
                getResources().getString(R.string.menu_ch_cancel));

    }

    private static int mLongPressChannel;

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case MENU_CH_SET:
            setChannel(mLongPressChannel);
            mView.updateChannelButtons();
            return true;
        case MENU_CH_CLEAR:
            clearChannel(mLongPressChannel);
            mView.updateChannelButtons();
            return true;
        case MENU_CH_CANCEL:
            return true;
        default:
            return super.onContextItemSelected(item);
        }
    }

    /*
     * private void clearFmObjects() {
     * mSharedPrefs.unregisterOnSharedPreferenceChangeListener(this); if ((null
     * != mFmReceiver) && (null != mFmReceiverEventHandler)) { Log.d(TAG,
     * "Call mFmReceiver.unregisterEventHandler(...)");
     * mFmReceiver.unregisterEventHandler(); }
     * 
     * if (mFmReceiver != null) { mFmReceiver.finish(); mFmReceiver = null; }
     * 
     * }
     */

    /**
     * Keep listening for settings changes that require actions from FM.
     *
     * @param sharedPreferences
     *            the shared preferences.
     * @param key
     *            the key name of the preference that was changed.
     */
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
        if (V) {
            Log.d(TAG, "onSharedPreferenceChanged key " + key);
        }
        /*
         * if (key.equals(FmRadioSettings.FM_PREF_ENABLE)) { boolean isRadioOn =
         * mFmReceiver == null? false: mFmReceiver.getRadioIsOn(); boolean
         * isTurnOnRequest
         * =sharedPreferences.getBoolean(FmRadioSettings.FM_PREF_ENABLE, true);
         * if (V) { Log.d(TAG,"Turn Radio On/Off: turnOnRequested: " +
         * isTurnOnRequest + ", radioIsOn" + isRadioOn); } if (isTurnOnRequest
         * && !isRadioOn) { powerUpSequence(); } else if (!isTurnOnRequest &&
         * isRadioOn){ powerDownSequence(); } } else
         */
        if (key.equals(FmRadioSettings.FM_PREF_AUDIO_MODE)) {
            updateAudioMode(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_AUDIO_PATH)) {
            updateAudioPath(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_SCAN_STEP)) {
            updateScanStep(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_RDS_ENABLED)) {
            updateRdsMode(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_AF_ENABLED)) {
            updateRdsMode(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_WORLD_REGION)) {
            updateWorldRegion(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_DEEMPHASIS)) {
            updateWorldRegion(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_LIVE_POLLING)) {
            updateLivePolling(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_LIVE_POLL_INT)) {
            updateLivePolling(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_NFL_MODE)) {
            updateNflEstimate(sharedPreferences);
        } else if (key.equals(FmRadioSettings.FM_PREF_SNR_THRESHOLD)) {
            updateSetSnrThreshold(sharedPreferences);
        }
    }

    /**
     * This class ensures that only the UI thread access view core.
     */
    protected Handler viewUpdateHandler = new Handler() {

        /**
         * Internal helper function to update the GUI frequency display.
         *
         * @param freq
         *            the frequency (multiplied by 100) to cache and display.
         */
        private void updateFrequency(int freq, int isCompletedSeekInt) {
            Log.d(TAG, "updateFrequency  : " + freq);
            if (freq < 0)
                return;

            boolean isCompletedSeek = (isCompletedSeekInt != 0);
            /*
             * only update GUI, setting of actual frequency is done outside of
             * this class
             */
            // fixme -- all viewUpdateHandler functions should ONLY update GUI
            mFrequency = freq;
            if (isCompletedSeek) {
                mView.setFrequencyGraphics(freq);
                mPendingFrequency = mFrequency;
            }
            mView.setSeekStatus(mSeekInProgress, (mPendingFrequency != mFrequency));
            mView.resetRdsText();
        }

        public void handleMessage(Message msg) {

            Log.d(TAG, "handleMessage  : " + msg);
            /*
             * Here it is safe to perform UI view access since this class is
             * running in UI context.
             */

            switch (msg.what) {
            case GUI_UPDATE_MSG_SIGNAL_STATUS:
                mView.setSignalStrength(msg.arg1);
                break;
            case GUI_UPDATE_MSG_FREQ_STATUS:
                updateFrequency(msg.arg1, msg.arg2);
                break;
            case GUI_UPDATE_MSG_MUTE_STATUS:
                mView.setMutedState(msg.arg1 == FmConstants.MUTE_STATE_MUTED, mInCall);
                break;
            case GUI_UPDATE_MSG_RDS_STATUS:
                mView.setRdsState(msg.arg1);
                break;
            case GUI_UPDATE_MSG_AF_STATUS:
                mView.setAfState(msg.arg1);
                break;
            case GUI_UPDATE_MSG_RDS_DATA:
                mView.setRdsText(msg.arg1, msg.arg2, (String) msg.obj);
                break;
            // case GUI_UPDATE_MSG_DEBUG:
            // ((TextView)mView.findViewById(msg.arg1)).setText((String)msg.obj);
            // break;
            case SIGNAL_CHECK_PENDING_EVENTS:
                retryPendingCommands();
                break;
            default:
                break;
            }
        }
    };

    /**
     * Internal class used to specify FM callback/callout events from the FM
     * Receiver Service subsystem.
     */
    protected class FmReceiverEventHandler implements IFmReceiverEventHandler {

        /**
         * Transfers execution of GUI function to the UI thread.
         * 
         * @param rssi
         *            the signal strength to display.
         */
        private void displayNewSignalStrength(int rssi) {
            Log.d(TAG, "displayNewSignalStrength  : " + rssi);
            /* Update signal strength icon. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_SIGNAL_STATUS;
            msg.arg1 = rssi;
            viewUpdateHandler.sendMessage(msg);
        }

        private void displayNewRdsData(int rdsDataType, int rdsIndex, String rdsText) {
            Log.d(TAG, "displayNewRdsData  : ");
            /* Update RDS texts. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_RDS_DATA;
            msg.arg1 = rdsDataType;
            if (rdsDataType == FmConstants.RDS_ID_PTY_EVT) {
                if (rdsIndex < 0 || rdsIndex >= mRdsProgramTypes.length)
                    return; // invalid index
                msg.arg2 = rdsIndex;
                msg.obj = mRdsProgramTypes[rdsIndex];
            } else {
                msg.obj = (Object) rdsText;
            }
            viewUpdateHandler.sendMessage(msg);
        }

        /**
         * Transfers execution of GUI function to the UI thread.
         * 
         * @param rdsMode
         *            the RDS mode to display.
         */
        private void displayNewRdsState(int rdsMode) {
            Log.d(TAG, "displayNewRdsState  : " + rdsMode);
            /* Update RDS state icon. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_RDS_STATUS;
            msg.arg1 = rdsMode;
            viewUpdateHandler.sendMessage(msg);
        }

        /**
         * Transfers execution of GUI function to the UI thread.
         * 
         * @param afMode
         *            the AF mode to display.
         */
        private void displayNewAfState(int afMode) {
            Log.d(TAG, "displayNewAfState  : ");
            /* Update AF state icon. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_AF_STATUS;
            msg.arg1 = afMode;
            viewUpdateHandler.sendMessage(msg);
        }

        /**
         * Transfers execution of GUI function to the UI thread.
         * 
         * @param freq
         *            the frequency (multiplied by 100) to cache and display.
         */
        private void displayNewFrequency(int freq, int isCompletedSeekInt) {
            Log.d(TAG, "displayNewFrequency  : " + freq);
            /* Update frequency data. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_FREQ_STATUS;
            msg.arg1 = freq;
            msg.arg2 = isCompletedSeekInt; // nonzero if completed seek
            viewUpdateHandler.sendMessage(msg);
        }

        /**
         * Transfers execution of GUI function to the UI thread.
         * 
         * @param isMute
         *            TRUE if muted, FALSE if not.
         */
        private void displayNewMutedState(boolean isMute) {

            Log.d(TAG, "displayNewMutedState  : " + isMute);
            /* Update frequency data. */
            Message msg = Message.obtain();
            msg.what = GUI_UPDATE_MSG_MUTE_STATUS;
            msg.arg1 = isMute ? FmConstants.MUTE_STATE_MUTED : FmConstants.MUTE_STATE_UNMUTED;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onAudioModeEvent(int audioMode) {
            if (V) {
                Log.v(TAG, "onAudioModeEvent(" + audioMode + ")");
            }
            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onAudioPathEvent(int audioPath) {
            if (V) {
                Log.v(TAG, "onAudioPathEvent(" + audioPath + ")");
            }
            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onEstimateNoiseFloorLevelEvent(int nfl) {
            if (V) {
                Log.v(TAG, "onEstimateNoiseFloorLevelEvent(" + nfl + ")");
            }
            /* Local cache only! Not currently used directly. */
            mNfl = nfl;
            /* Update GUI display variables. */
            mView.LOW_SIGNAL_STRENGTH = nfl;
            mView.MEDIUM_SIGNAL_STRENGTH = nfl - 15;
            mView.HIGH_SIGNAL_STRENGTH = nfl - 25;
            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onLiveAudioQualityEvent(int rssi, int snr) {
            if (V) {
                Log.v(TAG, "onLiveAudioQualityEvent(" + rssi + ", " + snr + " )");
            }
            /* Update signal strength icon. */
            displayNewSignalStrength(rssi);

            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onRdsDataEvent(int rdsDataType, int rdsIndex, String rdsText) {
            if (V) {
                Log.v(TAG, "onRdsDataEvent(" + rdsDataType + ", " + rdsIndex + ")");
            }

            /* Update GUI text. */
            displayNewRdsData(rdsDataType, rdsIndex, rdsText);

            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onRdsModeEvent(int rdsMode, int alternateFreqHopEnabled) {
            if (V) {
                Log.v(TAG, "onRdsModeEvent(" + rdsMode + ", " + alternateFreqHopEnabled + ")");
            }

            /* Update signal strength icon. */
            displayNewRdsState(rdsMode);

            /* Update mute status. */
            displayNewAfState(alternateFreqHopEnabled);

            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onSeekCompleteEvent(int freq, int rssi, int snr, boolean seeksuccess) {
            if (V) {
                Log.v(TAG, "onSeekCompleteEvent(" + freq + ", " + rssi + ", " + snr + ", "
                        + seeksuccess + ")");
            }
            mSeekInProgress = false;
            mFrequency = freq;

            /* Update frequency display. */
            displayNewFrequency(freq, 1);
            /* Update signal strength icon. */
            displayNewSignalStrength(rssi);

            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);

            // Message msg0 = Message.obtain();
            // msg0.what = GUI_UPDATE_MSG_DEBUG;
            // msg0.arg1 = R.id.debugview1;
            // msg0.obj = "Seek: f="+freq+" rssi="+rssi+" s="+seeksuccess +
            // " update="+updateCount++;
            // viewUpdateHandler.sendMessage(msg0);
        }

        public void onStatusEvent(int freq, int rssi, int snr, boolean radioIsOn,
                int rdsProgramType, String rdsProgramService, String rdsRadioText,
                String rdsProgramTypeName, boolean isMute) {
            if (V) {
                Log.v(TAG, "onStatusEvent(" + freq + ", " + rssi + ", " + snr + ", " + radioIsOn
                        + ", " + rdsProgramType + ", " + rdsProgramService + ", " + rdsRadioText
                        + ", " + rdsProgramTypeName + ", " + isMute + ")");
            }

            if (mPowerOffRadio && !radioIsOn) {
                finish();
            }

            // String debug =
            // "freq: " + freq + "\n "+
            // "rssi: " + rssi + "\n "+
            // "radioIsOn: " + radioIsOn + "\n "+
            // "rdsProgramType/name: " + rdsProgramType + " " +
            // rdsProgramTypeName + "\n "+
            // "rdsProgramService: " + rdsProgramService + "\n "+
            // "rdsRadioText: " + rdsRadioText + "\n "+
            // "isMute: " + isMute + "\n" +
            // "update: " +updateCount++;
            //
            // Message msg0 = Message.obtain();
            // msg0.what = GUI_UPDATE_MSG_DEBUG;
            // msg0.arg1 = R.id.debugview2;
            // msg0.obj = debug;
            // viewUpdateHandler.sendMessage(msg0);

            /* Update frequency display. */
            displayNewFrequency(freq, 0);

            /* Update signal strength icon. */
            displayNewSignalStrength(rssi);

            /* Update mute status. */
            displayNewMutedState(isMute);

            /* Update GUI with RDS material if available. */
            // mRadioView.setRdsData(rdsProgramService, rdsRadioText,
            // rdsProgramTypeName);

            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onWorldRegionEvent(int worldRegion) {
            if (V) {
                Log.v(TAG, "onWorldRegionEvent(" + worldRegion + ")");
            }

            /* Local cache. */
            mWorldRegion = worldRegion;
            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }

        public void onVolumeEvent(int status, int volume) {
            if (V) {
                Log.v(TAG, "onVolumeEvent(" + status + ", " + volume + ")");
            }

            /* Check if any pending functions can be run now. */
            Message msg = Message.obtain();
            msg.what = SIGNAL_CHECK_PENDING_EVENTS;
            viewUpdateHandler.sendMessage(msg);
        }
    }

    /* manages frequency wrap around and scan step */
    private void buttonSeekFrequencyUp() {
        Log.v(TAG, "buttonSeekFrequencyUp: region " + mPendingRegion);

        mSeekInProgress = false;
        /* Increase the current listening frequency by one step. */
        if (mPendingFrequency == 0) {
            updateFrequency(DEFAULT_FREQUENCY);
        } else {
            if (mPendingFrequency < mMaxFreq) {
                if (mPendingFrequency % FmConstants.SCAN_STEP_100KHZ != 0)
                    mPendingFrequency = (int) (mPendingFrequency + FmConstants.SCAN_STEP_50KHZ);
                else
                    mPendingFrequency = (int) (mPendingFrequency + mFrequencyStep);
            } else {
                mPendingFrequency = mMinFreq;
            }
            updateFrequency(mPendingFrequency);
        }
    }

    /* manages frequency wrap around and scan step */
    private void buttonSeekFrequencyDown() {
        Log.v(TAG, "buttonSeekFrequencyDown region: " + mPendingRegion);

        mSeekInProgress = false;
        /* Decrease the current listening frequency by one step. */
        if (mPendingFrequency == 0) {
            updateFrequency(DEFAULT_FREQUENCY);
        } else {
            if (mPendingFrequency > mMinFreq) {
                if (mPendingFrequency % FmConstants.SCAN_STEP_100KHZ != 0)
                    mPendingFrequency = (int) (mPendingFrequency - FmConstants.SCAN_STEP_50KHZ);
                else
                    mPendingFrequency = (int) (mPendingFrequency - mFrequencyStep);
            } else {
                mPendingFrequency = mMaxFreq;
            }
            updateFrequency(mPendingFrequency);
        }
    }

    public void handleButtonEvent(int buttonId, int event) {
        /* Perform the functionality linked to the activated GUI release. */
        /* For each button, perform the requested action. */
        switch (buttonId) {
        case FmConstants.BUTTON_POWER_OFF:
            /* Shutdown system. */
            if (mFmReceiver != null && mFmReceiver.getRadioIsOn() == true) {
                mPowerOffRadio = true;
                powerDownSequence();
            }
            break;

        case FmConstants.BUTTON_MUTE_ON:
            updateMuted(true);
            break;

        case FmConstants.BUTTON_MUTE_OFF:
            updateMuted(false);
            break;

        case FmConstants.BUTTON_TUNE_DOWN:
            /*
             * Scan downwards to next station. Let the FM server determine NFL
             * for us.
             */
            updateStationSearch(FmProxy.SCAN_MODE_DOWN);
            break;

        case FmConstants.BUTTON_TUNE_UP:
            /*
             * Scan upwards to next station. Let the FM server determine NFL for
             * us.
             */
            updateStationSearch(FmProxy.SCAN_MODE_UP);
            break;

        case FmConstants.BUTTON_SEEK_DOWN:
            /* Decrease the current listening frequency by one step. */
            buttonSeekFrequencyDown();
            break;

        case FmConstants.BUTTON_SEEK_UP:
            /* Increase the current listening frequency by one step. */
            buttonSeekFrequencyUp();
            break;

        case FmConstants.BUTTON_SETTINGS:
            // openOptionsMenu();
            Intent intent = new Intent();
            intent.setClass(FmRadio.this, FmRadioSettings.class);
            startActivity(intent);
            break;

        default:
            break;
        }
    }

    public int[] getChannels() {
        return mChannels;
    }

    public void setChannel(int position) {
        mChannels[position] = mPendingFrequency;
        Editor e = mSharedPrefs.edit();
        e.putInt(freqPreferenceKey + position, mPendingFrequency);
        e.apply();
    }

    public void clearChannel(int position) {
        mChannels[position] = 0;
        Editor e = mSharedPrefs.edit();
        e.putInt(freqPreferenceKey + position, 0);
        e.apply();
    }

    public void selectChannel(int position) {
        mSeekInProgress = false;
        updateFrequency(mChannels[position]);
    }

    // callback from frequency slider
    public void setFrequency(int freq) {
        mSeekInProgress = false;
        updateFrequency(freq);
    }

    private void wiredHeadsetIsOn(boolean isOn) {
        // TODO: fancy warning dialog
        View v = mView.findViewById(R.id.plug_headset_warning);
        v.setVisibility(isOn ? View.GONE : View.VISIBLE);
    }

    public class HeadsetPlugUnplugBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equalsIgnoreCase(Intent.ACTION_HEADSET_PLUG)) {
                int state = intent.getIntExtra("state", 0);
                // System.out.println("_______ HeadsetPlugUnplugBroadcastReceiver.onReceive() state="+state);
                FmRadio.this.wiredHeadsetIsOn(state != 0);
            }
        }
    }

    private void showNotification() {
        int icon = R.drawable.icon;
        String tickerText = String.format("FM Radio (%.02f MHz)",
                ((double) this.mPendingFrequency) / 100);
        long when = System.currentTimeMillis();
        if (mInCall)
            tickerText += " (in call - muted)";
        Notification notification = new Notification(icon, tickerText, when);

        Context context = getApplicationContext();
        String contentTitle = "FmRadio app";
        String contentText = String.format("%.02f MHz", ((double) this.mPendingFrequency) / 100);
        if (mInCall)
            contentTitle += " (in call - muted)";
        Intent notificationIntent = new Intent(this, FmRadio.class);
        PendingIntent contentIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);

        notification.setLatestEventInfo(context, contentTitle, contentText, contentIntent);

        final int PLAYING_ID = 1;
        mNotificationManager.notify(PLAYING_ID, notification);
    }

    class MyPhoneStateListener extends PhoneStateListener {
        public void onCallStateChanged(int state, String incomingNumber) {
            updatePhoneState(state);
        }
    }

    private void updatePhoneState(int state) {
        String stateString = "N/A";
        switch (state) {
        case TelephonyManager.CALL_STATE_IDLE:
            stateString = "Idle";
            if (mFmReceiver != null && mFmReceiver.getRadioIsOn()) {
                Handler handler = new Handler();
                handler.postDelayed(new Runnable() {
                    public void run() {
                        mFmReceiver.setAudioPath(mPendingAudioPath);
                    }
                }, 50);
            }
            break;
        case TelephonyManager.CALL_STATE_OFFHOOK:
            if (mFmReceiver != null)
                mFmReceiver.setAudioPath(FmProxy.AUDIO_PATH_NONE);
            stateString = "Off Hook";
            break;
        case TelephonyManager.CALL_STATE_RINGING:
            if (mFmReceiver != null)
                mFmReceiver.setAudioPath(FmProxy.AUDIO_PATH_NONE);
            stateString = "Ringing";
            break;
        }
        Log.d(TAG, "Call State : " + stateString);
    }
}
