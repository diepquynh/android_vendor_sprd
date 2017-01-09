package com.sprd.engineermode;



import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.widget.Toast;
import com.android.internal.telephony.TelephonyIntents;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;

public class LteBroadcastReceiver extends BroadcastReceiver{

    private static final String TAG = "LteBroadcastReceiver";
    @Override
    public void onReceive(Context context, Intent intent) {
         SharedPreferences pref = PreferenceManager.getDefaultSharedPreferences(context);
         Editor editor = pref.edit();
         boolean  isLTEReady = intent.getBooleanExtra("lte", false);
         editor.putBoolean("LTEReady", isLTEReady);
         editor.commit();
    }
}

