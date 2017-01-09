package com.sprd.systemupdate;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;
import java.lang.String;

import com.android.internal.telephony.TelephonyIntents;
import android.os.SystemProperties;

public class SysUpStartReceiver extends BroadcastReceiver {

    private static final String TAG = "SysUpStartReceiver";
    
    public SysUpStartReceiver() {
    }
    @Override
    public void onReceive(Context context, Intent intent) {
        Uri uri = intent.getData();
        String host = uri.getHost();

        Log.d(TAG, "uri=" + uri);
        Intent i = new Intent(Intent.ACTION_MAIN);
        i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

        if ("83782".equals(host)) {
            i.setClass(context, SystemUpdateActivity.class);
            context.startActivity(i);
        }
    }
}
