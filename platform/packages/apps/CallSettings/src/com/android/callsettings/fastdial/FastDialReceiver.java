package com.android.callsettings.fastdial;

import android.appwidget.AppWidgetManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.text.TextUtils;
import android.util.Log;

import com.android.internal.telephony.TelephonyIntents;

public class FastDialReceiver extends BroadcastReceiver {

    private static final String TAG = "FastDialReceiver";
    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        FastDialManager fastDialmanager = FastDialManager.getInstance();
        Log.d(TAG, "action:" + action);
        if ("android.intent.action.BOOT_COMPLETED".equals(action)) {
            FastDialManager.init(context);
        } else if (AppWidgetManager.ACTION_APPWIDGET_UPDATE.equals(action)) {
            if (fastDialmanager != null) {
                fastDialmanager.onAppWidgetUpdate();
            }
        } else if (!TextUtils.isEmpty(action)
                && (action).startsWith(TelephonyIntents.ACTION_SIM_STATE_CHANGED)) {
            if (fastDialmanager != null) {
                fastDialmanager.onSimStateChanged(intent);
            }
        } else if (Intent.ACTION_LOCALE_CHANGED.equals(action)) {
            if (fastDialmanager != null) {
                fastDialmanager.onLocaleChanged();
            }
        }
    }
}
