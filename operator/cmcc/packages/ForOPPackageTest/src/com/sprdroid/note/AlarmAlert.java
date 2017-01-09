package com.sprdroid.note;

import java.io.File;
import java.io.IOException;
import android.content.BroadcastReceiver;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.database.Cursor;

import android.net.Uri;
import android.os.Bundle;
import android.os.Vibrator;
import android.os.Handler;
import android.provider.MediaStore;
import android.widget.TextView;
import android.widget.Toast;
import android.text.format.DateFormat;
import android.util.Log;
import android.widget.Button;
import android.view.View;
import android.view.Window;
import android.view.View.OnClickListener;

import java.sql.Time;
import java.util.Calendar;
import java.util.Timer;
import java.util.TimerTask;
import android.app.KeyguardManager;
import android.app.KeyguardManager.KeyguardLock;
import android.os.PowerManager;

import android.media.AudioManager;

public class AlarmAlert extends Activity {
    public static final String STOP_ALARM = "com.sprdroid.note.STOP_ALARM";
    public static final String ALARM_DONE = "com.android.deskclock.ALARM_DONE";

    // protected CallAlarm receiver;
    int TIMEOUT = 60;// 闹铃响时长（s）
    //int[] _id;
    private int _id;
    private String action;
    private DBOperations dbo;

    private long mStartTime;
    private Handler mHandler = new Handler();
    private final Timer timer = new Timer();
    // private TimerTask task;
    // private Handler handler;
    private NotificationManager nm;
    private int Notification_ID = 100000010;

    private PowerManager pm;
    private PowerManager.WakeLock wakeLock;
    // 声明键盘管理器
    KeyguardManager mKeyguardManager = null;
    // 声明键盘锁
    private KeyguardLock mKeyguardLock = null;

    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        //requestWindowFeature(Window.FEATURE_NO_TITLE);
        requestWindowFeature(Window.FEATURE_CUSTOM_TITLE);    
        setContentView(R.layout.dialog_alarm_alert);
        getWindow().setFeatureInt(Window.FEATURE_CUSTOM_TITLE,      
                R.layout.dialog_alarm_alert_title);  
        
        //设置title右边为闹铃时间
        Context mContext = AlarmAlert.this;
        TextView alarm_time = (TextView) findViewById(R.id.title_right);
        Calendar clnd = Calendar.getInstance();
        int theHour = clnd.get(Calendar.HOUR_OF_DAY);
        int theMinute = clnd.get(Calendar.MINUTE);
        int thesecond = clnd.get(Calendar.SECOND);
        alarm_time.setText(new Time(theHour, theMinute, thesecond).toString());
        
        TextView content = (TextView) findViewById(R.id.tv_note_content);
        Button cancelBtn = (Button) findViewById(R.id.cancel_alarm);
        cancelBtn.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                closeMediaPlayer();
		//使通知图标消失
		if(nm != null) {
        		nm.cancel(Notification_ID);
		}
            }
        });

        IntentFilter filter = new IntentFilter();
        filter.addAction("com.sprdroid.note.STOP_ALARM");
        // 防止在灭屏时不能接受停止闹钟的广播，所以用静态注册。
        this.registerReceiver(this.receiver, filter);
        dbo = new DBOperations();

        // 获取电源的服务
        pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        // 获取系统服务
        mKeyguardManager = (KeyguardManager) getSystemService(Context.KEYGUARD_SERVICE);

        Intent intent = getIntent();
        // String id = intent.getStringExtra(DBOpenHelper.CLOCK_ID);
        // _id = Integer.parseInt(id);
        String c_id = intent.getStringExtra(DBOpenHelper.CLOCK_ID);
        Log.i("you", "alarmalert: c_id=" + c_id);
        String text = intent.getStringExtra(DBOpenHelper.CONTENT);
        String isVibrate = intent.getStringExtra(DBOpenHelper.CLOCK_ISVIBRATE);
        String rings = intent.getStringExtra(DBOpenHelper.CLOCK_URI);
        String time = intent.getStringExtra(DBOpenHelper.CLOCK_TIME);

        // 显示便签内容
        _id = Integer.parseInt(c_id);
        Cursor c = dbo.queryOneNote(this, _id);
        c.moveToFirst();
        startManagingCursor(c);

        String cnt = c.getString(c.getColumnIndex(DBOpenHelper.CONTENT));
        if (cnt != null && cnt.length() > 25) {// 如果内容太长,则采用如下方式显示
            cnt = cnt.substring(0, 25) + "...";
        }
        content.setText(cnt);

        Log.i("you", "pm.isScreenOn()=" + pm.isScreenOn());
        Log.i("you", "mKeyguardManager.inKeyguardRestrictedInputMode()="
                + mKeyguardManager.inKeyguardRestrictedInputMode());
        if (!pm.isScreenOn()
                && mKeyguardManager.inKeyguardRestrictedInputMode()) {
            // 点亮屏
            wakeLock = pm.newWakeLock(PowerManager.ACQUIRE_CAUSES_WAKEUP
                    | PowerManager.SCREEN_DIM_WAKE_LOCK, "My Tag");
            wakeLock.acquire();
            Log.i("you", "------>mKeyguardLock");
            // 初始化键盘锁，可以锁定或解开键盘锁
            mKeyguardLock = mKeyguardManager.newKeyguardLock("");
            // 禁用显示键盘锁定
            mKeyguardLock.disableKeyguard();
        }

        /*
         * new AlertDialog.Builder(this).setTitle(time)
         * .setMessage(R.string.default_label)
         * .setNegativeButton(R.string.Cancel, new
         * DialogInterface.OnClickListener(){
         * @Override public void onClick(DialogInterface dialog, int which) {
         * closeMediaPlayer();
         * } }).show();
         */
        // NotificationManager nm = (NotificationManager)getNotificationManager();
        nm = (NotificationManager) getSystemService(
                android.content.Context.NOTIFICATION_SERVICE);
        Notification n = new Notification();
        // 设置显示图标，该图标会在状态栏显示
        int icon = R.drawable.clock;
        // 设置显示提示信息，该信息也会在状态栏显示
        String tickerText = AlarmAlert.this.getResources().getString(
                R.string.notify_ticker);
        // 显示时间
        long when = System.currentTimeMillis();
        String notifyTitle = this.getResources().getString(R.string.Note_Alarm_Notify);
        
        n.icon = icon;
        n.tickerText = tickerText;
        n.when = when;
        n.defaults = Notification.DEFAULT_SOUND;
        n.defaults |= Notification.DEFAULT_VIBRATE;

        final Calendar cal = Calendar.getInstance();
        cal.setTimeInMillis(System.currentTimeMillis() + (1000 * 60));
        Intent intent1 = new Intent();
        // this.stopService(new Intent(this,AlarmService.class));
        intent1.setAction(STOP_ALARM); // 如果在非AlarmAlert界面，则不能接受处理广播。
        // intent1.putExtra(DBOpenHelper.CLOCK_ISVIBRATE, isVibrate);

        PendingIntent broadcast = PendingIntent.getBroadcast(mContext, 0,
                intent1, 0);
        n.setLatestEventInfo(
                mContext,
                notifyTitle,
                mContext.getResources().getString(
                        R.string.alarm_notify_snooze_text,
                        (String) DateFormat.format("kk:mm", cal)), broadcast);
        n.flags |= Notification.FLAG_AUTO_CANCEL
                | Notification.FLAG_ONGOING_EVENT;

        Log.v("you", "AlarmAlert:***************************notify()");
        nm.notify(Notification_ID, n);

        // 通知背景播放音乐mp3暂停，这个action被mp3注册
        Intent alarm_alert = new Intent("com.android.deskclock.ALARM_ALERT");
        this.sendBroadcast(alarm_alert);

        // 调用服务，播放闹铃
        // startService(new Intent ("com.sprdroid.note.AlarmService"));
        Intent intent2 = new Intent(this, AlarmService.class);
        intent2.putExtra(DBOpenHelper.CLOCK_URI, rings);
        intent2.putExtra(DBOpenHelper.CLOCK_ISVIBRATE, isVibrate);
        startService(intent2);
        // 音量控制
        setVolumeControlStream(AudioManager.STREAM_ALARM);
        mStartTime = System.currentTimeMillis();
        Log.v("you", "AlarmAlert:***************************mStartTime="
                + mStartTime);

        // timer.cancel();
        timer.schedule(new TimerTask() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                // handler.post(runnable);
                Log.v("you", "AlarmAlert:*******02**********run():time="
                        + System.currentTimeMillis());
                // sendBroadcast(new Intent(STOP_ALARM));
                closeMediaPlayer();

            }
        }, TIMEOUT * 1000);
        /*
         * //handler= new Handler(); //handler.removeCallbacks(runnable);
         * //只允许闹钟响60秒 //handler.postDelayed(new Runnable(){
         * 
         * @Override public void run() {
         * Log.v("you","AlarmAlert:*******01**********run():time=" +
         * System.currentTimeMillis()); sendBroadcast(new Intent(STOP_ALARM)); }
         * }, TIMEOUT*1000);
         */

    }

    /*
     * public static Handler mHandler = new Handler(); 
     * public static void closeMedia() { mHandler.sendEmptyMessage(what); }
     */

    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        super.onResume();

    }

    @Override
    protected void onPause() {
        // TODO Auto-generated method stub
        super.onPause();

    }

    @Override
    public void onDestroy() {
        if (wakeLock != null && mKeyguardLock != null) {
            wakeLock.release();
            mKeyguardLock.reenableKeyguard();
        }
        unregisterReceiver(receiver);
        timer.cancel();

        //dbo.close();
        super.onDestroy();
    }

    // 用于接受广播，停止震动
    private BroadcastReceiver receiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            action = intent.getAction();

            if (action != null && action.equals("com.sprdroid.note.STOP_ALARM")) {
                // TODO

            }
        }
    };

    public void closeMediaPlayer() {
        // 停止mp服务
        // this.stopService(new Intent(this,AlarmService.class));
        Intent intent3 = new Intent(this, AlarmService.class);
        // intent3.setAction("com.sprdroid.note.STOP_ALARM");
        Log.v("you", "AlarmAlert:****************stopService(intent3)");
        stopService(intent3);

        // 恢复音量控制状态
        setVolumeControlStream(AudioManager.USE_DEFAULT_STREAM_TYPE);

        // 通知背景播放音乐mp3再次启动
        Intent alarm_done = new Intent(ALARM_DONE);
        this.sendBroadcast(alarm_done);

        mHandler.post(mDisplayToast);
	//修改鬧鈴数据库，使得“开启鬧鈴”状态为否
        dbo.update_alarm_enable(this, _id, 0);

        this.finish();

    }

    Runnable mDisplayToast = new Runnable() {
        @Override
        public void run() {
            // TODO Auto-generated method stub
            String displayTime = AlarmAlert.this.getResources().getString(
                    R.string.alarm_alert_snooze_set);
            Toast.makeText(AlarmAlert.this, displayTime, Toast.LENGTH_LONG)
                    .show();
        }

    };

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            // mp.stop();
        }
        super.onConfigurationChanged(newConfig);
    }

}
