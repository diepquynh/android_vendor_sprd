package com.sprd.generalsecurity.network;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ContentValues;
import android.content.Intent;

import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import java.lang.Override;
import java.lang.String;
import java.lang.System;

import android.os.Bundle;
import android.util.Log;

import android.preference.PreferenceManager;
import android.content.SharedPreferences;


public class NetworkStateReceiver extends BroadcastReceiver {
    private static final String TAG = "NetworkStateReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {

        ConnectivityManager cm = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo activeNetwork = cm.getActiveNetworkInfo();
        Log.e(TAG, "receiver:" + activeNetwork + ":" + intent + "\n time:" + System.currentTimeMillis());
        Intent it = new Intent(context, DataFlowService.class);
        context.startService(it);

        //stop real speed view if network disconnected.
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(context);
        if (sharedPref.getBoolean("networkspeed_switch", false)) {
            realSpeedSetting(activeNetwork, context);
        }
    }

    private void realSpeedSetting(NetworkInfo info, Context context) {
        if (info != null && info.isConnectedOrConnecting()) {
            FloatKeyView.getInstance(context).startRealSpeed();
        } else {
            FloatKeyView.getInstance(context).stopRealSpeed();
            Log.e(TAG, "stopped real speed");
        }
    }
}