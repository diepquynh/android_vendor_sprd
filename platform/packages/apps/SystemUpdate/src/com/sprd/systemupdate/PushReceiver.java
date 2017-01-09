package com.sprd.systemupdate;

import android.util.Log;
import android.net.NetworkInfo;
import android.content.Context;
import android.content.Intent;
import android.content.BroadcastReceiver;
import android.net.ConnectivityManager;
import android.os.SystemProperties;

public class PushReceiver extends BroadcastReceiver {

    private static final String TAG = "SystemUpdate--PushReceiver";

    @Override
    public void onReceive(Context context, Intent outerIntent) {
        String action = outerIntent.getAction();
        Log.i(TAG, "onReceive:" + action);
        if (action != null && (ConnectivityManager.CONNECTIVITY_ACTION).equals(action)) {
            ConnectivityManager cm = (ConnectivityManager) context
                    .getSystemService(Context.CONNECTIVITY_SERVICE);
            NetworkInfo info = cm.getActiveNetworkInfo();
            /** BEGIN BUG565617 zhijie.yang 2016/05/30 **/
            boolean isAutoPush = (SystemProperties.getInt("persist.sys.autopush.enable",0) == 1);
            Log.d(TAG,"get the value of autopush is: " + isAutoPush);
            Log.i(TAG, "info:" + info);
            if (info == null) {
                Log.i(TAG, "push down");
                if (isAutoPush) {
                    Intent intent = new Intent(context, PushService.class);
                    intent.putExtra(PushService.KEY_MODE,
                            PushService.MODE_NETWORK_DOWN);
                    context.startService(intent);
                }
            } else if (info != null && info.isConnected()) {
                Log.i(TAG, "push up");
                if (isAutoPush) {
                    Intent intent = new Intent(context, PushService.class);
                    intent.putExtra(PushService.KEY_MODE,
                            PushService.MODE_NETWORK_UP);
                    context.startService(intent);
                }
            }
            /** END BUG565617 zhijie.yang 2016/05/30 **/
        }
    }
}
