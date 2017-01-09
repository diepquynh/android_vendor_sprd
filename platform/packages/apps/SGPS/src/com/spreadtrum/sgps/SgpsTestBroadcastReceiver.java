
package com.spreadtrum.sgps;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

public class SgpsTestBroadcastReceiver extends BroadcastReceiver {

    private static final String TAG = SgpsTestBroadcastReceiver.class.getSimpleName();

    public SgpsTestBroadcastReceiver() {
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        Uri uri = intent.getData();
        Log.d(TAG, "onReceive uri = " + uri);
        String host = null;

        if (uri != null) {
            host = uri.getHost();
        }

        if ("2266".equals(host)) {
            Intent i = new Intent(Intent.ACTION_MAIN);
            i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            i.setClass(context, SgpsActivity.class);
            context.startActivity(i);
        } else {
            Log.d(TAG, "can not start SgpsActivity because host wrong !");
        }
    }
}
