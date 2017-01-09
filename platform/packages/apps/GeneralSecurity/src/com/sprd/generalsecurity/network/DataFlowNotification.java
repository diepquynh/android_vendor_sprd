package com.sprd.generalsecurity.network;

import android.content.Context;
import android.content.res.Resources;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationManagerCompat;
import com.sprd.generalsecurity.R;
public class DataFlowNotification{
    public static void showDataUseNotification(Context context, int type,long useFlow) {
//        LogUtils.v("Displaying alarm notification for alarm instance: " + instance.mId);
        NotificationManagerCompat nm = NotificationManagerCompat.from(context);

        Resources resources = context.getResources();
        NotificationCompat.Builder notification = new NotificationCompat.Builder(context)
                .setContentTitle("111")
                .setContentText("aaa")
                .setSmallIcon(R.drawable.lock_screen_flow_icon)
                .setOngoing(true)
                .setAutoCancel(false)
                .setDefaults(NotificationCompat.DEFAULT_LIGHTS)
                .setWhen(0)
                .setCategory(NotificationCompat.CATEGORY_EVENT)
                .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
                .setLocalOnly(true);

        // Setup Snooze Action
//        Intent snoozeIntent = AlarmStateManager.createStateChangeIntent(context,
//                AlarmStateManager.ALARM_SNOOZE_TAG, instance, AlarmInstance.SNOOZE_STATE);
//        snoozeIntent.putExtra(AlarmStateManager.FROM_NOTIFICATION_EXTRA, true);
//        PendingIntent snoozePendingIntent = PendingIntent.getBroadcast(context, instance.hashCode(),
//                snoozeIntent,
//                PendingIntent.FLAG_UPDATE_CURRENT);
//        notification.addAction(R.drawable.ic_snooze_24dp,
//                resources.getString(R.string.alarm_alert_snooze_text), snoozePendingIntent);

        // Setup Dismiss Action
//        Intent dismissIntent = AlarmStateManager.createStateChangeIntent(context,
//                AlarmStateManager.ALARM_DISMISS_TAG, instance, AlarmInstance.DISMISSED_STATE);
//        dismissIntent.putExtra(AlarmStateManager.FROM_NOTIFICATION_EXTRA, true);
//        PendingIntent dismissPendingIntent = PendingIntent.getBroadcast(context,
//                instance.hashCode(), dismissIntent, PendingIntent.FLAG_UPDATE_CURRENT);
//        notification.addAction(R.drawable.ic_alarm_off_24dp,
//                resources.getString(R.string.alarm_alert_dismiss_text),
//                dismissPendingIntent);
    }
}
