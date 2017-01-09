
package com.sprd.audioprofile;

import java.util.Date;

import android.R.integer;
import android.app.Dialog;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.ContentObserver;
import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Handler;
import android.os.Parcel;
import android.os.Parcelable;
import android.preference.VolumePreference;
import android.provider.Settings;
import android.provider.Settings.Global;
import android.provider.Settings.System;
import android.telephony.TelephonyManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import com.sprd.audioprofile.AudioProfileSoundSettings;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;

/**
 * Special preference type that allows configuration of both the ring volume and
 * notification volume.
 */
public class AudioProfileRingerVolumePreference extends VolumePreference implements
        View.OnKeyListener {
    private static final String TAG = "AudioProfileRingerVolumePreference";

    private AudioProfile mAudioProfile, mSelectProfile;
    private int mEditId = -1;
    private int mCurrentRingMode = -1;
    private Context mContext;
    private boolean mIsCurrentSelected = true;
    private AudioManager mAudioManager;
    protected boolean mSoundEnable = true;
    private VolumeReceiver mReceiver;
    private static int mTouchSeekBar = 0;
    // private CheckBox mNotificationsUseRingVolumeCheckbox;
    protected AudioProfileSeekBarVolumizer[] mSeekBarVolumizer;
    protected static final int[] SEEKBAR_ID = new int[] {
            R.id.ring_seekbar,
            R.id.notification_seekbar,
            R.id.media_volume_seekbar,
            R.id.alarm_volume_seekbar
    };
    protected static final int[] SEEKBAR_TYPE = new int[] {
            AudioManager.STREAM_RING,
            AudioManager.STREAM_NOTIFICATION,
            AudioManager.STREAM_MUSIC,
            AudioManager.STREAM_ALARM
    };

    public AudioProfileRingerVolumePreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        mAudioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        setDialogLayoutResource(R.layout.audio_profile_preference_dialog_ringervolume);
        setDialogIcon(R.drawable.ic_settings_sound);
        mSeekBarVolumizer = new AudioProfileSeekBarVolumizer[SEEKBAR_ID.length];
    }

    @Override
    protected void onBindDialogView(View view) {
        Log.d(TAG, "onBindDialogView start");
        mCurrentRingMode = mAudioManager.getRingerMode();
        mSelectProfile = AudioProfile.getSelectedProfile(mContext);
        if (mEditId != -1) {
            mAudioProfile = AudioProfile.restoreProfileWithId(mContext, mEditId);
            if (mAudioManager.getRingerMode() != AudioManager.RINGER_MODE_SILENT
                    && mAudioManager.getRingerMode() != AudioManager.RINGER_MODE_VIBRATE
                    && mAudioProfile.mIsSelected == AudioProfile.IS_SELECTED) {
                mAudioProfile.updateVolume(mContext);
            }
            if (mAudioProfile.mIsSelected == AudioProfile.NOT_SELECTED) {
                mIsCurrentSelected = false;
            }
        }
        Log.d(TAG, "mEditId = " + mEditId + " mIsCurrentSelected = " + mIsCurrentSelected);
        for (int i = 0; i < SEEKBAR_ID.length; i++) {
            SeekBar seekBar = (SeekBar) view.findViewById(SEEKBAR_ID[i]);
            mSeekBarVolumizer[i] = new AudioProfileSeekBarVolumizer(mContext, seekBar,SEEKBAR_TYPE[i]);
        }
        /* SPRD Modified for bug 542129, change volume is not allowed when device is in totally silent mode @{ */
        if (Global.getInt(mContext.getContentResolver(), "zen_mode", 0) == Global.ZEN_MODE_NO_INTERRUPTIONS){
            for (int i = 0; i < SEEKBAR_ID.length; i++) {
                mSeekBarVolumizer[i].getSeekBar().setEnabled(false);
            }
        }
        /* @} */
        mReceiver = new VolumeReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(AudioManager.VOLUME_CHANGED_ACTION);
        mContext.registerReceiver(mReceiver, filter);
        view.setOnKeyListener(this);
        view.setFocusableInTouchMode(true);
        view.requestFocus();
        Log.d(TAG, "onBindDialogView end");
    }

    public boolean onKey(View v, int keyCode, KeyEvent event) {
        // If key arrives immediately after the activity has been cleaned up.
        if (mSeekBarVolumizer == null)
            return true;
        if (mSeekBarVolumizer[0] == null)
            return true;
        boolean isdown = (event.getAction() == KeyEvent.ACTION_DOWN);
        switch (keyCode) {
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                if (isdown) {
                    if (mAudioManager.isMusicActive()) {
                        //SPRD:for Bug 452137
                        mSeekBarVolumizer[2].changeVolumeBy(-1);
                    } else {
                        mSeekBarVolumizer[mTouchSeekBar].changeVolumeBy(-1);
                    }
                }
                return true;
            case KeyEvent.KEYCODE_VOLUME_UP:
                if (isdown) {
                    if (mAudioManager.isMusicActive()) {
                      //SPRD:for Bug 452137
                        mSeekBarVolumizer[2].changeVolumeBy(1);
                    } else {
                        mSeekBarVolumizer[mTouchSeekBar].changeVolumeBy(1);
                    }
                }
                return true;
            default:
                return false;
        }
    }
    private class VolumeReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(AudioManager.VOLUME_CHANGED_ACTION)) {
                Log.d(TAG, "AudioManager.VOLUME_CHANGED_ACTION");
                int streamType = intent.getIntExtra(
                        AudioManager.EXTRA_VOLUME_STREAM_TYPE, -1);
              /* SPRD:for Bug 452137 @{ */
                /*if (streamType != AudioManager.STREAM_RING) {
                    return;
                }*/
             if (mAudioManager.isMusicActive()) {
                 if (streamType != AudioManager.STREAM_MUSIC) {
                     return;
                 }
                 if (mSeekBarVolumizer[2] != null) {
                     int volume = mAudioManager.getStreamVolume(streamType);
                     Log.d(TAG, "mSeekBarVolumizer[2].mLastProgress = "
                             + mSeekBarVolumizer[2].mLastProgress + " AudioManager Volume "
                             + volume);
                     if (volume != mSeekBarVolumizer[2].mLastProgress){
                         mSeekBarVolumizer[2].updateProgress(volume);
                     }
                 }
            }else {
                if (streamType != AudioManager.STREAM_RING) {
                    return;
                }
                if (mSeekBarVolumizer[0] != null) {
                    int volume = mAudioManager.getStreamVolume(streamType);
                    Log.d(TAG, "mSeekBarVolumizer[0].mLastProgress = "
                            + mSeekBarVolumizer[0].mLastProgress + " AudioManager Volume "
                            + volume);
                    if (volume != mSeekBarVolumizer[0].mLastProgress){
                        mSeekBarVolumizer[0].updateProgress(volume);
                    }
                }
            }
            /* @} */
            }
        }
    }

    public void setEditId(int id) {
        mEditId = id;
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        Log.d(TAG,"onDialogClosed");
        // SPRD: Modified for bug 544064, get current ring mode when close dialog
        mCurrentRingMode = mAudioManager.getRingerMode();
        if (!mIsCurrentSelected && mCurrentRingMode != AudioManager.RINGER_MODE_SILENT &&
                mCurrentRingMode != AudioManager.RINGER_MODE_VIBRATE) {
            Log.d(TAG,"!mIsCurrentSelected");
            for (AudioProfileSeekBarVolumizer vol : mSeekBarVolumizer) {
                if (vol != null) {
                    vol.revertSystemVolume();
                }
            }
        }
        if (!positiveResult) {
            Log.d(TAG,"!positiveResult");
            // pressed cancel,revert system and DB settings
            for (AudioProfileSeekBarVolumizer vol : mSeekBarVolumizer) {
                if (vol != null) {
                    vol.stopSample();
                    /* SPRD modified for bug 524316, abandonAudioFocus only when DialogClosed*/
                    mAudioManager.abandonAudioFocus(vol.mAudioFocusListener);
                    // SPRD Modified for bug 570335, cancel the changes when in priority zen mode
                    if (Global.getInt(getContext().getContentResolver(),
                            Global.ZEN_MODE, 0) == Global.ZEN_MODE_IMPORTANT_INTERRUPTIONS
                            ||mCurrentRingMode != AudioManager.RINGER_MODE_SILENT &&
                            mCurrentRingMode != AudioManager.RINGER_MODE_VIBRATE){
                        vol.revertSystemVolume();
                    }
                    if ((mCurrentRingMode == AudioManager.RINGER_MODE_SILENT ||
                            mCurrentRingMode == AudioManager.RINGER_MODE_VIBRATE) &&
                            vol.mStreamType == AudioManager.STREAM_ALARM) {
                        vol.revertSystemVolume();
                    }
                }
            }
        } else {
            Log.d(TAG,"positiveResult");
            // pressed ok, revert system settings if I'm not the current profile
            for (AudioProfileSeekBarVolumizer vol : mSeekBarVolumizer) {
                if (vol != null) {
                    vol.stopSample();
                    /* SPRD modified for bug 524316, abandonAudioFocus only when DialogClosed*/
                    mAudioManager.abandonAudioFocus(vol.mAudioFocusListener);
                    vol.setProfileVolume();
                }
            }
        }
        for (AudioProfileSeekBarVolumizer vol : mSeekBarVolumizer) {
            if (vol != null) {
                vol.destoryHandlerThread();
            }
        }
        mContext.unregisterReceiver(mReceiver);
    }

    public void stopAllVolume() {
        mSoundEnable = false;
        for (AudioProfileSeekBarVolumizer vol : mSeekBarVolumizer) {
            if (vol != null)
                vol.stopSample();
        }
    }

    protected void stopOtherVolume(AudioProfileSeekBarVolumizer volumizer) {
        for (AudioProfileSeekBarVolumizer vol : mSeekBarVolumizer) {
            if (vol != null && vol != volumizer)
                vol.stopSample();
        }
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        final Parcelable superState = super.onSaveInstanceState();
        if (isPersistent()) {
            return superState;
        }
        final SavedState myState = new SavedState(superState);
        VolumeStore[] volumeStore = myState.getVolumeStore(SEEKBAR_ID.length);
        for (int i = 0; i < SEEKBAR_ID.length; i++) {
            AudioProfileSeekBarVolumizer vol = mSeekBarVolumizer[i];
            if (vol != null) {
                vol.onSaveInstanceState(volumeStore[i]);
            }
        }
        return myState;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        if (state == null || !state.getClass().equals(SavedState.class)) {
            super.onRestoreInstanceState(state);
            return;
        }
        SavedState myState = (SavedState) state;
        super.onRestoreInstanceState(myState.getSuperState());
        VolumeStore[] volumeStore = myState.getVolumeStore(SEEKBAR_ID.length);
        for (int i = 0; i < SEEKBAR_ID.length; i++) {
            AudioProfileSeekBarVolumizer vol = mSeekBarVolumizer[i];
            if (vol != null) {
                vol.onRestoreInstanceState(volumeStore[i]);
            }
        }
    }

    public static class VolumeStore {
        public int volume = -1;
        public int originalVolume = -1;
        public int profileOriginalVolume = -1;
    }
    private static class SavedState extends BaseSavedState {
        VolumeStore[] mVolumeStore;

        public SavedState(Parcel source) {
            super(source);
            mVolumeStore = new VolumeStore[SEEKBAR_ID.length];
            for (int i = 0; i < SEEKBAR_ID.length; i++) {
                mVolumeStore[i] = new VolumeStore();
                mVolumeStore[i].volume = source.readInt();
                mVolumeStore[i].originalVolume = source.readInt();
                mVolumeStore[i].profileOriginalVolume = source.readInt();
            }
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            super.writeToParcel(dest, flags);
            for (int i = 0; i < SEEKBAR_ID.length; i++) {
                dest.writeInt(mVolumeStore[i].volume);
                dest.writeInt(mVolumeStore[i].originalVolume);
                dest.writeInt(mVolumeStore[i].profileOriginalVolume);
            }
        }

        VolumeStore[] getVolumeStore(int count) {
            if (mVolumeStore == null || mVolumeStore.length != count) {
                mVolumeStore = new VolumeStore[count];
                for (int i = 0; i < count; i++) {
                    mVolumeStore[i] = new VolumeStore();
                }
            }
            return mVolumeStore;
        }

        public SavedState(Parcelable superState) {
            super(superState);
        }

        public static final Parcelable.Creator<SavedState> CREATOR =
                new Parcelable.Creator<SavedState>() {
                    public SavedState createFromParcel(Parcel in) {
                        return new SavedState(in);
                    }

                    public SavedState[] newArray(int size) {
                        return new SavedState[size];
                    }
                };
    }

    /**
     * Turns a {@link SeekBar} into a volume control.
     */
    public class AudioProfileSeekBarVolumizer implements OnSeekBarChangeListener, Runnable {

        private Handler mHandler = new Handler();

        private int mStreamType;
        public int mOriginalSystemVolume;
        private int mOriginalProfileVolume;
        private Ringtone mRingtone;

        private int mLastProgress = -1;
        private SeekBar mSeekBar;
        private boolean mHasAudioFocus = false;
        private boolean mStoppedByLossOfFocus = false;
        private HandlerThread mThread;
        private Handler mRingtoneHandler;
        private Uri mDefaultUri = null;
        // SPRD: added for bug 591802
        private static final int SAFE_MUSIC_VOLUME = 10;

        private Runnable mRingtonePlayRunnable = new Runnable() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                Log.d(TAG, "mRingtonePlayRunnable");
                //SPRD 578882: The notification is ring in vibrate mode
                if (mStreamType != AudioManager.STREAM_RING && mStreamType != AudioManager.STREAM_NOTIFICATION) {
                    if (mRingtone != null && !mRingtone.isPlaying() && mHasAudioFocus && mSoundEnable) {
                        mRingtone.play();
                    }
                } else {
                    if (mCurrentRingMode != AudioManager.RINGER_MODE_SILENT
                            && mCurrentRingMode != AudioManager.RINGER_MODE_VIBRATE) {
                        if (mRingtone != null && !mRingtone.isPlaying()
                                && mHasAudioFocus && mSoundEnable) {
                            mRingtone.play();
                        }
                    }
                }

            }
        };
        private Runnable mRingtoneStopRunnable = new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "mRingtoneStopRunnable");
                // TODO Auto-generated method stub
                if (mRingtone != null && mRingtone.isPlaying()) {
                    mRingtone.stop();
                }
            }
        };
        private Runnable mRingtoneSettings = new Runnable() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                if(mDefaultUri != null && !mDefaultUri.toString().isEmpty()){
                    Log.d(TAG,"defaultUri = "+mDefaultUri.toString());
                    mRingtone = RingtoneManager.getRingtone(mContext, mDefaultUri);
                }
                if (mRingtone != null) {
                    mRingtone.setStreamType(mStreamType);
                    mRingtone.setLooping(true);
                }
            }
        };

        private ContentObserver mVolumeObserver = new ContentObserver(mHandler) {
            @Override
            public void onChange(boolean selfChange) {
                super.onChange(selfChange);
                if (mSeekBar != null) {
                    int volume = System.getInt(mContext.getContentResolver(),
                            System.VOLUME_SETTINGS[mStreamType], -1);
                    // Works around an atomicity problem with volume updates
                    // TODO: Fix the actual issue, probably in AudioService
                    if (volume >= 0) {
                        mSeekBar.setProgress(volume);
                    }
                }
            }
        };

        public AudioProfileSeekBarVolumizer(Context context, SeekBar seekBar, int streamType) {
            mStreamType = streamType;
            mSeekBar = seekBar;
            initSeekBar(seekBar);
        }

        private Uri getMediaVolumeUri(Context context) {
            return Uri.parse(ContentResolver.SCHEME_ANDROID_RESOURCE + "://"
                    + context.getPackageName()
                    + "/" + R.raw.media_volume);
        }

        private void initSeekBar(SeekBar seekBar) {
            seekBar.setMax(mAudioManager.getStreamMaxVolume(mStreamType));
            if(mStreamType == AudioManager.STREAM_RING){
            	// relate to feature 475775, delete first
                //seekBar.setMin(1);
            }
            /* SPRD: Modified for bug 591802, set Music stream max volume when headset on @{ */
            if(mStreamType == AudioManager.STREAM_MUSIC && mAudioManager.isWiredHeadsetOn()
                    && mAudioManager.getSafeMediaVolumeEnabled()) {
                seekBar.setMax(SAFE_MUSIC_VOLUME);
            }
            /* @} */
            mOriginalSystemVolume = mAudioManager.getStreamVolume(mStreamType);
            mOriginalProfileVolume = mAudioProfile.getStreamVolume(mStreamType);
            if (mIsCurrentSelected) {
                mLastProgress = mOriginalSystemVolume;
            } else {
                mLastProgress = mOriginalProfileVolume;
            }
            seekBar.setProgress(mLastProgress);
            seekBar.setOnSeekBarChangeListener(this);

            mContext.getContentResolver().registerContentObserver(
                    System.getUriFor(System.VOLUME_SETTINGS[mStreamType]), false, mVolumeObserver);

            if (mStreamType == AudioManager.STREAM_RING) {
                int phoneCount = TelephonyManager.getDefault().getPhoneCount();
                if (phoneCount > 1) {
                    int i = 0;
                    for (i = 0; i < phoneCount; i++) {
                        if (TelephonyManager.getDefault().hasIccCard(i)
                                && mAudioProfile.mRingtoneUri[i] != null) {
                            mDefaultUri = Uri.parse(mAudioProfile.mRingtoneUri[i]);
                            break;
                        }
                    }
                    if (mDefaultUri == null) {
                        mDefaultUri = Settings.System.DEFAULT_RINGTONE_URI;
                    }
                } else {
                    if (mAudioProfile.mRingtoneUri[0] != null) {
                        mDefaultUri = Uri.parse(mAudioProfile.mRingtoneUri[0]);
                    }
                }
            } else if (mStreamType == AudioManager.STREAM_ALARM) {
                mDefaultUri = Settings.System.DEFAULT_ALARM_ALERT_URI;
            /* SPRD: 493526 change uri for notification @{ */
            } else if (mStreamType == AudioManager.STREAM_NOTIFICATION) {
                if (mAudioProfile.mNotificationUri != null) {
                    mDefaultUri = Uri.parse(mAudioProfile.mNotificationUri);
                }
                if (mDefaultUri == null) {
                    mDefaultUri = getMediaVolumeUri(getContext());
                }
            /* @} */
            } else {
                mDefaultUri = getMediaVolumeUri(getContext());
            }

            mThread = new HandlerThread("AudioProfileSeekBarVolumizer");
            mThread.start();
            mRingtoneHandler = new Handler(mThread.getLooper());
            mRingtoneHandler.post(mRingtoneSettings);
        }


        private void destoryHandlerThread(){
            if(mThread != null){
                Log.d("TAG", "HandlerThread quitSafely");
                mThread.quitSafely();
            }
        }

        public void stop() {
            stopSample();
            mContext.getContentResolver().unregisterContentObserver(mVolumeObserver);
            mSeekBar.setOnSeekBarChangeListener(null);
        }

        public void revertProfileVolume() {
            mAudioProfile.setStreamVolume(mContext, mStreamType, mOriginalProfileVolume);
        }

        public void revertSystemVolume() {
            // update db
            mAudioManager.setStreamVolume(mStreamType, mOriginalSystemVolume, 0);
            if (mAudioProfile.mNotificationsUseRingVolume == AudioProfile.NOTIFICATIONS_USE_RING_VOLUME
                    && mStreamType == AudioManager.STREAM_RING) {
                mAudioManager.setStreamVolume(AudioManager.STREAM_NOTIFICATION, mOriginalSystemVolume, 0);
            }
        }

        public void setProfileVolume() {
            int saveVolume = mLastProgress;
            if (mStreamType == AudioManager.STREAM_RING && mLastProgress == 0) {
                saveVolume = 1;
            }
            mAudioProfile.setStreamVolume(mContext, mStreamType, saveVolume);
            if (mAudioProfile.mNotificationsUseRingVolume == AudioProfile.NOTIFICATIONS_USE_RING_VOLUME
                    && mStreamType == AudioManager.STREAM_RING) {
                mAudioProfile.setStreamVolume(mContext, AudioManager.STREAM_NOTIFICATION,
                        saveVolume);
            }
        }

        public void setSystemVolume() {
            mAudioManager.setStreamVolume(mStreamType, mLastProgress, 0);
            if (mAudioProfile.mNotificationsUseRingVolume == AudioProfile.NOTIFICATIONS_USE_RING_VOLUME
                    && mStreamType == AudioManager.STREAM_RING) {
                mAudioManager.setStreamVolume(AudioManager.STREAM_NOTIFICATION, mLastProgress, 0);
            }
        }

        public void onProgressChanged(SeekBar seekBar, int progress,
                boolean fromTouch) {
            if (!fromTouch) {
                return;
            }
            mLastProgress = progress;
            //SPRD 578882: The notification is ring in vibrate mode

            /* SPRD add for bug 629999 @{ */
            if (mStreamType == AudioManager.STREAM_RING) {
                mTouchSeekBar = 0;
            } else if (mStreamType == AudioManager.STREAM_NOTIFICATION) {
                mTouchSeekBar = 1;
            } else if (mStreamType == AudioManager.STREAM_MUSIC) {
                mTouchSeekBar = 2;
            } else {
                mTouchSeekBar = 3;
            }
            /* @} */

            if (mStreamType != AudioManager.STREAM_RING && mStreamType != AudioManager.STREAM_NOTIFICATION) {
                setSystemVolume();
                if (mStreamType == AudioManager.STREAM_MUSIC && mAudioManager.isMusicActive()) {
                    return;
                }
                if (progress > 0) {
                    mRingtoneHandler.post(mRingtonePlayRunnable);
                } else {
                    mRingtoneHandler.post(mRingtoneStopRunnable);
                }

            } else {
                //SPRD: Modified for bug 561430, set the ring volume mutable under IMPORTANT_INTERRUPTIONS zen mode
                //SPRD 578882: The notification is ring in vibrate mode
                /* SPRD: 596282 set Notification Volume mutable under IMPORTANT_INTERRUPTIONS zen mode @{ */
                if (mCurrentRingMode == AudioManager.RINGER_MODE_VIBRATE) {
                    return;
                }
                if (Global.getInt(getContext().getContentResolver(),
                        Global.ZEN_MODE, 0) == Global.ZEN_MODE_IMPORTANT_INTERRUPTIONS)
                {
                    if (mSelectProfile != null && (mSelectProfile.mId == AudioProfile.DEFAULT_SILENT_PROFILE_ID
                            || mSelectProfile.mId == AudioProfile.DEFAULT_VIBRATE_PROFILE_ID
                            || mSelectProfile.mId == AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID))
                        return;
                }
                /* @} */
                if (Global.getInt(getContext().getContentResolver(),
                        Global.ZEN_MODE, 0) == Global.ZEN_MODE_IMPORTANT_INTERRUPTIONS
                        || mCurrentRingMode != AudioManager.RINGER_MODE_SILENT
                        && mCurrentRingMode != AudioManager.RINGER_MODE_VIBRATE) {

                    setSystemVolume();
                    if (progress > 0) {
                        mRingtoneHandler.post(mRingtonePlayRunnable);
                    } else {
                        mRingtoneHandler.post(mRingtoneStopRunnable);
                    }
                }
            }

        }

        void postSetVolume(int progress) {
            // Do the volume changing separately to give responsive UI
            mLastProgress = progress;
            mHandler.removeCallbacks(this);
            mHandler.post(this);
        }

        public void onStartTrackingTouch(SeekBar seekBar) {
            if (mRingtone != null && !mRingtone.isPlaying()) {
                if (mStreamType == AudioManager.STREAM_MUSIC && mAudioManager.isMusicActive()) {
                    return;
                }
                /* SPRD 491048 Automatically exit the editor interface @{ */
                //SPRD 578882: The notification is ring in vibrate mode
                if (mStreamType != AudioManager.STREAM_RING && mStreamType != AudioManager.STREAM_NOTIFICATION
                        || (mCurrentRingMode != AudioManager.RINGER_MODE_SILENT
                        && mCurrentRingMode != AudioManager.RINGER_MODE_VIBRATE)) {
                    setSystemVolume();
                }
                /* @} */
                sample();
            }
        }

        public void onStopTrackingTouch(SeekBar seekBar) {

        }

        public void run() {
            // update db
            setProfileVolume();
        }

        private void sample() {
            stopOtherVolume(this);
            /* SPRD: Modified for bug 544553, don't sample notification sound in silent mode */
            //SPRD 578882: The notification is ring in vibrate mode
            boolean sampleNotification = !(mStreamType == AudioManager.STREAM_NOTIFICATION
                    && (mCurrentRingMode == AudioManager.RINGER_MODE_SILENT
                    || mCurrentRingMode == AudioManager.RINGER_MODE_VIBRATE));
            /* SPRD:According to the latest requirement document,changes the Audiofocus request mode @{ */
            // SPRD: Modified for bug 530944,when FM is running in background, cancel the sample rintone.
            if (mAudioManager != null && !mAudioManager.isMusicActive()
                    && sampleNotification) {
            /* @} */
                if (!mHasAudioFocus && mAudioManager.requestAudioFocus(mAudioFocusListener, mStreamType,
                                AudioManager.AUDIOFOCUS_GAIN_TRANSIENT) != AudioManager.AUDIOFOCUS_REQUEST_FAILED) {
                    mAudioManager.requestAudioFocus(mAudioFocusListener, mStreamType,
                            AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
                    mHasAudioFocus = true;
                }
                mRingtoneHandler.post(mRingtonePlayRunnable);
            }
            /* @} */
        }

        public void stopSample() {
            mRingtoneHandler.post(mRingtoneStopRunnable);
            if ((mStoppedByLossOfFocus || mHasAudioFocus) && mAudioManager != null) {
                /* SPRD modified for bug 524316, abandonAudioFocus only when DialogClosed*/
                //mAudioManager.abandonAudioFocus(mAudioFocusListener);
                mHasAudioFocus = false;
                mStoppedByLossOfFocus = false;
            }

        }

        private OnAudioFocusChangeListener mAudioFocusListener = new OnAudioFocusChangeListener() {

            @Override
            public void onAudioFocusChange(int focusChange) {
                if (mRingtone == null) {
                    mAudioManager.abandonAudioFocus(this);

                    return;
                }
                Log.d(TAG, "focusChange = " + focusChange);
                switch (focusChange) {
                    case AudioManager.AUDIOFOCUS_LOSS:
                    case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                        mRingtoneHandler.post(mRingtoneStopRunnable);
                        mHasAudioFocus = false;
                        mStoppedByLossOfFocus = true;
                        break;
                    case AudioManager.AUDIOFOCUS_GAIN:
                    case AudioManager.AUDIOFOCUS_GAIN_TRANSIENT:
                        mHasAudioFocus = true;
                        if (mStoppedByLossOfFocus) {
                            mStoppedByLossOfFocus = false;
                            mRingtoneHandler.post(mRingtonePlayRunnable);
                        }
                        break;

                }
            }
        };

        public SeekBar getSeekBar() {
            return mSeekBar;
        }

        public void changeVolumeBy(int amount) {
            /* SPRD:for Bug 452137 @{ */
            if (mAudioManager.isMusicActive()) {
                Log.d(TAG, "music active");
                mAudioManager.adjustStreamVolume(AudioManager.STREAM_MUSIC, amount,
                        AudioManager.FLAG_VIBRATE | AudioManager.FLAG_SHOW_UI);
                return;
            } 
            /*@}*/
            if (mIsCurrentSelected) {
                mAudioManager.adjustStreamVolume(mStreamType,
                        amount, AudioManager.FLAG_VIBRATE | AudioManager.FLAG_SHOW_UI);
                int progress = 0;
                if ((mLastProgress + amount) <= mAudioManager.getStreamMaxVolume(mStreamType)
                        && (mLastProgress + amount) >= 0) {
                    progress = mLastProgress + amount;
                } else {
                    progress = mLastProgress;
                }
                if (progress != 0) {
                    mSeekBar.setProgress(progress);
                    if (mRingtone != null && !mRingtone.isPlaying()) {
                        sample();
                    }
                    postSetVolume(progress);
                }

            } else {
                mAudioManager.adjustStreamVolume(mStreamType,
                        amount, AudioManager.FLAG_VIBRATE | AudioManager.FLAG_SHOW_UI);
                /* SPRD: bug497313, can not change outdoor volume in any mode @{ */
                if (mSelectProfile != null && mSelectProfile.mId != AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID) {
                    mSelectProfile.updateRingerVolume(mContext);
                }
                /* @} */
                mOriginalSystemVolume = mAudioManager.getStreamVolume(mStreamType);
            }
        }
        public void updateProgress(int amount) {
            if (mIsCurrentSelected) {
                int progress = mLastProgress;
                if (amount <= mAudioManager.getStreamMaxVolume(mStreamType)
                        && amount > 0) {
                    progress = amount;
                }

                /* SPRD: 497428 check if music is active and set progress @{ */
                boolean isMusicActive = mAudioManager.isMusicActive();
                if (isMusicActive && amount == 0) {
                    progress = amount;
                }

                if ((isMusicActive && progress == 0) || progress > 0) {
                    /* @} */
                    mSeekBar.setProgress(progress);
                    if (mRingtone != null && !mRingtone.isPlaying()) {
                        sample();
                    }
                    postSetVolume(progress);
                }

            } else {
                if (mSelectProfile != null) {
                    mSelectProfile.updateRingerVolume(mContext);
                }
                mOriginalSystemVolume = mAudioManager.getStreamVolume(mStreamType);
            }
        }

        public void onSaveInstanceState(VolumeStore volumeStore) {
            if (mLastProgress >= 0) {
                volumeStore.volume = mLastProgress;
                volumeStore.originalVolume = mOriginalSystemVolume;
                volumeStore.profileOriginalVolume = mOriginalProfileVolume;
            }
        }

        public void onRestoreInstanceState(VolumeStore volumeStore) {
            if (volumeStore.volume != -1) {
                mOriginalSystemVolume = volumeStore.originalVolume;
                mLastProgress = volumeStore.volume;
                mOriginalProfileVolume = volumeStore.profileOriginalVolume;
                postSetVolume(mLastProgress);
            }
        }
    }
}
