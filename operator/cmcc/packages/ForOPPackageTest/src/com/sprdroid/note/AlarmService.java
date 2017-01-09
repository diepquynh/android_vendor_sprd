package com.sprdroid.note;

import java.io.IOException;
import java.util.Calendar;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.IBinder;
import android.os.Vibrator;
import android.util.Log;

import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.RingtoneManager;

//该类只用来处理多媒体铃声播放
public class AlarmService extends Service {

    private static String TAG = "alarmService";
    private static AlarmService alarmService = null;
    private AlarmManager am;
    private MediaPlayer mp;
    private String action;
    private Vibrator v;
    private String rings;
    private String isVibrate;

    @Override
    public IBinder onBind(Intent arg0) {
        return null;
    }

    public Service getService() {
        return alarmService;
    }

    public void onCreate() {
        super.onCreate();
        am = (AlarmManager) getSystemService(ALARM_SERVICE);
        alarmService = this;

        IntentFilter filter = new IntentFilter();
        filter.addAction("com.sprdroid.note.STOP_ALARM");
        // 防止在灭屏时不能接受停止闹钟的广播，所以用静态注册。
        this.registerReceiver(this.receiver, filter);
    }

    public void onStart(Intent intent, int startId) {
        super.onStart(intent, startId);
        Log.d(TAG, "service start");
        AudioManager adm = (AudioManager) getSystemService(AUDIO_SERVICE);
        v = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        // String id = intent.getStringExtra("id");
        // String time = intent.getStringExtra("time");
        // Calendar cal = Calendar.getInstance();
        /*
         * String[] temp = time.split(":"); cal.set(Calendar.HOUR_OF_DAY,
         * Integer.parseInt(temp[0])); cal.set(Calendar.MINUTE,
         * Integer.parseInt(temp[1])); cal.set(Calendar.SECOND, 0);
         * cal.set(Calendar.MILLISECOND, 0); Intent i = new
         * Intent(AlarmService.this,CallAlarm.class);
         * intent.putExtra(DBOpenHelper.CLOCK_ID, id); PendingIntent pi =
         * PendingIntent.getBroadcast(AlarmService.this, Integer.parseInt(id),
         * intent, 0); am.set(AlarmManager.RTC_WAKEUP, cal.getTimeInMillis(),
         * pi);
         */

        mp = new MediaPlayer();
	if (intent != null) {
		rings = intent.getStringExtra(DBOpenHelper.CLOCK_URI);
		isVibrate = intent.getStringExtra(DBOpenHelper.CLOCK_ISVIBRATE);
		Log.v("you", "Alarmservice:*************01**************isVibrate="
		        + isVibrate);
	}

        //if (isVibrate.equals(getResources().getString(R.string.yes))) {
        //   v.vibrate(new long[] { 1000, 3000, 500, 2000 }, 2);
        if (isVibrate.equals("1")) {
            v.vibrate(new long[] { 1000, 2000 }, 0);
        } else {
                if (v != null) {
                    v.cancel();
                }
	}	
        // ContentValues cv = new ContentValues();
        // if(mp!=null) return;

        // File file = new File(text);
        try {
            // if(rings.equals("")&&!file.exists()){
            if (rings.equals("")) {
                mp.setDataSource(this, RingtoneManager
                        .getDefaultUri(RingtoneManager.TYPE_ALARM));
            } else {
                /*
                 * cv.put(MediaStore.MediaColumns.DATA, file.getAbsolutePath());
                 * cv.put(MediaStore.MediaColumns.TITLE, file.getName());
                 * cv.put(MediaStore.MediaColumns.SIZE, file.length());
                 * cv.put(MediaStore.MediaColumns.MIME_TYPE, "audio/mp3");
                 * cv.put(MediaStore.Audio.Media.IS_RINGTONE, false);
                 * cv.put(MediaStore.Audio.Media.IS_NOTIFICATION, false);
                 * cv.put(MediaStore.Audio.Media.IS_ALARM, true);
                 * cv.put(MediaStore.Audio.Media.IS_MUSIC, false); 
                 * Uri uri = MediaStore
                 * .Audio.Media.getContentUriForPath(file.getAbsolutePath());
                 * Uri newUri = this.getContentResolver().insert(uri, cv);
                 * RingtoneManager.setActualDefaultRingtoneUri(this,
                 * RingtoneManager.TYPE_ALARM, newUri);
                 */
                mp.setDataSource(AlarmService.this, Uri.parse(rings));
            }

            if (adm.getStreamVolume(AudioManager.STREAM_ALARM) != 0) {
                // 设置声音播放通道
                mp.setAudioStreamType(AudioManager.STREAM_ALARM);
                mp.setLooping(true);
                mp.prepare();
                // mp.setVolume(0.2F, 0.2F);
                Log.v("you", "AlarmAlert:***************************mp.start");
                mp.start();
            }

        } catch (IOException e1) {
            e1.printStackTrace();
        } catch (IllegalStateException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

    }

    private BroadcastReceiver receiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            action = intent.getAction();

            if (action != null && action.equals("com.sprdroid.note.STOP_ALARM")) {
                Log.v("you", "AlarmAlert:*************closeMediaPlayer():time="
                        + System.currentTimeMillis());

                if (mp != null && mp.isPlaying()) {

                    mp.stop();
                    mp.release();
                    mp = null;
                }// 判断震动的状态 //v.hasVibrator()

                if (v != null) {
                    v.cancel();
                }
            }

        }
    };

    public void onDestroy() {

        try {
            if (mp != null && mp.isPlaying()) {
                Log.v("you", "closeMediaPlayer()**************************");
                mp.stop();
                mp.release();
                mp = null;
                // mp.setAudioStreamType(AudioManager.USE_DEFAULT_STREAM_TYPE);
                if (v != null) {
                    v.cancel();
                }
            }

        } catch (IllegalStateException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IllegalArgumentException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        unregisterReceiver(receiver);
        super.onDestroy();
    }

}
