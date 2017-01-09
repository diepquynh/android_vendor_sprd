package com.sprd.audioprofile;

import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.media.AudioManager;
import android.media.RingtoneManager;
import android.media.RingtoneManagerEx;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;
import java.io.File;

import android.telephony.TelephonyManager;

public class AudioProfileProvider extends ContentProvider {

    private static final String DATABASE_NAME = "ProfileProvider.db";

    private static final int VERSION = 4;
    private static final int ITEMS = 1;
    private static final int ITEM = 2;
    private SQLiteDatabase mDatabase;
    private Context mContext;
    private static int mPhoneCount = TelephonyManager.getDefault().getPhoneCount();
    public static String [] ringtoneStr = new String [mPhoneCount];
    public static String [] mMessagetoneStr = new String [mPhoneCount];
    private static final String TAG = "AudioProfileProvider";

    //sprd:Bug 400062
    private AudioManager mAudioManager;

    private static final UriMatcher sURIMatcher;

    static {
        sURIMatcher = new UriMatcher(UriMatcher.NO_MATCH);
        sURIMatcher.addURI(AudioProfile.AUTHORITY, AudioProfile.PROFILE_TABLE, ITEMS);
        sURIMatcher.addURI(AudioProfile.AUTHORITY, AudioProfile.PROFILE_TABLE + "/#", ITEM);
        for (int i = 0; i < mPhoneCount; i++) {
            ringtoneStr[i] = AudioProfileColumns.RING_URI + i;
            mMessagetoneStr[i] = AudioProfileColumns.MSG_URI + i;
        }
    }

    @Override
    public int delete(Uri uri, String whereClause, String[] whereArgs) {
        int count = 0;
        switch (sURIMatcher.match(uri)) {
            case ITEMS:
                deleteAllProfiles(getDatabase());
                resetDefProfiles(getDatabase());
                break;
            case ITEM:
                String id = uri.getPathSegments().get(1);
                count = getDatabase().delete(AudioProfile.PROFILE_TABLE, AudioProfileColumns.ID
                        + "=" + id  + (!TextUtils.isEmpty(whereClause) ? "AND (" + whereClause
                                + ')' : ""), whereArgs);
                break;
        }
        getContext().getContentResolver().notifyChange(uri, null);
        return count;
    }

    @Override
    public String getType(Uri uri) {
        return null;
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        Uri insertUri = null;
        switch (sURIMatcher.match(uri)) {
            case ITEMS:
                long rowId = getDatabase().insert(AudioProfile.PROFILE_TABLE, null, values);
                insertUri = ContentUris.withAppendedId(AudioProfile.CONTENT_URI, rowId);
                break;
            case ITEM:
                break;
        }
        getContext().getContentResolver().notifyChange(uri, null);
        return insertUri;
    }

    @Override
    public boolean onCreate() {
        mContext = getContext();
        return true;
    }

    private boolean isDatabaseExist() {
        File file = getContext().getDatabasePath(DATABASE_NAME);
        if (!file.exists()) {
            return false;
        }
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
            String[] selectionArgs, String sortOrder) {
        if (isDatabaseExist() == false) {
            synchronized (this) {
                mDatabase = null;
            }
        }
        Cursor c = null;
        int match = sURIMatcher.match(uri);
        switch (match) {
            case ITEMS:
                c = getDatabase().query(AudioProfile.PROFILE_TABLE, projection, selection,
                        selectionArgs, null, null, sortOrder);
                break;
            case ITEM:
                c = getDatabase().query(AudioProfile.PROFILE_TABLE, projection, AudioProfileColumns.ID
                                + " = " + uri.getPathSegments().get(1), selectionArgs,
                        null, null, sortOrder);
                break;
        }
        return c;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection,
            String[] selectionArgs) {
        int count = 0;
        switch (sURIMatcher.match(uri)) {
            case ITEMS:
                count = getDatabase().update(AudioProfile.PROFILE_TABLE, values, selection,
                        selectionArgs);
                break;
            case ITEM:
                String id = uri.getPathSegments().get(1);
                count = getDatabase().update(AudioProfile.PROFILE_TABLE,values, AudioProfileColumns.ID
                        + "=" + id + (!TextUtils.isEmpty(selection) ? " AND ("
                                        + selection + ')' : ""), selectionArgs);
                break;
        }
        getContext().getContentResolver().notifyChange(uri, null);
        return count;
    }

    private synchronized SQLiteDatabase getDatabase() {
        if(mDatabase == null){
            SQLiteOpenHelper mOpenHelper = new ProfileDatabaseHelper(getContext(), DATABASE_NAME);
            mDatabase = mOpenHelper.getWritableDatabase();
        }
        return mDatabase;
    }

    private void createProfileTable(SQLiteDatabase db) {

        ContentResolver resolver = mContext.getContentResolver();
        int defDTMFTone = Settings.System.getInt(resolver,Settings.System.DTMF_TONE_WHEN_DIALING, 1);
        int defSoundEffects = AudioProfile.IS_SOUND_EFFECTS;
        int defChargingSounds = Settings.Global.getInt(resolver, Settings.Global.CHARGING_SOUNDS_ENABLED, 1);
        int defLockSounds = Settings.System.getInt(resolver,Settings.System.LOCKSCREEN_SOUNDS_ENABLED, 1);
        int defHapticFeedback = AudioProfile.IS_HAPTIC_FEEDBACK;
        Uri defUri = RingtoneManager.getActualDefaultRingtoneUri(mContext, RingtoneManager.TYPE_RINGTONE);
        String defRingUri = "";
        if (defUri != null) {
            defRingUri = defUri.toString();
        }
        Uri defTwoUri = RingtoneManagerEx.getActualDefaultRingtoneUri(mContext,RingtoneManager.TYPE_RINGTONE, 1);
        String defRingTwoUri = "";
        if (defTwoUri != null) {
            defRingTwoUri = defTwoUri.toString();
        }
        Log.w("mytest", "" + defRingTwoUri);
        Uri defNotUri = RingtoneManager.getActualDefaultRingtoneUri(mContext,RingtoneManager.TYPE_NOTIFICATION);
        String defNotificationUri = "";
        String defMesOneUri = "";
        String defMesTwoUri = "";
        if (defNotUri != null){
            defNotificationUri = defNotUri.toString();
            defMesOneUri = defNotUri.toString();
            defMesTwoUri = defNotUri.toString();
        }
        String profileColumns = AudioProfileColumns.NAME + " TEXT, "
                + AudioProfileColumns.DISPLAY_NAME + " TEXT DEFAULT '" + AudioProfile.DEFAULT_DISPLAY_NAME + "', "
                + AudioProfileColumns.IS_SELECTED + " INTEGER DEFAULT " + AudioProfile.NOT_SELECTED + ", "
                + AudioProfileColumns.IS_VIBRATE + " INTEGER DEFAULT " + AudioProfile.NOT_VIBRATE + ", "
                + AudioProfileColumns.IS_VIBRATE_MSG + " INTEGER DEFAULT " + AudioProfile.NOT_VIBRATE_MSG + ", "
                + AudioProfileColumns.IS_SILENT + " INTEGER DEFAULT " + AudioProfile.NOT_SILENT + ", "
                + AudioProfileColumns.MESSAGE_URI + " TEXT DEFAULT '" + defNotificationUri + "', "
                + AudioProfileColumns.NOTIFICATION_URI + " TEXT DEFAULT '" + defNotificationUri + "', "
                + AudioProfileColumns.RING_VOLUME_INDEX + " INTEGER DEFAULT " + AudioProfile.DEFAULT_STREAM_RING_VOLUME + ", "
                + AudioProfileColumns.ALARM_VOLUME_INDEX + " INTEGER DEFAULT " + AudioProfile.DEFAULT_STREAM_ALARM_VOLUME + ", "
                + AudioProfileColumns.NOTIFICATION_VOLUME_INDEX + " INTEGER DEFAULT " + AudioProfile.DEFAULT_STREAM_NOTIFICATION_VOLUME + ", "
                + AudioProfileColumns.MEDIA_VOLUME_INDEX + " INTEGER DEFAULT " + AudioProfile.DEFAULT_STREAM_MEDIA_VOLUME + ", "
                + AudioProfileColumns.DTMF_TONE + " INTEGER DEFAULT " + defDTMFTone + ", "
                + AudioProfileColumns.SOUND_EFFECTS + " INTEGER DEFAULT " + defSoundEffects + ", "
                + AudioProfileColumns.CHARGING_SOUNDS + " INTEGER DEFAULT " + defChargingSounds + ", "
                + AudioProfileColumns.LOCK_SOUNDS + " INTEGER DEFAULT " + defLockSounds + ", "
                + AudioProfileColumns.HAPTIC_FEEDBACK + " INTEGER DEFAULT " + defHapticFeedback + ", "
                + AudioProfileColumns.NOTIFICATIONS_USE_RING_VOLUME + " INTEGER DEFAULT " + AudioProfile.NOTIFICATIONS_NOT_USE_RING_VOLUME + ", " ;
        String providerColumns;
        StringBuffer ringtoneColumns = new StringBuffer();
        for (int i = 0; i < mPhoneCount; i++) {
            String simRingtoneColumns = "";
            if (mPhoneCount - 1 == i) {
                simRingtoneColumns = ringtoneStr[i] + " TEXT DEFAULT '" + defTwoUri + "', "
                + mMessagetoneStr[i] + " TEXT DEFAULT '" + defMesTwoUri + "'";
            } else {
                simRingtoneColumns = ringtoneStr[i] + " TEXT DEFAULT '" + defRingUri + "', "
                + mMessagetoneStr[i] + " TEXT DEFAULT '" + defMesOneUri + "', ";
            }
            ringtoneColumns.append(simRingtoneColumns);
        }
        providerColumns = profileColumns + ringtoneColumns + " )";
        String sqlDrop = "DROP TABLE IF EXISTS " + AudioProfile.PROFILE_TABLE;
        db.execSQL(sqlDrop);
        String sql = "CREATE TABLE " + AudioProfile.PROFILE_TABLE + " ("
                + AudioProfileColumns.ID + " INTEGER PRIMARY KEY AUTOINCREMENT, " + providerColumns;
        db.execSQL(sql);
    }

    private void deleteAllProfiles(SQLiteDatabase db) {
        String delSql = "DELETE FROM " + AudioProfile.PROFILE_TABLE;
        db.execSQL(delSql);
        String resetSeqSql = "UPDATE SQLITE_SEQUENCE SET SEQ=0 WHERE NAME='" + AudioProfile.PROFILE_TABLE + "'";
        db.execSQL(resetSeqSql);
    }

    private void resetDefProfiles(SQLiteDatabase db) {
        // general
        Uri defRingtoneUri = AudioProfile.getDefaultUri(mContext, RingtoneManager.TYPE_RINGTONE);
        Uri defMessagetoneUri = AudioProfile.getDefaultUri(mContext, RingtoneManager.TYPE_NOTIFICATION);
        Uri defNotificationUri = AudioProfile.getDefaultUri(mContext,
                RingtoneManager.TYPE_NOTIFICATION);
        String genernalColumns = AudioProfileColumns.NAME + "', '"
                + AudioProfileColumns.DISPLAY_NAME + "', '"
                + AudioProfileColumns.IS_SELECTED + "', '"
                + AudioProfileColumns.NOTIFICATION_URI + "', '";
        String genernalValues = AudioProfile.GENERAL_NAME + "', '"
                + AudioProfile.GENERAL_NAME + "', '"
                + AudioProfile.IS_SELECTED + "', '"
                + defNotificationUri + "', '";
        for (int i = 0; i < mPhoneCount; i++) {
            if (mPhoneCount - 1 == i) {
                genernalColumns += ringtoneStr[i] + "', '" + mMessagetoneStr[i];
                genernalValues += defRingtoneUri + "', '" + defMessagetoneUri;
            } else {
                genernalColumns += ringtoneStr[i] + "', '" + mMessagetoneStr[i] + "', '";
                genernalValues += defRingtoneUri + "', '" + defMessagetoneUri + "', '";
            }

        }
        db.execSQL("INSERT INTO " + AudioProfile.PROFILE_TABLE + "( '" + genernalColumns
                + "' ) VALUES ( '"
                + genernalValues
                + "' )");

        // silent except id others invalid
        db.execSQL("INSERT INTO " + AudioProfile.PROFILE_TABLE + "( '"
                + AudioProfileColumns.NAME + "', '"
                + AudioProfileColumns.DISPLAY_NAME + "', '"
                + AudioProfileColumns.IS_SILENT
                + "') VALUES ('"
                + AudioProfile.SILENT_NAME + "', '"
                + AudioProfile.SILENT_NAME + "', '"
                + AudioProfile.IS_SILENT
                + "' )");
        // vibration except id others invalid
        db.execSQL("INSERT INTO " + AudioProfile.PROFILE_TABLE + "( '"
                + AudioProfileColumns.NAME + "', '"
                + AudioProfileColumns.DISPLAY_NAME + "', '"
                + AudioProfileColumns.IS_SILENT + "', '"
                + AudioProfileColumns.IS_VIBRATE + "', '"
                + AudioProfileColumns.IS_VIBRATE_MSG
                + "') VALUES ('"
                + AudioProfile.VIBRATION_NAME + "', '"
                + AudioProfile.VIBRATION_NAME + "', '"
                + AudioProfile.IS_SILENT + "', '"
                + AudioProfile.IS_VIBRATE + "', '"
                + AudioProfile.IS_VIBRATE_MSG
                + "' )");
        // outdoor
        AudioManager mAudioManager = (AudioManager) mContext
                .getSystemService(Context.AUDIO_SERVICE);
        int maxRingVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_RING);
        int maxNotificationVolume = mAudioManager
                .getStreamMaxVolume(AudioManager.STREAM_NOTIFICATION);
        int maxAlarmVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_ALARM);
        String outdoorColumns = AudioProfileColumns.NAME + "', '"
                + AudioProfileColumns.DISPLAY_NAME + "', '"
                + AudioProfileColumns.IS_VIBRATE + "', '"
                + AudioProfileColumns.IS_VIBRATE_MSG + "', '"
                + AudioProfileColumns.RING_VOLUME_INDEX + "', '"
                + AudioProfileColumns.NOTIFICATION_VOLUME_INDEX + "', '"
                + AudioProfileColumns.ALARM_VOLUME_INDEX + "', '"
                + AudioProfileColumns.DTMF_TONE + "', '"
                + AudioProfileColumns.SOUND_EFFECTS + "', '"
                + AudioProfileColumns.LOCK_SOUNDS + "', '"
                + AudioProfileColumns.HAPTIC_FEEDBACK + "', '"
                + AudioProfileColumns.NOTIFICATION_URI + "', '";
        String outdoorValues = AudioProfile.OUTDOOR_NAME + "', '"
                + AudioProfile.OUTDOOR_NAME + "', '"
                + AudioProfile.IS_VIBRATE + "', '"
                + AudioProfile.IS_VIBRATE_MSG + "', '"
                + maxRingVolume + "', '"
                + maxNotificationVolume + "', '"
                + maxAlarmVolume + "', '"
                + AudioProfile.IS_DTMF_TONE + "', '"
                + AudioProfile.IS_SOUND_EFFECTS + "', '"
                + AudioProfile.IS_LOCK_SOUNDS + "', '"
                + AudioProfile.IS_HAPTIC_FEEDBACK + "', '"
                + defNotificationUri + "', '";
        for (int i = 0; i < mPhoneCount; i++) {
            if (mPhoneCount - 1 == i) {
                outdoorColumns += ringtoneStr[i] + "', '" + mMessagetoneStr[i];
                outdoorValues += defRingtoneUri + "', '" + defMessagetoneUri;

            } else {
                outdoorColumns += ringtoneStr[i] + "', '" + mMessagetoneStr[i] + "', '";;
                outdoorValues += defRingtoneUri + "', '" + defMessagetoneUri + "', '";
            }

        }
        db.execSQL("INSERT INTO " + AudioProfile.PROFILE_TABLE + "( '"
                + outdoorColumns
                + "') VALUES ('"
                + outdoorValues
                + "' )");
    }

    private void insertDefProfiles(SQLiteDatabase db) {
        // general
        /* sprd:Bug 400062  Get ALARM_VOLUME_INDEX from settings.db and sync to Audioprofile.db when creating the Audioprofile.db @{ */
        int alarmVolumeIndex = AudioProfile.DEFAULT_STREAM_ALARM_VOLUME;
        int ringVolumeIndex = AudioProfile.DEFAULT_STREAM_RING_VOLUME;
        int notificationVolumeIndex = AudioProfile.DEFAULT_STREAM_NOTIFICATION_VOLUME;
        if (mContext != null) {
            mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
            /* SPRD: Modified for bug 600020, Notification volume is not correct after first power on @{ */
            if (mAudioManager.getRingerMode() == mAudioManager.RINGER_MODE_NORMAL) {
                alarmVolumeIndex = mAudioManager.getStreamVolume(AudioManager.STREAM_ALARM);
                ringVolumeIndex = mAudioManager.getStreamVolume(AudioManager.STREAM_RING);
                notificationVolumeIndex = mAudioManager.getStreamVolume(AudioManager.STREAM_NOTIFICATION);
            }
            /* @} */
        }
        db.execSQL("INSERT INTO " + AudioProfile.PROFILE_TABLE + "( '"
                + AudioProfileColumns.NAME + "', '"
                + AudioProfileColumns.DISPLAY_NAME + "', '"
                + AudioProfileColumns.IS_SELECTED + "', '"
                + AudioProfileColumns.ALARM_VOLUME_INDEX + "', '"
                + AudioProfileColumns.RING_VOLUME_INDEX + "', '"
                + AudioProfileColumns.NOTIFICATION_VOLUME_INDEX
                + "' ) VALUES ( '"
                + AudioProfile.GENERAL_NAME + "', '"
                + AudioProfile.GENERAL_NAME + "', '"
                + AudioProfile.IS_SELECTED + "', '"
                + alarmVolumeIndex + "', '"
                + ringVolumeIndex + "', '"
                + notificationVolumeIndex
                + "' )");
        /* @} */

        // silent except id others invalid
        db.execSQL("INSERT INTO " + AudioProfile.PROFILE_TABLE + "( '"
                + AudioProfileColumns.NAME + "', '"
                + AudioProfileColumns.DISPLAY_NAME + "', '"
                + AudioProfileColumns.IS_SILENT
                + "') VALUES ('"
                + AudioProfile.SILENT_NAME + "', '"
                + AudioProfile.SILENT_NAME + "', '"
                + AudioProfile.IS_SILENT
                + "' )");
        // vibration except id others invalid
        db.execSQL("INSERT INTO " + AudioProfile.PROFILE_TABLE + "( '"
                + AudioProfileColumns.NAME + "', '"
                + AudioProfileColumns.DISPLAY_NAME + "', '"
                + AudioProfileColumns.IS_SILENT + "', '"
                + AudioProfileColumns.IS_VIBRATE + "', '"
                + AudioProfileColumns.IS_VIBRATE_MSG
                + "') VALUES ('"
                + AudioProfile.VIBRATION_NAME + "', '"
                + AudioProfile.VIBRATION_NAME + "', '"
                + AudioProfile.IS_SILENT + "', '"
                + AudioProfile.IS_VIBRATE + "', '"
                + AudioProfile.IS_VIBRATE_MSG
                + "' )");
        // outdoor
        /* SPRD: Modify for bug462950. @{ */
        if (mContext != null) {
            AudioManager mAudioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
            int maxRingVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_RING);
            int maxNotificationVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_NOTIFICATION);
            int maxAlarmVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_ALARM);
            int maxMediaVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            db.execSQL("INSERT INTO " + AudioProfile.PROFILE_TABLE + "( '"
                    + AudioProfileColumns.NAME + "', '"
                    + AudioProfileColumns.DISPLAY_NAME + "', '"
                    + AudioProfileColumns.IS_VIBRATE + "', '"
                    + AudioProfileColumns.IS_VIBRATE_MSG + "', '"
                    + AudioProfileColumns.RING_VOLUME_INDEX + "', '"
                    + AudioProfileColumns.NOTIFICATION_VOLUME_INDEX + "', '"
                    + AudioProfileColumns.ALARM_VOLUME_INDEX + "', '"
                    + AudioProfileColumns.MEDIA_VOLUME_INDEX + "', '"
                    + AudioProfileColumns.DTMF_TONE + "', '"
                    + AudioProfileColumns.SOUND_EFFECTS + "', '"
                    + AudioProfileColumns.LOCK_SOUNDS + "', '"
                    + AudioProfileColumns.HAPTIC_FEEDBACK
                    + "') VALUES ('"
                    + AudioProfile.OUTDOOR_NAME + "', '"
                    + AudioProfile.OUTDOOR_NAME + "', '"
                    + AudioProfile.IS_VIBRATE + "', '"
                    + AudioProfile.IS_VIBRATE_MSG + "', '"
                    + maxRingVolume + "', '"
                    + maxNotificationVolume + "', '"
                    + maxAlarmVolume + "', '"
                    + maxMediaVolume + "', '"
                    + AudioProfile.IS_DTMF_TONE + "', '"
                    + AudioProfile.IS_SOUND_EFFECTS + "', '"
                    + AudioProfile.IS_LOCK_SOUNDS + "', '"
                    + AudioProfile.IS_HAPTIC_FEEDBACK
                    + "' )");
        }
        /* @} */
    }

    private class ProfileDatabaseHelper extends SQLiteOpenHelper {

        public ProfileDatabaseHelper(Context context, String name) {
            super(context, name, null, VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            createProfileTable(db);
            insertDefProfiles(db);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            /* SPRD: BUG 507632 after OTA, there is no messagetone, setting crash @{ */
            if (oldVersion >= 4) {
                return;
            }
            if (!checkColumnExist(db, AudioProfileColumns.IS_VIBRATE_MSG)) {
                db.beginTransaction();
                try {
                    db.execSQL("ALTER TABLE " + AudioProfile.PROFILE_TABLE + " ADD "
                            + AudioProfileColumns.IS_VIBRATE_MSG + " INTEGER DEFAULT "
                            + 0);
                    db.setTransactionSuccessful();
                } finally {
                    db.endTransaction();
                }
            }
            if (!checkColumnExist(db, AudioProfileColumns.MSG_URI + 0)) {
                db.beginTransaction();
                try {
                    Uri defNotificationUri = RingtoneManager.getActualDefaultRingtoneUri(
                            mContext, RingtoneManager.TYPE_NOTIFICATION);
                    String defNotificationString = defNotificationUri != null ? defNotificationUri
                            .toString()
                            : null;
                    for (int i = 0; i < mPhoneCount; i++) {
                        db.execSQL("ALTER TABLE " + AudioProfile.PROFILE_TABLE + " ADD "
                                + (AudioProfileColumns.MSG_URI + i) + " TEXT DEFAULT '"
                                + defNotificationString + "'");
                    }
                    db.setTransactionSuccessful();
                } finally {
                    db.endTransaction();
                }
            }
            if (!checkColumnExist(db, AudioProfileColumns.CHARGING_SOUNDS)) {
                int defChargingSound = Settings.Global.getInt(mContext.getContentResolver(),
                        Settings.Global.CHARGING_SOUNDS_ENABLED, 1);
                db.beginTransaction();
                try {
                    db.execSQL("ALTER TABLE " + AudioProfile.PROFILE_TABLE + " ADD "
                            + AudioProfileColumns.CHARGING_SOUNDS + " INTEGER DEFAULT "
                            + defChargingSound);
                    db.setTransactionSuccessful();
                } finally {
                    db.endTransaction();
                }
            }
            // add the MEDIA_VOLUME_INDEX ,as the descendence of Bug 484553
            if (!checkColumnExist(db, AudioProfileColumns.MEDIA_VOLUME_INDEX)) {
                db.beginTransaction();
                try {
                    db.execSQL("ALTER TABLE " + AudioProfile.PROFILE_TABLE + " ADD "
                            + AudioProfileColumns.MEDIA_VOLUME_INDEX + " INTEGER DEFAULT "
                            + AudioProfile.DEFAULT_STREAM_MEDIA_VOLUME);
                    db.execSQL("UPDATE " + AudioProfile.PROFILE_TABLE + " SET "
                            + AudioProfileColumns.NOTIFICATIONS_USE_RING_VOLUME + " = "
                            + AudioProfile.NOTIFICATIONS_NOT_USE_RING_VOLUME);
                    db.setTransactionSuccessful();
                } finally {
                    db.endTransaction();
                }
            }
        }

        private boolean checkColumnExist(SQLiteDatabase db, String columnToQuery) {
            boolean result = false;
            Cursor cursor = null;
            try {
                cursor = db.rawQuery("SELECT * FROM " + AudioProfile.PROFILE_TABLE + " LIMIT 0",
                        null);
                result = cursor != null && cursor.getColumnIndex(columnToQuery) != -1;
            } catch (Exception e) {
                Log.d(TAG, "checkColumnExist Exception:" + e.getMessage());
            } finally {
                if (cursor != null && !cursor.isClosed()) {
                    cursor.close();
                }
            }
            return result;
        }
        /* @} */
    }

}
