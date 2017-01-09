/*
 * Copyright (C) 2007 The Android Open Source Project
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

package android.media;

import com.android.internal.database.SortCursor;

import android.annotation.SdkConstant;
import android.annotation.SdkConstant.SdkConstantType;
import android.app.Activity;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;
import android.os.Process;
import android.provider.MediaStore;
import android.provider.Settings;
import android.provider.Settings.System;
import android.provider.SettingsEx;
import android.provider.SettingsEx.SystemEx;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

import libcore.io.Streams;

import android.telephony.TelephonyManager;
import android.os.EnvironmentEx;
import android.os.ParcelFileDescriptor;
import android.os.SystemProperties;
import com.android.internal.telephony.TelephonyProperties;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;


/**
 * @hide
 *
 */
public class RingtoneManagerEx extends RingtoneManager{

    private static final String TAG = "RingtoneManagerEx";

    private final Activity mActivity;
    private final Context mContext;

    /*
     * must refers the MEDIA_COLUMNS in RingtoneManager.
     */
    private static final String[] MEDIA_COLUMNS = new String[] {
        MediaStore.Audio.Media._ID, MediaStore.Audio.Media.TITLE,
        "\"" + MediaStore.Audio.Media.EXTERNAL_CONTENT_URI + "\"",
        MediaStore.Audio.Media.TITLE_KEY
    };

    public RingtoneManagerEx(Activity activity) {
        super(activity);
        mActivity = activity;
        mContext = activity;
    }

    public RingtoneManagerEx(Context context) {
        super(context);
        mActivity = null;
        mContext = context;
    }

    /**
     * @param type The ringtone type whose default should be returned.
     * @param phoneid phoneid
     * @return The {@link Uri} of the default ringtone for the given type.
     * @hide
     */
    public static Uri getActualDefaultRingtoneUri(Context context, int type, int phoneId) {
        String setting = getSettingForType(type, phoneId);
        if (setting == null)
            return null;
        final String uriString = Settings.System.getString(context.getContentResolver(), setting);
        Uri uri = (uriString != null ? Uri.parse(uriString) : null);
        Cursor cursor = null;
        try {
            ParcelFileDescriptor pfd = null;
            pfd = context.getContentResolver().openFileDescriptor(uri, "r");
            if (pfd != null) {
                pfd.close();
            }
        } catch (FileNotFoundException ex) {
            uri = getProfileDefaultUri(context, type);
        } catch (Exception sqle) {
            Log.d(TAG, sqle.toString());
        }
        return uri;
    }

    public static String getSettingForType(int type, int phoneId) {
        if ((type & RingtoneManager.TYPE_RINGTONE) != 0) {
            return getSetting(Settings.System.RINGTONE, phoneId);
        } else if ((type & RingtoneManager.TYPE_NOTIFICATION) != 0) {
            return Settings.System.NOTIFICATION_SOUND;
        } else if ((type & RingtoneManager.TYPE_ALARM) != 0) {
            return Settings.System.ALARM_ALERT;
        } else {
            return null;
        }
    }

    public static Uri getCacheForType(int type, int phoneId) {
        if ((type & RingtoneManager.TYPE_RINGTONE) != 0) {
            return getCacheUriForType(Settings.System.RINGTONE_CACHE, phoneId);
        } else if ((type & RingtoneManager.TYPE_NOTIFICATION) != 0) {
            return Settings.System.NOTIFICATION_SOUND_CACHE_URI;
        } else if ((type & RingtoneManager.TYPE_ALARM) != 0) {
            return Settings.System.ALARM_ALERT_CACHE_URI;
        } else {
            return null;
        }
    }

    public static String getSetting(String defaultSetting, int phoneId) {
        if (isMultiSimEnabledEx()) {
            return defaultSetting + phoneId;
        } else {
            return defaultSetting;
        }
    }

    public static Uri getCacheUriForType(String defaultSetting, int phoneId) {
        if (isMultiSimEnabledEx()) {
            return Settings.System.getUriFor(defaultSetting + phoneId);
        } else {
            return Settings.System.getUriFor(defaultSetting);
        }
    }

    public static String getSetting(String defaultSetting, long subId) {
        if (isMultiSimEnabledEx()) {
            return defaultSetting + subId;
        } else {
            return defaultSetting;
        }
    }

    private static boolean isMultiSimEnabledEx() {
        String multiSimConfig =
                SystemProperties.get(TelephonyProperties.PROPERTY_MULTI_SIM_CONFIG);
        return (multiSimConfig.equals("dsds") || multiSimConfig.equals("dsda") || multiSimConfig
                .equals("tsts"));
    }

    /**
     * @param type The ringtone type whose default should be returned.
     * @param uri ringtone uri
     * @param phoneid phoneid
     * @return void
     * @hide
     */
    public static void setActualDefaultRingtoneUri(Context context, int type, Uri ringtoneUri,
            int phoneId) {
        final ContentResolver resolver = context.getContentResolver();
        String setting = getSettingForType(type, phoneId);
        if (setting == null)
            return;
        Settings.System.putString(context.getContentResolver(), setting,
                ringtoneUri != null ? ringtoneUri.toString() : null);

        // Stream selected ringtone into cache so it's available for playback
        // when CE storage is still locked
        if (ringtoneUri != null) {
            final Uri cacheUri = getCacheForType(type, phoneId);
            try (InputStream in = openRingtone(context, ringtoneUri);
                    OutputStream out = resolver.openOutputStream(cacheUri)) {
                Streams.copy(in, out);
            } catch (IOException e) {
                Log.w(TAG, "Failed to cache ringtone: " + e);
            }
        }
    }

    /**
     * @hide
     */
    public static int getRingtonePhoneId(Uri uri) {
        if (uri == null) {
            return TelephonyManager.getDefault().getDefaultSim();
        }
        String uriStr = uri.toString();
        String ringtoneUriStr = Settings.System.DEFAULT_RINGTONE_URI.toString();
        if (uriStr.startsWith((ringtoneUriStr))
                && uriStr.length() != ringtoneUriStr.length()) {
            return Integer.parseInt(uriStr.substring(ringtoneUriStr.length(),
                    ringtoneUriStr.length() + 1));
        }
        return TelephonyManager.getDefault().getDefaultSim();
    }

    /**
     * Gets the current default sound's {@link Uri}. This will give the actual sound {@link Uri},
     * instead of using this, most clients can use {@link System#DEFAULT_RINGTONE_URI}.
     *
     * @param context A context used for querying.
     * @param type The type whose default sound should be returned. One of {@link #TYPE_RINGTONE},
     *            {@link #TYPE_NOTIFICATION}, or {@link #TYPE_ALARM}.
     * @return A {@link Uri} pointing to the default sound for the sound type.
     * @see #setActualDefaultRingtoneUri(Context, int, Uri)
     * @hide
     */
    public static Uri getProfileDefaultUri(Context context, int type) {

        String ringerUriString = Settings.System.getString(context.getContentResolver(),
                SettingsEx.SystemEx.DEFAULT_RINGTONE);
        Uri defaultRingtoneUri = (ringerUriString != null ? Uri.parse(ringerUriString) : null);
        String notificationUriString = Settings.System.getString(context.getContentResolver(),
                SettingsEx.SystemEx.DEFAULT_NOTIFICATION);
        Uri defaultNotificationUri = (notificationUriString != null ? Uri
                .parse(notificationUriString) : null);
        String alarmUriString = Settings.System.getString(context.getContentResolver(),
                SettingsEx.SystemEx.DEFAULT_ALARM);
        Uri defaultAlarmUri = (alarmUriString != null ? Uri.parse(alarmUriString) : null);
        if ((type & RingtoneManager.TYPE_RINGTONE) != 0) {
            return defaultRingtoneUri;
        } else if ((type & RingtoneManager.TYPE_NOTIFICATION) != 0) {
            return defaultNotificationUri;
        } else if ((type & RingtoneManager.TYPE_ALARM) != 0) {
            return defaultAlarmUri;
        } else {
            return null;
        }

    }

    public static final String EXTRA_RINGTONE_INCLUDE_EXTERNAL = "android.intent.extra.ringtone.INCLUDE_EXTERNAL";
    protected Cursor mCustomCursor;
    private boolean mIncludeDrm;

    /*
     * must refers the query method in RingtoneManager.
     */
    private Cursor query(Uri uri,
            String[] projection,
            String selection,
            String[] selectionArgs,
            String sortOrder) {
        if (mActivity != null) {
            return mActivity.managedQuery(uri, projection, selection, selectionArgs, sortOrder);
        } else {
            return mContext.getContentResolver().query(uri, projection, selection, selectionArgs,
                    sortOrder);
        }
    }

    /**
    * @hide
    */
    public Cursor getExternalMusics() {
        // Get the external media cursor. First check to see if it is mounted.
        //final String status = Environment.getExternalStorageState();
        //if(!status.equals(Environment.MEDIA_MOUNTED) && !status.equals(Environment.MEDIA_MOUNTED_READ_ONLY)){
         //   return null;
        //}

        /*SPRD: when unmount sd card, scan customize ringtones @{*/
        final String external_status = EnvironmentEx.getExternalStoragePathState();
        final String internal_status = EnvironmentEx.getInternalStoragePathState();
        if(!external_status.equals(Environment.MEDIA_MOUNTED) && !internal_status.equals(Environment.MEDIA_MOUNTED)
                && !external_status.equals(Environment.MEDIA_MOUNTED_READ_ONLY) && !internal_status.equals(Environment.MEDIA_MOUNTED_READ_ONLY)) {
              return null;
        }
        /*}@*/

        try {
            if (mCustomCursor != null && !mCustomCursor.isClosed() && mCustomCursor.requery()) {
                return mCustomCursor;
            }
        } catch (android.database.StaleDataException e) {
            Log.e(TAG, "requery error: cursor is closed" + e);
        }

        // filter
        StringBuilder where = new StringBuilder();
        where.append(MediaStore.Audio.Media.TITLE + " != ''");
        where.append(" AND " + MediaStore.Audio.Media.IS_MUSIC + "=1");
        /*SPRD:Bug506079 DRM music should not be set as rings.Bug510488 @{*/
        where.append(" AND (" + MediaStore.Audio.Media.IS_DRM + "!=1");
        where.append(" OR " + MediaStore.Audio.Media.IS_DRM + " is NULL )");
        /*}@*/

        //SPRD: when unmount sd card, scan customize ringtones
        return mCustomCursor = ((external_status.equals(Environment.MEDIA_MOUNTED) || internal_status
                .equals(Environment.MEDIA_MOUNTED) || external_status.equals(Environment.MEDIA_MOUNTED_READ_ONLY) || internal_status
                .equals(Environment.MEDIA_MOUNTED_READ_ONLY)) ? query(
                MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, MEDIA_COLUMNS,
                where.toString(), null,
                MediaStore.Audio.Media.DEFAULT_SORT_ORDER) : null);
    }

    /**
    * @hide
    */
    public int getCunstomRingtonePosition(Uri ringtoneUri) {

        if (ringtoneUri == null) return -1;

        final Cursor cursor = getExternalMusics();

        if (cursor == null) {
            return -1;
        }

        //final int cursorCount = cursor.getCount();

        if (!cursor.moveToFirst()) {
            return -1;
        }

        // Only create Uri objects when the actual URI changes
        Uri currentUri = null;
        String previousUriString = null;
        //for (int i = 0; i < cursorCount; i++) {
        while(!cursor.isAfterLast()) {
            String uriString = cursor.getString(URI_COLUMN_INDEX);
            if (currentUri == null || !uriString.equals(previousUriString)) {
                currentUri = Uri.parse(uriString);
            }

            if (ringtoneUri.equals(ContentUris.withAppendedId(currentUri, cursor
                    .getLong(ID_COLUMN_INDEX)))) {
                return cursor.getPosition();
            }

            // SPRD: if cursor has moved to end, we'll cancel the next move which is needless.
            // This special handling is mainly for avoiding exception from AbstractCursor.checkPosition
            //if (cursor.getPosition() < cursorCount - 1) {
            cursor.moveToNext();
            //}

            previousUriString = uriString;
        }

        return -1;
    }

    /*
     * must refers the getUriFromCursor method in RingtoneManager.
     */
    private static Uri getUriFromCursor(Cursor cursor) {
        return ContentUris.withAppendedId(Uri.parse(cursor.getString(URI_COLUMN_INDEX)), cursor
                .getLong(ID_COLUMN_INDEX));
    }

    /**
    * @hide
    */
    public Uri getCustomRingtoneUri(int position) {
        final Cursor cursor = getExternalMusics();
        if(cursor == null){
            return null;
        }

        if (!cursor.moveToPosition(position)) {
            return null;
        }

        return getUriFromCursor(cursor);
    }

    /**
     * Returns a {@link Ringtone} for a given sound URI on the given stream
     * type. Normally, if you change the stream type on the returned
     * {@link Ringtone}, it will re-create the {@link MediaPlayer}. This is just
     * an optimized route to avoid that.
     *
     * @param streamType The stream type for the ringtone, or -1 if it should
     *            not be set (and the default used instead).
     * @see #getRingtone(Context, Uri)
     */
    private static Ringtone getRingtone(final Context context, Uri ringtoneUri, int streamType) {
        try {
            final Ringtone r = new Ringtone(context, true);
            if (streamType >= 0) {
                r.setStreamType(streamType);
            }
            r.setUri(ringtoneUri);
            return r;
        } catch (Exception ex) {
            Log.e(TAG, "Failed to open ringtone " + ringtoneUri + ": " + ex);
        }

        return null;
    }

    /**
    * @hide
    */
    public Ringtone getCustomRingtone(int position) {
        if (getStopPreviousRingtone() && getPreviousRingtone() != null) {
            getPreviousRingtone().stop();
        }

        Ringtone ringtone = getRingtone(mContext, getCustomRingtoneUri(position), inferStreamType());
        setPreviousRingtone(ringtone);
        return ringtone;
    }

    /**
     * Try opening the given ringtone locally first, but failover to
     * {@link IRingtonePlayer} if we can't access it directly. Typically happens
     * when process doesn't hold
     * {@link android.Manifest.permission#READ_EXTERNAL_STORAGE}.
     */
    private static InputStream openRingtone(Context context, Uri uri) throws IOException {
        final ContentResolver resolver = context.getContentResolver();
        try {
            return resolver.openInputStream(uri);
        } catch (SecurityException | IOException e) {
            Log.w(TAG, "Failed to open directly; attempting failover: " + e);
            final IRingtonePlayer player = context.getSystemService(AudioManager.class)
                    .getRingtonePlayer();
            try {
                return new ParcelFileDescriptor.AutoCloseInputStream(player.openRingtone(uri));
            } catch (Exception e2) {
                throw new IOException(e2);
            }
        }
    }
}
