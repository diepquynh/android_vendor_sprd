package com.sprd.engineermode.telephony;

import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.os.SystemProperties;

public class NotificationBroadcastReceiver extends BroadcastReceiver {
    private static final String TAG = "NotificationBroadcastReceiver";
    private int NOTIFICATION_ID = 0x1123;
    @Override
    public void onReceive(Context context, Intent intent) {
        int type = intent.getIntExtra("notificationId", -1);
        Log.d(TAG, "type is"+type);
        NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        if(type == NOTIFICATION_ID){
            Log.d(TAG, "run here");
            SystemProperties.set("persist.radio.engtest.enable", "false");
            Log.d(TAG, "get vSystemProperties is " +SystemProperties.get("persist.radio.engtest.enable"));
            notificationManager.cancel(NOTIFICATION_ID);
        }
    }

}
