package com.sprd.audioprofile;

import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.media.AudioManager;
import android.media.AudioSystem;
import android.net.Uri;
import android.provider.Settings;
import android.provider.SettingsEx;
import android.telephony.TelephonyManager;
import android.content.ContentResolver;
import android.util.Log;
import com.sprd.audioprofile.AudioProfileSettings;
import com.sprd.audioprofile.AudioProfileProvider;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.RingtoneManager;
import android.media.RingtoneManagerEx;

public class AudioProfile {

    public static final String PROFILE_TABLE = "profiles";
    public static final String AUTHORITY = "com.sprd.audioprofile.provider";
    public static final Uri CONTENT_URI = Uri.parse("content://" + AUTHORITY + "/" + PROFILE_TABLE);

    public static final String GENERAL_NAME = "General";
    public static final String SILENT_NAME = "Silent";
    public static final String VIBRATION_NAME = "Vibration";
    public static final String OUTDOOR_NAME = "Outdoor";

    public static final String DEFAULT_DISPLAY_NAME = "Untitled Profile";

    public static final int IS_SELECTED = 1;
    public static final int NOT_SELECTED = 0;
    public static final int IS_VIBRATE = 1;
    public static final int NOT_VIBRATE = 0;
    public static final int IS_VIBRATE_MSG = 1;
    public static final int NOT_VIBRATE_MSG = 0;
    public static final int IS_SILENT = 1;
    public static final int NOT_SILENT = 0;
    public static final int DEFAULT_RINGER_MODE = AudioManager.RINGER_MODE_NORMAL;
    public static final int DEFAULT_STREAM_RING_VOLUME = AudioSystem.DEFAULT_STREAM_VOLUME[AudioManager.STREAM_RING];
    public static final int DEFAULT_STREAM_NOTIFICATION_VOLUME = AudioSystem.DEFAULT_STREAM_VOLUME[AudioManager.STREAM_NOTIFICATION];
    public static final int DEFAULT_STREAM_ALARM_VOLUME = AudioSystem.DEFAULT_STREAM_VOLUME[AudioManager.STREAM_ALARM];
    public static final int DEFAULT_STREAM_MEDIA_VOLUME = AudioSystem.DEFAULT_STREAM_VOLUME[AudioManager.STREAM_MUSIC];
    public static final int IS_DTMF_TONE = 1;
    public static final int NOT_DTMF_TONE = 0;
    public static final int IS_SOUND_EFFECTS = 1;
    public static final int NOT_SOUND_EFFECTS = 0;
    public static final int IS_LOCK_SOUNDS = 1;
    public static final int NOT_LOCK_SOUNDS = 0;
    public static final int IS_HAPTIC_FEEDBACK = 1;
    public static final int NOT_HAPTIC_FEEDBACK = 0;
    public static final int IS_CHARGING_SOUNDS = 1;
    public static final int NOT_CHARGING_SOUNDS = 0;

    public static final int NOTIFICATIONS_USE_RING_VOLUME = 1;
    public static final int NOTIFICATIONS_NOT_USE_RING_VOLUME = 0;

    public static final int DEFAULT_GENERAL_PROFILE_ID = 1;
    public static final int DEFAULT_SILENT_PROFILE_ID = 2;
    public static final int DEFAULT_VIBRATE_PROFILE_ID = 3;
    public static final int DEFAULT_OUTDOOR_PROFILE_ID = 4;
    public static final int PREDEFINED_PROFILE_COUNT = 4;

    public int mId;
    public String mDisplayName;
    public int mIsSelected;
    public int mIsVibrate;
    public int mIsVibrateMsg;
    public String mNotificationUri;
    public int mRingtoneVolumeIndex;
    public int mAlarmVolumeIndex;
    public int mNotificationVolumeIndex;
    public int mMediaVolumeIndex;

    public int mDTMFTone;
    public int mSoundEffects;
    public int mChargingSounds;
    public int mLockSounds;
    public int mHapticFeedback;

    public int mNotificationsUseRingVolume;
    public String [] mRingtoneUri;
    public String [] mMessagetoneUri;
    private static int mPhoneCount = 1;

    public static AudioProfile restoreProfileWithId(Context context, int id) {
        Uri uri = ContentUris.withAppendedId(CONTENT_URI, id);
        Cursor c = context.getContentResolver().query(uri, null, null, null, null);
        try {
            if (c != null && c.moveToFirst()) {
                return restore(c);
            } else {
                return null;
            }
        } finally {
            if (c != null) {
                c.close();
            }
        }
    }

    public static Uri getDefaultUri(Context context, int type) {

        String ringerUriString = Settings.System.getString(context.getContentResolver(),
                SettingsEx.SystemEx.DEFAULT_RINGTONE);
        Uri defaultRingtoneUri = (ringerUriString != null ? Uri.parse(ringerUriString) : null);
        String notificationUriString = Settings.System.getString(context.getContentResolver(),
                SettingsEx.SystemEx.DEFAULT_NOTIFICATION);
        Uri defaultNotificationUri = (notificationUriString != null ? Uri
                .parse(notificationUriString) : null);
        if ((type & RingtoneManager.TYPE_RINGTONE) != 0) {
            return defaultRingtoneUri;
        } else if ((type & RingtoneManager.TYPE_NOTIFICATION) != 0) {
            return defaultNotificationUri;
        } else {
            return null;
        }
    }

    public static AudioProfile restoreProfileWithName(Context context, String name) {
        Cursor c = context.getContentResolver().query(CONTENT_URI, new String[] {
            AudioProfileColumns.NAME
        }, AudioProfileColumns.NAME + "=?", new String[] { name }, null);
        try {
            if (c != null && c.moveToFirst()) {
                return restore(c);
            } else {
                return null;
            }
        } finally {
            if (c != null) {
                c.close();
            }
        }
    }

    public static AudioProfile restore(Cursor cursor) {
        mPhoneCount = TelephonyManager.getDefault().getPhoneCount();

        AudioProfile p = new AudioProfile();
        p.mRingtoneUri = new String[mPhoneCount];

        p.mMessagetoneUri = new String[mPhoneCount];

        p.mId = cursor.getInt(cursor.getColumnIndex(AudioProfileColumns.ID));

        p.mDisplayName = cursor.getString(cursor
                .getColumnIndex(AudioProfileColumns.DISPLAY_NAME));
        p.mIsSelected = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.IS_SELECTED));
        p.mIsVibrate = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.IS_VIBRATE));
        p.mIsVibrateMsg = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.IS_VIBRATE_MSG));
        p.mNotificationUri = cursor.getString(cursor
                .getColumnIndex(AudioProfileColumns.NOTIFICATION_URI));
        p.mRingtoneVolumeIndex = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.RING_VOLUME_INDEX));
        p.mAlarmVolumeIndex = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.ALARM_VOLUME_INDEX));
        p.mNotificationVolumeIndex = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.NOTIFICATION_VOLUME_INDEX));
        p.mMediaVolumeIndex = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.MEDIA_VOLUME_INDEX));
        p.mDTMFTone = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.DTMF_TONE));
        p.mChargingSounds = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.CHARGING_SOUNDS));
        p.mSoundEffects = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.SOUND_EFFECTS));
        p.mLockSounds = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.LOCK_SOUNDS));
        p.mHapticFeedback = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.HAPTIC_FEEDBACK));
        p.mNotificationsUseRingVolume = cursor.getInt(cursor
                .getColumnIndex(AudioProfileColumns.NOTIFICATIONS_USE_RING_VOLUME));
        for (int i = 0; i < mPhoneCount; i++) {
            if (AudioProfileProvider.ringtoneStr[i] != null) {
            } else {
                AudioProfileProvider.ringtoneStr = new String[mPhoneCount];
                AudioProfileProvider.ringtoneStr[i] = AudioProfileColumns.RING_URI + i;
            }
            p.mRingtoneUri[i] = cursor.getString(cursor
                    .getColumnIndex(AudioProfileProvider.ringtoneStr[i]));
            if (AudioProfileProvider.mMessagetoneStr[i] == null) {
                AudioProfileProvider.mMessagetoneStr = new String[mPhoneCount];
                AudioProfileProvider.mMessagetoneStr[i] = AudioProfileColumns.MSG_URI + i;
            }
            p.mMessagetoneUri[i] = cursor.getString(cursor.getColumnIndex(AudioProfileProvider.mMessagetoneStr[i]));
        }
        return p;
    }

    public ContentValues toContentValues() {
        ContentValues values = new ContentValues();
        values.put(AudioProfileColumns.RING_VOLUME_INDEX, mRingtoneVolumeIndex);
        values.put(AudioProfileColumns.ALARM_VOLUME_INDEX, mAlarmVolumeIndex);
        values.put(AudioProfileColumns.NOTIFICATION_VOLUME_INDEX, mNotificationVolumeIndex);
        values.put(AudioProfileColumns.MEDIA_VOLUME_INDEX, mMediaVolumeIndex);
        return values;
    }

    public void update(Context context, ContentValues values) {
        context.getContentResolver()
                .update(ContentUris.withAppendedId(CONTENT_URI, mId), values, null, null);
    }

    public void updateRingtone(Context context) {
        ContentValues values = new ContentValues();
        /* SPRD modified for bug 530430,
         * update two SIM ringtones when two SIM cards are inserted @{
         */
        int mPhoneCount = TelephonyManager.getDefault().getPhoneCount();
        for(int i = 0; i < mPhoneCount; i++) {
            String stringUri = AudioProfileColumns.RING_URI + i;
            /* SPRD: Modified for bug 510837 @{ */
            String stringUriValue = "";
            if (RingtoneManagerEx.getActualDefaultRingtoneUri(context, RingtoneManager.TYPE_RINGTONE, i) != null) {
                stringUriValue = RingtoneManagerEx.
                        getActualDefaultRingtoneUri(context, RingtoneManager.TYPE_RINGTONE, i).toString();
            }
            values.put(stringUri,stringUriValue);
            /* @} */
            update(context, values);
        }
        /* @} */
    }

    public void updateVolume(Context context) {
        AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        ContentValues values = new ContentValues();
        values.put(AudioProfileColumns.RING_VOLUME_INDEX,
                audioManager.getStreamVolume(AudioManager.STREAM_RING));
        values.put(AudioProfileColumns.NOTIFICATION_VOLUME_INDEX,
                audioManager.getStreamVolume(AudioManager.STREAM_NOTIFICATION));
        values.put(AudioProfileColumns.ALARM_VOLUME_INDEX,
                audioManager.getStreamVolume(AudioManager.STREAM_ALARM));
        values.put(AudioProfileColumns.MEDIA_VOLUME_INDEX,
                audioManager.getStreamVolume(AudioManager.STREAM_MUSIC));
        update(context, values);
    }

    public void updateRingerVolume(Context context) {
        AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        int ringIndex = audioManager.getStreamVolume(AudioManager.STREAM_RING);
        int notificationIndex = audioManager.getStreamVolume(AudioManager.STREAM_NOTIFICATION);
        if (ringIndex == 0) {
            ringIndex = 1;
        }
        if (notificationIndex == 0) {
            notificationIndex = 1;
        }
        ContentValues values = new ContentValues();
        values.put(AudioProfileColumns.RING_VOLUME_INDEX, ringIndex);
        values.put(AudioProfileColumns.NOTIFICATION_VOLUME_INDEX, notificationIndex);
        update(context, values);
    }

    public void updateDisplayName(Context context, String displayName) {
        // udpate db
        ContentValues values = new ContentValues();
        values.put(AudioProfileColumns.DISPLAY_NAME, displayName);
        update(context, values);
        mDisplayName = displayName;
        Intent intent = new Intent(AudioProfileSettings.INTENT_ACTION_MODE_NAME_CHANGED);
        intent.putExtra(AudioProfileSettings.EDIT_ID_KEY, mId);
        intent.putExtra(AudioProfileSettings.EDITED_NAME_KEY, displayName);
        context.sendBroadcast(intent);
    }

    public static void updateVolumeOfNormal(Context context, int id) {
        AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        ContentResolver resolver = context.getContentResolver();
        ContentValues values = new ContentValues();
        int mode = Settings.System.getInt(resolver, "currentAudioProfileId", -1);

        if (AudioProfile.DEFAULT_OUTDOOR_PROFILE_ID == mode) {
            /* SPRD: Modified for bug 580388, Ringer volume needn't update after change mode @{ */
            /*
            values.put(AudioProfileColumns.RING_VOLUME_INDEX,
                    audioManager.getStreamMaxVolume(AudioManager.STREAM_RING));
            values.put(AudioProfileColumns.NOTIFICATION_VOLUME_INDEX,
                    audioManager.getStreamMaxVolume(AudioManager.STREAM_NOTIFICATION));
            //SPRD: Modified for bug 573433, Ringer volume is not correct after change mode
            resolver.update(ContentUris.withAppendedId(CONTENT_URI, id), values, null, null);
            */
            /* @} */
            /* SPRD: Modified for bug 577743, Ringer volume is not update after change mode @{ */
        } else if (AudioProfile.DEFAULT_VIBRATE_PROFILE_ID != mode) {
            values.put(AudioProfileColumns.RING_VOLUME_INDEX,
                    audioManager.getStreamVolume(AudioManager.STREAM_RING));
            values.put(AudioProfileColumns.NOTIFICATION_VOLUME_INDEX,
                    audioManager.getStreamVolume(AudioManager.STREAM_NOTIFICATION));
            values.put(AudioProfileColumns.ALARM_VOLUME_INDEX,
                    audioManager.getStreamVolume(AudioManager.STREAM_ALARM));
            values.put(AudioProfileColumns.MEDIA_VOLUME_INDEX,
                    audioManager.getStreamVolume(AudioManager.STREAM_MUSIC));
            resolver.update(ContentUris.withAppendedId(CONTENT_URI, id), values, null, null);
            /* @} */
        }
        /* SPRD: Modified for bug 573433, Ringer volume is not correct after change mode @{ */
        /*else {
            values.put(AudioProfileColumns.RING_VOLUME_INDEX,
                    audioManager.getStreamVolume(AudioManager.STREAM_RING));
            values.put(AudioProfileColumns.NOTIFICATION_VOLUME_INDEX,
                    audioManager.getStreamVolume(AudioManager.STREAM_NOTIFICATION));
            values.put(AudioProfileColumns.ALARM_VOLUME_INDEX,
                    audioManager.getStreamVolume(AudioManager.STREAM_ALARM));
            values.put(AudioProfileColumns.MEDIA_VOLUME_INDEX,
                    audioManager.getStreamVolume(AudioManager.STREAM_MUSIC));
        }*/
        /* @} */
    }

    public void setSelected(Context context) {
        ContentValues values = new ContentValues();
        // clear all other selected state
        values.put(AudioProfileColumns.IS_SELECTED, 0);
        context.getContentResolver().update(CONTENT_URI, values,
                AudioProfileColumns.IS_SELECTED + "=?",
                new String[] {String.valueOf(IS_SELECTED)});

        // set current profile to be selected
        values = new ContentValues();
        values.put(AudioProfileColumns.IS_SELECTED, 1);
        update(context, values);
    }

    public static AudioProfile getSelectedProfile(Context context) {
        Cursor cursor = context.getContentResolver().query(CONTENT_URI, new String[] {
                AudioProfileColumns.ID, AudioProfileColumns.IS_SELECTED
        }, AudioProfileColumns.IS_SELECTED + "=?", new String[] {
                String.valueOf(AudioProfile.IS_SELECTED)
        }, null);
        int id = -1;
        try {
            if (cursor.moveToFirst()) {
                id = cursor.getInt(cursor.getColumnIndex(AudioProfileColumns.ID));
            }
        } finally {
            cursor.close();
        }
        return restoreProfileWithId(context, id);
    }

    public int getStreamVolume(int streamType) {
        if (streamType == AudioManager.STREAM_RING) {
            return mRingtoneVolumeIndex;
        } else if (streamType == AudioManager.STREAM_NOTIFICATION) {
            return mNotificationVolumeIndex;
        } else if (streamType == AudioManager.STREAM_ALARM) {
            return mAlarmVolumeIndex;
        } else if (streamType == AudioManager.STREAM_MUSIC) {
            return mMediaVolumeIndex;
        } else {
            return -1;
        }
    }

    public void setStreamVolume(Context context, int streamType, int index) {
        if (streamType == AudioManager.STREAM_RING) {
            mRingtoneVolumeIndex = index;
        } else if (streamType == AudioManager.STREAM_NOTIFICATION) {
            mNotificationVolumeIndex = index;
        } else if (streamType == AudioManager.STREAM_ALARM) {
            mAlarmVolumeIndex = index;
        } else if (streamType == AudioManager.STREAM_MUSIC) {
            mMediaVolumeIndex = index;
        } else {
        }
        update(context, toContentValues());
    }

}

interface AudioProfileColumns {
    public static final String ID = "_id";
    public static final String NAME = "name";
    public static final String DISPLAY_NAME = "displayName";
    public static final String IS_SELECTED = "isSelected";
    public static final String IS_VIBRATE = "isVibrate";
    public static final String IS_VIBRATE_MSG = "isVibrateMsg";
    public static final String IS_SILENT = "isSilent";
    public static final String RING_URI = "ringUri";
    public static final String RINGTWO_URI = "ringTwoUri";
    public static final String MESSAGE_URI = "messageUri";
    public static final String MSG_URI = "messagetoneUri";
    public static final String NOTIFICATION_URI = "notificationUri";
    public static final String RING_VOLUME_INDEX = "ringVolumeIndex";
    public static final String ALARM_VOLUME_INDEX = "alarmVolumeIndex";
    public static final String NOTIFICATION_VOLUME_INDEX = "notificationVolumeIndex";
    public static final String MEDIA_VOLUME_INDEX = "mediaVolumeIndex";
    public static final String DTMF_TONE = "dtmfTone";
    public static final String CHARGING_SOUNDS = "chargingsounds";
    public static final String SOUND_EFFECTS = "soundEffects";
    public static final String LOCK_SOUNDS = "lockSounds";
    public static final String HAPTIC_FEEDBACK = "hapticFeedback";
    public static final String NOTIFICATIONS_USE_RING_VOLUME = "notificationsUseRingVolume";
}
