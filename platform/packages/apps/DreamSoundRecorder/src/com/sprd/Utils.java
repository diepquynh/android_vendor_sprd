package com.sprd.soundrecorder;

import java.text.NumberFormat;

import android.util.Log;
import android.telephony.TelephonyManager;
import android.os.AsyncTask;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.Context;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.widget.Toast;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.RingtoneManager;
import android.media.RingtoneManagerEx;
import android.net.Uri;
import android.database.Cursor;
import android.provider.MediaStore;
import android.text.format.DateUtils;

import com.android.soundrecorder.R;

public class Utils {
    public static String LOGTAG = "Utils";
    public static AudioManager audioManager =null;
    public static String makeTimeString4MillSec(Context context, int millSec) {
        String str = "";
        int hour = 0;
        int minute = 0;
        int second = 0;
        second =  Math.round((float)millSec/1000);
        if (second > 59) {
            minute = second / 60;
            second = second % 60;
        }
        if (minute > 59) {
            hour = minute / 60;
            minute = minute % 60;
        }
        str = (hour < 10 ? "0" + hour : hour) + ":"
                + (minute < 10 ? "0" + minute : minute) + ":"
                + (second < 10 ? "0" + second : second);
        if(hour == 0 && minute ==0 && second == 0){
            str = "< "+ context.getString(R.string.less_than_one_second);
        }
        return str;
    }

    /* SPRD: add new feature @{ */
    public static void doChoiceRingtone(final Context context, final long audioId) {
        int phoneCount = TelephonyManager.getDefault().getPhoneCount();
        boolean sim1Exist = isSimCardExist(context, 0);
        boolean sim2Exist = isSimCardExist(context, 1);
        if (phoneCount == 2) {
            if (sim1Exist && sim2Exist) {
                AlertDialog.Builder ringtonebuilder = new AlertDialog.Builder(context);
                String[] items = {
                        context.getString(R.string.ringtone_title_sim1),
                        context.getString(R.string.ringtone_title_sim2)
                };
                ringtonebuilder.setItems(items, new OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        switch (which) {
                            case 0:
                                setRingtone(context, audioId, 0);
                                break;
                            case 1:
                                setRingtone(context, audioId, 1);
                                break;
                            default:
                                Log.e(LOGTAG, "dialoginterface onclick  is null");
                                break;
                        }
                    }
                });
                ringtonebuilder.setTitle(R.string.ringtone_menu_short);
                ringtonebuilder.show();
            } else if (sim1Exist && !sim2Exist) {
                setRingtone(context, audioId, 0);
            } else if (!sim1Exist && sim2Exist) {
                setRingtone(context, audioId, 1);
            } else {
                Toast.makeText(context, R.string.please_insert_sim_card, 1000).show();
            }
        } else {
            if (sim1Exist) {
                setRingtone(context, audioId, -1);
            } else {
                Toast.makeText(context, R.string.please_insert_sim_card, 1000).show();
            }
        }
    }

    private static boolean isSimCardExist(Context context, int phoneID) {
        return TelephonyManager.getDefault().hasIccCard(phoneID);
    }

    public static Cursor query(Context context, Uri uri, String[] projection,
            String selection, String[] selectionArgs, String sortOrder, int limit) {
        try {
            ContentResolver resolver = context.getContentResolver();
            if (resolver == null) {
                return null;
            }
            if (limit > 0) {
                uri = uri.buildUpon().appendQueryParameter("limit", "" + limit).build();
            }
            return resolver.query(uri, projection, selection, selectionArgs, sortOrder);
         } catch (UnsupportedOperationException ex) {
            return null;
        }
    }

    public static Cursor query(Context context, Uri uri, String[] projection,
            String selection, String[] selectionArgs, String sortOrder) {
        return query(context, uri, projection, selection, selectionArgs, sortOrder, 0);
    }

    public static void setRingtone(Context context, long id, final int simID) {
        final Context tmpContext = context;
        final long tmpId = id;
        audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);

        AsyncTask<Void, Void, Integer> setRingTask = new AsyncTask<Void, Void, Integer>() {
            private static final int SET_SUCESS = 0;
            private static final int SET_FAIL_4_CANNOT_PLAY = 1;
            private static final int SET_FAIL_4_DB = 2;
            private String path = "";

            @Override
            protected Integer doInBackground(Void... params) {
                return setRingtoneInternal(simID, tmpContext, tmpId);
            }

            private Integer setRingtoneInternal(final int simID, final Context tmpContext,
                    final long tmpId) {
                ContentResolver resolver = tmpContext.getContentResolver();
                // Set the flag in the database to mark this as a ringtone
                Uri ringUri = ContentUris.withAppendedId(
                        MediaStore.Audio.Media.EXTERNAL_CONTENT_URI, tmpId);
                if (!isCanPlay(tmpContext, ringUri)) {
                    return SET_FAIL_4_CANNOT_PLAY;
                }
                try {
                    ContentValues values = new ContentValues(2);
                    values.put(MediaStore.Audio.Media.IS_RINGTONE, "1");
                    values.put(MediaStore.Audio.Media.IS_ALARM, "1");
                    resolver.update(ringUri, values, null, null);
                } catch (UnsupportedOperationException ex) {
                    // most likely the card just got unmounted
                    Log.e(LOGTAG, "couldn't set ringtone flag for id " + tmpId);
                    return SET_FAIL_4_DB;
                }

                String[] cols = new String[] {
                        MediaStore.Audio.Media._ID, MediaStore.Audio.Media.DATA,
                        MediaStore.Audio.Media.TITLE
                };

                String where = MediaStore.Audio.Media._ID + "=" + tmpId;
                Cursor cursor = query(tmpContext,
                        MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,
                        cols, where, null, null);
                try {
                    if (cursor != null && cursor.getCount() == 1) {
                        // Set the system setting to make this the current ringtone
                        cursor.moveToFirst();
                        if (simID == -1) {
                            RingtoneManagerEx.setActualDefaultRingtoneUri(tmpContext,
                                    RingtoneManager.TYPE_RINGTONE, ringUri,-1);
                        } else {
                            RingtoneManagerEx.setActualDefaultRingtoneUri(
                             tmpContext, RingtoneManager.TYPE_RINGTONE,
                              ringUri, simID);
                        }
                        path = cursor.getString(2);
                        return SET_SUCESS;
                    }
                } finally {
                    if (cursor != null) {
                        cursor.close();
                    }
                }
                return SET_FAIL_4_DB;
            }

            private boolean isCanPlay(Context tmpContext, Uri ringUri) {
                MediaPlayer mp = new MediaPlayer();
                mp.reset();
                try {
                    mp.setDataSource(tmpContext, ringUri);
                    return true;
                } catch (Exception e) {
                    return false;
                } finally {
                    if (mp != null) {
                        mp.release();
                        mp = null;
                    }
                }
            }

            @Override
            protected void onPostExecute(Integer result) {
                super.onPostExecute(result);
                if (audioManager.getRingerMode() == AudioManager.RINGER_MODE_SILENT
                        || audioManager.getRingerMode() == AudioManager.RINGER_MODE_VIBRATE) {
                    Toast.makeText(tmpContext, R.string.ring_set_silent_vibrate, 1000).show();
                } else if (result == SET_SUCESS) {
                    String message = null;
                    if (simID == -1) {
                        message = tmpContext.getString(R.string.ringtone_set, path);
                    } else if (simID == 0) {
                        message = tmpContext.getString(R.string.ringtone_set_sim1, path);
                    } else {
                        message = tmpContext.getString(R.string.ringtone_set_sim2, path);
                    }
                    Toast.makeText(tmpContext, message, Toast.LENGTH_SHORT).show();
                } else if (result == SET_FAIL_4_CANNOT_PLAY) {
                    Toast.makeText(tmpContext, R.string.ring_set_fail, 1000).show();
                }
            }
        };
        setRingTask.execute((Void[]) null);
    }
    /* @} */

    /* SPRD: add new feature @{ */
    public static String formatElapsedTimeUntilAlarm(Context context, long delta) {
        String[] formats = context.getResources().getStringArray(R.array.timer_recording_time);
        if (delta < DateUtils.MINUTE_IN_MILLIS) {
            return formats[0];
        }
        final long remainder = delta % DateUtils.MINUTE_IN_MILLIS;
        delta += remainder == 0 ? 0 : (DateUtils.MINUTE_IN_MILLIS - remainder);
        int hours = (int) delta / (1000 * 60 * 60);
        int minutes = (int) delta / (1000 * 60) % 60;
        String hourSeq = getNumberFormattedQuantityString(context, R.plurals.hours, hours);
        String minSeq = getNumberFormattedQuantityString(context, R.plurals.minutes, minutes);

        boolean showHours = hours > 0;
        boolean showMinutes = minutes > 0;
        int index = (showHours ? 1 : 0) | (showMinutes ? 2 : 0);
        return String.format(formats[index], hourSeq, minSeq);
    }

    public static String getNumberFormattedQuantityString(Context context, int id, int quantity) {
        final String localizedQuantity = NumberFormat.getInstance().format(quantity);
        return context.getResources().getQuantityString(id, quantity, localizedQuantity);
    }

    public static void updateStatusBarColor(Activity activity, boolean actionMode) {
        if (actionMode) {
            int cabStatusBarColor = activity.getResources().getColor(
                    R.color.status_bar_color_gray);
            activity.getWindow().setStatusBarColor(cabStatusBarColor);
        } else {
            int cabStatusBarColor = activity.getResources().getColor(
                    R.color.status_bar_color_orange);
            activity.getWindow().setStatusBarColor(cabStatusBarColor);
        }
    }
    /* @} */
}
