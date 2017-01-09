package com.sprd.generalsecurity.network;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import com.sprd.generalsecurity.utils.Contract;

import android.os.Bundle;
import com.sprd.generalsecurity.utils.TeleUtils;

import android.content.SharedPreferences;
import android.preference.PreferenceManager;


public class BootReceiver extends BroadcastReceiver {
    private FloatKeyView mFloatKeyView;
    private static final String TAG = "BootReceiverGS";

    @Override
    public void onReceive(Context context, Intent intent) {
        SharedPreferences sharedPref = PreferenceManager.getDefaultSharedPreferences(context);
        if (sharedPref.getBoolean("networkspeed_switch", false)) {
            Log.d(TAG, "switched on");
            mFloatKeyView = FloatKeyView.getInstance(context);
            mFloatKeyView.addToWindow();
            Log.d(TAG, "intent:" + mFloatKeyView.isShown());
        } else {
            Log.d(TAG, "switch false");
        }

        if (sharedPref.getBoolean("keyguard_data_switch", false) || sharedPref.getBoolean("networkspeed_switch", false)) {
            Log.d(TAG, "keyguard_data_switch on:" );
            Intent it = new Intent(context, ScreenStateService.class);
            context.startService(it);
        } else {
            Log.d(TAG, "keyguard_data_switch false");
        }
    }
}