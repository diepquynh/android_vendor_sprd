package com.sprdroid.note;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.IntentFilter;
import android.text.format.DateFormat;
import android.widget.Toast;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.os.Bundle;
import java.util.Calendar;
import android.util.Log;

import com.sprdroid.note.*;

//import android.os.SystemProperties;

public class CallAlarm extends BroadcastReceiver {

    private DBOperations dbo;
    private Context mContext;
    private static final String DEFAULT_SNOOZE = "10";
    int clock_id;

    @Override
    public void onReceive(Context context, Intent intent) {
        mContext = context;

        // 接受其他闹钟事件，电话事件，短信事件等，进行交互处理
        String action = intent.getAction();
        /*
         * if (action.equals(Alarms.ALARM_SNOOZE_ACTION)) { snooze(); } else if
         * (action.equals(Alarms.ALARM_DISMISS_ACTION)) { dismiss(false); } else
         * if (action!= null &&
         * action.equals("android.intent.action.PHONE_STATE")){
         * //saveSnoozeAlert(context, -1, -1); } else
         */
        if (action != null
                && action.equals("android.intent.action.PHONE_STATE")) {
            Log.v("you", "onReceive:action.PHONE_STATE");
            snooze();
        } else if (action != null
                && action.equals("android.provider.Telephony.SMS_RECEIVED")) {
            Log.v("you", "onReceive:Telephony.SMS_RECEIVED");
            snooze();
        } else {

            dbo = new DBOperations();
            clock_id = intent.getIntExtra(DBOpenHelper.CLOCK_ID, 0);
            // 查询数据库闹钟信息，转发给AlarmAlert
            Cursor cursor1 = dbo.getClock(context, clock_id);
            if (!cursor1.moveToFirst())
                return;
            String c_id = Integer.toString(clock_id);
            // String text = cursor.getString(cursor.getColumnIndex(DBOpenHelper.FIELD_CONTENT));
            String uri = cursor1.getString(cursor1
                    .getColumnIndex(DBOpenHelper.CLOCK_URI));
            String isVibrate = cursor1.getString(cursor1
                    .getColumnIndex(DBOpenHelper.CLOCK_ISVIBRATE));
            String date = cursor1.getString(cursor1
                    .getColumnIndex(DBOpenHelper.CLOCK_DATE));
            String time = cursor1.getString(cursor1
                    .getColumnIndex(DBOpenHelper.CLOCK_TIME));
            cursor1.close();
            //dbo.close();

            if (date != null && !date.equals("")) {
                // if(!date.equals(Util.getDateString()))return;
            }
            Intent i = new Intent(context, AlarmAlert.class);
            i.putExtra(DBOpenHelper.CLOCK_ID, c_id);
            // i.putExtra(DBOper.FIELD_CONTENT, text);
            i.putExtra(DBOpenHelper.CLOCK_ISVIBRATE, isVibrate);
            i.putExtra(DBOpenHelper.CLOCK_URI, uri);
            i.putExtra(DBOpenHelper.CLOCK_TIME, time);
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(i);
        }
    }

    // Attempt to snooze this alert.
    private void snooze() {
        // Do not snooze if the snooze button is disabled.

        final String snooze = DEFAULT_SNOOZE;
        int snoozeMinutes = Integer.parseInt(snooze);
        final long snoozeTime = System.currentTimeMillis()
                + (1000 * 60 * snoozeMinutes);

        // Get the display time for the snooze and update the notification.
        final Calendar c = Calendar.getInstance();
        c.setTimeInMillis(snoozeTime);
        // Append (snoozed) to the label.
        String label = mContext.getResources()
                .getString(R.string.default_label);
        label = mContext.getResources().getString(
                R.string.alarm_notify_snooze_label, label);

        // Notify the user that the alarm has been snoozed.
        Intent cancelSnooze = new Intent();
        cancelSnooze.setAction("com.sprdroid.note.STOP_ALARM");
        cancelSnooze.putExtra("alarm_id", clock_id);
        mContext.sendBroadcast(cancelSnooze);

        PendingIntent broadcast = PendingIntent.getBroadcast(mContext,
                clock_id, cancelSnooze, 0);
        // NotificationManager nm = (NotificationManager)
        // getNotificationManager();
        Notification n = new Notification(R.drawable.stat_notify_alarm, label,
                0);
        n.setLatestEventInfo(
                mContext,
                label,
                mContext.getResources().getString(
                        R.string.alarm_notify_snooze_text,
                        (String) DateFormat.format("kk:mm", c)), broadcast);
        n.flags |= Notification.FLAG_AUTO_CANCEL
                | Notification.FLAG_ONGOING_EVENT;
        // nm.notify(clock_id, n);
        /*
         * String displayTime =
         * mContext.getResources().getString(R.string.alarm_alert_snooze_set,
         * snoozeMinutes); // Display the snooze minutes in a toast.
         * Toast.makeText(mContext, displayTime, Toast.LENGTH_LONG).show();
         */
        // finish();
    }
    /*
     * private NotificationManager getNotificationManager() { return
     * (NotificationManager) getSystemService(NOTIFICATION_SERVICE); }
     */
    /*
     * // Dismiss the alarm. 
     * private void dismiss(boolean killed) { 
     * Log.v("you",killed ? "Alarm killed" : "Alarm dismissed by user"); 
     * The service told us
     * that the alarm has been killed, do not modify the notification or stop
     * the service. if (!killed) { Cancel the notification and stop playing the
     * alarm NotificationManager nm = getNotificationManager();
     * nm.cancel(clock_id);
     * 
     * stopService(new Intent("com.sprdroid.note.AlarmAlert")); } finish(); }
     */

}
