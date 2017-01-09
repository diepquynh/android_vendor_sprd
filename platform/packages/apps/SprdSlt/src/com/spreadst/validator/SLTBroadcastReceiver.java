package com.spreadst.validator;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

public class SLTBroadcastReceiver extends BroadcastReceiver {
    public SLTBroadcastReceiver() {

    }

    @Override
    public void onReceive(Context context, Intent intent) {

        if ("".equals(intent) || intent == null) {
            return;
        }
        Uri uri = intent.getData();

        if ("".equals(uri) || uri == null) {
            return;
        }
        String host = uri.getHost();

        Log.d("onReceive", "uri=" + uri);

        if ("".equals(host) || host == null) {
            return;
        }

        if ("6699".equals(host)) {
            Intent i = new Intent(Intent.ACTION_MAIN);
            i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            i.setClass(context, UIDialog.class);
            context.startActivity(i);
        }
    }
}
